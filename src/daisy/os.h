// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/os.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_OS_H
#define DAISY_OS_H

#include "ox/event/ILogger.h"

namespace daisy {
namespace os {

//! Forwards messages to the device's logger, if there is one yet.
class Printer
{
public:
    //! Prints the message and a newline to stdout, bypassing the logger.
    static void print(const char* message);
    static void log(const char* message, const char* hint, ox::event::ELOG_LEVEL level);
    //! Logs a printf-style message. The va_list is handed on as the logger's first variadic
    //! argument, so the format's arguments are never substituted (see os.cpp).
    static void log(const char* format, ox::event::ELOG_LEVEL level, ...);
    static void log(const wchar_t* format, ox::event::ELOG_LEVEL level, ...);
    static void logWithInfo(const wchar_t* format, const wchar_t* file, int line, ox::event::ELOG_LEVEL level,
        ...);

    //! The logger of the device; set by the CIrrDeviceStub constructor.
    static ox::event::ILogger* Logger;
};

//! The wall clock (gettimeofday).
class Timer
{
public:
    //! Does nothing on Linux.
    static void initTimer();
    //! Milliseconds: tv_sec * 1000 + tv_usec / 1000, truncated to 32 bits (it wraps).
    static unsigned int getTime();
    //! Seconds: tv_sec + tv_usec / 1000000.0.
    static double getFloatTime();
};

} // end namespace os
} // end namespace daisy

#endif
