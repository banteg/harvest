// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIFont.
#ifndef OX_GUI_IGUIFONT_H
#define OX_GUI_IGUIFONT_H
#include "ox/IUnknown.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CRect.h"
#include "ox/video/SColor.h"
namespace ox {
namespace gui {
enum EFontHorizontalAlign { EFHA_LEFT = 0, EFHA_CENTER = 1 };
enum EFontVerticalAlign { EFVA_TOP = 0, EFVA_CENTER = 1 };
class IGUIFont : public IUnknown
{
public:
    virtual void draw(const wchar_t* text, const core::CRect<int>& position, video::SColor color,
        EFontHorizontalAlign horizontal, EFontVerticalAlign vertical, const core::CRect<int>* clip) = 0;
    virtual core::CDimension2d<int> getDimension(const wchar_t* text) = 0;
    //! The index of the character at pixel offset x, or -1.
    virtual int getCharacterFromPos(const wchar_t* text, int x) = 0;
    virtual void setOriginalColor(const video::SColor& color) = 0;
    virtual video::SColor getRecentColor() = 0;
};
} // end namespace gui
} // end namespace ox
#endif
