// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Irrlicht 1.2's GLSL material renderer adapted to the ox interfaces. Not used by the game. Member
// offsets are the Linux amd64 ones (sizeof 0x60).

#ifndef DAISY_VIDEO_OPENGL_COPENGLSLMATERIALRENDERER_H
#define DAISY_VIDEO_OPENGL_COPENGLSLMATERIALRENDERER_H

#include "CVideoOpenGL.h"
#include "ox/core/CString.h"
#include "ox/video/IGPUProgrammingServices.h"
#include "ox/video/IMaterialRenderer.h"
#include "ox/video/IMaterialRendererServices.h"
#include <vector>

namespace ox {
namespace video {
class IShaderConstantSetCallBack;
} // end namespace video
} // end namespace ox

namespace daisy {
namespace video {

//! Material drawn with a GLSL program object (ARB shader objects).
class COpenGLSLMaterialRenderer : public ox::video::IMaterialRenderer, public ox::video::IMaterialRendererServices
{
public:
    //! outMaterialTypeNr receives the new material type, or stays as it is when GLSL is not
    //! supported. The entry points and compile targets are ignored.
    COpenGLSLMaterialRenderer(CVideoOpenGL* driver, int& outMaterialTypeNr, const char* vertexShaderProgram,
        const char* vertexShaderEntryPointName, ox::video::E_VERTEX_SHADER_TYPE vsCompileTarget,
        const char* pixelShaderProgram, const char* pixelShaderEntryPointName,
        ox::video::E_PIXEL_SHADER_TYPE psCompileTarget, ox::video::IShaderConstantSetCallBack* callback,
        ox::video::IMaterialRenderer* baseMaterial, int userData);

    virtual ~COpenGLSLMaterialRenderer();

    virtual void OnSetMaterial(ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
        bool resetAllRenderstates, ox::video::IMaterialRendererServices* services);
    virtual bool OnRender(ox::video::IMaterialRendererServices* service, ox::video::E_VERTEX_TYPE vtxtype);
    virtual void OnUnsetMaterial();
    virtual bool isTransparent();

    virtual void setBasicRenderStates(const ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
        bool resetAllRenderstates);
    virtual bool setVertexShaderConstant(const char* name, const float* floats, int count);
    virtual void setVertexShaderConstant(const float* data, int startRegister, int constantAmount = 1);
    virtual bool setPixelShaderConstant(const char* name, const float* floats, int count);
    virtual void setPixelShaderConstant(const float* data, int startRegister, int constantAmount = 1);
    virtual ox::video::IVideoDriver* getVideoDriver();

protected:
    //! For derived renderers that call init themselves.
    COpenGLSLMaterialRenderer(CVideoOpenGL* driver, ox::video::IShaderConstantSetCallBack* callback,
        ox::video::IMaterialRenderer* baseMaterial, int userData);

    void init(int& outMaterialTypeNr, const char* vertexShaderProgram, const char* pixelShaderProgram);

    bool createProgram();
    bool createShader(GLenum shaderType, const char* shader);
    bool linkProgram();

    //! An active uniform of the linked program.
    struct SUniformInfo
    {
        ox::core::CString<char> name;
        GLenum type;
    };

    // 0x20
    CVideoOpenGL* Driver;
    ox::video::IShaderConstantSetCallBack* CallBack;
    ox::video::IMaterialRenderer* BaseMaterial;
    GLhandleARB Program;
    std::vector<SUniformInfo> UniformInfo;
    int UserData;
};

} // end namespace video
} // end namespace daisy

#endif
