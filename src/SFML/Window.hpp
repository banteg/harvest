// Stand-in for the SFML 2.0 headers the Linux 1.18 build was compiled against; not SFML's source.
// The build links libsfml-window.so.2.0 and libsfml-system.so.2.0 from a pre-release 2012 snapshot
// whose window titles are std::string rather than sf::String. Only what Harvest uses is declared,
// with the signatures of that snapshot's exports and the layouts the game's code reads.

#ifndef SFML_WINDOW_HPP
#define SFML_WINDOW_HPP

#include <string>
#include <vector>

namespace sf {

typedef unsigned int Uint32;
//! The X11 window id.
typedef unsigned long WindowHandle;

template <typename T>
class Vector2
{
public:
    Vector2() : x(0), y(0) {}
    Vector2(T X, T Y) : x(X), y(Y) {}

    T x;
    T y;
};

typedef Vector2<int> Vector2i;
typedef Vector2<unsigned int> Vector2u;

class Keyboard
{
public:
    enum Key
    {
        Unknown = -1,
        A = 0, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
        Escape,
        LControl, LShift, LAlt, LSystem,
        RControl, RShift, RAlt, RSystem,
        Menu,
        LBracket, RBracket, SemiColon, Comma, Period, Quote, Slash, BackSlash, Tilde, Equal, Dash,
        Space, Return, BackSpace, Tab,
        PageUp, PageDown, End, Home, Insert, Delete,
        Add, Subtract, Multiply, Divide,
        Left, Right, Up, Down,
        Numpad0, Numpad1, Numpad2, Numpad3, Numpad4, Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15,
        Pause,
        KeyCount
    };
};

class Window;

class Mouse
{
public:
    enum Button
    {
        Left,
        Right,
        Middle,
        XButton1,
        XButton2,
        ButtonCount
    };

    static void setPosition(const Vector2i& position, const Window& relativeTo);
};

class Joystick
{
public:
    enum
    {
        Count = 8,
        ButtonCount = 32,
        AxisCount = 8
    };

    enum Axis
    {
        X, Y, Z, R, U, V, PovX, PovY
    };

    static bool isConnected(unsigned int joystick);
    static bool isButtonPressed(unsigned int joystick, unsigned int button);
    //! The axis position, from -100 to 100.
    static float getAxisPosition(unsigned int joystick, Axis axis);
    static void update();
};

class Event
{
public:
    struct SizeEvent
    {
        unsigned int width;
        unsigned int height;
    };

    struct KeyEvent
    {
        Keyboard::Key code;
        bool alt;
        bool control;
        bool shift;
        bool system;
    };

    struct TextEvent
    {
        Uint32 unicode;
    };

    struct MouseMoveEvent
    {
        int x;
        int y;
    };

    struct MouseButtonEvent
    {
        Mouse::Button button;
        int x;
        int y;
    };

    struct MouseWheelEvent
    {
        int delta;
        int x;
        int y;
    };

    struct JoystickConnectEvent
    {
        unsigned int joystickId;
    };

    struct JoystickMoveEvent
    {
        unsigned int joystickId;
        Joystick::Axis axis;
        float position;
    };

    struct JoystickButtonEvent
    {
        unsigned int joystickId;
        unsigned int button;
    };

    enum EventType
    {
        Closed,
        Resized,
        LostFocus,
        GainedFocus,
        TextEntered,
        KeyPressed,
        KeyReleased,
        MouseWheelMoved,
        MouseButtonPressed,
        MouseButtonReleased,
        MouseMoved,
        MouseEntered,
        MouseLeft,
        JoystickButtonPressed,
        JoystickButtonReleased,
        JoystickMoved,
        JoystickConnected,
        JoystickDisconnected,
        Count
    };

    EventType type;

    union
    {
        SizeEvent size;
        KeyEvent key;
        TextEvent text;
        MouseMoveEvent mouseMove;
        MouseButtonEvent mouseButton;
        MouseWheelEvent mouseWheel;
        JoystickMoveEvent joystickMove;
        JoystickButtonEvent joystickButton;
        JoystickConnectEvent joystickConnect;
    };
};

class VideoMode
{
public:
    VideoMode();
    VideoMode(unsigned int modeWidth, unsigned int modeHeight, unsigned int modeBitsPerPixel = 32);

    static VideoMode getDesktopMode();
    //! The fullscreen modes, best first.
    static const std::vector<VideoMode>& getFullscreenModes();

    unsigned int width;
    unsigned int height;
    unsigned int bitsPerPixel;
};

struct ContextSettings
{
    explicit ContextSettings(unsigned int depth = 0, unsigned int stencil = 0, unsigned int antialiasing = 0,
        unsigned int major = 2, unsigned int minor = 0)
        : depthBits(depth), stencilBits(stencil), antialiasingLevel(antialiasing), majorVersion(major),
          minorVersion(minor)
    {
    }

    unsigned int depthBits;
    unsigned int stencilBits;
    unsigned int antialiasingLevel;
    unsigned int majorVersion;
    unsigned int minorVersion;
};

namespace Style
{
    enum
    {
        None = 0,
        Titlebar = 1 << 0,
        Resize = 1 << 1,
        Close = 1 << 2,
        Fullscreen = 1 << 3,
        Default = Titlebar | Resize | Close
    };
}

class Window
{
public:
    Window(VideoMode mode, const std::string& title, Uint32 style = Style::Default,
        const ContextSettings& settings = ContextSettings());
    virtual ~Window();

    void create(VideoMode mode, const std::string& title, Uint32 style = Style::Default,
        const ContextSettings& settings = ContextSettings());
    void close();
    bool isOpen() const;
    bool pollEvent(Event& event);
    Vector2u getSize() const;
    void setSize(Vector2u size);
    void setTitle(const std::string& title);
    void setVisible(bool visible);
    void setVerticalSyncEnabled(bool enabled);
    void setMouseCursorVisible(bool visible);
    void setKeyRepeatEnabled(bool enabled);
    void display();
    WindowHandle getSystemHandle() const;

protected:
    virtual void onCreate();
    virtual void onResize();

private:
    // The snapshot's sf::Window is 40 bytes (operator new(0x28)); its private members are opaque here.
    void* m_impl;
    void* m_context;
    long long m_clock;
    long long m_frameTimeLimit;
};

} // end namespace sf

#endif
