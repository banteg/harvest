#include "device/CSDLOperator.h"

#include <wchar.h>

namespace port {

char* wideToUtf8(const wchar_t* text)
{
    return SDL_iconv_string("UTF-8", "WCHAR_T", (const char*)text, (wcslen(text) + 1) * sizeof(wchar_t));
}

CSDLOperator::CSDLOperator()
    : Window(0), Platform(SDL_GetPlatform())
{
}

const wchar_t* CSDLOperator::getOperationSystemVersion()
{
    return Platform.c_str();
}

void CSDLOperator::copyToClipboard(const char* text)
{
    SDL_SetClipboardText(text);
}

//! The clipboard as UTF-8; CGUIEditBox widens it with ansiToWide.
char* CSDLOperator::getTextFromClipboard()
{
    char* text = SDL_GetClipboardText();
    ClipboardText = text;
    SDL_free(text);
    return (char*)ClipboardText.c_str();
}

const wchar_t* CSDLOperator::getComputerName()
{
    return L"";
}

int CSDLOperator::getRegistryIntValue(int root, const wchar_t* key, const wchar_t* name)
{
    return 0;
}

void CSDLOperator::setRegistryIntValue(int root, const wchar_t* key, const wchar_t* name, int value)
{
}

//! A modal box with an OK button (the only caller is CGameMain's missing-file error); returns 0.
int CSDLOperator::messageBox(const wchar_t* caption, const wchar_t* text, int flags)
{
    char* title = wideToUtf8(caption);
    char* message = wideToUtf8(text);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, message, Window);
    SDL_free(title);
    SDL_free(message);
    return 0;
}

bool CSDLOperator::openURL(const wchar_t* url)
{
    char* utf8 = wideToUtf8(url);
    bool opened = SDL_OpenURL(utf8);
    SDL_free(utf8);
    return opened;
}

int CSDLOperator::getProcessID()
{
    return 0;
}

const char* CSDLOperator::getProcessExecutableName()
{
    return "";
}

bool CSDLOperator::runApplication(const wchar_t* application, const wchar_t* arguments)
{
    return false;
}

//! On Linux the original's "$HOME/." + application (~/.Harvest), so existing profiles and saves are
//! found. Elsewhere, and on Linux without HOME, SDL's per-user data directory for the application
//! (~/Library/Application Support/Harvest on macOS, as the Mac build used, %APPDATA%\Harvest on
//! Windows), without the trailing separator.
ox::core::CString<char> CSDLOperator::getApplicationSupportPath(const char* application)
{
#if defined(SDL_PLATFORM_LINUX)
    const char* home = SDL_getenv("HOME");
    if (home && *home)
        return ox::core::CString<char>(home) + "/." + application;
#endif

    char* pref = SDL_GetPrefPath("", application);
    if (!pref)
    {
        SDL_Log("cannot find a user data directory: %s", SDL_GetError());
        return ox::core::CString<char>(application);
    }
    ox::core::CString<char> path(pref);
    SDL_free(pref);
    return path.subString(0, path.size() - 1);
}

} // end namespace port
