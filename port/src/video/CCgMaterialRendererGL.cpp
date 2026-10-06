#include "video/CCgMaterialRendererGL.h"
#include "video/CVideoGL.h"
#include "video/Shaders.h"
#include "daisy/os.h"
#include "ox/video/IShaderConstantSetCallBack.h"
#include <string.h>

namespace port {
namespace video {

using namespace gl;

CCgMaterialRendererGL::CCgMaterialRendererGL(CVideoGL* driver, int& outMaterialTypeNr, const char* vertexSource,
    const char* pixelSource, const char* name, ox::video::IShaderConstantSetCallBack* callback,
    ox::video::IMaterialRenderer* baseMaterial, int userData)
    : Driver(driver), CallBack(callback), BaseMaterial(baseMaterial), UserData(userData), Program(0),
      FirstVariableError(true)
{
    if (BaseMaterial)
        BaseMaterial->grab();

    if (CallBack)
        CallBack->grab();

    outMaterialTypeNr = -1;

    Program = CFixedFunction::createProgram(vertexSource, pixelSource, name);
    if (!Program)
        return;

    addVariables(vertexSource, true);
    addVariables(pixelSource, false);

    // Cg's TEXn semantics: the samplers read fixed texture units
    Driver->fixedFunction().useProgram(Program);
    for (int i = 0; CgSamplerUnits[i].Name; ++i)
    {
        GLint location = glGetUniformLocation(Program, CgSamplerUnits[i].Name);
        if (location >= 0)
            glUniform1i(location, CgSamplerUnits[i].Unit);
    }

    outMaterialTypeNr = Driver->addMaterialRenderer(this, 0);
}

//! Original bug, kept: the callback grabbed by the constructor is never dropped.
CCgMaterialRendererGL::~CCgMaterialRendererGL()
{
    if (Program)
    {
        if (Driver->fixedFunction().getShaderProgram() == Program)
            Driver->fixedFunction().bindShaderProgram(0);
        Driver->fixedFunction().useProgram(0);
        glDeleteProgram(Program);
    }

    if (BaseMaterial)
        BaseMaterial->drop();
}

void CCgMaterialRendererGL::addVariables(const char* source, bool vertex)
{
    const char* prefix = vertex ? "vs_" : "ps_";
    const int prefixLength = 3;

    for (const char* line = source; line && *line;)
    {
        const char* end = strchr(line, '\n');
        if (!end)
            end = line + strlen(line);

        // "uniform <type> <prefix><name>;"
        if (strncmp(line, "uniform ", 8) == 0)
        {
            const char* type = line + 8;
            const char* typeEnd = strchr(type, ' ');
            if (typeEnd && typeEnd < end)
            {
                const char* glslName = typeEnd + 1;
                const char* nameEnd = strchr(glslName, ';');
                if (nameEnd && nameEnd < end && strncmp(glslName, prefix, prefixLength) == 0)
                {
                    SVariable variable;
                    variable.Type = ox::core::CString<char>(type, (int)(typeEnd - type));
                    ox::core::CString<char> fullName(glslName, (int)(nameEnd - glslName));
                    variable.Name = ox::core::CString<char>(glslName + prefixLength,
                        (int)(nameEnd - glslName) - prefixLength);
                    variable.Vertex = vertex;
                    variable.Location = glGetUniformLocation(Program, fullName.c_str());
                    Variables.push_back(variable);
                }
            }
        }

        line = *end ? end + 1 : end;
    }
}

//! On a material change: constants with user data 0, the program, then the base material's states
//! as if everything had to be reset.
void CCgMaterialRendererGL::OnSetMaterial(ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
    bool resetAllRenderstates, ox::video::IMaterialRendererServices* services)
{
    if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
    {
        if (CallBack)
            CallBack->OnSetConstants(this, 0);

        Driver->fixedFunction().bindShaderProgram(Program);

        if (BaseMaterial)
            BaseMaterial->OnSetMaterial(material, material, true, this);
    }

    services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
}

bool CCgMaterialRendererGL::OnRender(ox::video::IMaterialRendererServices* service, ox::video::E_VERTEX_TYPE vtxtype)
{
    if (CallBack && Program)
        CallBack->OnSetConstants(this, UserData);

    return true;
}

void CCgMaterialRendererGL::OnUnsetMaterial()
{
    Driver->fixedFunction().bindShaderProgram(0);

    if (BaseMaterial)
        BaseMaterial->OnUnsetMaterial();
}

bool CCgMaterialRendererGL::isTransparent()
{
    return BaseMaterial ? BaseMaterial->isTransparent() : false;
}

void CCgMaterialRendererGL::setBasicRenderStates(const ox::video::SMaterial& material,
    const ox::video::SMaterial& lastMaterial, bool resetAllRenderstates)
{
    Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
}

//! cgSetParameterValuefc: count floats, matrices in column-major order (OpenGL's).
bool CCgMaterialRendererGL::setVariable(bool vertex, const char* name, const float* floats, int count)
{
    for (unsigned int i = 0; i < Variables.size(); ++i)
    {
        const SVariable& variable = Variables[i];
        if (variable.Vertex != vertex || strcmp(variable.Name.c_str(), name) != 0)
            continue;

        if (variable.Location < 0)
            return true;

        Driver->fixedFunction().useProgram(Program);

        const char* type = variable.Type.c_str();
        if (!strcmp(type, "float"))
            glUniform1fv(variable.Location, 1, floats);
        else if (!strcmp(type, "vec2") && count >= 2)
            glUniform2fv(variable.Location, 1, floats);
        else if (!strcmp(type, "vec3") && count >= 3)
            glUniform3fv(variable.Location, 1, floats);
        else if (!strcmp(type, "vec4") && count >= 4)
            glUniform4fv(variable.Location, 1, floats);
        else if (!strcmp(type, "mat4") && count >= 16)
            glUniformMatrix4fv(variable.Location, 1, GL_FALSE, floats);
        else
        {
            daisy::os::Printer::log("Error setting variable", name, ox::event::ELL_WARNING);
            return false;
        }
        return true;
    }

    if (FirstVariableError)
    {
        FirstVariableError = false;

        ox::core::CString<char> message;
        message = "CG variable not found: '";
        message.append(ox::core::CString<char>(name));
        message.append(ox::core::CString<char>("'. Available variables are:"));
        daisy::os::Printer::log(message.c_str(), "", ox::event::ELL_WARNING);
        printVariables();
    }

    return false;
}

void CCgMaterialRendererGL::printVariables()
{
    for (int stage = 0; stage < 2; ++stage)
    {
        const bool vertex = stage == 0;
        daisy::os::Printer::log(vertex ? "Vertex program parameters:" : "Pixel program parameters:", "",
            ox::event::ELL_WARNING);

        for (unsigned int i = 0; i < Variables.size(); ++i)
        {
            if (Variables[i].Vertex != vertex)
                continue;

            ox::core::CString<char> line("- ");
            line.append(Variables[i].Name);
            line.append(ox::core::CString<char>(" - "));
            line.append(Variables[i].Type);
            daisy::os::Printer::log(line.c_str(), "", ox::event::ELL_WARNING);
        }
    }
}

bool CCgMaterialRendererGL::setVertexShaderConstant(const char* name, const float* floats, int count)
{
    return setVariable(true, name, floats, count);
}

void CCgMaterialRendererGL::setVertexShaderConstant(const float* data, int startRegister, int constantAmount)
{
    daisy::os::Printer::log("Cannot set constant, please use high level shader call instead.", "",
        ox::event::ELL_INFORMATION);
}

bool CCgMaterialRendererGL::setPixelShaderConstant(const char* name, const float* floats, int count)
{
    return setVariable(false, name, floats, count);
}

void CCgMaterialRendererGL::setPixelShaderConstant(const float* data, int startRegister, int constantAmount)
{
    daisy::os::Printer::log("Cannot set constant, please use high level shader call instead.", "",
        ox::event::ELL_INFORMATION);
}

ox::video::IVideoDriver* CCgMaterialRendererGL::getVideoDriver()
{
    return Driver;
}

} // end namespace video
} // end namespace port
