// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "COpenGLSLMaterialRenderer.h"
#include "daisy/os.h"
#include "ox/video/IShaderConstantSetCallBack.h"
#include <string.h>
// The native object includes an iostream static initializer.
#include <iostream> // IWYU pragma: keep

namespace daisy {
namespace video {

COpenGLSLMaterialRenderer::COpenGLSLMaterialRenderer(CVideoOpenGL* driver, int& outMaterialTypeNr,
    const char* vertexShaderProgram, const char* vertexShaderEntryPointName,
    ox::video::E_VERTEX_SHADER_TYPE vsCompileTarget, const char* pixelShaderProgram,
    const char* pixelShaderEntryPointName, ox::video::E_PIXEL_SHADER_TYPE psCompileTarget,
    ox::video::IShaderConstantSetCallBack* callback, ox::video::IMaterialRenderer* baseMaterial, int userData)
    : Driver(driver), CallBack(callback), BaseMaterial(baseMaterial), Program(0), UserData(userData)
{
    if (BaseMaterial)
        BaseMaterial->grab();

    if (CallBack)
        CallBack->grab();

    if (!Driver->queryFeature(ox::video::EVDF_ARB_GLSL))
        return;

    if (!Driver->queryFeature(ox::video::EVDF_ARB_VERTEX_PROGRAM_1))
        return;

    if (!Driver->queryFeature(ox::video::EVDF_ARB_FRAGMENT_PROGRAM_1))
        return;

    init(outMaterialTypeNr, vertexShaderProgram, pixelShaderProgram);
}

COpenGLSLMaterialRenderer::COpenGLSLMaterialRenderer(CVideoOpenGL* driver,
    ox::video::IShaderConstantSetCallBack* callback, ox::video::IMaterialRenderer* baseMaterial, int userData)
    : Driver(driver), CallBack(callback), BaseMaterial(baseMaterial), Program(0), UserData(userData)
{
    if (BaseMaterial)
        BaseMaterial->grab();

    if (CallBack)
        CallBack->grab();
}

COpenGLSLMaterialRenderer::~COpenGLSLMaterialRenderer()
{
    if (CallBack)
        CallBack->drop();

    if (Program)
    {
        Driver->extGlDeleteObjectARB(Program);
        Program = 0;
    }

    UniformInfo.clear();

    if (BaseMaterial)
        BaseMaterial->drop();
}

void COpenGLSLMaterialRenderer::init(int& outMaterialTypeNr, const char* vertexShaderProgram,
    const char* pixelShaderProgram)
{
    outMaterialTypeNr = -1;

    if (!createProgram())
        return;

    if (!createShader(GL_VERTEX_SHADER_ARB, vertexShaderProgram))
        return;

    if (!createShader(GL_FRAGMENT_SHADER_ARB, pixelShaderProgram))
        return;

    if (!linkProgram())
        return;

    // register myself as new material
    outMaterialTypeNr = Driver->addMaterialRenderer(this, 0);
}

bool COpenGLSLMaterialRenderer::OnRender(ox::video::IMaterialRendererServices* service, ox::video::E_VERTEX_TYPE vtxtype)
{
    // call callback to set shader constants
    if (CallBack && Program)
        CallBack->OnSetConstants(this, UserData);

    return true;
}

//! Original bug (from Irrlicht 1.2, which had four textures): binds Textures[2] and Textures[3],
//! which read past the two textures into the flags; setTexture ignores those units, since at most
//! two are used.
void COpenGLSLMaterialRenderer::OnSetMaterial(ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
    bool resetAllRenderstates, ox::video::IMaterialRendererServices* services)
{
    Driver->setTexture(3, material.Textures[3]);
    Driver->setTexture(2, material.Textures[2]);
    Driver->setTexture(1, material.Textures[1]);
    Driver->setTexture(0, material.Textures[0]);
    Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

    if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
    {
        if (Program)
            Driver->extGlUseProgramObjectARB(Program);

        if (BaseMaterial)
            BaseMaterial->OnSetMaterial(material, material, true, this);
    }

    setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
}

void COpenGLSLMaterialRenderer::OnUnsetMaterial()
{
    Driver->extGlUseProgramObjectARB(0);

    if (BaseMaterial)
        BaseMaterial->OnUnsetMaterial();
}

bool COpenGLSLMaterialRenderer::isTransparent()
{
    return BaseMaterial ? BaseMaterial->isTransparent() : false;
}

bool COpenGLSLMaterialRenderer::createProgram()
{
    Program = Driver->extGlCreateProgramObjectARB();
    return true;
}

bool COpenGLSLMaterialRenderer::createShader(GLenum shaderType, const char* shader)
{
    GLhandleARB shaderHandle = Driver->extGlCreateShaderObjectARB(shaderType);

    Driver->extGlShaderSourceARB(shaderHandle, 1, &shader, 0);
    Driver->extGlCompileShaderARB(shaderHandle);

    int status = 0;

    Driver->extGlGetObjectParameterivARB(shaderHandle, GL_OBJECT_COMPILE_STATUS_ARB, &status);

    if (!status)
    {
        os::Printer::log("GLSL shader failed to compile", ox::event::ELL_INFORMATION);
        // check error message and log it
        int maxLength = 0;
        GLsizei length;
        Driver->extGlGetObjectParameterivARB(shaderHandle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &maxLength);
        if (maxLength > 0)
        {
            GLcharARB* pInfoLog = new GLcharARB[maxLength];
            Driver->extGlGetInfoLogARB(shaderHandle, maxLength, &length, pInfoLog);
            os::Printer::log(reinterpret_cast<const char*>(pInfoLog), ox::event::ELL_INFORMATION);
            delete[] pInfoLog;
        }

        return false;
    }

    Driver->extGlAttachObjectARB(Program, shaderHandle);

    return true;
}

bool COpenGLSLMaterialRenderer::linkProgram()
{
    Driver->extGlLinkProgramARB(Program);

    int status = 0;

    Driver->extGlGetObjectParameterivARB(Program, GL_OBJECT_LINK_STATUS_ARB, &status);

    if (!status)
    {
        os::Printer::log("GLSL shader program failed to link", ox::event::ELL_INFORMATION);
        // check error message and log it
        int maxLength = 0;
        GLsizei length;
        Driver->extGlGetObjectParameterivARB(Program, GL_OBJECT_INFO_LOG_LENGTH_ARB, &maxLength);
        if (maxLength > 0)
        {
            GLcharARB* pInfoLog = new GLcharARB[maxLength];
            Driver->extGlGetInfoLogARB(Program, maxLength, &length, pInfoLog);
            os::Printer::log(reinterpret_cast<const char*>(pInfoLog), ox::event::ELL_INFORMATION);
            delete[] pInfoLog;
        }

        return false;
    }

    // get uniforms information

    int num = 0;
    Driver->extGlGetObjectParameterivARB(Program, GL_OBJECT_ACTIVE_UNIFORMS_ARB, &num);

    if (num == 0)
    {
        // no uniforms
        return true;
    }

    int maxlen = 0;
    Driver->extGlGetObjectParameterivARB(Program, GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB, &maxlen);

    if (maxlen == 0)
    {
        os::Printer::log("GLSL: failed to retrieve uniform information", ox::event::ELL_INFORMATION);
        return false;
    }

    char* buf = new char[maxlen];

    UniformInfo.clear();

    for (int i = 0; i < num; ++i)
    {
        SUniformInfo ui;
        memset(buf, 0, maxlen);

        GLint size;
        Driver->extGlGetActiveUniformARB(Program, i, maxlen, 0, &size, &ui.type, reinterpret_cast<GLcharARB*>(buf));
        ui.name = buf;

        UniformInfo.push_back(ui);
    }

    delete[] buf;

    return true;
}

void COpenGLSLMaterialRenderer::setBasicRenderStates(const ox::video::SMaterial& material,
    const ox::video::SMaterial& lastMaterial, bool resetAllRenderstates)
{
    // forward
    Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
}

//! Vertex and pixel uniforms share one name space in GLSL.
bool COpenGLSLMaterialRenderer::setVertexShaderConstant(const char* name, const float* floats, int count)
{
    return setPixelShaderConstant(name, floats, count);
}

void COpenGLSLMaterialRenderer::setVertexShaderConstant(const float* data, int startRegister, int constantAmount)
{
    os::Printer::log("Cannot set constant, please use high level shader call instead.", ox::event::ELL_INFORMATION);
}

//! Original bug (from Irrlicht 1.2): the uniform's index in the active uniform list is passed as
//! its location instead of glGetUniformLocation's result; they often but not always agree.
bool COpenGLSLMaterialRenderer::setPixelShaderConstant(const char* name, const float* floats, int count)
{
    int i;
    const int num = (int)UniformInfo.size();

    for (i = 0; i < num; ++i)
    {
        if (UniformInfo[i].name == name)
            break;
    }

    if (i == num)
        return false;

    switch (UniformInfo[i].type)
    {
    case GL_FLOAT:
        Driver->extGlUniform1fvARB(i, count, floats);
        break;
    case GL_FLOAT_VEC2_ARB:
        Driver->extGlUniform2fvARB(i, count / 2, floats);
        break;
    case GL_FLOAT_VEC3_ARB:
        Driver->extGlUniform3fvARB(i, count / 3, floats);
        break;
    case GL_FLOAT_VEC4_ARB:
        Driver->extGlUniform4fvARB(i, count / 4, floats);
        break;
    case GL_FLOAT_MAT2_ARB:
        Driver->extGlUniformMatrix2fvARB(i, count / 4, false, floats);
        break;
    case GL_FLOAT_MAT3_ARB:
        Driver->extGlUniformMatrix3fvARB(i, count / 9, false, floats);
        break;
    case GL_FLOAT_MAT4_ARB:
        Driver->extGlUniformMatrix4fvARB(i, count / 16, false, floats);
        break;
    default:
        Driver->extGlUniform1ivARB(i, count, reinterpret_cast<const GLint*>(floats));
        break;
    }

    return true;
}

void COpenGLSLMaterialRenderer::setPixelShaderConstant(const float* data, int startRegister, int constantAmount)
{
    os::Printer::log("Cannot set constant, use high level shader call.", ox::event::ELL_INFORMATION);
}

ox::video::IVideoDriver* COpenGLSLMaterialRenderer::getVideoDriver()
{
    return Driver;
}

} // end namespace video
} // end namespace daisy
