// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef DAISY_VIDEO_NULL_CSPRITEANIMATIONIMAGE_H
#define DAISY_VIDEO_NULL_CSPRITEANIMATIONIMAGE_H

#include "ox/video/ISpriteAnimationImage.h"

namespace ox {
namespace video {
class IVideoDriver;
} // end namespace video
} // end namespace ox

namespace daisy {
namespace video {

//! The record that starts every sprite and bundle in a sprite package file.
struct SSpriteImageHeader
{
    //! Ids below SPRITE_BUNDLE_FIRST_ID are sprites, the others bundles.
    int Id;
    //! Where the trimmed image is drawn relative to the sprite origin.
    int OffsetX;
    int OffsetY;
    //! The offset before trimming.
    int OriginalOffsetX;
    int OriginalOffsetY;
};

//! A sprite image of a package: a rectangle of one of its textures.
struct SSpriteImage : public SSpriteImageHeader
{
    int TextureIndex;
    int X;
    int Y;
    int Width;
    int Height;
    //! The size before trimming.
    int OriginalWidth;
    int OriginalHeight;
};

//! Draws one sprite image of a package.
class CSpriteAnimationImage : public ox::video::ISpriteAnimationImage
{
public:
    CSpriteAnimationImage(ox::video::ISpritePackage* package, SSpriteImage* image, ox::video::IVideoDriver* driver);
    virtual ~CSpriteAnimationImage();

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
    ox::core::CPosition2d<int> Offset;
    ox::core::CPosition2d<int> OriginalOffset;
    ox::video::ITexture* Texture;
    ox::core::CRect<int> SourceRect;
    ox::core::CDimension2d<int> OriginalSize;
    int ImageId;
    ox::video::IVideoDriver* Driver;
};

} // end namespace video
} // end namespace daisy

#endif
