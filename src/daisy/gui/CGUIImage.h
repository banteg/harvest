// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUIIMAGE_H
#define DAISY_GUI_CGUIIMAGE_H

#include "ox/gui/IGUIImage.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUIImage : public ox::gui::IGUIImage
{
public:
    CGUIImage(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id, ox::core::CRect<int> rectangle);
    virtual ~CGUIImage();
    virtual void setImage(ox::video::ITexture* image);
    virtual void setAnimation(const char* animation, ox::video::ISpritePackage* package);
    virtual void setOverrideColor(ox::video::SColor color);

private:
    char Unrecovered[0xd0 - sizeof(ox::gui::IGUIImage)];
};

} // end namespace gui
} // end namespace daisy

#endif
