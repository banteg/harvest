// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef DAISY_VIDEO_NULL_CSPRITEANIMATIONIMAGEBUNDLE_H
#define DAISY_VIDEO_NULL_CSPRITEANIMATIONIMAGEBUNDLE_H

#include "CSpriteAnimationImage.h"
#include "ox/TArray.h"

namespace daisy {
namespace video {

//! Image ids from this one on name bundles; lower ids name sprites.
const int SPRITE_BUNDLE_FIRST_ID = 100000;

//! A sprite package record for an image too large for one texture: a grid of sprite images, row by row.
struct SSpriteBundle
{
    SSpriteImageHeader Header;
    int Columns;
    ox::TArray<SSpriteImage> Images;
};

//! One cell of a bundle, resolved to its texture.
struct SImageRenderData
{
    ox::video::ITexture* Texture;
    ox::core::CRect<int> SourceRect;
};

//! Draws a bundle of sprite images as a grid. Only plain drawing is supported.
class CSpriteAnimationImageBundle : public ox::video::ISpriteAnimationImage
{
public:
    CSpriteAnimationImageBundle(ox::video::ISpritePackage* package, SSpriteBundle* bundle,
        ox::video::IVideoDriver* driver);
    virtual ~CSpriteAnimationImageBundle();

    virtual void draw(const ox::core::CPosition2d<int>& position, const ox::core::CRect<int>* clip,
        ox::video::SColor color);
    virtual void drawMultipleColors(const ox::core::CAffineRect<float>& rect, const ox::video::SColorCorners& colors);
    virtual void drawCornerColors(const ox::core::CAffineRect<float>& rect, const ox::video::SColorCorners& colors);
    virtual void drawScaled(const ox::core::CPosition2d<float>& position, float scale, ox::video::SColor color);
    virtual void drawRotated(const ox::core::CPosition2d<float>& position, float rotation, float scale,
        ox::video::SColor color);
    virtual void drawMirrored(const ox::core::CPosition2d<float>& position, float scale, ox::video::SColor color);
    virtual void drawFreeShape(const ox::core::CPosition2d<float>& corner1, const ox::core::CPosition2d<float>& corner2,
        const ox::core::CPosition2d<float>& corner3, const ox::core::CPosition2d<float>& corner4,
        ox::video::SColor color);
    virtual void draw3d(const ox::core::CVector3d<float>& position, float rotation, float scale, bool mirrored,
        ox::video::SColor color);

    virtual ox::core::CPosition2d<int> getOffset();
    virtual ox::core::CPosition2d<int> getOriginalOffset();
    virtual ox::core::CDimension2d<int> getSize();
    virtual ox::core::CDimension2d<int> getOriginalSize();
    virtual ox::video::ITexture* getTexture();
    virtual ox::core::CRect<int> getTexturePosition();
    virtual int getImageId() { return ImageId; }

private:
    int Columns;
    ox::core::CPosition2d<int> Offset;
    ox::core::CPosition2d<int> OriginalOffset;
    ox::TArray<SImageRenderData> RenderData;
    int ImageId;
    ox::video::IVideoDriver* Driver;
};

} // end namespace video
} // end namespace daisy

#endif
