#include "video/CFixedFunction.h"
#include "video/Shaders.h"
#include "daisy/os.h"
#include "ox/core/CString.h"
#include <string.h>

namespace port {
namespace video {

using namespace gl;

namespace {

const float Identity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

void set4(float out[4], float a, float b, float c, float d)
{
    out[0] = a;
    out[1] = b;
    out[2] = c;
    out[3] = d;
}

void copy4(float out[4], const float in[4])
{
    memcpy(out, in, 4 * sizeof(float));
}

//! The inverse transpose of the upper 3x3 of a column-major 4x4 matrix, column major: OpenGL's
//! normal matrix. A singular matrix gives zero normals.
void normalMatrix(float out[9], const float m[16])
{
    // a(r, c) of the upper 3x3
    const float a00 = m[0], a10 = m[1], a20 = m[2];
    const float a01 = m[4], a11 = m[5], a21 = m[6];
    const float a02 = m[8], a12 = m[9], a22 = m[10];

    const float c00 = a11 * a22 - a12 * a21;
    const float c01 = a12 * a20 - a10 * a22;
    const float c02 = a10 * a21 - a11 * a20;
    const float c10 = a02 * a21 - a01 * a22;
    const float c11 = a00 * a22 - a02 * a20;
    const float c12 = a01 * a20 - a00 * a21;
    const float c20 = a01 * a12 - a02 * a11;
    const float c21 = a02 * a10 - a00 * a12;
    const float c22 = a00 * a11 - a01 * a10;

    const float det = a00 * c00 + a01 * c01 + a02 * c02;
    const float inv = det != 0.0f ? 1.0f / det : 0.0f;

    // inverse(r, c) = cofactor(c, r) / det, so the transposed inverse is cofactor(r, c) / det
    out[0] = c00 * inv;
    out[1] = c10 * inv;
    out[2] = c20 * inv;
    out[3] = c01 * inv;
    out[4] = c11 * inv;
    out[5] = c21 * inv;
    out[6] = c02 * inv;
    out[7] = c12 * inv;
    out[8] = c22 * inv;
}

GLuint compileShader(GLenum type, const char* body, const char* name)
{
    // The common subset of GLSL ES 3.00 and GLSL 3.30; precision qualifiers are no-ops in the
    // latter.
    const char* header = isES() ? "#version 300 es\nprecision highp float;\nprecision highp int;\n" :
                                  "#version 330 core\n";
    const char* sources[2] = {header, body};

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 2, sources, 0);
    glCompileShader(shader);

    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        char log[4096] = "";
        glGetShaderInfoLog(shader, sizeof(log), 0, log);
        daisy::os::Printer::log(type == GL_VERTEX_SHADER ? "Could not compile vertex shader" :
                                                           "Could not compile fragment shader",
            name, ox::event::ELL_ERROR);
        daisy::os::Printer::log(log, "", ox::event::ELL_ERROR);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

} // end anonymous namespace

CFixedFunction::CFixedFunction()
    : ActiveUnit(0), AlphaTest(false), AlphaRef(0.0f), Fog(false), FogLinear(false), FogStart(0.0f),
      FogEnd(1.0f), FogDensity(1.0f), Lighting(false), MaterialShininess(0.0f), Dirty(true), Program(0),
      ShaderProgram(0), BoundProgram(0), VertexArray(0), VertexBuffer(0), IndexBuffer(0)
{
    // OpenGL's initial state
    for (int i = 0; i < MAX_UNITS; ++i)
    {
        Units[i].Enabled = false;
        Units[i].TexGen = false;
        Units[i].Mode = ETEM_MODULATE;
        Units[i].CombineRgb = ECF_COMBINE_MODULATE;
        Units[i].Source[0] = ECS_TEXTURE;
        Units[i].Source[1] = ECS_PREVIOUS;
        Units[i].RgbScale = 1.0f;
    }

    set4(FogColor, 0, 0, 0, 0);

    for (int i = 0; i < MAX_LIGHTS; ++i)
    {
        SLightState& light = Lights[i];
        light.Enabled = false;
        set4(light.Position, 0, 0, 1, 0);
        set4(light.Ambient, 0, 0, 0, 1);
        if (i == 0)
        {
            set4(light.Diffuse, 1, 1, 1, 1);
            set4(light.Specular, 1, 1, 1, 1);
        }
        else
        {
            set4(light.Diffuse, 0, 0, 0, 1);
            set4(light.Specular, 0, 0, 0, 1);
        }
        light.Attenuation[0] = 1.0f;
        light.Attenuation[1] = 0.0f;
        light.Attenuation[2] = 0.0f;
    }

    set4(LightModelAmbient, 0.2f, 0.2f, 0.2f, 1.0f);
    set4(MaterialAmbient, 0.2f, 0.2f, 0.2f, 1.0f);
    set4(MaterialDiffuse, 0.8f, 0.8f, 0.8f, 1.0f);
    set4(MaterialSpecular, 0, 0, 0, 1);
    set4(MaterialEmission, 0, 0, 0, 1);

    memcpy(ModelView, Identity, sizeof(ModelView));
    memcpy(Projection, Identity, sizeof(Projection));
    normalMatrix(NormalMatrix, ModelView);

    memset(&Loc, 0, sizeof(Loc));
}

CFixedFunction::~CFixedFunction()
{
    if (Program)
        glDeleteProgram(Program);
    if (VertexBuffer)
        glDeleteBuffers(1, &VertexBuffer);
    if (IndexBuffer)
        glDeleteBuffers(1, &IndexBuffer);
    if (VertexArray)
        glDeleteVertexArrays(1, &VertexArray);
}

GLuint CFixedFunction::createProgram(const char* vertexBody, const char* fragmentBody, const char* name)
{
    GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexBody, name);
    GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentBody, name);
    if (!vertex || !fragment)
    {
        if (vertex)
            glDeleteShader(vertex);
        if (fragment)
            glDeleteShader(fragment);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glBindAttribLocation(program, EA_POSITION, "aPosition");
    glBindAttribLocation(program, EA_NORMAL, "aNormal");
    glBindAttribLocation(program, EA_COLOR, "aColor");
    glBindAttribLocation(program, EA_TEXCOORD0, "aTexCoord0");
    glBindAttribLocation(program, EA_TEXCOORD1, "aTexCoord1");
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status)
    {
        char log[4096] = "";
        glGetProgramInfoLog(program, sizeof(log), 0, log);
        daisy::os::Printer::log("Could not link shader program", name, ox::event::ELL_ERROR);
        daisy::os::Printer::log(log, "", ox::event::ELL_ERROR);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

bool CFixedFunction::init()
{
    Program = createProgram(FixedVertexShader, FixedFragmentShader, "fixed function");
    if (!Program)
        return false;

    getLocations();

    useProgram(Program);
    glUniform1i(glGetUniformLocation(Program, "uTexture0"), 0);
    glUniform1i(glGetUniformLocation(Program, "uTexture1"), 1);

    glGenVertexArrays(1, &VertexArray);
    glGenBuffers(1, &VertexBuffer);
    glGenBuffers(1, &IndexBuffer);
    glBindVertexArray(VertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer);

    Dirty = true;
    return true;
}

void CFixedFunction::getLocations()
{
    Loc.ModelView = glGetUniformLocation(Program, "uModelView");
    Loc.Projection = glGetUniformLocation(Program, "uProjection");
    Loc.NormalMatrix = glGetUniformLocation(Program, "uNormalMatrix");
    Loc.Lighting = glGetUniformLocation(Program, "uLighting");
    Loc.LightModelAmbient = glGetUniformLocation(Program, "uLightModelAmbient");
    Loc.MaterialAmbient = glGetUniformLocation(Program, "uMaterialAmbient");
    Loc.MaterialDiffuse = glGetUniformLocation(Program, "uMaterialDiffuse");
    Loc.MaterialSpecular = glGetUniformLocation(Program, "uMaterialSpecular");
    Loc.MaterialEmission = glGetUniformLocation(Program, "uMaterialEmission");
    Loc.MaterialShininess = glGetUniformLocation(Program, "uMaterialShininess");
    Loc.LightEnabled = glGetUniformLocation(Program, "uLightEnabled");
    Loc.LightPosition = glGetUniformLocation(Program, "uLightPosition");
    Loc.LightAmbient = glGetUniformLocation(Program, "uLightAmbient");
    Loc.LightDiffuse = glGetUniformLocation(Program, "uLightDiffuse");
    Loc.LightSpecular = glGetUniformLocation(Program, "uLightSpecular");
    Loc.LightAttenuation = glGetUniformLocation(Program, "uLightAttenuation");
    Loc.TexGenSphere = glGetUniformLocation(Program, "uTexGenSphere");
    Loc.TextureEnabled = glGetUniformLocation(Program, "uTextureEnabled");
    Loc.TexEnvMode = glGetUniformLocation(Program, "uTexEnvMode");
    Loc.CombineRgb = glGetUniformLocation(Program, "uCombineRgb");
    Loc.CombineSource0 = glGetUniformLocation(Program, "uCombineSource0");
    Loc.CombineSource1 = glGetUniformLocation(Program, "uCombineSource1");
    Loc.RgbScale = glGetUniformLocation(Program, "uRgbScale");
    Loc.AlphaTest = glGetUniformLocation(Program, "uAlphaTest");
    Loc.AlphaRef = glGetUniformLocation(Program, "uAlphaRef");
    Loc.FogMode = glGetUniformLocation(Program, "uFogMode");
    Loc.FogStart = glGetUniformLocation(Program, "uFogStart");
    Loc.FogEnd = glGetUniformLocation(Program, "uFogEnd");
    Loc.FogDensity = glGetUniformLocation(Program, "uFogDensity");
    Loc.FogColor = glGetUniformLocation(Program, "uFogColor");
}

void CFixedFunction::activeTexture(int unit)
{
    ActiveUnit = unit;
    glActiveTexture(GL_TEXTURE0 + unit);
}

void CFixedFunction::setTexture2D(bool enabled)
{
    Units[ActiveUnit].Enabled = enabled;
    Dirty = true;
}

void CFixedFunction::setTexEnvMode(E_TEXTURE_ENV_MODE mode)
{
    Units[ActiveUnit].Mode = mode;
    Dirty = true;
}

void CFixedFunction::setCombineRgb(E_COMBINE_FUNCTION function)
{
    Units[ActiveUnit].CombineRgb = function;
    Dirty = true;
}

void CFixedFunction::setCombineSource(int argument, E_COMBINE_SOURCE source)
{
    // the light-map renderer also sets GL_SOURCE2_RGB, which only GL_INTERPOLATE reads
    if (argument < 2)
        Units[ActiveUnit].Source[argument] = source;
    Dirty = true;
}

void CFixedFunction::setRgbScale(float scale)
{
    Units[ActiveUnit].RgbScale = scale;
    Dirty = true;
}

void CFixedFunction::setTexGenSphereMap(bool enabled)
{
    Units[ActiveUnit].TexGen = enabled;
    Dirty = true;
}

void CFixedFunction::setAlphaTest(bool enabled)
{
    AlphaTest = enabled;
    Dirty = true;
}

void CFixedFunction::setAlphaRef(float ref)
{
    AlphaRef = ref;
    Dirty = true;
}

void CFixedFunction::setFog(bool enabled)
{
    Fog = enabled;
    Dirty = true;
}

void CFixedFunction::setFogMode(bool linear, float start, float end, float density, const float color[4])
{
    FogLinear = linear;
    FogStart = start;
    FogEnd = end;
    FogDensity = density;
    copy4(FogColor, color);
    Dirty = true;
}

void CFixedFunction::setLighting(bool enabled)
{
    Lighting = enabled;
    Dirty = true;
}

void CFixedFunction::setLightEnabled(int light, bool enabled)
{
    Lights[light].Enabled = enabled;
    Dirty = true;
}

void CFixedFunction::setLightPosition(int light, const float p[4])
{
    const float* m = ModelView;
    float* out = Lights[light].Position;
    for (int r = 0; r < 4; ++r)
        out[r] = m[r] * p[0] + m[4 + r] * p[1] + m[8 + r] * p[2] + m[12 + r] * p[3];
    Dirty = true;
}

void CFixedFunction::setLightColors(int light, const float ambient[4], const float diffuse[4],
    const float specular[4])
{
    copy4(Lights[light].Ambient, ambient);
    copy4(Lights[light].Diffuse, diffuse);
    copy4(Lights[light].Specular, specular);
    Dirty = true;
}

void CFixedFunction::setLightAttenuation(int light, float constant, float linear, float quadratic)
{
    Lights[light].Attenuation[0] = constant;
    Lights[light].Attenuation[1] = linear;
    Lights[light].Attenuation[2] = quadratic;
    Dirty = true;
}

void CFixedFunction::setLightModelAmbient(const float color[4])
{
    copy4(LightModelAmbient, color);
    Dirty = true;
}

void CFixedFunction::setMaterialColors(const float ambient[4], const float diffuse[4], const float specular[4],
    const float emission[4], float shininess)
{
    copy4(MaterialAmbient, ambient);
    copy4(MaterialDiffuse, diffuse);
    copy4(MaterialSpecular, specular);
    copy4(MaterialEmission, emission);
    MaterialShininess = shininess;
    Dirty = true;
}

void CFixedFunction::loadModelView(const float matrix[16])
{
    memcpy(ModelView, matrix, sizeof(ModelView));
    normalMatrix(NormalMatrix, ModelView);
    Dirty = true;
}

void CFixedFunction::loadProjection(const float matrix[16])
{
    memcpy(Projection, matrix, sizeof(Projection));
    Dirty = true;
}

void CFixedFunction::loadIdentities()
{
    loadModelView(Identity);
    loadProjection(Identity);
}

void CFixedFunction::bindShaderProgram(GLuint program)
{
    ShaderProgram = program;
}

void CFixedFunction::useProgram(GLuint program)
{
    if (BoundProgram != program)
    {
        glUseProgram(program);
        BoundProgram = program;
    }
}

void CFixedFunction::uploadState()
{
    glUniformMatrix4fv(Loc.ModelView, 1, GL_FALSE, ModelView);
    glUniformMatrix4fv(Loc.Projection, 1, GL_FALSE, Projection);
    glUniformMatrix3fv(Loc.NormalMatrix, 1, GL_FALSE, NormalMatrix);

    glUniform1i(Loc.Lighting, Lighting);
    if (Lighting)
    {
        GLint enabled[MAX_LIGHTS];
        float position[MAX_LIGHTS * 4], ambient[MAX_LIGHTS * 4], diffuse[MAX_LIGHTS * 4], specular[MAX_LIGHTS * 4];
        float attenuation[MAX_LIGHTS * 3];
        for (int i = 0; i < MAX_LIGHTS; ++i)
        {
            enabled[i] = Lights[i].Enabled;
            copy4(position + i * 4, Lights[i].Position);
            copy4(ambient + i * 4, Lights[i].Ambient);
            copy4(diffuse + i * 4, Lights[i].Diffuse);
            copy4(specular + i * 4, Lights[i].Specular);
            memcpy(attenuation + i * 3, Lights[i].Attenuation, 3 * sizeof(float));
        }
        glUniform1iv(Loc.LightEnabled, MAX_LIGHTS, enabled);
        glUniform4fv(Loc.LightPosition, MAX_LIGHTS, position);
        glUniform4fv(Loc.LightAmbient, MAX_LIGHTS, ambient);
        glUniform4fv(Loc.LightDiffuse, MAX_LIGHTS, diffuse);
        glUniform4fv(Loc.LightSpecular, MAX_LIGHTS, specular);
        glUniform3fv(Loc.LightAttenuation, MAX_LIGHTS, attenuation);
        glUniform4fv(Loc.LightModelAmbient, 1, LightModelAmbient);
        glUniform4fv(Loc.MaterialAmbient, 1, MaterialAmbient);
        glUniform4fv(Loc.MaterialDiffuse, 1, MaterialDiffuse);
        glUniform4fv(Loc.MaterialSpecular, 1, MaterialSpecular);
        glUniform4fv(Loc.MaterialEmission, 1, MaterialEmission);
        glUniform1f(Loc.MaterialShininess, MaterialShininess);
    }

    GLint texGen[MAX_UNITS], enabled[MAX_UNITS], mode[MAX_UNITS], combine[MAX_UNITS], source0[MAX_UNITS],
        source1[MAX_UNITS];
    float scale[MAX_UNITS];
    for (int i = 0; i < MAX_UNITS; ++i)
    {
        texGen[i] = Units[i].TexGen;
        enabled[i] = Units[i].Enabled;
        mode[i] = Units[i].Mode;
        combine[i] = Units[i].CombineRgb;
        source0[i] = Units[i].Source[0];
        source1[i] = Units[i].Source[1];
        scale[i] = Units[i].RgbScale;
    }
    glUniform1iv(Loc.TexGenSphere, MAX_UNITS, texGen);
    glUniform1iv(Loc.TextureEnabled, MAX_UNITS, enabled);
    glUniform1iv(Loc.TexEnvMode, MAX_UNITS, mode);
    glUniform1iv(Loc.CombineRgb, MAX_UNITS, combine);
    glUniform1iv(Loc.CombineSource0, MAX_UNITS, source0);
    glUniform1iv(Loc.CombineSource1, MAX_UNITS, source1);
    glUniform1fv(Loc.RgbScale, MAX_UNITS, scale);

    glUniform1i(Loc.AlphaTest, AlphaTest);
    glUniform1f(Loc.AlphaRef, AlphaRef);

    glUniform1i(Loc.FogMode, Fog ? (FogLinear ? 1 : 2) : 0);
    glUniform1f(Loc.FogStart, FogStart);
    glUniform1f(Loc.FogEnd, FogEnd);
    glUniform1f(Loc.FogDensity, FogDensity);
    glUniform4fv(Loc.FogColor, 1, FogColor);

    Dirty = false;
}

void CFixedFunction::draw(GLenum mode, const SVertexArrays& arrays, const unsigned short* indices, int indexCount)
{
    if (ShaderProgram)
        useProgram(ShaderProgram);
    else
    {
        useProgram(Program);
        if (Dirty)
            uploadState();
    }

    // S3DVertex2TCoords starts with S3DVertex's members: Pos, Normal, Color, TCoords
    const ox::video::S3DVertex layout;
    const char* base = (const char*)&layout;
    glBindVertexArray(VertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)arrays.Count * arrays.Stride, arrays.Vertices, GL_STREAM_DRAW);

    glEnableVertexAttribArray(EA_POSITION);
    glVertexAttribPointer(EA_POSITION, 3, GL_FLOAT, GL_FALSE, arrays.Stride,
        (const void*)((const char*)&layout.Pos - base));
    glEnableVertexAttribArray(EA_NORMAL);
    glVertexAttribPointer(EA_NORMAL, 3, GL_FLOAT, GL_FALSE, arrays.Stride,
        (const void*)((const char*)&layout.Normal - base));
    glEnableVertexAttribArray(EA_COLOR);
    glVertexAttribPointer(EA_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, arrays.Stride,
        (const void*)((const char*)&layout.Color - base));
    glEnableVertexAttribArray(EA_TEXCOORD0);
    glVertexAttribPointer(EA_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, arrays.Stride,
        (const void*)((const char*)&layout.TCoords - base));
    if (arrays.TCoords2Offset >= 0)
    {
        glEnableVertexAttribArray(EA_TEXCOORD1);
        glVertexAttribPointer(EA_TEXCOORD1, 2, GL_FLOAT, GL_FALSE, arrays.Stride,
            (const void*)(ptrdiff_t)arrays.TCoords2Offset);
    }
    else
    {
        glDisableVertexAttribArray(EA_TEXCOORD1);
        glVertexAttrib4f(EA_TEXCOORD1, 0.0f, 0.0f, 0.0f, 1.0f);
    }

    if (indices)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)indexCount * sizeof(unsigned short), indices,
            GL_STREAM_DRAW);
        glDrawElements(mode, indexCount, GL_UNSIGNED_SHORT, (const void*)0);
    }
    else
        glDrawArrays(mode, 0, arrays.Count);
}

} // end namespace video
} // end namespace port
