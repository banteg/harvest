// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CAudioDriver.h"
#include "daisy/os.h"
#include "ox/core/CBasic.h"
#include "ox/scene/ICameraSceneNode.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace audio {

//! Sounds started again within this many milliseconds are dropped.
static const unsigned int MIN_SOUND_REPEAT_MS = 50;
//! Gain of the other sounds while a voice line plays.
static const float VOICE_DAMPEN_FACTOR = 0.25f;

CAudioDriver::CAudioDriver()
    : NextTrackedSoundHandle(1), Voice(0), UseGlobalPitchModifierForMusic(false), GlobalRollOffFactor(1.0f),
      SoundPriority(1.0f)
{
    initCommon();
}

void CAudioDriver::initCommon()
{
    SoundEffectVolume = 5;
    SoundEffectGain = 1.0f;
    MusicVolume = 3;
    MusicGain = 0.6f;
    DampenSoundsOnVoice = true;
}

CAudioDriver::~CAudioDriver()
{
    eraseSounds();
}

void CAudioDriver::eraseSounds()
{
    for (TrackedSoundMap::iterator it = TrackedSounds.begin(); it != TrackedSounds.end(); ++it)
        delete it->second;

    for (SoundMap::iterator it = Sounds.begin(); it != Sounds.end(); ++it)
        delete it->second;

    for (MusicMap::iterator it = Music.begin(); it != Music.end(); ++it)
        delete it->second;

    if (Voice)
    {
        delete Voice;
        Voice = 0;
    }
}

void CAudioDriver::periodicStreamUpdate()
{
}

void CAudioDriver::setDampenSoundsOnVoice(bool enabled)
{
    DampenSoundsOnVoice = enabled;
}

void CAudioDriver::setUseGlobalPitchModifierForMusic(bool enabled)
{
    UseGlobalPitchModifierForMusic = enabled;
}

void CAudioDriver::setGlobalRollOffFactor(float factor)
{
    GlobalRollOffFactor = factor;
}

bool CAudioDriver::loadSound(const char* name)
{
    ox::core::CString<char> key = name;
    SoundMap::iterator it = Sounds.find(key);
    if (it == Sounds.end())
    {
        CSoundInfoStub* sound = deviceLoadSound(name);
        if (!sound)
            return false;
        Sounds[key] = sound;
    }
    return true;
}

CAudioDriver::SoundMap::iterator CAudioDriver::findOrLoadSound(const char* name)
{
    ox::core::CString<char> key = name;
    SoundMap::iterator it = Sounds.find(key);
    if (it != Sounds.end())
        return it;
    if (loadSound(name))
        it = Sounds.find(key);
    return it;
}

void CAudioDriver::playSound(const char* name, float volume, float pan, float pitch)
{
    if (SoundEffectGain <= 0.0f)
        return;

    SoundMap::iterator it = findOrLoadSound(name);
    if (it == Sounds.end())
        return;

    unsigned int now = os::Timer::getTime();
    if (now - it->second->LastPlayTime < MIN_SOUND_REPEAT_MS)
        return;

    float finalVolume = volume;
    if (Voice)
    {
        if (deviceIsMusicPlaying(Voice))
        {
            if (DampenSoundsOnVoice)
                finalVolume = volume * VOICE_DAMPEN_FACTOR;
        }
        else
            stopVoice();
    }

    it->second->LastPlayTime = now;
    devicePlaySound(it->second, finalVolume, pan, pitch);
}

void CAudioDriver::loopSound(const char* name, float volume, float pan, float pitch, float fade)
{
    ox::core::CString<char> key = name;
    SoundMap::iterator it = Sounds.find(key);
    if (it == Sounds.end())
    {
        if (!loadSound(name))
            return;
        it = Sounds.find(key);
        if (it == Sounds.end())
            return;
    }

    float finalVolume = volume;
    if (Voice)
    {
        if (deviceIsMusicPlaying(Voice))
            finalVolume = volume * VOICE_DAMPEN_FACTOR;
        else
            stopVoice();
    }

    deviceLoopSound(it->second, finalVolume, pan, pitch, fade);
}

void CAudioDriver::stopLoopSound(const char* name)
{
    ox::core::CString<char> key = name;
    SoundMap::iterator it = Sounds.find(key);
    if (it == Sounds.end())
        return;
    deviceStopLoopSound(it->second);
}

bool CAudioDriver::loadMusic(const char* name)
{
    ox::core::CString<char> key = name;
    MusicMap::iterator it = Music.find(key);
    if (it == Music.end())
    {
        CMusicInfoStub* music = deviceLoadMusic(name);
        if (!music)
            return false;
        Music[key] = music;
    }
    return true;
}

void CAudioDriver::playMusic(const char* name, float volume, bool loop)
{
    if (MusicGain <= 0.0f)
        return;

    ox::core::CString<char> key = name;
    MusicMap::iterator it = Music.find(key);
    if (it == Music.end())
    {
        if (!loadMusic(name))
            return;
        it = Music.find(key);
        if (it == Music.end())
            return;
    }

    it->second->Volume = volume;
    devicePlayMusic(it->second, volume, loop, false);
}

void CAudioDriver::playVoice(const char* name)
{
    CMusicInfoStub* previous = Voice;
    stopVoice();
    Voice = deviceLoadMusic(name);
    if (Voice)
    {
        devicePlayMusic(Voice, 1.0f, false, true);
        if (!previous && DampenSoundsOnVoice)
            deviceDampenAllSounds(VOICE_DAMPEN_FACTOR);
    }
}

bool CAudioDriver::isVoicePlaying()
{
    bool result = false;
    if (Voice)
    {
        result = true;
        if (!deviceIsMusicPlaying(Voice))
        {
            stopVoice();
            result = false;
        }
    }
    return result;
}

void CAudioDriver::stopVoice()
{
    if (Voice)
    {
        deviceStopMusic(Voice);
        delete Voice;
        Voice = 0;
    }
}

void CAudioDriver::stopMusic(const char* name)
{
    ox::core::CString<char> key = name;
    MusicMap::iterator it = Music.find(key);
    if (it != Music.end())
        deviceStopMusic(it->second);
}

void CAudioDriver::stopAllSounds()
{
    stopAllMusic();
    stopAllTrackedSounds();
    for (SoundMap::iterator it = Sounds.begin(); it != Sounds.end(); ++it)
        deviceStopSound(it->second);
}

bool CAudioDriver::updateMusic(const char* name, float volume)
{
    ox::core::CString<char> key = name;
    MusicMap::iterator it = Music.find(key);
    bool result = false;
    if (it != Music.end())
    {
        it->second->Volume = volume;
        result = deviceUpdateMusic(it->second, volume);
    }
    return result;
}

void CAudioDriver::setSoundEffectPath(const char* path)
{
    SoundEffectPath = path;
}

void CAudioDriver::setSoundEffectVolume(int volume)
{
    SoundEffectVolume = volume;
    SoundEffectGain = ox::core::clamp((float)(volume * 20) * 0.01f, 0.0f, 1.0f);
}

void CAudioDriver::setMusicVolume(int volume)
{
    MusicVolume = volume;
    MusicGain = ox::core::clamp((float)(volume * 20) * 0.01f, 0.0f, 1.0f);

    for (MusicMap::iterator it = Music.begin(); it != Music.end(); ++it)
        deviceUpdateMusic(it->second, it->second->Volume);
}

int CAudioDriver::getSoundEffectVolume()
{
    return SoundEffectVolume;
}

int CAudioDriver::getMusicVolume()
{
    return MusicVolume;
}

void CAudioDriver::stopAllMusic()
{
    for (MusicMap::iterator it = Music.begin(); it != Music.end(); ++it)
        deviceStopMusic(it->second);
}

void CAudioDriver::set2dOrientation()
{
    ox::core::CVector3d<float> forward(0.0f, 0.0f, 1.0f);
    ox::core::CVector3d<float> up(0.0f, -1.0f, 0.0f);
    deviceSetListenerOrientation(forward, up);
}

void CAudioDriver::set3dOrientation(const ox::core::CVector3d<float>& forward, const ox::core::CVector3d<float>& up)
{
    deviceSetListenerOrientation(forward, up);
}

void CAudioDriver::set3dCameraPosition(const ox::scene::ICameraSceneNode* camera)
{
    // OpenAL's z axis points the other way.
    ox::core::CVector3d<float> position = camera->getPosition();
    ox::core::CVector3d<float> forward = camera->getTarget() - position;
    ox::core::CVector3d<float> up = camera->getUpVector();
    position.Z = -position.Z;
    deviceSetListenerPosition(position);
    ListenerPosition = position;
    forward.Z = -forward.Z;
    up.Z = -up.Z;
    deviceSetListenerOrientation(forward, up);
}

void CAudioDriver::setListenerPosition(const ox::core::CVector3d<float>& position,
    const ox::core::CVector3d<float>& velocity)
{
    ListenerPosition = position;
    deviceSetListenerPosition(position);
    deviceSetListenerVelocity(velocity);
}

void CAudioDriver::setOrientedSoundCone(int innerAngle, int outerAngle, const ox::core::CVector3d<float>& direction)
{
    ConeInnerAngle = innerAngle;
    ConeOuterAngle = outerAngle;
    ConeDirection = direction;
}

void CAudioDriver::playOrientedSound(const char* name, float volume, float pitch,
    const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity)
{
    if (SoundEffectGain <= 0.0f)
        return;

    SoundMap::iterator it = findOrLoadSound(name);
    if (it == Sounds.end())
        return;

    unsigned int now = os::Timer::getTime();
    if (now - it->second->LastPlayTime < MIN_SOUND_REPEAT_MS)
        return;

    float finalVolume = volume;
    if (Voice)
    {
        if (deviceIsMusicPlaying(Voice))
        {
            if (DampenSoundsOnVoice)
                finalVolume = volume * VOICE_DAMPEN_FACTOR;
        }
        else
            stopVoice();
    }

    it->second->LastPlayTime = now;
    devicePlayOrientedSound(it->second, finalVolume, pitch, position, velocity);
}

void CAudioDriver::devicePlayOrientedSound(CSoundInfoStub* sound, float volume, float pitch,
    const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity)
{
    float pan;
    if (position.X < ListenerPosition.X)
        pan = 0.5f;
    else if (position.X > ListenerPosition.X)
        pan = -0.5f;
    else
        pan = 0.0f;
    devicePlaySound(sound, volume, pan, pitch);
}

int CAudioDriver::startTrackedSound(const char* name, float volume, float pitch,
    const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity, float fade)
{
    SoundMap::iterator it = findOrLoadSound(name);
    if (it == Sounds.end())
        return -1;
    CTrackedSoundInfoStub* tracked = deviceStartTrackedSound(it->second, volume, pitch, position, velocity, fade);
    if (!tracked)
        return -1;
    int handle = NextTrackedSoundHandle++;
    TrackedSounds[handle] = tracked;
    return handle;
}

void CAudioDriver::stopTrackedSound(int handle)
{
    TrackedSoundMap::iterator it = TrackedSounds.find(handle);
    if (it != TrackedSounds.end())
    {
        deviceStopTrackedSound(it->second);
        delete it->second;
        TrackedSounds.erase(it);
    }
}

void CAudioDriver::stopAllTrackedSounds()
{
    for (TrackedSoundMap::iterator it = TrackedSounds.begin(); it != TrackedSounds.end(); ++it)
    {
        deviceStopTrackedSound(it->second);
        delete it->second;
    }
    TrackedSounds.clear();
    NextTrackedSoundHandle = 1;
}

void CAudioDriver::updateTrackedSound(int handle, float volume, float pitch,
    const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity)
{
    TrackedSoundMap::iterator it = TrackedSounds.find(handle);
    if (it != TrackedSounds.end())
        deviceUpdateTrackedSound(it->second, volume, pitch, position, velocity);
}

} // end namespace audio
} // end namespace daisy
