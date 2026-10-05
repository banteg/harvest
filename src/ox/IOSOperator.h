// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IOSOperator.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox namespace; not the original source. The virtual order follows the Mac
// 1.18 vtable of daisy::COSXOperator; return types unused by recovered callers are provisional.

#ifndef OX_IOSOPERATOR_H
#define OX_IOSOPERATOR_H

#include "core/CString.h"

namespace ox {

//! Operating system services.
class IOSOperator
{
public:
    virtual ~IOSOperator() {}

    virtual const wchar_t* getOperationSystemVersion() = 0;
    virtual void copyToClipboard(const char* text) = 0;
    virtual char* getTextFromClipboard() = 0;
    virtual const wchar_t* getComputerName() = 0;
    virtual int getRegistryIntValue(int root, const wchar_t* key, const wchar_t* name) = 0;
    virtual void setRegistryIntValue(int root, const wchar_t* key, const wchar_t* name, int value) = 0;
    virtual int messageBox(const wchar_t* caption, const wchar_t* text, int flags) = 0;
    virtual bool openURL(const wchar_t* url) = 0;
    virtual int getProcessID() = 0;
    virtual const char* getProcessExecutableName() = 0;
    virtual bool runApplication(const wchar_t* application, const wchar_t* arguments) = 0;
    //! The directory for the application's user data, named after application.
    virtual core::CString<char> getApplicationSupportPath(const char* application) = 0;
};

} // end namespace ox

#endif
