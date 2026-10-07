// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CSpritePackage.h"
#include "CImage.h"
#include "ox/algo/CArrayFunctions.h"
#include "ox/algo/SPointerSortFunctor.h"
#include "ox/io/CHelpIO.h"
#include "ox/io/IFileSystem.h"
#include "ox/io/IReadFile.h"
#include "ox/video/IVideoDriver.h"
#include <stdlib.h>
#include <string.h>
// The native object includes an iostream static initializer.
#include <iostream> // IWYU pragma: keep

namespace daisy {
namespace video {

bool CSpritePackage::load(ox::io::IReadFile* file, const char* filename)
{
    unsigned int version = ox::io::CHelpIO::readInt(file);
    if (version > 3)
        return false;

    TextureSize = ox::io::CHelpIO::readInt(file);
    TextureFormat = ox::io::CHelpIO::readInt(file);
    TextureCount = ox::io::CHelpIO::readInt(file);
    SpriteCount = ox::io::CHelpIO::readInt(file);
    BundleCount = ox::io::CHelpIO::readInt(file);
    AnimationCount = ox::io::CHelpIO::readInt(file);
    HeaderSize = SPRITE_PACKAGE_HEADER_SIZE;
    TextureDataSize = TextureSize * TextureSize;
    switch (TextureFormat)
    {
    case 0:
        TextureDataSize *= 3;
        break;
    case 1:
        TextureDataSize *= 4;
        break;
    }

    for (int i = 0; i < TextureCount; ++i)
    {
        ox::core::CString<char> name(i);
        name.append(ox::core::CString<char>("#"));
        name.append(ox::core::CString<char>(file->getFileName()));
        TextureNames.push_back(name);
        TextureLoaded.push_back(0);
    }
    Filename = filename;

    file->seek(TextureCount * TextureDataSize + HeaderSize);
    for (int i = 0; i < SpriteCount; ++i)
    {
        SSpriteImage image;
        readSprite(image, file, version);
        Sprites.push_back(image);
    }
    std::sort(Sprites.begin(), Sprites.end(), SImageIdSortFunctor<SSpriteImage>());

    for (int i = 0; i < BundleCount; ++i)
    {
        SSpriteBundle bundle;
        readSpriteHeader(bundle.Header, file, version);
        bundle.Columns = ox::io::CHelpIO::readInt(file);
        int count = ox::io::CHelpIO::readInt(file);
        for (int j = count; j > 0; --j)
        {
            SSpriteImage image;
            readSprite(image, file, version);
            bundle.Images.push_back(image);
        }
        Bundles.push_back(bundle);
    }
    std::sort(Bundles.begin(), Bundles.end(), SImageIdSortFunctor<SSpriteBundle>());

    for (int i = 0; i < AnimationCount; ++i)
    {
        SAnimationData animation;
        char name[256];
        char* p = name;
        char c = 0;
        do
        {
            file->read(&c, 1);
            *p = c;
        } while (*p++);
        animation.Name = name;
        animation.Id = atoi(animation.Name.c_str());

        int count = ox::io::CHelpIO::readInt(file);
        int value;
        for (int j = count; j > 0; --j)
        {
            value = ox::io::CHelpIO::readInt(file);
            animation.Frames.push_back(value);
        }
        for (int j = count; j > 0; --j)
        {
            value = ox::io::CHelpIO::readInt(file);
            animation.Jumps.push_back(value);
        }
        for (int j = count; j > 0; --j)
        {
            value = ox::io::CHelpIO::readInt(file);
            animation.Durations.push_back(value);
        }

        Animations.push_back(animation);
        AnimationNames.push_back(animation.Name);
    }
    return true;
}

CSpritePackage::CSpritePackage(ox::video::IVideoDriver* driver, bool keepStates)
    : TextureDataSize(0), HeaderSize(0), KeepStates(keepStates), StatesSorted(false)
{
    Driver = driver;
}

CSpritePackage::~CSpritePackage()
{
    for (unsigned int i = 0; i < States.size(); ++i)
        States[i]->drop();
    States.clear();

    for (unsigned int i = 0; i < Images.size(); ++i)
        while (!Images[i]->drop())
            ;
    Images.clear();

    if (Driver)
    {
        for (unsigned int i = 0; i < TextureNames.size(); ++i)
            Driver->removeTexture(TextureNames[i].c_str());
        TextureNames.clear();
    }
}

void CSpritePackage::removeImage(ox::video::ISpriteAnimationImage* image)
{
    int index = ox::algo::binarySearchIf(Images, SAnimationImageIdSearcher<ox::video::ISpriteAnimationImage*>(),
        image->getImageId());
    if (index >= 0 && image->drop())
        Images.erase(Images.begin() + index);
}

void CSpritePackage::updateAllAnimations(float frameDelta)
{
    for (unsigned int i = 0; i < States.size(); ++i)
        States[i]->update(frameDelta);
}

const ox::TArray<ox::core::CString<char> >& CSpritePackage::getTextureList()
{
    return TextureNames;
}

const ox::TArray<ox::core::CString<char> >& CSpritePackage::getAnimationList()
{
    return AnimationNames;
}

bool CSpritePackage::animationStateExists(const ox::core::CString<char>& name)
{
    return ox::algo::linearSearchIf(Animations.begin(), Animations.end(), SAnimationMatchName(), name) >= 0;
}

ox::video::ISpriteAnimationState* CSpritePackage::addNewAnimationState(const ox::core::CString<char>& name)
{
    int index = ox::algo::linearSearchIf(Animations.begin(), Animations.end(), SAnimationMatchName(), name);
    if (index < 0)
        return 0;
    ox::video::ISpriteAnimationState* state = new CSpriteAnimationState(this, &Animations[index], Driver);
    if (KeepStates)
        States.push_back(state);
    StatesSorted = false;
    return state;
}

ox::video::ISpriteAnimationState* CSpritePackage::addNewAnimationState(int id)
{
    int index = ox::algo::linearSearchIf(Animations.begin(), Animations.end(), SAnimationMatchId(), id);
    if (index < 0)
        return 0;
    ox::video::ISpriteAnimationState* state = new CSpriteAnimationState(this, &Animations[index], Driver);
    if (KeepStates)
        States.push_back(state);
    StatesSorted = false;
    return state;
}

void CSpritePackage::removeAnimationState(ox::video::ISpriteAnimationState* state)
{
    if (!StatesSorted)
    {
        std::sort(States.begin(), States.end());
        StatesSorted = true;
    }

    int index = ox::algo::binarySearchIf(States, SAnimationPointerSearcher(), state);
    if (index >= 0)
    {
        state->drop();
        States.erase(States.begin() + index);
    }
    else if (state)
    {
        state->drop();
    }
}

ox::video::ISpriteAnimationImage* CSpritePackage::addNewImage(int id)
{
    int index = ox::algo::binarySearchIf(Images, SAnimationImageIdSearcher<ox::video::ISpriteAnimationImage*>(), id);
    if (index >= 0)
    {
        Images[index]->grab();
        return Images[index];
    }

    ox::video::ISpriteAnimationImage* bundle;
    ox::video::ISpriteAnimationImage* sprite;
    if (id >= SPRITE_BUNDLE_FIRST_ID)
    {
        index = ox::algo::binarySearchIf(Bundles, SImageIdSearcher<SSpriteBundle>(), id);
        if (index < 0)
            return 0;
        bundle = new CSpriteAnimationImageBundle(this, &Bundles[index], Driver);
        Images.push_back(bundle);
        std::sort(Images.begin(), Images.end(), ox::algo::SPointerSortFunctor<ox::video::ISpriteAnimationImage*>());
        return bundle;
    }
    index = ox::algo::binarySearchIf(Sprites, SImageIdSearcher<SSpriteImage>(), id);
    if (index < 0)
        return 0;
    sprite = new CSpriteAnimationImage(this, &Sprites[index], Driver);
    Images.push_back(sprite);
    std::sort(Images.begin(), Images.end(), ox::algo::SPointerSortFunctor<ox::video::ISpriteAnimationImage*>());
    return sprite;
}

ox::video::ITexture* CSpritePackage::getTexture(int index)
{
    if (TextureLoaded[index])
        return Driver->getTexture(TextureNames[index].c_str());
    return readTexture(index);
}

ox::video::ITexture* CSpritePackage::readTexture(int index)
{
    if (index < 0 || index >= (int)TextureNames.size())
        return 0;

    ox::io::IReadFile* file = Driver->getFileSystem()->createAndOpenFile(Filename.c_str());
    if (!file)
        return 0;
    file->seek(TextureDataSize * index + HeaderSize);

    int pixelCount = TextureSize * TextureSize;
    std::vector<unsigned char> red;
    std::vector<unsigned char> green;
    std::vector<unsigned char> blue;
    std::vector<unsigned char> alpha;
    red.resize(pixelCount);
    green.resize(pixelCount);
    blue.resize(pixelCount);
    alpha.resize(pixelCount);

    ox::video::ECOLOR_FORMAT format = (ox::video::ECOLOR_FORMAT)2;
    switch (TextureFormat)
    {
    case 0:
        for (int i = 0; i < pixelCount; ++i)
            alpha[i] = 0xff;
        Driver->setTextureCreationFlag((ox::video::E_TEXTURE_CREATION_FLAG)8, true);
        break;
    case 2:
        for (int i = 0; i < pixelCount; ++i)
        {
            red[i] = 0xff;
            green[i] = 0xff;
            blue[i] = 0xff;
        }
    // fall through
    case 1:
    case 3:
        Driver->setTextureCreationFlag((ox::video::E_TEXTURE_CREATION_FLAG)4, true);
        // The channel shifts of an A8R8G8B8 layout; see CColorConverter::convert32BitTo32Bit.
        format = (ox::video::ECOLOR_FORMAT)0x08101800;
        break;
    }

    CImage* image = new CImage(format, ox::core::CDimension2d<int>(TextureSize, TextureSize));
    if (TextureFormat == 0 || TextureFormat == 1)
    {
        file->read(&red[0], pixelCount);
        file->read(&green[0], pixelCount);
        file->read(&blue[0], pixelCount);
    }
    if (TextureFormat == 1 || TextureFormat == 2)
        file->read(&alpha[0], pixelCount);
    if (TextureFormat == 3)
    {
        for (int i = 0; i < pixelCount; ++i)
        {
            unsigned char value = 0;
            file->read(&value, 1);
            if (value & 0x80)
            {
                value <<= 1;
                alpha[i] = value;
                red[i] = 0xff;
                green[i] = 0xff;
                blue[i] = 0xff;
            }
            else
            {
                alpha[i] = 0;
                red[i] = 0;
                green[i] = 0;
                blue[i] = 0;
            }
        }
    }

    int* pixels = (int*)image->lock();
    for (int y = 0; y < TextureSize; ++y)
    {
        int row = y * TextureSize;
        for (int x = 0; x < TextureSize; ++x)
            pixels[row + x] = alpha[row + x] << 24 | red[row + x] << 16 | green[row + x] << 8 | blue[row + x];
    }
    image->unlock();

    ox::video::ITexture* texture = Driver->addTexture(TextureNames[index].c_str(), image);
    if (texture)
        TextureLoaded[index] = 1;
    image->drop();
    file->drop();
    return texture;
}

ox::video::ITexture* CSpritePackage::getTexture(const ox::core::CString<char>& name)
{
    return Driver->getTexture(name.c_str());
}

void CSpritePackage::readSprite(SSpriteImage& image, ox::io::IReadFile* file, int version)
{
    readSpriteHeader(image.Header, file, version);
    image.TextureIndex = ox::io::CHelpIO::readInt(file);
    image.X = ox::io::CHelpIO::readInt(file);
    image.Y = ox::io::CHelpIO::readInt(file);
    image.Width = ox::io::CHelpIO::readInt(file);
    image.Height = ox::io::CHelpIO::readInt(file);
    if (version < 3)
    {
        image.OriginalWidth = image.Width;
        image.OriginalHeight = image.Height;
    }
    else
    {
        image.OriginalWidth = ox::io::CHelpIO::readInt(file);
        image.OriginalHeight = ox::io::CHelpIO::readInt(file);
    }
}

void CSpritePackage::readSpriteHeader(SSpriteImageHeader& header, ox::io::IReadFile* file, int version)
{
    // The name is only for the tools.
    char c = 0;
    do
    {
        file->read(&c, 1);
    } while (c);

    header.Id = ox::io::CHelpIO::readInt(file);
    header.OffsetX = ox::io::CHelpIO::readInt(file);
    header.OffsetY = ox::io::CHelpIO::readInt(file);
    header.OriginalOffsetX = header.OffsetX;
    header.OriginalOffsetY = header.OffsetY;
    if (version >= 2)
    {
        header.OffsetX += ox::io::CHelpIO::readInt(file);
        header.OffsetY += ox::io::CHelpIO::readInt(file);
    }
}

} // end namespace video
} // end namespace daisy
