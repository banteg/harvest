// A portable stand-in for glob(3); see glob.h for the rules.

#include "glob.h"

#include <algorithm>
#include <string.h>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

namespace port {

namespace {

//! Matches one bracket expression at *pattern (just past the '['). Returns the pattern after the
//! closing ']' and sets matched, or 0 if the bracket is not closed (then '[' is literal).
const char* matchBracket(const char* pattern, unsigned char c, bool* matched)
{
    bool negate = false;
    if (*pattern == '!' || *pattern == '^')
    {
        negate = true;
        ++pattern;
    }

    bool found = false;
    bool first = true;
    while (*pattern && (first || *pattern != ']'))
    {
        first = false;
        unsigned char low = (unsigned char)*pattern++;
        if (low == '\\' && *pattern)
            low = (unsigned char)*pattern++;

        unsigned char high = low;
        if (pattern[0] == '-' && pattern[1] && pattern[1] != ']')
        {
            ++pattern;
            high = (unsigned char)*pattern++;
            if (high == '\\' && *pattern)
                high = (unsigned char)*pattern++;
        }

        if (low <= c && c <= high)
            found = true;
    }

    if (*pattern != ']')
        return 0;

    *matched = found != negate;
    return pattern + 1;
}

bool matchFrom(const char* pattern, const char* name, bool atStart)
{
    for (;;)
    {
        // A leading '.' is only matched by a literal '.'.
        bool hiddenStart = atStart && *name == '.';
        atStart = false;

        switch (*pattern)
        {
        case '\0':
            return *name == '\0';

        case '*':
            if (hiddenStart)
                return false;
            while (*pattern == '*')
                ++pattern;
            if (!*pattern)
                return true;
            for (const char* rest = name; *rest; ++rest)
                if (matchFrom(pattern, rest, false))
                    return true;
            return matchFrom(pattern, name + strlen(name), false);

        case '?':
            if (!*name || hiddenStart)
                return false;
            ++pattern;
            ++name;
            break;

        case '[':
        {
            if (!*name)
                return false;
            bool matched = false;
            const char* next = matchBracket(pattern + 1, (unsigned char)*name, &matched);
            if (next)
            {
                if (!matched || hiddenStart)
                    return false;
                pattern = next;
                ++name;
                break;
            }
            // An unclosed '[' is a literal character.
            if (*name != '[')
                return false;
            ++pattern;
            ++name;
            break;
        }

        case '\\':
            if (pattern[1])
                ++pattern;
            // fall through
        default:
            if (*pattern != *name)
                return false;
            ++pattern;
            ++name;
            break;
        }
    }
}

bool lessBytes(const std::string& a, const std::string& b)
{
    return strcmp(a.c_str(), b.c_str()) < 0;
}

//! Appends the working directory's entries matching pattern, with '/' after directories if mark.
void listWorkingDirectory(const char* pattern, bool mark, std::vector<std::string>& names)
{
#ifdef _WIN32
    WIN32_FIND_DATAA data;
    HANDLE find = FindFirstFileA("*", &data);
    if (find == INVALID_HANDLE_VALUE)
        return;
    do
    {
        if (!matchPattern(pattern, data.cFileName))
            continue;
        std::string name(data.cFileName);
        if (mark && (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            name += '/';
        names.push_back(name);
    } while (FindNextFileA(find, &data));
    FindClose(find);
#else
    DIR* dir = opendir(".");
    if (!dir)
        return;
    while (dirent* entry = readdir(dir))
    {
        if (!matchPattern(pattern, entry->d_name))
            continue;
        std::string name(entry->d_name);
        struct stat info;
        if (mark && stat(entry->d_name, &info) == 0 && S_ISDIR(info.st_mode))
            name += '/';
        names.push_back(name);
    }
    closedir(dir);
#endif
}

} // end anonymous namespace

bool matchPattern(const char* pattern, const char* name)
{
    return matchFrom(pattern, name, true);
}

int glob(const char* pattern, int flags, int (*)(const char*, int), glob_t* result)
{
    std::vector<std::string> names;
    listWorkingDirectory(pattern, (flags & GLOB_MARK) != 0, names);
    std::sort(names.begin(), names.end(), lessBytes);

    result->gl_pathc = names.size();
    result->gl_pathv = new char*[names.size() + 1];
    for (size_t i = 0; i < names.size(); ++i)
    {
        result->gl_pathv[i] = new char[names[i].size() + 1];
        memcpy(result->gl_pathv[i], names[i].c_str(), names[i].size() + 1);
    }
    result->gl_pathv[names.size()] = 0;

    return names.empty() ? 3 : 0;
}

void globfree(glob_t* result)
{
    for (size_t i = 0; i < result->gl_pathc; ++i)
        delete[] result->gl_pathv[i];
    delete[] result->gl_pathv;
    result->gl_pathc = 0;
    result->gl_pathv = 0;
}

} // end namespace port
