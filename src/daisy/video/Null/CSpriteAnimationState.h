// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef DAISY_VIDEO_NULL_CSPRITEANIMATIONSTATE_H
#define DAISY_VIDEO_NULL_CSPRITEANIMATIONSTATE_H

#include "ox/TArray.h"
#include "ox/core/CString.h"
#include "ox/video/ISpriteAnimationState.h"
#include <functional>

namespace ox {
namespace video {
class ISpriteAnimationImage;
class IVideoDriver;
} // end namespace video
} // end namespace ox

namespace daisy {
namespace video {

//! Flags of a sprite animation state.
enum E_SPRITE_ANIMATION_STATE_FLAG
{
    //! update() keeps the current frame.
    ESASF_PAUSED = 1
};

//! A frame with a negative duration is shown for this many seconds (48 hours), i.e. until replaced.
const float SPRITE_FRAME_FOREVER = 172800.0f;

//! An animation of a sprite package: a sequence of image ids with per frame durations and jumps.
struct SAnimationData
{
    ox::core::CString<char> Name;
    //! The animation name read as a number.
    int Id;
    //! The image id of each frame.
    ox::TArray<int> Frames;
    //! The duration of each frame in milliseconds; negative stops the animation on that frame.
    ox::TArray<int> Durations;
    //! How many frames to move on after each frame: 1 plays on, 0 holds, negative values loop back.
    ox::TArray<int> Jumps;
};

//! Finds an animation by name.
struct SAnimationMatchName : public std::binary_function<SAnimationData, ox::core::CString<char>, bool>
{
    bool operator()(const SAnimationData& data, const ox::core::CString<char>& name) const
    {
        return data.Name == name;
    }
};

//! Finds an animation by id.
struct SAnimationMatchId : public std::binary_function<SAnimationData, int, bool>
{
    bool operator()(const SAnimationData& data, const int& id) const
    {
        return data.Id == id;
    }
};

//! A running animation of a sprite package.
class CSpriteAnimationState : public ox::video::ISpriteAnimationState
{
public:
    CSpriteAnimationState(ox::video::ISpritePackage* package, SAnimationData* data, ox::video::IVideoDriver* driver);
    virtual ~CSpriteAnimationState();

    virtual void remove();
    virtual void reset();
    //! Advances the animation; returns true when it jumped back to its first frame.
    virtual bool update(float frameDelta);
    virtual bool read(ox::io::IReadFile* file);
    virtual bool write(ox::io::IWriteFile* file);

    virtual void draw(const ox::core::CPosition2d<int>& position, const ox::core::CRect<int>* clip,
        ox::video::SColor color);
    virtual void drawMirrored(const ox::core::CPosition2d<float>& position, float scale, ox::video::SColor color);
    virtual void drawMultipleColors(const ox::core::CAffineRect<float>& rect, const ox::video::SColorCorners& colors);
    virtual void drawCornerColors(const ox::core::CAffineRect<float>& rect, const ox::video::SColorCorners& colors);
    virtual void drawScaled(const ox::core::CPosition2d<float>& position, float scale, ox::video::SColor color);
    virtual void drawRotated(const ox::core::CPosition2d<float>& position, float rotation, float scale,
        ox::video::SColor color);
    virtual void drawFreeShape(const ox::core::CPosition2d<float>& corner1, const ox::core::CPosition2d<float>& corner2,
        const ox::core::CPosition2d<float>& corner3, const ox::core::CPosition2d<float>& corner4,
        ox::video::SColor color);
    virtual void draw3d(const ox::core::CVector3d<float>& position, float rotation, float scale, bool mirrored,
        ox::video::SColor color);

    virtual ox::core::CPosition2d<int> getFrameOffset(int frame);
    virtual ox::core::CPosition2d<int> getFrameOriginalOffset(int frame);
    virtual ox::core::CDimension2d<int> getFrameSize(int frame);
    virtual ox::core::CDimension2d<int> getFrameOriginalSize(int frame);
    virtual ox::video::ITexture* getFrameTexture(int frame);
    virtual ox::core::CRect<int> getFrameTexturePosition(int frame);

private:
    SAnimationData* Data;
    ox::TArray<ox::video::ISpriteAnimationImage*> Images;
    int CurrentFrame;
    //! Seconds left on the current frame.
    float FrameTime;
    ox::video::IVideoDriver* Driver;
};

} // end namespace video
} // end namespace daisy

#endif
