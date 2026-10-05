// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual interface follows the Mac vtable (names from daisy::video::CSpritePackage); return
// types that no recovered code uses are not verified.

#ifndef OX_VIDEO_ISPRITEPACKAGE_H
#define OX_VIDEO_ISPRITEPACKAGE_H

#include "../IUnknown.h"
#include "../TArray.h"
#include "../core/CString.h"

namespace ox {
namespace video {

class ISpriteAnimation;
class ISpriteAnimationImage;
class ISpriteAnimationState;
class ITexture;

//! A set of textures and the sprite animations cut from them.
class ISpritePackage : public IUnknown
{
public:
    virtual void updateAllAnimations(float frameDelta) = 0;
    virtual const TArray<ITexture*>& getTextureList() = 0;
    //! The animation names (CUnicodeFont reads them as character codes).
    virtual const TArray<core::CString<char> >& getAnimationList() = 0;
    virtual ITexture* getTexture(int index) = 0;
    virtual ITexture* getTexture(const core::CString<char>& name) = 0;
    //! Starts a running state of the named animation.
    virtual ISpriteAnimationState* addNewAnimationState(const core::CString<char>& name) = 0;
    virtual ISpriteAnimationState* addNewAnimationState(int index) = 0;
    virtual void removeAnimationState(ISpriteAnimationState* state) = 0;
    virtual ISpriteAnimationImage* addNewImage(int index) = 0;
    virtual void removeImage(ISpriteAnimationImage* image) = 0;
    virtual bool animationStateExists(const core::CString<char>& name) = 0;
};

} // end namespace video
} // end namespace ox

#endif
