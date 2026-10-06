// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CVideoNull.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#include "CVideoNull.h"
#include "CImage.h"
#include "CParticlePackage.h"
#include "CSpritePackage.h"
#include "daisy/os.h"
#include "daisy/video/Software/CSoftwareTexture.h"
#include "ox/algo/CArrayFunctions.h"
#include "ox/core/CAabbox3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CStringFunctions.h"
#include "ox/core/CTriangle3d.h"
#include "ox/io/IFileSystem.h"
#include "ox/io/IReadFile.h"
#include "ox/scene/IMeshBuffer.h"
#include "ox/video/IImageLoader.h"
#include "ox/video/IMaterialRenderer.h"
#include "ox/video/ITexture.h"
#include <stdio.h>
#include <string.h>
// The native object includes an iostream static initializer.
#include <iostream> // IWYU pragma: keep

extern "C" {
CGcontext cgCreateContext();
void cgDestroyContext(CGcontext context);
}

namespace daisy {
namespace video {

using namespace ox;
using namespace ox::video;
using ox::event::ELL_INFORMATION;
using ox::event::ELL_WARNING;
using ox::event::ELL_ERROR;

IImageLoader* createImageLoaderBmp();
IImageLoader* createImageLoaderJPG();
IImageLoader* createImageLoaderTGA();
IImageLoader* createImageLoaderPSD();
IImageLoader* createImageLoaderPCX();

//! Names of the built-in material renderers, in E_MATERIAL_TYPE order.
static const char* const sBuiltInMaterialTypeNames[] =
{
    "solid",
    "solid_2layer",
    "lightmap",
    "lightmap_add",
    "lightmap_m2",
    "lightmap_m4",
    "lightmap_light",
    "lightmap_light_m2",
    "lightmap_light_m4",
    "sphere_map",
    "reflection_2layer",
    "trans_add",
    "trans_alphach",
    "trans_vertex_alpha",
    "trans_reflection_2layer",
    0
};

//! An A8R8G8B8 color as A1R5G5B5; any nonzero alpha sets the alpha bit.
static inline short toA1R5G5B5(SColor color)
{
    unsigned int c = color.color;
    return (short)(((c & 0xff000000) ? 0x8000 : 0) | ((c >> 9) & 0x7c00) | ((c >> 6) & 0x3e0) | ((c >> 3) & 0x1f));
}

CVideoNull::CVideoNull(ox::io::IFileSystem* io, const core::CDimension2d<int>& screenSize)
    : InvHalfWidth(0), InvHalfHeight(0), ViewOffsetX(0),
      ViewOffsetY(0), FileSystem(io), ViewPort(0, 0, 0, 0), ScreenSize(screenSize),
      PhysicalScreenSize(screenSize), PrimitivesDrawn(0), TextureCreationFlags(0), UseMaterialShaderFor2D(false),
      ForcePointSampling(false)
{
    // The 2d batch state (Current2DTexture up to StatusSwitches) is zeroed by the derived driver.
    for (int i = 0; i < 8; ++i)
        PPSurfaces[i] = 0;
    InputTextures[0] = 0;
    InputTextures[1] = 0;
    NumPPSurfaces = 0;
    Unknown431b0 = 0;
    DebugColors[0] = SColor(0xff0000);
    DebugColors[1] = SColor(0xffff00);
    DebugColors[2] = SColor(0xff00);

    update2dViewValues(0, 0);
    setFog(SColor(0, 255, 255, 255), true, 50.0f, 100.0f, 0.01f, false, false);

    setTextureCreationFlag(ETCF_OPTIMIZED_FOR_SPEED, true);
    setTextureCreationFlag(ETCF_CREATE_MIP_MAPS, false);

    ViewPort = core::CRect<int>(core::CPosition2d<int>(0, 0), screenSize);

    if (FileSystem)
        FileSystem->grab();

    SurfaceLoader.push_back(createImageLoaderBmp());
    SurfaceLoader.push_back(createImageLoaderJPG());
    SurfaceLoader.push_back(createImageLoaderTGA());
    SurfaceLoader.push_back(createImageLoaderPSD());
    SurfaceLoader.push_back(createImageLoaderPCX());

    memset(&ExposedData, 0, sizeof(ExposedData));
#ifdef HARVEST_PORT
    // The port has no Cg runtime: its renderer translates the Cg shaders to GLSL.
    CgContext = 0;
#else
    CgContext = cgCreateContext();
#endif
}

CVideoNull::~CVideoNull()
{
    if (FileSystem)
        FileSystem->drop();

    deleteAllTextures();
    removeAllParticlePackages();
    removeAllSpritePackages();

    for (int i = 0; i < (int)SurfaceLoader.size(); ++i)
        SurfaceLoader[i]->drop();

    freePPSurfaces();
    deleteMaterialRenders();
#ifndef HARVEST_PORT
    cgDestroyContext(CgContext);
#endif
}

void CVideoNull::deleteAllTextures()
{
    for (unsigned int i = 0; i < Textures.size(); ++i)
        Textures[i].Surface->drop();

    Textures.clear();
}

void CVideoNull::deleteMaterialRenders()
{
    for (int i = 0; i < (int)MaterialRenderers.size(); ++i)
        if (MaterialRenderers[i].Renderer)
            MaterialRenderers[i].Renderer->drop();

    MaterialRenderers.clear();
}

void CVideoNull::addExternalImageLoader(IImageLoader* loader)
{
    if (!loader)
        return;

    loader->grab();
    SurfaceLoader.push_back(loader);
}

bool CVideoNull::beginScene(bool backBuffer, bool zBuffer, SColor color)
{
    PrimitivesDrawn = 0;
    StatusSwitches = 0;
    return true;
}

bool CVideoNull::endScene()
{
    FPSCounter.registerFrame(os::Timer::getTime());
    flush2dRendering();
    NumStatusSwitches = StatusSwitches;
    return true;
}

bool CVideoNull::queryFeature(E_VIDEO_DRIVER_FEATURE feature)
{
    return false;
}

void CVideoNull::setTransform(E_TRANSFORMATION_STATE state, const core::CMatrix4& mat)
{
}

core::CMatrix4 CVideoNull::getTransform(E_TRANSFORMATION_STATE state)
{
    return core::CMatrix4();
}

void CVideoNull::setMaterial(const SMaterial& material)
{
}

void CVideoNull::removeTexture(ITexture* texture)
{
    for (unsigned int i = 0; i < Textures.size(); ++i)
        if (Textures[i].Surface == texture)
        {
            texture->drop();
            Textures.erase(algo::advanceIterator(Textures.begin(), i));
            return;
        }
}

void CVideoNull::removeTexture(const char* name)
{
    for (unsigned int i = 0; i < Textures.size(); ++i)
        if (Textures[i].Filename.equals_ignore_case(core::CString<char>(name)))
        {
            Textures[i].Surface->drop();
            Textures.erase(algo::advanceIterator(Textures.begin(), i));
            return;
        }
}

void CVideoNull::removeAllTextures()
{
    deleteAllTextures();
}

ISpritePackage* CVideoNull::getSpritePackage(const char* filename, bool keepStates)
{
    ISpritePackage* package = findSpritePackage(filename);
    if (package)
        return package;

    ox::io::IReadFile* file = FileSystem->createAndOpenFile(filename);
    if (file)
    {
        CSpritePackage* sprites = new CSpritePackage(this, keepStates);
        if (sprites->load(file, filename))
        {
            SSprites s;
            s.Filename = filename;
            s.Package = sprites;
            core::CStringFunctions::ansiMakeLower(s.Filename);
            SpritePackages.push_back(s);
            algo::sort(SpritePackages.begin(), SpritePackages.end());
            os::Printer::log("Loaded sprite package", filename, ELL_INFORMATION);
            package = sprites;
        }
        else
            sprites->drop();
        file->drop();
    }
    else
        os::Printer::log("Could not open file of sprite package", filename, ELL_ERROR);

    if (!package)
        os::Printer::log("Could not load sprite package", filename, ELL_ERROR);

    return package;
}

IParticlePackage* CVideoNull::getParticlePackage(const char* filename)
{
    IParticlePackage* package = findParticlePackage(filename);
    if (package)
        return package;

    ox::io::IReadFile* file = FileSystem->createAndOpenFile(filename);
    if (file)
    {
        CParticlePackage* particles = new CParticlePackage(this);
        if (particles->load(file))
        {
            SParticles s;
            s.Filename = filename;
            s.Package = particles;
            core::CStringFunctions::ansiMakeLower(s.Filename);
            ParticlePackages.push_back(s);
            algo::sort(ParticlePackages.begin(), ParticlePackages.end());
            os::Printer::log("Loaded particle package", file->getFileName(), ELL_INFORMATION);
            package = particles;
        }
        else
            particles->drop();
        file->drop();
    }
    else
        os::Printer::log("Could not open file of particle package", filename, ELL_ERROR);

    if (!package)
        os::Printer::log("Could not load particle package", filename, ELL_ERROR);

    return package;
}

void CVideoNull::removeSpritePackage(const char* filename)
{
    for (unsigned int i = 0; i < SpritePackages.size(); ++i)
        if (SpritePackages[i].Filename.equals_ignore_case(core::CString<char>(filename)))
        {
            SpritePackages[i].Package->drop();
            SpritePackages.erase(algo::advanceIterator(SpritePackages.begin(), i));
            return;
        }
}

void CVideoNull::removeParticlePackage(const char* filename)
{
    for (unsigned int i = 0; i < ParticlePackages.size(); ++i)
        if (ParticlePackages[i].Filename.equals_ignore_case(core::CString<char>(filename)))
        {
            ParticlePackages[i].Package->drop();
            ParticlePackages.erase(algo::advanceIterator(ParticlePackages.begin(), i));
            return;
        }
}

void CVideoNull::removeAllSpritePackages()
{
    for (unsigned int i = 0; i < SpritePackages.size(); ++i)
        SpritePackages[i].Package->drop();

    SpritePackages.clear();
}

void CVideoNull::removeAllParticlePackages()
{
    for (unsigned int i = 0; i < ParticlePackages.size(); ++i)
        ParticlePackages[i].Package->drop();

    ParticlePackages.clear();
}

ISpritePackage* CVideoNull::findSpritePackage(const char* filename)
{
    SSprites s;
    if (!filename)
        filename = "";
    s.Filename = filename;
    core::CStringFunctions::ansiMakeLower(s.Filename);

    int index = algo::binarySearchPos(SpritePackages.begin(), SpritePackages.end(), s);
    if (index != -1)
        return SpritePackages[index].Package;

    return 0;
}

IParticlePackage* CVideoNull::findParticlePackage(const char* filename)
{
    SParticles s;
    if (!filename)
        filename = "";
    s.Filename = filename;
    core::CStringFunctions::ansiMakeLower(s.Filename);

    int index = algo::binarySearchPos(ParticlePackages.begin(), ParticlePackages.end(), s);
    if (index != -1)
        return ParticlePackages[index].Package;

    return 0;
}

ITexture* CVideoNull::getTexture(const char* filename)
{
    ITexture* texture = findTexture(filename);
    if (texture)
        return texture;

    ox::io::IReadFile* file = FileSystem->createAndOpenFile(filename);
    if (file)
    {
        texture = loadTextureFromFile(file);
        file->drop();

        if (texture)
        {
            addTexture(texture, filename);
            texture->drop(); // the cache holds the only reference now
        }
    }
    else
        os::Printer::log("Could not open file of texture", filename, ELL_ERROR);

    if (!texture)
        os::Printer::log("Could not load texture", filename, ELL_ERROR);

    return texture;
}

ITexture* CVideoNull::findTexture(const char* filename)
{
    SSurface s;
    if (!filename)
        filename = "";
    s.Filename = filename;
    core::CStringFunctions::ansiMakeLower(s.Filename);

    int index = algo::binarySearchPos(Textures.begin(), Textures.end(), s);
    if (index != -1)
        return Textures[index].Surface;

    return 0;
}

ITexture* CVideoNull::loadTextureFromFile(ox::io::IReadFile* file)
{
    ITexture* texture = 0;
    IImage* image = createImageFromFile(file);

    if (image)
    {
        texture = createDeviceDependentTexture(image);
        os::Printer::log("Loaded texture", file->getFileName(), ELL_INFORMATION);
        image->drop();
    }

    return texture;
}

void CVideoNull::addTexture(ITexture* texture, const char* filename)
{
    if (texture)
    {
        if (!filename)
            filename = "";

        SSurface s;
        s.Filename = filename;
        core::CStringFunctions::ansiMakeLower(s.Filename);
        s.Surface = texture;
        texture->grab();

        Textures.push_back(s);
        algo::sort(Textures.begin(), Textures.end());
    }
}

ITexture* CVideoNull::getTexture(ox::io::IReadFile* file)
{
    ITexture* texture = 0;

    if (file)
    {
        texture = findTexture(file->getFileName());
        if (texture)
            return texture;

        texture = loadTextureFromFile(file);
        if (texture)
        {
            addTexture(texture, file->getFileName());
            texture->drop(); // the cache holds the only reference now
        }
    }

    if (!texture)
        os::Printer::log("Could not load texture", file->getFileName(), ELL_ERROR);

    return texture;
}

int CVideoNull::getNumTextures()
{
    return Textures.size();
}

ITexture* CVideoNull::addTexture(const char* name, IImage* image)
{
    if (!name || !image)
        return 0;

    ITexture* t = createDeviceDependentTexture(image);
    addTexture(t, name);
    t->drop();
    return t;
}

ITexture* CVideoNull::addTexture(const core::CDimension2d<int>& size, const char* name, ECOLOR_FORMAT format)
{
    if (!name)
        return 0;

    IImage* image = new CImage(format, size);
    ITexture* t = createDeviceDependentTexture(image);
    image->drop();
    addTexture(t, name);

    if (t)
        t->drop();

    return t;
}

ITexture* CVideoNull::createDeviceDependentTexture(IImage* surface)
{
    return new CSoftwareTexture(surface);
}

bool CVideoNull::setRenderTarget(ITexture* texture, bool clearBackBuffer, bool clearZBuffer, SColor color)
{
    return false;
}

ITexture* CVideoNull::createRenderTargetTexture(const core::CDimension2d<int>& size)
{
    return 0;
}

ITexture* CVideoNull::createScreenTexture(const core::CDimension2d<int>& size)
{
    os::Printer::log(L"CVideoNull::createScreenTexture() should not be called", ELL_WARNING);
    return 0;
}

void CVideoNull::setViewPort(const core::CRect<int>& area)
{
}

const core::CRect<int>& CVideoNull::getViewPort() const
{
    return ViewPort;
}

void CVideoNull::drawIndexedTriangleList(const S3DVertex* vertices, int vertexCount,
    const unsigned short* indexList, int triangleCount)
{
    PrimitivesDrawn += triangleCount;
}

void CVideoNull::drawIndexedTriangleList(const S3DVertex2TCoords* vertices, int vertexCount,
    const unsigned short* indexList, int triangleCount)
{
    PrimitivesDrawn += triangleCount;
}

void CVideoNull::drawIndexedTriangleFan(const S3DVertex2TCoords* vertices, int vertexCount,
    const unsigned short* indexList, int triangleCount)
{
    PrimitivesDrawn += triangleCount;
}

void CVideoNull::drawIndexedTriangleFan(const S3DVertex* vertices, int vertexCount,
    const unsigned short* indexList, int triangleCount)
{
    PrimitivesDrawn += triangleCount;
}

void CVideoNull::draw3DLine(const core::CVector3d<float>& start, const core::CVector3d<float>& end, SColor color)
{
    core::CVector3d<float> vect = start.crossProduct(end);
    vect.normalize();
    vect *= 1.0f;

    S3DVertex vtx[4];

    vtx[0].Color = color;
    vtx[1].Color = color;
    vtx[2].Color = color;
    vtx[3].Color = color;

    vtx[0].Pos = start;
    vtx[1].Pos = end;

    vtx[2].Pos = start + vect;
    vtx[3].Pos = end + vect;

    unsigned short idx[12] = {0, 1, 2, 0, 2, 1, 0, 1, 3, 0, 3, 1};

    drawIndexedTriangleList(vtx, 4, idx, 4);
}

void CVideoNull::draw3DTriangle(const core::CTriangle3d<float>& triangle, SColor color)
{
    draw3DLine(triangle.pointA, triangle.pointB, color);
    draw3DLine(triangle.pointB, triangle.pointC, color);
    draw3DLine(triangle.pointC, triangle.pointA, color);
}

void CVideoNull::draw3DBox(core::CAabbox3d<float> box, SColor color)
{
    core::CVector3d<float> edges[8];
    box.getEdges(edges);

    draw3DLine(edges[5], edges[1], color);
    draw3DLine(edges[1], edges[3], color);
    draw3DLine(edges[3], edges[7], color);
    draw3DLine(edges[7], edges[5], color);
    draw3DLine(edges[0], edges[2], color);
    draw3DLine(edges[2], edges[6], color);
    draw3DLine(edges[6], edges[4], color);
    draw3DLine(edges[4], edges[0], color);
    draw3DLine(edges[1], edges[0], color);
    draw3DLine(edges[3], edges[2], color);
    draw3DLine(edges[7], edges[6], color);
    draw3DLine(edges[5], edges[4], color);
}

void CVideoNull::draw2DImage(ITexture* texture, const core::CPosition2d<int>& destPos)
{
}

void CVideoNull::draw2DImage(ITexture* texture, const core::CPosition2d<int>& destPos,
    const core::CRect<int>& sourceRect, const core::CRect<int>* clipRect, SColor* colors,
    bool useAlphaChannelOfTexture)
{
}

void CVideoNull::draw2DImage(ITexture* texture, const core::CPosition2d<int>& destPos,
    const core::CRect<int>& sourceRect, const core::CRect<int>* clipRect, SColor color,
    bool useAlphaChannelOfTexture)
{
}

void CVideoNull::draw2DRectangle(SColor color, const core::CRect<int>& pos, const core::CRect<int>* clip)
{
}

void CVideoNull::draw2DLine(const core::CPosition2d<int>& start, const core::CPosition2d<int>& end, SColor color)
{
}

void CVideoNull::draw2DLineFloat(const core::CPosition2d<float>& start, const core::CPosition2d<float>& end,
    SColor color)
{
}

//! A cubic Bezier curve from start to end whose control points are start + startControl and
//! end + endControl, drawn as lines with step segments per unit of the curve parameter.
void CVideoNull::draw2DBezier(const core::CVector2d<float>& start, const core::CVector2d<float>& end,
    const core::CVector2d<float>& startControl, const core::CVector2d<float>& endControl, float step,
    SColor color)
{
    core::CVector2d<float> last = start;
    core::CVector2d<float> point;
    float t = 0.0f;
    do
    {
        float u = 1.0f - t;
        float b0 = u * u * u;
        float b1 = 3.0f * u * u * t;
        float b2 = 3.0f * u * t * t;
        float b3 = t * t * t;
        point.X = end.X * b3 + (end.X + endControl.X) * b2 + (start.X + startControl.X) * b1 + start.X * b0;
        point.Y = end.Y * b3 + (end.Y + endControl.Y) * b2 + (start.Y + startControl.Y) * b1 + start.Y * b0;
        draw2DLine(core::CPosition2d<int>((int)last.X, (int)last.Y),
            core::CPosition2d<int>((int)point.X, (int)point.Y), color);
        last = point;
        t += 1.0f / step;
    } while (t < 1.0f);

    draw2DLine(core::CPosition2d<int>((int)point.X, (int)point.Y),
        core::CPosition2d<int>((int)end.X, (int)end.Y), color);
}

//! A cubic Hermite curve from start to end with the given end tangents, drawn as lines with step
//! segments per unit of the curve parameter.
void CVideoNull::draw2DHermite(const core::CVector2d<float>& start, const core::CVector2d<float>& end,
    const core::CVector2d<float>& startTangent, const core::CVector2d<float>& endTangent, float step,
    SColor color)
{
    core::CVector2d<float> last = start;
    core::CVector2d<float> point;
    float t = 0.0f;
    do
    {
        float t2 = t * t;
        float t3 = t2 * t;
        float h1 = 2.0f * t3 - 3.0f * t2 + 1.0f;
        float h2 = -2.0f * t3 + 3.0f * t2;
        float h3 = t3 - 2.0f * t2 + t;
        float h4 = t3 - t2;
        point.X = end.X * h2 + start.X * h1 + startTangent.X * h3 + endTangent.X * h4;
        point.Y = end.Y * h2 + start.Y * h1 + startTangent.Y * h3 + endTangent.Y * h4;
        draw2DLine(core::CPosition2d<int>((int)last.X, (int)last.Y),
            core::CPosition2d<int>((int)point.X, (int)point.Y), color);
        last = point;
        t += 1.0f / step;
    } while (t < 1.0f);

    draw2DLine(core::CPosition2d<int>((int)point.X, (int)point.Y),
        core::CPosition2d<int>((int)end.X, (int)end.Y), color);
}

void CVideoNull::update2dViewValues(int offsetX, int offsetY)
{
    // The render size is rounded up to even so the 2d origin falls on a whole pixel.
    int width = ScreenSize.Width + (ScreenSize.Width & 1);
    int height = ScreenSize.Height + (ScreenSize.Height & 1);
    int halfWidth = width >> 1;
    int halfHeight = height >> 1;

    InvHalfWidth = 1.0f / halfWidth;
    InvHalfHeight = 1.0f / halfHeight;
    ViewOffsetX = offsetX - halfWidth;
    ViewOffsetY = height - halfHeight - offsetY;
}

core::CDimension2d<int> CVideoNull::getScreenSize()
{
    return ScreenSize;
}

core::CDimension2d<int> CVideoNull::getPhysicalScreenSize()
{
    return PhysicalScreenSize;
}

void CVideoNull::setRenderScreenSize(int width, int height)
{
    if (width > 0)
    {
        ScreenSize.Width = width;
        ScreenSize.Height = height;
    }
    else
        ScreenSize = PhysicalScreenSize;

    update2dViewValues(0, 0);
}

void CVideoNull::setForcePointSampling(bool force)
{
    if (force != ForcePointSampling)
    {
        flush2dRendering();
        ForcePointSampling = force;
    }
}

int CVideoNull::getFPS()
{
    return FPSCounter.getFPS();
}

int CVideoNull::getNumStatusSwitches()
{
    return NumStatusSwitches;
}

int CVideoNull::getPrimitiveCountDrawn()
{
    return PrimitivesDrawn;
}

void CVideoNull::setAmbientLight(const SColorf& color)
{
}

const wchar_t* CVideoNull::getName()
{
    return L"NullDevice";
}

void CVideoNull::drawStencilShadowVolume(const core::CVector3d<float>* triangles, int count, bool zfail)
{
}

void CVideoNull::drawStencilShadow(bool clearStencilBuffer, SColor leftUpEdge, SColor rightUpEdge,
    SColor leftDownEdge, SColor rightDownEdge)
{
}

void CVideoNull::deleteAllDynamicLights()
{
    Lights.clear();
}

void CVideoNull::addDynamicLight(const SLight& light)
{
    Lights.push_back(light);
}

int CVideoNull::getMaximalDynamicLightAmount()
{
    return 0;
}

int CVideoNull::getDynamicLightCount()
{
    return Lights.size();
}

const SLight& CVideoNull::getDynamicLight(int idx)
{
    if (idx < 0 || idx >= (int)Lights.size())
        return *((SLight*)0);

    return Lights[idx];
}

void CVideoNull::makeColorKeyTexture(ITexture* texture, SColor color)
{
    if (!texture)
        return;

    if (texture->getColorFormat() != ECF_A1R5G5B5 && texture->getColorFormat() != ECF_A8R8G8B8)
    {
        os::Printer::log("Error: Unsupported texture color format for making color key channel.", ELL_ERROR);
        return;
    }

    if (texture->getColorFormat() == ECF_A1R5G5B5)
    {
        short* p = (short*)texture->lock();
        if (!p)
        {
            os::Printer::log("Could not lock texture for making color key channel.", ELL_ERROR);
            return;
        }

        core::CDimension2d<int> dim = texture->getSize();
        int pitch = texture->getPitch() / 2;

        short ref = (0x0 << 15) | (~(0x1 << 15) & toA1R5G5B5(color));
        short blackalpha = (0x0 << 15) | (~(0x1 << 15) & 0);

        for (int x = 0; x < pitch; ++x)
            for (int y = 0; y < dim.Height; ++y)
            {
                short c = (0x0 << 15) | (~(0x1 << 15) & p[y * pitch + x]);
                p[y * pitch + x] = (c == ref) ? blackalpha : ((0x1 << 15) | (~(0x1 << 15) & c));
            }

        texture->unlock();
    }
    else
    {
        int* p = (int*)texture->lock();
        if (!p)
        {
            os::Printer::log("Could not lock texture for making color key channel.", ELL_ERROR);
            return;
        }

        core::CDimension2d<int> dim = texture->getSize();
        int pitch = texture->getPitch() / 4;

        int ref = (0x0 << 24) | (~(0xFF << 24) & color.color);
        int blackalpha = (0x0 << 24) | (~(0xFF << 24) & 0);

        for (int x = 0; x < pitch; ++x)
            for (int y = 0; y < dim.Height; ++y)
            {
                int c = (0x0 << 24) | (~(0xFF << 24) & p[y * pitch + x]);
                p[y * pitch + x] = (c == ref) ? blackalpha : ((0xFF << 24) | (~(0xFF << 24) & c));
            }

        texture->unlock();
    }
}

void CVideoNull::makeColorKeyTexture(ITexture* texture, core::CPosition2d<int> colorKeyPixelPos)
{
    if (!texture)
        return;

    if (texture->getColorFormat() != ECF_A1R5G5B5 && texture->getColorFormat() != ECF_A8R8G8B8)
    {
        os::Printer::log("Error: Unsupported texture color format for making color key channel.", ELL_ERROR);
        return;
    }

    if (texture->getColorFormat() == ECF_A1R5G5B5)
    {
        short* p = (short*)texture->lock();
        if (!p)
        {
            os::Printer::log("Could not lock texture for making color key channel.", ELL_ERROR);
            return;
        }

        core::CDimension2d<int> dim = texture->getSize();
        int pitch = texture->getPitch() / 2;

        short ref = (0x0 << 15) | (~(0x1 << 15) & p[colorKeyPixelPos.Y * dim.Width + colorKeyPixelPos.X]);
        short blackalpha = (0x0 << 15) | (~(0x1 << 15) & 0);

        for (int x = 0; x < pitch; ++x)
            for (int y = 0; y < dim.Height; ++y)
            {
                short c = (0x0 << 15) | (~(0x1 << 15) & p[y * pitch + x]);
                p[y * pitch + x] = (c == ref) ? blackalpha : ((0x1 << 15) | (~(0x1 << 15) & c));
            }

        texture->unlock();
    }
    else
    {
        int* p = (int*)texture->lock();
        if (!p)
        {
            os::Printer::log("Could not lock texture for making color key channel.", ELL_ERROR);
            return;
        }

        core::CDimension2d<int> dim = texture->getSize();
        int pitch = texture->getPitch() / 4;

        int ref = (0x0 << 24) | (~(0xFF << 24) & p[colorKeyPixelPos.Y * dim.Width + colorKeyPixelPos.X]);
        int blackalpha = (0x0 << 24) | (~(0xFF << 24) & 0);

        for (int x = 0; x < pitch; ++x)
            for (int y = 0; y < dim.Height; ++y)
            {
                int c = (0x0 << 24) | (~(0xFF << 24) & p[y * pitch + x]);
                p[y * pitch + x] = (c == ref) ? blackalpha : ((0xFF << 24) | (~(0xFF << 24) & c));
            }

        texture->unlock();
    }
}

int CVideoNull::getMaximalPrimitiveCount()
{
    return 65535;
}

bool CVideoNull::checkPrimitiveCount(int vertexCount)
{
    int m = getMaximalPrimitiveCount();

    if ((vertexCount - 1) > m)
    {
        char tmp[1024];
        sprintf(tmp, "Could not draw triangles, too many vertices(%d), maxium is %d.", vertexCount, m);
        os::Printer::log(tmp, ELL_ERROR);
        return false;
    }

    return true;
}

void CVideoNull::setTextureCreationFlag(E_TEXTURE_CREATION_FLAG flag, bool enabled)
{
    if (enabled && ((flag == ETCF_ALWAYS_16_BIT) || (flag == ETCF_ALWAYS_32_BIT) ||
        (flag == ETCF_OPTIMIZED_FOR_QUALITY) || (flag == ETCF_OPTIMIZED_FOR_SPEED)))
    {
        // the four format flags exclude each other
        setTextureCreationFlag(ETCF_ALWAYS_16_BIT, false);
        setTextureCreationFlag(ETCF_ALWAYS_32_BIT, false);
        setTextureCreationFlag(ETCF_OPTIMIZED_FOR_QUALITY, false);
        setTextureCreationFlag(ETCF_OPTIMIZED_FOR_SPEED, false);
    }

    TextureCreationFlags = (TextureCreationFlags & (~flag)) | ((((unsigned int)!enabled) - 1) & flag);
}

bool CVideoNull::getTextureCreationFlag(E_TEXTURE_CREATION_FLAG flag)
{
    return (TextureCreationFlags & flag) != 0;
}

IImage* CVideoNull::createImageFromFile(const char* filename)
{
    IImage* image = 0;
    ox::io::IReadFile* file = FileSystem->createAndOpenFile(filename);

    if (file)
    {
        image = createImageFromFile(file);
        file->drop();
    }
    else
        os::Printer::log("Could not open file of image", filename, ELL_ERROR);

    return image;
}

IImage* CVideoNull::createImageFromFile(ox::io::IReadFile* file)
{
    IImage* image = 0;

    // try to load file based on file extension
    unsigned int i;
    for (i = 0; i < SurfaceLoader.size(); ++i)
        if (SurfaceLoader[i]->isALoadableFileExtension(file->getFileName()))
        {
            image = SurfaceLoader[i]->loadImage(file);
            if (image)
                break;
        }

    // try to load file based on what is in it
    if (!image)
        for (i = 0; i < SurfaceLoader.size(); ++i)
        {
            if (i != 0)
                file->seek(0);

            if (SurfaceLoader[i]->isALoadableFileFormat(file))
            {
                file->seek(0);
                image = SurfaceLoader[i]->loadImage(file);
                if (image)
                    break;
            }
        }

    return image;
}

IImage* CVideoNull::createImageFromData(ECOLOR_FORMAT format, const core::CDimension2d<int>& size, void* data)
{
    return new CImage(format, size, data);
}

void CVideoNull::setFog(SColor color, bool linearFog, float start, float end, float density, bool pixelFog,
    bool rangeFog)
{
    FogColor = color;
    LinearFog = linearFog;
    FogStart = start;
    FogEnd = end;
    FogDensity = density;
    PixelFog = pixelFog;
    RangeFog = rangeFog;
}

void CVideoNull::drawMeshBuffer(scene::IMeshBuffer* mb)
{
    if (!mb)
        return;

    switch (mb->getVertexType())
    {
    case EVT_STANDARD:
        drawIndexedTriangleList((S3DVertex*)mb->getVertices(), mb->getVertexCount(), mb->getIndices(),
            mb->getIndexCount() / 3);
        break;
    case EVT_2TCOORDS:
        drawIndexedTriangleList((S3DVertex2TCoords*)mb->getVertices(), mb->getVertexCount(), mb->getIndices(),
            mb->getIndexCount() / 3);
        break;
    }
}

void CVideoNull::OnResize(const core::CDimension2d<int>& size)
{
    ScreenSize = size;
    PhysicalScreenSize = size;
    update2dViewValues(0, 0);

    // recreate the post-processing surfaces at the new size
    unsigned int count = NumPPSurfaces;
    freePPSurfaces();
    allocatePPSurfaces(count);
}

int CVideoNull::addAndDropMaterialRenderer(IMaterialRenderer* renderer)
{
    int i = addMaterialRenderer(renderer, 0);

    if (renderer)
        renderer->drop();

    return i;
}

int CVideoNull::addMaterialRenderer(IMaterialRenderer* renderer, const char* name)
{
    if (!renderer)
        return -1;

    SMaterialRenderer r;
    r.Renderer = renderer;
    r.Name = name;

    if (name == 0 && MaterialRenderers.size() < (sizeof(sBuiltInMaterialTypeNames) / sizeof(char*)) - 1)
    {
        // the built-in renderers are named by their material type
        r.Name = sBuiltInMaterialTypeNames[MaterialRenderers.size()];
    }

    MaterialRenderers.push_back(r);
    renderer->grab();

    return MaterialRenderers.size() - 1;
}

void CVideoNull::setMaterialRendererName(int idx, const char* name)
{
    if (idx < int(sizeof(sBuiltInMaterialTypeNames) / sizeof(char*)) - 1 || idx >= (int)MaterialRenderers.size())
        return;

    MaterialRenderers[idx].Name = name;
}

SExposedVideoData CVideoNull::getExposedVideoData()
{
    return ExposedData;
}

int CVideoNull::getDriverType()
{
    return 0;
}

IMaterialRenderer* CVideoNull::getMaterialRenderer(int idx)
{
    if (idx < 0 || idx >= (int)MaterialRenderers.size())
        return 0;

    return MaterialRenderers[idx].Renderer;
}

void* CVideoNull::getGPUProgrammingServices()
{
    return 0;
}

void* CVideoNull::getPostProcessingServices()
{
    return 0;
}

int CVideoNull::addHighLevelShaderMaterial(const char* vertexShaderProgram, const char* vertexShaderEntryPointName,
    E_VERTEX_SHADER_TYPE vsCompileTarget, const char* pixelShaderProgram, const char* pixelShaderEntryPointName,
    E_PIXEL_SHADER_TYPE psCompileTarget, IShaderConstantSetCallBack* callback, E_MATERIAL_TYPE baseMaterial,
    int userData)
{
    os::Printer::log("High level shader materials not available (yet) in this driver, sorry", ELL_INFORMATION);
    return -1;
}

int CVideoNull::addHighLevelShaderMaterialFromFiles(const char* vertexShaderProgramFile,
    const char* vertexShaderEntryPointName, E_VERTEX_SHADER_TYPE vsCompileTarget, const char* pixelShaderProgramFile,
    const char* pixelShaderEntryPointName, E_PIXEL_SHADER_TYPE psCompileTarget,
    IShaderConstantSetCallBack* callback, E_MATERIAL_TYPE baseMaterial, int userData)
{
    ox::io::IReadFile* vsfile = 0;
    ox::io::IReadFile* psfile = 0;

    if (vertexShaderProgramFile)
    {
        vsfile = FileSystem->createAndOpenFile(vertexShaderProgramFile);
        if (!vsfile)
        {
            os::Printer::log("Could not open vertex shader program file", vertexShaderProgramFile, ELL_WARNING);
            return -1;
        }
    }

    if (pixelShaderProgramFile)
    {
        psfile = FileSystem->createAndOpenFile(pixelShaderProgramFile);
        if (!psfile)
        {
            os::Printer::log("Could not open pixel shader program file", pixelShaderProgramFile, ELL_WARNING);
            if (vsfile)
                vsfile->drop();
            return -1;
        }
    }

    int result = addHighLevelShaderMaterialFromFiles(vsfile, vertexShaderEntryPointName, vsCompileTarget, psfile,
        pixelShaderEntryPointName, psCompileTarget, callback, baseMaterial, userData);

    if (psfile)
        psfile->drop();

    if (vsfile)
        vsfile->drop();

    return result;
}

int CVideoNull::addHighLevelShaderMaterialFromFiles(ox::io::IReadFile* vertexShaderProgram,
    const char* vertexShaderEntryPointName, E_VERTEX_SHADER_TYPE vsCompileTarget, ox::io::IReadFile* pixelShaderProgram,
    const char* pixelShaderEntryPointName, E_PIXEL_SHADER_TYPE psCompileTarget,
    IShaderConstantSetCallBack* callback, E_MATERIAL_TYPE baseMaterial, int userData)
{
    char* vs = 0;
    char* ps = 0;

    if (vertexShaderProgram)
    {
        int size = vertexShaderProgram->getSize();
        if (size)
        {
            vs = new char[size + 1];
            vertexShaderProgram->read(vs, size);
            vs[size] = 0;
        }
    }

    if (pixelShaderProgram)
    {
        int size = pixelShaderProgram->getSize();
        if (size)
        {
            ps = new char[size + 1];
            pixelShaderProgram->read(ps, size);
            ps[size] = 0;
        }
    }

    int result = addHighLevelShaderMaterial(vs, vertexShaderEntryPointName, vsCompileTarget, ps,
        pixelShaderEntryPointName, psCompileTarget, callback, baseMaterial, userData);

    delete [] vs;
    delete [] ps;

    return result;
}

int CVideoNull::addShaderMaterial(const char* vertexShaderProgram, const char* pixelShaderProgram,
    IShaderConstantSetCallBack* callback, E_MATERIAL_TYPE baseMaterial, int userData)
{
    os::Printer::log("Shader materials not implemented yet in this driver, sorry.", ELL_INFORMATION);
    return -1;
}

int CVideoNull::addShaderMaterialFromFiles(ox::io::IReadFile* vertexShaderProgram, ox::io::IReadFile* pixelShaderProgram,
    IShaderConstantSetCallBack* callback, E_MATERIAL_TYPE baseMaterial, int userData)
{
    char* vs = 0;
    char* ps = 0;

    if (vertexShaderProgram)
    {
        int size = vertexShaderProgram->getSize();
        if (size)
        {
            vs = new char[size + 1];
            vertexShaderProgram->read(vs, size);
            vs[size] = 0;
        }
    }

    if (pixelShaderProgram)
    {
        int size = pixelShaderProgram->getSize();
        if (size)
        {
            ps = new char[size + 1];
            pixelShaderProgram->read(ps, size);
            ps[size] = 0;
        }
    }

    int result = addShaderMaterial(vs, ps, callback, baseMaterial, userData);

    delete [] vs;
    delete [] ps;

    return result;
}

int CVideoNull::addShaderMaterialFromFiles(const char* vertexShaderProgramFileName,
    const char* pixelShaderProgramFileName, IShaderConstantSetCallBack* callback, E_MATERIAL_TYPE baseMaterial,
    int userData)
{
    ox::io::IReadFile* vsfile = 0;
    ox::io::IReadFile* psfile = 0;

    if (vertexShaderProgramFileName)
    {
        vsfile = FileSystem->createAndOpenFile(vertexShaderProgramFileName);
        if (!vsfile)
        {
            os::Printer::log("Could not open vertex shader program file", vertexShaderProgramFileName, ELL_WARNING);
            return -1;
        }
    }

    if (pixelShaderProgramFileName)
    {
        psfile = FileSystem->createAndOpenFile(pixelShaderProgramFileName);
        if (!psfile)
        {
            os::Printer::log("Could not open pixel shader program file", pixelShaderProgramFileName, ELL_WARNING);
            if (vsfile)
                vsfile->drop();
            return -1;
        }
    }

    int result = addShaderMaterialFromFiles(vsfile, psfile, callback, baseMaterial, userData);

    if (psfile)
        psfile->drop();

    if (vsfile)
        vsfile->drop();

    return result;
}

int CVideoNull::addCgShaderMaterial(const char* vertexShaderProgram, const char* vertexShaderEntryPointName,
    const char* pixelShaderProgram, const char* pixelShaderEntryPointName, IShaderConstantSetCallBack* callback,
    E_MATERIAL_TYPE baseMaterial, bool flag, int userData)
{
    os::Printer::log(L"CG Shaders not available yet", ELL_INFORMATION);
    return -1;
}

int CVideoNull::addCgShaderMaterialFromFiles(const char* vertexShaderProgramFileName,
    const char* vertexShaderEntryPointName, const char* pixelShaderProgramFileName,
    const char* pixelShaderEntryPointName, IShaderConstantSetCallBack* callback, E_MATERIAL_TYPE baseMaterial,
    bool flag, int userData)
{
    ox::io::IReadFile* vsfile = 0;
    ox::io::IReadFile* psfile = 0;

    if (vertexShaderProgramFileName)
    {
        vsfile = FileSystem->createAndOpenFile(vertexShaderProgramFileName);
        if (!vsfile)
        {
            os::Printer::log("Could not open vertex shader program file", vertexShaderProgramFileName, ELL_WARNING);
            return -1;
        }
    }

    if (pixelShaderProgramFileName)
    {
        psfile = FileSystem->createAndOpenFile(pixelShaderProgramFileName);
        if (!psfile)
        {
            os::Printer::log("Could not open pixel shader program file", pixelShaderProgramFileName, ELL_WARNING);
            if (vsfile)
                vsfile->drop();
            return -1;
        }
    }

    int result = addCgShaderMaterialFromFiles(vsfile, vertexShaderEntryPointName, psfile, pixelShaderEntryPointName,
        callback, baseMaterial, flag, userData);

    if (psfile)
        psfile->drop();

    if (vsfile)
        vsfile->drop();

    return result;
}

int CVideoNull::addCgShaderMaterialFromFiles(ox::io::IReadFile* vertexShaderProgram,
    const char* vertexShaderEntryPointName, ox::io::IReadFile* pixelShaderProgram, const char* pixelShaderEntryPointName,
    IShaderConstantSetCallBack* callback, E_MATERIAL_TYPE baseMaterial, bool flag, int userData)
{
    char* vs = 0;
    char* ps = 0;

    if (vertexShaderProgram)
    {
        int size = vertexShaderProgram->getSize();
        if (size)
        {
            vs = new char[size + 1];
            vertexShaderProgram->read(vs, size);
            vs[size] = 0;
        }
    }

    if (pixelShaderProgram)
    {
        int size = pixelShaderProgram->getSize();
        if (size)
        {
            ps = new char[size + 1];
            pixelShaderProgram->read(ps, size);
            ps[size] = 0;
        }
    }

    int result = addCgShaderMaterial(vs, vertexShaderEntryPointName, ps, pixelShaderEntryPointName, callback,
        baseMaterial, flag, userData);

    delete [] vs;
    delete [] ps;

    return result;
}

void CVideoNull::allocatePPSurfaces(unsigned int count)
{
    if (count > 8)
    {
        os::Printer::log(L"Trying to allocate more post processing surfaces than supported", ELL_WARNING);
        count = 8;
    }

    unsigned int i;
    for (i = NumPPSurfaces; i < count; ++i)
    {
        PPSurfaces[i] = createScreenTexture(PhysicalScreenSize);
        if (i < 2)
        {
            InputTextures[i] = PPSurfaces[i];
            InputTextures[i]->grab();
        }
    }

    for (i = count; i < NumPPSurfaces; ++i)
    {
        PPSurfaces[i]->drop();
        PPSurfaces[i] = 0;
        if (i < 2)
        {
            InputTextures[i]->drop();
            InputTextures[i] = 0;
        }
    }

    NumPPSurfaces = count;
}

void CVideoNull::freePPSurfaces()
{
    for (int i = 0; i < 8; ++i)
        if (PPSurfaces[i])
        {
            PPSurfaces[i]->drop();
            PPSurfaces[i] = 0;
        }

    for (int i = 0; i < 2; ++i)
        if (InputTextures[i])
        {
            InputTextures[i]->drop();
            InputTextures[i] = 0;
        }

    NumPPSurfaces = 0;
}

void CVideoNull::captureScreenBuffer(unsigned int index)
{
    captureScreenBuffer(index, core::CRect<int>(0, 0, ScreenSize.Width, ScreenSize.Height));
}

void CVideoNull::captureScreenBuffer(unsigned int index, const core::CRect<int>& area)
{
    os::Printer::log(L"A method that should be overridden wasn't", ELL_WARNING);
}

ITexture* CVideoNull::getCapturedBuffer(unsigned int index)
{
    if (index < NumPPSurfaces)
        return PPSurfaces[index];

    return 0;
}

void CVideoNull::runPPShader(int materialType)
{
    core::CRect<int> dest(0, 0, ScreenSize.Width, ScreenSize.Height);
    runPPShader(materialType, dest, 0, 0, 0);
}

void CVideoNull::drawPPImage(const core::CRect<int>& destRect, const core::CRect<int>& sourceRect,
    const core::CDimension2d<int>& surfaceSize, const core::CRect<int>* clipRect,
    const core::CDimension2d<int>* textureSize)
{
}

void CVideoNull::runPPShader(int materialType, core::CRect<int>& destRect, core::CRect<int>* sourceRect,
    core::CRect<int>* clipRect, unsigned int sizeTextureStage)
{
    SMaterial material;
    material.MaterialType = (E_MATERIAL_TYPE)materialType;
    material.ZBuffer = false;
    material.ZWriteEnable = false;
    material.TrilinearFilter = false;
    material.Texture1 = InputTextures[0];
    material.Texture2 = InputTextures[1];

    // the source rectangle is in render-size pixels; the surfaces have the physical size
    core::CRect<int> source = destRect;
    if (sourceRect)
        source = *sourceRect;

    if (ScreenSize != PhysicalScreenSize)
    {
        float scaleX = (float)PhysicalScreenSize.Width / (float)ScreenSize.Width;
        source.UpperLeftCorner.X = (int)(source.UpperLeftCorner.X * scaleX);
        source.LowerRightCorner.X = (int)(source.LowerRightCorner.X * scaleX);
        float scaleY = (float)PhysicalScreenSize.Height / (float)ScreenSize.Height;
        source.UpperLeftCorner.Y = (int)(source.UpperLeftCorner.Y * scaleY);
        source.LowerRightCorner.Y = (int)(source.LowerRightCorner.Y * scaleY);
    }

    core::CDimension2d<int> textureSize(0, 0);
    if (clipRect)
        textureSize = InputTextures[sizeTextureStage]->getSize();

    bool useMaterialShader = UseMaterialShaderFor2D;
    useMaterialShaderFor2D(true);
    setMaterial(material);
    drawPPImage(destRect, source, PPSurfaces[0]->getSize(), clipRect, &textureSize);
    useMaterialShaderFor2D(useMaterialShader);
}

void CVideoNull::setInputTexture(int stage, ITexture* texture)
{
    if (stage < 2)
    {
        if (InputTextures[stage])
            InputTextures[stage]->drop();

        if (!texture)
            texture = PPSurfaces[stage];

        InputTextures[stage] = texture;
        texture->grab();
        return;
    }

    os::Printer::log(L"Invalid texture stage to CVideoNull::setInputTexture", ELL_WARNING);
}

IVideoDriver* createNullDriver(ox::io::IFileSystem* io, const core::CDimension2d<int>& screenSize)
{
    return new CVideoNull(io, screenSize);
}

} // end namespace video
} // end namespace daisy
