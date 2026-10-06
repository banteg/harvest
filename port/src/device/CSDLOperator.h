// The OS services of the SDL3 device (CLinuxOperator's replacement).

#ifndef PORT_DEVICE_CSDLOPERATOR_H
#define PORT_DEVICE_CSDLOPERATOR_H

#include <SDL3/SDL.h>
#include "ox/IOSOperator.h"

namespace port {

//! Wide strings to UTF-8 (wchar_t is UTF-32, or UTF-16 on Windows). The result is SDL_malloc'ed.
char* wideToUtf8(const wchar_t* text);

//! The clipboard, URLs, message boxes and the user data directory through SDL. The services the
//! game never calls return empty values instead of the Linux stubs' garbage.
class CSDLOperator : public ox::IOSOperator
{
public:
    CSDLOperator();

    //! The parent of message boxes; 0 until the device has a window.
    void setWindow(SDL_Window* window) { Window = window; }

    virtual const wchar_t* getOperationSystemVersion();
    virtual void copyToClipboard(const char* text);
    virtual char* getTextFromClipboard();
    virtual const wchar_t* getComputerName();
    virtual int getRegistryIntValue(int root, const wchar_t* key, const wchar_t* name);
    virtual void setRegistryIntValue(int root, const wchar_t* key, const wchar_t* name, int value);
    virtual int messageBox(const wchar_t* caption, const wchar_t* text, int flags);
    virtual bool openURL(const wchar_t* url);
    virtual int getProcessID();
    virtual const char* getProcessExecutableName();
    virtual bool runApplication(const wchar_t* application, const wchar_t* arguments);
    virtual ox::core::CString<char> getApplicationSupportPath(const char* application);

private:
    SDL_Window* Window;
    //! Owns the text getTextFromClipboard returns, until the next call.
    ox::core::CString<char> ClipboardText;
    ox::core::CString<wchar_t> Platform;
};

} // end namespace port

#endif
