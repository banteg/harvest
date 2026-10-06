#include "audio/Mixer.h"
#include <SDL3/SDL_log.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace port {
namespace audio {

namespace {

const float PI = 3.14159265358979f;

//! OpenAL's defaults, which the original never changes: reference distance 1, no maximum distance,
//! outer cone gain 0, gains clamped to [0, 1].
const float REFERENCE_DISTANCE = 1.0f;
const float MAX_DISTANCE = 3.40282347e+38f;
const float CONE_OUTER_GAIN = 0.0f;
const float MAX_GAIN = 1.0f;

float clampf(float value, float low, float high)
{
    return std::min(std::max(value, low), high);
}

//! OpenAL's distance attenuation (the 1.1 specification's formulas, as OpenAL Soft computes them).
float distanceAttenuation(int model, float distance, float rolloff)
{
    const float ref = REFERENCE_DISTANCE;
    const float max = MAX_DISTANCE;
    switch (model)
    {
    case Mixer::INVERSE_DISTANCE_CLAMPED:
        distance = clampf(distance, ref, max);
        // fall through
    case Mixer::INVERSE_DISTANCE:
    {
        float denominator = ref + rolloff * (distance - ref);
        return denominator > 0.0f ? ref / denominator : 1.0f;
    }
    case Mixer::LINEAR_DISTANCE_CLAMPED:
        distance = clampf(distance, ref, max);
        // fall through
    case Mixer::LINEAR_DISTANCE:
        return std::max(1.0f - rolloff * (std::min(distance, max) - ref) / (max - ref), 0.0f);
    case Mixer::EXPONENT_DISTANCE_CLAMPED:
        distance = clampf(distance, ref, max);
        // fall through
    case Mixer::EXPONENT_DISTANCE:
        return distance > 0.0f ? std::pow(distance / ref, -rolloff) : 1.0f;
    default:
        return 1.0f;
    }
}

//! OpenAL's sound cone: full gain inside the inner cone, the outer gain outside the outer cone,
//! linear in between. Angles are the cones' full apex angles, as in the OpenAL 1.1 specification.
float coneGain(const Vec3& direction, const Vec3& toSource, float inner, float outer)
{
    float cosine = clampf(-direction.dotProduct(toSource), -1.0f, 1.0f);
    float angle = 2.0f * std::acos(cosine) * 180.0f / PI;
    if (angle <= inner)
        return 1.0f;
    if (angle < outer)
        return 1.0f + (CONE_OUTER_GAIN - 1.0f) * (angle - inner) / (outer - inner);
    return CONE_OUTER_GAIN;
}

} // end namespace

Source::Source()
    : Data(0), Queue(0), Playing(false), Looping(false), Relative(false), Cursor(0.0), Offset(0.0), Gain(1.0f),
      Pitch(1.0f), Rolloff(1.0f), ConeInner(360.0f), ConeOuter(360.0f)
{
}

void Source::play()
{
    Cursor = 0.0;
    if (Data && Offset > 0.0)
    {
        double frame = Offset * Data->Rate;
        if (frame < (double)Data->Frames)
            Cursor = frame;
    }
    Offset = 0.0;
    Playing = true;
}

void Source::setGain(float gain)
{
    if (gain >= 0.0f)
        Gain = gain;
}

void Source::setPitch(float pitch)
{
    if (pitch >= 0.0f)
        Pitch = pitch;
}

void Source::setOffset(float seconds)
{
    if (seconds >= 0.0f)
        Offset = seconds;
}

Mixer::Mixer()
    : ListenerForward(0.0f, 0.0f, -1.0f), ListenerUp(0.0f, 1.0f, 0.0f), DeviceOpen(false), SampleRate(0), Lock(0),
      DistanceModel(INVERSE_DISTANCE_CLAMPED)
{
}

Mixer::~Mixer()
{
    close();
}

bool Mixer::openDevice()
{
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = 0;
    config.dataCallback = dataCallback;
    config.pUserData = this;
    ma_result result = ma_device_init(0, &config, &Device);
    if (result != MA_SUCCESS)
    {
        SDL_Log("cannot open an audio device: %s", ma_result_description(result));
        return false;
    }
    result = ma_device_start(&Device);
    if (result != MA_SUCCESS)
    {
        SDL_Log("cannot start the %s audio device: %s", ma_get_backend_name(Device.pContext->backend),
            ma_result_description(result));
        ma_device_uninit(&Device);
        return false;
    }
    DeviceOpen = true;
    SampleRate = Device.sampleRate;
    SDL_Log("audio: %s, %s, %u Hz", ma_get_backend_name(Device.pContext->backend), Device.playback.name,
        SampleRate);
    return true;
}

void Mixer::openOffline(unsigned int sampleRate)
{
    close();
    SampleRate = sampleRate;
}

void Mixer::close()
{
    if (DeviceOpen)
    {
        ma_device_uninit(&Device);
        DeviceOpen = false;
    }
    SampleRate = 0;
}

void Mixer::lock()
{
    ma_spinlock_lock(&Lock);
}

void Mixer::unlock()
{
    ma_spinlock_unlock(&Lock);
}

void Mixer::addSource(Source* source)
{
    Sources.push_back(source);
}

void Mixer::removeSource(Source* source)
{
    Sources.erase(std::remove(Sources.begin(), Sources.end(), source), Sources.end());
}

void Mixer::setDistanceModel(int model)
{
    if (model == DISTANCE_NONE || (model >= INVERSE_DISTANCE && model <= EXPONENT_DISTANCE_CLAMPED))
        DistanceModel = model;
}

void Mixer::dataCallback(ma_device* device, void* output, const void*, ma_uint32 frames)
{
    static_cast<Mixer*>(device->pUserData)->mix(static_cast<float*>(output), frames);
}

void Mixer::mix(float* output, ma_uint32 frames)
{
    std::memset(output, 0, sizeof(float) * 2 * frames);
    MixerLock lock(*this);
    for (size_t i = 0; i < Sources.size(); ++i)
    {
        if (Sources[i]->Playing)
            mixSource(*Sources[i], output, frames);
    }
}

unsigned int Mixer::getPlayingCount()
{
    MixerLock lock(*this);
    unsigned int count = 0;
    for (size_t i = 0; i < Sources.size(); ++i)
        count += Sources[i]->Playing ? 1 : 0;
    return count;
}

void Mixer::computeGains(const Source& source, unsigned int channels, float gains[2]) const
{
    // Multichannel sources play their channels straight to the speakers: no position, distance or
    // cone.
    if (channels != 1)
    {
        gains[0] = gains[1] = clampf(source.Gain, 0.0f, MAX_GAIN);
        return;
    }

    Vec3 position = source.Position;
    Vec3 direction = source.Direction;
    if (!source.Relative)
    {
        // Into listener space: x right, y up, z backwards.
        Vec3 forward = ListenerForward;
        forward.normalize();
        Vec3 right = forward.crossProduct(ListenerUp);
        right.normalize();
        Vec3 up = right.crossProduct(forward);
        Vec3 offset = position - ListenerPosition;
        position.set(offset.dotProduct(right), offset.dotProduct(up), -offset.dotProduct(forward));
        direction.set(direction.dotProduct(right), direction.dotProduct(up), -direction.dotProduct(forward));
    }

    float distance = (float)position.getLength();
    Vec3 toSource = distance > 0.0f ? position / distance : Vec3();
    float gain = source.Gain * distanceAttenuation(DistanceModel, distance, source.Rolloff);
    if (direction.getLengthSQ() > 0.0)
        gain *= coneGain(direction.normalize(), toSource, source.ConeInner, source.ConeOuter);
    gain = clampf(gain, 0.0f, MAX_GAIN);

    // OpenAL Soft's stereo panning: constant power between speakers at -90 and +90 degrees, by the
    // horizontal direction's lateral angle; a source above, below or on the listener blends towards
    // equal gains of sqrt(1/2).
    const float ambient = std::sqrt(0.5f);
    float horizontal = std::sqrt(toSource.X * toSource.X + toSource.Z * toSource.Z);
    float left = ambient;
    float right = ambient;
    if (horizontal > 0.0f)
    {
        float lateral = clampf(toSource.X / horizontal, -1.0f, 1.0f);
        float alpha = (std::asin(lateral) + PI * 0.5f) * 0.5f;
        left = ambient + (std::cos(alpha) - ambient) * horizontal;
        right = ambient + (std::sin(alpha) - ambient) * horizontal;
    }
    gains[0] = gain * left;
    gains[1] = gain * right;
}

void Mixer::mixSource(Source& source, float* output, ma_uint32 frames)
{
    const unsigned int channels = source.Data ? source.Data->Channels : source.Queue->Channels;
    const unsigned int rate = source.Data ? source.Data->Rate : source.Queue->Rate;
    float gains[2];
    computeGains(source, channels, gains);
    const double step = (double)source.Pitch * rate / SampleRate;
    // Mono feeds both speakers; otherwise the first two channels feed left and right.
    const unsigned int second = channels == 1 ? 0 : 1;

    for (ma_uint32 f = 0; f < frames; ++f)
    {
        ma_uint64 index = (ma_uint64)source.Cursor;
        ma_uint64 next = index + 1;
        const float* a;
        const float* b;
        if (source.Data)
        {
            const Buffer& data = *source.Data;
            if (index >= data.Frames)
            {
                source.Playing = false;
                return;
            }
            if (next >= data.Frames)
                next = source.Looping ? 0 : index;
            a = &data.Samples[index * channels];
            b = &data.Samples[next * channels];
        }
        else
        {
            const StreamQueue& queue = *source.Queue;
            if (next >= queue.Written)
            {
                if (!queue.Ended)
                    return; // waiting for the game thread to queue more
                if (index >= queue.Written)
                {
                    source.Playing = false;
                    return;
                }
                next = index;
            }
            a = &queue.Samples[(index % queue.Capacity) * channels];
            b = &queue.Samples[(next % queue.Capacity) * channels];
        }

        float t = (float)(source.Cursor - (double)index);
        output[f * 2] += (a[0] + (b[0] - a[0]) * t) * gains[0];
        output[f * 2 + 1] += (a[second] + (b[second] - a[second]) * t) * gains[1];

        source.Cursor += step;
        if (source.Data && source.Looping && source.Cursor >= (double)source.Data->Frames)
            source.Cursor = std::fmod(source.Cursor, (double)source.Data->Frames);
    }
}

} // end namespace audio
} // end namespace port
