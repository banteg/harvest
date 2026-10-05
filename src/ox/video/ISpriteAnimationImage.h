// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual interface follows the Mac vtable of daisy::video::CSpriteAnimationImage.

#ifndef OX_VIDEO_ISPRITEANIMATIONIMAGE_H
#define OX_VIDEO_ISPRITEANIMATIONIMAGE_H

#include "../IUnknown.h"
#include "../core/CDimension2d.h"
#include "../core/CPosition2d.h"
#include "../core/CRect.h"
#include "../core/CVector3d.h"
#include "ISpritePackage.h"
#include "SColor.h"

namespace ox {
namespace core {
template <class T> class CAffineRect;
} // end namespace core

namespace video {

class ITexture;
struct SColorCorners;

//! One frame of a sprite animation: a sprite image, or a bundle of images drawn as a grid.
//! Images are shared by the animation states of a sprite package and reference counted.
class ISpriteAnimationImage : public IUnknown
{
public:
    ISpriteAnimationImage(ISpritePackage* package)
        : Package(package)
    {
    }

    //! Releases the image from its package.
    virtual void remove()
    {
        if (Package)
            Package->removeImage(this);
    }

    virtual void draw(const core::CPosition2d<int>& position, const core::CRect<int>* clip, SColor color) = 0;
    virtual void drawMultipleColors(const core::CAffineRect<float>& rect, const SColorCorners& colors) = 0;
    virtual void drawCornerColors(const core::CAffineRect<float>& rect, const SColorCorners& colors) = 0;
    virtual void drawScaled(const core::CPosition2d<float>& position, float scale, SColor color) = 0;
    //! Draws rotated by an angle in radians and scaled.
    virtual void drawRotated(const core::CPosition2d<float>& position, float rotation, float scale, SColor color) = 0;
    virtual void drawMirrored(const core::CPosition2d<float>& position, float scale, SColor color) = 0;
    //! Draws the image stretched onto four corners.
    virtual void drawFreeShape(const core::CPosition2d<float>& corner1, const core::CPosition2d<float>& corner2,
        const core::CPosition2d<float>& corner3, const core::CPosition2d<float>& corner4, SColor color) = 0;
    //! Draws the image as a quad in the XY plane, rotated by an angle in radians and scaled.
    virtual void draw3d(const core::CVector3d<float>& position, float rotation, float scale, bool mirrored,
        SColor color) = 0;

    virtual core::CPosition2d<int> getOffset() = 0;
    virtual core::CPosition2d<int> getOriginalOffset() = 0;
    virtual core::CDimension2d<int> getSize() = 0;
    virtual core::CDimension2d<int> getOriginalSize() = 0;
    virtual ITexture* getTexture() = 0;
    //! The source rectangle of the image in its texture.
    virtual core::CRect<int> getTexturePosition() = 0;
    virtual int getImageId() = 0;

    //! Orders images by id, for the sorted image list of a package.
    bool operator<(ISpriteAnimationImage& other)
    {
        return getImageId() < other.getImageId();
    }

protected:
    ISpritePackage* Package;
};

} // end namespace video
} // end namespace ox

#endif
