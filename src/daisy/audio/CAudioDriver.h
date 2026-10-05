// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The sound logic shared by the audio backends; COpenALDriver implements the device* methods.

#ifndef DAISY_AUDIO_CAUDIODRIVER_H
#define DAISY_AUDIO_CAUDIODRIVER_H

#include <map>
#include "ox/audio/IAudioDriver.h"
#include "ox/core/CString.h"

namespace daisy {
namespace audio {

//! A loaded sound effect; the backend derives its own.
class CSoundInfoStub
{
public:
    virtual ~CSoundInfoStub() {}
    //! Timer time of the last start, to drop repeats within 50 ms.
    unsigned int LastPlayTime;
};

//! A loaded music stream or voice line; the backend derives its own.
class CMusicInfoStub
{
public:
    virtual ~CMusicInfoStub() {}
    float Volume;
};

//! A playing sound whose position can be updated; the backend derives its own.
class CTrackedSoundInfoStub
{
public:
    virtual ~CTrackedSoundInfoStub() {}
};

class CAudioDriver : public ox::audio::IAudioDriver
{
public:
    CAudioDriver();
    virtual ~CAudioDriver();

    virtual void stopAllSounds();
    virtual void periodicStreamUpdate();
    virtual void setSoundEffectPath(const char* path);
    virtual void setSoundEffectVolume(int volume);
    virtual void setMusicVolume(int volume);
    virtual int getSoundEffectVolume();
    virtual int getMusicVolume();
    virtual void setDampenSoundsOnVoice(bool enabled);
    virtual void setGlobalPitchModifier(float pitch) {}
    virtual void setUseGlobalPitchModifierForMusic(bool enabled);
    virtual void setGlobalRollOffFactor(float factor);
    virtual void setOpenAlDistanceModel(int model) {}
    virtual void setSoundPriority(float priority) { SoundPriority = priority; }
    virtual bool loadSound(const char* name);
    virtual void playSound(const char* name, float volume, float pan, float pitch);
    virtual void loopSound(const char* name, float volume, float pan, float pitch, float fade);
    virtual void stopLoopSound(const char* name);
    virtual bool loadMusic(const char* name);
    virtual void playMusic(const char* name, float volume, bool loop);
    virtual void stopMusic(const char* name);
    virtual bool updateMusic(const char* name, float volume);
    virtual void stopAllMusic();
    virtual void playVoice(const char* name);
    virtual void stopVoice();
    virtual bool isVoicePlaying();
    virtual void set2dOrientation();
    virtual void set3dOrientation(const ox::core::CVector3d<float>& forward, const ox::core::CVector3d<float>& up);
    virtual void set3dCameraPosition(const ox::scene::ICameraSceneNode* camera);
    virtual void setListenerPosition(const ox::core::CVector3d<float>& position,
        const ox::core::CVector3d<float>& velocity);
    virtual void setOrientedSoundCone(int innerAngle, int outerAngle, const ox::core::CVector3d<float>& direction);
    virtual void playOrientedSound(const char* name, float volume, float pitch,
        const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity);
    virtual int startTrackedSound(const char* name, float volume, float pitch,
        const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity, float fade);
    virtual void stopTrackedSound(int handle);
    virtual void stopAllTrackedSounds();
    virtual void updateTrackedSound(int handle, float volume, float pitch,
        const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity);

protected:
    typedef std::map<ox::core::CString<char>, CSoundInfoStub*> SoundMap;
    typedef std::map<int, CTrackedSoundInfoStub*> TrackedSoundMap;
    typedef std::map<ox::core::CString<char>, CMusicInfoStub*> MusicMap;

    virtual CSoundInfoStub* deviceLoadSound(const char* name) = 0;
    virtual void devicePlaySound(CSoundInfoStub* sound, float volume, float pan, float pitch) = 0;
    //! Scales the volume of the playing sounds while a voice line plays.
    virtual void deviceDampenAllSounds(float factor) = 0;
    virtual void deviceLoopSound(CSoundInfoStub* sound, float volume, float pan, float pitch, float fade) {}
    virtual void deviceStopLoopSound(CSoundInfoStub* sound) {}
    virtual void deviceStopSound(CSoundInfoStub* sound) {}
    virtual CMusicInfoStub* deviceLoadMusic(const char* name) = 0;
    virtual void devicePlayMusic(CMusicInfoStub* music, float volume, bool loop, bool voice) = 0;
    virtual void deviceStopMusic(CMusicInfoStub* music) = 0;
    virtual bool deviceUpdateMusic(CMusicInfoStub* music, float frameDelta) = 0;
    virtual bool deviceIsMusicPlaying(CMusicInfoStub* music) = 0;
    virtual void deviceSetListenerPosition(const ox::core::CVector3d<float>& position) = 0;
    virtual void deviceSetListenerVelocity(const ox::core::CVector3d<float>& velocity) = 0;
    virtual void deviceSetListenerOrientation(const ox::core::CVector3d<float>& forward,
        const ox::core::CVector3d<float>& up) = 0;
    //! Without 3d support: plays the sound panned towards the listener's side.
    virtual void devicePlayOrientedSound(CSoundInfoStub* sound, float volume, float pitch,
        const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity);
    virtual CTrackedSoundInfoStub* deviceStartTrackedSound(CSoundInfoStub* sound, float volume, float pitch,
        const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity, float fade) = 0;
    virtual void deviceStopTrackedSound(CTrackedSoundInfoStub* sound) = 0;
    virtual void deviceUpdateTrackedSound(CTrackedSoundInfoStub* sound, float volume, float pitch,
        const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity) = 0;

    void initCommon();
    void eraseSounds();
    SoundMap::iterator findOrLoadSound(const char* name);

    SoundMap Sounds;
    TrackedSoundMap TrackedSounds;
    MusicMap Music;
    int NextTrackedSoundHandle;
    CMusicInfoStub* Voice;
    bool DampenSoundsOnVoice;
    bool UseGlobalPitchModifierForMusic;
    float GlobalRollOffFactor;
    ox::core::CString<char> SoundEffectPath;
    //! The volume settings (0 to 5) as gains.
    float SoundEffectGain;
    float MusicGain;
    int SoundEffectVolume;
    int MusicVolume;
    ox::core::CVector3d<float> ListenerPosition;
    int ConeInnerAngle;
    int ConeOuterAngle;
    ox::core::CVector3d<float> ConeDirection;
    float SoundPriority;
};

} // end namespace audio
} // end namespace daisy

#endif
