// The original CVideoOpenGL (src/daisy/video/OpenGL/CVideoOpenGL.cpp) function by function. Where it
// called OpenGL 1.x, this calls OpenGL ES 3.0 / OpenGL 3.3 core for the state they still have and
// CFixedFunction for the rest. Deviations from the original are marked "Port:".
//
// 2D conventions (see CVideoNull::update2dViewValues): a pixel position (x, y), y down, maps to
// clip space ((x + ViewOffsetX) * InvHalfWidth, (ViewOffsetY - y) * InvHalfHeight) with identity
// matrices, so integer positions fall on pixel edges. Images are collected in the 2D quad batch
// (switch2dRendering) and drawn when the texture, the alpha-channel flag or the filter changes;
// rectangles and lines are drawn at once, after flushing the batch.

#include "video/CVideoGL.h"
#include "video/CCgMaterialRendererGL.h"
#include "video/CTextureGL.h"
#include "video/MaterialRenderers.h"
#include "video/Shaders.h"
#include "platform/Seams.h"
#include "daisy/os.h"
#include "ox/IOxDevice.h"
#include "ox/core/CBasic.h"
#include "ox/io/IFileSystem.h"
#include "ox/io/IWriteFile.h"
#include "ox/video/S3DVertex2TCoords.h"
#include "ox/video/S3DVertexInline.h"
#include "ox/video/SColorArray.h"
#include "ox/video/SLight.h"
#include "ox/video/SMaterialInline.h"
#include <ctype.h>
#include <string.h>
#include <stb_image_write.h>

namespace port {
namespace video {

using namespace gl;

namespace {

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

//! An SColor as the four floats glMaterialfv and friends take.
inline void toFloats(float out[4], ox::video::SColor color)
{
    const float inv = 1.0f / 255.0f;
    out[0] = color.getRed() * inv;
    out[1] = color.getGreen() * inv;
    out[2] = color.getBlue() * inv;
    out[3] = color.getAlpha() * inv;
}

inline void toFloats(float out[4], const ox::video::SColorf& color)
{
    out[0] = color.r;
    out[1] = color.g;
    out[2] = color.b;
    out[3] = color.a;
}

//! The file name of a path, after its last '/' or '\'.
const char* baseName(const char* path)
{
    const char* name = path;
    for (const char* p = path; *p; ++p)
        if (*p == '/' || *p == '\\')
            name = p + 1;
    return name;
}

bool equalsIgnoreCase(const char* a, const char* b)
{
    for (; *a && *b; ++a, ++b)
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return false;
    return *a == *b;
}

//! The GLSL translation of a Cg file, or 0.
const char* findTranslation(const char* fileName, bool vertex)
{
    if (!fileName)
        return 0;

    const char* name = baseName(fileName);
    for (int i = 0; CgTranslations[i].FileName; ++i)
        if (CgTranslations[i].Vertex == vertex && equalsIgnoreCase(CgTranslations[i].FileName, name))
            return CgTranslations[i].Source;
    return 0;
}

void writeToFile(void* context, void* data, int size)
{
    ((ox::io::IWriteFile*)context)->write(data, size);
}

} // end anonymous namespace

CVideoGL::CVideoGL(const ox::core::CDimension2d<int>& screenSize, ox::IOxDevice* device, ox::io::IFileSystem* io)
    : CVideoNull(io, screenSize), Device(device), Valid(false), CurrentRenderMode(ERM_NONE),
      ResetRenderStates(true), Transformation3DChanged(true), ClampTexture(false), Fullscreen(false),
      LastSetLight(-1), MaxTextureUnits(CFixedFunction::MAX_UNITS), PendingVertexShaderFile(0),
      PendingPixelShaderFile(0)
{
    // the 2D batch state the original zeroes with a memset
    Current2DTexture = 0;
    Current2DQuadCount = 0;
    Current2DAlphaChannel = false;
    Current2DFlag2 = false;
    StatusSwitches = 0;
    NumStatusSwitches = 0;
    for (int i = 0; i < CFixedFunction::MAX_UNITS; ++i)
        CurrentTexture[i] = 0;

    // the quads of the batch as two triangles each, the way OpenGL splits GL_QUADS
    for (int i = 0; i < daisy::video::MAX_2D_QUADS; ++i)
    {
        Indices2D[i * 6 + 0] = (unsigned short)(i * 4 + 0);
        Indices2D[i * 6 + 1] = (unsigned short)(i * 4 + 1);
        Indices2D[i * 6 + 2] = (unsigned short)(i * 4 + 2);
        Indices2D[i * 6 + 3] = (unsigned short)(i * 4 + 0);
        Indices2D[i * 6 + 4] = (unsigned short)(i * 4 + 2);
        Indices2D[i * 6 + 5] = (unsigned short)(i * 4 + 3);
    }

    daisy::os::Printer::log("OpenGL Renderer.", "", ox::event::ELL_INFORMATION);

    if (!gl::load())
        return;

    daisy::os::Printer::log((const char*)glGetString(GL_VERSION), "", ox::event::ELL_INFORMATION);
    daisy::os::Printer::log((const char*)glGetString(GL_VENDOR), "", ox::event::ELL_INFORMATION);
    daisy::os::Printer::log((const char*)glGetString(GL_RENDERER), "", ox::event::ELL_INFORMATION);

    if (!FixedFunction.init())
        return;

    createMaterialRenderers();

    setFog(FogColor, LinearFog, FogStart, FogEnd, FogDensity, PixelFog, RangeFog);

    glViewport(0, 0, PhysicalScreenSize.Width, PhysicalScreenSize.Height);
    Valid = true;
}

//! The material renderers in E_MATERIAL_TYPE order; the seven light-map types share one renderer.
void CVideoGL::createMaterialRenderers()
{
    addAndDropMaterialRenderer(new CMaterialRenderer_SOLID(this));
    addAndDropMaterialRenderer(new CMaterialRenderer_SOLID_2_LAYER(this));

    CMaterialRenderer_LIGHTMAP* lmr = new CMaterialRenderer_LIGHTMAP(this);
    addMaterialRenderer(lmr, 0); // EMT_LIGHTMAP
    addMaterialRenderer(lmr, 0); // EMT_LIGHTMAP_ADD
    addMaterialRenderer(lmr, 0); // EMT_LIGHTMAP_M2
    addMaterialRenderer(lmr, 0); // EMT_LIGHTMAP_M4
    addMaterialRenderer(lmr, 0); // EMT_LIGHTMAP_LIGHTING
    addMaterialRenderer(lmr, 0); // EMT_LIGHTMAP_LIGHTING_M2
    addMaterialRenderer(lmr, 0); // EMT_LIGHTMAP_LIGHTING_M4
    lmr->drop();

    addAndDropMaterialRenderer(new CMaterialRenderer_SPHERE_MAP(this, CMaterialRenderer_SPHERE_MAP::EK_SPHERE_MAP));
    addAndDropMaterialRenderer(
        new CMaterialRenderer_SPHERE_MAP(this, CMaterialRenderer_SPHERE_MAP::EK_REFLECTION_2_LAYER));
    // TRANSPARENT_ADD_COLOR, TRANSPARENT_ALPHA_CHANNEL (with the alpha test), TRANSPARENT_VERTEX_ALPHA
    addAndDropMaterialRenderer(new CMaterialRenderer_TRANSPARENT(this, false));
    addAndDropMaterialRenderer(new CMaterialRenderer_TRANSPARENT(this, true));
    addAndDropMaterialRenderer(new CMaterialRenderer_TRANSPARENT(this, false));
    addAndDropMaterialRenderer(
        new CMaterialRenderer_SPHERE_MAP(this, CMaterialRenderer_SPHERE_MAP::EK_TRANSPARENT_REFLECTION_2_LAYER));
}

CVideoGL::~CVideoGL()
{
    deleteAllTextures();
}

//! Presents the frame through the device (after writing a pending screenshot).
bool CVideoGL::endScene()
{
    CVideoNull::endScene();

    if (PendingScreenshot.size() > 1)
        writeScreenshot();

    return Device->swapBuffers();
}

bool CVideoGL::beginScene(bool backBuffer, bool zBuffer, ox::video::SColor color)
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

void CVideoGL::clearScreen(bool zBuffer, ox::video::SColor color)
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

ox::core::CMatrix4 CVideoGL::getTransform(ox::video::E_TRANSFORMATION_STATE state)
{
    return Matrizes[state];
}

void CVideoGL::loadModelView()
{
    FixedFunction.loadModelView((Matrizes[ox::video::ETS_VIEW] * Matrizes[ox::video::ETS_WORLD]).M);
}

void CVideoGL::loadProjection()
{
    // "flip z to compensate OpenGLs right-hand coordinate system": the original negates element 12
    // (the x translation), which is 0 in the game's perspective matrices
    float m[16];
    memcpy(m, Matrizes[ox::video::ETS_PROJECTION].M, sizeof(m));
    m[12] *= -1.0f;
    FixedFunction.loadProjection(m);
}

//! OpenGL has one modelview matrix, so view and world are multiplied (view * world).
void CVideoGL::setTransform(ox::video::E_TRANSFORMATION_STATE state, const ox::core::CMatrix4& mat)
{
    Transformation3DChanged = true;

    Matrizes[state] = mat;

    switch (state)
    {
    case ox::video::ETS_VIEW:
    case ox::video::ETS_WORLD:
        loadModelView();
        break;
    case ox::video::ETS_PROJECTION:
        loadProjection();
        break;
    default:
        break;
    }
}

void CVideoGL::draw3D(GLenum mode, const SVertexArrays& arrays, const unsigned short* indexList, int indexCount)
{
    setRenderStates3DMode();
    FixedFunction.draw(mode, arrays, indexList, indexCount);
}

void CVideoGL::drawIndexedTriangleList(const ox::video::S3DVertex* vertices, int vertexCount,
    const unsigned short* indexList, int triangleCount)
{
    if (!checkPrimitiveCount(vertexCount))
        return;

    CVideoNull::drawIndexedTriangleList(vertices, vertexCount, indexList, triangleCount);
    draw3D(GL_TRIANGLES, SVertexArrays(vertices, vertexCount), indexList, triangleCount * 3);
}

void CVideoGL::drawIndexedTriangleList(const ox::video::S3DVertex2TCoords* vertices, int vertexCount,
    const unsigned short* indexList, int triangleCount)
{
    if (!checkPrimitiveCount(vertexCount))
        return;

    CVideoNull::drawIndexedTriangleList(vertices, vertexCount, indexList, triangleCount);
    draw3D(GL_TRIANGLES, SVertexArrays(vertices, vertexCount), indexList, triangleCount * 3);
}

void CVideoGL::drawIndexedTriangleFan(const ox::video::S3DVertex* vertices, int vertexCount,
    const unsigned short* indexList, int triangleCount)
{
    if (!checkPrimitiveCount(vertexCount))
        return;

    CVideoNull::drawIndexedTriangleFan(vertices, vertexCount, indexList, triangleCount);
    draw3D(GL_TRIANGLE_FAN, SVertexArrays(vertices, vertexCount), indexList, triangleCount + 2);
}

void CVideoGL::drawIndexedTriangleFan(const ox::video::S3DVertex2TCoords* vertices, int vertexCount,
    const unsigned short* indexList, int triangleCount)
{
    if (!checkPrimitiveCount(vertexCount))
        return;

    CVideoNull::drawIndexedTriangleFan(vertices, vertexCount, indexList, triangleCount);
    draw3D(GL_TRIANGLE_FAN, SVertexArrays(vertices, vertexCount), indexList, triangleCount + 2);
}

//! Sets the 3D matrices when coming from 2D, then lets the material renderers switch: the old one
//! is unset when the type changes, the new one is set when anything in the material changed, and
//! its OnRender runs before every draw call.
void CVideoGL::setRenderStates3DMode()
{
    if (CurrentRenderMode != ERM_3D)
    {
        // switch back the matrices
        loadModelView();
        loadProjection();

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

        // set new material
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

//! Draws the whole texture (its original size) at destPos, opaque white, without its alpha channel.
void CVideoGL::draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<int>& destPos)
{
    if (!texture)
        return;

    draw2DImage(texture, destPos, ox::core::CRect<int>(ox::core::CPosition2d<int>(0, 0), texture->getOriginalSize()),
        0, ox::video::SColor(0xffffffff), false);
}

//! Draws sourceRect of the texture at pos (1:1 pixels), tinted by color, clipped to clipRect and the
//! screen. Texture coordinates are inset by half a texel on every side; positions are not offset.
//! Nearest-neighbour filtering.
void CVideoGL::draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<int>& pos,
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

    Vertices2D[Current2DQuadCount * 4 + 0] = ox::video::S3DVertex(npos.UpperLeftCorner.X, npos.UpperLeftCorner.Y, 0,
        0, 0, 0, color, tcoords.UpperLeftCorner.X, tcoords.UpperLeftCorner.Y);
    Vertices2D[Current2DQuadCount * 4 + 1] = ox::video::S3DVertex(npos.LowerRightCorner.X, npos.UpperLeftCorner.Y, 0,
        0, 0, 0, color, tcoords.LowerRightCorner.X, tcoords.UpperLeftCorner.Y);
    Vertices2D[Current2DQuadCount * 4 + 2] = ox::video::S3DVertex(npos.LowerRightCorner.X, npos.LowerRightCorner.Y,
        0, 0, 0, 0, color, tcoords.LowerRightCorner.X, tcoords.LowerRightCorner.Y);
    Vertices2D[Current2DQuadCount * 4 + 3] = ox::video::S3DVertex(npos.UpperLeftCorner.X, npos.LowerRightCorner.Y, 0,
        0, 0, 0, color, tcoords.UpperLeftCorner.X, tcoords.LowerRightCorner.Y);
    ++Current2DQuadCount;
}

//! Draws the quads of the batch when texture, useAlphaChannel or linearFilter differ from the
//! batch's, or when quads more would not fit, and starts a new batch. Every textured quad is drawn
//! with blending on (see setRenderStates2DMode).
void CVideoGL::switch2dRendering(ox::video::ITexture* texture, bool useAlphaChannel, bool linearFilter, int quads)
{
    if (Current2DTexture != texture || Current2DAlphaChannel != useAlphaChannel || Current2DFlag2 != linearFilter ||
        Current2DQuadCount + quads >= daisy::video::MAX_2D_QUADS)
    {
        // no effect: quads are only batched with a texture
        if (!Current2DTexture)
            Current2DTexture = texture;

        if (Current2DQuadCount > 0)
        {
            setTexture(0, Current2DTexture);
            setRenderStates2DMode(true, true, Current2DAlphaChannel, Current2DFlag2);

            // GL_QUADS, as two triangles per quad
            FixedFunction.draw(GL_TRIANGLES, SVertexArrays(Vertices2D, Current2DQuadCount * 4), Indices2D,
                Current2DQuadCount * 6);
            ++StatusSwitches;
        }

        Current2DTexture = texture;
        Current2DAlphaChannel = useAlphaChannel;
        Current2DQuadCount = 0;
        Current2DFlag2 = linearFilter;
    }
}

//! Like the color version, with a colour per corner (colors[0] upper left, [3] upper right, [2] lower
//! right, [1] lower left; opaque white if colors is 0). Texture coordinates run from half a texel in
//! to half a texel past the source rectangle, and positions are shifted by half a pixel down and
//! right (Irrlicht 0.7's convention).
void CVideoGL::draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<int>& pos,
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

    ox::video::SColor white[4] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
    if (!colors)
        colors = white;

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

    Vertices2D[Current2DQuadCount * 4 + 0] = ox::video::S3DVertex(npos.UpperLeftCorner.X, npos.UpperLeftCorner.Y, 0,
        0, 0, 0, colors[0], tcoords.UpperLeftCorner.X, tcoords.UpperLeftCorner.Y);
    Vertices2D[Current2DQuadCount * 4 + 1] = ox::video::S3DVertex(npos.LowerRightCorner.X, npos.UpperLeftCorner.Y, 0,
        0, 0, 0, colors[3], tcoords.LowerRightCorner.X, tcoords.UpperLeftCorner.Y);
    Vertices2D[Current2DQuadCount * 4 + 2] = ox::video::S3DVertex(npos.LowerRightCorner.X, npos.LowerRightCorner.Y,
        0, 0, 0, 0, colors[2], tcoords.LowerRightCorner.X, tcoords.LowerRightCorner.Y);
    Vertices2D[Current2DQuadCount * 4 + 3] = ox::video::S3DVertex(npos.UpperLeftCorner.X, npos.LowerRightCorner.Y, 0,
        0, 0, 0, colors[1], tcoords.UpperLeftCorner.X, tcoords.LowerRightCorner.Y);
    ++Current2DQuadCount;
}

//! Draws sourceRect scaled by scale with its upper left corner at destPos, through the float corner
//! version (so with linear filtering).
void CVideoGL::drawScaled2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<float>& destPos,
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

    ox::video::SColorArray colors(color);
    draw2DImage(texture, upperLeft, upperRight, lowerLeft, lowerRight, sourceRect, &colors, useAlphaChannelOfTexture);
}

//! Draws sourceRect into the quad corner1 (upper left), corner2 (upper right), corner3 (lower left),
//! corner4 (lower right). Texture coordinates are inset by half a texel; positions are shifted by
//! half a pixel. Linear filtering only when the quad is an axis-aligned rectangle. colors defaults
//! to transparent white (0x00ffffff).
void CVideoGL::draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<int>& corner1,
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

    // the corners are read before the batch is switched
    const ox::core::CPosition2d<int> upperLeft = corner1;
    const ox::core::CPosition2d<int> upperRight = corner2;
    const ox::core::CPosition2d<int> lowerLeft = corner3;
    const ox::core::CPosition2d<int> lowerRight = corner4;

    const float xFact = InvHalfWidth;
    const float yFact = InvHalfHeight;
    const int xPlus = ViewOffsetX;
    const int yPlus = ViewOffsetY;

    bool linearFilter = upperLeft.X == lowerLeft.X && upperLeft.Y == upperRight.Y && lowerLeft.Y == lowerRight.Y &&
        upperRight.X == lowerRight.X;

    switch2dRendering(texture, useAlphaChannelOfTexture, linearFilter, 1);

    ox::video::SColor transparentWhite[4] = {0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00ffffff};
    if (!colors)
        colors = transparentWhite;

    Vertices2D[Current2DQuadCount * 4 + 0] = ox::video::S3DVertex(((float)(upperLeft.X + xPlus) + 0.5f) * xFact,
        ((float)(yPlus - upperLeft.Y) + 0.5f) * yFact, 0, 0, 0, 0, colors[0], tcoords.UpperLeftCorner.X,
        tcoords.UpperLeftCorner.Y);
    Vertices2D[Current2DQuadCount * 4 + 1] = ox::video::S3DVertex(((float)(upperRight.X + xPlus) + 0.5f) * xFact,
        ((float)(yPlus - upperRight.Y) + 0.5f) * yFact, 0, 0, 0, 0, colors[3], tcoords.LowerRightCorner.X,
        tcoords.UpperLeftCorner.Y);
    Vertices2D[Current2DQuadCount * 4 + 2] = ox::video::S3DVertex(((float)(lowerRight.X + xPlus) + 0.5f) * xFact,
        ((float)(yPlus - lowerRight.Y) + 0.5f) * yFact, 0, 0, 0, 0, colors[2], tcoords.LowerRightCorner.X,
        tcoords.LowerRightCorner.Y);
    Vertices2D[Current2DQuadCount * 4 + 3] = ox::video::S3DVertex(((float)(lowerLeft.X + xPlus) + 0.5f) * xFact,
        ((float)(yPlus - lowerLeft.Y) + 0.5f) * yFact, 0, 0, 0, 0, colors[1], tcoords.UpperLeftCorner.X,
        tcoords.LowerRightCorner.Y);
    ++Current2DQuadCount;
}

//! Float version of the corner draw, for rotated and scaled images: no pixel offset, always linear
//! filtering; colors defaults to transparent white (0x00ffffff).
void CVideoGL::draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<float>& corner1,
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

    // the corners are read before the batch is switched
    const ox::core::CPosition2d<float> upperLeft = corner1;
    const ox::core::CPosition2d<float> upperRight = corner2;
    const ox::core::CPosition2d<float> lowerLeft = corner3;
    const ox::core::CPosition2d<float> lowerRight = corner4;

    const float xPlus = (float)ViewOffsetX;
    const float xFact = InvHalfWidth;
    const float yPlus = (float)ViewOffsetY;
    const float yFact = InvHalfHeight;

    switch2dRendering(texture, useAlphaChannelOfTexture, true, 1);

    const ox::video::SColorArray transparentWhite(ox::video::SColor(0x00ffffff));
    if (!colors)
        colors = &transparentWhite;

    Vertices2D[Current2DQuadCount * 4 + 0] = ox::video::S3DVertex((upperLeft.X + xPlus) * xFact,
        (yPlus - upperLeft.Y) * yFact, 0, 0, 0, 0, colors->Colors[0], tcoords.UpperLeftCorner.X,
        tcoords.UpperLeftCorner.Y);
    Vertices2D[Current2DQuadCount * 4 + 1] = ox::video::S3DVertex((upperRight.X + xPlus) * xFact,
        (yPlus - upperRight.Y) * yFact, 0, 0, 0, 0, colors->Colors[3], tcoords.LowerRightCorner.X,
        tcoords.UpperLeftCorner.Y);
    Vertices2D[Current2DQuadCount * 4 + 2] = ox::video::S3DVertex((lowerRight.X + xPlus) * xFact,
        (yPlus - lowerRight.Y) * yFact, 0, 0, 0, 0, colors->Colors[2], tcoords.LowerRightCorner.X,
        tcoords.LowerRightCorner.Y);
    Vertices2D[Current2DQuadCount * 4 + 3] = ox::video::S3DVertex((lowerLeft.X + xPlus) * xFact,
        (yPlus - lowerLeft.Y) * yFact, 0, 0, 0, 0, colors->Colors[1], tcoords.UpperLeftCorner.X,
        tcoords.LowerRightCorner.Y);
    ++Current2DQuadCount;
}

void CVideoGL::drawPrimitive2D(GLenum mode, const ox::core::CPosition2d<float>* positions, int count,
    ox::video::SColor color)
{
    ox::video::S3DVertex vertices[4];
    for (int i = 0; i < count; ++i)
        vertices[i] = ox::video::S3DVertex(positions[i].X, positions[i].Y, 0, 0, 0, 0, color, 0, 0);

    if (mode == GL_LINES)
        FixedFunction.draw(GL_LINES, SVertexArrays(vertices, count), 0, 0);
    else
        // GL_QUADS, as two triangles
        FixedFunction.draw(GL_TRIANGLES, SVertexArrays(vertices, count), Indices2D, 6);
}

//! A line between pixel positions; blended when the colour is translucent. The original asks for
//! 3 px inside glBegin/glEnd, where OpenGL ignores it: lines are 1 px wide (original-bugs.md).
void CVideoGL::draw2DLine(const ox::core::CPosition2d<int>& start, const ox::core::CPosition2d<int>& end,
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

    ox::core::CPosition2d<float> npos[2];
    npos[0].X = (float)(start.X + xPlus) * xFact;
    npos[0].Y = (float)(yPlus - start.Y) * yFact;
    npos[1].X = (float)(end.X + xPlus) * xFact;
    npos[1].Y = (float)(yPlus - end.Y) * yFact;

    drawPrimitive2D(GL_LINES, npos, 2, color);
}

//! A line between float positions, with linear filtering states; see draw2DLine.
void CVideoGL::draw2DLineFloat(const ox::core::CPosition2d<float>& start, const ox::core::CPosition2d<float>& end,
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

    ox::core::CPosition2d<float> npos[2];
    npos[0].X = (start.X + xPlus) * xFact;
    npos[0].Y = (yPlus - start.Y) * yFact;
    npos[1].X = (end.X + xPlus) * xFact;
    npos[1].Y = (yPlus - end.Y) * yFact;

    drawPrimitive2D(GL_LINES, npos, 2, color);
}

//! Sets the 2D render states. Textured images always come from the batch with alpha = true: with
//! alphaChannel they are blended with (SRC_ALPHA, ONE_MINUS_SRC_ALPHA) after the alpha test
//! alpha > 0; without it blending is enabled but the blend function and the texture environment are
//! whatever was set last. Untextured primitives blend with (SRC_ALPHA, ONE_MINUS_SRC_ALPHA) when
//! alpha.
void CVideoGL::setRenderStates2DMode(bool alpha, bool texture, bool alphaChannel, bool linearFilter)
{
    if (CurrentRenderMode != ERM_2D || Transformation3DChanged)
    {
        FixedFunction.loadIdentities();

        Transformation3DChanged = false;

        glDisable(GL_DEPTH_TEST);
        FixedFunction.setFog(false);
        // Port: glPolygonMode(GL_FILL) has no equivalent (OpenGL ES draws filled polygons only)
        FixedFunction.setLighting(false);

        activeTexture(0);

        FixedFunction.setTexGenSphereMap(false);

        ClampTexture = false;

        // unset last 3d material
        if (CurrentRenderMode == ERM_3D && LastMaterial.MaterialType >= 0 &&
            LastMaterial.MaterialType < (int)MaterialRenderers.size())
            MaterialRenderers[LastMaterial.MaterialType].Renderer->OnUnsetMaterial();
    }

    glDisable(GL_CULL_FACE);

    if (texture)
    {
        if (linearFilter && !ForcePointSampling)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }

        if (alphaChannel)
        {
            FixedFunction.setTexEnvMode(ETEM_MODULATE);
            glEnable(GL_BLEND);
            FixedFunction.setAlphaTest(true);
            FixedFunction.setAlphaRef(0.0f);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        else
        {
            if (alpha)
            {
                FixedFunction.setAlphaTest(false);
                glEnable(GL_BLEND);
            }
            else
            {
                FixedFunction.setTexEnvMode(ETEM_MODULATE);
                FixedFunction.setAlphaTest(false);
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
            FixedFunction.setTexEnvMode(ETEM_MODULATE);
            FixedFunction.setAlphaTest(false);
        }
        else
        {
            FixedFunction.setTexEnvMode(ETEM_MODULATE);
            glDisable(GL_BLEND);
            FixedFunction.setAlphaTest(false);
        }
    }

    CurrentRenderMode = ERM_2D;
}

void CVideoGL::activeTexture(int unit)
{
    FixedFunction.activeTexture(unit);
}

//! Binds the texture to a unit; textures already bound are skipped.
bool CVideoGL::setTexture(int stage, ox::video::ITexture* texture)
{
    if (stage >= MaxTextureUnits)
        return false;

    if (CurrentTexture[stage] == texture && texture)
        return true;

    activeTexture(stage);

    CurrentTexture[stage] = texture;

    if (!texture)
    {
        FixedFunction.setTexture2D(false);
        return true;
    }

    if (texture->getDriverType() != ox::video::EDT_OPENGL)
    {
        FixedFunction.setTexture2D(false);
        daisy::os::Printer::log("Fatal Error: Tried to set a texture not owned by this driver.", "",
            ox::event::ELL_ERROR);
        return false;
    }

    FixedFunction.setTexture2D(true);
    glBindTexture(GL_TEXTURE_2D, ((CTextureGL*)texture)->getTextureName());
    return true;
}

//! A filled rectangle (not batched), clipped to clip; blended when the colour is translucent.
void CVideoGL::draw2DRectangle(ox::video::SColor color, const ox::core::CRect<int>& position,
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

    ox::core::CPosition2d<float> corners[4] = {
        ox::core::CPosition2d<float>(npos.UpperLeftCorner.X, npos.UpperLeftCorner.Y),
        ox::core::CPosition2d<float>(npos.LowerRightCorner.X, npos.UpperLeftCorner.Y),
        ox::core::CPosition2d<float>(npos.LowerRightCorner.X, npos.LowerRightCorner.Y),
        ox::core::CPosition2d<float>(npos.UpperLeftCorner.X, npos.LowerRightCorner.Y),
    };
    drawPrimitive2D(GL_TRIANGLES, corners, 4, color);
}

//! Port: the original reports the extensions of the OpenGL 1.x context; these are what this
//! renderer supports. No render targets (setRenderTarget does nothing, as on Linux), no stencil
//! buffer, no ARB assembly or GLSL materials (only the Cg materials, through their translations).
bool CVideoGL::queryFeature(ox::video::E_VIDEO_DRIVER_FEATURE feature)
{
    switch (feature)
    {
    case ox::video::EVDF_MULTITEXTURE:
    case ox::video::EVDF_BILINEAR_FILTER:
    case ox::video::EVDF_MIP_MAP:
    case ox::video::EVDF_MIP_MAP_AUTO_UPDATE:
    case ox::video::EVDF_TEXTURE_NPOT:
        return true;
    default:
        return false;
    }
}

bool CVideoGL::disableTextures(int fromStage)
{
    bool result = true;
    for (int i = fromStage; i < MaxTextureUnits; ++i)
        result &= setTexture(i, 0);
    return result;
}

//! Textures get mip maps when ETCF_CREATE_MIP_MAPS is set (the game clears it).
ox::video::ITexture* CVideoGL::createDeviceDependentTexture(ox::video::IImage* surface)
{
    return new CTextureGL(surface, getTextureCreationFlag(ox::video::ETCF_CREATE_MIP_MAPS));
}

void CVideoGL::setMaterial(const ox::video::SMaterial& material)
{
    Material = material;

    for (int i = 0; i < ox::video::MATERIAL_MAX_TEXTURES; ++i)
        setTexture(i, Material.Textures[i]);
}

//! Applies the parts of material that differ from lastmaterial (all of them when
//! resetAllRenderstates). Original bug (from Irrlicht 0.7), kept: the material colours, the
//! shininess, the magnification filter and lighting are taken from the driver's current Material
//! rather than from material; they are the same object whenever the driver itself calls this.
void CVideoGL::setBasicRenderStates(const ox::video::SMaterial& material, const ox::video::SMaterial& lastmaterial,
    bool resetAllRenderstates)
{
    if (resetAllRenderstates || ox::video::colorsDiffer(lastmaterial.AmbientColor, material.AmbientColor) ||
        ox::video::colorsDiffer(lastmaterial.DiffuseColor, material.DiffuseColor) ||
        ox::video::colorsDiffer(lastmaterial.SpecularColor, material.SpecularColor) ||
        ox::video::colorsDiffer(lastmaterial.EmissiveColor, material.EmissiveColor) ||
        lastmaterial.Shininess != material.Shininess)
    {
        float ambient[4], diffuse[4], specular[4], emission[4];
        toFloats(ambient, Material.AmbientColor);
        toFloats(diffuse, Material.DiffuseColor);
        toFloats(specular, Material.SpecularColor);
        toFloats(emission, Material.EmissiveColor);
        FixedFunction.setMaterialColors(ambient, diffuse, specular, emission, Material.Shininess);
    }

    // bilinear
    if (resetAllRenderstates || lastmaterial.BilinearFilter != material.BilinearFilter)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Material.BilinearFilter ? GL_LINEAR : GL_NEAREST);

    // fill mode: Port: wireframe (glPolygonMode GL_LINE) has no OpenGL ES equivalent; the game never
    // sets it

    // lighting
    if (resetAllRenderstates || lastmaterial.Lighting != material.Lighting)
        FixedFunction.setLighting(Material.Lighting);

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
        glDepthMask(material.ZWriteEnable ? GL_TRUE : GL_FALSE);

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
        glFrontFace(material.FrontFaceCCW ? GL_CCW : GL_CW);

    // fog
    if (resetAllRenderstates || lastmaterial.FogEnable != material.FogEnable)
        FixedFunction.setFog(material.FogEnable);

    // texture wrap (the mirrored-repeat extension is always there)
    if (resetAllRenderstates || lastmaterial.TextureMirrorU != material.TextureMirrorU ||
        lastmaterial.TextureMirrorV != material.TextureMirrorV)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, material.TextureMirrorU ? GL_MIRRORED_REPEAT : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, material.TextureMirrorV ? GL_MIRRORED_REPEAT : GL_REPEAT);
    }
}

void CVideoGL::flush2dRendering()
{
    switch2dRendering(0, false, false, daisy::video::MAX_2D_QUADS);
}

//! Flushes the 2D batch and restricts drawing to rect (0: no scissor). Port: OpenGL's window y grows
//! upwards, so the scissor starts at height - LowerRightCorner.Y; the original's LowerRightCorner.Y
//! - height is only right for rectangles that end at the bottom (original-bugs.md).
void CVideoGL::setScissorRect(ox::core::CRect<int>* rect)
{
    flush2dRendering();

    if (rect)
    {
        glEnable(GL_SCISSOR_TEST);
        glScissor(rect->UpperLeftCorner.X, PhysicalScreenSize.Height - rect->LowerRightCorner.Y, rect->getWidth(),
            rect->getHeight());
    }
    else
        glDisable(GL_SCISSOR_TEST);
}

const wchar_t* CVideoGL::getName()
{
    return L"OpenGL ES 3.0";
}

void CVideoGL::deleteAllDynamicLights()
{
    for (int i = 0; i < LastSetLight + 1 && i < CFixedFunction::MAX_LIGHTS; ++i)
        FixedFunction.setLightEnabled(i, false);

    LastSetLight = -1;

    CVideoNull::deleteAllDynamicLights();
}

//! Adds a light with linear attenuation 1 / Radius. Port: the limit is the eight lights the shader
//! evaluates; the original compares with the enum value GL_MAX_LIGHTS (original-bugs.md).
void CVideoGL::addDynamicLight(const ox::video::SLight& light)
{
    ++LastSetLight;
    if (!(LastSetLight < CFixedFunction::MAX_LIGHTS))
        return;

    setTransform(ox::video::ETS_WORLD, ox::core::CMatrix4());

    CVideoNull::addDynamicLight(light);

    float position[4] = {light.Position.X, light.Position.Y, light.Position.Z, light.Directional ? 0.0f : 1.0f};
    FixedFunction.setLightPosition(LastSetLight, position);

    float ambient[4], diffuse[4], specular[4];
    toFloats(diffuse, light.DiffuseColor);
    toFloats(specular, light.SpecularColor);
    toFloats(ambient, light.AmbientColor);
    FixedFunction.setLightColors(LastSetLight, ambient, diffuse, specular);

    // 1.0f / (constant + linear * d + quadratic * d * d)
    FixedFunction.setLightAttenuation(LastSetLight, 0.0f, 1.0f / light.Radius, 0.0f);

    FixedFunction.setLightEnabled(LastSetLight, true);
}

int CVideoGL::getMaximalDynamicLightAmount()
{
    return CFixedFunction::MAX_LIGHTS;
}

void CVideoGL::setAmbientLight(const ox::video::SColorf& color)
{
    float data[4];
    toFloats(data, color);
    FixedFunction.setLightModelAmbient(data);
}

//! Clips area to the screen; OpenGL's viewport y counts from the bottom.
void CVideoGL::setViewPort(const ox::core::CRect<int>& area)
{
    ox::core::CRect<int> vp = area;
    ox::core::CRect<int> rendert(0, 0, ScreenSize.Width, ScreenSize.Height);
    clipAgainst(vp, rendert);

    if (vp.getHeight() > 0 && vp.getWidth() > 0)
        glViewport(vp.UpperLeftCorner.X, ScreenSize.Height - vp.UpperLeftCorner.Y - vp.getHeight(), vp.getWidth(),
            vp.getHeight());

    ViewPort = vp;
}

//! Port: stencil shadows need a stencil buffer, which the port's context does not ask for (the
//! original returns here too without one); the game draws none.
void CVideoGL::drawStencilShadowVolume(const ox::core::CVector3d<float>* triangles, int count, bool zfail)
{
}

void CVideoGL::drawStencilShadow(bool clearStencilBuffer, ox::video::SColor leftUpEdge, ox::video::SColor rightUpEdge,
    ox::video::SColor leftDownEdge, ox::video::SColor rightDownEdge)
{
}

void CVideoGL::setFog(ox::video::SColor c, bool linearFog, float start, float end, float density, bool pixelFog,
    bool rangeFog)
{
    CVideoNull::setFog(c, linearFog, start, end, density, pixelFog, rangeFog);

    float color[4];
    toFloats(color, ox::video::SColorf(c));
    FixedFunction.setFogMode(linearFog, start, end, density, color);
}

void CVideoGL::draw3DLine(const ox::core::CVector3d<float>& start, const ox::core::CVector3d<float>& end,
    ox::video::SColor color)
{
    setRenderStates3DMode();
    setTexture(0, 0);

    ox::video::S3DVertex vertices[2];
    vertices[0] = ox::video::S3DVertex(start.X, start.Y, start.Z, 0, 0, 0, color, 0, 0);
    vertices[1] = ox::video::S3DVertex(end.X, end.Y, end.Z, 0, 0, 0, color, 0, 0);
    FixedFunction.draw(GL_LINES, SVertexArrays(vertices, 2), 0, 0);
}

void CVideoGL::OnResize(const ox::core::CDimension2d<int>& size)
{
    CVideoNull::OnResize(size);
    glViewport(0, 0, size.Width, size.Height);
}

int CVideoGL::getDriverType()
{
    return ox::video::EDT_OPENGL;
}

//! ARB assembly constants have no equivalent; the original sets ARB program parameters here.
void CVideoGL::setVertexShaderConstant(const float* data, int startRegister, int constantAmount)
{
}

void CVideoGL::setPixelShaderConstant(const float* data, int startRegister, int constantAmount)
{
}

//! Original bug, kept: forwards to the pixel shader version, which only logs.
bool CVideoGL::setVertexShaderConstant(const char* name, const float* floats, int count)
{
    return setPixelShaderConstant(name, floats, count);
}

bool CVideoGL::setPixelShaderConstant(const char* name, const float* floats, int count)
{
    daisy::os::Printer::log(
        "Error: Please call services->setPixelShaderConstant(), not VideoDriver->setPixelShaderConstant().", "",
        ox::event::ELL_INFORMATION);
    return false;
}

//! Port: ARB assembly and GLSL 1.10 materials are not supported (the game uses neither).
int CVideoGL::addShaderMaterial(const char* vertexShaderProgram, const char* pixelShaderProgram,
    ox::video::IShaderConstantSetCallBack* callback, ox::video::E_MATERIAL_TYPE baseMaterial, int userData)
{
    daisy::os::Printer::log("Shader materials are not supported by this renderer.", "", ox::event::ELL_WARNING);
    return -1;
}

int CVideoGL::addHighLevelShaderMaterial(const char* vertexShaderProgram, const char* vertexShaderEntryPointName,
    ox::video::E_VERTEX_SHADER_TYPE vsCompileTarget, const char* pixelShaderProgram,
    const char* pixelShaderEntryPointName, ox::video::E_PIXEL_SHADER_TYPE psCompileTarget,
    ox::video::IShaderConstantSetCallBack* callback, ox::video::E_MATERIAL_TYPE baseMaterial, int userData)
{
    daisy::os::Printer::log("High level shader materials are not supported by this renderer.", "",
        ox::event::ELL_WARNING);
    return -1;
}

int CVideoGL::addCgShaderMaterialFromFiles(const char* vertexShaderProgramFileName,
    const char* vertexShaderEntryPointName, const char* pixelShaderProgramFileName,
    const char* pixelShaderEntryPointName, ox::video::IShaderConstantSetCallBack* callback,
    ox::video::E_MATERIAL_TYPE baseMaterial, bool precompiled, int userData)
{
    PendingVertexShaderFile = vertexShaderProgramFileName;
    PendingPixelShaderFile = pixelShaderProgramFileName;

    int result = CVideoNull::addCgShaderMaterialFromFiles(vertexShaderProgramFileName, vertexShaderEntryPointName,
        pixelShaderProgramFileName, pixelShaderEntryPointName, callback, baseMaterial, precompiled, userData);

    PendingVertexShaderFile = 0;
    PendingPixelShaderFile = 0;
    return result;
}

//! Creates the material from the GLSL translations of the Cg files being loaded; returns the new
//! material type, or -1.
int CVideoGL::addCgShaderMaterial(const char* vertexShaderProgram, const char* vertexShaderEntryPointName,
    const char* pixelShaderProgram, const char* pixelShaderEntryPointName,
    ox::video::IShaderConstantSetCallBack* callback, ox::video::E_MATERIAL_TYPE baseMaterial, bool precompiled,
    int userData)
{
    const char* vertexSource = findTranslation(PendingVertexShaderFile, true);
    const char* pixelSource = findTranslation(PendingPixelShaderFile, false);
    if (!vertexSource || !pixelSource)
    {
        daisy::os::Printer::log("No GLSL translation of the Cg shader",
            PendingVertexShaderFile ? PendingVertexShaderFile : "(source)", ox::event::ELL_WARNING);
        return -1;
    }

    int nr = -1;
    CCgMaterialRendererGL* r = new CCgMaterialRendererGL(this, nr, vertexSource, pixelSource,
        baseName(PendingVertexShaderFile), callback, getMaterialRenderer(baseMaterial), userData);

    r->drop();
    return nr;
}

ox::video::IVideoDriver* CVideoGL::getVideoDriver()
{
    return this;
}

void* CVideoGL::getGPUProgrammingServices()
{
    return static_cast<ox::video::IGPUProgrammingServices*>(this);
}

void* CVideoGL::getPostProcessingServices()
{
    return static_cast<ox::video::IPostProcessingServices*>(this);
}

//! An empty texture (no OpenGL name); size is ignored.
ox::video::ITexture* CVideoGL::createScreenTexture(const ox::core::CDimension2d<int>& size)
{
    return new CTextureGL(0, false);
}

//! Not supported: returns true without changing the target, so the game's minimap draws straight
//! to the screen, as on Linux.
bool CVideoGL::setRenderTarget(ox::video::ITexture* texture, bool clearBackBuffer, bool clearZBuffer,
    ox::video::SColor color)
{
    return true;
}

void CVideoGL::captureScreenBuffer(unsigned int index, const ox::core::CRect<int>& area)
{
}

bool CVideoGL::isFullscreen()
{
    return Fullscreen;
}

//! Records the mode (the device switches the window) and resets the 3D render states.
bool CVideoGL::setFullscreen(bool fullscreen)
{
    Fullscreen = fullscreen;
    setRenderStates3DMode();
    return true;
}

//! Chooses directory + (name or "Screen-") + yymmdd + "-NN" + ".jpg", with the first number NN (two
//! digits from 00, more past 99) that does not exist yet. Returns false, as the original does.
//! Port: the frame is read at the next endScene, before it is presented; the original read the back
//! buffer at once, which after a swap (the game calls this from an event handler) holds an
//! undefined image on most platforms.
bool CVideoGL::saveJpegScreenshot(const char* directory, const char* name)
{
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

    PendingScreenshot = path;
    return false;
}

//! Reads the viewport as RGB, bottom row last, and writes it as a JPEG at quality 100 (the
//! original's libjpeg setting).
void CVideoGL::writeScreenshot()
{
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    const int width = viewport[2];
    const int height = viewport[3];

    // OpenGL ES reads RGBA only
    std::vector<unsigned char> pixels(width * height * 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(viewport[0], viewport[1], width, height, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);

    // OpenGL's rows run bottom up; flip them and drop alpha
    std::vector<unsigned char> rgb(width * height * 3);
    for (int y = 0; y < height; ++y)
    {
        const unsigned char* source = &pixels[(height - 1 - y) * width * 4];
        unsigned char* target = &rgb[y * width * 3];
        for (int x = 0; x < width; ++x)
        {
            target[x * 3 + 0] = source[x * 4 + 0];
            target[x * 3 + 1] = source[x * 4 + 1];
            target[x * 3 + 2] = source[x * 4 + 2];
        }
    }

    ox::io::IWriteFile* file = FileSystem->createAndWriteFile(PendingScreenshot.c_str(), false);
    if (file)
    {
        stbi_write_jpg_to_func(writeToFile, file, width, height, 3, &rgb[0], 100);
        file->drop();
    }
    else
        daisy::os::Printer::log("Could not write screenshot", PendingScreenshot.c_str(), ox::event::ELL_ERROR);

    PendingScreenshot = "";
}

} // end namespace video

ox::video::IVideoDriver* createVideoDriver(ox::IOxDevice* device, ox::io::IFileSystem* fileSystem,
    const ox::core::CDimension2d<int>& screenSize)
{
    video::CVideoGL* driver = new video::CVideoGL(screenSize, device, fileSystem);
    if (!driver->isValid())
    {
        driver->drop();
        return 0;
    }
    return driver;
}

} // end namespace port
