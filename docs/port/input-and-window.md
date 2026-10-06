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

## The port's device

The port replaces `CIrrDeviceLinux`, `CLinuxOperator` and `CJoystickLinuxDriver` with an SDL3 device
in [`port/src/device/`](../../port/src/device/) (`port::CIrrDeviceSDL`, again a `CIrrDeviceStub`). It
keeps the behaviour above except where noted, and does not reproduce the Linux device bugs in
[original-bugs.md](original-bugs.md).

### Frames and events

- `port/src/main.cpp` runs the game from SDL3's main callbacks. SDL dispatches the queued events to
  `SDL_AppEvent` on the main thread right before each `SDL_AppIterate`, which runs one
  `CGameMain::update`. The device turns them into ox events there; `run()` only reports whether the
  device is still open. The order is the original's: events first, then the frame.
- **Loading is spread over frames.** The original's `CGameMain::setState` runs every
  `renderFirst`/`secondInit` step of a new state inside one call. Under `HARVEST_PORT` it only starts
  the load, and each `update` runs one step until the state is ready, so a frame never blocks for a
  whole load (the web build cannot block; the window keeps answering). Input that arrives while a
  state loads is held and replayed, in order, when it is ready, since the original handled no input
  during a load. A failed load logs the state's error message (the original dropped it).
- The 20 ms sleep while the window is inactive stays on the desktop and is compiled out for the web.
- Quit requests (closing the window, Cmd+Q, SIGTERM) and Ctrl+Q stop the device; the game loop ends
  on the next frame and the window is destroyed with the device.

### Window

- The window is created at the requested size, windowed or fullscreen directly, with an
  **OpenGL ES 3.0** context; where that fails it is recreated with **OpenGL 3.3 core**. macOS (no ES)
  always uses 3.3 core, forward-compatible; the web uses ES 3.0 (WebGL 2). Linux (EGL/Mesa) gets ES;
  Windows gets ES where the driver offers `WGL_EXT_create_context_es2_profile`, else core. The
  renderer reads the profile from `SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK)`. Depth is 24 bits;
  stencil 8 bits only when asked (the game never asks). The context is current when
  `port::createVideoDriver` runs; `swapBuffers` is `SDL_GL_SwapWindow`.
- **Vsync is on** (`--no-vsync` turns it off). The game always passed `vsync = false`, so the original
  ran uncapped; the frame step comes from the clock, so this changes nothing but the frame rate.
- **Fullscreen is borderless** on the window's display, at the desktop size (no mode switch; Linux
  switched to SFML's first fullscreen mode, normally the desktop's). It is asynchronous: the new size
  arrives as a resize. Leaving fullscreen restores the windowed size, including one set with
  `resizeDeviceWindow` while in fullscreen. A switch made through the window manager (the macOS
  green button) updates the device and driver too.
- **Resizes**: the window has a minimum size of 800×600 screen units; a windowed size outside 4:3 to
  16:9 is clamped as on Linux and the window is asked for the clamped size. The size the window
  actually has is always applied (camera aspect, driver `OnResize`, `EDE_FULLSCREEN_TOGGLED` with the
  size). SDL's window aspect constraint is not used: on macOS AppKit traps when such a window leaves
  fullscreen.
- `createUserSelectedDeviceWindow` keeps the Linux rule (¾ of the largest 4:3 box in the desktop,
  windowed, English). The video mode list keeps the Linux list and rule, and also records the
  desktop mode; the desktop is measured in screen units, so a Retina MacBook whose desktop "looks
  like" 1512×982 offers the modes up to 1440×960.

### Screen size and scale

The original ran at the window's pixel size, showed more of the world at higher resolutions and
never scaled its sprites or text, so at a high-density display's full pixel size everything is a
quarter of its intended size. The port separates the two sizes:

- The window asks for high pixel density (`SDL_WINDOW_HIGH_PIXEL_DENSITY`), so its drawable has every
  pixel of the display (2880×1800 for a 1440×900-point window on a Retina display).
- The **screen size** the game gets (`getScreenSize`, the camera aspect, `EDE_FULLSCREEN_TOGGLED`,
  GUI layout, mouse positions, `resizeDeviceWindow` and the settings' resolutions) is the drawable
  divided by the **scale**, rounded. The renderer draws into the whole drawable (see
  [renderer.md](renderer.md#in-the-port)), so the game looks as it does at that resolution, drawn
  with more pixels.
- The scale is the window's display scale (`SDL_GetWindowDisplayScale`: pixel density times content
  scale), so the game works in points on macOS and Wayland (2 on Retina), and in the DPI-scaled size
  on Windows (1.5 at 150 %) and X11 (`Xft.dpi`). `--scale <factor>` (at least 1) fixes it instead:
  `--scale 1` is the old behaviour, the game at the drawable's full resolution, and `--scale 2` gives
  crisp nearest-filtered sprites on a 150 % Windows display. Fractional scales work, but
  nearest-filtered sprites then repeat their texels unevenly (alternately one and two pixels at 1.5).
- Where the screen would come out smaller than 800×600, the smallest size the game was made for (a
  small display at a high scale, such as 1366×768 at 150 %), the scale is lowered to fit 800×600, but
  not below 1.
- A new drawable size or display scale (`SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED`,
  `SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED`: resizing, fullscreen, moving to a display of another
  density) recomputes the screen size. When only the drawable changed (a window moved between a
  Retina and a standard display keeps its points), the driver's `OnResize` alone runs, to rescale the
  viewport; the game sees no resize. The minimum window size follows the scale.
- A window size in screen units (the remembered resolution, `resizeDeviceWindow`) becomes the window
  size that gives that screen at the scale of the window's display (the primary display before the
  window exists). The device logs the screen and drawable sizes when it creates the window and when
  only the drawable changes.
- The caption is UTF-8 (Linux narrowed each character).

### Keys

`SDL_EVENT_KEY_DOWN`/`UP` use SDL's layout-aware key codes (as SFML's and Windows' codes were; SDL's
default `latin_letters` option gives Latin letters on non-Latin layouts). The table is
[`KeyMap.cpp`](../../port/src/device/KeyMap.cpp): the Linux table for the keys SFML knew, and Windows
virtual-key codes for the rest, so every key can be bound and keeps its Windows name in the key list.

| Key | Linux | Port |
|---|---|---|
| Left shift | 0x14 (`KEY_CAPITAL`) | 0xA0 (`KEY_LSHIFT`) |
| Right shift | garbage | 0xA1 (`KEY_RSHIFT`) |
| Caps Lock | — | 0x14 (`KEY_CAPITAL`) |
| Left and right Control | garbage | 0x11 (`KEY_CONTROL`) |
| Left and right Alt | garbage | 0x12 (`VK_MENU`) |
| Left and right GUI (Windows, Command) | garbage | 0x5B, 0x5C |
| Menu | garbage | 0x5D |
| Insert, Pause, Print Screen | garbage, garbage, — | 0x2D, 0x13, 0x2C |
| Num Lock, Scroll Lock | — | 0x90, 0x91 |
| `;` `=` `/` `` ` `` `[` `\` `]` `'` | garbage | 0xBA, 0xBB, 0xBF, 0xC0, 0xDB, 0xDC, 0xDD, 0xDE |
| Keypad Enter, keypad point | — | 0x0D (`KEY_RETURN`), 0x6E |
| F16–F24 | — | 0x7F–0x87 |
| anything else | garbage | 0 |

- `CPlayState` checks `KEY_SHIFT`/`KEY_LSHIFT`/`KEY_RSHIFT` and `KEY_CONTROL` for shift-click and
  control-click selection. On Linux neither ever matched (shift was `KEY_CAPITAL`, control
  uninitialised); in the port both work, as on Windows. A Linux profile with an action bound to left
  shift (stored as 0x14) now needs Caps Lock, or a new binding.
- Ctrl+V sends `EKIE_PASTE`, Ctrl+F `EKIE_TOGGLE_FULLSCREEN` (not on key repeat) and Ctrl+Q closes the
  device; on macOS Command counts as Control for these and for `KeyInput.Control`. Control with any
  other key is a normal press. Held keys repeat as presses.
- `SDL_EVENT_TEXT_INPUT` (text input is started with the window) is decoded from UTF-8 to one
  `EKIE_CHARACTER` event per code point; codes below 32 and 127 are dropped, and so are code points
  above U+FFFF on Windows, where `wchar_t` is 16 bits.

### Mouse and cursor

- Positions are screen units (window coordinates scaled by screen size ÷ window size, rounded down,
  so positions left of or above the window are negative).
- Left, right and middle buttons map to 0, 1 and 2; other buttons are ignored. Click counting is the
  stub's (250 ms, 10 units).
- **The wheel** sends SDL's precise amounts (`x`/`y`, in notches; trackpads and high-resolution
  wheels send fractions), so a trackpad pans the map smoothly while a mouse wheel keeps the
  original's 10 per notch: `ScrollY` = 10 × `y` (positive up) and `ScrollX` = −10 × `x` (positive
  left), the directions in which `CPlayState` moves the view by −`ScrollX`, −`ScrollY`. The game
  truncates each event's amounts (`(int)(ScrollSpeed * ScrollY)`, and the GUI's `(int)ScrollY`), so
  the device sends whole amounts and carries the fractions to the next wheel event; an event that
  adds up to less than 1 is not sent. At a scroll speed other than the default 1.0 the game's own
  truncation still drops up to one pixel per event.
- The amounts keep SDL's direction, which is the platform's with the user's scrolling preference
  applied (`direction` only says whether it was flipped): with natural scrolling on macOS the map
  follows the fingers, as the content of other applications does, and a mouse wheel turned up
  scrolls the map up when natural scrolling is off. (Earlier the port undid the flip, so vertical
  scrolling ran against the system setting, and `ScrollX` had the opposite sign.) Linux sent no
  `ScrollX`; list boxes and scroll bars read only `ScrollY` (10 pixels per unit, 100 per notch).
- With `SDL_LOGGING=input=debug` the device logs every wheel event's amounts and every pointer warp.
- Motion, presses and the wheel are posted only inside the window, as on Linux; releases always are
  (SDL captures the mouse while a button is held, so a drag that ends outside the window ends).
- `ICursorControl::setPosition` warps the pointer (`SDL_WarpMouseInWindow`) without updating the stored
  position; `setVisible` shows or hides the system cursor. `CPlayState` warps the pointer back to its
  position after every wheel event, so the device leaves the pointer alone when it already is at
  that screen position (no sub-unit jump, and no macOS warp on every trackpad event). Otherwise it
  warps to a window coordinate that maps back to the position exactly: the first whole coordinate in
  it when window coordinates are at least as fine as screen units (some platforms warp to whole
  coordinates), else its middle.

### Joysticks

[`CJoystickSDLDriver`](../../port/src/device/CJoystickSDLDriver.h) keeps eight slots, filled in
connection order, opened from the start (so connections during loading are seen). Devices with an
SDL gamepad mapping report the numbering an Xbox pad had through SFML on Linux (xpad): buttons A 0,
B 1, X 2, Y 3, LB 4, RB 5, Back 6, Start 7, Guide 8, left stick 9, right stick 10, then SDL's order
(D-pad up, down, left, right 11–14, …); axes left X 0, left Y 1, left trigger 2, right trigger 3,
right X 4, right Y 5, D-pad X 6, D-pad Y 7 (SFML's X, Y, Z, R, U, V, PovX, PovY). Other joysticks
report SDL's raw numbers. Positions are −100 to 100 (triggers −100 released); sticks have a dead
zone of 10, since the shuttle race treats any deflection past 0.5 as a turn. Button events are types
0 and 1, connections 3 and 4; releases no longer also report a connection.

### Timing

- `getTime()` is still the wall clock in milliseconds (`os::Timer`): the GUI reads `os::Timer`
  directly and compares with it, and the game seeds its random generator from it.
- `getFloatTime()`, which only `CGameMain`'s time step reads, is SDL's monotonic clock in seconds
  (`SDL_GetTicksNS`), so wall-clock changes cannot produce a zero or negative step.

### Paths and OS services

- `$GAME_RESOURCES$` (the directory holding `harvestClientData/`) is `--data <dir>`, else the
  `HARVEST_DATA` environment variable, else `SDL_GetBasePath()` (the executable's directory; on macOS
  an app bundle's `Contents/Resources`), without a trailing separator. The device logs it and warns
  when `harvestClientData/` is not there. To run against the original data:
  `zig-out/bin/harvest --data orig/1.18-linux-amd64`.
- `$HARVEST_USERDATA$` is `$HOME/.Harvest` on Linux, as before, so existing profiles and saves are
  found. Elsewhere, and on Linux without `HOME`, it is `SDL_GetPrefPath("", "Harvest")`:
  `~/Library/Application Support/Harvest` on macOS (the Mac build's directory) and `%APPDATA%\Harvest`
  on Windows.
- The clipboard is SDL's (UTF-8; `CGUIEditBox` widens it with `ansiToWide`). `openURL` is
  `SDL_OpenURL`, `messageBox` a modal `SDL_ShowSimpleMessageBox` (the Linux one showed nothing). The
  unused services return empty values.
- `--null-video` uses daisy's null driver (it loads textures and packages but draws nothing; the
  window is cleared to the scene colour), `--no-audio` an audio driver that loads nothing. Both are
  also the fallbacks while the renderer or audio backend is not linked
  ([`SeamFallbacks.cpp`](../../port/src/device/SeamFallbacks.cpp)), and the null audio driver is used
  when the backend cannot start.

### Input scripts

`--input-script <file>` feeds the game synthetic SDL events, for testing without a person at the
keyboard ([`InputScript.h`](../../port/src/device/InputScript.h)). Each line is
`<frame> <command> [arguments]` (`#` starts a comment); the frame counts frames during which no state
loads, and the events are handled before the next frame. Positions are window coordinates (points on
macOS):

| Command | Events |
|---|---|
| `move <x> <y>` | pointer motion (the system pointer stays put) |
| `click <x> <y>` | left button down and up |
| `wheel <x> <y> [flipped]` | a wheel event with SDL's precise amounts, at the last position |
| `key <name>` | key down and up; SDL key names, `ctrl+` for Control (`key ctrl+t` takes a screenshot) |
| `size <w> <h>` | `SDL_SetWindowSize` |
| `display <n>` | moves the window to the middle of the n-th display (`SDL_GetDisplays`) |
| `fullscreen on\|off` | `SDL_SetWindowFullscreen` |
| `quit` | a quit request |

### Network

`daisy::net::CWinsockNetworkDevice` ([`port/src/net/`](../../port/src/net/)) never connects: the only
user is the online highscores, whose server is gone. `joinHost` (called from
`CHTTPConnectionHandler`'s thread) fails at once, and the next `pollDevice`, on the main thread,
sends `ENET_CONNECTION_FAILED`, so the highscore and statistics screens show their "Unable to
connect" error. The port includes its declaration in `CIrrDeviceStub.cpp` instead of the matching
build's sized placeholder.
