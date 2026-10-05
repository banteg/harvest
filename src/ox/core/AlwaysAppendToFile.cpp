// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The file name is ours; the Mac build has the function alone in an object between the ox network
// and GUI objects.

#include <cstdarg>
#include <cstdio>
// The native unit has an iostream static initializer, as do the other ox units.
#include <iostream> // IWYU pragma: keep

//! Appends the printf-style formatted text, at most 1 KB, to the file.
void AlwaysAppendToFile(const char* filename, const char* format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);

    FILE* file = fopen(filename, "a");
    if (file)
    {
        fputs(buffer, file);
        fclose(file);
    }
}
