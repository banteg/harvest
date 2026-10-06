// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The OpenAL backend of the audio driver: a fixed pool of sources for one-shot and looping effects,
// one dedicated source per tracked sound and per music stream (Ogg Vorbis, streamed through queued
// buffers). Sound effects are .ogg files decoded into one buffer or .wav files loaded by ALUT.

#ifndef DAISY_AUDIO_COPENALDRIVER_H
#define DAISY_AUDIO_COPENALDRIVER_H

#include <list>
#include <AL/al.h>
#include "CAudioDriver.h"

namespace daisy {
namespace audio {

class COpenALDriver : public CAudioDriver
{
public:
    //! Initializes OpenAL through alutInit and generates the source pool. When alutInit fails the
    //! driver stays silent: the pool keeps source name 0 and no error is reported.
    COpenALDriver();
    virtual ~COpenALDriver();

    //! Refills the music and voice streams; the game calls it every frame.
    virtual void periodicStreamUpdate();
    virtual void setGlobalPitchModifier(float pitch);
    //! Selects the OpenAL distance model: 0xD000 + model, so 1 is AL_INVERSE_DISTANCE,
    //! 2 AL_INVERSE_DISTANCE_CLAMPED (the OpenAL default when never called).
    virtual void setOpenAlDistanceModel(int model);

protected:
    virtual CSoundInfoStub* deviceLoadSound(const char* name);
    virtual void devicePlaySound(CSoundInfoStub* sound, float volume, float pan, float pitch);
    virtual void deviceDampenAllSounds(float factor);
    virtual void deviceLoopSound(CSoundInfoStub* sound, float volume, float pan, float pitch, float fade);
    virtual void deviceStopLoopSound(CSoundInfoStub* sound);
    virtual void deviceStopSound(CSoundInfoStub* sound);
    virtual CMusicInfoStub* deviceLoadMusic(const char* name);
    virtual void devicePlayMusic(CMusicInfoStub* music, float volume, bool loop, bool voice);
    virtual void deviceStopMusic(CMusicInfoStub* music);
    virtual bool deviceUpdateMusic(CMusicInfoStub* music, float frameDelta);
    virtual bool deviceIsMusicPlaying(CMusicInfoStub* music);
    virtual void deviceSetListenerPosition(const ox::core::CVector3d<float>& position);
    virtual void deviceSetListenerVelocity(const ox::core::CVector3d<float>& velocity);
    virtual void deviceSetListenerOrientation(const ox::core::CVector3d<float>& forward,
        const ox::core::CVector3d<float>& up);
    virtual void devicePlayOrientedSound(CSoundInfoStub* sound, float volume, float pitch,
        const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity);
    virtual CTrackedSoundInfoStub* deviceStartTrackedSound(CSoundInfoStub* sound, float volume, float pitch,
        const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity, float fade);
    virtual void deviceStopTrackedSound(CTrackedSoundInfoStub* sound);
    virtual void deviceUpdateTrackedSound(CTrackedSoundInfoStub* sound, float volume, float pitch,
        const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity);

private:
    //! An in-memory .ogg file read through libvorbisfile callbacks.
    struct VorbisSource;

    enum
    {
        //! Size of the source pool shared by one-shot and looping sound effects.
        NUM_SOURCES = 32
    };

    //! Returns a pool index, or NUM_SOURCES when every source is busy.
    unsigned int findFreeSoundSource();
    //! Decodes a whole .ogg file into one 16-bit buffer; returns 0 when the file cannot be opened.
    ALuint createBufferFromVorbisFile(const char* path);
    ALuint _loadOggFromMemory(void* data, int size);

    //! Every effect buffer, deleted with the driver.
    std::list<ALuint> Buffers;
    ALuint Sources[NUM_SOURCES];
    //! The pitch each pool source was started with, before the global pitch modifier.
    float SourcePitch[NUM_SOURCES];
    //! The sound priority each pool source was started with.
    float SourcePriority[NUM_SOURCES];
    //! Round-robin start of the next free-source search.
    unsigned int NextSource;
    float GlobalPitchModifier;
};

} // end namespace audio
} // end namespace daisy

#endif
