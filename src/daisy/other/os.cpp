// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/os.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#include "daisy/os.h"
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace os {

ox::event::ILogger* Printer::Logger = 0;

void Printer::print(const char* message)
{
    printf("%s\n", message);
}

void Timer::initTimer()
{
}

// Both clocks are the wall clock, not a monotonic clock started with the device: game code only
// ever uses differences between two readings.
unsigned int Timer::getTime()
{
    static timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

double Timer::getFloatTime()
{
    static timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

void Printer::log(const char* message, const char* hint, ox::event::ELOG_LEVEL level)
{
    if (Logger)
        Logger->log(message, hint, level);
}

// Original bug: the variadic forwarders hand their va_list to the logger's variadic log as if it were the first
// format argument, so a message such as "Could not load %s" prints garbage instead of the name.
void Printer::log(const char* format, ox::event::ELOG_LEVEL level, ...)
{
    va_list args;
    va_start(args, level);
    if (Logger)
        Logger->log(format, level, args);
    va_end(args);
}

void Printer::log(const wchar_t* format, ox::event::ELOG_LEVEL level, ...)
{
    va_list args;
    va_start(args, level);
    if (Logger)
        Logger->log(format, level, args);
    va_end(args);
}

void Printer::logWithInfo(const wchar_t* format, const wchar_t* file, int line, ox::event::ELOG_LEVEL level, ...)
{
    va_list args;
    va_start(args, level);
    if (Logger)
        Logger->logWithInfo(format, file, line, level, args);
    va_end(args);
}

} // end namespace os
} // end namespace daisy
