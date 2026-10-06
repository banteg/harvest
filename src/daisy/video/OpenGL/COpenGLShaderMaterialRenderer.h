// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/COpenGLShaderMaterialRenderer.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source. Not used by the
// game. Member offsets are the Linux amd64 ones (sizeof 0x40).

#ifndef DAISY_VIDEO_OPENGL_COPENGLSHADERMATERIALRENDERER_H
#define DAISY_VIDEO_OPENGL_COPENGLSHADERMATERIALRENDERER_H

#include "CVideoOpenGL.h"
#include "ox/video/IMaterialRenderer.h"

namespace ox {
namespace video {
class IShaderConstantSetCallBack;
} // end namespace video
} // end namespace ox

namespace daisy {
namespace video {

//! Material with ARB vertex and fragment assembly programs.
class COpenGLShaderMaterialRenderer : public ox::video::IMaterialRenderer
{
public:
    //! outMaterialTypeNr receives the new material type, or -1.
    COpenGLShaderMaterialRenderer(CVideoOpenGL* driver, int& outMaterialTypeNr, const char* vertexShaderProgram,
        const char* pixelShaderProgram, ox::video::IShaderConstantSetCallBack* callback,
        ox::video::IMaterialRenderer* baseMaterial, int userData);

    virtual ~COpenGLShaderMaterialRenderer();

    virtual void OnSetMaterial(ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
        bool resetAllRenderstates, ox::video::IMaterialRendererServices* services);
    virtual void OnUnsetMaterial();
    virtual bool isTransparent();

protected:
    bool createPixelShader(const char* pxsh);
    bool createVertexShader(const char* vtxsh);

    // 0x18
    CVideoOpenGL* Driver;
    ox::video::IShaderConstantSetCallBack* CallBack;
    ox::video::IMaterialRenderer* BaseMaterial;
    GLuint VertexShader;
    GLuint PixelShader;
    int UserData;
};

} // end namespace video
} // end namespace daisy

#endif
