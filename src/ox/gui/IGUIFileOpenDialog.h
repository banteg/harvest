// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGUIFileOpenDialog.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source. The virtual order follows the
// Mac 1.18 vtable of daisy::gui::CGUIFileOpenDialog.

#ifndef OX_GUI_IGUIFILEOPENDIALOG_H
#define OX_GUI_IGUIFILEOPENDIALOG_H

#include "IGUIElement.h"

namespace ox {
namespace gui {

//! Standard file chooser dialog.
class IGUIFileOpenDialog : public IGUIElement
{
public:
    IGUIFileOpenDialog(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUIElement(environment, parent, id, rectangle)
    {
    }

    //! Returns the filename of the selected file. Returns NULL, if no file was selected.
    virtual const wchar_t* getFilename() = 0;
};

} // end namespace gui
} // end namespace ox

#endif
