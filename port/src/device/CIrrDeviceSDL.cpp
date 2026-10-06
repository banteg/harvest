// CIrrDeviceLinux (src/daisy/other/CIrrDeviceLinux.cpp) is the behavioural reference; the Linux
// device's bugs (docs/port/original-bugs.md, "Linux device bugs") are not reproduced.

#include "device/CIrrDeviceSDL.h"

#include "device/CJoystickSDLDriver.h"
#include "device/CSDLOperator.h"
#include "device/CSDLTimer.h"
#include "device/KeyMap.h"
#include "device/NullDrivers.h"
#include "device/Options.h"
#include "platform/Paths.h"
#include "platform/Seams.h"
#include "ox/io/IFileSystem.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/scene/ICameraSceneNode.h"
#include "ox/scene/ISceneManager.h"
#include "ox/video/IVideoDriver.h"

namespace port {

CIrrDeviceSDL* CIrrDeviceSDL::Instance = 0;

namespace {

//! The resolutions the settings screen offers, as on Linux.
const int VIDEO_MODES[][2] =
{
    { 2048, 1536 }, { 1900, 1200 }, { 1920, 1080 }, { 1600, 1200 }, { 1680, 1050 }, { 1440, 1050 },
    { 1440, 960 }, { 1440, 900 }, { 1280, 800 }, { 1280, 768 }, { 1280, 720 }, { 1152, 768 },
    { 1024, 768 }, { 800, 600 }
};

const int MIN_WIDTH = 800;
const int MIN_HEIGHT = 600;

//! The primary display's desktop mode; 0 (logged) when SDL cannot read it.
const SDL_DisplayMode* getDesktopMode()
{
    const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
    if (!mode)
        SDL_Log("cannot read the desktop mode: %s", SDL_GetError());
    return mode;
}

//! Drawable pixels per screen unit on the display of a desktop mode: --scale, else the display's
//! scale (the mode's pixel density times the display's content scale: 2 on Retina displays, 1.5 on
//! Windows at 150 %), which is what SDL_GetWindowDisplayScale reports for a window on it.
float getPreferredScaleOn(const SDL_DisplayMode& mode)
{
    return g_options.Scale > 0 ? g_options.Scale : mode.pixel_density * SDL_GetDisplayContentScale(mode.displayID);
}

//! The primary display's desktop size in screen units: its pixels divided by the preferred scale.
ox::core::CDimension2d<int> getDesktopSize()
{
    const SDL_DisplayMode* mode = getDesktopMode();
    if (!mode)
        return ox::core::CDimension2d<int>(1024, 768);
    float scale = mode->pixel_density / getPreferredScaleOn(*mode);
    return ox::core::CDimension2d<int>((int)(mode->w * scale + 0.5f), (int)(mode->h * scale + 0.5f));
}

//! The window coordinate to warp to for screen position p, with screen units over window
//! coordinates: where p starts, rounded up to a whole coordinate when coordinates are at most as
//! large as units (some platforms warp to whole coordinates only), else the middle of p.
float toWindowCoordinate(int p, int screen, int window)
{
    if (window >= screen)
        return (float)SDL_ceil((double)p * window / screen);
    return (float)((p + 0.5) * window / screen);
}

const char* profileName(int profile)
{
    return profile == SDL_GL_CONTEXT_PROFILE_ES ? "OpenGL ES" : "OpenGL core";
}

} // end namespace

//! Declared in ox/IOxDevice.h; C linkage makes this the same function as ox::createDevice.
extern "C" ox::IOxDevice* createDevice(ox::video::E_DRIVER_TYPE driverType, ox::event::IEventReceiver* receiver,
    const wchar_t* version)
{
    return new CIrrDeviceSDL(driverType, receiver, version);
}

CIrrDeviceSDL::CIrrDeviceSDL(ox::video::E_DRIVER_TYPE driverType, ox::event::IEventReceiver* receiver,
    const wchar_t* version)
    : CIrrDeviceStub(version, receiver), DriverType(driverType), Window(0), Context(0), SDLOperator(0),
      SDLJoystickDriver(0), CursorPos(0, 0), RelativeCursorPos(0, 0), ScreenSize(0, 0), PixelSize(0, 0),
      MinimumSizeScale(0), WindowedSize(0, 0), WheelX(0), WheelY(0), CursorVisible(true), SelectedLanguageIndex(0),
      WindowActive(false), Fullscreen(false), Closed(false)
{
    Instance = this;

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        SDL_Log("cannot initialise SDL: %s", SDL_GetError());

    Timer->drop();
    Timer = new CSDLTimer();

    // The stub drops CursorControl too, so the device holds a second reference to that base.
    static_cast<ox::gui::ICursorControl*>(this)->grab();
    CursorControl = this;
    MouseButtonStates[0] = false;
    MouseButtonStates[1] = false;
    MouseButtonStates[2] = false;

    SDLOperator = new CSDLOperator();
    Operator = SDLOperator;

    // Joysticks are tracked from the start, so connections during loading are not missed.
    SDLJoystickDriver = new CJoystickSDLDriver();
    JoystickDriver = SDLJoystickDriver;

    // main.cpp found the data before the game started (the original used /proc/self/exe's directory)
    FileSystem->addDirectoryAlias("$GAME_RESOURCES$", getGameDataDirectory().c_str());

    // The modes strictly smaller than the desktop (in screen units) in both dimensions, all at 32 bits.
    ox::core::CDimension2d<int> desktop = getDesktopSize();
    if (const SDL_DisplayMode* mode = getDesktopMode())
        SDL_Log("%s desktop %dx%d, pixel density %g, content scale %g: %dx%d screen units",
            SDL_GetCurrentVideoDriver(), mode->w, mode->h, mode->pixel_density,
            SDL_GetDisplayContentScale(mode->displayID), desktop.Width, desktop.Height);
    VideoModeList.setDesktop(32, desktop);
    for (unsigned int m = 0; m < SDL_arraysize(VIDEO_MODES); ++m)
        if (VIDEO_MODES[m][0] < desktop.Width && VIDEO_MODES[m][1] < desktop.Height)
            VideoModeList.addMode(ox::core::CDimension2d<int>(VIDEO_MODES[m][0], VIDEO_MODES[m][1]), 32);
}

//! The GUI, scene manager and video driver go before the context they draw with; the stub's
//! destructor drops the rest.
CIrrDeviceSDL::~CIrrDeviceSDL()
{
    if (GUIEnvironment)
    {
        GUIEnvironment->drop();
        GUIEnvironment = 0;
    }
    if (SceneManager)
    {
        SceneManager->drop();
        SceneManager = 0;
    }
    if (VideoDriver)
    {
        VideoDriver->drop();
        VideoDriver = 0;
    }
    if (Context)
        SDL_GL_DestroyContext(Context);
    if (Window)
        SDL_DestroyWindow(Window);
    if (Instance == this)
        Instance = 0;
}

//! There is no options dialog, as on Linux: the window is 3/4 of the largest 4:3 box that fits the
//! desktop, windowed, and the language is English (its index, or the array size when it is missing).
bool CIrrDeviceSDL::createUserSelectedDeviceWindow(const ox::TArray<ox::core::CString<wchar_t> >* languages,
    unsigned int flags)
{
    ox::core::CDimension2d<int> desktop = getDesktopSize();
    int width = desktop.Width;
    int height = desktop.Height;
    if (width > height)
        width = (int)(height * (4.0 / 3.0));
    else
        height = (int)(width * 0.75);

    ox::core::CString<wchar_t> english(L"English");
    for (SelectedLanguageIndex = 0; SelectedLanguageIndex < (int)languages->size(); ++SelectedLanguageIndex)
        if ((*languages)[SelectedLanguageIndex] == english)
            break;

    return createDeviceWindow(ox::core::CDimension2d<int>((int)(width * 0.75), (int)(height * 0.75)), 32, false,
        false, false, flags);
}

bool CIrrDeviceSDL::createWindowAndContext(const ox::core::CDimension2d<int>& size, bool fullscreen,
    bool stencilBuffer, int profile, int major, int minor)
{
    SDL_GL_ResetAttributes();
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profile);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
    if (profile == SDL_GL_CONTEXT_PROFILE_CORE)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, stencilBuffer ? 8 : 0);

    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (fullscreen)
        flags |= SDL_WINDOW_FULLSCREEN;
    Window = SDL_CreateWindow(Title.c_str(), size.Width, size.Height, flags);
    if (!Window)
    {
        SDL_Log("cannot create a %s %d.%d window: %s", profileName(profile), major, minor, SDL_GetError());
        return false;
    }

    Context = SDL_GL_CreateContext(Window);
    if (!Context)
    {
        SDL_Log("cannot create a %s %d.%d context: %s", profileName(profile), major, minor, SDL_GetError());
        SDL_DestroyWindow(Window);
        Window = 0;
        return false;
    }

    typedef const unsigned char* (SDLCALL * GetStringFunc)(unsigned int);
    GetStringFunc getString = (GetStringFunc)SDL_GL_GetProcAddress("glGetString");
    if (getString)
        SDL_Log("%s context: %s, %s", profileName(profile), getString(0x1F02 /* GL_VERSION */),
            getString(0x1F01 /* GL_RENDERER */));
    return true;
}

//! Creates the window (windowed or fullscreen as asked, so there is no mode switch afterwards) with
//! an OpenGL ES 3.0 context, or OpenGL 3.3 core where ES is unavailable (always on macOS), then the
//! renderer, the GUI and the scene manager. The null driver type creates no window, as on Linux.
//! screenSize is in screen units; the window gets the size that has them at the primary display's
//! scale. bits and antiAlias are ignored; vsync is on unless --no-vsync (the game always passes
//! false).
bool CIrrDeviceSDL::createDeviceWindow(const ox::core::CDimension2d<int>& screenSize, unsigned int bits,
    bool fullscreen, bool stencilBuffer, bool vsync, unsigned int antiAlias)
{
    if (DriverType == ox::video::EDT_NULL || Window)
        return false;

    const SDL_DisplayMode* mode = getDesktopMode();
    float windowScale = mode ? getPreferredScaleOn(*mode) / mode->pixel_density : 1.0f;
    ox::core::CDimension2d<int> windowSize((int)(screenSize.Width * windowScale + 0.5f),
        (int)(screenSize.Height * windowScale + 0.5f));

#if defined(SDL_PLATFORM_MACOS)
    bool created = createWindowAndContext(windowSize, fullscreen, stencilBuffer, SDL_GL_CONTEXT_PROFILE_CORE, 3, 3);
#elif defined(SDL_PLATFORM_EMSCRIPTEN)
    bool created = createWindowAndContext(windowSize, fullscreen, stencilBuffer, SDL_GL_CONTEXT_PROFILE_ES, 3, 0);
#else
    bool created = createWindowAndContext(windowSize, fullscreen, stencilBuffer, SDL_GL_CONTEXT_PROFILE_ES, 3, 0)
        || createWindowAndContext(windowSize, fullscreen, stencilBuffer, SDL_GL_CONTEXT_PROFILE_CORE, 3, 3);
#endif
    if (!created)
        return false;

    if (!SDL_GL_SetSwapInterval(vsync || g_options.VSync ? 1 : 0))
        SDL_Log("cannot set the swap interval: %s", SDL_GetError());

    // No SDL_SetWindowAspectRatio: on macOS (SDL 3.4.16) AppKit traps when such a window leaves
    // fullscreen. onResized clamps the aspect instead, as the Linux device did.
    updateMinimumSize();
    SDL_StartTextInput(Window);
    SDLOperator->setWindow(Window);
    WindowActive = (SDL_GetWindowFlags(Window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    Fullscreen = fullscreen;
    WindowedSize = screenSize;
    PixelSize = getPixelSize();
    ScreenSize = getScreenSizeFor(PixelSize);
    SDL_Log("screen %dx%d in a %dx%d drawable", ScreenSize.Width, ScreenSize.Height, PixelSize.Width,
        PixelSize.Height);

    VideoDriver = g_options.NullVideo ? createNullVideoDriver(this, FileSystem, ScreenSize)
                                      : port::createVideoDriver(this, FileSystem, ScreenSize);
    if (!VideoDriver)
        return false;
    // ETCF_CREATE_MIP_MAPS, off as in the Linux build.
    VideoDriver->setTextureCreationFlag((ox::video::E_TEXTURE_CREATION_FLAG)0x10, false);
    VideoDriver->setFullscreen(fullscreen);

    createGUIAndScene();
    return true;
}

ox::core::CDimension2d<int> CIrrDeviceSDL::getPixelSize()
{
    int width = 0;
    int height = 0;
    SDL_GetWindowSizeInPixels(Window, &width, &height);
    return ox::core::CDimension2d<int>(width, height);
}

float CIrrDeviceSDL::getPreferredScale()
{
    return g_options.Scale > 0 ? g_options.Scale : SDL_GetWindowDisplayScale(Window);
}

float CIrrDeviceSDL::getWindowScale()
{
    return getPreferredScale() / SDL_GetWindowPixelDensity(Window);
}

//! The drawable divided by the preferred scale, rounded. Where that would be smaller than 800x600,
//! the smallest screen the game was made for (a small display at a high scale), the scale is lowered
//! to fit 800x600, but not below 1.
ox::core::CDimension2d<int> CIrrDeviceSDL::getScreenSizeFor(const ox::core::CDimension2d<int>& pixels)
{
    float scale = getPreferredScale();
    scale = SDL_min(scale, pixels.Width / (float)MIN_WIDTH);
    scale = SDL_min(scale, pixels.Height / (float)MIN_HEIGHT);
    scale = SDL_max(scale, 1.0f);
    return ox::core::CDimension2d<int>((int)(pixels.Width / scale + 0.5f), (int)(pixels.Height / scale + 0.5f));
}

//! 800x600 screen units at the preferred scale, set again when the window coordinates per screen
//! unit change (a move to a display of another scale).
void CIrrDeviceSDL::updateMinimumSize()
{
    float windowScale = getWindowScale();
    if (windowScale == MinimumSizeScale)
        return;
    MinimumSizeScale = windowScale;
    SDL_SetWindowMinimumSize(Window, (int)SDL_ceilf(MIN_WIDTH * windowScale),
        (int)SDL_ceilf(MIN_HEIGHT * windowScale));
}

//! Rounded down, so positions left of or above the window are negative.
ox::core::CPosition2d<int> CIrrDeviceSDL::toScreen(float x, float y)
{
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(Window, &width, &height);
    return ox::core::CPosition2d<int>((int)SDL_floor((double)x * ScreenSize.Width / width),
        (int)SDL_floor((double)y * ScreenSize.Height / height));
}

bool CIrrDeviceSDL::run()
{
    return !Closed;
}

bool CIrrDeviceSDL::swapBuffers()
{
    return SDL_GL_SwapWindow(Window);
}

//! Borderless fullscreen on the window's display (no mode switch). The new size arrives as a pixel
//! size event and goes through onResized; leaving fullscreen restores the windowed size, including
//! one set by resizeDeviceWindow while in fullscreen.
bool CIrrDeviceSDL::setFullscreenMode(bool fullscreen)
{
    if (!Window || Fullscreen == fullscreen)
        return false;

    Fullscreen = fullscreen;
    SDL_SetWindowFullscreen(Window, fullscreen);
    VideoDriver->setFullscreen(fullscreen);
    return true;
}

//! size is in screen units. In fullscreen only the size to restore is recorded; windowed, the window
//! is asked for the size that has it at the preferred scale (the granted size comes back through
//! onResized).
void CIrrDeviceSDL::resizeDeviceWindow(const ox::core::CDimension2d<int>& size)
{
    WindowedSize = size;
    if (Window && !Fullscreen)
    {
        float windowScale = getWindowScale();
        SDL_SetWindowSize(Window, (int)(size.Width * windowScale + 0.5f), (int)(size.Height * windowScale + 0.5f));
    }
}

//! Called for a new drawable size or display scale. When only the drawable changed (a move to a
//! display of another density at the default scale), only the driver is told, to scale its
//! viewport. A new screen size is applied; when windowed, a size outside aspect ratios 4:3 to 16:9
//! or below 800x600 is clamped as on Linux (a narrower window gets its height cut to width * 3/4, a
//! wider one its width cut to height * 16/9) and the window is asked for the clamped size (its
//! minimum size already keeps it from going below 800x600). The size the window has is applied
//! either way, so the frame always matches the drawable; when the window grants the clamped size,
//! that arrives as another resize.
void CIrrDeviceSDL::onResized()
{
    updateMinimumSize();

    ox::core::CDimension2d<int> pixels = getPixelSize();
    ox::core::CDimension2d<int> size = getScreenSizeFor(pixels);
    if (pixels == PixelSize && size == ScreenSize)
        return;

    PixelSize = pixels;
    if (size == ScreenSize)
    {
        SDL_Log("screen %dx%d in a %dx%d drawable", ScreenSize.Width, ScreenSize.Height, PixelSize.Width,
            PixelSize.Height);
        VideoDriver->OnResize(ScreenSize);
        return;
    }

    ScreenSize = size;

    if (!Fullscreen)
    {
        ox::core::CDimension2d<int> clamped = size;
        float aspect = (float)size.Width / (float)size.Height;
        if (aspect < 4.0 / 3.0)
            clamped.Height = (int)(size.Width * 0.75);
        else if (aspect > 16.0 / 9.0)
            clamped.Width = (int)(size.Height * (16.0 / 9.0));
        clamped.Width = clamped.Width < MIN_WIDTH ? MIN_WIDTH : clamped.Width;
        clamped.Height = clamped.Height < MIN_HEIGHT ? MIN_HEIGHT : clamped.Height;
        if (clamped != size)
            resizeDeviceWindow(clamped);
    }

    updateScreenSize();
}

void CIrrDeviceSDL::updateScreenSize()
{
    // The camera's aspect ratio is height / width, as the camera expects (docs/port/menu-scene.md).
    if (SceneManager && SceneManager->getActiveCamera())
        SceneManager->getActiveCamera()->setAspectRatio((float)ScreenSize.Height / (float)ScreenSize.Width);

    RelativeCursorPos.X = CursorPos.X / (float)ScreenSize.Width;
    RelativeCursorPos.Y = CursorPos.Y / (float)ScreenSize.Height;

    VideoDriver->OnResize(ScreenSize);

    ox::event::SEvent ev;
    ev.EventType = ox::event::EET_DEVICE_EVENT;
    ev.DeviceEvent.Type = ox::event::EDE_FULLSCREEN_TOGGLED;
    ev.DeviceEvent.Width = ScreenSize.Width;
    ev.DeviceEvent.Height = ScreenSize.Height;
    postEventFromUser(ev);

    if (!Fullscreen)
        WindowedSize = ScreenSize;
}

void CIrrDeviceSDL::handleEvent(const SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        closeDevice();
        return;
    default:
        break;
    }

    if (!Window)
        return;

    switch (event.type)
    {
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
        onResized();
        break;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        WindowActive = true;
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        WindowActive = false;
        break;
    case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
        // Also reached when the user switches through the window manager.
        if (!Fullscreen)
        {
            Fullscreen = true;
            VideoDriver->setFullscreen(true);
        }
        break;
    case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
        if (Fullscreen)
        {
            Fullscreen = false;
            VideoDriver->setFullscreen(false);
        }
        if (WindowedSize.Width > 0)
            resizeDeviceWindow(WindowedSize);
        break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        postKeyEvent(event.key);
        break;
    case SDL_EVENT_TEXT_INPUT:
        postTextEvent(event.text);
        break;
    case SDL_EVENT_MOUSE_MOTION:
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_MOUSE_WHEEL:
        postMouseEvent(event);
        break;
    default:
    {
        ox::event::SEvent ev;
        if (SDLJoystickDriver->translateEvent(event, ev))
            postEventFromUser(ev);
        break;
    }
    }
}

//! KeyInput.Key is the ox code (0 for keys without one) and Char is 0. With Control held (or Command
//! on macOS), a press of V becomes EKIE_PASTE, F becomes EKIE_TOGGLE_FULLSCREEN (not repeated) and
//! Q closes the device without an event; any other key is a normal press (Linux left the event type
//! unset). Held keys repeat as presses, as SFML's key repeat did.
void CIrrDeviceSDL::postKeyEvent(const SDL_KeyboardEvent& event)
{
    bool control = (event.mod & SDL_KMOD_CTRL) != 0;
#if defined(SDL_PLATFORM_MACOS)
    control = control || (event.mod & SDL_KMOD_GUI) != 0;
#endif

    ox::event::SEvent ev;
    ev.EventType = ox::event::EET_KEY_INPUT_EVENT;
    ev.KeyInput.Char = 0;
    ev.KeyInput.Key = toOxKey(event.key);
    ev.KeyInput.Event = event.down ? ox::event::EKIE_KEY_PRESSED_DOWN : ox::event::EKIE_KEY_LEFT_UP;
    ev.KeyInput.Shift = (event.mod & SDL_KMOD_SHIFT) != 0;
    ev.KeyInput.Control = control;

    if (control && event.down)
    {
        if (ev.KeyInput.Key == ox::KEY_KEY_V)
            ev.KeyInput.Event = ox::event::EKIE_PASTE;
        else if (ev.KeyInput.Key == ox::KEY_KEY_F)
        {
            if (event.repeat)
                return;
            ev.KeyInput.Event = ox::event::EKIE_TOGGLE_FULLSCREEN;
        }
        else if (ev.KeyInput.Key == ox::KEY_KEY_Q)
        {
            closeDevice();
            return;
        }
    }

    postEventFromUser(ev);
}

//! Each code point of the UTF-8 text becomes an EKIE_CHARACTER event with the character in Char.
//! Control characters (below 32) and DEL are dropped, and so are characters outside the BMP where
//! wchar_t is 16 bits (Windows).
void CIrrDeviceSDL::postTextEvent(const SDL_TextInputEvent& event)
{
    const char* text = event.text;
    while (*text)
    {
        Uint32 character = SDL_StepUTF8(&text, 0);
        if (character < 32 || character == 127 || (sizeof(wchar_t) == 2 && character > 0xFFFF))
            continue;

        ox::event::SEvent ev;
        ev.EventType = ox::event::EET_KEY_INPUT_EVENT;
        ev.KeyInput.Char = (wchar_t)character;
        ev.KeyInput.Key = (ox::EKEY_CODE)0;
        ev.KeyInput.Event = ox::event::EKIE_CHARACTER;
        ev.KeyInput.Shift = false;
        ev.KeyInput.Control = false;
        postEventFromUser(ev);
    }
}

//! Positions are screen units from the top left. The left, right and middle buttons send the
//! pressed events 0-2 with the click count of getClickCount (250 ms, 10 units) and the released
//! events 3-5 with the current count; other buttons are ignored. The wheel sends EMIE_MOUSE_WHEEL
//! at the pointer with ScrollY 10 per notch up and ScrollX 10 per notch left (the game moves the
//! view by minus these), from SDL's precise amounts: trackpads and high-resolution wheels send
//! fractions of notches, and the user's scrolling direction (natural scrolling) is applied as in
//! other applications. The amounts sent are whole, the fractions carried to the next wheel event
//! (WheelX, WheelY), since the game truncates them. Every event updates the cursor position;
//! motion, presses and the wheel are posted only inside the window, releases always (SDL captures
//! the mouse while a button is held, so a drag that ends outside still ends).
void CIrrDeviceSDL::postMouseEvent(const SDL_Event& event)
{
    ox::event::SEvent ev;
    ev.EventType = ox::event::EET_MOUSE_INPUT_EVENT;
    ev.MouseInput.Clicks = 0;
    ev.MouseInput.ScrollX = 0;
    ev.MouseInput.ScrollY = 0;

    ox::core::CPosition2d<int> pos;
    bool release = false;
    switch (event.type)
    {
    case SDL_EVENT_MOUSE_MOTION:
        pos = toScreen(event.motion.x, event.motion.y);
        ev.MouseInput.Event = ox::event::EMIE_MOUSE_MOVED;
        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        int button;
        switch (event.button.button)
        {
        case SDL_BUTTON_LEFT: button = 0; break;
        case SDL_BUTTON_RIGHT: button = 1; break;
        case SDL_BUTTON_MIDDLE: button = 2; break;
        default: return;
        }
        pos = toScreen(event.button.x, event.button.y);
        release = !event.button.down;
        MouseButtonStates[button] = event.button.down;
        if (event.button.down)
        {
            ev.MouseInput.Event = (ox::event::EMOUSE_INPUT_EVENT)(ox::event::EMIE_LMOUSE_PRESSED_DOWN + button);
            ev.MouseInput.Clicks = getClickCount(button, pos);
        }
        else
        {
            ev.MouseInput.Event = (ox::event::EMOUSE_INPUT_EVENT)(ox::event::EMIE_LMOUSE_LEFT_UP + button);
            ev.MouseInput.Clicks = ClickCount;
        }
        break;
    }

    case SDL_EVENT_MOUSE_WHEEL:
    {
        // SDL's x is positive to the right, y away from the user (up)
        WheelX -= event.wheel.x * 10.0f;
        WheelY += event.wheel.y * 10.0f;
        float scrollX = SDL_truncf(WheelX);
        float scrollY = SDL_truncf(WheelY);
        if (scrollX == 0.0f && scrollY == 0.0f)
            return;
        WheelX -= scrollX;
        WheelY -= scrollY;
        SDL_LogDebug(SDL_LOG_CATEGORY_INPUT, "wheel %g, %g (%s): ScrollX %g, ScrollY %g", event.wheel.x,
            event.wheel.y, event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? "flipped" : "normal", scrollX, scrollY);
        pos = toScreen(event.wheel.mouse_x, event.wheel.mouse_y);
        ev.MouseInput.Event = ox::event::EMIE_MOUSE_WHEEL;
        ev.MouseInput.ScrollY = scrollY;
        ev.MouseInput.ScrollX = scrollX;
        break;
    }

    default:
        return;
    }

    ev.MouseInput.X = pos.X;
    ev.MouseInput.Y = pos.Y;
    CursorPos = pos;
    RelativeCursorPos.X = pos.X / (float)ScreenSize.Width;
    RelativeCursorPos.Y = pos.Y / (float)ScreenSize.Height;

    if (release || (pos.X >= 0 && pos.Y >= 0 && pos.X < ScreenSize.Width && pos.Y < ScreenSize.Height))
        postEventFromUser(ev);
}

//! The caption as UTF-8 (Linux narrowed each character).
void CIrrDeviceSDL::setWindowCaption(const wchar_t* text)
{
    char* utf8 = wideToUtf8(text);
    Title = utf8;
    SDL_free(utf8);
    if (Window)
        SDL_SetWindowTitle(Window, Title.c_str());
}

//! Stops the device: run() returns false from now on, so the game loop ends and the game drops the
//! device, which destroys the window.
void CIrrDeviceSDL::closeDevice()
{
    Closed = true;
}

void CIrrDeviceSDL::setVisible(bool visible)
{
    CursorVisible = visible;
    if (visible)
        SDL_ShowCursor();
    else
        SDL_HideCursor();
}

//! The pointer is left alone when it is already at that position (the game warps it back to its
//! position after every wheel event); otherwise it goes to a window coordinate that maps back to
//! the position exactly.
void CIrrDeviceSDL::setPosition(int x, int y)
{
    float pointerX = 0;
    float pointerY = 0;
    SDL_GetMouseState(&pointerX, &pointerY);
    if (toScreen(pointerX, pointerY) == ox::core::CPosition2d<int>(x, y))
        return;

    int width = 0;
    int height = 0;
    SDL_GetWindowSize(Window, &width, &height);
    float windowX = toWindowCoordinate(x, ScreenSize.Width, width);
    float windowY = toWindowCoordinate(y, ScreenSize.Height, height);
    SDL_LogDebug(SDL_LOG_CATEGORY_INPUT, "pointer at %g, %g warped to %d, %d (%g, %g)", pointerX, pointerY, x, y,
        windowX, windowY);
    SDL_WarpMouseInWindow(Window, windowX, windowY);
}

//! The audio backend (or, with --no-audio or when it cannot start, a driver that plays nothing),
//! created on first use.
ox::audio::IAudioDriver* CIrrDeviceSDL::createAudioDriver()
{
    if (!AudioDriver && !g_options.NoAudio)
        AudioDriver = port::createAudioDriver(FileSystem);
    if (!AudioDriver)
        AudioDriver = createNullAudioDriver();
    return AudioDriver;
}

ox::input::IJoystickDriver* CIrrDeviceSDL::createJoystickDriver()
{
    return JoystickDriver;
}

} // end namespace port
