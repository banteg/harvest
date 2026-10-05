// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUISCROLLBAR_H
#define DAISY_GUI_CGUISCROLLBAR_H

#include "ox/gui/IGUIScrollBar.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUIScrollBar : public ox::gui::IGUIScrollBar
{
public:
    CGUIScrollBar(bool horizontal, ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id, ox::core::CRect<int> rectangle, bool noClip);
    virtual ~CGUIScrollBar();
    virtual void setMax(int max);
    virtual int getMax();
    virtual void setStepSizes(int smallStep, int largeStep);
    virtual int getPos();
    virtual void setPos(int pos);
    virtual bool isDragging();
    virtual void setAnimations(ox::video::ISpritePackage* package, const char* name);

private:
    char Unrecovered[0x180 - sizeof(ox::gui::IGUIScrollBar)];
};

} // end namespace gui
} // end namespace daisy

#endif
