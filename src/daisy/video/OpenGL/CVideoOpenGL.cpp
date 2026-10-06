// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CVideoOpenGL.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
//
// 2D conventions (see CVideoNull::update2dViewValues): a pixel position (x, y), y down, maps to
// clip space ((x + ViewOffsetX) * InvHalfWidth, (ViewOffsetY - y) * InvHalfHeight) with the projection
// and modelview matrices set to identity, so integer positions fall on pixel edges. Images are
// collected in the 2D quad batch (switch2dRendering) and drawn as GL_QUADS when the texture or the
// blend mode changes; rectangles and lines are drawn at once, after flushing the batch.

#include "CVideoOpenGL.h"
#include "COpenGLCGMaterialRenderer.h"
#include "COpenGLMaterialRenderer.h"
#include "COpenGLSLMaterialRenderer.h"
#include "COpenGLShaderMaterialRenderer.h"
#include "COpenGLTexture.h"
#include "daisy/os.h"
#include "daisy/video/Null/CImageLoaderJPG.h"
#include "ox/IOxDevice.h"
#include "ox/TArray.h"
#include "ox/core/CBasic.h"
#include "ox/core/CStringFunctions.h"
#include "ox/io/IFileSystem.h"
#include "ox/io/IWriteFile.h"
#include "ox/video/SColorArray.h"
#include "ox/video/SLight.h"
#include "ox/video/SMaterialInline.h"
#include <string.h>
#include <GL/glu.h>
#include <GL/glx.h>
// The native object includes an iostream static initializer.
#include <iostream> // IWYU pragma: keep

namespace daisy {
namespace video {

namespace {

//! Converts an ARGB color to the R, G, B, A byte order of GL_UNSIGNED_BYTE color arrays.
inline int toOpenGLColor(ox::video::SColor color)
{
    return ((color.color >> 16) & 0xff) | ((color.color & 0xff) << 16) | (color.color & 0xff00ff00);
}

//! Column-major copy of a matrix for glLoadMatrixf (CMatrix4 already stores it that way).
inline void createGLMatrix(GLfloat gl_matrix[16], const ox::core::CMatrix4& m)
{
    int i = 0;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            gl_matrix[i++] = m(c, r);
}

//! Irrlicht 0.7's rect::clipAgainst.
inline void clipAgainst(ox::core::CRect<int>& rect, const ox::core::CRect<int>& other)
{
    if (other.LowerRightCorner.X < rect.LowerRightCorner.X)
        rect.LowerRightCorner.X = other.LowerRightCorner.X;
    if (other.LowerRightCorner.Y < rect.LowerRightCorner.Y)
        rect.LowerRightCorner.Y = other.LowerRightCorner.Y;

    if (other.UpperLeftCorner.X > rect.UpperLeftCorner.X)
        rect.UpperLeftCorner.X = other.UpperLeftCorner.X;
    if (other.UpperLeftCorner.Y > rect.UpperLeftCorner.Y)
        rect.UpperLeftCorner.Y = other.UpperLeftCorner.Y;
}

} // end anonymous namespace

CVideoOpenGL::CVideoOpenGL(const ox::core::CDimension2d<int>& screenSize, ox::IOxDevice* device, bool fullscreen,
    bool stencilBuffer, ox::io::IFileSystem* io)
    : CVideoNull(io, screenSize), CurrentRenderMode(ERM_NONE), ResetRenderStates(true),
      Transformation3DChanged(true), MultiTextureExtension(false), StencilBuffer(stencilBuffer),
      ARBVertexProgramExtension(false), ARBFragmentProgramExtension(false), LastSetLight(-1), MaxTextureUnits(1),
      Device(device)
{
    memset(&Current2DTexture, 0, (char*)&StatusSwitches - (char*)&Current2DTexture);

    os::Printer::log("OpenGL Renderer.", ox::event::ELL_INFORMATION);

    loadExtensions();

    createMaterialRenderers();

    setFog(FogColor, LinearFog, FogStart, FogEnd, FogDensity, PixelFog, RangeFog);
}

//! Reads the extension list and loads the extension entry points (Irrlicht 1.3's extension
//! handler). The game needs only multitexturing; the Cg runtime loads its own entry points.
void CVideoOpenGL::loadExtensions()
{
    ox::core::CString<char> version((const char*)glGetString(GL_VERSION));
    ox::TArray<ox::core::CString<char> > parts;
    ox::core::splitString(parts, version, ox::core::CString<char>(" "));

    // "major.minor..."
    int major = parts[0][0] - '0';
    if (major > 1 || (parts[0][2] - '0' > 1 && major == 1))
        os::Printer::log("OpenGL driver version is 1.2 or better.", ox::event::ELL_INFORMATION);
    else
        os::Printer::log("OpenGL driver version is not 1.2 or better.", ox::event::ELL_WARNING);

    os::Printer::log((const char*)glGetString(GL_VERSION), ox::event::ELL_INFORMATION);
    os::Printer::log((const char*)glGetString(GL_VENDOR), ox::event::ELL_INFORMATION);
    os::Printer::log((const char*)glGetString(GL_RENDERER), ox::event::ELL_INFORMATION);

    const GLubyte* extensions = glGetString(GL_EXTENSIONS);

    MultiTextureExtension = false;
    MultiSamplingExtension = false;
    ARBVertexProgramExtension = false;
    ARBFragmentProgramExtension = false;
    ARBShadingLanguage100Extension = false;
    AnisotropyExtension = false;
    SeparateStencilExtension = false;
    GenerateMipmapExtension = false;
    TextureCompressionExtension = false;
    TextureNPOTExtension = false;
    FramebufferObjectExtension = false;
    PackedDepthStencilExtension = false;
    SeparateSpecularColorExtension = false;
    TextureMirroredRepeatExtension = false;

    // Original bug: meant to test for GLU 1.3 ("1.3"), but compares the characters with the
    // numbers 1 and 2, so gluCheckExtension is always used.
    const GLubyte* gluVersion = gluGetString(GLU_VERSION);
    if (gluVersion[0] > 1 || gluVersion[3] > 2)
    {
        MultiTextureExtension = gluCheckExtension((const GLubyte*)"GL_ARB_multitexture", extensions) != 0;
        MultiSamplingExtension = gluCheckExtension((const GLubyte*)"GL_ARB_multisample", extensions) != 0;
        ARBVertexProgramExtension = gluCheckExtension((const GLubyte*)"GL_ARB_vertex_program", extensions) != 0;
        ARBFragmentProgramExtension = gluCheckExtension((const GLubyte*)"GL_ARB_fragment_program", extensions) != 0;
        ARBShadingLanguage100Extension =
            gluCheckExtension((const GLubyte*)"GL_ARB_shading_language_100", extensions) != 0;
        AnisotropyExtension = gluCheckExtension((const GLubyte*)"GL_EXT_texture_filter_anisotropic", extensions) != 0;
        SeparateStencilExtension = gluCheckExtension((const GLubyte*)"GL_ATI_separate_stencil", extensions) != 0;
        SeparateStencilExtension = SeparateStencilExtension ||
            gluCheckExtension((const GLubyte*)"GL_ARB_separate_stencil", extensions) != 0;
        GenerateMipmapExtension = gluCheckExtension((const GLubyte*)"GL_SGIS_generate_mipmap", extensions) != 0;
        TextureCompressionExtension = gluCheckExtension((const GLubyte*)"GL_ARB_texture_compression", extensions) != 0;
        TextureNPOTExtension = gluCheckExtension((const GLubyte*)"GL_ARB_texture_non_power_of_two", extensions) != 0;
        FramebufferObjectExtension = gluCheckExtension((const GLubyte*)"GL_EXT_framebuffer_object", extensions) != 0;
        PackedDepthStencilExtension = gluCheckExtension((const GLubyte*)"GL_EXT_packed_depth_stencil", extensions) != 0;
        SeparateSpecularColorExtension =
            gluCheckExtension((const GLubyte*)"GL_EXT_separate_specular_color", extensions) != 0;
        TextureMirroredRepeatExtension =
            gluCheckExtension((const GLubyte*)"GL_EXT_texture_mirrored_repeat", extensions) != 0;
        TextureMirroredRepeatExtension = TextureMirroredRepeatExtension ||
            gluCheckExtension((const GLubyte*)"GL_EXT_texture_mirror_clamp", extensions) != 0;
    }
    else
    {
        int len = (int)strlen((const char*)extensions);
        char* str = new char[len + 1];
        char* p = str;

        for (int i = 0; i < len; ++i)
        {
            str[i] = (char)extensions[i];

            if (str[i] == ' ')
            {
                str[i] = 0;
                if (strstr(p, "GL_ARB_multitexture"))
                    MultiTextureExtension = true;
                else if (strstr(p, "GL_ARB_multisample"))
                    MultiSamplingExtension = true;
                else if (strstr(p, "GL_ARB_vertex_program"))
                    ARBVertexProgramExtension = true;
                else if (strstr(p, "GL_ARB_fragment_program"))
                    ARBFragmentProgramExtension = true;
                else if (strstr(p, "GL_ARB_shading_language_100"))
                    ARBShadingLanguage100Extension = true;
                else if (strstr(p, "GL_EXT_texture_filter_anisotropic"))
                    AnisotropyExtension = true;
                else if (strstr(p, "GL_ATI_separate_stencil") || strstr(p, "GL_ARB_separate_stencil"))
                    SeparateStencilExtension = true;
                else if (strstr(p, "GL_SGIS_generate_mipmap"))
                    GenerateMipmapExtension = true;
                else if (strstr(p, "GL_ARB_texture_compression"))
                    TextureCompressionExtension = true;
                else if (strstr(p, "GL_ARB_texture_non_power_of_two"))
                    TextureNPOTExtension = true;
                else if (strstr(p, "GL_EXT_framebuffer_object"))
                    FramebufferObjectExtension = true;
                else if (strstr(p, "GL_EXT_packed_depth_stencil"))
                    PackedDepthStencilExtension = true;
                else if (strstr(p, "GL_EXT_separate_specular_color"))
                    SeparateSpecularColorExtension = true;
                else if (strstr(p, "GL_EXT_texture_mirrored_repeat") || strstr(p, "GL_EXT_texture_mirror_clamp"))
                    TextureMirroredRepeatExtension = true;

                p = p + strlen(p) + 1;
            }
        }

        delete[] str;
    }

    if (MultiTextureExtension)
    {
        // The binary asks the device (CIrrDeviceLinux) for the display; its helper returns
        // XOpenDisplay(0), a new connection that is never closed.
        Display* display = XOpenDisplay(0);
        int major = 0;
        int minor = 0;
        glXQueryVersion(display, &major, &minor);

        __GLXextFuncPtr (*load)(const GLubyte*) = glXGetProcAddressARB;
        if (major > 1 || minor > 3)
            load = glXGetProcAddress;

        pGlActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)load((const GLubyte*)"glActiveTextureARB");
        pGlClientActiveTextureARB = (PFNGLCLIENTACTIVETEXTUREARBPROC)load((const GLubyte*)"glClientActiveTextureARB");
        pGlGenProgramsARB = (PFNGLGENPROGRAMSARBPROC)load((const GLubyte*)"glGenProgramsARB");
        pGlBindProgramARB = (PFNGLBINDPROGRAMARBPROC)load((const GLubyte*)"glBindProgramARB");
        pGlProgramStringARB = (PFNGLPROGRAMSTRINGARBPROC)load((const GLubyte*)"glProgramStringARB");
        pGlDeleteProgramsARB = (PFNGLDELETEPROGRAMSNVPROC)load((const GLubyte*)"glDeleteProgramsARB");
        pGlProgramLocalParameter4fvARB =
            (PFNGLPROGRAMLOCALPARAMETER4FVARBPROC)load((const GLubyte*)"glProgramLocalParameter4fvARB");
        pGlCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC)load((const GLubyte*)"glCreateShaderObjectARB");
        pGlShaderSourceARB = (PFNGLSHADERSOURCEARBPROC)load((const GLubyte*)"glShaderSourceARB");
        pGlCompileShaderARB = (PFNGLCOMPILESHADERARBPROC)load((const GLubyte*)"glCompileShaderARB");
        pGlCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC)load((const GLubyte*)"glCreateProgramObjectARB");
        pGlAttachObjectARB = (PFNGLATTACHOBJECTARBPROC)load((const GLubyte*)"glAttachObjectARB");
        pGlLinkProgramARB = (PFNGLLINKPROGRAMARBPROC)load((const GLubyte*)"glLinkProgramARB");
        pGlUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC)load((const GLubyte*)"glUseProgramObjectARB");
        pGlDeleteObjectARB = (PFNGLDELETEOBJECTARBPROC)load((const GLubyte*)"glDeleteObjectARB");
        pGlGetInfoLogARB = (PFNGLGETINFOLOGARBPROC)load((const GLubyte*)"glGetInfoLogARB");
        pGlGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC)load((const GLubyte*)"glGetObjectParameterivARB");
        pGlGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC)load((const GLubyte*)"glGetUniformLocationARB");
        pGlUniform4fvARB = (PFNGLUNIFORM4FVARBPROC)load((const GLubyte*)"glUniform4fvARB");
        pGlUniform1ivARB = (PFNGLUNIFORM1IVARBPROC)load((const GLubyte*)"glUniform1ivARB");
        pGlUniform1fvARB = (PFNGLUNIFORM1FVARBPROC)load((const GLubyte*)"glUniform1fvARB");
        pGlUniform2fvARB = (PFNGLUNIFORM2FVARBPROC)load((const GLubyte*)"glUniform2fvARB");
        pGlUniform3fvARB = (PFNGLUNIFORM3FVARBPROC)load((const GLubyte*)"glUniform3fvARB");
        pGlUniform4fvARB = (PFNGLUNIFORM4FVARBPROC)load((const GLubyte*)"glUniform4fvARB");
        pGlUniformMatrix2fvARB = (PFNGLUNIFORMMATRIX2FVARBPROC)load((const GLubyte*)"glUniformMatrix2fvARB");
        pGlUniformMatrix3fvARB = (PFNGLUNIFORMMATRIX3FVARBPROC)load((const GLubyte*)"glUniformMatrix3fvARB");
        pGlUniformMatrix4fvARB = (PFNGLUNIFORMMATRIX4FVARBPROC)load((const GLubyte*)"glUniformMatrix4fvARB");
        pGlGetActiveUniformARB = (PFNGLGETACTIVEUNIFORMARBPROC)load((const GLubyte*)"glGetActiveUniformARB");
        pGlPointParameterfARB = (PFNGLPOINTPARAMETERFARBPROC)load((const GLubyte*)"glPointParameterfARB");
        pGlPointParameterfvARB = (PFNGLPOINTPARAMETERFVARBPROC)load((const GLubyte*)"glPointParameterfvARB");
        pGlStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC)load((const GLubyte*)"glStencilFuncSeparate");
        pGlStencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)load((const GLubyte*)"glStencilOpSeparate");
        pGlStencilFuncSeparateATI = (PFNGLSTENCILFUNCSEPARATEATIPROC)load((const GLubyte*)"glStencilFuncSeparateATI");
        pGlStencilOpSeparateATI = (PFNGLSTENCILOPSEPARATEATIPROC)load((const GLubyte*)"glStencilOpSeparateATI");
        pGlxSwapIntervalSGI = (PFNGLXSWAPINTERVALSGIPROC_)load((const GLubyte*)"glXSwapIntervalSGI");
        pGlBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)load((const GLubyte*)"glBindFramebufferEXT");
        pGlDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)load((const GLubyte*)"glDeleteFramebuffersEXT");
        pGlGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)load((const GLubyte*)"glGenFramebuffersEXT");
        pGlCheckFramebufferStatusEXT =
            (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)load((const GLubyte*)"glCheckFramebufferStatusEXT");
        pGlFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)load((const GLubyte*)"glFramebufferTexture2DEXT");
        pGlBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)load((const GLubyte*)"glBindRenderbufferEXT");
        pGlDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC)load((const GLubyte*)"glDeleteRenderbuffersEXT");
        pGlGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)load((const GLubyte*)"glGenRenderbuffersEXT");
        pGlRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)load((const GLubyte*)"glRenderbufferStorageEXT");
        pGlFramebufferRenderbufferEXT =
            (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)load((const GLubyte*)"glFramebufferRenderbufferEXT");

        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &MaxTextureUnits);
        glGetIntegerv(GL_MAX_LIGHTS, &MaxLights);
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &MaxAnisotropy);
    }

    // Original bug: the entry points are only loaded with multitexturing and the constructor never
    // clears them, so without GL_ARB_multitexture this tests uninitialized memory.
    if (!pGlActiveTextureARB || !pGlClientActiveTextureARB)
    {
        MultiTextureExtension = false;
        os::Printer::log("Failed to load OpenGL's multitexture extension, proceeding without.", ox::event::ELL_WARNING);
    }
    else if (MaxTextureUnits < 2)
    {
        MultiTextureExtension = false;
        os::Printer::log("Warning: OpenGL device only has one texture unit. Disabling multitexturing.",
            ox::event::ELL_WARNING);
    }

    // at most two texture units are used
    MaxTextureUnits = (unsigned int)MaxTextureUnits > (unsigned int)ox::video::MATERIAL_MAX_TEXTURES ?
        ox::video::MATERIAL_MAX_TEXTURES : MaxTextureUnits;

    glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &MaxIndices);
}

//! The material renderers in E_MATERIAL_TYPE order; the seven lightmap types share one renderer.
void CVideoOpenGL::createMaterialRenderers()
{
    addAndDropMaterialRenderer(new COpenGLMaterialRenderer_SOLID(this));
    addAndDropMaterialRenderer(new COpenGLMaterialRenderer_SOLID_2_LAYER(this));

    // add the same renderer for all lightmap types
    COpenGLMaterialRenderer_LIGHTMAP* lmr = new COpenGLMaterialRenderer_LIGHTMAP(this);
    addMaterialRenderer(lmr, 0); // for EMT_LIGHTMAP:
    addMaterialRenderer(lmr, 0); // for EMT_LIGHTMAP_ADD:
    addMaterialRenderer(lmr, 0); // for EMT_LIGHTMAP_M2:
    addMaterialRenderer(lmr, 0); // for EMT_LIGHTMAP_M4:
    addMaterialRenderer(lmr, 0); // for EMT_LIGHTMAP_LIGHTING:
    addMaterialRenderer(lmr, 0); // for EMT_LIGHTMAP_LIGHTING_M2:
    addMaterialRenderer(lmr, 0); // for EMT_LIGHTMAP_LIGHTING_M4:
    lmr->drop();

    // add remaining material renderers
    addAndDropMaterialRenderer(new COpenGLMaterialRenderer_SPHERE_MAP(this));
    addAndDropMaterialRenderer(new COpenGLMaterialRenderer_REFLECTION_2_LAYER(this));
    addAndDropMaterialRenderer(new COpenGLMaterialRenderer_TRANSPARENT_ADD_COLOR(this));
    addAndDropMaterialRenderer(new COpenGLMaterialRenderer_TRANSPARENT_ALPHA_CHANNEL(this));
    addAndDropMaterialRenderer(new COpenGLMaterialRenderer_TRANSPARENT_VERTEX_ALPHA(this));
    addAndDropMaterialRenderer(new COpenGLMaterialRenderer_TRANSPARENT_REFLECTION_2_LAYER(this));
}

CVideoOpenGL::~CVideoOpenGL()
{
    deleteAllTextures();
}

//! Presents the frame through the device.
bool CVideoOpenGL::endScene()
{
    CVideoNull::endScene();

    return Device->swapBuffers();
}

bool CVideoOpenGL::beginScene(bool backBuffer, bool zBuffer, ox::video::SColor color)
{
    CVideoNull::beginScene(backBuffer, zBuffer, color);

    GLbitfield mask = 0;

    if (backBuffer)
    {
        const float inv = 1.0f / 255.0f;
        glClearColor(color.getRed() * inv, color.getGreen() * inv, color.getBlue() * inv, color.getAlpha() * inv);

        mask |= GL_COLOR_BUFFER_BIT;
    }

    if (zBuffer)
    {
        glDepthMask(GL_TRUE);
        mask |= GL_DEPTH_BUFFER_BIT;
    }

    glClear(mask);
    return true;
}

//! Clears the color buffer (and the depth buffer if zBuffer) in the middle of a frame.
void CVideoOpenGL::clearScreen(bool zBuffer, ox::video::SColor color)
{
    const float inv = 1.0f / 255.0f;
    glClearColor(color.getRed() * inv, color.getGreen() * inv, color.getBlue() * inv, color.getAlpha() * inv);

    GLbitfield mask = GL_COLOR_BUFFER_BIT;

    if (zBuffer)
    {
        glDepthMask(GL_TRUE);
        mask |= GL_DEPTH_BUFFER_BIT;
    }

    glClear(mask);
}

ox::core::CMatrix4 CVideoOpenGL::getTransform(ox::video::E_TRANSFORMATION_STATE state)
{
    return Matrizes[state];
}

//! OpenGL has one modelview matrix, so view and world are multiplied (view * world). The
//! projection's z is flipped to compensate for OpenGL's right-handed coordinate system.
void CVideoOpenGL::setTransform(ox::video::E_TRANSFORMATION_STATE state, const ox::core::CMatrix4& mat)
{
    Transformation3DChanged = true;

    GLfloat glmat[16];
    Matrizes[state] = mat;

    switch (state)
    {
    case ox::video::ETS_VIEW:
        createGLMatrix(glmat, Matrizes[ox::video::ETS_VIEW] * Matrizes[ox::video::ETS_WORLD]);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(glmat);
        break;
    case ox::video::ETS_WORLD:
        createGLMatrix(glmat, Matrizes[ox::video::ETS_VIEW] * Matrizes[ox::video::ETS_WORLD]);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(glmat);
        break;
    case ox::video::ETS_PROJECTION:
        {
            createGLMatrix(glmat, mat);

            // flip z to compensate OpenGLs right-hand coordinate system
            glmat[12] *= -1.0f;

            glMatrixMode(GL_PROJECTION);
            glLoadMatrixf(glmat);
        }
        break;
    default:
        break;
    }
}

void CVideoOpenGL::drawIndexedTriangleList(const ox::video::S3DVertex* vertices, int vertexCount,
    const unsigned short* indexList, int triangleCount)
{
    if (!checkPrimitiveCount(vertexCount))
        return;

    CVideoNull::drawIndexedTriangleList(vertices, vertexCount, indexList, triangleCount);

    setRenderStates3DMode();

    extGlClientActiveTextureARB(GL_TEXTURE0_ARB);

    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    // convert colors to gl color format.

    const ox::video::S3DVertex* p = vertices;
    ColorBuffer.resize(vertexCount);
    for (int i = 0; i < vertexCount; ++i)
    {
        ColorBuffer[i] = toOpenGLColor(p->Color);
        ++p;
    }

    // draw everything

    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ox::video::SColor), &ColorBuffer[0]);
    glNormalPointer(GL_FLOAT, sizeof(ox::video::S3DVertex), &vertices[0].Normal);
    glTexCoordPointer(2, GL_FLOAT, sizeof(ox::video::S3DVertex), &vertices[0].TCoords);
    glVertexPointer(3, GL_FLOAT, sizeof(ox::video::S3DVertex), &vertices[0].Pos);

    glDrawElements(GL_TRIANGLES, triangleCount * 3, GL_UNSIGNED_SHORT, indexList);

    glFlush();

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
}

//! Sets the 3D matrices when coming from 2D, then lets the material renderers switch: the old
//! one is unset when the type changes, the new one is set when anything in the material changed,
//! and its OnRender runs before every draw call.
void CVideoOpenGL::setRenderStates3DMode()
{
    if (CurrentRenderMode != ERM_3D)
    {
        // switch back the matrices
        GLfloat glmat[16];

        createGLMatrix(glmat, Matrizes[ox::video::ETS_VIEW] * Matrizes[ox::video::ETS_WORLD]);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(glmat);

        createGLMatrix(glmat, Matrizes[ox::video::ETS_PROJECTION]);
        glmat[12] *= -1.0f;
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(glmat);

        ResetRenderStates = true;
    }

    if (ResetRenderStates)
        glFrontFace(Material.FrontFaceCCW ? GL_CCW : GL_CW);

    if (ResetRenderStates || LastMaterial != Material)
    {
        // unset old material

        if (LastMaterial.MaterialType != Material.MaterialType && LastMaterial.MaterialType >= 0 &&
            LastMaterial.MaterialType < (int)MaterialRenderers.size())
            MaterialRenderers[LastMaterial.MaterialType].Renderer->OnUnsetMaterial();

        // set new material.

        if (Material.MaterialType >= 0 && Material.MaterialType < (int)MaterialRenderers.size())
            MaterialRenderers[Material.MaterialType].Renderer->OnSetMaterial(Material, LastMaterial,
                ResetRenderStates, this);
    }

    LastMaterial = Material;

    ResetRenderStates = false;

    if (Material.MaterialType >= 0 && Material.MaterialType < (int)MaterialRenderers.size())
        MaterialRenderers[Material.MaterialType].Renderer->OnRender(this, ox::video::EVT_STANDARD);

    CurrentRenderMode = ERM_3D;
}

void CVideoOpenGL::extGlClientActiveTextureARB(GLenum texture)
{
    if (MultiTextureExtension && pGlClientActiveTextureARB)
        pGlClientActiveTextureARB(texture);
}

void CVideoOpenGL::drawIndexedTriangleList(const ox::video::S3DVertex2TCoords* vertices, int vertexCount,
    const unsigned short* indexList, int triangleCount)
{
    if (!checkPrimitiveCount(vertexCount))
        return;

    CVideoNull::drawIndexedTriangleList(vertices, vertexCount, indexList, triangleCount);

    setRenderStates3DMode();

    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    // convert colors to gl color format.

    const ox::video::S3DVertex2TCoords* p = vertices;
    ColorBuffer.resize(vertexCount);
    for (int i = 0; i < vertexCount; ++i)
    {
        ColorBuffer[i] = toOpenGLColor(p->Color);
        ++p;
    }

    // draw everything

    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ox::video::SColor), &ColorBuffer[0]);
    glNormalPointer(GL_FLOAT, sizeof(ox::video::S3DVertex2TCoords), &vertices[0].Normal);
    glVertexPointer(3, GL_FLOAT, sizeof(ox::video::S3DVertex2TCoords), &vertices[0].Pos);

    // texture coordiantes
    if (MultiTextureExtension)
    {
        extGlClientActiveTextureARB(GL_TEXTURE0_ARB);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, sizeof(ox::video::S3DVertex2TCoords), &vertices[0].TCoords);

        extGlClientActiveTextureARB(GL_TEXTURE1_ARB);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, sizeof(ox::video::S3DVertex2TCoords), &vertices[0].TCoords2);
    }
    else
        glTexCoordPointer(2, GL_FLOAT, sizeof(ox::video::S3DVertex2TCoords), &vertices[0].TCoords);

    glDrawElements(GL_TRIANGLES, triangleCount * 3, GL_UNSIGNED_SHORT, indexList);

    glFlush();

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    if (MultiTextureExtension)
    {
        extGlClientActiveTextureARB(GL_TEXTURE0_ARB);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);

        extGlClientActiveTextureARB(GL_TEXTURE1_ARB);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    else
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    glDisableClientState(GL_NORMAL_ARRAY);
}

void CVideoOpenGL::drawIndexedTriangleFan(const ox::video::S3DVertex* vertices, int vertexCount,
    const unsigned short* indexList, int triangleCount)
{
    if (!checkPrimitiveCount(vertexCount))
        return;

    CVideoNull::drawIndexedTriangleFan(vertices, vertexCount, indexList, triangleCount);

    setRenderStates3DMode();

    extGlClientActiveTextureARB(GL_TEXTURE0_ARB);

    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    // convert colors to gl color format.

    const ox::video::S3DVertex* p = vertices;
    ColorBuffer.resize(vertexCount);
    for (int i = 0; i < vertexCount; ++i)
    {
        ColorBuffer[i] = toOpenGLColor(p->Color);
        ++p;
    }

    // draw everything

    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ox::video::SColor), &ColorBuffer[0]);
    glNormalPointer(GL_FLOAT, sizeof(ox::video::S3DVertex), &vertices[0].Normal);
    glTexCoordPointer(2, GL_FLOAT, sizeof(ox::video::S3DVertex), &vertices[0].TCoords);
    glVertexPointer(3, GL_FLOAT, sizeof(ox::video::S3DVertex), &vertices[0].Pos);

    glDrawElements(GL_TRIANGLE_FAN, triangleCount + 2, GL_UNSIGNED_SHORT, indexList);

    glFlush();

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
}

void CVideoOpenGL::drawIndexedTriangleFan(const ox::video::S3DVertex2TCoords* vertices, int vertexCount,
    const unsigned short* indexList, int triangleCount)
{
    if (!checkPrimitiveCount(vertexCount))
        return;

    CVideoNull::drawIndexedTriangleFan(vertices, vertexCount, indexList, triangleCount);

    setRenderStates3DMode();

    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    // convert colors to gl color format.

    const ox::video::S3DVertex2TCoords* p = vertices;
    ColorBuffer.resize(vertexCount);
    for (int i = 0; i < vertexCount; ++i)
    {
        ColorBuffer[i] = toOpenGLColor(p->Color);
        ++p;
    }

    // draw everything

    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ox::video::SColor), &ColorBuffer[0]);
    glNormalPointer(GL_FLOAT, sizeof(ox::video::S3DVertex2TCoords), &vertices[0].Normal);
    glVertexPointer(3, GL_FLOAT, sizeof(ox::video::S3DVertex2TCoords), &vertices[0].Pos);

    // texture coordiantes
    if (MultiTextureExtension)
    {
        extGlClientActiveTextureARB(GL_TEXTURE0_ARB);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, sizeof(ox::video::S3DVertex2TCoords), &vertices[0].TCoords);

        extGlClientActiveTextureARB(GL_TEXTURE1_ARB);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, sizeof(ox::video::S3DVertex2TCoords), &vertices[0].TCoords2);
    }
    else
        glTexCoordPointer(2, GL_FLOAT, sizeof(ox::video::S3DVertex2TCoords), &vertices[0].TCoords);

    glDrawElements(GL_TRIANGLE_FAN, triangleCount + 2, GL_UNSIGNED_SHORT, indexList);

    glFlush();

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    if (MultiTextureExtension)
    {
        extGlClientActiveTextureARB(GL_TEXTURE0_ARB);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);

        extGlClientActiveTextureARB(GL_TEXTURE1_ARB);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    else
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    glDisableClientState(GL_NORMAL_ARRAY);
}

//! Draws the whole texture (its original size) at destPos, opaque white, without its alpha channel.
void CVideoOpenGL::draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<int>& destPos)
{
    if (!texture)
        return;

    draw2DImage(texture, destPos,
        ox::core::CRect<int>(ox::core::CPosition2d<int>(0, 0), texture->getOriginalSize()), 0,
        ox::video::SColor(0xffffffff), false);
}

//! Draws sourceRect of the texture at pos (1:1 pixels), tinted by color, clipped to clipRect and
//! the screen. Texture coordinates are inset by half a texel on every side; positions are not
//! offset, so the image covers exactly the destination pixels. Nearest neighbour filtering.
void CVideoOpenGL::draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<int>& pos,
    const ox::core::CRect<int>& sourceRect, const ox::core::CRect<int>* clipRect, ox::video::SColor color,
    bool useAlphaChannelOfTexture)
{
    if (!texture)
        return;

    if (sourceRect.UpperLeftCorner.X >= sourceRect.LowerRightCorner.X ||
        sourceRect.UpperLeftCorner.Y >= sourceRect.LowerRightCorner.Y)
        return;

    ox::core::CPosition2d<int> targetPos = pos;
    ox::core::CPosition2d<int> sourcePos = sourceRect.UpperLeftCorner;
    ox::core::CDimension2d<int> sourceSize(sourceRect.getWidth(), sourceRect.getHeight());
    const ox::core::CDimension2d<int> targetSurfaceSize = ScreenSize;

    if (clipRect)
    {
        if (targetPos.X < clipRect->UpperLeftCorner.X)
        {
            sourceSize.Width += targetPos.X - clipRect->UpperLeftCorner.X;
            if (sourceSize.Width <= 0)
                return;

            sourcePos.X -= targetPos.X - clipRect->UpperLeftCorner.X;
            targetPos.X = clipRect->UpperLeftCorner.X;
        }

        if (targetPos.X + sourceSize.Width > clipRect->LowerRightCorner.X)
        {
            sourceSize.Width -= (targetPos.X + sourceSize.Width) - clipRect->LowerRightCorner.X;
            if (sourceSize.Width <= 0)
                return;
        }

        if (targetPos.Y < clipRect->UpperLeftCorner.Y)
        {
            sourceSize.Height += targetPos.Y - clipRect->UpperLeftCorner.Y;
            if (sourceSize.Height <= 0)
                return;

            sourcePos.Y -= targetPos.Y - clipRect->UpperLeftCorner.Y;
            targetPos.Y = clipRect->UpperLeftCorner.Y;
        }

        if (targetPos.Y + sourceSize.Height > clipRect->LowerRightCorner.Y)
        {
            sourceSize.Height -= (targetPos.Y + sourceSize.Height) - clipRect->LowerRightCorner.Y;
            if (sourceSize.Height <= 0)
                return;
        }
    }

    // clip these coordinates

    if (targetPos.X < 0)
    {
        sourceSize.Width += targetPos.X;
        if (sourceSize.Width <= 0)
            return;

        sourcePos.X -= targetPos.X;
        targetPos.X = 0;
    }

    if (targetPos.X + sourceSize.Width > targetSurfaceSize.Width)
    {
        sourceSize.Width -= (targetPos.X + sourceSize.Width) - targetSurfaceSize.Width;
        if (sourceSize.Width <= 0)
            return;
    }

    if (targetPos.Y < 0)
    {
        sourceSize.Height += targetPos.Y;
        if (sourceSize.Height <= 0)
            return;

        sourcePos.Y -= targetPos.Y;
        targetPos.Y = 0;
    }

    if (targetPos.Y + sourceSize.Height > targetSurfaceSize.Height)
    {
        sourceSize.Height -= (targetPos.Y + sourceSize.Height) - targetSurfaceSize.Height;
        if (sourceSize.Height <= 0)
            return;
    }

    // ok, we've clipped everything.
    // now draw it.

    switch2dRendering(texture, useAlphaChannelOfTexture, false, 1);

    ox::core::CRect<int> poss(targetPos, sourceSize);

    const float xFact = InvHalfWidth;
    const float yFact = InvHalfHeight;
    const int xPlus = ViewOffsetX;
    const int yPlus = ViewOffsetY;

    const ox::core::CDimension2d<int>& ss = texture->getOriginalSize();
    ox::core::CRect<float> tcoords;
    tcoords.UpperLeftCorner.X = ((float)sourcePos.X + 0.5f) / ss.Width;
    tcoords.UpperLeftCorner.Y = ((float)sourcePos.Y + 0.5f) / ss.Height;
    tcoords.LowerRightCorner.X = ((float)(sourcePos.X + sourceSize.Width) - 0.5f) / ss.Width;
    tcoords.LowerRightCorner.Y = ((float)(sourcePos.Y + sourceSize.Height) - 0.5f) / ss.Height;

    ox::core::CRect<float> npos;
    npos.UpperLeftCorner.X = (float)(poss.UpperLeftCorner.X + xPlus) * xFact;
    npos.UpperLeftCorner.Y = (float)(yPlus - poss.UpperLeftCorner.Y) * yFact;
    npos.LowerRightCorner.X = (float)(poss.LowerRightCorner.X + xPlus) * xFact;
    npos.LowerRightCorner.Y = (float)(yPlus - poss.LowerRightCorner.Y) * yFact;

    ox::video::S3DVertex* v = &Vertices2D[Current2DQuadCount * 4];
    v[0] = ox::video::S3DVertex(npos.UpperLeftCorner.X, npos.UpperLeftCorner.Y, 0, 0, 0, 0, color,
        tcoords.UpperLeftCorner.X, tcoords.UpperLeftCorner.Y);
    v[1] = ox::video::S3DVertex(npos.LowerRightCorner.X, npos.UpperLeftCorner.Y, 0, 0, 0, 0, color,
        tcoords.LowerRightCorner.X, tcoords.UpperLeftCorner.Y);
    v[2] = ox::video::S3DVertex(npos.LowerRightCorner.X, npos.LowerRightCorner.Y, 0, 0, 0, 0, color,
        tcoords.LowerRightCorner.X, tcoords.LowerRightCorner.Y);
    v[3] = ox::video::S3DVertex(npos.UpperLeftCorner.X, npos.LowerRightCorner.Y, 0, 0, 0, 0, color,
        tcoords.UpperLeftCorner.X, tcoords.LowerRightCorner.Y);
    ++Current2DQuadCount;
}

//! Draws the quads of the batch when texture, useAlphaChannel or linearFilter differ from the
//! batch's, or when quads more would not fit, and starts a new batch. Every textured quad is drawn
//! with blending on (see setRenderStates2DMode).
void CVideoOpenGL::switch2dRendering(ox::video::ITexture* texture, bool useAlphaChannel, bool linearFilter, int quads)
{
    if (Current2DTexture != texture || Current2DAlphaChannel != useAlphaChannel ||
        Current2DFlag2 != linearFilter || Current2DQuadCount + quads >= MAX_2D_QUADS)
    {
        // no effect: quads are only batched with a texture
        if (!Current2DTexture)
            Current2DTexture = texture;

        if (Current2DQuadCount > 0)
        {
            setTexture(0, Current2DTexture);
            setRenderStates2DMode(true, true, Current2DAlphaChannel, Current2DFlag2);

            glBegin(GL_QUADS);

            for (int i = 0; i < Current2DQuadCount * 4; i += 4)
            {
                for (int j = 0; j < 4; ++j)
                {
                    const ox::video::S3DVertex& vertex = Vertices2D[i + j];
                    glColor4ub(vertex.Color.getRed(), vertex.Color.getGreen(), vertex.Color.getBlue(),
                        vertex.Color.getAlpha());
                    glTexCoord2f(vertex.TCoords.X, vertex.TCoords.Y);
                    glVertex2f(vertex.Pos.X, vertex.Pos.Y);
                }
            }

            glEnd();
            ++StatusSwitches;
        }

        Current2DTexture = texture;
        Current2DAlphaChannel = useAlphaChannel;
        Current2DQuadCount = 0;
        Current2DFlag2 = linearFilter;
    }
}

//! Like the color version, with a color per corner (colors[0] upper left, [3] upper right,
//! [2] lower right, [1] lower left; opaque white if colors is 0). Texture coordinates and positions
//! are shifted by half a texel and half a pixel down and right (Irrlicht 0.7's convention).
void CVideoOpenGL::draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<int>& pos,
    const ox::core::CRect<int>& sourceRect, const ox::core::CRect<int>* clipRect, ox::video::SColor* colors,
    bool useAlphaChannelOfTexture)
{
    if (!texture)
        return;

    if (sourceRect.UpperLeftCorner.X >= sourceRect.LowerRightCorner.X ||
        sourceRect.UpperLeftCorner.Y >= sourceRect.LowerRightCorner.Y)
        return;

    ox::core::CPosition2d<int> targetPos = pos;
    ox::core::CPosition2d<int> sourcePos = sourceRect.UpperLeftCorner;
    ox::core::CDimension2d<int> sourceSize(sourceRect.getWidth(), sourceRect.getHeight());
    const ox::core::CDimension2d<int> targetSurfaceSize = ScreenSize;

    if (clipRect)
    {
        if (targetPos.X < clipRect->UpperLeftCorner.X)
        {
            sourceSize.Width += targetPos.X - clipRect->UpperLeftCorner.X;
            if (sourceSize.Width <= 0)
                return;

            sourcePos.X -= targetPos.X - clipRect->UpperLeftCorner.X;
            targetPos.X = clipRect->UpperLeftCorner.X;
        }

        if (targetPos.X + sourceSize.Width > clipRect->LowerRightCorner.X)
        {
            sourceSize.Width -= (targetPos.X + sourceSize.Width) - clipRect->LowerRightCorner.X;
            if (sourceSize.Width <= 0)
                return;
        }

        if (targetPos.Y < clipRect->UpperLeftCorner.Y)
        {
            sourceSize.Height += targetPos.Y - clipRect->UpperLeftCorner.Y;
            if (sourceSize.Height <= 0)
                return;

            sourcePos.Y -= targetPos.Y - clipRect->UpperLeftCorner.Y;
            targetPos.Y = clipRect->UpperLeftCorner.Y;
        }

        if (targetPos.Y + sourceSize.Height > clipRect->LowerRightCorner.Y)
        {
            sourceSize.Height -= (targetPos.Y + sourceSize.Height) - clipRect->LowerRightCorner.Y;
            if (sourceSize.Height <= 0)
                return;
        }
    }

    // clip these coordinates

    if (targetPos.X < 0)
    {
        sourceSize.Width += targetPos.X;
        if (sourceSize.Width <= 0)
            return;

        sourcePos.X -= targetPos.X;
        targetPos.X = 0;
    }

    if (targetPos.X + sourceSize.Width > targetSurfaceSize.Width)
    {
        sourceSize.Width -= (targetPos.X + sourceSize.Width) - targetSurfaceSize.Width;
        if (sourceSize.Width <= 0)
            return;
    }

    if (targetPos.Y < 0)
    {
        sourceSize.Height += targetPos.Y;
        if (sourceSize.Height <= 0)
            return;

        sourcePos.Y -= targetPos.Y;
        targetPos.Y = 0;
    }

    if (targetPos.Y + sourceSize.Height > targetSurfaceSize.Height)
    {
        sourceSize.Height -= (targetPos.Y + sourceSize.Height) - targetSurfaceSize.Height;
        if (sourceSize.Height <= 0)
            return;
    }

    switch2dRendering(texture, useAlphaChannelOfTexture, false, 1);

    bool tempColors = false;

    if (!colors)
    {
        colors = new ox::video::SColor[4];
        for (int i = 0; i < 4; ++i)
            colors[i] = ox::video::SColor(0xffffffff);
        tempColors = true;
    }

    ox::core::CRect<int> poss(targetPos, sourceSize);

    const float xFact = InvHalfWidth;
    const float yFact = InvHalfHeight;
    const int xPlus = ViewOffsetX;
    const int yPlus = ViewOffsetY;

    const ox::core::CDimension2d<int>& ss = texture->getOriginalSize();
    ox::core::CRect<float> tcoords;
    tcoords.UpperLeftCorner.X = ((float)sourcePos.X + 0.5f) / ss.Width;
    tcoords.UpperLeftCorner.Y = ((float)sourcePos.Y + 0.5f) / ss.Height;
    tcoords.LowerRightCorner.X = ((float)(sourcePos.X + sourceSize.Width) + 0.5f) / ss.Width;
    tcoords.LowerRightCorner.Y = ((float)(sourcePos.Y + sourceSize.Height) + 0.5f) / ss.Height;

    ox::core::CRect<float> npos;
    npos.UpperLeftCorner.X = ((float)(poss.UpperLeftCorner.X + xPlus) + 0.5f) * xFact;
    npos.UpperLeftCorner.Y = ((float)(yPlus - poss.UpperLeftCorner.Y) + 0.5f) * yFact;
    npos.LowerRightCorner.X = ((float)(poss.LowerRightCorner.X + xPlus) + 0.5f) * xFact;
    npos.LowerRightCorner.Y = ((float)(yPlus - poss.LowerRightCorner.Y) + 0.5f) * yFact;

    ox::video::S3DVertex* v = &Vertices2D[Current2DQuadCount * 4];
    v[0] = ox::video::S3DVertex(npos.UpperLeftCorner.X, npos.UpperLeftCorner.Y, 0, 0, 0, 0, colors[0],
        tcoords.UpperLeftCorner.X, tcoords.UpperLeftCorner.Y);
    v[1] = ox::video::S3DVertex(npos.LowerRightCorner.X, npos.UpperLeftCorner.Y, 0, 0, 0, 0, colors[3],
        tcoords.LowerRightCorner.X, tcoords.UpperLeftCorner.Y);
    v[2] = ox::video::S3DVertex(npos.LowerRightCorner.X, npos.LowerRightCorner.Y, 0, 0, 0, 0, colors[2],
        tcoords.LowerRightCorner.X, tcoords.LowerRightCorner.Y);
    v[3] = ox::video::S3DVertex(npos.UpperLeftCorner.X, npos.LowerRightCorner.Y, 0, 0, 0, 0, colors[1],
        tcoords.UpperLeftCorner.X, tcoords.LowerRightCorner.Y);
    ++Current2DQuadCount;

    if (tempColors)
        delete[] colors;
}

//! Draws sourceRect scaled by scale with its upper left corner at destPos, through the float
//! corner version (so with linear filtering).
void CVideoOpenGL::drawScaled2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<float>& destPos,
    const ox::core::CRect<int>& sourceRect, float scale, ox::video::SColor color, bool useAlphaChannelOfTexture)
{
    if (!texture)
        return;

    float width = sourceRect.getWidth() * scale;
    float height = sourceRect.getHeight() * scale;

    ox::core::CPosition2d<float> upperLeft(destPos.X, destPos.Y);
    ox::core::CPosition2d<float> upperRight(destPos.X + width, destPos.Y);
    ox::core::CPosition2d<float> lowerLeft(destPos.X, destPos.Y + height);
    ox::core::CPosition2d<float> lowerRight(destPos.X + width, destPos.Y + height);

    ox::video::SColorArray* colors = new ox::video::SColorArray(color);
    draw2DImage(texture, upperLeft, upperRight, lowerLeft, lowerRight, sourceRect, colors, useAlphaChannelOfTexture);
    delete colors;
}

//! Draws sourceRect into the quad corner1 (upper left), corner2 (upper right), corner3 (lower
//! left), corner4 (lower right). Texture coordinates are inset by half a texel; positions are
//! shifted by half a pixel. Linear filtering only when the quad is an axis-aligned rectangle.
//! colors defaults to transparent white (0x00ffffff).
void CVideoOpenGL::draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<int>& corner1,
    const ox::core::CPosition2d<int>& corner2, const ox::core::CPosition2d<int>& corner3,
    const ox::core::CPosition2d<int>& corner4, const ox::core::CRect<int>& sourceRect, ox::video::SColor* colors,
    bool useAlphaChannelOfTexture)
{
    if (!texture)
        return;

    const ox::core::CDimension2d<int>& ss = texture->getOriginalSize();
    ox::core::CRect<float> tcoords;
    tcoords.UpperLeftCorner.X = ((float)sourceRect.UpperLeftCorner.X + 0.5f) / ss.Width;
    tcoords.LowerRightCorner.X = ((float)sourceRect.LowerRightCorner.X - 0.5f) / ss.Width;
    tcoords.UpperLeftCorner.Y = ((float)sourceRect.UpperLeftCorner.Y + 0.5f) / ss.Height;
    tcoords.LowerRightCorner.Y = ((float)sourceRect.LowerRightCorner.Y - 0.5f) / ss.Height;

    const float xFact = InvHalfWidth;
    const float yFact = InvHalfHeight;
    const int xPlus = ViewOffsetX;
    const int yPlus = ViewOffsetY;

    bool linearFilter = corner1.X == corner3.X && corner1.Y == corner2.Y && corner3.Y == corner4.Y &&
        corner2.X == corner4.X;

    switch2dRendering(texture, useAlphaChannelOfTexture, linearFilter, 1);

    bool tempColors = false;

    if (!colors)
    {
        colors = new ox::video::SColor[4];
        for (int i = 0; i < 4; ++i)
            colors[i] = ox::video::SColor(0x00ffffff);
        tempColors = true;
    }

    ox::video::S3DVertex* v = &Vertices2D[Current2DQuadCount * 4];
    v[0] = ox::video::S3DVertex(((float)(corner1.X + xPlus) + 0.5f) * xFact, ((float)(yPlus - corner1.Y) + 0.5f) * yFact,
        0, 0, 0, 0, colors[0], tcoords.UpperLeftCorner.X, tcoords.UpperLeftCorner.Y);
    v[1] = ox::video::S3DVertex(((float)(corner2.X + xPlus) + 0.5f) * xFact, ((float)(yPlus - corner2.Y) + 0.5f) * yFact,
        0, 0, 0, 0, colors[3], tcoords.LowerRightCorner.X, tcoords.UpperLeftCorner.Y);
    v[2] = ox::video::S3DVertex(((float)(corner4.X + xPlus) + 0.5f) * xFact, ((float)(yPlus - corner4.Y) + 0.5f) * yFact,
        0, 0, 0, 0, colors[2], tcoords.LowerRightCorner.X, tcoords.LowerRightCorner.Y);
    v[3] = ox::video::S3DVertex(((float)(corner3.X + xPlus) + 0.5f) * xFact, ((float)(yPlus - corner3.Y) + 0.5f) * yFact,
        0, 0, 0, 0, colors[1], tcoords.UpperLeftCorner.X, tcoords.LowerRightCorner.Y);
    ++Current2DQuadCount;

    if (tempColors)
        delete[] colors;
}

//! Float version of the corner draw, for rotated and scaled images: no pixel offset, always linear
//! filtering; colors defaults to transparent white (0x00ffffff).
void CVideoOpenGL::draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<float>& corner1,
    const ox::core::CPosition2d<float>& corner2, const ox::core::CPosition2d<float>& corner3,
    const ox::core::CPosition2d<float>& corner4, const ox::core::CRect<int>& sourceRect,
    const ox::video::SColorArray* colors, bool useAlphaChannelOfTexture)
{
    if (!texture)
        return;

    const ox::core::CDimension2d<int>& ss = texture->getOriginalSize();
    ox::core::CRect<float> tcoords;
    tcoords.UpperLeftCorner.X = ((float)sourceRect.UpperLeftCorner.X + 0.5f) / ss.Width;
    tcoords.LowerRightCorner.X = ((float)sourceRect.LowerRightCorner.X - 0.5f) / ss.Width;
    tcoords.UpperLeftCorner.Y = ((float)sourceRect.UpperLeftCorner.Y + 0.5f) / ss.Height;
    tcoords.LowerRightCorner.Y = ((float)sourceRect.LowerRightCorner.Y - 0.5f) / ss.Height;

    const float xPlus = (float)ViewOffsetX;
    const float xFact = InvHalfWidth;
    const float yPlus = (float)ViewOffsetY;
    const float yFact = InvHalfHeight;

    switch2dRendering(texture, useAlphaChannelOfTexture, true, 1);

    bool tempColors = false;

    if (!colors)
    {
        colors = new ox::video::SColorArray(ox::video::SColor(0x00ffffff));
        tempColors = true;
    }

    ox::video::S3DVertex* v = &Vertices2D[Current2DQuadCount * 4];
    v[0] = ox::video::S3DVertex((corner1.X + xPlus) * xFact, (yPlus - corner1.Y) * yFact, 0, 0, 0, 0,
        colors->Colors[0], tcoords.UpperLeftCorner.X, tcoords.UpperLeftCorner.Y);
    v[1] = ox::video::S3DVertex((corner2.X + xPlus) * xFact, (yPlus - corner2.Y) * yFact, 0, 0, 0, 0,
        colors->Colors[3], tcoords.LowerRightCorner.X, tcoords.UpperLeftCorner.Y);
    v[2] = ox::video::S3DVertex((corner4.X + xPlus) * xFact, (yPlus - corner4.Y) * yFact, 0, 0, 0, 0,
        colors->Colors[2], tcoords.LowerRightCorner.X, tcoords.LowerRightCorner.Y);
    v[3] = ox::video::S3DVertex((corner3.X + xPlus) * xFact, (yPlus - corner3.Y) * yFact, 0, 0, 0, 0,
        colors->Colors[1], tcoords.UpperLeftCorner.X, tcoords.LowerRightCorner.Y);
    ++Current2DQuadCount;

    if (tempColors)
        delete colors;
}

//! A line between pixel positions; blended when the color is translucent.
void CVideoOpenGL::draw2DLine(const ox::core::CPosition2d<int>& start, const ox::core::CPosition2d<int>& end,
    ox::video::SColor color)
{
    bool alpha = color.getAlpha() < 255;
    switch2dRendering(0, alpha, false, 0);
    setRenderStates2DMode(alpha, false, false, false);
    setTexture(0, 0);

    const int xPlus = ViewOffsetX;
    const float xFact = InvHalfWidth;
    const int yPlus = ViewOffsetY;
    const float yFact = InvHalfHeight;

    ox::core::CPosition2d<float> npos_start;
    npos_start.X = (float)(start.X + xPlus) * xFact;
    npos_start.Y = (float)(yPlus - start.Y) * yFact;

    ox::core::CPosition2d<float> npos_end;
    npos_end.X = (float)(end.X + xPlus) * xFact;
    npos_end.Y = (float)(yPlus - end.Y) * yFact;

    glBegin(GL_LINES);
    // Original bug: glLineWidth is not allowed between glBegin and glEnd; OpenGL ignores it (with
    // GL_INVALID_OPERATION), so lines keep the current width, 1 pixel.
    glLineWidth(3.0f);
    glColor4ub(color.getRed(), color.getGreen(), color.getBlue(), color.getAlpha());
    glVertex2f(npos_start.X, npos_start.Y);
    glVertex2f(npos_end.X, npos_end.Y);
    glEnd();
}

//! Sets the 2D render states. Textured images always come from the batch with alpha = true:
//! with alphaChannel they are blended with (SRC_ALPHA, ONE_MINUS_SRC_ALPHA) after the alpha test
//! alpha > 0; without it blending is enabled but the blend function is whatever was set last
//! (normally the same, unless a 3D material changed it). Untextured primitives blend with
//! (SRC_ALPHA, ONE_MINUS_SRC_ALPHA) when alpha. The texture environment is GL_MODULATE.
void CVideoOpenGL::setRenderStates2DMode(bool alpha, bool texture, bool alphaChannel, bool linearFilter)
{
    if (CurrentRenderMode != ERM_2D || Transformation3DChanged)
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        Transformation3DChanged = false;

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_FOG);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_LIGHTING);

        if (MultiTextureExtension)
            extGlActiveTextureARB(GL_TEXTURE0_ARB);

        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);

        ClampTexture = false;

        // unset last 3d material
        if (CurrentRenderMode == ERM_3D && LastMaterial.MaterialType >= 0 &&
            LastMaterial.MaterialType < (int)MaterialRenderers.size())
            MaterialRenderers[LastMaterial.MaterialType].Renderer->OnUnsetMaterial();
    }

    glDisable(GL_CULL_FACE);

    if (texture)
    {
        GLint filter = linearFilter && !ForcePointSampling ? GL_LINEAR : GL_NEAREST;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);

        if (alphaChannel)
        {
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glEnable(GL_BLEND);
            glEnable(GL_ALPHA_TEST);
            glAlphaFunc(GL_GREATER, 0);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        else
        {
            if (alpha)
            {
                glDisable(GL_ALPHA_TEST);
                glEnable(GL_BLEND);
            }
            else
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                glDisable(GL_ALPHA_TEST);
                glDisable(GL_BLEND);
            }
        }
    }
    else
    {
        if (alpha)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glDisable(GL_ALPHA_TEST);
        }
        else
        {
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glDisable(GL_BLEND);
            glDisable(GL_ALPHA_TEST);
        }
    }

    CurrentRenderMode = ERM_2D;
}

//! Binds the texture to a unit; textures already bound are skipped (the cache is cleared when a
//! new texture is created, since that binds it).
bool CVideoOpenGL::setTexture(int stage, ox::video::ITexture* texture)
{
    if (stage >= MaxTextureUnits)
        return false;

    if (texture && CurrentTexture[stage] == texture)
        return true;

    if (MultiTextureExtension)
        extGlActiveTextureARB(GL_TEXTURE0_ARB + stage);

    CurrentTexture[stage] = texture;

    if (!texture)
    {
        glDisable(GL_TEXTURE_2D);
        return true;
    }

    if (texture->getDriverType() != ox::video::EDT_OPENGL)
    {
        glDisable(GL_TEXTURE_2D);
        os::Printer::log("Fatal Error: Tried to set a texture not owned by this driver.", ox::event::ELL_ERROR);
        return false;
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, ((COpenGLTexture*)texture)->getOpenGLTextureName());
    return true;
}

//! A line between float positions, with linear filtering states; see draw2DLine.
void CVideoOpenGL::draw2DLineFloat(const ox::core::CPosition2d<float>& start, const ox::core::CPosition2d<float>& end,
    ox::video::SColor color)
{
    bool alpha = color.getAlpha() < 255;
    switch2dRendering(0, alpha, true, 0);
    setRenderStates2DMode(alpha, false, false, true);
    setTexture(0, 0);

    const float xPlus = (float)ViewOffsetX;
    const float xFact = InvHalfWidth;
    const float yPlus = (float)ViewOffsetY;
    const float yFact = InvHalfHeight;

    ox::core::CPosition2d<float> npos_start;
    npos_start.X = (start.X + xPlus) * xFact;
    npos_start.Y = (yPlus - start.Y) * yFact;

    ox::core::CPosition2d<float> npos_end;
    npos_end.X = (end.X + xPlus) * xFact;
    npos_end.Y = (yPlus - end.Y) * yFact;

    glBegin(GL_LINES);
    // Original bug: ignored inside glBegin/glEnd, see draw2DLine.
    glLineWidth(3.0f);
    glColor4ub(color.getRed(), color.getGreen(), color.getBlue(), color.getAlpha());
    glVertex2f(npos_start.X, npos_start.Y);
    glVertex2f(npos_end.X, npos_end.Y);
    glEnd();
}

//! A filled rectangle (not batched), clipped to clip; blended when the color is translucent.
void CVideoOpenGL::draw2DRectangle(ox::video::SColor color, const ox::core::CRect<int>& position,
    const ox::core::CRect<int>* clip)
{
    ox::core::CRect<int> pos = position;

    if (clip)
    {
        if (!pos.isRectCollided(*clip))
            return;

        clipAgainst(pos, *clip);
    }

    bool alpha = color.getAlpha() < 255;
    switch2dRendering(0, alpha, false, 0);
    setRenderStates2DMode(alpha, false, false, false);
    setTexture(0, 0);

    const int xPlus = ViewOffsetX;
    const float xFact = InvHalfWidth;
    const int yPlus = ViewOffsetY;
    const float yFact = InvHalfHeight;

    ox::core::CRect<float> npos;
    npos.UpperLeftCorner.X = (float)(pos.UpperLeftCorner.X + xPlus) * xFact;
    npos.UpperLeftCorner.Y = (float)(yPlus - pos.UpperLeftCorner.Y) * yFact;
    npos.LowerRightCorner.X = (float)(pos.LowerRightCorner.X + xPlus) * xFact;
    npos.LowerRightCorner.Y = (float)(yPlus - pos.LowerRightCorner.Y) * yFact;

    glBegin(GL_QUADS);
    glColor4ub(color.getRed(), color.getGreen(), color.getBlue(), color.getAlpha());
    glVertex2f(npos.UpperLeftCorner.X, npos.UpperLeftCorner.Y);
    glVertex2f(npos.LowerRightCorner.X, npos.UpperLeftCorner.Y);
    glVertex2f(npos.LowerRightCorner.X, npos.LowerRightCorner.Y);
    glVertex2f(npos.UpperLeftCorner.X, npos.LowerRightCorner.Y);
    glEnd();
}

bool CVideoOpenGL::queryFeature(ox::video::E_VIDEO_DRIVER_FEATURE feature)
{
    switch (feature)
    {
    case ox::video::EVDF_MULTITEXTURE:
        return MultiTextureExtension;
    case ox::video::EVDF_BILINEAR_FILTER:
    case ox::video::EVDF_MIP_MAP:
        return true;
    case ox::video::EVDF_MIP_MAP_AUTO_UPDATE:
        return GenerateMipmapExtension;
    case ox::video::EVDF_STENCIL_BUFFER:
        return StencilBuffer;
    case ox::video::EVDF_ARB_VERTEX_PROGRAM_1:
        return ARBVertexProgramExtension;
    case ox::video::EVDF_ARB_FRAGMENT_PROGRAM_1:
        return ARBFragmentProgramExtension;
    case ox::video::EVDF_ARB_GLSL:
        return ARBShadingLanguage100Extension;
    case ox::video::EVDF_TEXTURE_NPOT:
        return TextureNPOTExtension;
    case ox::video::EVDF_FRAMEBUFFER_OBJECT:
        return FramebufferObjectExtension;
    default:
        return false;
    };
}

void CVideoOpenGL::extGlActiveTextureARB(GLenum texture)
{
    if (MultiTextureExtension && pGlActiveTextureARB)
        pGlActiveTextureARB(texture);
}

bool CVideoOpenGL::disableTextures(int fromStage)
{
    bool result = true;
    for (int i = fromStage; i < MaxTextureUnits; ++i)
        result &= setTexture(i, 0);
    return result;
}

//! Textures get mip maps when ETCF_CREATE_MIP_MAPS is set (the game clears it).
ox::video::ITexture* CVideoOpenGL::createDeviceDependentTexture(ox::video::IImage* surface)
{
    bool generateMipLevels = getTextureCreationFlag(ox::video::ETCF_CREATE_MIP_MAPS);

    // the new texture is bound while it is uploaded
    for (int i = 0; i < MaxTextureUnits; ++i)
        CurrentTexture[i] = 0;

    return new COpenGLTexture(surface, generateMipLevels);
}

void CVideoOpenGL::setMaterial(const ox::video::SMaterial& material)
{
    Material = material;

    for (int i = 0; i < ox::video::MATERIAL_MAX_TEXTURES; ++i)
        setTexture(i, Material.Textures[i]);
}

//! Applies the parts of material that differ from lastmaterial (all of them when
//! resetAllRenderstates). Original bug (from Irrlicht 0.7): the material colors, the shininess,
//! the magnification filter and lighting are taken from the driver's current Material rather than
//! from material; they are the same object whenever the driver itself calls this.
void CVideoOpenGL::setBasicRenderStates(const ox::video::SMaterial& material, const ox::video::SMaterial& lastmaterial,
    bool resetAllRenderstates)
{
    if (resetAllRenderstates || lastmaterial.AmbientColor.color != material.AmbientColor.color ||
        lastmaterial.DiffuseColor.color != material.DiffuseColor.color ||
        lastmaterial.SpecularColor.color != material.SpecularColor.color ||
        lastmaterial.EmissiveColor.color != material.EmissiveColor.color ||
        lastmaterial.Shininess != material.Shininess)
    {
        GLfloat color[4];

        const float inv = 1.0f / 255.0f;

        color[0] = Material.AmbientColor.getRed() * inv;
        color[1] = Material.AmbientColor.getGreen() * inv;
        color[2] = Material.AmbientColor.getBlue() * inv;
        color[3] = Material.AmbientColor.getAlpha() * inv;
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color);

        color[0] = Material.DiffuseColor.getRed() * inv;
        color[1] = Material.DiffuseColor.getGreen() * inv;
        color[2] = Material.DiffuseColor.getBlue() * inv;
        color[3] = Material.DiffuseColor.getAlpha() * inv;
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);

        color[0] = Material.SpecularColor.getRed() * inv;
        color[1] = Material.SpecularColor.getGreen() * inv;
        color[2] = Material.SpecularColor.getBlue() * inv;
        color[3] = Material.SpecularColor.getAlpha() * inv;
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);

        color[0] = Material.EmissiveColor.getRed() * inv;
        color[1] = Material.EmissiveColor.getGreen() * inv;
        color[2] = Material.EmissiveColor.getBlue() * inv;
        color[3] = Material.EmissiveColor.getAlpha() * inv;
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, color);

        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, Material.Shininess);
    }

    // bilinear

    if (resetAllRenderstates || lastmaterial.BilinearFilter != material.BilinearFilter)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Material.BilinearFilter ? GL_LINEAR : GL_NEAREST);

    // fillmode

    if (resetAllRenderstates || lastmaterial.Wireframe != material.Wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, material.Wireframe ? GL_LINE : GL_FILL);

    // lighting

    if (resetAllRenderstates || lastmaterial.Lighting != material.Lighting)
    {
        if (Material.Lighting)
            glEnable(GL_LIGHTING);
        else
            glDisable(GL_LIGHTING);
    }

    // zbuffer

    if (resetAllRenderstates || lastmaterial.ZBuffer != material.ZBuffer)
    {
        if (material.ZBuffer)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
    }

    // zwrite
    if (resetAllRenderstates || lastmaterial.ZWriteEnable != material.ZWriteEnable)
        glDepthMask(material.ZWriteEnable);

    // back face culling

    if (resetAllRenderstates || lastmaterial.BackfaceCulling != material.BackfaceCulling)
    {
        if (material.BackfaceCulling)
            glEnable(GL_CULL_FACE);
        else
            glDisable(GL_CULL_FACE);
    }

    // front face winding

    if (resetAllRenderstates || lastmaterial.FrontFaceCCW != material.FrontFaceCCW)
    {
        if (material.FrontFaceCCW)
            glFrontFace(GL_CCW);
        else
            glFrontFace(GL_CW);
    }

    // fog
    if (resetAllRenderstates || lastmaterial.FogEnable != material.FogEnable)
    {
        if (material.FogEnable)
            glEnable(GL_FOG);
        else
            glDisable(GL_FOG);
    }

    // texture wrap; a change of the v mirroring alone is only applied with the extension
    if (resetAllRenderstates || lastmaterial.TextureMirrorU != material.TextureMirrorU ||
        (lastmaterial.TextureMirrorV != material.TextureMirrorV && TextureMirroredRepeatExtension))
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, material.TextureMirrorU ? GL_MIRRORED_REPEAT : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, material.TextureMirrorV ? GL_MIRRORED_REPEAT : GL_REPEAT);
    }
}

void CVideoOpenGL::flush2dRendering()
{
    switch2dRendering(0, false, false, MAX_2D_QUADS);
}

//! Flushes the 2D batch and restricts drawing to rect (0: no scissor).
void CVideoOpenGL::setScissorRect(ox::core::CRect<int>* rect)
{
    flush2dRendering();

    if (rect)
    {
        glEnable(GL_SCISSOR_TEST);
        // Original bug: OpenGL's window y grows upwards, so y should be
        // height - LowerRightCorner.Y; this is its negative, right only for rectangles that end
        // at the bottom edge of the window.
        glScissor(rect->UpperLeftCorner.X, rect->LowerRightCorner.Y - PhysicalScreenSize.Height, rect->getWidth(),
            rect->getHeight());
    }
    else
        glDisable(GL_SCISSOR_TEST);
}

const wchar_t* CVideoOpenGL::getName()
{
    return L"OpenGL1.2";
}

void CVideoOpenGL::deleteAllDynamicLights()
{
    for (int i = 0; i < LastSetLight + 1; ++i)
        glDisable(GL_LIGHT0 + i);

    LastSetLight = -1;

    CVideoNull::deleteAllDynamicLights();
}

//! Adds an OpenGL light with linear attenuation 1 / Radius. Original bug (from Irrlicht 0.7):
//! the limit is the enum GL_MAX_LIGHTS (0x0d31), not the number of lights the driver supports.
void CVideoOpenGL::addDynamicLight(const ox::video::SLight& light)
{
    ++LastSetLight;
    if (!(LastSetLight < GL_MAX_LIGHTS))
        return;

    setTransform(ox::video::ETS_WORLD, ox::core::CMatrix4());

    CVideoNull::addDynamicLight(light);

    int lidx = GL_LIGHT0 + LastSetLight;
    GLfloat data[4];

    // set position
    data[0] = light.Position.X;
    data[1] = light.Position.Y;
    data[2] = light.Position.Z;
    data[3] = light.Directional ? 0.0f : 1.0f;
    glLightfv(lidx, GL_POSITION, data);

    // set diffuse color
    data[0] = light.DiffuseColor.r;
    data[1] = light.DiffuseColor.g;
    data[2] = light.DiffuseColor.b;
    data[3] = light.DiffuseColor.a;
    glLightfv(lidx, GL_DIFFUSE, data);

    // set specular color
    data[0] = light.SpecularColor.r;
    data[1] = light.SpecularColor.g;
    data[2] = light.SpecularColor.b;
    data[3] = light.SpecularColor.a;
    glLightfv(lidx, GL_SPECULAR, data);

    // set ambient color
    data[0] = light.AmbientColor.r;
    data[1] = light.AmbientColor.g;
    data[2] = light.AmbientColor.b;
    data[3] = light.AmbientColor.a;
    glLightfv(lidx, GL_AMBIENT, data);

    // 1.0f / (constant + linar * d + quadratic*(d*d);

    // set attenuation
    glLightf(lidx, GL_CONSTANT_ATTENUATION, 0.0f);
    glLightf(lidx, GL_LINEAR_ATTENUATION, 1.0f / light.Radius);
    glLightf(lidx, GL_QUADRATIC_ATTENUATION, 0.0f);

    glEnable(lidx);
}

int CVideoOpenGL::getMaximalDynamicLightAmount()
{
    return GL_MAX_LIGHTS;
}

void CVideoOpenGL::setAmbientLight(const ox::video::SColorf& color)
{
    GLfloat data[4] = {color.r, color.g, color.b, color.a};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, data);
}

//! Clips area to the screen; OpenGL's viewport y counts from the bottom.
void CVideoOpenGL::setViewPort(const ox::core::CRect<int>& area)
{
    ox::core::CRect<int> vp = area;
    ox::core::CRect<int> rendert(0, 0, ScreenSize.Width, ScreenSize.Height);
    clipAgainst(vp, rendert);

    if (vp.getHeight() > 0 && vp.getWidth() > 0)
        glViewport(vp.UpperLeftCorner.X, ScreenSize.Height - vp.UpperLeftCorner.Y - vp.getHeight(), vp.getWidth(),
            vp.getHeight());

    ViewPort = vp;
}

void CVideoOpenGL::drawStencilShadowVolume(const ox::core::CVector3d<float>* triangles, int count, bool zfail)
{
    if (!StencilBuffer || !count)
        return;

    // store current OpenGL	state
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT | GL_STENCIL_BUFFER_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_STENCIL_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // no color buffer drawing
    glStencilFunc(GL_ALWAYS, 1, 0xFFFFFFFFL);
    glColorMask(0, 0, 0, 0);
    glEnable(GL_CULL_FACE);

    // unset last 3d material
    if (CurrentRenderMode == ERM_3D && LastMaterial.MaterialType >= 0 &&
        LastMaterial.MaterialType < (int)MaterialRenderers.size())
    {
        MaterialRenderers[LastMaterial.MaterialType].Renderer->OnUnsetMaterial();
        ResetRenderStates = true;
    }

    if (!zfail)
    {
        // ZPASS Method

        glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
        glCullFace(GL_BACK);
        glBegin(GL_TRIANGLES);

        int i;
        for (i = 0; i < count; ++i)
            glVertex3f(triangles[i].X, triangles[i].Y, triangles[i].Z);

        glEnd();

        glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
        glCullFace(GL_FRONT);

        glBegin(GL_TRIANGLES);
        for (i = 0; i < count; ++i)
            glVertex3f(triangles[i].X, triangles[i].Y, triangles[i].Z);

        glEnd();
    }
    else
    {
        // ZFAIL Method

        glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
        glCullFace(GL_FRONT);

        glBegin(GL_TRIANGLES);

        int i;
        for (i = 0; i < count; i++)
            glVertex3f(triangles[i].X, triangles[i].Y, triangles[i].Z);

        glEnd();

        glStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
        glCullFace(GL_BACK);

        glBegin(GL_TRIANGLES);

        for (i = 0; i < count; i++)
            glVertex3f(triangles[i].X, triangles[i].Y, triangles[i].Z);

        glEnd();
    }

    glPopAttrib();
}

void CVideoOpenGL::drawStencilShadow(bool clearStencilBuffer, ox::video::SColor leftUpEdge,
    ox::video::SColor rightUpEdge, ox::video::SColor leftDownEdge, ox::video::SColor rightDownEdge)
{
    if (!StencilBuffer)
        return;

    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT | GL_STENCIL_BUFFER_BIT);

    glFrontFace(GL_CCW);

    // Enable Rendering To Colour Buffer For All Components
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);

    // Draw A Shadowing Rectangle Covering The Entire Screen

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glStencilFunc(GL_NOTEQUAL, 0, 0xFFFFFFFFL);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glEnable(GL_STENCIL_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);

    glPushMatrix();
    glLoadIdentity();

    glBegin(GL_TRIANGLE_STRIP);
    glColor4ub(leftUpEdge.getRed(), leftUpEdge.getGreen(), leftUpEdge.getBlue(), leftUpEdge.getAlpha());
    glVertex3f(-10.1f, 10.1f, 0.90f);

    glColor4ub(leftDownEdge.getRed(), leftDownEdge.getGreen(), leftDownEdge.getBlue(), leftDownEdge.getAlpha());
    glVertex3f(-10.1f, -10.1f, 0.90f);

    glColor4ub(rightUpEdge.getRed(), rightUpEdge.getGreen(), rightUpEdge.getBlue(), rightUpEdge.getAlpha());
    glVertex3f(10.1f, 10.1f, 0.90f);

    glColor4ub(rightDownEdge.getRed(), rightDownEdge.getGreen(), rightDownEdge.getBlue(), rightDownEdge.getAlpha());
    glVertex3f(10.1f, -10.1f, 0.90f);
    glEnd();

    glPopMatrix();
    glPopAttrib();

    if (clearStencilBuffer)
        glClear(GL_STENCIL_BUFFER_BIT);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void CVideoOpenGL::setFog(ox::video::SColor c, bool linearFog, float start, float end, float density, bool pixelFog,
    bool rangeFog)
{
    CVideoNull::setFog(c, linearFog, start, end, density, pixelFog, rangeFog);

    glFogi(GL_FOG_MODE, linearFog ? GL_LINEAR : GL_EXP);

    if (linearFog)
    {
        glFogf(GL_FOG_START, start);
        glFogf(GL_FOG_END, end);
    }
    else
        glFogf(GL_FOG_DENSITY, density);

    ox::video::SColorf color(c);
    GLfloat data[4] = {color.r, color.g, color.b, color.a};
    glFogfv(GL_FOG_COLOR, data);
}

void CVideoOpenGL::draw3DLine(const ox::core::CVector3d<float>& start, const ox::core::CVector3d<float>& end,
    ox::video::SColor color)
{
    setRenderStates3DMode();
    setTexture(0, 0);

    glBegin(GL_LINES);
    glColor4ub(color.getRed(), color.getGreen(), color.getBlue(), color.getAlpha());
    glVertex3f(start.X, start.Y, start.Z);

    glColor4ub(color.getRed(), color.getGreen(), color.getBlue(), color.getAlpha());
    glVertex3f(end.X, end.Y, end.Z);
    glEnd();
}

void CVideoOpenGL::OnResize(const ox::core::CDimension2d<int>& size)
{
    CVideoNull::OnResize(size);
    glViewport(0, 0, size.Width, size.Height);
}

int CVideoOpenGL::getDriverType()
{
    return ox::video::EDT_OPENGL;
}

void CVideoOpenGL::extGlGenProgramsARB(GLsizei n, GLuint* programs)
{
    if (pGlGenProgramsARB)
        pGlGenProgramsARB(n, programs);
}

void CVideoOpenGL::extGlBindProgramARB(GLenum target, GLuint program)
{
    if (pGlBindProgramARB)
        pGlBindProgramARB(target, program);
}

void CVideoOpenGL::extGlProgramStringARB(GLenum target, GLenum format, GLsizei len, const GLvoid* string)
{
    if (pGlProgramStringARB)
        pGlProgramStringARB(target, format, len, string);
}

void CVideoOpenGL::extGlDeleteProgramsARB(GLsizei n, const GLuint* programs)
{
    if (pGlDeleteProgramsARB)
        pGlDeleteProgramsARB(n, programs);
}

void CVideoOpenGL::extGlProgramLocalParameter4fvARB(GLenum target, GLuint index, const GLfloat* params)
{
    if (pGlProgramLocalParameter4fvARB)
        pGlProgramLocalParameter4fvARB(target, index, params);
}

GLhandleARB CVideoOpenGL::extGlCreateShaderObjectARB(GLenum shaderType)
{
    if (pGlCreateShaderObjectARB)
        return pGlCreateShaderObjectARB(shaderType);
    return 0;
}

void CVideoOpenGL::extGlShaderSourceARB(GLhandleARB shader, int numOfStrings, const char** strings, int* lenOfStrings)
{
    if (pGlShaderSourceARB)
        pGlShaderSourceARB(shader, numOfStrings, strings, lenOfStrings);
}

void CVideoOpenGL::extGlCompileShaderARB(GLhandleARB shader)
{
    if (pGlCompileShaderARB)
        pGlCompileShaderARB(shader);
}

GLhandleARB CVideoOpenGL::extGlCreateProgramObjectARB()
{
    if (pGlCreateProgramObjectARB)
        return pGlCreateProgramObjectARB();
    return 0;
}

void CVideoOpenGL::extGlAttachObjectARB(GLhandleARB program, GLhandleARB shader)
{
    if (pGlAttachObjectARB)
        pGlAttachObjectARB(program, shader);
}

void CVideoOpenGL::extGlLinkProgramARB(GLhandleARB program)
{
    if (pGlLinkProgramARB)
        pGlLinkProgramARB(program);
}

void CVideoOpenGL::extGlUseProgramObjectARB(GLhandleARB prog)
{
    if (pGlUseProgramObjectARB)
        pGlUseProgramObjectARB(prog);
}

void CVideoOpenGL::extGlDeleteObjectARB(GLhandleARB object)
{
    if (pGlDeleteObjectARB)
        pGlDeleteObjectARB(object);
}

void CVideoOpenGL::extGlGetInfoLogARB(GLhandleARB object, GLsizei maxLength, GLsizei* length, GLcharARB* infoLog)
{
    if (pGlGetInfoLogARB)
        pGlGetInfoLogARB(object, maxLength, length, infoLog);
}

void CVideoOpenGL::extGlGetObjectParameterivARB(GLhandleARB object, GLenum type, int* param)
{
    if (pGlGetObjectParameterivARB)
        pGlGetObjectParameterivARB(object, type, param);
}

GLint CVideoOpenGL::extGlGetUniformLocationARB(GLhandleARB program, const char* name)
{
    if (pGlGetUniformLocationARB)
        return pGlGetUniformLocationARB(program, name);
    return 0;
}

void CVideoOpenGL::extGlUniform4fvARB(GLint location, GLsizei count, const GLfloat* v)
{
    if (pGlUniform4fvARB)
        pGlUniform4fvARB(location, count, v);
}

void CVideoOpenGL::extGlUniform1ivARB(GLint location, GLsizei count, const GLint* v)
{
    if (pGlUniform1ivARB)
        pGlUniform1ivARB(location, count, v);
}

void CVideoOpenGL::extGlUniform1fvARB(GLint location, GLsizei count, const GLfloat* v)
{
    if (pGlUniform1fvARB)
        pGlUniform1fvARB(location, count, v);
}

void CVideoOpenGL::extGlUniform2fvARB(GLint location, GLsizei count, const GLfloat* v)
{
    if (pGlUniform2fvARB)
        pGlUniform2fvARB(location, count, v);
}

void CVideoOpenGL::extGlUniform3fvARB(GLint location, GLsizei count, const GLfloat* v)
{
    if (pGlUniform3fvARB)
        pGlUniform3fvARB(location, count, v);
}

void CVideoOpenGL::extGlUniformMatrix2fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat* v)
{
    if (pGlUniformMatrix2fvARB)
        pGlUniformMatrix2fvARB(location, count, transpose, v);
}

void CVideoOpenGL::extGlUniformMatrix3fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat* v)
{
    if (pGlUniformMatrix3fvARB)
        pGlUniformMatrix3fvARB(location, count, transpose, v);
}

void CVideoOpenGL::extGlUniformMatrix4fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat* v)
{
    if (pGlUniformMatrix4fvARB)
        pGlUniformMatrix4fvARB(location, count, transpose, v);
}

void CVideoOpenGL::extGlGetActiveUniformARB(GLhandleARB program, GLuint index, GLsizei maxlength, GLsizei* length,
    GLint* size, GLenum* type, GLcharARB* name)
{
    if (pGlGetActiveUniformARB)
        pGlGetActiveUniformARB(program, index, maxlength, length, size, type, name);
}

void CVideoOpenGL::extGlPointParameterfARB(GLint loc, GLfloat f)
{
    if (pGlPointParameterfARB)
        pGlPointParameterfARB(loc, f);
}

void CVideoOpenGL::extGlPointParameterfvARB(GLint loc, const GLfloat* v)
{
    if (pGlPointParameterfvARB)
        pGlPointParameterfvARB(loc, v);
}

void CVideoOpenGL::extGlStencilFuncSeparate(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask)
{
    if (pGlStencilFuncSeparate)
        pGlStencilFuncSeparate(frontfunc, backfunc, ref, mask);
    else if (pGlStencilFuncSeparateATI)
        pGlStencilFuncSeparateATI(frontfunc, backfunc, ref, mask);
}

void CVideoOpenGL::extGlStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
    if (pGlStencilOpSeparate)
        pGlStencilOpSeparate(face, fail, zfail, zpass);
    else if (pGlStencilOpSeparateATI)
        pGlStencilOpSeparateATI(face, fail, zfail, zpass);
}

//! Does nothing on Linux: the entry point is never loaded.
void CVideoOpenGL::extGlCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width,
    GLsizei height, GLint border, GLsizei imageSize, const void* data)
{
}

void CVideoOpenGL::extGlBindFramebufferEXT(GLenum target, GLuint framebuffer)
{
    if (pGlBindFramebufferEXT)
        pGlBindFramebufferEXT(target, framebuffer);
}

void CVideoOpenGL::extGlDeleteFramebuffersEXT(GLsizei n, const GLuint* framebuffers)
{
    if (pGlDeleteFramebuffersEXT)
        pGlDeleteFramebuffersEXT(n, framebuffers);
}

void CVideoOpenGL::extGlGenFramebuffersEXT(GLsizei n, GLuint* framebuffers)
{
    if (pGlGenFramebuffersEXT)
        pGlGenFramebuffersEXT(n, framebuffers);
}

GLenum CVideoOpenGL::extGlCheckFramebufferStatusEXT(GLenum target)
{
    if (pGlCheckFramebufferStatusEXT)
        return pGlCheckFramebufferStatusEXT(target);
    return 0;
}

void CVideoOpenGL::extGlFramebufferTexture2DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
    GLint level)
{
    if (pGlFramebufferTexture2DEXT)
        pGlFramebufferTexture2DEXT(target, attachment, textarget, texture, level);
}

void CVideoOpenGL::extGlBindRenderbufferEXT(GLenum target, GLuint renderbuffer)
{
    if (pGlBindRenderbufferEXT)
        pGlBindRenderbufferEXT(target, renderbuffer);
}

void CVideoOpenGL::extGlDeleteRenderbuffersEXT(GLsizei n, const GLuint* renderbuffers)
{
    if (pGlDeleteRenderbuffersEXT)
        pGlDeleteRenderbuffersEXT(n, renderbuffers);
}

void CVideoOpenGL::extGlGenRenderbuffersEXT(GLsizei n, GLuint* renderbuffers)
{
    if (pGlGenRenderbuffersEXT)
        pGlGenRenderbuffersEXT(n, renderbuffers);
}

void CVideoOpenGL::extGlRenderbufferStorageEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    if (pGlRenderbufferStorageEXT)
        pGlRenderbufferStorageEXT(target, internalformat, width, height);
}

void CVideoOpenGL::extGlFramebufferRenderbufferEXT(GLenum target, GLenum attachment, GLenum renderbuffertarget,
    GLuint renderbuffer)
{
    if (pGlFramebufferRenderbufferEXT)
        pGlFramebufferRenderbufferEXT(target, attachment, renderbuffertarget, renderbuffer);
}

bool CVideoOpenGL::hasMultiTextureExtension()
{
    return MultiTextureExtension;
}

//! ARB vertex program constants: one vec4 per register.
void CVideoOpenGL::setVertexShaderConstant(const float* data, int startRegister, int constantAmount)
{
    for (int i = 0; i < constantAmount; ++i)
        extGlProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, startRegister + i, &data[i * 4]);
}

void CVideoOpenGL::setPixelShaderConstant(const float* data, int startRegister, int constantAmount)
{
    for (int i = 0; i < constantAmount; ++i)
        extGlProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, startRegister + i, &data[i * 4]);
}

//! Original bug: forwards to the pixel shader version, which only logs an error.
bool CVideoOpenGL::setVertexShaderConstant(const char* name, const float* floats, int count)
{
    return setPixelShaderConstant(name, floats, count);
}

bool CVideoOpenGL::setPixelShaderConstant(const char* name, const float* floats, int count)
{
    os::Printer::log("Error: Please call services->setPixelShaderConstant(), not VideoDriver->setPixelShaderConstant().",
        ox::event::ELL_INFORMATION);
    return false;
}

int CVideoOpenGL::addShaderMaterial(const char* vertexShaderProgram, const char* pixelShaderProgram,
    ox::video::IShaderConstantSetCallBack* callback, ox::video::E_MATERIAL_TYPE baseMaterial, int userData)
{
    int nr = -1;
    COpenGLShaderMaterialRenderer* r = new COpenGLShaderMaterialRenderer(this, nr, vertexShaderProgram,
        pixelShaderProgram, callback, getMaterialRenderer(baseMaterial), userData);

    r->drop();
    return nr;
}

int CVideoOpenGL::addHighLevelShaderMaterial(const char* vertexShaderProgram, const char* vertexShaderEntryPointName,
    ox::video::E_VERTEX_SHADER_TYPE vsCompileTarget, const char* pixelShaderProgram,
    const char* pixelShaderEntryPointName, ox::video::E_PIXEL_SHADER_TYPE psCompileTarget,
    ox::video::IShaderConstantSetCallBack* callback, ox::video::E_MATERIAL_TYPE baseMaterial, int userData)
{
    int nr = -1;
    COpenGLSLMaterialRenderer* r = new COpenGLSLMaterialRenderer(this, nr, vertexShaderProgram,
        vertexShaderEntryPointName, vsCompileTarget, pixelShaderProgram, pixelShaderEntryPointName, psCompileTarget,
        callback, getMaterialRenderer(baseMaterial), userData);

    r->drop();
    return nr;
}

//! Creates a Cg material in the shared Cg context; returns the new material type, or -1.
int CVideoOpenGL::addCgShaderMaterial(const char* vertexShaderProgram, const char* vertexShaderEntryPointName,
    const char* pixelShaderProgram, const char* pixelShaderEntryPointName,
    ox::video::IShaderConstantSetCallBack* callback, ox::video::E_MATERIAL_TYPE baseMaterial, bool precompiled,
    int userData)
{
    int nr = -1;
    COpenGLCGMaterialRenderer* r = new COpenGLCGMaterialRenderer(CgContext, this, this, nr, vertexShaderProgram,
        vertexShaderEntryPointName, pixelShaderProgram, pixelShaderEntryPointName, callback,
        getMaterialRenderer(baseMaterial), precompiled, userData);

    r->drop();
    return nr;
}

ox::video::IVideoDriver* CVideoOpenGL::getVideoDriver()
{
    return this;
}

void* CVideoOpenGL::getGPUProgrammingServices()
{
    return static_cast<ox::video::IGPUProgrammingServices*>(this);
}

void* CVideoOpenGL::getPostProcessingServices()
{
    return static_cast<ox::video::IPostProcessingServices*>(this);
}

//! An empty texture (no OpenGL name); size is ignored.
ox::video::ITexture* CVideoOpenGL::createScreenTexture(const ox::core::CDimension2d<int>& size)
{
    return new COpenGLTexture(0, false);
}

//! Not supported: returns true without changing the target, so the game's minimap texture code
//! draws to the screen (CPlayState uses the texture only when render targets are supported).
bool CVideoOpenGL::setRenderTarget(ox::video::ITexture* texture, bool clearBackBuffer, bool clearZBuffer,
    ox::video::SColor color)
{
    return true;
}

void CVideoOpenGL::captureScreenBuffer(unsigned int index, const ox::core::CRect<int>& area)
{
}

bool CVideoOpenGL::isFullscreen()
{
    return Fullscreen;
}

bool CVideoOpenGL::setPresentationForFullscreen(bool presentation)
{
    return false;
}

//! Records the mode (the device switches the window) and resets the 3D render states.
bool CVideoOpenGL::setFullscreen(bool fullscreen)
{
    Fullscreen = fullscreen;
    setRenderStates3DMode();
    return true;
}

//! Saves the viewport as directory + (name or "Screen-") + yymmdd + "-NN" + ".jpg", with the first
//! number NN (two digits from 00, more past 99) that does not exist yet. Returns false.
bool CVideoOpenGL::saveJpegScreenshot(const char* directory, const char* name)
{
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    const int width = viewport[2];
    const int height = viewport[3];

    unsigned char* pixels = new unsigned char[width * height * 3];
    glReadPixels(viewport[0], viewport[1], width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    // OpenGL's rows run bottom up; flip them
    const int pitch = width * 3;
    unsigned char* flipped = new unsigned char[pitch * height];
    for (int y = 0; y < height; ++y)
        memcpy(flipped + (height - 1 - y) * pitch, pixels + y * pitch, pitch);

    delete[] pixels;

    ox::core::CString<char> path;
    int i = 0;
    do
    {
        ox::core::CString<char> fileName;
        if (name)
            fileName = name;
        else
            fileName = "Screen-";

        fileName.append(ox::core::CBasic::getTimeString((char*)"%y%m%d"));

        if (i > 9)
            fileName.append(ox::core::CString<char>("-"));
        else
            fileName.append(ox::core::CString<char>("-0"));

        fileName.append(i);
        fileName.append(ox::core::CString<char>(".jpg"));

        path = directory;
        path.append(fileName);
        ++i;
    } while (FileSystem->existFile(path.c_str(), false));

    ox::io::IWriteFile* file = FileSystem->createAndWriteFile(path.c_str(), false);
    CImageLoaderJPG::saveImage(file, flipped, width, height, 3);
    file->drop();

    delete[] flipped;

    return false;
}

ox::video::IVideoDriver* createOpenGLDriver(const ox::core::CDimension2d<int>& screenSize, ox::IOxDevice* device,
    bool fullscreen, bool stencilBuffer, ox::io::IFileSystem* io)
{
    return new CVideoOpenGL(screenSize, device, fullscreen, stencilBuffer, io);
}

} // end namespace video
} // end namespace daisy
