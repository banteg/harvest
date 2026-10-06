// SDL key codes to the ox key codes (Windows virtual-key codes) that profiles store.

#ifndef PORT_DEVICE_KEYMAP_H
#define PORT_DEVICE_KEYMAP_H

#include <SDL3/SDL_keycode.h>
#include "ox/Keycodes.h"

namespace port {

//! The ox code for an SDL key code (layout-aware, as SFML's and Windows' codes were), or 0 for keys
//! without one. Every code is below 256, the size of the game's key state arrays.
ox::EKEY_CODE toOxKey(SDL_Keycode key);

} // end namespace port

#endif
