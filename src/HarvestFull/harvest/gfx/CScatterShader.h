// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef HARVEST_GFX_CSCATTERSHADER_H
#define HARVEST_GFX_CSCATTERSHADER_H

#include "ox/video/IShaderConstantSetCallBack.h"

namespace ox {
namespace scene {
class IAnimatedMeshSceneNode;
class ICameraSceneNode;
} // end namespace scene
namespace video { class IVideoDriver; }
} // end namespace ox

namespace harvest {
namespace gfx {

//! The atmospheric scattering shader of a planet's ground or atmosphere.
class CScatterShader : public ox::video::IShaderConstantSetCallBack
{
public:
    CScatterShader(ox::video::IVideoDriver* driver, ox::scene::ICameraSceneNode* camera,
        ox::scene::IAnimatedMeshSceneNode* node);
    virtual ~CScatterShader();

    void initAtmo();
    void initGround(bool highQuality);

    virtual void OnSetConstants(ox::video::IMaterialRendererServices* services, int userData);

private:
    // Not recovered yet; keeps the Linux object size of 72 bytes, the virtual IUnknown base included.
    char Unrecovered[72 - 8 - sizeof(ox::IUnknown)];
};

} // end namespace gfx
} // end namespace harvest

#endif
