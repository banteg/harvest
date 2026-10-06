// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CCGMaterialRenderer.h"
#include "daisy/os.h"
#include "ox/core/CString.h"
#include "ox/video/IShaderConstantSetCallBack.h"
#include "ox/video/IVideoDriver.h"
// The native object includes an iostream static initializer.
#include <iostream> // IWYU pragma: keep

namespace daisy {
namespace video {

CCGMaterialRenderer::CCGMaterialRenderer(ox::video::IVideoDriver* driver, ox::video::IMaterialRendererServices* services,
    ox::video::IShaderConstantSetCallBack* callback, ox::video::IMaterialRenderer* baseMaterial, int userData)
    : Driver(driver), BaseServices(services), CallBack(callback), BaseMaterial(baseMaterial), UserData(userData),
      PixelProgram(0), VertexProgram(0), FirstVariableError(true)
{
    if (BaseMaterial)
        BaseMaterial->grab();

    if (CallBack)
        CallBack->grab();
}

int CCGMaterialRenderer::init(CGcontext context, const char* vertexProgram, const char* vertexEntryPoint,
    const char* pixelProgram, const char* pixelEntryPoint, bool precompiled)
{
    if (createCGVertexShader(vertexProgram, vertexEntryPoint, context, precompiled) &&
        createCGPixelShader(pixelProgram, pixelEntryPoint, context, precompiled))
        return Driver->addMaterialRenderer(this, 0);

    return -1;
}

CCGMaterialRenderer::~CCGMaterialRenderer()
{
    if (VertexProgram)
        cgDestroyProgram(VertexProgram);

    if (PixelProgram)
        cgDestroyProgram(PixelProgram);

    if (BaseMaterial)
        BaseMaterial->drop();
}

bool CCGMaterialRenderer::setVariable(bool vertex, const char* name, const float* floats, int count)
{
    CGparameter parameter = cgGetNamedParameter(vertex ? VertexProgram : PixelProgram, name);
    if (parameter)
    {
        cgSetParameterValuefc(parameter, count, floats);
        return !checkForError("Error setting variable");
    }

    if (FirstVariableError)
    {
        FirstVariableError = false;

        ox::core::CString<char> message;
        message = "CG variable not found: '";
        message.append(ox::core::CString<char>(name));
        message.append(ox::core::CString<char>("'. Available variables are:"));
        os::Printer::log(message.c_str(), ox::event::ELL_WARNING);
        printVariables();
    }

    return false;
}

bool CCGMaterialRenderer::checkForError(const char* message)
{
    bool error = false;

    for (CGerror e = cgGetError(); e != CG_NO_ERROR; e = cgGetError())
    {
        ox::core::CString<char> text(message);
        text.append(ox::core::CString<char>(": '"));
        text.append(ox::core::CString<char>(cgGetErrorString(e)));
        text.append(ox::core::CString<char>("'"));
        os::Printer::log(text.c_str(), ox::event::ELL_WARNING);
        error = true;
    }

    return error;
}

void CCGMaterialRenderer::printVariables()
{
    os::Printer::log("Vertex program parameters:", ox::event::ELL_WARNING);

    ox::core::CString<char> line;
    for (CGparameter parameter = cgGetFirstParameter(VertexProgram, CG_PROGRAM); parameter;
         parameter = cgGetNextParameter(parameter))
    {
        line = "- ";
        line.append(ox::core::CString<char>(cgGetParameterName(parameter)));
        line.append(ox::core::CString<char>(" - size "));
        line.append(cgGetParameterRows(parameter));
        line.append(ox::core::CString<char>("x"));
        line.append(cgGetParameterColumns(parameter));
        os::Printer::log(line.c_str(), ox::event::ELL_WARNING);
    }

    os::Printer::log("Pixel program parameters:", ox::event::ELL_WARNING);

    for (CGparameter parameter = cgGetFirstParameter(PixelProgram, CG_PROGRAM); parameter;
         parameter = cgGetNextParameter(parameter))
    {
        line = "- ";
        line.append(ox::core::CString<char>(cgGetParameterName(parameter)));
        line.append(ox::core::CString<char>(" - size "));
        line.append(cgGetParameterRows(parameter));
        line.append(ox::core::CString<char>("x"));
        line.append(cgGetParameterColumns(parameter));
        os::Printer::log(line.c_str(), ox::event::ELL_WARNING);
    }
}

bool CCGMaterialRenderer::OnRender(ox::video::IMaterialRendererServices* service, ox::video::E_VERTEX_TYPE vtxtype)
{
    if (CallBack && (VertexProgram || PixelProgram))
        CallBack->OnSetConstants(this, UserData);

    return true;
}

void CCGMaterialRenderer::setBasicRenderStates(const ox::video::SMaterial& material,
    const ox::video::SMaterial& lastMaterial, bool resetAllRenderstates)
{
    BaseServices->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
}

bool CCGMaterialRenderer::setVertexShaderConstant(const char* name, const float* floats, int count)
{
    return setVariable(true, name, floats, count);
}

void CCGMaterialRenderer::setVertexShaderConstant(const float* data, int startRegister, int constantAmount)
{
    os::Printer::log("Cannot set constant, please use high level shader call instead.", ox::event::ELL_INFORMATION);
}

bool CCGMaterialRenderer::setPixelShaderConstant(const char* name, const float* floats, int count)
{
    return setVariable(false, name, floats, count);
}

void CCGMaterialRenderer::setPixelShaderConstant(const float* data, int startRegister, int constantAmount)
{
    os::Printer::log("Cannot set constant, please use high level shader call instead.", ox::event::ELL_INFORMATION);
}

ox::video::IVideoDriver* CCGMaterialRenderer::getVideoDriver()
{
    return Driver;
}

} // end namespace video
} // end namespace daisy
