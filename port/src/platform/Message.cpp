#include "platform/Message.h"

#include <SDL3/SDL.h>

#if defined(SDL_PLATFORM_EMSCRIPTEN)
#include <emscripten.h>

// The page defines Module.showGameMessage (port/web/index.html); without it, the browser's alert.
EM_JS(void, showPageMessage, (const char* title, const char* message), {
    var t = UTF8ToString(title);
    var m = UTF8ToString(message);
    if (Module.showGameMessage)
        Module.showGameMessage(t, m);
    else
        alert(t + "\n\n" + m);
});
#endif

namespace port {

void showErrorMessage(const char* title, const char* message, SDL_Window* window)
{
    SDL_Log("%s: %s", title, message);
#if defined(SDL_PLATFORM_EMSCRIPTEN)
    (void)window;
    showPageMessage(title, message);
#else
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, message, window);
#endif
}

} // end namespace port
