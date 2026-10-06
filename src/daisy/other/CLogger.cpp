// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CLogger.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
//
// Output: every message that passes the level check goes straight to the process's stdout and
// ends with a newline; nothing goes to stderr, a file or the event receiver (Irrlicht's
// EET_LOG_TEXT_EVENT is gone). The critical section is never taken, so lines from several threads
// can interleave. The char variants print with vprintf, the wchar_t ones with vwprintf, and both
// add the newline with a narrow printf: on glibc a stream takes the orientation of its first
// output, so whichever kind is printed first wins and later output of the other kind is dropped.

#include "CLogger.h"
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>
#include "ox/core/CStringFunctions.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {

CLogger::CLogger(ox::event::IEventReceiver* receiver)
    : LogLevel(ox::event::ELL_INFORMATION), Receiver(receiver)
{
}

//! Returns the current log level.
ox::event::ELOG_LEVEL CLogger::getLogLevel()
{
    return LogLevel;
}

//! Messages below the level are dropped; ELL_NONE messages are always printed.
void CLogger::setLogLevel(ox::event::ELOG_LEVEL ll)
{
    LogLevel = ll;
}

//! Prints "text: hint". The joined string is used as a printf format, so a '%' in either part is
//! interpreted.
void CLogger::log(const char* text, const char* hint, ox::event::ELOG_LEVEL ll)
{
    if (ll < LogLevel)
        return;

    ox::core::CString<char> s = text;
    s += ": ";
    s += hint;
    log(s.c_str(), ll);
}

//! Narrows both strings with wcstombs and prints them as "text: hint".
void CLogger::log(const wchar_t* text, const wchar_t* hint, ox::event::ELOG_LEVEL ll)
{
    if (ll < LogLevel)
        return;

    ox::core::CString<char> s1 = ox::core::CStringFunctions::wideToAnsi(text);
    ox::core::CString<char> s2 = ox::core::CStringFunctions::wideToAnsi(hint);
    log(s1.c_str(), s2.c_str(), ll);
}

//! printf-style; prints the formatted text and a newline to stdout.
void CLogger::log(const char* format, ox::event::ELOG_LEVEL ll, ...)
{
    if (ll < LogLevel)
        return;

    va_list args;
    va_start(args, ll);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

//! wprintf-style; prints the formatted text to stdout, then a newline with the narrow printf.
void CLogger::log(const wchar_t* format, ox::event::ELOG_LEVEL ll, ...)
{
    if (ll < LogLevel)
        return;

    va_list args;
    va_start(args, ll);
    vwprintf(format, args);
    printf("\n");
    va_end(args);
}

//! Formats the message into a 129-character buffer, then logs "LEVEL@(file:line) message" through
//! the wide variadic log, LEVEL being INFO, WARN, ERR or NONE. As built this is broken in several
//! ways a port should not copy:
//! - the va_list is used twice without va_copy, so on amd64 the second vswprintf, whose output
//!   is the one logged, reads the arguments after the ones the format asks for;
//! - a message longer than 128 characters is formatted into a new 2-character buffer, which is
//!   deleted before it is logged;
//! - "%s" in a wide format expects narrow strings, but the level name, file and message are wide,
//!   so glibc prints only their first character.
void CLogger::logWithInfo(const wchar_t* format, const wchar_t* file, int line, ox::event::ELOG_LEVEL ll, ...)
{
    if (ll < LogLevel)
        return;

    va_list args;
    va_start(args, ll);

    wchar_t stackBuffer[129];
    wchar_t* buffer = stackBuffer;
    int size = 129;
    if (vswprintf(stackBuffer, 128, format, args) > 128)
    {
        buffer = new wchar_t[2];
        size = 2;
    }
    vswprintf(buffer, size, format, args);

    if (buffer != stackBuffer)
        delete [] buffer;

    const wchar_t* level;
    switch (ll)
    {
    case ox::event::ELL_INFORMATION:
        level = L"INFO";
        break;
    case ox::event::ELL_WARNING:
        level = L"WARN";
        break;
    case ox::event::ELL_ERROR:
        level = L"ERR";
        break;
    default:
        level = L"NONE";
        break;
    }

    log(L"%s@(%s:%i) %s", ll, level, file, line, buffer);
    va_end(args);
}

//! Stores the receiver; the logger never sends it anything.
void CLogger::setReceiver(ox::event::IEventReceiver* receiver)
{
    Receiver = receiver;
}

} // end namespace daisy
