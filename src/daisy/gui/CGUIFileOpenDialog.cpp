// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIFileOpenDialog.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye builds the dialog from a layout window with a directory list and a file list, and sends the
// result to the parent.

#include "CGUIFileOpenDialog.h"
#include "ox/core/CStringConversions.h"
#include "ox/gui/IGUIButton.h"
#include "ox/gui/IGUIElementInline.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUILayout.h"
#include "ox/gui/IGUIListBox.h"
#include "ox/gui/IGUIStaticText.h"
#include "ox/io/IFileList.h"
#include "ox/io/IFileSystem.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

//! File list modes of IFileSystem::createFileList.
static const ox::io::EFileList FILE_LIST_FILES = (ox::io::EFileList)1;
static const ox::io::EFileList FILE_LIST_DIRECTORIES = (ox::io::EFileList)2;

//! constructor
CGUIFileOpenDialog::CGUIFileOpenDialog(ox::io::IFileSystem* fs, const wchar_t* title,
    ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id, const char* directory,
    const char* filter)
    : IGUIFileOpenDialog(environment, parent, id, parent->getAbsolutePosition()), Filter(filter), Window(0),
      FileNameText(0), FileSystem(fs), FileList(0), DirList(0)
{
    Text = title;

    Window = Environment->addWindow(ox::core::CRect<int>(0, 0, 300, 300), false, title, this, -1);

    FileNameText = Environment->addStaticText(
        L"c:\\program files\\oxeye games\\the strategist\\strategist\\replays", ox::core::CRect<int>(0, 0, 200, 20),
        false, false, Window, -1, L"");
    FileNameText->LayoutFlags = "center";

    DirBox = Environment->addListBox(ox::core::CRect<int>(0, 0, 150, 250), Window, -1, true);
    DirBox->LayoutFlags = "br";

    FileBox = Environment->addListBox(ox::core::CRect<int>(0, 0, 300, 250), Window, -1, true);

    OKButton = Environment->addButton(ox::core::CRect<int>(0, 0, 90, 20), Window, -1, L"Open");
    OKButton->LayoutFlags = "br";

    CancelButton = Environment->addButton(ox::core::CRect<int>(0, 0, 90, 20), Window, -1, L"Cancel");

    Window->sortRiver(true, 10, 10, false);
    Window->centerOnParent();

    fillListBox(directory);
}

//! destructor
CGUIFileOpenDialog::~CGUIFileOpenDialog()
{
    if (FileList)
        FileList->drop();

    if (DirList)
        DirList->drop();
}

//! removes a child; the dialog removes itself with its window
void CGUIFileOpenDialog::removeChild(ox::gui::IGUIElement* child)
{
    IGUIElement::removeChild(child);

    if (Children.empty())
        remove();
}

//! returns the filename of the selected file. Returns NULL, if no file was selected.
const wchar_t* CGUIFileOpenDialog::getFilename()
{
    return FileName.c_str();
}

//! called if an event happened.
bool CGUIFileOpenDialog::OnEvent(const ox::event::SEvent& event)
{
    switch (event.EventType)
    {
    case ox::event::EET_GUI_EVENT:
        switch (event.GUIEvent.EventType)
        {
        case ox::gui::EGET_BUTTON_CLICKED:
            if (event.GUIEvent.Caller == CancelButton)
            {
                sendCancelEvent();
                remove();
                return true;
            }
            else if (event.GUIEvent.Caller == OKButton && FileName != L"")
            {
                sendSelectedEvent();
                remove();
                return true;
            }
            break;

        case ox::gui::EGET_LISTBOX_CHANGED:
            if (event.GUIEvent.Caller == FileBox)
            {
                int selected = FileBox->getSelected();
                if (FileList && FileSystem)
                {
                    if (FileList->isDirectory(selected))
                        FileName = L"";
                    else
                        FileName = ox::core::CStringFunctions::ansiToWide(FileList->getFullFileName(selected));
                }
            }
            break;

        case ox::gui::EGET_LISTBOX_SELECTED_AGAIN:
            if (event.GUIEvent.Caller == DirBox)
            {
                int selected = DirBox->getSelected();
                if (DirList && FileSystem)
                {
                    fillListBox(DirList->getFullFileName(selected));
                    FileName = L"";
                }
            }
            else
            {
                int selected = FileBox->getSelected();
                FileName = ox::core::CStringFunctions::ansiToWide(FileList->getFullFileName(selected));
                if (FileName.size() > 0)
                {
                    sendSelectedEvent();
                    remove();
                }
                return true;
            }
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }

    return Parent ? Parent->OnEvent(event) : false;
}

//! draws the element and its children
void CGUIFileOpenDialog::draw()
{
    if (!IsVisible)
        return;

    IGUIElement::draw();
}

//! fills the listboxes with the directories and the files of a directory
void CGUIFileOpenDialog::fillListBox(const char* directory)
{
    if (!FileSystem || !FileBox || !DirBox)
        return;

    if (FileList)
        FileList->drop();

    if (DirList)
        DirList->drop();

    FileBox->clear();
    DirBox->clear();

    FileList = FileSystem->createFileList(Filter.c_str(), directory, FILE_LIST_FILES);
    DirList = FileSystem->createFileList("*", directory, FILE_LIST_DIRECTORIES);

    ox::core::CString<wchar_t> s;

    for (int i = 0; i < DirList->getFileCount(); ++i)
    {
        s = L"";
        if (DirList->isDirectory(i))
            s = L"<";

        s.append(ox::core::CStringFunctions::ansiToWide(DirList->getFileName(i)));

        if (DirList->isDirectory(i))
            s.append(ox::core::CString<wchar_t>(L">"));

        DirBox->addTextItem(s.c_str(), 0, ox::video::SColor(0xffffffff), false, true);
    }

    DirBox->sortItems(false);
    DirBox->setSelectable(true);

    for (int i = 0; i < FileList->getFileCount(); ++i)
    {
        s = L"";
        if (FileList->isDirectory(i))
            s = L"<Folder> ";

        s.append(ox::core::CStringFunctions::ansiToWide(FileList->getFileName(i)));

        FileBox->addTextItem(s.c_str(), 0, ox::video::SColor(0xffffffff), false, true);
    }

    FileBox->sortItems(false);
    FileBox->setSelectable(true);

    if (FileNameText)
    {
        s = ox::core::CStringFunctions::ansiToWide(FileSystem->getWorkingDirectory());
        FileNameText->setText(s.c_str());
    }
}

//! sends the event that the file has been selected.
void CGUIFileOpenDialog::sendSelectedEvent()
{
    ox::event::SEvent event;
    event.EventType = ox::event::EET_GUI_EVENT;
    event.GUIEvent.Caller = this;
    event.GUIEvent.EventType = ox::gui::EGET_FILE_SELECTED;
    Parent->OnEvent(event);
}

//! sends the event that the file choose process has been canceld
void CGUIFileOpenDialog::sendCancelEvent()
{
    ox::event::SEvent event;
    event.EventType = ox::event::EET_GUI_EVENT;
    event.GUIEvent.Caller = this;
    event.GUIEvent.EventType = ox::gui::EGET_FILE_CHOOSE_DIALOG_CANCELLED;
    Parent->OnEvent(event);
}

} // end namespace gui
} // end namespace daisy
