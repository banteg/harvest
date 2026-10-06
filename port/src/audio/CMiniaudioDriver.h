// The port's audio backend: daisy::audio::CAudioDriver's device methods on the port's mixer, made to
// behave like the original OpenAL backend (src/daisy/audio/COpenALDriver.cpp). Files are read
// through the game's file system, so sounds can come from archives. See docs/port/audio.md.

#ifndef PORT_AUDIO_CMINIAUDIODRIVER_H
#define PORT_AUDIO_CMINIAUDIODRIVER_H

#include "daisy/audio/CAudioDriver.h"
#include "audio/Mixer.h"

namespace ox {
namespace io { class IFileSystem; }
}

namespace port {
namespace audio {

class CMiniaudioDriver : public daisy::audio::CAudioDriver
{
public:
    //! Opens the default playback device; when that fails the driver stays silent, as the original
    //! did when alutInit failed. With offlineRate the driver opens no device and the caller pulls
    //! the mix through getMixer().mix() at that rate.
    CMiniaudioDriver(ox::io::IFileSystem* fileSystem, unsigned int offlineRate = 0);
    virtual ~CMiniaudioDriver();

    //! Refills the music and voice streams; the game calls it every frame.
    virtual void periodicStreamUpdate();
    virtual void setGlobalPitchModifier(float pitch);
    //! Selects the OpenAL distance model 0xD000 + model, as the original.
    virtual void setOpenAlDistanceModel(int model);

    Mixer& getMixer() { return Mix; }

protected:
    virtual daisy::audio::CSoundInfoStub* deviceLoadSound(const char* name);
    virtual void devicePlaySound(daisy::audio::CSoundInfoStub* sound, float volume, float pan, float pitch);
    virtual void deviceDampenAllSounds(float factor);
    virtual void deviceLoopSound(daisy::audio::CSoundInfoStub* sound, float volume, float pan, float pitch,
        float fade);
    virtual void deviceStopLoopSound(daisy::audio::CSoundInfoStub* sound);
    virtual void deviceStopSound(daisy::audio::CSoundInfoStub* sound);
    virtual daisy::audio::CMusicInfoStub* deviceLoadMusic(const char* name);
    virtual void devicePlayMusic(daisy::audio::CMusicInfoStub* music, float volume, bool loop, bool voice);
    virtual void deviceStopMusic(daisy::audio::CMusicInfoStub* music);
    virtual bool deviceUpdateMusic(daisy::audio::CMusicInfoStub* music, float volume);
    virtual bool deviceIsMusicPlaying(daisy::audio::CMusicInfoStub* music);
    virtual void deviceSetListenerPosition(const ox::core::CVector3d<float>& position);
    virtual void deviceSetListenerVelocity(const ox::core::CVector3d<float>& velocity);
    virtual void deviceSetListenerOrientation(const ox::core::CVector3d<float>& forward,
        const ox::core::CVector3d<float>& up);
    virtual void devicePlayOrientedSound(daisy::audio::CSoundInfoStub* sound, float volume, float pitch,
        const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity);
    virtual daisy::audio::CTrackedSoundInfoStub* deviceStartTrackedSound(daisy::audio::CSoundInfoStub* sound,
        float volume, float pitch, const ox::core::CVector3d<float>& position,
        const ox::core::CVector3d<float>& velocity, float fade);
    virtual void deviceStopTrackedSound(daisy::audio::CTrackedSoundInfoStub* sound);
    virtual void deviceUpdateTrackedSound(daisy::audio::CTrackedSoundInfoStub* sound, float volume, float pitch,
        const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity);

private:
    enum
    {
        //! Size of the source pool shared by one-shot and looping sound effects.
        NUM_SOURCES = 32
    };

    //! Returns a pool index, or NUM_SOURCES when every source is busy. Call with the lock held.
    unsigned int findFreeSoundSource();
    //! The sound effect path joined with the name, as the original builds it.
    ox::core::CString<char> getPath(const char* name) const;

    ox::io::IFileSystem* FileSystem;
    Mixer Mix;
    Source Sources[NUM_SOURCES];
    //! The pitch each pool source was started with, before the global pitch modifier.
    float SourcePitch[NUM_SOURCES];
    //! The sound priority each pool source was started with.
    float SourcePriority[NUM_SOURCES];
    //! Round-robin start of the next free-source search.
    unsigned int NextSource;
    float GlobalPitchModifier;
};

} // end namespace audio
} // end namespace port

#endif
