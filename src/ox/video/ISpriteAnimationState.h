// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: the virtual interface follows the Mac vtable (names from daisy::video::CSpriteAnimationState);
// return types that no recovered code uses are not verified, and the members are not recovered yet.

#ifndef OX_VIDEO_ISPRITEANIMATIONSTATE_H
#define OX_VIDEO_ISPRITEANIMATIONSTATE_H

#include "../IUnknown.h"
#include "../core/CDimension2d.h"
#include "../core/CPosition2d.h"
#include "../core/CRect.h"
#include "../core/CVector3d.h"
#include "SColor.h"

namespace ox {
namespace io {
class IReadFile;
class IWriteFile;
} // end namespace io

namespace core {
template <class T> class CAffineRect;
} // end namespace core

namespace video {

class ITexture;
struct SColorCorners;

//! A running animation of a sprite.
class ISpriteAnimationState : public IUnknown
{
public:
    virtual void remove() = 0;
    virtual void reset() = 0;
    virtual bool update(float frameDelta) = 0;

    virtual void draw(const core::CPosition2d<int>& position, const core::CRect<int>* clip, SColor color) = 0;
    virtual void drawMultipleColors(const core::CAffineRect<float>& rect, const SColorCorners& colors) = 0;
    virtual void drawCornerColors(const core::CAffineRect<float>& rect, const SColorCorners& colors) = 0;
    virtual void drawScaled(const core::CPosition2d<float>& position, float scale, SColor color) = 0;
    //! Draws rotated by an angle in radians and scaled.
    virtual void drawRotated(const core::CPosition2d<float>& position, float rotation, float scale, SColor color) = 0;
    virtual void drawMirrored(const core::CPosition2d<float>& position, float scale, SColor color) = 0;
    //! Draws the frame stretched onto four corners.
    virtual void drawFreeShape(const core::CPosition2d<float>& corner1, const core::CPosition2d<float>& corner2,
        const core::CPosition2d<float>& corner3, const core::CPosition2d<float>& corner4, SColor color) = 0;
    virtual void draw3d(const core::CVector3d<float>& position, float scale, float rotation, bool billboard,
        SColor color) = 0;

    virtual core::CPosition2d<int> getFrameOffset(int frame) = 0;
    virtual core::CPosition2d<int> getFrameOriginalOffset(int frame) = 0;
    virtual core::CDimension2d<int> getFrameSize(int frame) = 0;
    virtual core::CDimension2d<int> getFrameOriginalSize(int frame) = 0;
    virtual ITexture* getFrameTexture(int frame) = 0;
    virtual core::CPosition2d<int> getFrameTexturePosition(int frame) = 0;

    virtual void setFlag(int flag, bool enabled);
    virtual bool hasFlag(int flag);

    virtual bool read(io::IReadFile* file) = 0;
    virtual bool write(io::IWriteFile* file) = 0;
};

} // end namespace video
} // end namespace ox

#endif
