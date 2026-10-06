# Input, window and timing

The Linux device is `daisy::CIrrDeviceLinux` on SFML 2
([`src/daisy/other/`](../../src/daisy/other/)), with `CIrrDeviceStub` as its base, `CLinuxOperator`
for OS services and `os::Timer` for time. It turns SFML events into ox events
([`ox/event/IEventReceiver.h`](../../src/ox/event/IEventReceiver.h)).

## Event routing

Each event goes to the GUI environment first, then to the current receiver (the game state), then to
the scene manager. The first one that returns true consumes it.

## Keys

Key bindings in profiles are stored as ox key codes ([`ox/Keycodes.h`](../../src/ox/Keycodes.h)), so a
port has to produce the same codes:

| SFML key | ox code |
|---|---|
| A–Z | 0x41–0x5A |
| Num0–Num9 | 0x30–0x39 |
| Numpad0–Numpad9 | 0x60–0x69 |
| F1–F15 | 0x70–0x7E |
| Escape, Space, Return, BackSpace, Tab | 0x1B, 0x20, 0x0D, 0x08, 0x09 |
| Comma, Period, Dash | 0xBC, 0xBE, 0xBD |
| Add, Subtract, Multiply, Divide | 0x6B, 0x6D, 0x6A, 0x6F |
| Left, Up, Right, Down | 0x25, 0x26, 0x27, 0x28 |
| PageUp, PageDown, End, Home, Delete | 0x21, 0x22, 0x23, 0x24, 0x2E |
| LShift | 0x14 (`KEY_CAPITAL`) |

Insert, Pause, both Control keys, both Alt keys, both System keys, RShift, Menu, the brackets,
semicolon, quote, slash, backslash, tilde and equals are not mapped; on Linux they report an
uninitialised code (see [original-bugs.md](original-bugs.md)), so they cannot be bound reliably.

- Key presses and releases (`EKIE_KEY_LEFT_UP`) carry the code with `Char` 0 and the Shift and
  Control states. Key repeat is on.
- Ctrl+V sends `EKIE_PASTE` (the edit box then reads the clipboard), Ctrl+F sends
  `EKIE_TOGGLE_FULLSCREEN`, and Ctrl+Q closes the device without an event.
- Typed text arrives as `EKIE_CHARACTER` with the UTF-32 code point in `Char`. Codes below 32 and 127
  are dropped.

## Mouse

- Positions are window pixels. The left, right and middle buttons send the pressed events 0–2 and
  the released events 3–5.
- A press carries a click count: a press of the same button at most 250 ms after the previous one and
  at most 10 px away on each axis continues the series. A release carries the current count.
- The wheel sends `EMIE_MOUSE_WHEEL` with `ScrollY` = delta × 10.
- The cursor position and the relative position (position ÷ window size) update on every event;
  only events inside the window are posted. `setPosition` moves the pointer but does not update the
  stored position.

## Joysticks

- Button presses and releases are joystick events of type 0 and 1 with SFML's joystick and button
  numbers; connecting and disconnecting are types 3 and 4.
- Axes are SFML's raw −100 to 100, not −1 to 1. `getJoystickAxes(joystick, stick)` reads the axes
  `stick / 3`, +1 and +2. Up to 8 joysticks are polled at the start of every frame.

## Window and video modes

- On Linux `createUserSelectedDeviceWindow` shows no dialog: the window is ¾ of the largest 4:3 box
  that fits the desktop, and the language is English.
- `createDeviceWindow` creates the SFML window (empty title, default style, vsync as asked, key
  repeat on), then the OpenGL driver with mip maps off, then applies the fullscreen setting and
  creates the GUI and scene.
- Fullscreen uses the first mode SFML offers; windowed mode uses the remembered window size, or the
  desktop size the first time. A windowed resize is clamped to an aspect between 4:3 and 16:9 and to at
  least 800×600. Every applied size sets the menu camera's aspect, resizes the driver and posts a
  device event with the new width and height.
- The settings screen offers the modes 2048×1536, 1900×1200, 1920×1080, 1600×1200, 1680×1050,
  1440×1050, 1440×960, 1440×900, 1280×800, 1280×768, 1280×720, 1152×768, 1024×768 and 800×600 that are
  strictly smaller than the desktop in both dimensions, at 32 bits, smallest first.

## Timing

- `getTime()` is wall-clock milliseconds from `gettimeofday` (seconds × 1000 + microseconds / 1000),
  truncated to 32 bits: not time since start, not monotonic, and it wraps.
- `getFloatTime()` is the same clock in seconds as a double. The game loop's time step comes from it
  (see `CGameMain`).

## Paths and OS services

- `$HARVEST_USERDATA$` is `$HOME/.Harvest` (`/.Harvest` when `HOME` is unset); `$GAME_RESOURCES$` is the
  directory of the executable (`/proc/self/exe`). See [file-system.md](file-system.md).
- The clipboard is GTK's; `openURL` runs `xdg-open` on the URL.
- Engine log messages go to stdout when their level is at least the log level (default 0).
