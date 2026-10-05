// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

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

//! Which shader a CScatterShader was set up with; the names are ours.
enum EScatterShaderType
{
    ESST_NONE = 0,
    ESST_GROUND = 1,
    ESST_ATMOSPHERE = 2
};

//! The atmospheric scattering shader of a planet's ground or atmosphere.
class CScatterShader : public ox::video::IShaderConstantSetCallBack
{
public:
    CScatterShader(ox::video::IVideoDriver* driver, ox::scene::ICameraSceneNode* camera,
        ox::scene::IAnimatedMeshSceneNode* node);
    virtual ~CScatterShader();

    void initAtmo();
    //! Without scattering the ground uses the simpler scatterGroundSCG shaders.
    void initGround(bool scattering);

    virtual void OnSetConstants(ox::video::IMaterialRendererServices* services, int userData);

private:
    EScatterShaderType Type;
    ox::video::IVideoDriver* Driver;
    ox::scene::ICameraSceneNode* Camera;
    ox::scene::IAnimatedMeshSceneNode* Node;
    bool Scattering;
};

} // end namespace gfx
} // end namespace harvest

#endif
