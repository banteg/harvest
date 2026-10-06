// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CLogger.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_CLOGGER_H
#define DAISY_CLOGGER_H

#include "ox/event/ILogger.h"
#include "ox/event/IEventReceiver.h"
#include "ox/core/CCriticalSection.h"

namespace daisy {

//! The device logger: prints messages at or above the log level to stdout.
class CLogger : public ox::event::ILogger
{
public:
    CLogger(ox::event::IEventReceiver* receiver);

    virtual ox::event::ELOG_LEVEL getLogLevel();
    virtual void setLogLevel(ox::event::ELOG_LEVEL ll);
    virtual void log(const char* text, const char* hint, ox::event::ELOG_LEVEL ll);
    virtual void log(const wchar_t* text, const wchar_t* hint, ox::event::ELOG_LEVEL ll);
    virtual void log(const char* format, ox::event::ELOG_LEVEL ll, ...);
    virtual void log(const wchar_t* format, ox::event::ELOG_LEVEL ll, ...);
    virtual void logWithInfo(const wchar_t* format, const wchar_t* file, int line, ox::event::ELOG_LEVEL ll, ...);

    //! Sets the receiver; the Linux logger never sends it anything.
    void setReceiver(ox::event::IEventReceiver* receiver);

private:
    ox::event::ELOG_LEVEL LogLevel;
    ox::event::IEventReceiver* Receiver;
    ox::core::CCriticalSection Lock;
};

} // end namespace daisy

#endif
