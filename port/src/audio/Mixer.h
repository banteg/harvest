// The port's mixer: the part of OpenAL the original audio backend used, rebuilt on a miniaudio
// playback device. Sources play whole decoded buffers or streamed queues, and are positioned,
// attenuated and panned the way OpenAL Soft rendered them to stereo. See docs/port/audio.md.
//
// Threads: the game thread changes sources and the listener while holding the mixer lock (MixerLock);
// miniaudio's device callback takes the same lock for each block it mixes. Streamed data is decoded
// on the game thread (periodicStreamUpdate), so the callback only reads memory.

#ifndef PORT_AUDIO_MIXER_H
#define PORT_AUDIO_MIXER_H

#include <vector>
#include <miniaudio.h>
#include "ox/core/CVector3d.h"

namespace port {
namespace audio {

typedef ox::core::CVector3d<float> Vec3;

//! Decoded PCM, 32-bit float, interleaved: an effect decoded whole (an OpenAL buffer).
struct Buffer
{
    Buffer() : Channels(0), Rate(0), Frames(0) {}
    std::vector<float> Samples;
    unsigned int Channels;
    unsigned int Rate;
    ma_uint64 Frames;
};

//! A streamed source's queue: a ring of decoded frames the game thread refills and the mixer drains.
//! Frames are counted from the start of playback; frame n lives at (n % Capacity).
struct StreamQueue
{
    StreamQueue() : Channels(0), Rate(0), Capacity(0), Written(0), Ended(false) {}
    std::vector<float> Samples;
    unsigned int Channels;
    unsigned int Rate;
    ma_uint64 Capacity;
    //! Frames queued so far.
    ma_uint64 Written;
    //! No more frames will be queued: the source stops when it has played them all.
    bool Ended;
};

//! An OpenAL source: what it plays and the parameters the original set on it. The setters follow
//! OpenAL's rule for out-of-range values: they are rejected and the old value stays.
struct Source
{
    Source();

    //! Plays from the start (or from Offset seconds into a buffer, like AL_SEC_OFFSET).
    void play();
    void setGain(float gain);
    void setPitch(float pitch);
    //! AL_SEC_OFFSET before play(); an offset past the end is rejected.
    void setOffset(float seconds);

    const Buffer* Data;
    StreamQueue* Queue;
    bool Playing;
    bool Looping;
    //! AL_SOURCE_RELATIVE: the position is in listener space.
    bool Relative;
    //! Playback position in frames of the source data.
    double Cursor;
    double Offset;
    float Gain;
    float Pitch;
    float Rolloff;
    //! Cone angles in degrees (360: no cone); the outer cone gain is OpenAL's default, 0.
    float ConeInner;
    float ConeOuter;
    Vec3 Position;
    //! Kept for completeness: the port does no Doppler shift (the game never moves sounds).
    Vec3 Velocity;
    //! The cone axis; zero means omnidirectional.
    Vec3 Direction;
};

class Mixer
{
public:
    //! OpenAL's distance models (alDistanceModel values).
    enum
    {
        DISTANCE_NONE = 0,
        INVERSE_DISTANCE = 0xD001,
        INVERSE_DISTANCE_CLAMPED = 0xD002,
        LINEAR_DISTANCE = 0xD003,
        LINEAR_DISTANCE_CLAMPED = 0xD004,
        EXPONENT_DISTANCE = 0xD005,
        EXPONENT_DISTANCE_CLAMPED = 0xD006
    };

    Mixer();
    ~Mixer();

    //! Opens and starts the default playback device (stereo, 32-bit float, its native rate).
    bool openDevice();
    //! Mixes without a device: the caller pulls blocks with mix() at this rate (tests, rendering).
    void openOffline(unsigned int sampleRate);
    //! Stops the device. Sources stay registered.
    void close();
    //! False when no device could be opened: nothing plays, as with the original when alutInit
    //! failed.
    bool isAvailable() const { return SampleRate != 0; }
    unsigned int getSampleRate() const { return SampleRate; }

    void lock();
    void unlock();

    //! Registers a source for mixing; call with the lock held.
    void addSource(Source* source);
    void removeSource(Source* source);

    //! Mixes the playing sources into interleaved stereo; takes the lock.
    void mix(float* output, ma_uint32 frames);
    //! The number of playing sources; takes the lock.
    unsigned int getPlayingCount();

    //! Sets the distance model by its OpenAL value; invalid values are rejected, as by OpenAL.
    void setDistanceModel(int model);

    // The listener, in OpenAL's coordinates. Change with the lock held.
    Vec3 ListenerPosition;
    Vec3 ListenerVelocity;
    Vec3 ListenerForward;
    Vec3 ListenerUp;

private:
    static void dataCallback(ma_device* device, void* output, const void* input, ma_uint32 frames);
    //! The source's gain per output channel; for a stereo source, per source channel.
    void computeGains(const Source& source, unsigned int channels, float gains[2]) const;
    void mixSource(Source& source, float* output, ma_uint32 frames);

    ma_device Device;
    bool DeviceOpen;
    unsigned int SampleRate;
    volatile ma_spinlock Lock;
    int DistanceModel;
    std::vector<Source*> Sources;
};

//! Holds the mixer lock for a scope.
class MixerLock
{
public:
    explicit MixerLock(Mixer& mixer) : Mix(mixer) { Mix.lock(); }
    ~MixerLock() { Mix.unlock(); }

private:
    Mixer& Mix;
};

} // end namespace audio
} // end namespace port

#endif
