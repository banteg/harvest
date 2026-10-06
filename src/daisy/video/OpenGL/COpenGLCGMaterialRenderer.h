// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef DAISY_VIDEO_OPENGL_COPENGLCGMATERIALRENDERER_H
#define DAISY_VIDEO_OPENGL_COPENGLCGMATERIALRENDERER_H

#include "daisy/video/Null/CCGMaterialRenderer.h"

namespace daisy {
namespace video {

//! Cg material for OpenGL: binds the programs with the latest GL profiles and draws with the render
//! states of its base material. The main menu's atmospheric scattering shaders use it, with
//! EMT_TRANSPARENT_ADD_COLOR (atmosphere) and EMT_SOLID_2_LAYER (ground) as base materials.
class COpenGLCGMaterialRenderer : public CCGMaterialRenderer
{
public:
    //! Compiles both programs; outMaterialTypeNr receives the new material type, or -1.
    COpenGLCGMaterialRenderer(CGcontext context, ox::video::IMaterialRendererServices* services,
        ox::video::IVideoDriver* driver, int& outMaterialTypeNr, const char* vertexShaderProgram,
        const char* vertexShaderEntryPointName, const char* pixelShaderProgram, const char* pixelShaderEntryPointName,
        ox::video::IShaderConstantSetCallBack* callback, ox::video::IMaterialRenderer* baseMaterial, bool precompiled,
        int userData);

    virtual ~COpenGLCGMaterialRenderer();

    virtual void OnSetMaterial(ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
        bool resetAllRenderstates, ox::video::IMaterialRendererServices* services);
    virtual void OnUnsetMaterial();
    virtual bool isTransparent();

    virtual bool createCGVertexShader(const char* program, const char* entryPoint, CGcontext context, bool precompiled);
    virtual bool createCGPixelShader(const char* program, const char* entryPoint, CGcontext context, bool precompiled);
};

} // end namespace video
} // end namespace daisy

#endif
