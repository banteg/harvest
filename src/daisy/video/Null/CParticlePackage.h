// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The member names are inferred from their use; the file format reads them in declaration order
// except where readParticleInfo says otherwise.

#ifndef DAISY_VIDEO_CPARTICLEPACKAGE_H
#define DAISY_VIDEO_CPARTICLEPACKAGE_H

#include "ox/TArray.h"
#include "ox/core/CString.h"
#include "ox/core/CVector3d.h"
#include "ox/video/IParticlePackage.h"
#include "ox/video/SColor.h"

namespace ox {
namespace video {
class IVideoDriver;
} // end namespace video
} // end namespace ox

namespace daisy {
namespace video {

class CParticleState;

//! Waveforms of a particle function, evaluated at time * Frequency + Phase.
enum E_PARTICLE_FUNCTION_TYPE
{
    EPFT_CONSTANT = 0,
    EPFT_LINEAR,
    EPFT_SINE,
    EPFT_TRIANGLE,
    EPFT_SQUARE,
    //! A pulse of 1 / frameDelta whenever the integer part of the argument changes.
    EPFT_STEP,
    EPFT_EXP,
    EPFT_INVERSE_EXP
};

//! How a particle function's waveform is clamped before scaling by the amplitude.
enum E_PARTICLE_FUNCTION_CLAMP
{
    EPFC_NONE = 0,
    EPFC_ABSOLUTE,
    EPFC_POSITIVE
};

//! A waveform that drives one speed component of a particle over its life.
struct SParticleFunction
{
    float Amplitude;
    //! Per millisecond in the file, per second here.
    float Frequency;
    float Phase;
    int Type;
    int Clamp;
};

//! A particle type read from a particle package.
struct SParticleTypeInfo
{
    bool operator<(const SParticleTypeInfo& other) const
    {
        return Name < other.Name;
    }

    ox::core::CString<char> Name;
    //! The particle is only created when this is at most the package's importance level.
    int Importance;
    //! One of them is picked at random.
    ox::TArray<ox::core::CString<char> > AnimationNames;
    //! One of them is played when the particle starts.
    ox::TArray<ox::core::CString<char> > SoundNames;
    bool GroundSprite;
    bool CastsShadow;
    //! Moves along the unit start direction instead of the start speed vector.
    bool NormalizeSpeed;
    //! Accelerates along the speed direction.
    SParticleFunction SpeedFunction;
    SParticleFunction MoveXFunction;
    SParticleFunction MoveYFunction;
    SParticleFunction MoveZFunction;
    //! Speed factor after a bounce.
    float Bounciness;
    float WindModifier;
    //! The particle dies on the bounce after this many; negative bounces forever.
    int MaxBounces;
    //! Milliseconds; 0 lives until the animation or the pulses end.
    int MinLifeTime;
    int MaxLifeTime;
    bool DieWithAnimation;
    //! Pulses start with the second pulse period.
    bool SkipFirstPulse;
    //! Draws mirrored while moving left instead of rotated.
    bool MirrorByDirection;
    //! Turns towards the screen movement.
    bool RotateToDirection;
    ox::video::SColor StartColor;
    ox::video::SColor EndColor;
    float StartScale;
    float EndScale;
    //! Particles spawned when the particle dies. A name starting with '@' starts the group used for
    //! the callback's marker at the death position.
    ox::TArray<ox::core::CString<char> > OnDieParticles;
    //! Degrees per second.
    float RotationSpeed;
    //! Particles per pulse; the first one is spawned at the pulse start.
    int MinPulseParticles;
    int MaxPulseParticles;
    //! Milliseconds a pulse spreads its particles over.
    int MinPulseDuration;
    int MaxPulseDuration;
    //! Milliseconds between pulses.
    int MinPulseDelay;
    int MaxPulseDelay;
    //! The particle dies after this many pulses; 0 pulses forever.
    int MaxPulses;
    //! Offset range of the pulse particles.
    ox::core::CVector3d<float> MinPulseOffset;
    ox::core::CVector3d<float> MaxPulseOffset;
    //! Start direction in degrees around the vertical axis.
    int MinDirection;
    //! Start elevation in degrees.
    int MinElevation;
    int MinSpeed;
    int MaxDirection;
    int MaxElevation;
    //! The start speed is only set when this is positive.
    int MaxSpeed;
    //! Share of the parent speed that spawned particles inherit.
    float ParentSpeedModifier;
    ox::TArray<ox::core::CString<char> > PulseParticles;
};

//! Compares a particle type name with a particle type for ox::algo::binarySearchIf.
template <class T>
struct SParticleTypeInfoSearcher
{
    int operator()(const ox::core::CString<char>& name, const T& info)
    {
        if (name < info->Name)
            return -1;
        if (info->Name < name)
            return 1;
        return 0;
    }
};

//! Particle types loaded from a particle package file, sorted by name.
class CParticlePackage : public ox::video::IParticlePackage
{
public:
    CParticlePackage(ox::video::IVideoDriver* driver);
    virtual ~CParticlePackage();

    virtual bool load(ox::io::IReadFile* file);
    virtual const ox::TArray<ox::core::CString<char> >& getParticleNames();
    //! The sound names of all particle types, collected on the first call.
    virtual const ox::TArray<ox::core::CString<char> >& getParticleSounds();
    virtual void setSpritePackage(ox::video::ISpritePackage* package);
    virtual ox::video::IParticleState* addNewParticleState(const ox::core::CString<char>& name);
    virtual void removeParticleState(ox::video::IParticleState* state);
    virtual bool spriteFulfillsImportance(const ox::core::CString<char>& name);

private:
    void readParticleInfo(SParticleTypeInfo* info, ox::io::IReadFile* file, int version);
    //! Appends the non-empty '/'-separated parts of text.
    void splitNames(ox::TArray<ox::core::CString<char> >& names, ox::core::CString<char> text);
    void readParticleFunction(SParticleFunction* function, ox::io::IReadFile* file, int version);

    ox::video::IVideoDriver* Driver;
    ox::video::ISpritePackage* SpritePackage;
    //! Nothing recovered sets it.
    ox::core::CString<char> Name;
    ox::TArray<ox::core::CString<char> > ParticleNames;
    ox::TArray<ox::core::CString<char> > ParticleSounds;
    ox::TArray<SParticleTypeInfo*> TypeInfos;
    //! Nothing recovered adds to it; removeParticleState drops what it finds.
    ox::TArray<CParticleState*> States;
};

} // end namespace video
} // end namespace daisy

#endif
