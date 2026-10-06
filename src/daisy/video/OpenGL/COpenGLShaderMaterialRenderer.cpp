// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/COpenGLShaderMaterialRenderer.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "COpenGLShaderMaterialRenderer.h"
#include "daisy/os.h"
#include "ox/video/IShaderConstantSetCallBack.h"
#include <stdio.h>
#include <string.h>
// The native object includes an iostream static initializer.
#include <iostream> // IWYU pragma: keep

namespace daisy {
namespace video {

COpenGLShaderMaterialRenderer::COpenGLShaderMaterialRenderer(CVideoOpenGL* driver, int& outMaterialTypeNr,
    const char* vertexShaderProgram, const char* pixelShaderProgram, ox::video::IShaderConstantSetCallBack* callback,
    ox::video::IMaterialRenderer* baseMaterial, int userData)
    : Driver(driver), CallBack(callback), BaseMaterial(baseMaterial), VertexShader(0), PixelShader(0),
      UserData(userData)
{
    if (BaseMaterial)
        BaseMaterial->grab();

    if (CallBack)
        CallBack->grab();

    outMaterialTypeNr = -1;

    // create vertex shader
    if (!createVertexShader(vertexShaderProgram))
        return;

    // create pixel shader
    if (!createPixelShader(pixelShaderProgram))
        return;

    // register as a new material
    outMaterialTypeNr = driver->addMaterialRenderer(this, 0);
}

COpenGLShaderMaterialRenderer::~COpenGLShaderMaterialRenderer()
{
    if (CallBack)
        CallBack->drop();

    if (VertexShader)
        Driver->extGlDeleteProgramsARB(1, &VertexShader);

    if (PixelShader)
        Driver->extGlDeleteProgramsARB(1, &PixelShader);

    if (BaseMaterial)
        BaseMaterial->drop();
}

//! Original bug (from Irrlicht): the fragment program target is bound to the vertex program's
//! name instead of PixelShader.
void COpenGLShaderMaterialRenderer::OnSetMaterial(ox::video::SMaterial& material,
    const ox::video::SMaterial& lastMaterial, bool resetAllRenderstates, ox::video::IMaterialRendererServices* services)
{
    if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
    {
        if (CallBack)
            CallBack->OnSetConstants(services, UserData);

        if (VertexShader)
        {
            // set new vertex shader
            Driver->extGlBindProgramARB(GL_VERTEX_PROGRAM_ARB, VertexShader);
            glEnable(GL_VERTEX_PROGRAM_ARB);
        }

        // set new pixel shader
        if (PixelShader)
        {
            Driver->extGlBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, VertexShader);
            glEnable(GL_FRAGMENT_PROGRAM_ARB);
        }

        if (BaseMaterial)
            BaseMaterial->OnSetMaterial(material, material, true, services);
    }

    services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
}

void COpenGLShaderMaterialRenderer::OnUnsetMaterial()
{
    // disable vertex shader
    if (VertexShader)
        glDisable(GL_VERTEX_PROGRAM_ARB);

    if (PixelShader)
        glDisable(GL_FRAGMENT_PROGRAM_ARB);

    if (BaseMaterial)
        BaseMaterial->OnUnsetMaterial();
}

bool COpenGLShaderMaterialRenderer::isTransparent()
{
    return BaseMaterial ? BaseMaterial->isTransparent() : false;
}

bool COpenGLShaderMaterialRenderer::createPixelShader(const char* pxsh)
{
    if (!pxsh)
        return true;

    Driver->extGlGenProgramsARB(1, &PixelShader);
    Driver->extGlBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, PixelShader);

    // clear error buffer
    while (glGetError() != GL_NO_ERROR)
    {
    }

    // compile
    Driver->extGlProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(pxsh), pxsh);

    if (glGetError() != GL_NO_ERROR)
    {
        GLint errPos;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);

        char tmp[2048];
        sprintf(tmp, "Pixel shader compilation failed at position %d:\n%s", errPos,
            glGetString(GL_PROGRAM_ERROR_STRING_ARB));
        os::Printer::log(tmp, ox::event::ELL_INFORMATION);

        return false;
    }

    return true;
}

bool COpenGLShaderMaterialRenderer::createVertexShader(const char* vtxsh)
{
    if (!vtxsh)
        return true;

    Driver->extGlGenProgramsARB(1, &VertexShader);
    Driver->extGlBindProgramARB(GL_VERTEX_PROGRAM_ARB, VertexShader);

    // clear error buffer
    while (glGetError() != GL_NO_ERROR)
    {
    }

    // compile
    Driver->extGlProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(vtxsh), vtxsh);

    if (glGetError() != GL_NO_ERROR)
    {
        GLint errPos;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);

        char tmp[2048];
        sprintf(tmp, "Vertex shader compilation failed at position %d:\n%s", errPos,
            glGetString(GL_PROGRAM_ERROR_STRING_ARB));
        os::Printer::log(tmp, ox::event::ELL_INFORMATION);

        return false;
    }

    return true;
}

} // end namespace video
} // end namespace daisy
