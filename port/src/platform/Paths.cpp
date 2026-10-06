#include "platform/Paths.h"

#include <SDL3/SDL.h>
#include <vector>
#include "device/Options.h"

#ifdef SDL_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace port {

namespace {

//! The name the game passes to getApplicationSupportPath, and so its user data directory's.
const char* const APPLICATION = "Harvest";
const char* const CLIENT_DATA = "harvestClientData";
//! Steam's install folder for Harvest: Massive Encounter (app 15400's `installdir`), the same on
//! every platform.
const char* const STEAM_FOLDER = "Harvest Massive Encounter";
//! In the user data directory: the last folder given with --data or HARVEST_DATA.
const char* const REMEMBERED_FILE = "data-directory.txt";

#ifdef SDL_PLATFORM_WINDOWS
const char SEPARATOR = '\\';
const char* const SEPARATORS = "\\/";
#else
const char SEPARATOR = '/';
const char* const SEPARATORS = "/";
#endif

std::string g_gameData;

bool endsWithSeparator(const std::string& path)
{
    return !path.empty() && SDL_strchr(SEPARATORS, path[path.size() - 1]);
}

std::string withoutTrailingSeparators(std::string path)
{
    while (path.size() > 1 && endsWithSeparator(path))
        path.erase(path.size() - 1);
    return path;
}

//! base/relative, with relative's '/' turned into the platform's separator; empty when base is.
std::string join(const std::string& base, const char* relative)
{
    if (base.empty())
        return base;
    std::string path = base;
    if (!endsWithSeparator(path))
        path += SEPARATOR;
    for (const char* c = relative; *c; ++c)
        path += *c == '/' ? SEPARATOR : *c;
    return path;
}

std::string parentOf(const std::string& path)
{
    std::string trimmed = withoutTrailingSeparators(path);
    std::string::size_type slash = trimmed.find_last_of(SEPARATORS);
    if (slash == std::string::npos)
        return std::string();
    return trimmed.substr(0, slash == 0 ? 1 : slash);
}

bool endsWith(const std::string& text, const char* suffix)
{
    size_t length = SDL_strlen(suffix);
    return text.size() >= length && text.compare(text.size() - length, length, suffix) == 0;
}

std::string getEnvironment(const char* name)
{
    const char* value = SDL_getenv(name);
    return value ? value : "";
}

std::string getHome()
{
    const char* home = SDL_GetUserFolder(SDL_FOLDER_HOME);
    return home ? withoutTrailingSeparators(home) : "";
}

bool isDirectory(const std::string& path)
{
    SDL_PathInfo info;
    return !path.empty() && SDL_GetPathInfo(path.c_str(), &info) && info.type == SDL_PATHTYPE_DIRECTORY;
}

bool isAbsolute(const std::string& path)
{
#ifdef SDL_PLATFORM_WINDOWS
    return (path.size() > 1 && path[1] == ':') || (!path.empty() && SDL_strchr(SEPARATORS, path[0]));
#else
    return !path.empty() && path[0] == '/';
#endif
}

std::string makeAbsolute(const std::string& path)
{
    if (isAbsolute(path))
        return path;
    char* current = SDL_GetCurrentDirectory();
    if (!current)
        return path;
    std::string absolute = join(current, path.c_str());
    SDL_free(current);
    return absolute;
}

//! The folder holding harvestClientData/ that a candidate stands for: the candidate itself, the
//! Contents/Resources of the app bundle it is, or of an app bundle in it (the Mac release keeps its
//! data in Harvest Steam.app/Contents/Resources). Empty when there is none.
std::string resolve(const std::string& candidate)
{
    if (isDirectory(join(candidate, CLIENT_DATA)))
        return candidate;
    std::string resources = join(candidate, "Contents/Resources");
    if (isDirectory(join(resources, CLIENT_DATA)))
        return resources;

    std::string found;
    int count = 0;
    char** bundles = SDL_GlobDirectory(candidate.c_str(), "*.app", 0, &count);
    for (int i = 0; bundles && i < count && found.empty(); ++i)
    {
        resources = join(join(candidate, bundles[i]), "Contents/Resources");
        if (isDirectory(join(resources, CLIENT_DATA)))
            found = resources;
    }
    SDL_free(bundles);
    return found;
}

//! The candidates looked at so far, and the folder found.
class CSearch
{
public:
    //! Looks at a candidate (once; empty ones are skipped). Returns true when it holds the data.
    bool look(const std::string& candidate, const char* source)
    {
        if (candidate.empty())
            return false;
        std::string path = withoutTrailingSeparators(candidate);
        for (unsigned int i = 0; i < Searched.size(); ++i)
            if (Searched[i] == path)
                return false;
        Searched.push_back(path);
        Found = resolve(path);
        if (Found.empty())
            return false;
        SDL_Log("game data: %s (%s)", Found.c_str(), source);
        return true;
    }

    std::vector<std::string> Searched;
    std::string Found;
};

#ifdef SDL_PLATFORM_WINDOWS
std::string readRegistryString(HKEY root, const wchar_t* key, const wchar_t* name)
{
    wchar_t value[MAX_PATH];
    DWORD size = sizeof(value);
    if (RegGetValueW(root, key, name, RRF_RT_REG_SZ, 0, value, &size) != ERROR_SUCCESS)
        return std::string();
    char utf8[MAX_PATH * 4];
    int length = WideCharToMultiByte(CP_UTF8, 0, value, -1, utf8, sizeof(utf8), 0, 0);
    return length > 0 ? std::string(utf8) : std::string();
}
#endif

//! Where Steam may be installed, for the platform (Steam's own default folders, and on Windows the
//! folder Steam records in the registry).
std::vector<std::string> getSteamRoots()
{
    std::vector<std::string> roots;
#if defined(SDL_PLATFORM_WINDOWS)
    roots.push_back(readRegistryString(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath"));
    roots.push_back(readRegistryString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Valve\\Steam", L"InstallPath"));
    roots.push_back(join(getEnvironment("ProgramFiles(x86)"), "Steam"));
    roots.push_back(join(getEnvironment("ProgramFiles"), "Steam"));
#elif defined(SDL_PLATFORM_MACOS)
    roots.push_back(join(getHome(), "Library/Application Support/Steam"));
#else
    std::string dataHome = getEnvironment("XDG_DATA_HOME");
    roots.push_back(join(dataHome.empty() ? join(getHome(), ".local/share") : dataHome, "Steam"));
    roots.push_back(join(getHome(), ".steam/steam"));
    // Flatpak and Snap
    roots.push_back(join(getHome(), ".var/app/com.valvesoftware.Steam/.local/share/Steam"));
    roots.push_back(join(getHome(), "snap/steam/common/.local/share/Steam"));
#endif
    return roots;
}

//! The quoted strings of a Valve KeyValues (.vdf) text, with \\ and \" unescaped.
std::vector<std::string> readVdfStrings(const char* text)
{
    std::vector<std::string> strings;
    for (const char* c = text; *c; ++c)
    {
        if (*c == '/' && c[1] == '/')
        {
            while (c[1] && c[1] != '\n')
                ++c;
            continue;
        }
        if (*c != '"')
            continue;
        std::string value;
        for (++c; *c && *c != '"'; ++c)
        {
            if (*c == '\\' && (c[1] == '\\' || c[1] == '"'))
                ++c;
            value += *c;
        }
        strings.push_back(value);
        if (!*c)
            break;
    }
    return strings;
}

//! A Steam installation's libraries: its own folder and the "path"s in steamapps/libraryfolders.vdf.
std::vector<std::string> getSteamLibraries(const std::string& root)
{
    std::vector<std::string> libraries(1, root);
    char* text = (char*)SDL_LoadFile(join(root, "steamapps/libraryfolders.vdf").c_str(), 0);
    if (!text)
        return libraries;
    std::vector<std::string> strings = readVdfStrings(text);
    SDL_free(text);
    for (unsigned int i = 0; i + 1 < strings.size(); ++i)
        if (!SDL_strcasecmp(strings[i].c_str(), "path"))
            libraries.push_back(strings[++i]);
    return libraries;
}

std::string getRememberedPath()
{
    return join(getUserDataPath(APPLICATION), REMEMBERED_FILE);
}

std::string readRemembered()
{
    size_t size = 0;
    char* text = (char*)SDL_LoadFile(getRememberedPath().c_str(), &size);
    if (!text)
        return std::string();
    std::string path(text, size);
    SDL_free(text);
    while (!path.empty() && SDL_strchr(" \t\r\n", path[path.size() - 1]))
        path.erase(path.size() - 1);
    return path;
}

void remember(const std::string& directory)
{
    if (readRemembered() == directory)
        return;
    std::string line = directory + "\n";
    if (!SDL_CreateDirectory(getUserDataPath(APPLICATION).c_str()) ||
        !SDL_SaveFile(getRememberedPath().c_str(), line.c_str(), line.size()))
        SDL_Log("cannot remember the game data folder: %s", SDL_GetError());
}

//! Where to copy harvestClientData so the game finds it: next to the app bundle the executable is
//! in, else next to the executable.
std::string getExecutableFolder()
{
    const char* base = SDL_GetBasePath();
    if (!base)
        return std::string();
    std::string folder = withoutTrailingSeparators(base);
    if (endsWith(folder, ".app/Contents/Resources"))
        return parentOf(parentOf(parentOf(folder)));
    return folder;
}

std::string howToFix(const std::string& executableFolder)
{
    std::string text = "To fix it, do one of these:\n";
    if (!executableFolder.empty())
        text += "- copy the harvestClientData folder into " + executableFolder + "\n";
    text += "- start the game with --data <folder>, where <folder> holds harvestClientData\n"
            "- set the HARVEST_DATA environment variable to that folder\n"
            "A folder given with --data or HARVEST_DATA is remembered for the next start.";
    return text;
}

} // end namespace

std::string getUserDataPath(const char* application)
{
#if defined(SDL_PLATFORM_LINUX)
    const char* home = SDL_getenv("HOME");
    if (home && *home)
        return std::string(home) + "/." + application;
#endif

    char* pref = SDL_GetPrefPath("", application);
    if (!pref)
    {
        SDL_Log("cannot find a user data directory: %s", SDL_GetError());
        return application;
    }
    std::string path = withoutTrailingSeparators(pref);
    SDL_free(pref);
    return path;
}

bool findGameData(std::string& problem)
{
    const std::string executableFolder = getExecutableFolder();

    // A folder named on the command line or in the environment is used as given, or not at all.
    const char* explicitSources[2][2] = {
        { "--data", g_options.DataDirectory },
        { "HARVEST_DATA", SDL_getenv("HARVEST_DATA") },
    };
    for (unsigned int i = 0; i < SDL_arraysize(explicitSources); ++i)
    {
        const char* source = explicitSources[i][0];
        const char* value = explicitSources[i][1];
        if (!value || !*value)
            continue;
        g_gameData = resolve(makeAbsolute(withoutTrailingSeparators(value)));
        if (g_gameData.empty())
        {
            problem = std::string("Harvest cannot find its game data: ") + source + " names " + value +
                ", which holds no harvestClientData folder (nor does an app bundle in it).\n\n" +
                howToFix(executableFolder);
            return false;
        }
        SDL_Log("game data: %s (%s)", g_gameData.c_str(), source);
        remember(g_gameData);
        return true;
    }

    CSearch search;
    bool found = search.look(readRemembered(), "the folder given last time");

    const char* base = SDL_GetBasePath();
    if (!found && base)
    {
        std::string folder = withoutTrailingSeparators(base);
        if (endsWith(folder, ".app/Contents/Resources"))
            found = search.look(folder, "inside the app bundle") ||
                search.look(executableFolder, "next to the app bundle");
        else
            found = search.look(folder, "next to the executable") ||
                search.look(parentOf(folder), "above the executable");
    }

    std::vector<std::string> steamRoots = getSteamRoots();
    bool steamSeen = false;
    for (unsigned int r = 0; !found && r < steamRoots.size(); ++r)
    {
        if (!isDirectory(steamRoots[r]))
            continue;
        steamSeen = true;
        std::vector<std::string> libraries = getSteamLibraries(steamRoots[r]);
        for (unsigned int l = 0; !found && l < libraries.size(); ++l)
            found = search.look(join(join(libraries[l], "steamapps/common"), STEAM_FOLDER), "Steam");
    }
    if (!found && !steamSeen)
        search.Searched.push_back("Steam libraries (no Steam installation found)");

#if defined(SDL_PLATFORM_MACOS)
    // app bundles holding the data, such as the DRM-free release's
    found = found || search.look("/Applications", "Applications") ||
        search.look(join(getHome(), "Applications"), "Applications");
#elif !defined(SDL_PLATFORM_WINDOWS)
    found = found || search.look(join(join(getHome(), "Games"), STEAM_FOLDER), "Games") ||
        search.look(join(getHome(), "Games/Harvest"), "Games");
#endif

    found = found || search.look(getUserDataPath(APPLICATION), "the user data folder");

    if (found)
    {
        g_gameData = search.Found;
        return true;
    }

    problem = "Harvest cannot find its game data: the harvestClientData folder from the original game "
              "(the Steam release or the DRM-free one).\n\nLooked in:\n";
    for (unsigned int i = 0; i < search.Searched.size(); ++i)
        problem += "- " + search.Searched[i] + "\n";
    problem += "\n" + howToFix(executableFolder);
    return false;
}

const std::string& getGameDataDirectory()
{
    return g_gameData;
}

} // end namespace port
