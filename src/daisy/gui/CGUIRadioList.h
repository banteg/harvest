// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef DAISY_GUI_CGUIRADIOLIST_H
#define DAISY_GUI_CGUIRADIOLIST_H

#include <vector>
#include "ox/gui/IGUIRadioList.h"

namespace daisy {
namespace gui {

class CGUICheckBox;

//! Radio buttons made of check boxes whose id is the negated id of the list.
class CGUIRadioList : public ox::gui::IGUIRadioList
{
public:
    CGUIRadioList(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent,
        ox::core::CRect<int> rectangle, int id);
    ~CGUIRadioList();

    //! Keeps one button checked and reports EGET_RADIOLIST_CHANGED.
    virtual bool OnEvent(const ox::event::SEvent& event);

    virtual ox::gui::IGUICheckBox* addRadioButton(const wchar_t* text);
    virtual int getRadioCount();
    virtual ox::gui::IGUICheckBox* getRadioButton(int index);
    virtual void checkRadioButton(int index);
    virtual int getSelection();
    virtual void setTextColor(ox::video::SColor color);

private:
    std::vector<CGUICheckBox*> Radios;
    int Selection;
    ox::video::SColor TextColor;
};

} // end namespace gui
} // end namespace daisy

#endif
