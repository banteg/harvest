// Error messages for the player: a message box on the desktop; on the web, the page's own message
// (a modal alert would block the browser's main thread).

#ifndef HARVEST_PORT_PLATFORM_MESSAGE_H
#define HARVEST_PORT_PLATFORM_MESSAGE_H

struct SDL_Window;

namespace port {

//! Shows an error with a title and UTF-8 text, over `window` where there is one.
void showErrorMessage(const char* title, const char* message, SDL_Window* window);

} // end namespace port

#endif
