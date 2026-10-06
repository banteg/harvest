// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CVideoOpenGL.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source. Oxeye merged
// the extension handling of later Irrlicht versions (1.2/1.3) into the 0.7 driver and added the
// 2D quad batch, GLSL and Cg materials and the material flags of SMaterial. Member offsets are the
// Linux amd64 ones (sizeof 0x434c8).

#ifndef DAISY_VIDEO_OPENGL_CVIDEOOPENGL_H
#define DAISY_VIDEO_OPENGL_CVIDEOOPENGL_H

#include "daisy/video/Null/CVideoNull.h"
#include "ox/core/CMatrix4.h"
#include "ox/video/IMaterialRendererServices.h"
#include "ox/video/SMaterial.h"
#include <vector>

#define GL_GLEXT_LEGACY 1
#include <GL/gl.h>
#undef GL_GLEXT_LEGACY
#include <GL/glext.h>

namespace ox {
class IOxDevice;
} // end namespace ox

namespace daisy {
namespace video {

class CVideoOpenGL : public CVideoNull, public ox::video::IMaterialRendererServices
{
public:
    //! device is the window device (CIrrDeviceLinux); endScene presents through it. The fullscreen
    //! argument is ignored on Linux.
    CVideoOpenGL(const ox::core::CDimension2d<int>& screenSize, ox::IOxDevice* device, bool fullscreen,
        bool stencilBuffer, ox::io::IFileSystem* io);

    virtual ~CVideoOpenGL();

    virtual bool beginScene(bool backBuffer, bool zBuffer, ox::video::SColor color);
    virtual void clearScreen(bool zBuffer, ox::video::SColor color);
    virtual bool endScene();
    virtual bool queryFeature(ox::video::E_VIDEO_DRIVER_FEATURE feature);
    virtual void setTransform(ox::video::E_TRANSFORMATION_STATE state, const ox::core::CMatrix4& mat);
    virtual ox::core::CMatrix4 getTransform(ox::video::E_TRANSFORMATION_STATE state);
    virtual void setMaterial(const ox::video::SMaterial& material);

    //! Records the flag; the OpenGL driver never reads it.
    virtual void useMaterialShaderFor2D(bool enabled)
    {
        UseMaterialShaderFor2D = enabled;
        UseMaterialShaderFor2DSet = true;
    }

    virtual void flushRender() {}

    virtual ox::video::ITexture* createScreenTexture(const ox::core::CDimension2d<int>& size);
    virtual bool setRenderTarget(ox::video::ITexture* texture, bool clearBackBuffer, bool clearZBuffer,
        ox::video::SColor color);
    virtual void setViewPort(const ox::core::CRect<int>& area);

    virtual void drawIndexedTriangleList(const ox::video::S3DVertex* vertices, int vertexCount,
        const unsigned short* indexList, int triangleCount);
    virtual void drawIndexedTriangleList(const ox::video::S3DVertex2TCoords* vertices, int vertexCount,
        const unsigned short* indexList, int triangleCount);
    virtual void drawIndexedTriangleFan(const ox::video::S3DVertex* vertices, int vertexCount,
        const unsigned short* indexList, int triangleCount);
    virtual void drawIndexedTriangleFan(const ox::video::S3DVertex2TCoords* vertices, int vertexCount,
        const unsigned short* indexList, int triangleCount);
    virtual void draw3DLine(const ox::core::CVector3d<float>& start, const ox::core::CVector3d<float>& end,
        ox::video::SColor color);

    virtual void draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<int>& destPos);
    virtual void draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<int>& destPos,
        const ox::core::CRect<int>& sourceRect, const ox::core::CRect<int>* clipRect, ox::video::SColor color,
        bool useAlphaChannelOfTexture);
    virtual void drawScaled2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<float>& destPos,
        const ox::core::CRect<int>& sourceRect, float scale, ox::video::SColor color,
        bool useAlphaChannelOfTexture);
    virtual void draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<int>& destPos,
        const ox::core::CRect<int>& sourceRect, const ox::core::CRect<int>* clipRect, ox::video::SColor* colors,
        bool useAlphaChannelOfTexture);
    virtual void draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<int>& corner1,
        const ox::core::CPosition2d<int>& corner2, const ox::core::CPosition2d<int>& corner3,
        const ox::core::CPosition2d<int>& corner4, const ox::core::CRect<int>& sourceRect,
        ox::video::SColor* colors, bool useAlphaChannelOfTexture);
    virtual void draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<float>& corner1,
        const ox::core::CPosition2d<float>& corner2, const ox::core::CPosition2d<float>& corner3,
        const ox::core::CPosition2d<float>& corner4, const ox::core::CRect<int>& sourceRect,
        const ox::video::SColorArray* colors, bool useAlphaChannelOfTexture);
    virtual void draw2DRectangle(ox::video::SColor color, const ox::core::CRect<int>& pos,
        const ox::core::CRect<int>* clip);
    virtual void draw2DLine(const ox::core::CPosition2d<int>& start, const ox::core::CPosition2d<int>& end,
        ox::video::SColor color);
    virtual void draw2DLineFloat(const ox::core::CPosition2d<float>& start, const ox::core::CPosition2d<float>& end,
        ox::video::SColor color);

    virtual void drawStencilShadowVolume(const ox::core::CVector3d<float>* triangles, int count, bool zfail);
    virtual void drawStencilShadow(bool clearStencilBuffer, ox::video::SColor leftUpEdge,
        ox::video::SColor rightUpEdge, ox::video::SColor leftDownEdge, ox::video::SColor rightDownEdge);
    virtual void setFog(ox::video::SColor color, bool linearFog, float start, float end, float density,
        bool pixelFog, bool rangeFog);

    virtual void deleteAllDynamicLights();
    virtual void addDynamicLight(const ox::video::SLight& light);
    virtual void setAmbientLight(const ox::video::SColorf& color);
    virtual int getMaximalDynamicLightAmount();
    virtual const wchar_t* getName();
    virtual void OnResize(const ox::core::CDimension2d<int>& size);
    virtual int getDriverType();
    virtual bool isFullscreen();
    virtual bool setFullscreen(bool fullscreen);
    //! Mac only: a full screen presentation mode; does nothing here.
    bool setPresentationForFullscreen(bool presentation);
    virtual void setScissorRect(ox::core::CRect<int>* rect);
    virtual void* getGPUProgrammingServices();
    virtual void* getPostProcessingServices();
    virtual bool saveJpegScreenshot(const char* directory, const char* name);

    virtual int addHighLevelShaderMaterial(const char* vertexShaderProgram, const char* vertexShaderEntryPointName,
        ox::video::E_VERTEX_SHADER_TYPE vsCompileTarget, const char* pixelShaderProgram,
        const char* pixelShaderEntryPointName, ox::video::E_PIXEL_SHADER_TYPE psCompileTarget,
        ox::video::IShaderConstantSetCallBack* callback, ox::video::E_MATERIAL_TYPE baseMaterial, int userData);
    virtual int addShaderMaterial(const char* vertexShaderProgram, const char* pixelShaderProgram,
        ox::video::IShaderConstantSetCallBack* callback, ox::video::E_MATERIAL_TYPE baseMaterial, int userData);
    virtual int addCgShaderMaterial(const char* vertexShaderProgram, const char* vertexShaderEntryPointName,
        const char* pixelShaderProgram, const char* pixelShaderEntryPointName,
        ox::video::IShaderConstantSetCallBack* callback, ox::video::E_MATERIAL_TYPE baseMaterial,
        bool precompiled, int userData);

    virtual void captureScreenBuffer(unsigned int index, const ox::core::CRect<int>& area);
    virtual void flush2dRendering();

    // IMaterialRendererServices
    virtual void setBasicRenderStates(const ox::video::SMaterial& material, const ox::video::SMaterial& lastmaterial,
        bool resetAllRenderstates);
    virtual void setVertexShaderConstant(const float* data, int startRegister, int constantAmount = 1);
    virtual void setPixelShaderConstant(const float* data, int startRegister, int constantAmount = 1);
    virtual bool setVertexShaderConstant(const char* name, const float* floats, int count);
    virtual bool setPixelShaderConstant(const char* name, const float* floats, int count);
    virtual ox::video::IVideoDriver* getVideoDriver();

    //! Binds texture to a texture unit (or unbinds it for 0); false if the texture belongs to
    //! another driver.
    bool setTexture(int stage, ox::video::ITexture* texture);
    //! Unbinds the textures of all units from fromStage on.
    bool disableTextures(int fromStage = 0);

    // public access to the (loaded) extensions; each does nothing when its function is missing
    void extGlActiveTextureARB(GLenum texture);
    void extGlClientActiveTextureARB(GLenum texture);
    void extGlGenProgramsARB(GLsizei n, GLuint* programs);
    void extGlBindProgramARB(GLenum target, GLuint program);
    void extGlProgramStringARB(GLenum target, GLenum format, GLsizei len, const GLvoid* string);
    void extGlDeleteProgramsARB(GLsizei n, const GLuint* programs);
    void extGlProgramLocalParameter4fvARB(GLenum target, GLuint index, const GLfloat* params);
    GLhandleARB extGlCreateShaderObjectARB(GLenum shaderType);
    void extGlShaderSourceARB(GLhandleARB shader, int numOfStrings, const char** strings, int* lenOfStrings);
    void extGlCompileShaderARB(GLhandleARB shader);
    GLhandleARB extGlCreateProgramObjectARB();
    void extGlAttachObjectARB(GLhandleARB program, GLhandleARB shader);
    void extGlLinkProgramARB(GLhandleARB program);
    void extGlUseProgramObjectARB(GLhandleARB prog);
    void extGlDeleteObjectARB(GLhandleARB object);
    void extGlGetInfoLogARB(GLhandleARB object, GLsizei maxLength, GLsizei* length, GLcharARB* infoLog);
    void extGlGetObjectParameterivARB(GLhandleARB object, GLenum type, int* param);
    GLint extGlGetUniformLocationARB(GLhandleARB program, const char* name);
    void extGlUniform4fvARB(GLint location, GLsizei count, const GLfloat* v);
    void extGlUniform1ivARB(GLint location, GLsizei count, const GLint* v);
    void extGlUniform1fvARB(GLint location, GLsizei count, const GLfloat* v);
    void extGlUniform2fvARB(GLint location, GLsizei count, const GLfloat* v);
    void extGlUniform3fvARB(GLint location, GLsizei count, const GLfloat* v);
    void extGlUniformMatrix2fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat* v);
    void extGlUniformMatrix3fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat* v);
    void extGlUniformMatrix4fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat* v);
    void extGlGetActiveUniformARB(GLhandleARB program, GLuint index, GLsizei maxlength, GLsizei* length,
        GLint* size, GLenum* type, GLcharARB* name);
    void extGlPointParameterfARB(GLint loc, GLfloat f);
    void extGlPointParameterfvARB(GLint loc, const GLfloat* v);
    void extGlStencilFuncSeparate(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);
    void extGlStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass);
    void extGlCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width,
        GLsizei height, GLint border, GLsizei imageSize, const void* data);
    void extGlBindFramebufferEXT(GLenum target, GLuint framebuffer);
    void extGlDeleteFramebuffersEXT(GLsizei n, const GLuint* framebuffers);
    void extGlGenFramebuffersEXT(GLsizei n, GLuint* framebuffers);
    GLenum extGlCheckFramebufferStatusEXT(GLenum target);
    void extGlFramebufferTexture2DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
        GLint level);
    void extGlBindRenderbufferEXT(GLenum target, GLuint renderbuffer);
    void extGlDeleteRenderbuffersEXT(GLsizei n, const GLuint* renderbuffers);
    void extGlGenRenderbuffersEXT(GLsizei n, GLuint* renderbuffers);
    void extGlRenderbufferStorageEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    void extGlFramebufferRenderbufferEXT(GLenum target, GLenum attachment, GLenum renderbuffertarget,
        GLuint renderbuffer);
    bool hasMultiTextureExtension();

private:
    //! Rendering modes, to switch the render states only when the mode changes.
    enum E_RENDER_MODE
    {
        //! no render state has been set yet
        ERM_NONE = 0,
        ERM_2D,
        ERM_3D
    };

    typedef int (*PFNGLXSWAPINTERVALSGIPROC_)(int interval);

    virtual ox::video::ITexture* createDeviceDependentTexture(ox::video::IImage* surface);

    //! Adds the quads of the current batch to the screen when the texture or blend mode changes or
    //! the batch would overflow, then starts a new batch for texture.
    void switch2dRendering(ox::video::ITexture* texture, bool useAlphaChannel, bool linearFilter, int quads);

    //! Sets the 3D matrices and material when coming from 2D or when the material changed.
    void setRenderStates3DMode();
    //! Sets the 2D render states: identity matrices, no depth test, fog, lighting or culling, and the
    //! blending of the image or primitive.
    void setRenderStates2DMode(bool alpha, bool texture, bool alphaChannel, bool linearFilter);

    void loadExtensions();
    void createMaterialRenderers();

    // 0x431d0
    ox::core::CMatrix4 Matrizes[ox::video::ETS_COUNT];
    // 0x43290: vertex colors converted from ARGB to OpenGL's RGBA byte order
    std::vector<int> ColorBuffer;
    // 0x432a8
    E_RENDER_MODE CurrentRenderMode;
    // 0x432b0
    ox::video::ITexture* CurrentTexture[2];
    // 0x432c0
    bool ResetRenderStates;
    bool Transformation3DChanged;
    bool MultiTextureExtension;
    bool ClampTexture;
    bool StencilBuffer;
    bool ARBVertexProgramExtension;
    bool ARBFragmentProgramExtension;
    //! Never written.
    bool UnusedFlag;
    bool MultiSamplingExtension;
    bool AnisotropyExtension;
    bool ARBShadingLanguage100Extension;
    bool SeparateStencilExtension;
    bool GenerateMipmapExtension;
    bool TextureCompressionExtension;
    bool TextureNPOTExtension;
    bool FramebufferObjectExtension;
    bool PackedDepthStencilExtension;
    bool SeparateSpecularColorExtension;
    bool TextureMirroredRepeatExtension;
    // 0x432d3
    bool Fullscreen;
    bool UseMaterialShaderFor2DSet;
    bool UseMaterialShaderFor2D;

    // 0x432d8
    ox::video::SMaterial Material;
    ox::video::SMaterial LastMaterial;
    // 0x43348
    int LastSetLight;
    float MaxAnisotropy;
    int MaxTextureUnits;
    int MaxLights;
    int MaxIndices;

    // 0x43360: extension entry points, loaded with glXGetProcAddress
    PFNGLACTIVETEXTUREARBPROC pGlActiveTextureARB;
    PFNGLCLIENTACTIVETEXTUREARBPROC pGlClientActiveTextureARB;
    PFNGLGENPROGRAMSARBPROC pGlGenProgramsARB;
    PFNGLBINDPROGRAMARBPROC pGlBindProgramARB;
    PFNGLPROGRAMSTRINGARBPROC pGlProgramStringARB;
    PFNGLDELETEPROGRAMSNVPROC pGlDeleteProgramsARB;
    PFNGLPROGRAMLOCALPARAMETER4FVARBPROC pGlProgramLocalParameter4fvARB;
    PFNGLCREATESHADEROBJECTARBPROC pGlCreateShaderObjectARB;
    PFNGLSHADERSOURCEARBPROC pGlShaderSourceARB;
    PFNGLCOMPILESHADERARBPROC pGlCompileShaderARB;
    PFNGLCREATEPROGRAMOBJECTARBPROC pGlCreateProgramObjectARB;
    PFNGLATTACHOBJECTARBPROC pGlAttachObjectARB;
    PFNGLLINKPROGRAMARBPROC pGlLinkProgramARB;
    PFNGLUSEPROGRAMOBJECTARBPROC pGlUseProgramObjectARB;
    PFNGLDELETEOBJECTARBPROC pGlDeleteObjectARB;
    PFNGLGETINFOLOGARBPROC pGlGetInfoLogARB;
    PFNGLGETOBJECTPARAMETERIVARBPROC pGlGetObjectParameterivARB;
    PFNGLGETUNIFORMLOCATIONARBPROC pGlGetUniformLocationARB;
    PFNGLUNIFORM1IVARBPROC pGlUniform1ivARB;
    PFNGLUNIFORM1FVARBPROC pGlUniform1fvARB;
    PFNGLUNIFORM2FVARBPROC pGlUniform2fvARB;
    PFNGLUNIFORM3FVARBPROC pGlUniform3fvARB;
    PFNGLUNIFORM4FVARBPROC pGlUniform4fvARB;
    PFNGLUNIFORMMATRIX2FVARBPROC pGlUniformMatrix2fvARB;
    PFNGLUNIFORMMATRIX3FVARBPROC pGlUniformMatrix3fvARB;
    PFNGLUNIFORMMATRIX4FVARBPROC pGlUniformMatrix4fvARB;
    PFNGLGETACTIVEUNIFORMARBPROC pGlGetActiveUniformARB;
    PFNGLPOINTPARAMETERFARBPROC pGlPointParameterfARB;
    PFNGLPOINTPARAMETERFVARBPROC pGlPointParameterfvARB;
    PFNGLSTENCILFUNCSEPARATEPROC pGlStencilFuncSeparate;
    PFNGLSTENCILOPSEPARATEPROC pGlStencilOpSeparate;
    PFNGLSTENCILFUNCSEPARATEATIPROC pGlStencilFuncSeparateATI;
    PFNGLSTENCILOPSEPARATEATIPROC pGlStencilOpSeparateATI;
    PFNGLXSWAPINTERVALSGIPROC_ pGlxSwapIntervalSGI;
    PFNGLBINDFRAMEBUFFEREXTPROC pGlBindFramebufferEXT;
    PFNGLDELETEFRAMEBUFFERSEXTPROC pGlDeleteFramebuffersEXT;
    PFNGLGENFRAMEBUFFERSEXTPROC pGlGenFramebuffersEXT;
    PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC pGlCheckFramebufferStatusEXT;
    PFNGLFRAMEBUFFERTEXTURE2DEXTPROC pGlFramebufferTexture2DEXT;
    PFNGLBINDRENDERBUFFEREXTPROC pGlBindRenderbufferEXT;
    PFNGLDELETERENDERBUFFERSEXTPROC pGlDeleteRenderbuffersEXT;
    PFNGLGENRENDERBUFFERSEXTPROC pGlGenRenderbuffersEXT;
    PFNGLRENDERBUFFERSTORAGEEXTPROC pGlRenderbufferStorageEXT;
    PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC pGlFramebufferRenderbufferEXT;

    // 0x434c0
    ox::IOxDevice* Device;
};

//! Creates the OpenGL driver for the device's window.
ox::video::IVideoDriver* createOpenGLDriver(const ox::core::CDimension2d<int>& screenSize, ox::IOxDevice* device,
    bool fullscreen, bool stencilBuffer, ox::io::IFileSystem* io);

} // end namespace video
} // end namespace daisy

#endif
