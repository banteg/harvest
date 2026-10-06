// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
//
// Port notes (behaviour the OpenAL backend has):
// - Sound effects: "<sound effect path><name>". Names ending in ".ogg" are decoded completely into one
//   16-bit buffer (mono or stereo by channel count, at the file's sample rate); names ending in ".wav"
//   go through alutCreateBufferFromFile; any other extension fails to load. Every effect buffer is
//   kept until the driver is destroyed.
// - Effects play on a pool of 32 sources (NUM_SOURCES) searched round-robin. A source is free when it
//   is not AL_PLAYING; while searching, a source whose recorded priority is below the current sound
//   priority (setSoundPriority) is stopped first, so it becomes free. When no source is free the
//   sound is dropped. A source records the sound priority it was started with.
// - Plain effects are listener-relative at (2 * pan, 0, 0.1) with rolloff 1 and no cone; looping
//   effects use the same position but the global rolloff factor and the oriented-sound cone;
//   oriented (3d) and tracked sounds are absolute, with the global rolloff factor and the cone.
// - Gain is the volume passed in, unscaled (AL_GAIN = volume) for effects, music and voice alike.
//   Pitch is the requested pitch times the global pitch modifier; setGlobalPitchModifier re-applies
//   it to every playing pool source and tracked sound, and sets it on the music when the modifier is
//   enabled for music. deviceDampenAllSounds does nothing here, so a voice line only dampens the
//   effects started while it plays (CAudioDriver scales their volume).
// - The `fade` argument of loopSound and startTrackedSound is an AL_SEC_OFFSET: where playback starts.
// - The distances are OpenAL's defaults (reference distance 1, no maximum); the distance model is
//   OpenAL's default (inverse clamped) unless setOpenAlDistanceModel is called.
// - Music and voice lines are streamed from Ogg Vorbis files, each on its own source: 10 queued
//   buffers of up to 4096 bytes of 16-bit signed little-endian PCM (ov_read word size 2, signed,
//   little-endian), mono or stereo by channel count, at the file's rate. The source is listener-
//   relative at the origin with rolloff 0, never AL_LOOPING. periodicStreamUpdate (every frame)
//   refills each processed buffer, restarts a source that ran dry, and sets the music pitch (the
//   global modifier when enabled for music, else 1). Looping is done by the stream: when a piece
//   finds the file at its end before decoding anything, it seeks back to 0 (once per piece). A
//   non-looping stream stops refilling at its end and is released once all 10 buffers are processed.

#include "COpenALDriver.h"
#include <AL/alut.h>
#include <vorbis/vorbisfile.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

namespace daisy {
namespace audio {

//! Number of buffers queued on a music stream.
static const int STREAM_BUFFER_COUNT = 10;
//! Size of each streamed buffer in bytes.
static const int STREAM_BUFFER_SIZE = 4096;

//! A loaded sound effect: one buffer, and the pool source that loops it.
class COpenALSoundInfo : public CSoundInfoStub
{
public:
    COpenALSoundInfo(ALuint buffer)
        : Buffer(buffer), Source(0)
    {
    }

    ALuint Buffer;
    //! The pool source looping this sound, 0 when it is not looping.
    ALuint Source;
};

//! A tracked sound owns its own source, outside the pool.
class COpenALTrackedSoundInfo : public CTrackedSoundInfoStub
{
public:
    COpenALTrackedSoundInfo(CSoundInfoStub* sound, ALuint source, float pitch)
        : CTrackedSoundInfoStub(sound), Source(source), Pitch(pitch)
    {
    }

    ALuint Source;
    //! The requested pitch, before the global pitch modifier.
    float Pitch;
};

//! A music stream or voice line: an Ogg Vorbis file streamed through STREAM_BUFFER_COUNT buffers on
//! its own source. The file is opened when it first plays and closed by release().
class COpenALMusicInfo : public CMusicInfoStub
{
public:
    COpenALMusicInfo(const char* fileName)
        : IsOpen(false), FileName(fileName), File(0), Info(0)
    {
    }

    ~COpenALMusicInfo()
    {
        release();
    }

    //! Refills the processed buffers; returns false when the stream ended.
    bool update(float pitch);
    void open();
    bool startPlayback(bool loop);
    void release();
    bool isPlaying();
    //! Decodes up to STREAM_BUFFER_SIZE bytes into buffer; false at the end of a non-looping stream.
    bool streamNextPiece(ALuint buffer);

    bool IsOpen;
    ox::core::CString<char> FileName;
    FILE* File;
    OggVorbis_File Stream;
    vorbis_info* Info;
    bool Loop;
    ALuint Source;
    ALuint Buffers[STREAM_BUFFER_COUNT];
    ALenum Format;
};

struct COpenALDriver::VorbisSource
{
    static size_t read(void* ptr, size_t size, size_t nmemb, void* datasource);
    static int seek(void* datasource, ogg_int64_t offset, int whence);
    static int close(void* datasource);
    static long tell(void* datasource);

    char* Data;
    ogg_int64_t Position;
    ogg_int64_t Size;
};

COpenALDriver::COpenALDriver()
    : NextSource(0), GlobalPitchModifier(1.0f)
{
    ConeInnerAngle = 30;
    ConeOuterAngle = 75;
    ConeDirection.X = 0.0f;
    ConeDirection.Y = 0.0f;
    ConeDirection.Z = -1.0f;

    for (int i = 0; i < NUM_SOURCES; ++i)
    {
        Sources[i] = 0;
        SourcePitch[i] = 1.0f;
        SourcePriority[i] = 1.0f;
    }

    if (alutInit(0, 0))
    {
        alGetError();
        alGenSources(NUM_SOURCES, Sources);
        alGetError();

        set2dOrientation();

        // Queried and unused: the sources keep OpenAL's default distances.
        ALint maxDistance = 0;
        ALint referenceDistance = 0;
        alGetSourcei(Sources[0], AL_MAX_DISTANCE, &maxDistance);
        alGetSourcei(Sources[0], AL_REFERENCE_DISTANCE, &referenceDistance);
    }
    else
    {
        // The error string was probably logged by a macro compiled out of the release build.
        alutGetErrorString(alutGetError());
    }
}

COpenALDriver::~COpenALDriver()
{
    stopAllSounds();

    for (std::list<ALuint>::iterator it = Buffers.begin(); it != Buffers.end(); ++it)
        alDeleteBuffers(1, &*it);

    alDeleteSources(NUM_SOURCES, Sources);
    alutExit();
}

void COpenALDriver::periodicStreamUpdate()
{
    float pitch = UseGlobalPitchModifierForMusic ? GlobalPitchModifier : 1.0f;

    for (MusicMap::iterator it = Music.begin(); it != Music.end(); ++it)
        static_cast<COpenALMusicInfo*>(it->second)->update(pitch);

    if (Voice)
        static_cast<COpenALMusicInfo*>(Voice)->update(pitch);
}

bool COpenALMusicInfo::update(float pitch)
{
    bool active = false;
    if (IsOpen)
    {
        ALint processed = 0;
        alGetSourcei(Source, AL_BUFFERS_PROCESSED, &processed);

        active = true;
        int count = processed;
        while (count-- > 0 && active)
        {
            ALuint buffer;
            alSourceUnqueueBuffers(Source, 1, &buffer);
            active = streamNextPiece(buffer);
            if (active)
                alSourceQueueBuffers(Source, 1, &buffer);
        }

        if (active)
        {
            // The source stops when it runs out of queued data.
            if (!isPlaying())
                alSourcePlay(Source);
        }
        else if (processed == STREAM_BUFFER_COUNT)
            release();

        alSourcef(Source, AL_PITCH, pitch);
    }
    return active;
}

void COpenALDriver::setGlobalPitchModifier(float pitch)
{
    GlobalPitchModifier = pitch;

    for (int i = 0; i < NUM_SOURCES; ++i)
    {
        ALint state;
        alGetSourcei(Sources[i], AL_SOURCE_STATE, &state);
        if (state == AL_PLAYING)
            alSourcef(Sources[i], AL_PITCH, pitch * SourcePitch[i]);
    }

    for (TrackedSoundMap::iterator it = TrackedSounds.begin(); it != TrackedSounds.end(); ++it)
    {
        COpenALTrackedSoundInfo* tracked = static_cast<COpenALTrackedSoundInfo*>(it->second);
        alSourcef(tracked->Source, AL_PITCH, pitch * tracked->Pitch);
    }

    if (UseGlobalPitchModifierForMusic)
    {
        for (MusicMap::iterator it = Music.begin(); it != Music.end(); ++it)
            alSourcef(static_cast<COpenALMusicInfo*>(it->second)->Source, AL_PITCH, pitch);
    }
}

void COpenALDriver::setOpenAlDistanceModel(int model)
{
    alDistanceModel(AL_INVERSE_DISTANCE - 1 + model);
}

CSoundInfoStub* COpenALDriver::deviceLoadSound(const char* name)
{
    ox::core::CString<char> path = SoundEffectPath;
    path.append(ox::core::CString<char>(name));

    ALuint buffer = 0;
    if (path.endsWith(ox::core::CString<char>(".ogg")))
        buffer = createBufferFromVorbisFile(path.c_str());
    else if (path.endsWith(ox::core::CString<char>(".wav")))
        buffer = alutCreateBufferFromFile(path.c_str());

    if (!buffer)
    {
        alutGetError();
        return 0;
    }
    Buffers.push_back(buffer);
    return new COpenALSoundInfo(buffer);
}

ALuint COpenALDriver::createBufferFromVorbisFile(const char* path)
{
    std::fstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return 0;

    int size = file.tellg();
    file.seekg(0);
    char* data = (char*)malloc(size);
    file.read(data, size);
    file.close();
    ALuint buffer = _loadOggFromMemory(data, size);
    free(data);
    return buffer;
}

size_t COpenALDriver::VorbisSource::read(void* ptr, size_t size, size_t nmemb, void* datasource)
{
    VorbisSource* source = (VorbisSource*)datasource;
    int bytes = size * nmemb;
    int remaining = source->Size - source->Position;
    if (bytes > remaining)
        bytes = remaining;
    if (bytes > 0)
    {
        memcpy(ptr, source->Data + source->Position, bytes);
        source->Position += bytes;
    }
    return bytes / size;
}

int COpenALDriver::VorbisSource::seek(void* datasource, ogg_int64_t offset, int whence)
{
    VorbisSource* source = (VorbisSource*)datasource;
    if (whence == SEEK_SET)
        source->Position = offset;
    else if (whence == SEEK_END)
        source->Position = source->Size - offset;
    else if (whence == SEEK_CUR)
        source->Position += offset;
    return 0;
}

int COpenALDriver::VorbisSource::close(void* datasource)
{
    return 0;
}

long COpenALDriver::VorbisSource::tell(void* datasource)
{
    return ((VorbisSource*)datasource)->Position;
}

ALuint COpenALDriver::_loadOggFromMemory(void* data, int size)
{
    VorbisSource source;
    source.Data = (char*)data;
    source.Position = 0;
    source.Size = size;

    ov_callbacks callbacks;
    callbacks.read_func = VorbisSource::read;
    callbacks.seek_func = VorbisSource::seek;
    callbacks.close_func = VorbisSource::close;
    callbacks.tell_func = VorbisSource::tell;

    OggVorbis_File file;
    ov_open_callbacks(&source, &file, 0, 0, callbacks);
    vorbis_info* info = ov_info(&file, -1);

    ALenum format = info->channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
    // The PCM buffer is sized as nominal bitrate (bits per second) times duration, in bytes;
    // decoded data beyond that is cut off.
    int bitrate = info->bitrate_nominal > 0 ? info->bitrate_nominal : info->rate;
    int capacity = (int)(ov_time_total(&file, -1) * bitrate);
    char* pcm = (char*)malloc(capacity);

    int bitstream = -1;
    int length = 0;
    do
    {
        int result = ov_read(&file, pcm + length, capacity - length, 0, 2, 1, &bitstream);
        if (result <= 0)
            break;
        length += result;
    } while (length < capacity);

    ALuint buffer;
    alGenBuffers(1, &buffer);
    alBufferData(buffer, format, pcm, length, info->rate);
    alGetError();
    free(pcm);
    ov_clear(&file);
    return buffer;
}

void COpenALDriver::devicePlaySound(CSoundInfoStub* sound, float volume, float pan, float pitch)
{
    unsigned int i = findFreeSoundSource();
    if (i >= NUM_SOURCES)
        return;

    SourcePitch[i] = pitch;
    alSourcei(Sources[i], AL_BUFFER, static_cast<COpenALSoundInfo*>(sound)->Buffer);
    alSourcef(Sources[i], AL_PITCH, pitch * GlobalPitchModifier);
    alSourcef(Sources[i], AL_GAIN, volume);
    alSourcei(Sources[i], AL_LOOPING, AL_FALSE);
    alSourcei(Sources[i], AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcef(Sources[i], AL_ROLLOFF_FACTOR, 1.0f);
    alSource3f(Sources[i], AL_DIRECTION, 0.0f, 0.0f, 0.0f);
    alSourcei(Sources[i], AL_CONE_INNER_ANGLE, 360);
    alSourcei(Sources[i], AL_CONE_OUTER_ANGLE, 360);
    // Panned left or right of the listener, slightly in front.
    alSource3f(Sources[i], AL_POSITION, pan + pan, 0.0f, 0.1f);
    alSourcePlay(Sources[i]);
}

unsigned int COpenALDriver::findFreeSoundSource()
{
    unsigned int start = NextSource;
    do
    {
        unsigned int i = NextSource;
        ++NextSource;
        if (NextSource >= NUM_SOURCES)
            NextSource = 0;

        if (alIsSource(Sources[i]))
        {
            if (SoundPriority > SourcePriority[i])
                alSourceStop(Sources[i]);

            ALint state;
            alGetSourcei(Sources[i], AL_SOURCE_STATE, &state);
            if (state != AL_PLAYING)
            {
                SourcePriority[i] = SoundPriority;
                return i;
            }
        }
    } while (NextSource != start);

    return NUM_SOURCES;
}

void COpenALDriver::devicePlayOrientedSound(CSoundInfoStub* sound, float volume, float pitch,
    const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity)
{
    unsigned int i = findFreeSoundSource();
    if (i >= NUM_SOURCES)
        return;

    SourcePitch[i] = pitch;
    alSourcei(Sources[i], AL_BUFFER, static_cast<COpenALSoundInfo*>(sound)->Buffer);
    alSourcef(Sources[i], AL_PITCH, pitch * GlobalPitchModifier);
    alSourcef(Sources[i], AL_GAIN, volume);
    alSourcei(Sources[i], AL_LOOPING, AL_FALSE);
    alSourcei(Sources[i], AL_SOURCE_RELATIVE, AL_FALSE);
    alSourcef(Sources[i], AL_ROLLOFF_FACTOR, GlobalRollOffFactor);
    alSource3f(Sources[i], AL_POSITION, position.X, position.Y, position.Z);
    alSource3f(Sources[i], AL_VELOCITY, velocity.X, velocity.Y, velocity.Z);
    alSource3f(Sources[i], AL_DIRECTION, ConeDirection.X, ConeDirection.Y, ConeDirection.Z);
    alSourcei(Sources[i], AL_CONE_INNER_ANGLE, ConeInnerAngle);
    alSourcei(Sources[i], AL_CONE_OUTER_ANGLE, ConeOuterAngle);
    alSourcePlay(Sources[i]);
}

void COpenALDriver::deviceDampenAllSounds(float factor)
{
}

void COpenALDriver::deviceLoopSound(CSoundInfoStub* sound, float volume, float pan, float pitch, float fade)
{
    COpenALSoundInfo* info = static_cast<COpenALSoundInfo*>(sound);
    ALuint source = info->Source;
    if (!source)
    {
        unsigned int i = findFreeSoundSource();
        if (i >= NUM_SOURCES)
            return;
        info->Source = Sources[i];
        SourcePitch[i] = pitch;
        source = info->Source;
    }

    alGetError();
    alSourcef(source, AL_PITCH, pitch * GlobalPitchModifier);
    alSourcef(source, AL_GAIN, volume);
    alSourcei(source, AL_LOOPING, AL_TRUE);
    alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcef(source, AL_ROLLOFF_FACTOR, GlobalRollOffFactor);
    alSource3f(source, AL_POSITION, pan + pan, 0.0f, 0.1f);
    alSource3f(source, AL_DIRECTION, ConeDirection.X, ConeDirection.Y, ConeDirection.Z);
    alSourcei(source, AL_CONE_INNER_ANGLE, ConeInnerAngle);
    alSourcei(source, AL_CONE_OUTER_ANGLE, ConeOuterAngle);

    // An already looping sound only has its parameters updated.
    ALint state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING)
    {
        alSourcei(source, AL_BUFFER, info->Buffer);
        alSourcef(source, AL_SEC_OFFSET, fade);
        alSourcePlay(source);
    }
    alGetError();
}

void COpenALDriver::deviceStopLoopSound(CSoundInfoStub* sound)
{
    COpenALSoundInfo* info = static_cast<COpenALSoundInfo*>(sound);
    if (info->Source)
    {
        alSourceStop(info->Source);
        info->Source = 0;
    }
}

void COpenALDriver::deviceStopSound(CSoundInfoStub* sound)
{
    COpenALSoundInfo* info = static_cast<COpenALSoundInfo*>(sound);
    if (info->Source)
    {
        alSourceStop(info->Source);
        info->Source = 0;
    }
}

CMusicInfoStub* COpenALDriver::deviceLoadMusic(const char* name)
{
    ox::core::CString<char> path = SoundEffectPath;
    path.append(ox::core::CString<char>(name));
    COpenALMusicInfo* music = new COpenALMusicInfo(path.c_str());
    return music;
}

void COpenALDriver::devicePlayMusic(CMusicInfoStub* music, float volume, bool loop, bool voice)
{
    COpenALMusicInfo* info = static_cast<COpenALMusicInfo*>(music);
    info->open();
    if (info->startPlayback(loop))
    {
        alSourcef(info->Source, AL_GAIN, volume);
        // Looping is done by the stream, not the source.
        alSourcei(info->Source, AL_LOOPING, AL_FALSE);
    }
}

void COpenALMusicInfo::open()
{
    if (IsOpen)
        return;

    File = fopen(FileName.c_str(), "rb");
    if (!File)
        return;

    if (ov_open_callbacks(File, &Stream, 0, 0, OV_CALLBACKS_DEFAULT) < 0)
    {
        fclose(File);
        File = 0;
        return;
    }

    Info = ov_info(&Stream, -1);
    if (Info)
        Format = Info->channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

    alGenBuffers(STREAM_BUFFER_COUNT, Buffers);
    alGenSources(1, &Source);
    alSource3f(Source, AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSource3f(Source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    alSource3f(Source, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
    alSourcef(Source, AL_ROLLOFF_FACTOR, 0.0f);
    alSource3f(Source, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
    alSourcei(Source, AL_CONE_INNER_ANGLE, 360);
    alSourcei(Source, AL_CONE_OUTER_ANGLE, 360);
    alSourcei(Source, AL_SOURCE_RELATIVE, AL_TRUE);
    IsOpen = true;
}

bool COpenALMusicInfo::startPlayback(bool loop)
{
    bool result = false;
    if (IsOpen)
    {
        Loop = loop;
        result = true;
        if (!isPlaying())
        {
            for (int i = 0; i < STREAM_BUFFER_COUNT; ++i)
                streamNextPiece(Buffers[i]);
            alSourceQueueBuffers(Source, STREAM_BUFFER_COUNT, Buffers);
            alSourcePlay(Source);
        }
    }
    return result;
}

void COpenALDriver::deviceStopMusic(CMusicInfoStub* music)
{
    static_cast<COpenALMusicInfo*>(music)->release();
}

void COpenALMusicInfo::release()
{
    if (!IsOpen)
        return;

    alSourceStop(Source);
    ALint queued;
    alGetSourcei(Source, AL_BUFFERS_QUEUED, &queued);
    while (queued-- > 0)
    {
        ALuint buffer;
        alSourceUnqueueBuffers(Source, 1, &buffer);
    }
    alDeleteSources(1, &Source);
    alDeleteBuffers(STREAM_BUFFER_COUNT, Buffers);
    ov_clear(&Stream);
    IsOpen = false;
}

bool COpenALDriver::deviceUpdateMusic(CMusicInfoStub* music, float frameDelta)
{
    COpenALMusicInfo* info = static_cast<COpenALMusicInfo*>(music);
    if (info->isPlaying())
    {
        // The volume arrives in frameDelta (CAudioDriver::updateMusic passes the volume).
        alSourcef(info->Source, AL_GAIN, frameDelta);
        return true;
    }
    return false;
}

bool COpenALMusicInfo::isPlaying()
{
    bool result = false;
    if (IsOpen)
    {
        ALint state;
        alGetSourcei(Source, AL_SOURCE_STATE, &state);
        result = state == AL_PLAYING;
    }
    return result;
}

bool COpenALDriver::deviceIsMusicPlaying(CMusicInfoStub* music)
{
    return static_cast<COpenALMusicInfo*>(music)->isPlaying();
}

void COpenALDriver::deviceSetListenerPosition(const ox::core::CVector3d<float>& position)
{
    alListener3f(AL_POSITION, position.X, position.Y, position.Z);
}

void COpenALDriver::deviceSetListenerVelocity(const ox::core::CVector3d<float>& velocity)
{
    alListener3f(AL_VELOCITY, velocity.X, velocity.Y, velocity.Z);
}

void COpenALDriver::deviceSetListenerOrientation(const ox::core::CVector3d<float>& forward,
    const ox::core::CVector3d<float>& up)
{
    ALfloat orientation[6] = { forward.X, forward.Y, forward.Z, up.X, up.Y, up.Z };
    alListenerfv(AL_ORIENTATION, orientation);
}

CTrackedSoundInfoStub* COpenALDriver::deviceStartTrackedSound(CSoundInfoStub* sound, float volume, float pitch,
    const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity, float fade)
{
    alGetError();
    ALuint source;
    alGenSources(1, &source);
    if (alGetError() != AL_NO_ERROR)
        return 0;

    COpenALTrackedSoundInfo* tracked = new COpenALTrackedSoundInfo(sound, source, pitch);
    alSourcei(source, AL_BUFFER, static_cast<COpenALSoundInfo*>(sound)->Buffer);
    alSourcef(source, AL_PITCH, pitch * GlobalPitchModifier);
    alSourcef(source, AL_GAIN, volume);
    alSourcei(source, AL_LOOPING, AL_TRUE);
    alSourcei(source, AL_SOURCE_RELATIVE, AL_FALSE);
    alSourcef(source, AL_ROLLOFF_FACTOR, GlobalRollOffFactor);
    alSource3f(source, AL_DIRECTION, ConeDirection.X, ConeDirection.Y, ConeDirection.Z);
    alSourcei(source, AL_CONE_INNER_ANGLE, ConeInnerAngle);
    alSourcei(source, AL_CONE_OUTER_ANGLE, ConeOuterAngle);
    alSource3f(source, AL_POSITION, position.X, position.Y, position.Z);
    alSource3f(source, AL_VELOCITY, velocity.X, velocity.Y, velocity.Z);
    alSourcef(source, AL_SEC_OFFSET, fade);
    alSourcePlay(source);
    return tracked;
}

void COpenALDriver::deviceStopTrackedSound(CTrackedSoundInfoStub* sound)
{
    COpenALTrackedSoundInfo* tracked = static_cast<COpenALTrackedSoundInfo*>(sound);
    alSourceStop(tracked->Source);
    alDeleteSources(1, &tracked->Source);
}

void COpenALDriver::deviceUpdateTrackedSound(CTrackedSoundInfoStub* sound, float volume, float pitch,
    const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity)
{
    COpenALTrackedSoundInfo* tracked = static_cast<COpenALTrackedSoundInfo*>(sound);
    tracked->Pitch = pitch;
    alSourcef(tracked->Source, AL_PITCH, pitch * GlobalPitchModifier);
    alSourcef(tracked->Source, AL_GAIN, volume);
    alSource3f(tracked->Source, AL_POSITION, position.X, position.Y, position.Z);
    alSource3f(tracked->Source, AL_VELOCITY, velocity.X, velocity.Y, velocity.Z);
}

bool COpenALMusicInfo::streamNextPiece(ALuint buffer)
{
    char data[STREAM_BUFFER_SIZE];
    int size = 0;
    int section = 0;
    bool loop = Loop;
    while (size < STREAM_BUFFER_SIZE)
    {
        int result = ov_read(&Stream, data + size, STREAM_BUFFER_SIZE - size, 0, 2, 1, &section);
        if (result > 0)
            size += result;
        else if (result < 0)
            return false;
        else if (result == 0 && size == 0 && loop)
        {
            ov_time_seek(&Stream, 0.0);
            loop = false;
        }
        else
            break;
    }

    if (size == 0)
        return false;

    alGetError();
    alBufferData(buffer, Format, data, size, Info->rate);
    alGetError();
    return true;
}

} // end namespace audio
} // end namespace daisy
