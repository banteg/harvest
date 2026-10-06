// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CIrrDeviceLinux.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye replaced Irrlicht's Xlib/GLX code with SFML 2 (a 2012 snapshot); X11 is only used to read
// the real window size, GTK 2 (in CLinuxOperator) only for the clipboard.

#include "CIrrDeviceLinux.h"
#include <unistd.h>
#include <X11/Xlib.h>
#include "CLinuxOperator.h"
#include "daisy/audio/COpenALDriver.h"
#include "daisy/input/CJoystickLinuxDriver.h"
#include "ox/io/IFileSystem.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ICameraSceneNode.h"
#include "ox/video/IVideoDriver.h"

// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace video {
ox::video::IVideoDriver* createNullDriver(ox::io::IFileSystem* io, const ox::core::CDimension2d<int>& screenSize);
ox::video::IVideoDriver* createSoftwareDriver(const ox::core::CDimension2d<int>& windowSize, bool fullscreen,
    ox::io::IFileSystem* io, IImagePresenter* presenter);
ox::video::IVideoDriver* createOpenGLDriver(const ox::core::CDimension2d<int>& screenSize, CIrrDeviceLinux* device,
    bool fullscreen, bool stencilBuffer, ox::io::IFileSystem* io);
} // end namespace video

Display* CIrrDeviceLinux::XDisplay;

void CIrrDeviceLinux::unknownEmpty()
{
}

//! Software frames are never shown on Linux.
void CIrrDeviceLinux::present(ox::video::IImage* surface)
{
}

bool CIrrDeviceLinux::isWindowActive()
{
    return WindowActive;
}

bool CIrrDeviceLinux::isVisible()
{
    return CursorVisible;
}

CIrrDeviceLinux::CIrrDeviceLinux(ox::video::E_DRIVER_TYPE driverType, ox::event::IEventReceiver* receiver,
    const wchar_t* version)
    : CIrrDeviceStub(version, receiver), CursorPos(0, 0), RelativeCursorPos(0, 0), ScreenSize(0, 0),
      WindowedSize(0, 0), DriverType(driverType), CursorVisible(true)
{
    XDisplay = XOpenDisplay(0);

    static_cast<ox::gui::ICursorControl*>(this)->grab();
    MouseButtonStates[0] = false;
    MouseButtonStates[1] = false;
    MouseButtonStates[2] = false;
    CursorControl = this;

    if (Operator)
    {
        Operator->drop();
        Operator = 0;
    }
    Operator = new CLinuxOperator();

    // $GAME_RESOURCES$ is the directory of the executable. readlink does not terminate the path, so
    // the scan starts at an unwritten byte.
    char path[1024];
    int i = readlink("/proc/self/exe", path, 1023);
    while (path[i] != '/' && i >= 0)
        --i;
    path[i] = 0;
    FileSystem->addDirectoryAlias("$GAME_RESOURCES$", path);

    // The resolutions offered in the settings; a mode is listed only when it is strictly smaller than
    // the desktop in both dimensions, so the desktop size itself is never offered. All are 32 bits.
    // The desktop mode is never stored (getDesktopResolution stays 0x0).
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    int modes[][2] =
    {
        { 2048, 1536 }, { 1900, 1200 }, { 1920, 1080 }, { 1600, 1200 }, { 1680, 1050 }, { 1440, 1050 },
        { 1440, 960 }, { 1440, 900 }, { 1280, 800 }, { 1280, 768 }, { 1280, 720 }, { 1152, 768 },
        { 1024, 768 }, { 800, 600 }, { 0, 0 }
    };
    for (int m = 0; modes[m][0] > 0; ++m)
        if (modes[m][0] < desktop.width && modes[m][1] < desktop.height)
            VideoModeList.addMode(ox::core::CDimension2d<int>(modes[m][0], modes[m][1]), 32);

    // SFML key codes to ox (Windows virtual-key) codes. Keys not listed (Insert, Pause, the control,
    // alt and system keys, right shift, menu, brackets, semicolon, quote, slash, backslash, tilde,
    // equal) keep uninitialized table entries. Left shift maps to KEY_CAPITAL.
    KeyMap[sf::Keyboard::Num1] = ox::KEY_KEY_1;
    KeyMap[sf::Keyboard::Num2] = ox::KEY_KEY_2;
    KeyMap[sf::Keyboard::Num3] = ox::KEY_KEY_3;
    KeyMap[sf::Keyboard::Num4] = ox::KEY_KEY_4;
    KeyMap[sf::Keyboard::Num5] = ox::KEY_KEY_5;
    KeyMap[sf::Keyboard::Num6] = ox::KEY_KEY_6;
    KeyMap[sf::Keyboard::Num7] = ox::KEY_KEY_7;
    KeyMap[sf::Keyboard::Num8] = ox::KEY_KEY_8;
    KeyMap[sf::Keyboard::Num9] = ox::KEY_KEY_9;
    KeyMap[sf::Keyboard::Num0] = ox::KEY_KEY_0;
    KeyMap[sf::Keyboard::BackSpace] = ox::KEY_BACK;
    KeyMap[sf::Keyboard::Dash] = ox::KEY_MINUS;
    KeyMap[sf::Keyboard::Space] = ox::KEY_SPACE;
    KeyMap[sf::Keyboard::Comma] = ox::KEY_COMMA;
    KeyMap[sf::Keyboard::Period] = ox::KEY_PERIOD;
    KeyMap[sf::Keyboard::Escape] = ox::KEY_ESCAPE;
    KeyMap[sf::Keyboard::LShift] = ox::KEY_CAPITAL;
    KeyMap[sf::Keyboard::Tab] = ox::KEY_TAB;
    KeyMap[sf::Keyboard::Return] = ox::KEY_RETURN;
    KeyMap[sf::Keyboard::B] = ox::KEY_KEY_B;
    KeyMap[sf::Keyboard::A] = ox::KEY_KEY_A;
    KeyMap[sf::Keyboard::C] = ox::KEY_KEY_C;
    KeyMap[sf::Keyboard::D] = ox::KEY_KEY_D;
    KeyMap[sf::Keyboard::E] = ox::KEY_KEY_E;
    KeyMap[sf::Keyboard::F] = ox::KEY_KEY_F;
    KeyMap[sf::Keyboard::G] = ox::KEY_KEY_G;
    KeyMap[sf::Keyboard::H] = ox::KEY_KEY_H;
    KeyMap[sf::Keyboard::I] = ox::KEY_KEY_I;
    KeyMap[sf::Keyboard::J] = ox::KEY_KEY_J;
    KeyMap[sf::Keyboard::K] = ox::KEY_KEY_K;
    KeyMap[sf::Keyboard::L] = ox::KEY_KEY_L;
    KeyMap[sf::Keyboard::M] = ox::KEY_KEY_M;
    KeyMap[sf::Keyboard::N] = ox::KEY_KEY_N;
    KeyMap[sf::Keyboard::O] = ox::KEY_KEY_O;
    KeyMap[sf::Keyboard::P] = ox::KEY_KEY_P;
    KeyMap[sf::Keyboard::Q] = ox::KEY_KEY_Q;
    KeyMap[sf::Keyboard::R] = ox::KEY_KEY_R;
    KeyMap[sf::Keyboard::S] = ox::KEY_KEY_S;
    KeyMap[sf::Keyboard::T] = ox::KEY_KEY_T;
    KeyMap[sf::Keyboard::U] = ox::KEY_KEY_U;
    KeyMap[sf::Keyboard::V] = ox::KEY_KEY_V;
    KeyMap[sf::Keyboard::W] = ox::KEY_KEY_W;
    KeyMap[sf::Keyboard::X] = ox::KEY_KEY_X;
    KeyMap[sf::Keyboard::Y] = ox::KEY_KEY_Y;
    KeyMap[sf::Keyboard::Z] = ox::KEY_KEY_Z;
    KeyMap[sf::Keyboard::F1] = ox::KEY_F1;
    KeyMap[sf::Keyboard::F2] = ox::KEY_F2;
    KeyMap[sf::Keyboard::F3] = ox::KEY_F3;
    KeyMap[sf::Keyboard::F4] = ox::KEY_F4;
    KeyMap[sf::Keyboard::F5] = ox::KEY_F5;
    KeyMap[sf::Keyboard::F6] = ox::KEY_F6;
    KeyMap[sf::Keyboard::F7] = ox::KEY_F7;
    KeyMap[sf::Keyboard::F8] = ox::KEY_F8;
    KeyMap[sf::Keyboard::F9] = ox::KEY_F9;
    KeyMap[sf::Keyboard::F10] = ox::KEY_F10;
    KeyMap[sf::Keyboard::F11] = ox::KEY_F11;
    KeyMap[sf::Keyboard::F12] = ox::KEY_F12;
    KeyMap[sf::Keyboard::F13] = ox::KEY_F13;
    KeyMap[sf::Keyboard::F14] = ox::KEY_F14;
    KeyMap[sf::Keyboard::F15] = ox::KEY_F15;
    KeyMap[sf::Keyboard::Numpad0] = ox::KEY_NUMPAD0;
    KeyMap[sf::Keyboard::Numpad1] = ox::KEY_NUMPAD1;
    KeyMap[sf::Keyboard::Numpad2] = ox::KEY_NUMPAD2;
    KeyMap[sf::Keyboard::Numpad3] = ox::KEY_NUMPAD3;
    KeyMap[sf::Keyboard::Numpad4] = ox::KEY_NUMPAD4;
    KeyMap[sf::Keyboard::Numpad5] = ox::KEY_NUMPAD5;
    KeyMap[sf::Keyboard::Numpad6] = ox::KEY_NUMPAD6;
    KeyMap[sf::Keyboard::Numpad7] = ox::KEY_NUMPAD7;
    KeyMap[sf::Keyboard::Numpad8] = ox::KEY_NUMPAD8;
    KeyMap[sf::Keyboard::Numpad9] = ox::KEY_NUMPAD9;
    KeyMap[sf::Keyboard::Add] = ox::KEY_ADD;
    KeyMap[sf::Keyboard::Subtract] = ox::KEY_SUBTRACT;
    KeyMap[sf::Keyboard::Divide] = ox::KEY_DIVIDE;
    KeyMap[sf::Keyboard::Multiply] = ox::KEY_MULTIPLY;
    KeyMap[sf::Keyboard::Up] = ox::KEY_UP;
    KeyMap[sf::Keyboard::Down] = ox::KEY_DOWN;
    KeyMap[sf::Keyboard::Left] = ox::KEY_LEFT;
    KeyMap[sf::Keyboard::Right] = ox::KEY_RIGHT;
    KeyMap[sf::Keyboard::PageUp] = ox::KEY_PRIOR;
    KeyMap[sf::Keyboard::PageDown] = ox::KEY_NEXT;
    KeyMap[sf::Keyboard::Home] = ox::KEY_HOME;
    KeyMap[sf::Keyboard::End] = ox::KEY_END;
    KeyMap[sf::Keyboard::Delete] = ox::KEY_DELETE;
}

//! Declared in ox/IOxDevice.h; C linkage makes this the same function as ox::createDevice.
extern "C" ox::IOxDevice* createDevice(ox::video::E_DRIVER_TYPE driverType, ox::event::IEventReceiver* receiver,
    const wchar_t* version)
{
    return new CIrrDeviceLinux(driverType, receiver, version);
}

//! There is no options dialog on Linux: the window is 3/4 of the largest 4:3 box that fits the
//! desktop, windowed, and the language is English (the index of "English" in languages, or the
//! array size when it is missing).
bool CIrrDeviceLinux::createUserSelectedDeviceWindow(const ox::TArray<ox::core::CString<wchar_t> >* languages,
    unsigned int flags)
{
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    unsigned int width = desktop.width;
    unsigned int height = desktop.height;
    if (width > height)
        width = (unsigned int)(height * (4.0 / 3.0));
    else
        height = (unsigned int)(width * 0.75);

    ox::core::CDimension2d<int> windowSize((int)(width * 0.75), (int)(height * 0.75));

    ox::core::CString<wchar_t> english(L"English");
    for (SelectedLanguageIndex = 0; SelectedLanguageIndex < languages->size(); ++SelectedLanguageIndex)
        if ((*languages)[SelectedLanguageIndex] == english)
            break;

    return createDeviceWindow(windowSize, 32, false, false, false, flags);
}

ox::audio::IAudioDriver* CIrrDeviceLinux::createAudioDriver()
{
    if (!AudioDriver)
        AudioDriver = new audio::COpenALDriver();
    return AudioDriver;
}

ox::input::IJoystickDriver* CIrrDeviceLinux::createJoystickDriver()
{
    if (!JoystickDriver)
        JoystickDriver = new input::CJoystickLinuxDriver();
    return JoystickDriver;
}

//! Original bug: declared to return bool (the Mac device returns true) but returns nothing.
bool CIrrDeviceLinux::swapBuffers()
{
    Window->display();
}

//! Recreates the SFML window: fullscreen in SFML's first (best) fullscreen mode, windowed at the
//! remembered windowed size or, before there is one, the desktop size. The video driver is told the
//! new mode and the new size goes through onResized.
bool CIrrDeviceLinux::setFullscreenMode(bool fullscreen)
{
    if (Fullscreen == fullscreen)
        return false;

    Fullscreen = fullscreen;
    sf::Uint32 style = fullscreen ? sf::Style::Fullscreen : sf::Style::Default;

    sf::VideoMode mode;
    if (Fullscreen)
        mode = sf::VideoMode::getFullscreenModes()[0];
    else if (WindowedSize.Width > 0)
        mode = sf::VideoMode(WindowedSize.Width, WindowedSize.Height, 32);
    else
        mode = sf::VideoMode::getDesktopMode();

    Window->create(mode, Title, style, sf::ContextSettings());
    getVideoDriver()->setFullscreen(fullscreen);
    onResized(ox::core::CDimension2d<int>(mode.width, mode.height));
    return true;
}

//! Polls the joysticks and all pending SFML events; returns false once the window is closed.
bool CIrrDeviceLinux::run()
{
    if (JoystickDriver)
        static_cast<input::CJoystickLinuxDriver*>(JoystickDriver)->update();

    sf::Event event;
    while (Window->pollEvent(event))
    {
        switch (event.type)
        {
        case sf::Event::Closed:
            Window->close();
            break;
        case sf::Event::Resized:
            onResized(ox::core::CDimension2d<int>(event.size.width, event.size.height));
            break;
        case sf::Event::LostFocus:
            WindowActive = false;
            break;
        case sf::Event::GainedFocus:
            WindowActive = true;
            break;
        case sf::Event::TextEntered:
            postTextEvent(event);
            break;
        case sf::Event::KeyPressed:
            postKeyEvent(event, true);
            break;
        case sf::Event::KeyReleased:
            postKeyEvent(event, false);
            break;
        case sf::Event::MouseWheelMoved:
        case sf::Event::MouseButtonPressed:
        case sf::Event::MouseButtonReleased:
        case sf::Event::MouseMoved:
            postMouseEvent(event);
            break;
        case sf::Event::JoystickButtonPressed:
            postJoystickButtonEvent(event, true);
            break;
        case sf::Event::JoystickDisconnected:
            postJoystickConnectEvent(event, false);
            break;
        case sf::Event::JoystickButtonReleased:
            postJoystickButtonEvent(event, false);
            // Original bug: no break, so every button release also posts a "connected" event.
        case sf::Event::JoystickConnected:
            postJoystickConnectEvent(event, true);
            break;
        default:
            break;
        }
    }

    return Window->isOpen();
}

//! A windowed size is forced into aspect ratios from 4:3 to 16:9 (a narrower window gets its height
//! cut to width * 3/4, a wider one its width cut to height * 16/9) and to at least 800x600, then
//! the window is resized to it. If the window manager does not grant that size, the granted size
//! wins; if it does, nothing more happens here (the size is applied when SFML reports the resize).
void CIrrDeviceLinux::onResized(const ox::core::CDimension2d<int>& size)
{
    if (ScreenSize == size)
        return;

    ScreenSize = size;

    if (!Fullscreen)
    {
        float width = (float)ScreenSize.Width;
        float height = (float)ScreenSize.Height;
        float aspect = width / height;

        if (aspect < 4.0 / 3.0)
            ScreenSize.Height = (int)(width * 0.75);
        else if (aspect > 16.0 / 9.0)
            ScreenSize.Width = (int)(height * (16.0 / 9.0));

        ScreenSize.Width = ScreenSize.Width < 800 ? 800 : ScreenSize.Width;
        ScreenSize.Height = ScreenSize.Height < 600 ? 600 : ScreenSize.Height;
    }

    if (ScreenSize != size)
    {
        Window->setSize(sf::Vector2u(ScreenSize.Width, ScreenSize.Height));
        ox::core::CDimension2d<int> actual = getWindowSize();
        if (actual != ScreenSize)
        {
            ScreenSize = actual;
            updateScreenSize();
        }
    }
    else
        updateScreenSize();
}

ox::core::CDimension2d<int> CIrrDeviceLinux::getWindowSize()
{
    usleep(10);
    XWindowAttributes attributes;
    XGetWindowAttributes(XDisplay, Window->getSystemHandle(), &attributes);
    return ox::core::CDimension2d<int>(attributes.width, attributes.height);
}

//! In fullscreen only the size to restore is recorded; windowed, the SFML window is resized (the
//! resulting resize event goes through onResized).
void CIrrDeviceLinux::resizeDeviceWindow(const ox::core::CDimension2d<int>& size)
{
    if (Fullscreen)
        WindowedSize = size;
    else
        Window->setSize(sf::Vector2u(size.Width, size.Height));
}

//! SFML mouse events. Positions are window pixels from the top left. Buttons map Left/Right/Middle
//! to the L/R/M events (other SFML buttons index past them). A press carries the click count of
//! getClickCount in MouseInput.Clicks, a release the current count. The wheel sends EMIE_MOUSE_WHEEL
//! with ScrollY = delta * 10 (one notch is 10) and leaves Wheel and ScrollX unset. Every event
//! updates the cursor position, but only events inside the window are posted.
void CIrrDeviceLinux::postMouseEvent(const sf::Event& event)
{
    ox::event::SEvent ev;
    ev.EventType = ox::event::EET_MOUSE_INPUT_EVENT;
    ev.MouseInput.X = event.mouseButton.x;
    ev.MouseInput.Y = event.mouseButton.y;

    if (event.type == sf::Event::MouseButtonPressed)
    {
        ev.MouseInput.Event = (ox::event::EMOUSE_INPUT_EVENT)event.mouseButton.button;
        MouseButtonStates[event.mouseButton.button] = true;
        ev.MouseInput.Clicks = getClickCount(event.mouseButton.button,
            ox::core::CPosition2d<int>(ev.MouseInput.X, ev.MouseInput.Y));
    }
    else if (event.type == sf::Event::MouseMoved)
    {
        ev.MouseInput.X = event.mouseMove.x;
        ev.MouseInput.Y = event.mouseMove.y;
        ev.MouseInput.Event = ox::event::EMIE_MOUSE_MOVED;
    }
    else if (event.type == sf::Event::MouseWheelMoved)
    {
        ev.MouseInput.Event = ox::event::EMIE_MOUSE_WHEEL;
        ev.MouseInput.ScrollY = event.mouseWheel.delta * 10;
    }
    else
    {
        ev.MouseInput.Event = (ox::event::EMOUSE_INPUT_EVENT)(event.mouseButton.button + ox::event::EMIE_LMOUSE_LEFT_UP);
        MouseButtonStates[event.mouseButton.button] = false;
        ev.MouseInput.Clicks = ClickCount;
    }

    CursorPos.X = ev.MouseInput.X;
    CursorPos.Y = ev.MouseInput.Y;
    RelativeCursorPos.X = ev.MouseInput.X / (float)ScreenSize.Width;
    RelativeCursorPos.Y = ev.MouseInput.Y / (float)ScreenSize.Height;

    if (ev.MouseInput.X >= 0 && ev.MouseInput.Y >= 0 && ev.MouseInput.X < Window->getSize().x
        && ev.MouseInput.Y < Window->getSize().y)
        postEventFromUser(ev);
}

void CIrrDeviceLinux::updateScreenSize()
{
    int height = ScreenSize.Height;
    int width = ScreenSize.Width;

    // The camera's aspect ratio is set to height / width.
    if (SceneManager && SceneManager->getActiveCamera())
        SceneManager->getActiveCamera()->setAspectRatio((float)height / (float)width);

    RelativeCursorPos.X = CursorPos.X / (float)ScreenSize.Width;
    RelativeCursorPos.Y = CursorPos.Y / (float)ScreenSize.Height;

    getVideoDriver()->OnResize(ScreenSize);

    ox::event::SEvent ev;
    ev.EventType = ox::event::EET_DEVICE_EVENT;
    ev.DeviceEvent.Type = ox::event::EDE_FULLSCREEN_TOGGLED;
    ev.DeviceEvent.Width = ScreenSize.Width;
    ev.DeviceEvent.Height = ScreenSize.Height;
    postEventFromUser(ev);

    if (!Fullscreen)
        WindowedSize = ScreenSize;
}

//! SFML KeyPressed/KeyReleased: KeyInput.Key is KeyMap[code] (no bounds check: SFML's Unknown key,
//! -1, reads the field before the table, SelectedLanguageIndex), Char is 0. With Control held, a
//! press of V becomes EKIE_PASTE, F becomes EKIE_TOGGLE_FULLSCREEN and Q closes the device without
//! an event; a press of any other key with Control leaves KeyInput.Event unset. Releases are
//! always EKIE_KEY_LEFT_UP.
void CIrrDeviceLinux::postKeyEvent(const sf::Event& event, bool pressed)
{
    ox::event::SEvent ev;
    ev.EventType = ox::event::EET_KEY_INPUT_EVENT;
    ev.KeyInput.Shift = event.key.shift;
    ev.KeyInput.Control = event.key.control;
    ev.KeyInput.Key = (ox::EKEY_CODE)KeyMap[event.key.code];
    ev.KeyInput.Char = 0;

    if (event.key.control && pressed)
    {
        if (ev.KeyInput.Key == ox::KEY_KEY_V)
            ev.KeyInput.Event = ox::event::EKIE_PASTE;
        else if (ev.KeyInput.Key == ox::KEY_KEY_F)
            ev.KeyInput.Event = ox::event::EKIE_TOGGLE_FULLSCREEN;
        else if (ev.KeyInput.Key == ox::KEY_KEY_Q)
        {
            closeDevice();
            return;
        }
    }
    else
        ev.KeyInput.Event = pressed ? ox::event::EKIE_KEY_PRESSED_DOWN : ox::event::EKIE_KEY_LEFT_UP;

    postEventFromUser(ev);
}

//! SFML TextEntered: an EKIE_CHARACTER key event with the UTF-32 character in KeyInput.Char.
//! Control characters (below 32) and DEL (127) are dropped; Key, Shift and Control are left unset.
void CIrrDeviceLinux::postTextEvent(const sf::Event& event)
{
    if (event.text.unicode > 31 && event.text.unicode != 127)
    {
        ox::event::SEvent ev;
        ev.EventType = ox::event::EET_KEY_INPUT_EVENT;
        ev.KeyInput.Char = event.text.unicode;
        ev.KeyInput.Event = ox::event::EKIE_CHARACTER;
        postEventFromUser(ev);
    }
}

//! SFML joystick buttons: type 0 for a press, 1 for a release, with SFML's joystick and button
//! numbers.
void CIrrDeviceLinux::postJoystickButtonEvent(const sf::Event& event, bool pressed)
{
    ox::event::SEvent ev;
    ev.EventType = ox::event::EET_JOYSTICK_INPUT_EVENT;
    ev.JoystickEvent.Type = !pressed;
    ev.JoystickEvent.Joystick = event.joystickButton.joystickId;
    ev.JoystickEvent.Button = event.joystickButton.button;
    postEventFromUser(ev);
}

//! SFML JoystickConnected/JoystickDisconnected: a joystick event of type 3 (connected) or 4
//! (disconnected) with the joystick number and no button.
void CIrrDeviceLinux::postJoystickConnectEvent(const sf::Event& event, bool connected)
{
    ox::event::SEvent ev;
    ev.EventType = ox::event::EET_JOYSTICK_INPUT_EVENT;
    ev.JoystickEvent.Type = connected ? 3 : 4;
    ev.JoystickEvent.Joystick = event.joystickConnect.joystickId;
    postEventFromUser(ev);
}

//! The title is the caption narrowed character by character (no UTF-8 encoding).
void CIrrDeviceLinux::setWindowCaption(const wchar_t* text)
{
    Title.clear();
    while (*text)
    {
        Title += (char)*text;
        ++text;
    }
    Window->setTitle(Title);
}

//! Closes and deletes the SFML window. Window is not reset: the destructor calls this again, and
//! run() keeps polling the deleted window after Ctrl+Q.
void CIrrDeviceLinux::closeDevice()
{
    Window->close();
    delete Window;
}

Display* CIrrDeviceLinux::openDisplay()
{
    return XOpenDisplay(0);
}

void CIrrDeviceLinux::setVisible(bool visible)
{
    CursorVisible = visible;
    Window->setMouseCursorVisible(visible);
}

//! Warps the mouse; CursorPos follows with the next mouse-moved event only.
void CIrrDeviceLinux::setPosition(int x, int y)
{
    sf::Mouse::setPosition(sf::Vector2i(x, y), *Window);
}

CIrrDeviceLinux::~CIrrDeviceLinux()
{
    closeDevice();
}

//! The window is created windowed at windowSize (no title, vsync as asked, key repeat on) and then
//! switched with setFullscreenMode. The null driver type creates no window. bits and antiAlias are
//! ignored; OpenGL textures are created without mipmaps.
bool CIrrDeviceLinux::createDeviceWindow(const ox::core::CDimension2d<int>& windowSize, unsigned int bits,
    bool fullscreen, bool stencilBuffer, bool vsync, unsigned int antiAlias)
{
    if (DriverType == ox::video::EDT_NULL)
        return false;

    Window = new sf::Window(sf::VideoMode(windowSize.Width, windowSize.Height, 32), "", sf::Style::Default,
        sf::ContextSettings());
    Window->setVerticalSyncEnabled(vsync);
    Window->setKeyRepeatEnabled(true);
    Window->setVisible(true);

    switch (DriverType)
    {
    case ox::video::EDT_NULL:
        VideoDriver = video::createNullDriver(FileSystem, windowSize);
        break;
    case ox::video::EDT_SOFTWARE:
        VideoDriver = video::createSoftwareDriver(windowSize, fullscreen, FileSystem, this);
        break;
    case ox::video::EDT_OPENGL:
        VideoDriver = video::createOpenGLDriver(windowSize, this, fullscreen, stencilBuffer, FileSystem);
        // ETCF_CREATE_MIP_MAPS
        VideoDriver->setTextureCreationFlag((ox::video::E_TEXTURE_CREATION_FLAG)0x10, false);
        break;
    default:
        break;
    }

    setFullscreenMode(fullscreen);
    createGUIAndScene();
    return true;
}

} // end namespace daisy
