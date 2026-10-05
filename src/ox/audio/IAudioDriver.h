// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: virtual slots follow daisy::audio::CAudioDriver through isVoicePlaying.
// Return types unused by recovered callers are provisional.
#ifndef OX_AUDIO_IAUDIODRIVER_H
#define OX_AUDIO_IAUDIODRIVER_H
namespace ox {
namespace audio {
class IAudioDriver
{
public:
    virtual ~IAudioDriver();
    virtual void stopAllSounds() = 0;
    virtual void periodicStreamUpdate() = 0;
    virtual void setSoundEffectPath(const char* path) = 0;
    virtual void setSoundEffectVolume(int volume) = 0;
    virtual void setMusicVolume(int volume) = 0;
    virtual int getSoundEffectVolume() = 0;
    virtual int getMusicVolume() = 0;
    virtual void setDampenSoundsOnVoice(bool enabled) = 0;
    virtual void setGlobalPitchModifier(float pitch) = 0;
    virtual void setUseGlobalPitchModifierForMusic(bool enabled) = 0;
    virtual void setGlobalRollOffFactor(float factor) = 0;
    virtual void setOpenAlDistanceModel(int model) = 0;
    virtual void setSoundPriority(float priority) = 0;
    virtual void loadSound(const char* name) = 0;
    virtual void playSound(const char* name, float volume, float pan, float pitch) = 0;
    virtual void loopSound(const char* name, float volume, float pan, float pitch, float fade) = 0;
    virtual void stopLoopSound(const char* name) = 0;
    virtual bool loadMusic(const char* name) = 0;
    virtual void playMusic(const char* name, float volume, bool loop) = 0;
    virtual void stopMusic(const char* name) = 0;
    virtual bool updateMusic(const char* name, float frameDelta) = 0;
    virtual void stopAllMusic() = 0;
    //! Plays a voice line, dampening the other sounds while it plays.
    virtual void playVoice(const char* name) = 0;
    virtual void stopVoice() = 0;
    virtual bool isVoicePlaying() = 0;
};
} // end namespace audio
} // end namespace ox
#endif
