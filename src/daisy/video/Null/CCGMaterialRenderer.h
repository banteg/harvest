// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The driver-independent half of the Cg material renderers (daisy::video::COpenGLCGMaterialRenderer
// is the OpenGL half). Member offsets are the Linux amd64 ones (sizeof 0x60).

#ifndef DAISY_VIDEO_NULL_CCGMATERIALRENDERER_H
#define DAISY_VIDEO_NULL_CCGMATERIALRENDERER_H

#include "daisy/video/CgApi.h"
#include "ox/video/IMaterialRenderer.h"
#include "ox/video/IMaterialRendererServices.h"

namespace ox {
namespace video {
class IShaderConstantSetCallBack;
class IVideoDriver;
} // end namespace video
} // end namespace ox

namespace daisy {
namespace video {

//! A material drawn with a Cg vertex and pixel program. The programs read their uniforms by name
//! (setVertexShaderConstant/setPixelShaderConstant with a name); the callback sets them in
//! OnSetConstants, and the render states come from a base material renderer.
class CCGMaterialRenderer : public ox::video::IMaterialRenderer, public ox::video::IMaterialRendererServices
{
public:
    CCGMaterialRenderer(ox::video::IVideoDriver* driver, ox::video::IMaterialRendererServices* services,
        ox::video::IShaderConstantSetCallBack* callback, ox::video::IMaterialRenderer* baseMaterial, int userData);

    //! Destroys the programs and drops the base material; the callback grabbed by the constructor
    //! is never dropped.
    virtual ~CCGMaterialRenderer();

    //! Calls the callback's OnSetConstants with the user data before each draw call.
    virtual bool OnRender(ox::video::IMaterialRendererServices* service, ox::video::E_VERTEX_TYPE vtxtype);

    //! Compiles (or loads, when precompiled) and loads the program for the latest profile.
    virtual bool createCGVertexShader(const char* program, const char* entryPoint, CGcontext context,
        bool precompiled) = 0;
    virtual bool createCGPixelShader(const char* program, const char* entryPoint, CGcontext context,
        bool precompiled) = 0;

    virtual void setBasicRenderStates(const ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
        bool resetAllRenderstates);
    virtual bool setVertexShaderConstant(const char* name, const float* floats, int count);
    virtual void setVertexShaderConstant(const float* data, int startRegister, int constantAmount = 1);
    virtual bool setPixelShaderConstant(const char* name, const float* floats, int count);
    virtual void setPixelShaderConstant(const float* data, int startRegister, int constantAmount = 1);
    virtual ox::video::IVideoDriver* getVideoDriver();

protected:
    //! Creates both programs and registers the renderer with the driver; returns the new material
    //! type, or -1.
    int init(CGcontext context, const char* vertexProgram, const char* vertexEntryPoint, const char* pixelProgram,
        const char* pixelEntryPoint, bool precompiled);

    //! Sets a uniform of the vertex or pixel program from count floats. The first unknown name
    //! logs the program's parameters.
    bool setVariable(bool vertex, const char* name, const float* floats, int count);
    void printVariables();
    //! Logs every pending Cg error after message; true if there was one.
    bool checkForError(const char* message);

    // 0x20
    ox::video::IVideoDriver* Driver;
    ox::video::IMaterialRendererServices* BaseServices;
    ox::video::IShaderConstantSetCallBack* CallBack;
    ox::video::IMaterialRenderer* BaseMaterial;
    int UserData;
    // 0x48
    CGprogram PixelProgram;
    CGprogram VertexProgram;
    //! Only the first missing variable is reported.
    bool FirstVariableError;
};

} // end namespace video
} // end namespace daisy

#endif
