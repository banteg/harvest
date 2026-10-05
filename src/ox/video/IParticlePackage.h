// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: the virtual interface follows the Mac vtable (names from daisy::video::CParticlePackage);
// return types that no recovered code uses are not verified, and the members are not recovered yet.

#ifndef OX_VIDEO_IPARTICLEPACKAGE_H
#define OX_VIDEO_IPARTICLEPACKAGE_H

#include "../IUnknown.h"
#include "../TArray.h"
#include "../core/CString.h"

namespace ox {
namespace io {
class IReadFile;
} // end namespace io

namespace video {

class IParticleEngineCallback;
class IParticleState;
class ISpritePackage;

//! A set of particle definitions that creates particle states by name.
class IParticlePackage : public IUnknown
{
public:
    virtual bool load(io::IReadFile* file) = 0;
    virtual const TArray<core::CString<char> >& getParticleNames() = 0;
    virtual const TArray<core::CString<char> >& getParticleSounds() = 0;
    virtual void setSpritePackage(ISpritePackage* package) = 0;
    virtual IParticleState* addNewParticleState(const core::CString<char>& name) = 0;
    virtual void removeParticleState(IParticleState* state) = 0;
    virtual void setCallbackEngine(IParticleEngineCallback* callback);
    virtual IParticleEngineCallback* getCallbackEngine();
    virtual bool spriteFulfillsImportance(const core::CString<char>& name) = 0;

    IParticleEngineCallback* CallbackEngine;
    //! Particles less important than this are not created; game states set it directly.
    int ImportanceLevel;
};

} // end namespace video
} // end namespace ox

#endif
