// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIMessageBox.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_GUI_CGUIMESSAGEBOX_H
#define DAISY_GUI_CGUIMESSAGEBOX_H

#include "CGUIWindow.h"
#include "ox/gui/IGUIStaticText.h"

namespace daisy {
namespace gui {

class CGUIMessageBox : public CGUIWindow
{
public:
    //! constructor
    CGUIMessageBox(ox::gui::IGUIEnvironment* environment, const wchar_t* caption, const wchar_t* text, int flags,
        ox::gui::IGUIElement* parent, int id, ox::core::CRect<int> rectangle);

    //! destructor
    ~CGUIMessageBox();

    //! called if an event happened.
    virtual bool OnEvent(const ox::event::SEvent& event);

    //! Return answers yes or OK and escape answers cancel or no.
    virtual bool OnEventInNonFocusState(const ox::event::SEvent& event);

private:
    ox::gui::IGUIButton* OkButton;
    ox::gui::IGUIButton* CancelButton;
    ox::gui::IGUIButton* YesButton;
    ox::gui::IGUIButton* NoButton;
    //! Holds the buttons in a centered row.
    ox::gui::IGUILayout* ButtonLayout;
    ox::gui::IGUIStaticText* StaticText;
    //! Set when return or escape went down, so that only their release answers.
    bool KeyPressed;
};

} // end namespace gui
} // end namespace daisy

#endif
