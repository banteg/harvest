// The fixed-function OpenGL state the original renderer relied on, emulated over OpenGL ES 3.0 /
// OpenGL 3.3 core. CVideoOpenGL and its material renderers drove OpenGL 1.x: texture environments,
// GL_TEXTURE_2D enables per unit, the alpha test, lighting, materials, fog, sphere-map texture
// generation, the modelview and projection matrices and immediate-mode drawing. This class keeps
// that state exactly as those calls would leave it and draws with one shader program that applies
// it (shaders/fixed.vert, shaders/fixed.frag), so the renderer can issue the original's calls in the
// original's order and get the same leaks of state from one draw into the next.
//
// State that core OpenGL still has (blending, depth, culling, front face, scissor, viewport, texture
// binding, texture parameters, the active texture unit) stays in OpenGL and is set directly.

#ifndef PORT_VIDEO_CFIXEDFUNCTION_H
#define PORT_VIDEO_CFIXEDFUNCTION_H

#include "video/GL.h"
#include "ox/video/S3DVertex2TCoords.h"

namespace port {
namespace video {

//! The texture environment modes the original sets (glTexEnvi GL_TEXTURE_ENV_MODE).
enum E_TEXTURE_ENV_MODE
{
    ETEM_MODULATE = 0,
    ETEM_DECAL,
    ETEM_REPLACE,
    ETEM_ADD,
    ETEM_COMBINE
};

//! GL_COMBINE_RGB functions used by the light-map renderer.
enum E_COMBINE_FUNCTION
{
    ECF_COMBINE_REPLACE = 0,
    ECF_COMBINE_MODULATE
};

//! GL_SOURCEn_RGB values used by the light-map renderer.
enum E_COMBINE_SOURCE
{
    ECS_TEXTURE = 0,
    ECS_PREVIOUS,
    ECS_CONSTANT
};

//! The vertex arrays of a draw: positions, normals, colours and texture coordinates of
//! ox::video::S3DVertex or S3DVertex2TCoords, which share the layout of their first four members.
struct SVertexArrays
{
    SVertexArrays(const ox::video::S3DVertex* vertices, int count)
        : Vertices(vertices), Count(count), Stride(sizeof(ox::video::S3DVertex)), TCoords2Offset(-1)
    {
    }

    SVertexArrays(const ox::video::S3DVertex2TCoords* vertices, int count)
        : Vertices(vertices), Count(count), Stride(sizeof(ox::video::S3DVertex2TCoords)),
          TCoords2Offset((int)((const char*)&vertices->TCoords2 - (const char*)vertices))
    {
    }

    const void* Vertices;
    int Count;
    int Stride;
    //! Byte offset of the second texture coordinates, or -1 if the vertices have none (the second
    //! unit then reads the current texture coordinate (0, 0), as OpenGL does).
    int TCoords2Offset;
};

class CFixedFunction
{
public:
    //! The fixed attribute locations shared by every program, including the shader materials'.
    enum E_ATTRIBUTE
    {
        EA_POSITION = 0,
        EA_NORMAL,
        EA_COLOR,
        EA_TEXCOORD0,
        EA_TEXCOORD1
    };

    //! Lights the shader evaluates (OpenGL 1.x guarantees eight).
    static const int MAX_LIGHTS = 8;
    //! Texture units the original uses.
    static const int MAX_UNITS = 2;

    CFixedFunction();
    ~CFixedFunction();

    //! Compiles the program and creates the vertex array and buffers; the context must be current.
    bool init();

    //! Compiles and links a program from GLSL bodies without a #version line; the attribute
    //! locations are bound as E_ATTRIBUTE names them (aPosition, aNormal, aColor, aTexCoord0,
    //! aTexCoord1). Returns 0 and logs on failure.
    static gl::GLuint createProgram(const char* vertexBody, const char* fragmentBody, const char* name);

    // texture units (the active unit is OpenGL's)
    void activeTexture(int unit);
    int getActiveTexture() const { return ActiveUnit; }
    //! glEnable/glDisable(GL_TEXTURE_2D) on the active unit.
    void setTexture2D(bool enabled);
    //! glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode) on the active unit.
    void setTexEnvMode(E_TEXTURE_ENV_MODE mode);
    void setCombineRgb(E_COMBINE_FUNCTION function);
    void setCombineSource(int argument, E_COMBINE_SOURCE source);
    void setRgbScale(float scale);
    //! glEnable/glDisable(GL_TEXTURE_GEN_S and _T) on the active unit; the mode is always sphere map.
    void setTexGenSphereMap(bool enabled);

    // per-fragment state
    //! glEnable/glDisable(GL_ALPHA_TEST); the function is always GL_GREATER.
    void setAlphaTest(bool enabled);
    void setAlphaRef(float ref);
    void setFog(bool enabled);
    void setFogMode(bool linear, float start, float end, float density, const float color[4]);

    // lighting
    void setLighting(bool enabled);
    void setLightEnabled(int light, bool enabled);
    //! glLightfv(GL_POSITION): transformed by the current modelview matrix, as OpenGL does.
    void setLightPosition(int light, const float position[4]);
    void setLightColors(int light, const float ambient[4], const float diffuse[4], const float specular[4]);
    void setLightAttenuation(int light, float constant, float linear, float quadratic);
    void setLightModelAmbient(const float color[4]);
    void setMaterialColors(const float ambient[4], const float diffuse[4], const float specular[4],
        const float emission[4], float shininess);

    // matrices (column major, as glLoadMatrixf takes them)
    void loadModelView(const float matrix[16]);
    void loadProjection(const float matrix[16]);
    void loadIdentities();

    //! The program shader materials bind (cgGLBindProgram); 0 returns to the fixed-function program.
    void bindShaderProgram(gl::GLuint program);
    gl::GLuint getShaderProgram() const { return ShaderProgram; }
    //! glUseProgram with a cache of the bound program.
    void useProgram(gl::GLuint program);

    //! Draws indexed (or, without indices, consecutive) vertices with the current state.
    void draw(gl::GLenum mode, const SVertexArrays& arrays, const unsigned short* indices, int indexCount);

private:
    struct SUnit
    {
        bool Enabled;
        bool TexGen;
        E_TEXTURE_ENV_MODE Mode;
        E_COMBINE_FUNCTION CombineRgb;
        E_COMBINE_SOURCE Source[2];
        float RgbScale;
    };

    struct SLightState
    {
        bool Enabled;
        float Position[4];
        float Ambient[4];
        float Diffuse[4];
        float Specular[4];
        float Attenuation[3];
    };

    void uploadState();
    void getLocations();

    int ActiveUnit;
    SUnit Units[MAX_UNITS];

    bool AlphaTest;
    float AlphaRef;
    bool Fog;
    bool FogLinear;
    float FogStart;
    float FogEnd;
    float FogDensity;
    float FogColor[4];

    bool Lighting;
    SLightState Lights[MAX_LIGHTS];
    float LightModelAmbient[4];
    float MaterialAmbient[4];
    float MaterialDiffuse[4];
    float MaterialSpecular[4];
    float MaterialEmission[4];
    float MaterialShininess;

    float ModelView[16];
    float Projection[16];
    float NormalMatrix[9];

    //! Whether the fixed-function program's uniforms are stale.
    bool Dirty;

    gl::GLuint Program;
    gl::GLuint ShaderProgram;
    gl::GLuint BoundProgram;
    gl::GLuint VertexArray;
    gl::GLuint VertexBuffer;
    gl::GLuint IndexBuffer;

    struct SLocations
    {
        gl::GLint ModelView, Projection, NormalMatrix;
        gl::GLint Lighting, LightModelAmbient;
        gl::GLint MaterialAmbient, MaterialDiffuse, MaterialSpecular, MaterialEmission, MaterialShininess;
        gl::GLint LightEnabled, LightPosition, LightAmbient, LightDiffuse, LightSpecular, LightAttenuation;
        gl::GLint TexGenSphere, TextureEnabled, TexEnvMode, CombineRgb, CombineSource0, CombineSource1, RgbScale;
        gl::GLint AlphaTest, AlphaRef;
        gl::GLint FogMode, FogStart, FogEnd, FogDensity, FogColor;
    } Loc;
};

} // end namespace video
} // end namespace port

#endif
