// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CSpriteAnimationState.h"
#include "ox/io/CHelpIO.h"
#include "ox/video/ISpriteAnimationImage.h"
#include "ox/video/ISpritePackage.h"
// The native object includes an iostream static initializer.
#include <iostream> // IWYU pragma: keep

namespace daisy {
namespace video {

CSpriteAnimationState::CSpriteAnimationState(ox::video::ISpritePackage* package, SAnimationData* data,
                                             ox::video::IVideoDriver* driver)
    : ox::video::ISpriteAnimationState(package), Data(data), CurrentFrame(0), FrameTime(0.0f), Driver(driver)
{
    for (unsigned int i = 0; i < Data->Frames.size(); ++i)
    {
        ox::video::ISpriteAnimationImage* image = Package->addNewImage(Data->Frames[i]);
        if (image)
            Images.push_back(image);
    }
    reset();
}

CSpriteAnimationState::~CSpriteAnimationState()
{
}

void CSpriteAnimationState::remove()
{
    if (!Package)
        return;
    for (unsigned int i = 0; i < Images.size(); ++i)
        Images[i]->remove();
    Images.clear();
    Package->removeAnimationState(this);
}

void CSpriteAnimationState::reset()
{
    CurrentFrame = 0;
    FrameTime = Data->Durations[0] * 0.001f;
    if (FrameTime < 0.0f)
        FrameTime = SPRITE_FRAME_FOREVER;
}

bool CSpriteAnimationState::update(float frameDelta)
{
    bool restarted = false;
    if (!hasFlag(ESASF_PAUSED))
    {
        FrameTime -= frameDelta;
        if (FrameTime <= 0.0f)
        {
            CurrentFrame += Data->Jumps[CurrentFrame];
            if (CurrentFrame > 0)
            {
                if (CurrentFrame >= (int)Images.size())
                    CurrentFrame = Images.size() - 1;
            }
            else
            {
                CurrentFrame = 0;
                restarted = true;
            }
            float duration = Data->Durations[CurrentFrame] * 0.001f;
            if (!(duration >= 0.0f))
                FrameTime += SPRITE_FRAME_FOREVER;
            else
                FrameTime += duration;
        }
    }
    return restarted;
}

bool CSpriteAnimationState::read(ox::io::IReadFile* file)
{
    if (!file)
        return false;
    Flags = ox::io::CHelpIO::readInt(file);
    CurrentFrame = ox::io::CHelpIO::readInt(file);
    FrameTime = ox::io::CHelpIO::readFloat(file);
    CurrentFrame = CurrentFrame % (int)Images.size();
    return true;
}

bool CSpriteAnimationState::write(ox::io::IWriteFile* file)
{
    if (!file)
        return false;
    ox::io::CHelpIO::writeInt(file, Flags);
    ox::io::CHelpIO::writeInt(file, CurrentFrame);
    ox::io::CHelpIO::writeFloat(file, FrameTime);
    return true;
}

void CSpriteAnimationState::draw(const ox::core::CPosition2d<int>& position, const ox::core::CRect<int>* clip,
                                 ox::video::SColor color)
{
    Images[CurrentFrame]->draw(position, clip, color);
}

void CSpriteAnimationState::drawMirrored(const ox::core::CPosition2d<float>& position, float scale,
                                         ox::video::SColor color)
{
    Images[CurrentFrame]->drawMirrored(position, scale, color);
}

void CSpriteAnimationState::drawMultipleColors(const ox::core::CAffineRect<float>& rect,
                                               const ox::video::SColorCorners& colors)
{
    Images[CurrentFrame]->drawMultipleColors(rect, colors);
}

void CSpriteAnimationState::drawCornerColors(const ox::core::CAffineRect<float>& rect,
                                             const ox::video::SColorCorners& colors)
{
    Images[CurrentFrame]->drawCornerColors(rect, colors);
}

void CSpriteAnimationState::drawScaled(const ox::core::CPosition2d<float>& position, float scale,
                                       ox::video::SColor color)
{
    Images[CurrentFrame]->drawScaled(position, scale, color);
}

void CSpriteAnimationState::drawRotated(const ox::core::CPosition2d<float>& position, float rotation, float scale,
                                        ox::video::SColor color)
{
    Images[CurrentFrame]->drawRotated(position, rotation, scale, color);
}

void CSpriteAnimationState::drawFreeShape(const ox::core::CPosition2d<float>& corner1,
                                          const ox::core::CPosition2d<float>& corner2,
                                          const ox::core::CPosition2d<float>& corner3,
                                          const ox::core::CPosition2d<float>& corner4, ox::video::SColor color)
{
    Images[CurrentFrame]->drawFreeShape(corner1, corner2, corner3, corner4, color);
}

void CSpriteAnimationState::draw3d(const ox::core::CVector3d<float>& position, float rotation, float scale,
                                   bool mirrored, ox::video::SColor color)
{
    Images[CurrentFrame]->draw3d(position, rotation, scale, mirrored, color);
}

ox::core::CPosition2d<int> CSpriteAnimationState::getFrameOffset(int frame)
{
    if (frame < 0 || frame >= (int)Images.size())
        return ox::core::CPosition2d<int>(0, 0);
    return Images[frame]->getOffset();
}

ox::core::CPosition2d<int> CSpriteAnimationState::getFrameOriginalOffset(int frame)
{
    if (frame < 0 || frame >= (int)Images.size())
        return ox::core::CPosition2d<int>(0, 0);
    return Images[frame]->getOriginalOffset();
}

ox::core::CDimension2d<int> CSpriteAnimationState::getFrameSize(int frame)
{
    if (frame < 0 || frame >= (int)Images.size())
        return ox::core::CDimension2d<int>(1, 1);
    return Images[frame]->getSize();
}

ox::core::CDimension2d<int> CSpriteAnimationState::getFrameOriginalSize(int frame)
{
    if (frame < 0 || frame >= (int)Images.size())
        return ox::core::CDimension2d<int>(1, 1);
    return Images[frame]->getOriginalSize();
}

ox::video::ITexture* CSpriteAnimationState::getFrameTexture(int frame)
{
    if (frame < 0 || frame >= (int)Images.size())
        return 0;
    return Images[frame]->getTexture();
}

ox::core::CRect<int> CSpriteAnimationState::getFrameTexturePosition(int frame)
{
    if (frame < 0 || frame >= (int)Images.size())
        return ox::core::CRect<int>(0, 0, 0, 0);
    return Images[frame]->getTexturePosition();
}

} // end namespace video
} // end namespace daisy
