// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CIrrDeviceLinux.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// The Linux 1.18 device is built on SFML 2 (window, events, joysticks) plus X11; the virtual order
// follows the Mac 1.18 vtable of daisy::CIrrDeviceMacOSX, the layout the Linux constructor (sizeof 0x360).

#ifndef DAISY_CIRRDEVICELINUX_H
#define DAISY_CIRRDEVICELINUX_H

#include <string>
#include <SFML/Window.hpp>
#include "CIrrDeviceStub.h"
#include "daisy/video/IImagePresenter.h"
#include "ox/gui/ICursorControl.h"

typedef struct _XDisplay Display;

namespace daisy {

class CIrrDeviceLinux : public CIrrDeviceStub, public video::IImagePresenter, public ox::gui::ICursorControl
{
public:
    CIrrDeviceLinux(ox::video::E_DRIVER_TYPE driverType, ox::event::IEventReceiver* receiver,
        const wchar_t* version);
    virtual ~CIrrDeviceLinux();

    virtual bool createDeviceWindow(const ox::core::CDimension2d<int>& windowSize, unsigned int bits,
        bool fullscreen, bool stencilBuffer, bool vsync, unsigned int antiAlias);
    virtual bool createUserSelectedDeviceWindow(const ox::TArray<ox::core::CString<wchar_t> >* languages,
        unsigned int flags);
    virtual int getSelectedLanguageIndex()
    {
        return SelectedLanguageIndex;
    }

    //! The driver cannot be changed after creation; returns true regardless.
    virtual bool setVideoDriver(ox::video::E_DRIVER_TYPE type)
    {
        return true;
    }

    virtual bool run();
    virtual bool swapBuffers();
    virtual bool setFullscreenMode(bool fullscreen);
    virtual void resizeDeviceWindow(const ox::core::CDimension2d<int>& size);
    virtual void setWindowCaption(const wchar_t* text);
    virtual bool isWindowActive();
    virtual void closeDevice();

    virtual ox::audio::IAudioDriver* createAudioDriver();
    virtual ox::input::IJoystickDriver* createJoystickDriver();

    // IImagePresenter
    virtual void present(ox::video::IImage* surface);

    // ICursorControl
    virtual void setVisible(bool visible);
    virtual bool isVisible();
    virtual void setPosition(const ox::core::CPosition2d<float>& position)
    {
        setPosition(position.X, position.Y);
    }

    //! Relative coordinates (0 to 1) are scaled by the window size.
    virtual void setPosition(float x, float y)
    {
        setPosition((int)(ScreenSize.Width * x), (int)(ScreenSize.Height * y));
    }

    virtual void setPosition(const ox::core::CPosition2d<int>& position)
    {
        setPosition(position.X, position.Y);
    }

    //! Warps the mouse to window pixels; CursorPos only follows with the next mouse-moved event.
    virtual void setPosition(int x, int y);

    //! The cursor position in window pixels, as of the last mouse event.
    virtual ox::core::CPosition2d<int> getPosition()
    {
        return CursorPos;
    }

    //! The cursor position divided by the window size.
    virtual ox::core::CPosition2d<float> getRelativePosition()
    {
        return RelativeCursorPos;
    }

    //! Whether a mouse button (0 left, 1 right, 2 middle) is held down.
    virtual bool getButtonState(int button)
    {
        return MouseButtonStates[button];
    }

    //! Opens a new connection to the default X display; CVideoOpenGL::loadExtensions calls it
    //! through the device pointer, so it is a member that ignores this.
    Display* openDisplay();
    //! An empty member nothing calls; its name is not known.
    void unknownEmpty();

private:
    //! The X display opened by the constructor; only getWindowSize uses it.
    static Display* XDisplay;

    void postJoystickConnectEvent(const sf::Event& event, bool connected);
    void postJoystickButtonEvent(const sf::Event& event, bool pressed);
    void postTextEvent(const sf::Event& event);
    void postKeyEvent(const sf::Event& event, bool pressed);
    void postMouseEvent(const sf::Event& event);
    //! Applies a new ScreenSize: camera aspect, cursor scale, driver viewport and the resize event.
    void updateScreenSize();
    //! The size of the X window as the server reports it.
    ox::core::CDimension2d<int> getWindowSize();
    //! Handles SFML's resize event (and mode switches): clamps windowed sizes, then updateScreenSize.
    void onResized(const ox::core::CDimension2d<int>& size);

    ox::core::CPosition2d<int> CursorPos;
    ox::core::CPosition2d<float> RelativeCursorPos;
    ox::core::CDimension2d<int> ScreenSize;
    //! The size to restore when leaving fullscreen; 0 until the first windowed resize.
    ox::core::CDimension2d<int> WindowedSize;
    ox::video::E_DRIVER_TYPE DriverType;
    bool MouseButtonStates[3];
    bool CursorVisible;
    int SelectedLanguageIndex;
    //! ox key codes by SFML key code (see the constructor); entries the constructor leaves out are
    //! never initialized.
    int KeyMap[sf::Keyboard::KeyCount];
    //! Never referenced.
    int Unused[16];
    sf::Window* Window;
    //! Unset until SFML reports the first focus change.
    bool WindowActive;
    std::string Title;
    //! Original bug: never initialized, so the first setFullscreenMode compares against heap garbage.
    bool Fullscreen;
};

} // end namespace daisy

#endif
