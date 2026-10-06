#include "platform/locale.h"

#include <SDL3/SDL.h>
#include <locale.h>

namespace port {

void useUtf8Locale()
{
#ifdef _WIN32
    // Windows 10 version 1803 and later
    const char* utf8 = ".UTF-8";
#else
    // glibc 2.35 and later (and most distributions before), musl, macOS
    const char* utf8 = "C.UTF-8";
#endif
    if (setlocale(LC_CTYPE, utf8))
        return;
    SDL_Log("the C library has no %s locale; non-ASCII text may be lost", utf8);
#ifndef _WIN32
    setlocale(LC_CTYPE, "");
#endif
}

} // end namespace port
