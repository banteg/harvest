// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CParticlePackage.h"
#include "CParticleState.h"
#include "ox/algo/CArrayFunctions.h"
#include "ox/algo/SPointerSortFunctor.h"
#include "ox/io/CHelpIO.h"
#include <algorithm>
// The native object includes an iostream static initializer.
#include <iostream> // IWYU pragma: keep

namespace daisy {
namespace video {

//! The newest particle package format.
const int PARTICLE_PACKAGE_VERSION = 8;

CParticlePackage::CParticlePackage(ox::video::IVideoDriver* driver)
    : Driver(driver), SpritePackage(0)
{
}

CParticlePackage::~CParticlePackage()
{
    for (int i = 0; i < (int)States.size(); ++i)
        States[i]->drop();
    States.clear();

    for (int i = 0; i < (int)TypeInfos.size(); ++i)
        delete TypeInfos[i];
    TypeInfos.clear();
}

ox::video::IParticleState* CParticlePackage::addNewParticleState(const ox::core::CString<char>& name)
{
    if (!SpritePackage)
        return 0;

    int index = ox::algo::binarySearchIf(TypeInfos, SParticleTypeInfoSearcher<SParticleTypeInfo*>(), name);
    if (index < 0)
        return 0;

    return new CParticleState(this, SpritePackage, TypeInfos[index]);
}

bool CParticlePackage::spriteFulfillsImportance(const ox::core::CString<char>& name)
{
    int index = ox::algo::binarySearchIf(TypeInfos, SParticleTypeInfoSearcher<SParticleTypeInfo*>(), name);
    bool result = false;
    if (index >= 0)
        result = TypeInfos[index]->Importance <= ImportanceLevel;
    return result;
}

void CParticlePackage::setSpritePackage(ox::video::ISpritePackage* package)
{
    SpritePackage = package;
}

void CParticlePackage::removeParticleState(ox::video::IParticleState* state)
{
    int index = ox::algo::linearSearchPos(States.begin(), States.end(), state);
    if (index >= 0)
        States.erase(States.begin() + index);
    state->drop();
}

void CParticlePackage::readParticleInfo(SParticleTypeInfo* info, ox::io::IReadFile* file, int version)
{
    ox::core::CString<char> animations;
    ox::io::CHelpIO::readString(file, animations);
    splitNames(info->AnimationNames, animations);

    if (version >= 6)
    {
        ox::core::CString<char> sounds;
        ox::io::CHelpIO::readString(file, sounds);
        splitNames(info->SoundNames, sounds);
    }

    readParticleFunction(&info->SpeedFunction, file, version);
    readParticleFunction(&info->MoveXFunction, file, version);
    readParticleFunction(&info->MoveYFunction, file, version);
    readParticleFunction(&info->MoveZFunction, file, version);

    if (version < 4)
        info->GroundSprite = false;
    else
        info->GroundSprite = ox::io::CHelpIO::readInt(file) != 0;

    if (version < 5)
    {
        info->CastsShadow = false;
        info->NormalizeSpeed = true;
    }
    else
    {
        info->CastsShadow = ox::io::CHelpIO::readInt(file) != 0;
        info->NormalizeSpeed = ox::io::CHelpIO::readInt(file) != 0;
    }

    if (version >= 2)
    {
        info->MinDirection = ox::io::CHelpIO::readInt(file);
        info->MinElevation = ox::io::CHelpIO::readInt(file);
        info->MinSpeed = ox::io::CHelpIO::readInt(file);
        info->MaxDirection = ox::io::CHelpIO::readInt(file);
        info->MaxElevation = ox::io::CHelpIO::readInt(file);
        info->MaxSpeed = ox::io::CHelpIO::readInt(file);
        info->ParentSpeedModifier = ox::io::CHelpIO::readFloat(file);
    }

    info->Bounciness = ox::io::CHelpIO::readFloat(file);
    info->MaxBounces = ox::io::CHelpIO::readInt(file);

    if (version < 8)
        info->WindModifier = 1.0f;
    else
        info->WindModifier = ox::io::CHelpIO::readFloat(file);

    info->MinLifeTime = ox::io::CHelpIO::readInt(file);
    if (version < 3)
    {
        info->MaxLifeTime = info->MinLifeTime;
        info->SkipFirstPulse = false;
        info->MirrorByDirection = false;
        info->RotateToDirection = false;
    }
    else
    {
        info->MaxLifeTime = ox::io::CHelpIO::readInt(file);
        info->SkipFirstPulse = ox::io::CHelpIO::readInt(file) != 0;
        info->MirrorByDirection = ox::io::CHelpIO::readInt(file) != 0;
        info->RotateToDirection = ox::io::CHelpIO::readInt(file) != 0;
    }
    info->DieWithAnimation = ox::io::CHelpIO::readInt(file) != 0;

    int r = ox::io::CHelpIO::readInt(file);
    int g = ox::io::CHelpIO::readInt(file);
    int b = ox::io::CHelpIO::readInt(file);
    info->StartColor = ox::video::SColor(ox::io::CHelpIO::readInt(file), r, g, b);
    r = ox::io::CHelpIO::readInt(file);
    g = ox::io::CHelpIO::readInt(file);
    b = ox::io::CHelpIO::readInt(file);
    info->EndColor = ox::video::SColor(ox::io::CHelpIO::readInt(file), r, g, b);
    info->StartScale = ox::io::CHelpIO::readFloat(file);
    info->EndScale = ox::io::CHelpIO::readFloat(file);

    if (version >= 4)
    {
        int count = ox::io::CHelpIO::readInt(file);
        for (int i = 0; i < count; ++i)
        {
            ox::core::CString<char> name;
            ox::io::CHelpIO::readString(file, name);
            info->OnDieParticles.push_back(name);
        }
    }
    else
    {
        ox::core::CString<char> name;
        ox::io::CHelpIO::readString(file, name);
        if (name.size() > 0)
            info->OnDieParticles.push_back(name);
    }

    info->RotationSpeed = (float)ox::io::CHelpIO::readInt(file);
    info->MinPulseParticles = ox::io::CHelpIO::readInt(file);
    info->MaxPulseParticles = ox::io::CHelpIO::readInt(file);
    info->MinPulseDuration = ox::io::CHelpIO::readInt(file);
    info->MaxPulseDuration = ox::io::CHelpIO::readInt(file);
    info->MinPulseDelay = ox::io::CHelpIO::readInt(file);
    info->MaxPulseDelay = ox::io::CHelpIO::readInt(file);
    info->MaxPulses = ox::io::CHelpIO::readInt(file);

    int x = ox::io::CHelpIO::readInt(file);
    int y = ox::io::CHelpIO::readInt(file);
    int z = ox::io::CHelpIO::readInt(file);
    info->MinPulseOffset = ox::core::CVector3d<float>((float)x, (float)y, (float)z);
    x = ox::io::CHelpIO::readInt(file);
    y = ox::io::CHelpIO::readInt(file);
    z = ox::io::CHelpIO::readInt(file);
    info->MaxPulseOffset = ox::core::CVector3d<float>((float)x, (float)y, (float)z);

    int count = ox::io::CHelpIO::readInt(file);
    for (int i = 0; i < count; ++i)
    {
        ox::core::CString<char> name;
        ox::io::CHelpIO::readString(file, name);
        info->PulseParticles.push_back(name);
    }

    if (version < 7)
        info->Importance = 0;
    else
        info->Importance = ox::io::CHelpIO::readInt(file);
}

void CParticlePackage::splitNames(ox::TArray<ox::core::CString<char> >& names, ox::core::CString<char> text)
{
    int position = text.findFirst('/');
    if (position > 0)
    {
        int start = 0;
        while (position > 0)
        {
            ox::core::CString<char> name = text.subString(start, position - start);
            if (name.size() > 0)
                names.push_back(name);

            start = position + 1;
            position = text.findNext('/', start);
            if (position == -1)
            {
                name = text.subString(start, text.size() - start);
                if (name.size() > 0)
                    names.push_back(name);
            }
        }
    }
    else if (text.size() > 0)
        names.push_back(text);
}

void CParticlePackage::readParticleFunction(SParticleFunction* function, ox::io::IReadFile* file, int version)
{
    if (version >= 3)
    {
        function->Amplitude = ox::io::CHelpIO::readFloat(file);
        function->Frequency = ox::io::CHelpIO::readFloat(file) * 0.001f;
        function->Phase = ox::io::CHelpIO::readFloat(file) * 0.001f;
        function->Type = ox::io::CHelpIO::readInt(file);
        function->Clamp = ox::io::CHelpIO::readInt(file);
    }
    else
    {
        function->Amplitude = (float)ox::io::CHelpIO::readInt(file);
        function->Frequency = 1.0f;
        function->Phase = 0.0f;
        function->Type = EPFT_CONSTANT;
        function->Clamp = EPFC_NONE;
    }
}

bool CParticlePackage::load(ox::io::IReadFile* file)
{
    int version = ox::io::CHelpIO::readInt(file);
    if (version < 0 || version > PARTICLE_PACKAGE_VERSION)
        return false;

    int count = ox::io::CHelpIO::readInt(file);
    for (int i = 0; i < count; ++i)
    {
        ox::core::CString<char> name;
        ox::io::CHelpIO::readString(file, name);
        SParticleTypeInfo* info = new SParticleTypeInfo;
        info->Name = name;
        readParticleInfo(info, file, version);
        TypeInfos.push_back(info);
        ParticleNames.push_back(name);
    }

    std::sort(TypeInfos.begin(), TypeInfos.end(), ox::algo::SPointerSortFunctor<SParticleTypeInfo*>());
    return true;
}

const ox::TArray<ox::core::CString<char> >& CParticlePackage::getParticleNames()
{
    return ParticleNames;
}

const ox::TArray<ox::core::CString<char> >& CParticlePackage::getParticleSounds()
{
    if (ParticleSounds.empty())
    {
        for (unsigned int i = 0; i < TypeInfos.size(); ++i)
        {
            int count = (int)TypeInfos[i]->SoundNames.size();
            for (int j = 0; j < count; ++j)
                ParticleSounds.push_back(TypeInfos[i]->SoundNames[j]);
        }
    }
    return ParticleSounds;
}

} // end namespace video
} // end namespace daisy
