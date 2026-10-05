// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIInOutFader.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_GUI_CGUIINOUTFADER_H
#define DAISY_GUI_CGUIINOUTFADER_H

#include "ox/gui/IGUIInOutFader.h"

namespace daisy {
namespace gui {

class CGUIInOutFader : public ox::gui::IGUIInOutFader
{
public:
    //! constructor
    CGUIInOutFader(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle);

    //! draws the element and its children
    virtual void draw();

    //! Gets the color to fade out to or to fade in from.
    virtual ox::video::SColor getColor() const;

    //! Sets the color to fade out to or to fade in from.
    virtual void setColor(ox::video::SColor color);

    //! Starts the fade in process.
    virtual void fadeIn(unsigned int time);

    //! Starts the fade out process.
    virtual void fadeOut(unsigned int time);

    //! Returns if the fade in or out process is done.
    virtual bool isReady() const;

private:
    enum EFadeAction
    {
        EFA_NOTHING = 0,
        EFA_FADE_IN,
        EFA_FADE_OUT
    };

    unsigned int StartTime;
    unsigned int EndTime;
    EFadeAction Action;

    ox::video::SColor Color;
    ox::video::SColor FullColor;
    ox::video::SColor TransColor;
};

} // end namespace gui
} // end namespace daisy

#endif
