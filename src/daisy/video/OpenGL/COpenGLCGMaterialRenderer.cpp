// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "COpenGLCGMaterialRenderer.h"
#include "ox/video/IShaderConstantSetCallBack.h"
// The native object includes an iostream static initializer.
#include <iostream> // IWYU pragma: keep

namespace daisy {
namespace video {

COpenGLCGMaterialRenderer::COpenGLCGMaterialRenderer(CGcontext context, ox::video::IMaterialRendererServices* services,
    ox::video::IVideoDriver* driver, int& outMaterialTypeNr, const char* vertexShaderProgram,
    const char* vertexShaderEntryPointName, const char* pixelShaderProgram, const char* pixelShaderEntryPointName,
    ox::video::IShaderConstantSetCallBack* callback, ox::video::IMaterialRenderer* baseMaterial, bool precompiled,
    int userData)
    : CCGMaterialRenderer(driver, services, callback, baseMaterial, userData)
{
    outMaterialTypeNr = init(context, vertexShaderProgram, vertexShaderEntryPointName, pixelShaderProgram,
        pixelShaderEntryPointName, precompiled);
}

COpenGLCGMaterialRenderer::~COpenGLCGMaterialRenderer()
{
}

//! On a material change the callback sets the constants (with user data 0) before the programs are
//! bound; the base material then sets its render states as if everything had to be reset.
void COpenGLCGMaterialRenderer::OnSetMaterial(ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
    bool resetAllRenderstates, ox::video::IMaterialRendererServices* services)
{
    if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
    {
        if (CallBack)
            CallBack->OnSetConstants(this, 0);

        if (VertexProgram)
        {
            cgGLBindProgram(VertexProgram);
            cgGLEnableProfile(cgGetProgramProfile(VertexProgram));
            checkForError("Error binding CG vertex program");
        }

        if (PixelProgram)
        {
            cgGLBindProgram(PixelProgram);
            cgGLEnableProfile(cgGetProgramProfile(PixelProgram));
            checkForError("Error binding CG pixel program");
        }

        if (BaseMaterial)
            BaseMaterial->OnSetMaterial(material, material, true, this);
    }

    services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
}

void COpenGLCGMaterialRenderer::OnUnsetMaterial()
{
    if (VertexProgram)
    {
        cgGLUnbindProgram(cgGetProgramProfile(VertexProgram));
        cgGLDisableProfile(cgGetProgramProfile(VertexProgram));
        checkForError("Error unbinding CG vertex program");
    }

    if (PixelProgram)
    {
        cgGLUnbindProgram(cgGetProgramProfile(PixelProgram));
        cgGLDisableProfile(cgGetProgramProfile(PixelProgram));
        checkForError("Error unbinding CG pixel program");
    }

    if (BaseMaterial)
        BaseMaterial->OnUnsetMaterial();
}

bool COpenGLCGMaterialRenderer::isTransparent()
{
    return BaseMaterial ? BaseMaterial->isTransparent() : false;
}

bool COpenGLCGMaterialRenderer::createCGVertexShader(const char* program, const char* entryPoint, CGcontext context,
    bool precompiled)
{
    CGprofile profile = cgGLGetLatestProfile(CG_GL_VERTEX);
    cgGLSetOptimalOptions(profile);
    VertexProgram = cgCreateProgram(context, precompiled ? CG_OBJECT : CG_SOURCE, program, profile, entryPoint, 0);
    if (checkForError("Error creating CG vertex program"))
        return false;

    cgGLLoadProgram(VertexProgram);
    return !checkForError("Error loading CG vertex program");
}

bool COpenGLCGMaterialRenderer::createCGPixelShader(const char* program, const char* entryPoint, CGcontext context,
    bool precompiled)
{
    CGprofile profile = cgGLGetLatestProfile(CG_GL_FRAGMENT);
    cgGLSetOptimalOptions(profile);
    PixelProgram = cgCreateProgram(context, precompiled ? CG_OBJECT : CG_SOURCE, program, profile, entryPoint, 0);
    if (checkForError("Error creating CG pixel program"))
        return false;

    cgGLLoadProgram(PixelProgram);
    return !checkForError("Error loading CG pixel program");
}

} // end namespace video
} // end namespace daisy
