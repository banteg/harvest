// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CSpriteAnimationImageBundle.h"
#include "ox/video/IVideoDriver.h"
// The native object includes an iostream static initializer.
#include <iostream> // IWYU pragma: keep

namespace daisy {
namespace video {

CSpriteAnimationImageBundle::CSpriteAnimationImageBundle(ox::video::ISpritePackage* package, SSpriteBundle* bundle,
                                                         ox::video::IVideoDriver* driver)
    : ox::video::ISpriteAnimationImage(package), Driver(driver)
{
    Offset.X = bundle->Header.OffsetX;
    Offset.Y = bundle->Header.OffsetY;
    OriginalOffset.X = bundle->Header.OriginalOffsetX;
    OriginalOffset.Y = bundle->Header.OriginalOffsetY;
    ImageId = bundle->Header.Id;
    Columns = bundle->Columns;
    for (unsigned int i = 0; i < bundle->Images.size(); ++i)
    {
        SImageRenderData data;
        data.Texture = package->getTexture(bundle->Images[i].TextureIndex);
        const SSpriteImage& image = bundle->Images[i];
        data.SourceRect = ox::core::CRect<int>(image.X, image.Y, image.X + image.Width, image.Y + image.Height);
        RenderData.push_back(data);
    }
}

CSpriteAnimationImageBundle::~CSpriteAnimationImageBundle()
{
}

void CSpriteAnimationImageBundle::draw(const ox::core::CPosition2d<int>& position, const ox::core::CRect<int>* clip,
                                       ox::video::SColor color)
{
    int column = Columns;
    ox::core::CPosition2d<int> cell(position);
    for (unsigned int i = 0; i < RenderData.size(); ++i)
    {
        Driver->draw2DImage(RenderData[i].Texture, cell + Offset, RenderData[i].SourceRect, clip, color, true);
        if (--column > 0)
        {
            cell.X += RenderData[i].SourceRect.getWidth();
        }
        else
        {
            column = Columns;
            cell.Y += RenderData[i].SourceRect.getHeight();
            cell.X = position.X;
        }
    }
}

void CSpriteAnimationImageBundle::drawMirrored(const ox::core::CPosition2d<float>& position, float scale,
                                               ox::video::SColor color)
{
}

void CSpriteAnimationImageBundle::drawMultipleColors(const ox::core::CAffineRect<float>& rect,
                                                     const ox::video::SColorCorners& colors)
{
}

void CSpriteAnimationImageBundle::drawCornerColors(const ox::core::CAffineRect<float>& rect,
                                                   const ox::video::SColorCorners& colors)
{
}

void CSpriteAnimationImageBundle::drawScaled(const ox::core::CPosition2d<float>& position, float scale,
                                             ox::video::SColor color)
{
}

void CSpriteAnimationImageBundle::drawRotated(const ox::core::CPosition2d<float>& position, float rotation,
                                              float scale, ox::video::SColor color)
{
}

void CSpriteAnimationImageBundle::drawFreeShape(const ox::core::CPosition2d<float>& corner1,
                                                const ox::core::CPosition2d<float>& corner2,
                                                const ox::core::CPosition2d<float>& corner3,
                                                const ox::core::CPosition2d<float>& corner4, ox::video::SColor color)
{
}

void CSpriteAnimationImageBundle::draw3d(const ox::core::CVector3d<float>& position, float rotation, float scale,
                                         bool mirrored, ox::video::SColor color)
{
}

ox::core::CPosition2d<int> CSpriteAnimationImageBundle::getOffset()
{
    return Offset;
}

ox::core::CPosition2d<int> CSpriteAnimationImageBundle::getOriginalOffset()
{
    return OriginalOffset;
}

ox::core::CDimension2d<int> CSpriteAnimationImageBundle::getSize()
{
    return ox::core::CDimension2d<int>(
        RenderData[0].SourceRect.getWidth() * (Columns - 1) + RenderData[Columns - 1].SourceRect.getWidth(),
        RenderData[0].SourceRect.getHeight() * (RenderData.size() / Columns - 1)
            + RenderData[RenderData.size() - 1].SourceRect.getHeight());
}

ox::core::CDimension2d<int> CSpriteAnimationImageBundle::getOriginalSize()
{
    return getSize();
}

ox::video::ITexture* CSpriteAnimationImageBundle::getTexture()
{
    return RenderData[0].Texture;
}

ox::core::CRect<int> CSpriteAnimationImageBundle::getTexturePosition()
{
    return RenderData[0].SourceRect;
}

} // end namespace video
} // end namespace daisy
