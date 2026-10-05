// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef DAISY_VIDEO_NULL_CSPRITEPACKAGE_H
#define DAISY_VIDEO_NULL_CSPRITEPACKAGE_H

#include "CSpriteAnimationImageBundle.h"
#include "CSpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"

namespace ox {
namespace io {
class IReadFile;
} // end namespace io
} // end namespace ox

namespace daisy {
namespace video {

//! Orders running animation states by address, for removeAnimationState.
struct SAnimationPointerSearcher
{
    int operator()(ox::video::ISpriteAnimationState* state, ox::video::ISpriteAnimationState* element) const
    {
        if (state < element)
            return -1;
        if (state > element)
            return 1;
        return 0;
    }
};

//! Finds a shared image instance by image id.
template <class T>
struct SAnimationImageIdSearcher
{
    int operator()(int id, T image) const
    {
        return id - image->getImageId();
    }
};

//! Finds a sprite or bundle record by id.
template <class T>
struct SImageIdSearcher
{
    int operator()(int id, const T& image) const
    {
        return id - image.Header.Id;
    }
};

//! Sorts sprite and bundle records by id.
template <class T>
struct SImageIdSortFunctor
{
    bool operator()(const T& a, const T& b) const
    {
        return a.Header.Id < b.Header.Id;
    }
};

//! A sprite package (.dat): square textures followed by the sprites cut from them, the bundles
//! that tile larger images from sprites, and the animations over sprite and bundle ids.
//!
//! The file is little endian: the version (at most 3), the texture size, the texture format, then
//! the texture, sprite, bundle and animation counts (a 28 byte header). The textures follow as raw
//! channel planes (see readTexture) and are decoded on first use. Every sprite and bundle record
//! starts with a NUL terminated name, the id and the offset (version 2 adds a trim offset). A
//! sprite adds the texture index and its rectangle (version 3 adds the untrimmed size); a bundle
//! adds its column count and its sprites. An animation is a NUL terminated name (its id when read
//! as a number), the frame count and that many image ids, frame jumps and durations.
class CSpritePackage : public ox::video::ISpritePackage
{
public:
    //! keepStates: whether the package tracks the states it creates, for updateAllAnimations.
    CSpritePackage(ox::video::IVideoDriver* driver, bool keepStates);
    virtual ~CSpritePackage();

    virtual void updateAllAnimations(float frameDelta);
    virtual const ox::TArray<ox::core::CString<char> >& getTextureList();
    virtual const ox::TArray<ox::core::CString<char> >& getAnimationList();
    virtual bool animationStateExists(const ox::core::CString<char>& name);
    virtual ox::video::ISpriteAnimationState* addNewAnimationState(const ox::core::CString<char>& name);
    virtual ox::video::ISpriteAnimationState* addNewAnimationState(int id);
    virtual void removeAnimationState(ox::video::ISpriteAnimationState* state);
    virtual ox::video::ISpriteAnimationImage* addNewImage(int id);
    virtual void removeImage(ox::video::ISpriteAnimationImage* image);
    virtual ox::video::ITexture* getTexture(int index);
    virtual ox::video::ITexture* getTexture(const ox::core::CString<char>& name);

    //! Reads the package; filename names its textures. Returns false for an unknown version.
    bool load(ox::io::IReadFile* file, const char* filename);

private:
    //! Decodes texture index from the package file and adds it to the driver.
    ox::video::ITexture* readTexture(int index);
    void readSprite(SSpriteImage& image, ox::io::IReadFile* file, int version);
    void readSpriteHeader(SSpriteImageHeader& header, ox::io::IReadFile* file, int version);

    ox::video::IVideoDriver* Driver;
    //! The width and height of every texture.
    int TextureSize;
    //! 0: RGB planes, 1: RGBA planes, 2: an alpha plane, 3: one byte per pixel, alpha in the top bit.
    int TextureFormat;
    int TextureCount;
    int SpriteCount;
    int BundleCount;
    int AnimationCount;
    ox::TArray<ox::core::CString<char> > TextureNames;
    //! Whether each texture was decoded already.
    ox::TArray<int> TextureLoaded;
    int TextureDataSize;
    int HeaderSize;
    ox::core::CString<char> Filename;
    ox::TArray<ox::core::CString<char> > AnimationNames;
    //! Sprites and bundles, sorted by id.
    ox::TArray<SSpriteImage> Sprites;
    ox::TArray<SSpriteBundle> Bundles;
    ox::TArray<SAnimationData> Animations;
    ox::TArray<ox::video::ISpriteAnimationState*> States;
    //! The shared image instances, sorted by image id.
    ox::TArray<ox::video::ISpriteAnimationImage*> Images;
    bool KeepStates;
    bool StatesSorted;
};

} // end namespace video
} // end namespace daisy

#endif
