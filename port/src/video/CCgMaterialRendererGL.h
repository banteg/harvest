// The Cg material renderer (the original's CCGMaterialRenderer and COpenGLCGMaterialRenderer,
// src/daisy/video/Null/CCGMaterialRenderer.* and src/daisy/video/OpenGL/COpenGLCGMaterialRenderer.*)
// over GLSL translations of the Cg programs. The behaviour the game sees is the original's:
// - on a change of material type the callback's OnSetConstants(services, 0) runs, the programs are
//   bound, and the base renderer's OnSetMaterial(material, material, true, this) sets its states;
// - every draw calls OnSetConstants(this, userData) first;
// - setVertexShaderConstant/setPixelShaderConstant(name, floats, count) set the uniform of that
//   name in the vertex or pixel program; the first unknown name logs "CG variable not found" and
//   both programs' parameters;
// - the callback is grabbed and never dropped, as in the original.
//
// The vertex and pixel programs keep separate uniform namespaces: the translations prefix vertex
// uniforms with vs_ and pixel uniforms with ps_. A name counts as found when the translation
// declares it, even if the GLSL compiler removed it as unused (Cg kept unreferenced parameters).

#ifndef PORT_VIDEO_CCGMATERIALRENDERERGL_H
#define PORT_VIDEO_CCGMATERIALRENDERERGL_H

#include "video/GL.h"
#include "ox/core/CString.h"
#include "ox/video/IMaterialRenderer.h"
#include "ox/video/IMaterialRendererServices.h"
#include <vector>

namespace ox {
namespace video {
class IShaderConstantSetCallBack;
} // end namespace video
} // end namespace ox

namespace port {
namespace video {

class CVideoGL;

class CCgMaterialRendererGL : public ox::video::IMaterialRenderer, public ox::video::IMaterialRendererServices
{
public:
    //! Links the program from the GLSL vertex and pixel sources and registers the renderer with the
    //! driver; outMaterialTypeNr receives the new material type, or -1.
    CCgMaterialRendererGL(CVideoGL* driver, int& outMaterialTypeNr, const char* vertexSource,
        const char* pixelSource, const char* name, ox::video::IShaderConstantSetCallBack* callback,
        ox::video::IMaterialRenderer* baseMaterial, int userData);
    virtual ~CCgMaterialRendererGL();

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

private:
    //! A uniform a translation declares.
    struct SVariable
    {
        //! The Cg name (without the vs_ or ps_ prefix).
        ox::core::CString<char> Name;
        //! The GLSL type: float, vec2, vec3, vec4, mat4 or sampler2D.
        ox::core::CString<char> Type;
        bool Vertex;
        gl::GLint Location;
    };

    //! Reads the uniform declarations of a translation.
    void addVariables(const char* source, bool vertex);
    bool setVariable(bool vertex, const char* name, const float* floats, int count);
    void printVariables();

    CVideoGL* Driver;
    ox::video::IShaderConstantSetCallBack* CallBack;
    ox::video::IMaterialRenderer* BaseMaterial;
    int UserData;
    gl::GLuint Program;
    std::vector<SVariable> Variables;
    //! Only the first missing variable is reported.
    bool FirstVariableError;
};

} // end namespace video
} // end namespace port

#endif
