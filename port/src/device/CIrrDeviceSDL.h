// The port's device: CIrrDeviceLinux's role on SDL3. See docs/port/input-and-window.md.

#ifndef PORT_DEVICE_CIRRDEVICESDL_H
#define PORT_DEVICE_CIRRDEVICESDL_H

#include <SDL3/SDL.h>
#include "daisy/other/CIrrDeviceStub.h"
#include "ox/gui/ICursorControl.h"

namespace port {

class CJoystickSDLDriver;
class CSDLOperator;

//! The window, its OpenGL context and the input of the game, on SDL3.
//!
//! Events arrive through handleEvent (main.cpp's SDL_AppEvent, between frames) instead of being
//! pumped by run(), which only reports whether the device is still open. The window is created with
//! an OpenGL ES 3.0 context (OpenGL 3.3 core on macOS, and wherever ES is unavailable) and the
//! renderer from port::createVideoDriver.
class CIrrDeviceSDL : public daisy::CIrrDeviceStub, public ox::gui::ICursorControl
{
public:
    CIrrDeviceSDL(ox::video::E_DRIVER_TYPE driverType, ox::event::IEventReceiver* receiver,
        const wchar_t* version);
    virtual ~CIrrDeviceSDL();

    //! The device createDevice made last, or 0; main.cpp hands it the SDL events.
    static CIrrDeviceSDL* getInstance() { return Instance; }

    //! Turns one SDL event into ox events.
    void handleEvent(const SDL_Event& event);

    virtual bool createDeviceWindow(const ox::core::CDimension2d<int>& windowSize, unsigned int bits,
        bool fullscreen, bool stencilBuffer, bool vsync, unsigned int antiAlias);
    virtual bool createUserSelectedDeviceWindow(const ox::TArray<ox::core::CString<wchar_t> >* languages,
        unsigned int flags);
    virtual int getSelectedLanguageIndex() { return SelectedLanguageIndex; }
    //! The driver cannot be changed after creation; returns true regardless, as on Linux.
    virtual bool setVideoDriver(ox::video::E_DRIVER_TYPE type) { return true; }

    virtual bool run();
    virtual bool swapBuffers();
    virtual bool setFullscreenMode(bool fullscreen);
    virtual void resizeDeviceWindow(const ox::core::CDimension2d<int>& size);
    virtual void setWindowCaption(const wchar_t* text);
    virtual bool isWindowActive() { return WindowActive; }
    virtual void closeDevice();

    virtual ox::audio::IAudioDriver* createAudioDriver();
    virtual ox::input::IJoystickDriver* createJoystickDriver();

    // ICursorControl
    virtual void setVisible(bool visible);
    virtual bool isVisible() { return CursorVisible; }
    virtual void setPosition(const ox::core::CPosition2d<float>& position) { setPosition(position.X, position.Y); }
    virtual void setPosition(float x, float y)
    {
        setPosition((int)(ScreenSize.Width * x), (int)(ScreenSize.Height * y));
    }
    virtual void setPosition(const ox::core::CPosition2d<int>& position) { setPosition(position.X, position.Y); }
    //! Warps the pointer to render pixels; CursorPos follows with the next motion event only.
    virtual void setPosition(int x, int y);
    virtual ox::core::CPosition2d<int> getPosition() { return CursorPos; }
    virtual ox::core::CPosition2d<float> getRelativePosition() { return RelativeCursorPos; }
    virtual bool getButtonState(int button) { return button >= 0 && button < 3 && MouseButtonStates[button]; }

private:
    //! Creates the window and a current OpenGL context of the given profile; false if either fails.
    bool createWindowAndContext(const ox::core::CDimension2d<int>& size, bool fullscreen, bool stencilBuffer,
        int profile, int major, int minor);
    //! The render size of the window in pixels.
    ox::core::CDimension2d<int> getPixelSize();
    //! Window coordinates (SDL's mouse positions) to render pixels.
    ox::core::CPosition2d<int> toPixels(float x, float y);
    //! Applies a size the window now has: clamps windowed sizes (asking the window for the clamped
    //! size when it differs), then updateScreenSize.
    void onResized(const ox::core::CDimension2d<int>& size);
    //! Applies ScreenSize: camera aspect, cursor scale, driver viewport and the resize event.
    void updateScreenSize();

    void postKeyEvent(const SDL_KeyboardEvent& event);
    void postTextEvent(const SDL_TextInputEvent& event);
    void postMouseEvent(const SDL_Event& event);

    static CIrrDeviceSDL* Instance;

    ox::video::E_DRIVER_TYPE DriverType;
    SDL_Window* Window;
    SDL_GLContext Context;
    CSDLOperator* SDLOperator;
    CJoystickSDLDriver* SDLJoystickDriver;
    //! The caption as UTF-8, kept for a window created after setWindowCaption.
    ox::core::CString<char> Title;

    ox::core::CPosition2d<int> CursorPos;
    ox::core::CPosition2d<float> RelativeCursorPos;
    //! The render size in pixels.
    ox::core::CDimension2d<int> ScreenSize;
    //! The window size to restore when leaving fullscreen; 0 until known.
    ox::core::CDimension2d<int> WindowedSize;
    bool MouseButtonStates[3];
    bool CursorVisible;
    int SelectedLanguageIndex;
    bool WindowActive;
    bool Fullscreen;
    bool Closed;
};

} // end namespace port

#endif
