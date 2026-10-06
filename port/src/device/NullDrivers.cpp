#include "device/NullDrivers.h"

#include <SDL3/SDL.h>
#include "daisy/audio/CAudioDriver.h"
#include "daisy/video/Null/CVideoNull.h"
#include "ox/IOxDevice.h"

namespace port {

namespace {

class CNullVideoDriver : public daisy::video::CVideoNull
{
public:
    CNullVideoDriver(ox::IOxDevice* device, ox::io::IFileSystem* fileSystem,
        const ox::core::CDimension2d<int>& screenSize)
        : daisy::video::CVideoNull(fileSystem, screenSize), Device(device), Fullscreen(false)
    {
        ClearColor = (ClearColorFunc)SDL_GL_GetProcAddress("glClearColor");
        Clear = (ClearFunc)SDL_GL_GetProcAddress("glClear");
    }

    virtual bool beginScene(bool backBuffer, bool zBuffer, ox::video::SColor color)
    {
        CVideoNull::beginScene(backBuffer, zBuffer, color);
        if (backBuffer && ClearColor && Clear)
        {
            ClearColor(color.getRed() / 255.0f, color.getGreen() / 255.0f, color.getBlue() / 255.0f,
                color.getAlpha() / 255.0f);
            Clear(GL_COLOR_BUFFER_BIT);
        }
        return true;
    }

    virtual bool endScene()
    {
        CVideoNull::endScene();
        return Device->swapBuffers();
    }

    //! CHarvestFullMain::init accepts only OpenGL and DirectX 9 drivers on the first run.
    virtual int getDriverType() { return ox::video::EDT_OPENGL; }

    virtual bool isFullscreen() { return Fullscreen; }
    virtual bool setFullscreen(bool fullscreen)
    {
        Fullscreen = fullscreen;
        return true;
    }

private:
    enum { GL_COLOR_BUFFER_BIT = 0x4000 };
    typedef void (SDLCALL* ClearColorFunc)(float, float, float, float);
    typedef void (SDLCALL* ClearFunc)(unsigned int);

    ox::IOxDevice* Device;
    bool Fullscreen;
    ClearColorFunc ClearColor;
    ClearFunc Clear;
};

class CNullAudioDriver : public daisy::audio::CAudioDriver
{
protected:
    virtual daisy::audio::CSoundInfoStub* deviceLoadSound(const char* name) { return 0; }
    virtual void devicePlaySound(daisy::audio::CSoundInfoStub* sound, float volume, float pan, float pitch) {}
    virtual void deviceDampenAllSounds(float factor) {}
    virtual daisy::audio::CMusicInfoStub* deviceLoadMusic(const char* name) { return 0; }
    virtual void devicePlayMusic(daisy::audio::CMusicInfoStub* music, float volume, bool loop, bool voice) {}
    virtual void deviceStopMusic(daisy::audio::CMusicInfoStub* music) {}
    virtual bool deviceUpdateMusic(daisy::audio::CMusicInfoStub* music, float frameDelta) { return false; }
    virtual bool deviceIsMusicPlaying(daisy::audio::CMusicInfoStub* music) { return false; }
    virtual void deviceSetListenerPosition(const ox::core::CVector3d<float>& position) {}
    virtual void deviceSetListenerVelocity(const ox::core::CVector3d<float>& velocity) {}
    virtual void deviceSetListenerOrientation(const ox::core::CVector3d<float>& forward,
        const ox::core::CVector3d<float>& up)
    {
    }
    virtual daisy::audio::CTrackedSoundInfoStub* deviceStartTrackedSound(daisy::audio::CSoundInfoStub* sound,
        float volume, float pitch, const ox::core::CVector3d<float>& position,
        const ox::core::CVector3d<float>& velocity, float fade)
    {
        return 0;
    }
    virtual void deviceStopTrackedSound(daisy::audio::CTrackedSoundInfoStub* sound) {}
    virtual void deviceUpdateTrackedSound(daisy::audio::CTrackedSoundInfoStub* sound, float volume, float pitch,
        const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& velocity)
    {
    }
};

} // end namespace

ox::video::IVideoDriver* createNullVideoDriver(ox::IOxDevice* device, ox::io::IFileSystem* fileSystem,
    const ox::core::CDimension2d<int>& screenSize)
{
    return new CNullVideoDriver(device, fileSystem, screenSize);
}

ox::audio::IAudioDriver* createNullAudioDriver()
{
    return new CNullAudioDriver();
}

} // end namespace port
