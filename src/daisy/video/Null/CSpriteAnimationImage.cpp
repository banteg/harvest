// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CSpriteAnimationImage.h"
#include "ox/core/CAffineRect.h"
#include "ox/core/CMatrix4.h"
#include "ox/video/ITexture.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/S3DVertex.h"
#include "ox/video/SColorArray.h"
#include "ox/video/SMaterial.h"
#include <math.h>
// The native object includes an iostream static initializer.
#include <iostream> // IWYU pragma: keep

namespace daisy {
namespace video {

CSpriteAnimationImage::CSpriteAnimationImage(ox::video::ISpritePackage* package, SSpriteImage* image,
                                             ox::video::IVideoDriver* driver)
    : ox::video::ISpriteAnimationImage(package), Driver(driver)
{
    Offset.X = image->Header.OffsetX;
    Offset.Y = image->Header.OffsetY;
    OriginalOffset.X = image->Header.OriginalOffsetX;
    OriginalOffset.Y = image->Header.OriginalOffsetY;
    ImageId = image->Header.Id;
    OriginalSize.Width = image->OriginalWidth;
    OriginalSize.Height = image->OriginalHeight;
    Texture = Package->getTexture(image->TextureIndex);
    SourceRect = ox::core::CRect<int>(image->X, image->Y, image->X + image->Width, image->Y + image->Height);
}

CSpriteAnimationImage::~CSpriteAnimationImage()
{
}

void CSpriteAnimationImage::draw(const ox::core::CPosition2d<int>& position, const ox::core::CRect<int>* clip,
                                 ox::video::SColor color)
{
    Driver->draw2DImage(Texture, position + Offset,
        SourceRect, clip, color, true);
}

void CSpriteAnimationImage::drawMirrored(const ox::core::CPosition2d<float>& position, float scale,
                                         ox::video::SColor color)
{
    ox::core::CRect<int> sourceRect(SourceRect);
    ox::video::SColorArray colors(color);
    int width = SourceRect.getWidth();
    float x = (1 - Offset.X - width) * scale + position.X;
    float y = Offset.Y * scale + position.Y;
    float right = width * scale + x;
    float bottom = SourceRect.getHeight() * scale + y;
    ox::core::CPosition2d<float> upperLeft(right, y);
    ox::core::CPosition2d<float> upperRight(x, y);
    ox::core::CPosition2d<float> lowerLeft(right, bottom);
    ox::core::CPosition2d<float> lowerRight(x, bottom);
    Driver->draw2DImage(Texture, upperLeft, upperRight, lowerLeft, lowerRight, sourceRect, &colors, true);
}

void CSpriteAnimationImage::drawCornerColors(const ox::core::CAffineRect<float>& rect,
                                             const ox::video::SColorCorners& colors)
{
    ox::video::SColorArray corners;
    corners.Colors[0] = colors.Colors[0];
    corners.Colors[3] = colors.Colors[2];
    corners.Colors[1] = colors.Colors[6];
    corners.Colors[2] = colors.Colors[8];
    Driver->draw2DImage(Texture, rect.UpperLeftCorner, rect.UpperRightCorner, rect.LowerLeftCorner,
        rect.LowerRightCorner, SourceRect, &corners, true);
}

void CSpriteAnimationImage::drawMultipleColors(const ox::core::CAffineRect<float>& rect,
                                               const ox::video::SColorCorners& colors)
{
    ox::core::CPosition2d<float> center(
        (rect.UpperLeftCorner.X + rect.UpperRightCorner.X + rect.LowerLeftCorner.X + rect.LowerRightCorner.X) * 0.25f,
        (rect.UpperLeftCorner.Y + rect.UpperRightCorner.Y + rect.LowerLeftCorner.Y + rect.LowerRightCorner.Y) * 0.25f);
    ox::core::CPosition2d<float> top((rect.UpperLeftCorner.X + rect.UpperRightCorner.X) * 0.5f,
        (rect.UpperLeftCorner.Y + rect.UpperRightCorner.Y) * 0.5f);
    ox::core::CPosition2d<float> left((rect.UpperLeftCorner.X + rect.LowerLeftCorner.X) * 0.5f,
        (rect.UpperLeftCorner.Y + rect.LowerLeftCorner.Y) * 0.5f);
    ox::core::CPosition2d<float> right((rect.UpperRightCorner.X + rect.LowerRightCorner.X) * 0.5f,
        (rect.UpperRightCorner.Y + rect.LowerRightCorner.Y) * 0.5f);
    ox::core::CPosition2d<float> bottom((rect.LowerLeftCorner.X + rect.LowerRightCorner.X) * 0.5f,
        (rect.LowerLeftCorner.Y + rect.LowerRightCorner.Y) * 0.5f);
    int halfWidth = SourceRect.getWidth() >> 1;
    int halfHeight = SourceRect.getHeight() >> 1;
    const ox::core::CPosition2d<int>& from = SourceRect.UpperLeftCorner;
    const ox::core::CPosition2d<int>& to = SourceRect.LowerRightCorner;

    ox::video::SColorArray quadColors;
    quadColors.Colors[0] = colors.Colors[0];
    quadColors.Colors[3] = colors.Colors[1];
    quadColors.Colors[1] = colors.Colors[3];
    quadColors.Colors[2] = colors.Colors[4];
    Driver->draw2DImage(Texture, rect.UpperLeftCorner, top, left, center,
        ox::core::CRect<int>(from.X, from.Y, from.X + halfWidth, from.Y + halfHeight), &quadColors, true);
    quadColors.Colors[0] = colors.Colors[1];
    quadColors.Colors[3] = colors.Colors[2];
    quadColors.Colors[1] = colors.Colors[4];
    quadColors.Colors[2] = colors.Colors[5];
    Driver->draw2DImage(Texture, top, rect.UpperRightCorner, center, right,
        ox::core::CRect<int>(from.X + halfWidth, from.Y, to.X, from.Y + halfHeight), &quadColors, true);
    quadColors.Colors[0] = colors.Colors[3];
    quadColors.Colors[3] = colors.Colors[4];
    quadColors.Colors[1] = colors.Colors[6];
    quadColors.Colors[2] = colors.Colors[7];
    Driver->draw2DImage(Texture, left, center, rect.LowerLeftCorner, bottom,
        ox::core::CRect<int>(from.X, from.Y + halfHeight, from.X + halfWidth, to.Y), &quadColors, true);
    quadColors.Colors[0] = colors.Colors[4];
    quadColors.Colors[3] = colors.Colors[5];
    quadColors.Colors[1] = colors.Colors[7];
    quadColors.Colors[2] = colors.Colors[8];
    Driver->draw2DImage(Texture, center, right, bottom, rect.LowerRightCorner,
        ox::core::CRect<int>(from.X + halfWidth, from.Y + halfHeight, to.X, to.Y), &quadColors, true);
}

void CSpriteAnimationImage::drawScaled(const ox::core::CPosition2d<float>& position, float scale,
                                       ox::video::SColor color)
{
    float x = Offset.X * scale + position.X;
    float y = Offset.Y * scale + position.Y;
    float right = SourceRect.getWidth() * scale + x;
    float bottom = SourceRect.getHeight() * scale + y;
    ox::core::CPosition2d<float> upperLeft(x, y);
    ox::core::CPosition2d<float> upperRight(right, y);
    ox::core::CPosition2d<float> lowerLeft(x, bottom);
    ox::core::CPosition2d<float> lowerRight(right, bottom);
    ox::video::SColorArray colors(color);
    Driver->draw2DImage(Texture, upperLeft, upperRight, lowerLeft, lowerRight, SourceRect, &colors, true);
}

void CSpriteAnimationImage::drawRotated(const ox::core::CPosition2d<float>& position, float rotation, float scale,
                                        ox::video::SColor color)
{
    float c = cos(rotation) * scale;
    float s = sin(rotation) * scale;
    float x = Offset.X;
    float y = Offset.Y;
    ox::core::CPosition2d<float> upperLeft(x * c - y * s + position.X, y * c + x * s + position.Y);
    float right = Offset.X + SourceRect.getWidth();
    ox::core::CPosition2d<float> upperRight(right * c - y * s + position.X, y * c + right * s + position.Y);
    float bottom = Offset.Y + SourceRect.getHeight();
    ox::core::CPosition2d<float> lowerLeft(x * c - bottom * s + position.X, x * s + bottom * c + position.Y);
    ox::core::CPosition2d<float> lowerRight(right * c - bottom * s + position.X, bottom * c + right * s + position.Y);
    ox::video::SColorArray colors(color);
    Driver->draw2DImage(Texture, upperLeft, upperRight, lowerLeft, lowerRight, SourceRect, &colors, true);
}

void CSpriteAnimationImage::drawFreeShape(const ox::core::CPosition2d<float>& corner1,
                                          const ox::core::CPosition2d<float>& corner2,
                                          const ox::core::CPosition2d<float>& corner3,
                                          const ox::core::CPosition2d<float>& corner4, ox::video::SColor color)
{
    ox::video::SColorArray colors(color);
    Driver->draw2DImage(Texture, corner1, corner2, corner3, corner4, SourceRect, &colors, true);
}

void CSpriteAnimationImage::draw3d(const ox::core::CVector3d<float>& position, float rotation, float scale,
                                   bool mirrored, ox::video::SColor color)
{
    ox::video::S3DVertex vertices[4];
    ox::video::SMaterial material;
    material.MaterialType = ox::video::EMT_TRANSPARENT_ALPHA_CHANNEL;
    material.Texture1 = Texture;
    material.Lighting = false;
    material.BackfaceCulling = false;

    const ox::core::CDimension2d<int>& textureSize = Texture->getSize();
    if (mirrored)
    {
        vertices[0].TCoords.X = (float)SourceRect.LowerRightCorner.X / textureSize.Width;
        vertices[1].TCoords.X = (float)SourceRect.UpperLeftCorner.X / textureSize.Width;
    }
    else
    {
        vertices[0].TCoords.X = (float)SourceRect.UpperLeftCorner.X / textureSize.Width;
        vertices[1].TCoords.X = (float)SourceRect.LowerRightCorner.X / textureSize.Width;
    }
    vertices[0].TCoords.Y = (float)SourceRect.LowerRightCorner.Y / textureSize.Height;
    vertices[1].TCoords.Y = vertices[0].TCoords.Y;
    vertices[2].TCoords.X = vertices[1].TCoords.X;
    vertices[2].TCoords.Y = (float)SourceRect.UpperLeftCorner.Y / textureSize.Height;
    vertices[3].TCoords.X = vertices[0].TCoords.X;
    vertices[3].TCoords.Y = vertices[2].TCoords.Y;
    vertices[0].Color = color;
    vertices[1].Color = color;
    vertices[2].Color = color;
    vertices[3].Color = color;

    float c = cos(rotation) * scale;
    float s = sin(rotation) * scale;
    float x = mirrored ? 1 - Offset.X - SourceRect.getWidth() : Offset.X;
    float y = Offset.Y;
    float right = SourceRect.getWidth() + x;
    float bottom = SourceRect.getHeight() + y;
    vertices[0].Pos = ox::core::CVector3d<float>(x * c - bottom * s, -(bottom * c + x * s), 0.0f) + position;
    vertices[1].Pos = ox::core::CVector3d<float>(right * c - bottom * s, -(bottom * c + right * s), 0.0f) + position;
    vertices[2].Pos = ox::core::CVector3d<float>(right * c - y * s, -(y * c + right * s), 0.0f) + position;
    vertices[3].Pos = ox::core::CVector3d<float>(x * c - y * s, -(y * c + x * s), 0.0f) + position;

    unsigned short indices[6] = { 0, 2, 1, 0, 3, 2 };
    Driver->setTransform((ox::video::E_TRANSFORMATION_STATE)1, ox::core::CMatrix4());
    Driver->setMaterial(material);
    Driver->drawIndexedTriangleList(vertices, 4, indices, 2);
}

ox::core::CPosition2d<int> CSpriteAnimationImage::getOffset()
{
    return Offset;
}

ox::core::CPosition2d<int> CSpriteAnimationImage::getOriginalOffset()
{
    return OriginalOffset;
}

ox::core::CDimension2d<int> CSpriteAnimationImage::getSize()
{
    return ox::core::CDimension2d<int>(SourceRect.getWidth(), SourceRect.getHeight());
}

ox::core::CDimension2d<int> CSpriteAnimationImage::getOriginalSize()
{
    return OriginalSize;
}

ox::video::ITexture* CSpriteAnimationImage::getTexture()
{
    return Texture;
}

ox::core::CRect<int> CSpriteAnimationImage::getTexturePosition()
{
    return SourceRect;
}

} // end namespace video
} // end namespace daisy
