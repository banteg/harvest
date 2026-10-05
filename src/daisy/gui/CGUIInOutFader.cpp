// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIInOutFader.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#include "CGUIInOutFader.h"
#include "ox/gui/IGUIElementInline.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/video/IVideoDriver.h"
#include "daisy/os.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

//! constructor
CGUIInOutFader::CGUIInOutFader(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
    ox::core::CRect<int> rectangle)
    : IGUIInOutFader(environment, parent, id, rectangle)
{
    Action = EFA_NOTHING;
    StartTime = 0;
    EndTime = 0;

    setColor(ox::video::SColor(0, 0, 0, 0));
}

//! draws the element and its children
void CGUIInOutFader::draw()
{
    if (!IsVisible || !Action)
        return;

    unsigned int now = os::Timer::getTime();
    if (now > EndTime && Action == EFA_FADE_IN)
    {
        Action = EFA_NOTHING;
        return;
    }

    ox::video::IVideoDriver* driver = Environment->getVideoDriver();

    if (driver)
    {
        float d;

        if (now > EndTime)
            d = 0.0f;
        else
            d = (EndTime - now) / (float)(EndTime - StartTime);

        ox::video::SColor newCol = FullColor.getInterpolated(TransColor, d);
        driver->draw2DRectangle(newCol, AbsoluteRect, &AbsoluteClippingRect);
    }

    IGUIElement::draw();
}

//! Gets the color to fade out to or to fade in from.
ox::video::SColor CGUIInOutFader::getColor() const
{
    return Color;
}

//! Sets the color to fade out to or to fade in from.
void CGUIInOutFader::setColor(ox::video::SColor color)
{
    Color = color;
    FullColor = Color;
    TransColor = Color;

    if (Action == EFA_FADE_OUT)
    {
        FullColor.setAlpha(0);
        TransColor.setAlpha(255);
    }
    else if (Action == EFA_FADE_IN)
    {
        FullColor.setAlpha(255);
        TransColor.setAlpha(0);
    }
}

//! Returns if the fade in or out process is done.
bool CGUIInOutFader::isReady() const
{
    unsigned int now = os::Timer::getTime();
    return (now > EndTime);
}

//! Starts the fade in process.
void CGUIInOutFader::fadeIn(unsigned int time)
{
    StartTime = os::Timer::getTime();
    EndTime = StartTime + time;
    Action = EFA_FADE_IN;
    setColor(Color);
}

//! Starts the fade out process.
void CGUIInOutFader::fadeOut(unsigned int time)
{
    StartTime = os::Timer::getTime();
    EndTime = StartTime + time;
    Action = EFA_FADE_OUT;
    setColor(Color);
}

} // end namespace gui
} // end namespace daisy
