// Recovered for Harvest from the Linux 1.18 build; not the original source.

#include "CLinuxOperator.h"
#include <stdlib.h>
#include <string.h>
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

// The GTK 2 calls used here, declared as in <gtk/gtk.h> (the build flags carry no GTK include paths).
extern "C" {
typedef struct _GdkDisplay GdkDisplay;
typedef struct _GdkAtom* GdkAtom;
typedef struct _GtkClipboard GtkClipboard;
void gtk_init(int* argc, char*** argv);
GdkDisplay* gdk_display_get_default();
GtkClipboard* gtk_clipboard_get_for_display(GdkDisplay* display, GdkAtom selection);
char* gtk_clipboard_wait_for_text(GtkClipboard* clipboard);
void gtk_clipboard_set_text(GtkClipboard* clipboard, const char* text, int len);
}
//! _GDK_MAKE_ATOM(69), the CLIPBOARD selection.
#define GDK_SELECTION_CLIPBOARD ((GdkAtom)69)

namespace daisy {

// Original bug: the services below have empty bodies on Linux, value-returning ones included, so their results
// are whatever the return register held: callers must not rely on them. messageBox in particular
// shows nothing (CGameMain's fatal error box is silent on Linux).

const wchar_t* CLinuxOperator::getOperationSystemVersion()
{
}

const wchar_t* CLinuxOperator::getComputerName()
{
}

int CLinuxOperator::getRegistryIntValue(int root, const wchar_t* key, const wchar_t* name)
{
}

void CLinuxOperator::setRegistryIntValue(int root, const wchar_t* key, const wchar_t* name, int value)
{
}

int CLinuxOperator::messageBox(const wchar_t* caption, const wchar_t* text, int flags)
{
}

int CLinuxOperator::getProcessID()
{
}

const char* CLinuxOperator::getProcessExecutableName()
{
}

bool CLinuxOperator::runApplication(const wchar_t* application, const wchar_t* arguments)
{
}

CLinuxOperator::CLinuxOperator()
{
    gtk_init(0, 0);
}

CLinuxOperator::~CLinuxOperator()
{
}

//! The CLIPBOARD selection (not PRIMARY). The returned text is GTK's copy and is never freed.
char* CLinuxOperator::getTextFromClipboard()
{
    return gtk_clipboard_wait_for_text(gtk_clipboard_get_for_display(gdk_display_get_default(),
        GDK_SELECTION_CLIPBOARD));
}

void CLinuxOperator::copyToClipboard(const char* text)
{
    GtkClipboard* clipboard = gtk_clipboard_get_for_display(gdk_display_get_default(), GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clipboard, text, strlen(text));
}

//! "$HOME/." followed by the application name, e.g. "/home/user/.Harvest" (no trailing slash); with
//! HOME unset, "/.Harvest". The game mounts it as $HARVEST_USERDATA$.
ox::core::CString<char> CLinuxOperator::getApplicationSupportPath(const char* application)
{
    ox::core::CString<char> path(getenv("HOME"));
    return path + "/." + application;
}

//! Opens the URL with xdg-open through the shell; the wide URL is narrowed character by character and
//! not quoted. Original bug: returns nothing defined.
bool CLinuxOperator::openURL(const wchar_t* url)
{
    ox::core::CString<char> command("xdg-open ");
    command.append(ox::core::CString<char>(url));
    system(command.c_str());
}

//! "$HOME/" followed by the application name.
ox::core::CString<char> CLinuxOperator::getDocumentsPath(const char* application)
{
    ox::core::CString<char> path(getenv("HOME"));
    return path + "/" + application;
}

} // end namespace daisy
