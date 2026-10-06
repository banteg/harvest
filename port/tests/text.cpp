// Tests the game's text handling where it depends on the C library: wide-string formatting of
// localized texts, the multibyte conversions, the language files, configuration files with non-ASCII
// values and file names and folders with non-ASCII names. These differ between glibc, macOS and
// Windows (wchar_t is 2 bytes there, the CRT's wide printf swaps %s and %S, and its C locale is not
// UTF-8), so the Windows smoke test (port/smoke/smoke-windows.sh) also runs it under Wine.
//
//   zig build test-text -- <data dir> [<scratch dir>]
//
// <data dir> holds harvestClientData/. The test writes into a new folder with a non-ASCII name inside
// <scratch dir> (default: SDL's preference path for "HarvestTextTest").

#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>
#include <SDL3/SDL.h>
#include "daisy/io/CFileSystem.h"
#include "harvest/settings/CSystemConfig.h"
#include "ox/core/CStringConversions.h"
#include "ox/game/CConfiguration.h"
#include "ox/io/IFileList.h"
#include "platform/locale.h"

using ox::core::CString;
using ox::core::CStringFunctions;

namespace {

int g_failures = 0;

std::string utf8(const wchar_t* text)
{
    char* converted = SDL_iconv_string("UTF-8", "WCHAR_T", (const char*)text, (wcslen(text) + 1) * sizeof(wchar_t));
    std::string result = converted ? converted : "?";
    SDL_free(converted);
    return result;
}

std::string number(long value)
{
    char text[32];
    std::snprintf(text, sizeof(text), "%ld", value);
    return text;
}

void check(bool condition, const std::string& what)
{
    std::printf("%s  %s\n", condition ? "ok  " : "FAIL", what.c_str());
    if (!condition)
        ++g_failures;
}

void checkText(const CString<wchar_t>& actual, const wchar_t* expected, const char* what)
{
    check(actual == CString<wchar_t>(expected), std::string(what) + ": \"" + utf8(actual.c_str()) + "\"");
}

const wchar_t NAME[] = L"Zoë Ångström 한글";
const char NAME_UTF8[] = "Zo\xc3\xab \xc3\x85ngstr\xc3\xb6m \xed\x95\x9c\xea\xb8\x80";

void testFormatting()
{
    harvest::settings::CSystemConfig* config = harvest::settings::gp_systemConfig;
    // "%s" in a language file takes a wide string (the game turns it into "%S" for glibc)
    checkText(config->getLocalizedText(L"prio:prioFor", NAME), L"Attack priorities for Zoë Ångström 한글",
        "a wide %s argument");
    checkText(config->getLocalizedText(L"gamepopups:deselect", L"ESC"), L"Deselect Building (ESC)",
        "a short wide %s argument");
    checkText(config->getLocalizedText(L"entity:construction", L"Solar Plant", 42), L"Construction of Solar Plant (42%)",
        "%s, %d and %%");
    checkText(config->getLocalizedText(L"no:suchKey"), L"??? no:suchKey ???", "a missing key");
    char number[32];
    std::snprintf(number, sizeof(number), "%.2f|%lld|%zu", 1.5, 1234567890123LL, (size_t)7);
    check(!std::strcmp(number, "1.50|1234567890123|7"), std::string("narrow printf: ") + number);
}

void testConversions()
{
    CString<wchar_t> wide = CStringFunctions::ansiToWide(CString<char>(NAME_UTF8));
    checkText(wide, NAME, "ansiToWide decodes UTF-8 (pasted text)");
    // the original sizes the buffer in characters, so the UTF-8 is cut at that many bytes (only
    // logs and the unused highscore host are narrowed)
    CString<char> narrow = CStringFunctions::wideToAnsi(CString<wchar_t>(NAME));
    check(narrow.size() > 0 && !std::strncmp(narrow.c_str(), NAME_UTF8, narrow.size()),
        "wideToAnsi encodes UTF-8 (cut to the original's buffer): " + std::string(narrow.c_str()));
    check(wcstol(L"  -42 ", 0, 10) == -42, "wcstol");
    check(sizeof(wchar_t) == 2 || sizeof(wchar_t) == 4, "wchar_t is " + number(sizeof(wchar_t)) + " bytes");
}

void testLanguages()
{
    harvest::settings::CSystemConfig* config = harvest::settings::gp_systemConfig;
    ox::TArray<harvest::settings::SLanguageFile> languages;
    config->getAllLanguages(languages);
    std::printf("\n");
    check(languages.size() == 8, "8 language files listed, got " + number(languages.size()));
    bool korean = false;
    for (unsigned int i = 0; i < languages.size(); ++i)
    {
        std::string name = utf8(languages[i].Name.c_str());
        bool opened = config->openLanguageFile(languages[i].Filename);
        CString<wchar_t> text = config->getLocalizedText(L"prio:prioFor", L"X");
        check(opened && text.size() > 2 && text.findFirst(L'X') != -1,
            "language " + name + ": \"" + utf8(text.c_str()) + "\"");
        if (languages[i].Name.findFirst(L'한') != -1)
            korean = true;
    }
    check(korean, "the Korean language file's name has Hangul");
    config->openLanguageFile(CString<char>("$GAME_RESOURCES$/harvestClientData/lang/english.cfg"));
}

void testFiles(ox::io::IFileSystem* fileSystem)
{
    // (existFile cannot tell: it fopens the path, which fails for folders on Windows)
    fileSystem->createDirectory("$HARVEST_USERDATA$/");
    fileSystem->createDirectory("$HARVEST_USERDATA$/profiles/");

    // a profile-like configuration with a non-ASCII value in a file with a non-ASCII name
    const char* path = "$HARVEST_USERDATA$/profiles/Pr\xc3\xb8" "fil.cfg";
    ox::game::CConfiguration* written = new ox::game::CConfiguration(fileSystem);
    written->setAttribute(L"profile:name", NAME);
    written->write(path);
    delete written;

    ox::game::CConfiguration* read = new ox::game::CConfiguration(fileSystem);
    check(read->read(path), "the configuration file is read back");
    CString<wchar_t> name;
    read->getAttribute(L"profile:name", name);
    checkText(name, NAME, "the configuration value survives");
    delete read;

    ox::io::IFileList* files = fileSystem->createFileList("*.cfg", "$HARVEST_USERDATA$/profiles/", (ox::io::EFileList)1);
    check(files->getFileCount() == 1, "one profile listed, got " + number(files->getFileCount()));
    if (files->getFileCount() == 1)
    {
        check(!std::strcmp(files->getFileName(0), "Pr\xc3\xb8" "fil.cfg"),
            std::string("the listed name is UTF-8: ") + files->getFileName(0));
        check(fileSystem->existFile(files->getFullFileName(0), false),
            std::string("the listed full path opens: ") + files->getFullFileName(0));
    }
    files->drop();
    check(fileSystem->deleteFile(path), "the profile is deleted");
}

} // end namespace

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::fprintf(stderr, "usage: %s <data dir> [<scratch dir>]\n", argv[0]);
        return 2;
    }
    port::useUtf8Locale();

    std::string scratch;
    if (argc > 2)
        scratch = std::string(argv[2]) + "/";
    else
    {
        char* pref = SDL_GetPrefPath("", "HarvestTextTest");
        scratch = pref ? pref : "./";
        SDL_free(pref);
    }

    daisy::io::CFileSystem* fileSystem = new daisy::io::CFileSystem();
    fileSystem->addDirectoryAlias("$GAME_RESOURCES$", argv[1]);
    fileSystem->addDirectoryAlias("$HARVEST_USERDATA$", (scratch + "H\xc3\xa5rvest \xed\x95\x9c").c_str());

    harvest::settings::gp_systemConfig =
        new harvest::settings::CSystemConfig(fileSystem, "$HARVEST_USERDATA$/no-such-settings.cfg");

    testFormatting();
    testConversions();
    testLanguages();
    testFiles(fileSystem);

    delete harvest::settings::gp_systemConfig;
    fileSystem->drop();
    std::printf("%d failure%s\n", g_failures, g_failures == 1 ? "" : "s");
    return g_failures ? 1 : 0;
}
