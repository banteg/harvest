// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/os.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Partial: only what recovered units use is declared.

#ifndef DAISY_OS_H
#define DAISY_OS_H

#include "ox/event/ILogger.h"

namespace daisy {
namespace os {

class Printer
{
public:
    //! Logs a text followed by a hint (usually a file name).
    static void log(const char* text, const char* hint, ox::event::ELOG_LEVEL level);
    //! Logs a printf-style message.
    static void log(const char* format, ox::event::ELOG_LEVEL level, ...);
    //! Logs a printf-style wide message.
    static void log(const wchar_t* format, ox::event::ELOG_LEVEL level, ...);
};

class Timer
{
public:
    //! Milliseconds since the timer was initialized.
    static unsigned int getTime();
};

} // end namespace os
} // end namespace daisy

#endif
