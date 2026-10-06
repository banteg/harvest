// Recovered for Harvest from the Linux 1.18 build; not the original source.

#ifndef DAISY_CLINUXOPERATOR_H
#define DAISY_CLINUXOPERATOR_H

#include "ox/IOSOperator.h"

namespace daisy {

//! The Linux operating system services: the clipboard through GTK 2, URLs through xdg-open and the
//! user data directories under $HOME. Most other services are empty stubs.
class CLinuxOperator : public ox::IOSOperator
{
public:
    CLinuxOperator();
    virtual ~CLinuxOperator();

    virtual const wchar_t* getOperationSystemVersion();
    virtual void copyToClipboard(const char* text);
    virtual char* getTextFromClipboard();
    virtual const wchar_t* getComputerName();
    virtual int getRegistryIntValue(int root, const wchar_t* key, const wchar_t* name);
    virtual void setRegistryIntValue(int root, const wchar_t* key, const wchar_t* name, int value);
    virtual int messageBox(const wchar_t* caption, const wchar_t* text, int flags);
    virtual bool openURL(const wchar_t* url);
    virtual int getProcessID();
    virtual const char* getProcessExecutableName();
    virtual bool runApplication(const wchar_t* application, const wchar_t* arguments);
    virtual ox::core::CString<char> getApplicationSupportPath(const char* application);
    //! A Linux-only slot the game never calls; the name is inferred.
    virtual ox::core::CString<char> getDocumentsPath(const char* application);

private:
    //! Unused on Linux; the Mac operator keeps the last clipboard text here.
    ox::core::CString<char> ClipboardText;
};

} // end namespace daisy

#endif
