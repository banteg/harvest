// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/ILogger.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::event namespace; not the original source. The virtual order follows the
// Mac 1.18 vtable of daisy::CLogger.

#ifndef OX_EVENT_ILOGGER_H
#define OX_EVENT_ILOGGER_H

#include "../IUnknown.h"

namespace ox {
namespace event {

//! Possible log levels.
enum ELOG_LEVEL
{
    ELL_INFORMATION = 0,
    ELL_WARNING,
    ELL_ERROR,
    ELL_NONE
};

//! Interface for logging messages, warnings and errors. Messages below the log level are dropped.
class ILogger : public IUnknown
{
public:
    virtual ~ILogger() {}

    virtual ELOG_LEVEL getLogLevel() = 0;
    virtual void setLogLevel(ELOG_LEVEL ll) = 0;
    //! Logs "text: hint".
    virtual void log(const char* text, const char* hint, ELOG_LEVEL ll) = 0;
    virtual void log(const wchar_t* text, const wchar_t* hint, ELOG_LEVEL ll) = 0;
    //! printf-style messages.
    virtual void log(const char* format, ELOG_LEVEL ll, ...) = 0;
    virtual void log(const wchar_t* format, ELOG_LEVEL ll, ...) = 0;
    //! A printf-style message with the source file and line.
    virtual void logWithInfo(const wchar_t* format, const wchar_t* file, int line, ELOG_LEVEL ll, ...) = 0;
};

} // end namespace event
} // end namespace ox

#endif
