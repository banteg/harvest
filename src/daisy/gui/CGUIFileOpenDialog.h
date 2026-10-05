// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIFileOpenDialog.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_GUI_CGUIFILEOPENDIALOG_H
#define DAISY_GUI_CGUIFILEOPENDIALOG_H

#include "ox/gui/IGUIFileOpenDialog.h"

namespace ox {
namespace io {
class IFileList;
class IFileSystem;
} // end namespace io
namespace gui {
class IGUIButton;
class IGUILayout;
class IGUIListBox;
class IGUIStaticText;
} // end namespace gui
} // end namespace ox

namespace daisy {
namespace gui {

//! A window that lists the directories and the files matching a filter.
class CGUIFileOpenDialog : public ox::gui::IGUIFileOpenDialog
{
public:
    //! constructor; the dialog covers its parent and lists directory
    CGUIFileOpenDialog(ox::io::IFileSystem* fs, const wchar_t* title, ox::gui::IGUIEnvironment* environment,
        ox::gui::IGUIElement* parent, int id, const char* directory, const char* filter);

    //! destructor
    virtual ~CGUIFileOpenDialog();

    //! returns the filename of the selected file. Returns NULL, if no file was selected.
    virtual const wchar_t* getFilename();

    //! called if an event happened.
    virtual bool OnEvent(const ox::event::SEvent& event);

    //! removes a child; the dialog removes itself with its window
    virtual void removeChild(ox::gui::IGUIElement* child);

    //! draws the element and its children
    virtual void draw();

private:
    //! fills the listboxes with the directories and the files of a directory
    void fillListBox(const char* directory);

    //! sends the event that the file has been selected.
    void sendSelectedEvent();

    //! sends the event that the file choose process has been canceld
    void sendCancelEvent();

    ox::core::CPosition2d<int> DragStart;
    ox::core::CString<wchar_t> FileName;
    ox::core::CString<char> Filter;
    ox::gui::IGUILayout* Window;
    ox::gui::IGUIStaticText* FileNameText;
    ox::gui::IGUIButton* OKButton;
    ox::gui::IGUIButton* CancelButton;
    ox::gui::IGUIListBox* FileBox;
    ox::gui::IGUIListBox* DirBox;
    ox::io::IFileSystem* FileSystem;
    ox::io::IFileList* FileList;
    ox::io::IFileList* DirList;
};

} // end namespace gui
} // end namespace daisy

#endif
