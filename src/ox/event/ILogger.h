// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/ILogger.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::event namespace; not the original source. Partial: only the log levels.

#ifndef OX_EVENT_ILOGGER_H
#define OX_EVENT_ILOGGER_H

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

} // end namespace event
} // end namespace ox

#endif
