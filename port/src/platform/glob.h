// A portable stand-in for glob(3), for daisy::io::CFileList (see docs/port/file-system.md).
//
// Only what CFileList uses: one pattern for the entries of the working directory, GLOB_MARK, no
// error callback. The pattern is matched against each entry name with shell rules: '*', '?',
// "[...]" (ranges, '!' or '^' negates, a leading ']' is literal), and '\' escapes the next
// character. Matching is case-sensitive and a leading '.' must be matched literally, as glob(3)
// does without GLOB_PERIOD. Patterns with a '/' are not supported (the game passes none).
//
// The names come back sorted by byte order (strcmp), the "C" locale's order. The original's glob
// sorted with strcoll under the user's locale (gtk_init sets it), so its order varied by system.

#ifndef HARVEST_PORT_PLATFORM_GLOB_H
#define HARVEST_PORT_PLATFORM_GLOB_H

#include <stddef.h>

namespace port {

enum
{
    //! Append '/' to the names of directories (symbolic links are followed).
    GLOB_MARK = 1 << 1
};

struct glob_t
{
    size_t gl_pathc;
    char** gl_pathv;
};

//! Fills result with the matching names of the working directory's entries. Returns 0, or 3
//! (glibc's GLOB_NOMATCH) when nothing matched; result is valid either way.
int glob(const char* pattern, int flags, int (*errfunc)(const char* path, int error), glob_t* result);

//! Frees the names glob allocated.
void globfree(glob_t* result);

//! Whether name matches the shell pattern, with the rules above.
bool matchPattern(const char* pattern, const char* name);

} // end namespace port

#endif
