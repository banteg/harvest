// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIImage.

#ifndef OX_GUI_IGUIIMAGE_H
#define OX_GUI_IGUIIMAGE_H

#include "IGUIElement.h"
#include "../video/SColor.h"

namespace ox {
namespace video { class ITexture; class ISpritePackage; }
namespace gui {

//! Shows a texture or a sprite animation.
class IGUIImage : public IGUIElement
{
public:
    virtual void setImage(video::ITexture* image) = 0;
    //! Shows the named animation of the package instead of a texture.
    virtual void setAnimation(const char* name, video::ISpritePackage* package) = 0;
    virtual void setOverrideColor(video::SColor color) = 0;
};

} // end namespace gui
} // end namespace ox

#endif
