// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CVideoNull.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// The member layout follows the Linux 1.18 build (object size 0x431c8); the virtual order follows
// the Mac 1.18 vtable of daisy::video::CVideoNull.

#ifndef DAISY_VIDEO_NULL_CVIDEONULL_H
#define DAISY_VIDEO_NULL_CVIDEONULL_H

#include "CFPSCounter.h"
#include "ox/core/CString.h"
#include "ox/video/IGPUProgrammingServices.h"
#include "ox/video/IPostProcessingServices.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/S3DVertex.h"
#include "ox/video/SLight.h"
#include <vector>

typedef struct _CGcontext* CGcontext;

namespace daisy {
namespace video {

//! The most quads the 2d batch holds before the driver flushes it.
const int MAX_2D_QUADS = 1024;

//! A vertex of the second, unused 2d vertex array (see CVideoNull::Unused2DVertices).
struct S2DVertex
{
    ox::core::CVector3d<float> Pos;
    ox::video::SColor Color;
    ox::video::SColor Color2;
    ox::core::CVector2d<float> TCoords;
};

//! The driver base class: the texture, sprite package, particle package and material renderer
//! caches, image loading, the 2d view values and the frame statistics. It draws nothing itself;
//! CVideoOpenGL (and the unused software driver) derive from it.
//!
//! Caches are keyed by the lower-cased name passed in (no path normalization), kept sorted, and
//! hold one reference to each object. The getters return the cached object without granting a
//! reference; the remove functions drop the cache's reference.
class CVideoNull : public ox::video::IVideoDriver, public ox::video::IGPUProgrammingServices,
    public ox::video::IPostProcessingServices
{
public:
    CVideoNull(ox::io::IFileSystem* io, const ox::core::CDimension2d<int>& screenSize);
    virtual ~CVideoNull();

    virtual bool beginScene(bool backBuffer, bool zBuffer, ox::video::SColor color);
    virtual void clearScreen(bool zBuffer, ox::video::SColor color) {}
    virtual bool endScene();
    virtual bool queryFeature(ox::video::E_VIDEO_DRIVER_FEATURE feature);
    virtual void setTransform(ox::video::E_TRANSFORMATION_STATE state, const ox::core::CMatrix4& mat);
    virtual ox::core::CMatrix4 getTransform(ox::video::E_TRANSFORMATION_STATE state);
    virtual void setMaterial(const ox::video::SMaterial& material);
    virtual void useMaterialShaderFor2D(bool enabled) {}
    virtual void flushRender() {}

    virtual ox::video::ITexture* getTexture(const char* filename);
    virtual ox::video::ITexture* getTexture(ox::io::IReadFile* file);
    virtual ox::video::ITexture* addTexture(const ox::core::CDimension2d<int>& size, const char* name,
        ox::video::ECOLOR_FORMAT format);
    virtual ox::video::ITexture* addTexture(const char* name, ox::video::IImage* image);
    virtual void removeTexture(ox::video::ITexture* texture);
    virtual void removeTexture(const char* name);
    virtual void removeAllTextures();
    virtual int getNumTextures();

    virtual ox::video::ISpritePackage* getSpritePackage(const char* filename, bool keepStates);
    virtual void removeSpritePackage(const char* filename);
    virtual void removeAllSpritePackages();
    virtual ox::video::IParticlePackage* getParticlePackage(const char* filename);
    virtual void removeParticlePackage(const char* filename);
    virtual void removeAllParticlePackages();

    virtual void makeColorKeyTexture(ox::video::ITexture* texture, ox::video::SColor color);
    virtual void makeColorKeyTexture(ox::video::ITexture* texture, ox::core::CPosition2d<int> colorKeyPixelPos);
    virtual ox::video::ITexture* createRenderTargetTexture(const ox::core::CDimension2d<int>& size);
    virtual ox::video::ITexture* createScreenTexture(const ox::core::CDimension2d<int>& size);
    virtual bool setRenderTarget(ox::video::ITexture* texture, bool clearBackBuffer, bool clearZBuffer,
        ox::video::SColor color);
    virtual void setViewPort(const ox::core::CRect<int>& area);
    virtual const ox::core::CRect<int>& getViewPort() const;

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
    virtual void draw3DTriangle(const ox::core::CTriangle3d<float>& triangle, ox::video::SColor color);
    virtual void draw3DBox(ox::core::CAabbox3d<float> box, ox::video::SColor color);

    virtual void draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<int>& destPos);
    virtual void draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<int>& destPos,
        const ox::core::CRect<int>& sourceRect, const ox::core::CRect<int>* clipRect, ox::video::SColor color,
        bool useAlphaChannelOfTexture);
    virtual void drawScaled2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<float>& destPos,
        const ox::core::CRect<int>& sourceRect, float scale, ox::video::SColor color,
        bool useAlphaChannelOfTexture) {}
    virtual void draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<int>& destPos,
        const ox::core::CRect<int>& sourceRect, const ox::core::CRect<int>* clipRect, ox::video::SColor* colors,
        bool useAlphaChannelOfTexture);
    virtual void draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<int>& corner1,
        const ox::core::CPosition2d<int>& corner2, const ox::core::CPosition2d<int>& corner3,
        const ox::core::CPosition2d<int>& corner4, const ox::core::CRect<int>& sourceRect, ox::video::SColor* colors,
        bool useAlphaChannelOfTexture) {}
    virtual void draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<float>& corner1,
        const ox::core::CPosition2d<float>& corner2, const ox::core::CPosition2d<float>& corner3,
        const ox::core::CPosition2d<float>& corner4, const ox::core::CRect<int>& sourceRect,
        const ox::video::SColorArray* colors, bool useAlphaChannelOfTexture) {}
    virtual void draw2DRectangle(ox::video::SColor color, const ox::core::CRect<int>& pos,
        const ox::core::CRect<int>* clip);
    virtual void draw2DTriangleList(ox::video::ITexture* texture, ox::core::CPosition2d<float>* positions,
        ox::core::CPosition2d<float>* textureCoords, ox::video::SColor* colors, int* indices, int vertexCount,
        int triangleCount) {}
    virtual void draw2DLine(const ox::core::CPosition2d<int>& start, const ox::core::CPosition2d<int>& end,
        ox::video::SColor color);
    virtual void draw2DLineFloat(const ox::core::CPosition2d<float>& start,
        const ox::core::CPosition2d<float>& end, ox::video::SColor color);
    virtual void draw2DBezier(const ox::core::CVector2d<float>& start, const ox::core::CVector2d<float>& control1,
        const ox::core::CVector2d<float>& control2, const ox::core::CVector2d<float>& end, float step,
        ox::video::SColor color);
    virtual void draw2DHermite(const ox::core::CVector2d<float>& start, const ox::core::CVector2d<float>& tangent1,
        const ox::core::CVector2d<float>& end, const ox::core::CVector2d<float>& tangent2, float step,
        ox::video::SColor color);
    virtual void drawStencilShadowVolume(const ox::core::CVector3d<float>* triangles, int count, bool zfail);
    virtual void drawStencilShadow(bool clearStencilBuffer, ox::video::SColor leftUpEdge,
        ox::video::SColor rightUpEdge, ox::video::SColor leftDownEdge, ox::video::SColor rightDownEdge);
    virtual void drawMeshBuffer(ox::scene::IMeshBuffer* mb);
    virtual void setFog(ox::video::SColor color, bool linearFog, float start, float end, float density,
        bool pixelFog, bool rangeFog);
    virtual ox::core::CDimension2d<int> getScreenSize();
    virtual ox::core::CDimension2d<int> getPhysicalScreenSize();
    virtual int getFPS();
    virtual int getNumStatusSwitches();
    virtual int getPrimitiveCountDrawn();
    virtual void deleteAllDynamicLights();
    virtual void addDynamicLight(const ox::video::SLight& light);
    virtual void setAmbientLight(const ox::video::SColorf& color);
    virtual int getMaximalDynamicLightAmount();
    virtual int getDynamicLightCount();
    virtual const ox::video::SLight& getDynamicLight(int idx);
    virtual const wchar_t* getName();
    virtual void addExternalImageLoader(ox::video::IImageLoader* loader);
    virtual int getMaximalPrimitiveCount();
    virtual void setTextureCreationFlag(ox::video::E_TEXTURE_CREATION_FLAG flag, bool enabled);
    virtual bool getTextureCreationFlag(ox::video::E_TEXTURE_CREATION_FLAG flag);
    virtual ox::video::IImage* createImageFromFile(const char* filename);
    virtual ox::video::IImage* createImageFromFile(ox::io::IReadFile* file);
    virtual ox::video::IImage* createImageFromData(ox::video::ECOLOR_FORMAT format,
        const ox::core::CDimension2d<int>& size, void* data);
    virtual void OnResize(const ox::core::CDimension2d<int>& size);
    virtual int addMaterialRenderer(ox::video::IMaterialRenderer* renderer, const char* name);
    virtual void setMaterialRendererName(int index, const char* name);
    virtual ox::video::IMaterialRenderer* getMaterialRenderer(int idx);
    virtual ox::video::SExposedVideoData getExposedVideoData();
    virtual int getDriverType();
    virtual bool isFullscreen() { return false; }
    virtual bool setFullscreen(bool fullscreen) { return false; }
    virtual void setScissorRect(ox::core::CRect<int>* rect) {}
    virtual void setRenderScreenSize(int width, int height);
    virtual void setForcePointSampling(bool force);
    virtual void* getGPUProgrammingServices();
    virtual void* getPostProcessingServices();
    virtual ox::io::IFileSystem* getFileSystem() { return FileSystem; }
    virtual bool saveJpegScreenshot(const char* directory, const char* name) { return false; }

    // IGPUProgrammingServices
    virtual int addHighLevelShaderMaterial(const char* vertexShaderProgram, const char* vertexShaderEntryPointName,
        ox::video::E_VERTEX_SHADER_TYPE vsCompileTarget, const char* pixelShaderProgram,
        const char* pixelShaderEntryPointName, ox::video::E_PIXEL_SHADER_TYPE psCompileTarget,
        ox::video::IShaderConstantSetCallBack* callback, ox::video::E_MATERIAL_TYPE baseMaterial, int userData);
    virtual int addHighLevelShaderMaterialFromFiles(const char* vertexShaderProgramFile,
        const char* vertexShaderEntryPointName, ox::video::E_VERTEX_SHADER_TYPE vsCompileTarget,
        const char* pixelShaderProgramFile, const char* pixelShaderEntryPointName,
        ox::video::E_PIXEL_SHADER_TYPE psCompileTarget, ox::video::IShaderConstantSetCallBack* callback,
        ox::video::E_MATERIAL_TYPE baseMaterial, int userData);
    virtual int addHighLevelShaderMaterialFromFiles(ox::io::IReadFile* vertexShaderProgram,
        const char* vertexShaderEntryPointName, ox::video::E_VERTEX_SHADER_TYPE vsCompileTarget,
        ox::io::IReadFile* pixelShaderProgram, const char* pixelShaderEntryPointName,
        ox::video::E_PIXEL_SHADER_TYPE psCompileTarget, ox::video::IShaderConstantSetCallBack* callback,
        ox::video::E_MATERIAL_TYPE baseMaterial, int userData);
    virtual int addShaderMaterial(const char* vertexShaderProgram, const char* pixelShaderProgram,
        ox::video::IShaderConstantSetCallBack* callback, ox::video::E_MATERIAL_TYPE baseMaterial, int userData);
    virtual int addShaderMaterialFromFiles(ox::io::IReadFile* vertexShaderProgram,
        ox::io::IReadFile* pixelShaderProgram, ox::video::IShaderConstantSetCallBack* callback,
        ox::video::E_MATERIAL_TYPE baseMaterial, int userData);
    virtual int addShaderMaterialFromFiles(const char* vertexShaderProgramFileName,
        const char* pixelShaderProgramFileName, ox::video::IShaderConstantSetCallBack* callback,
        ox::video::E_MATERIAL_TYPE baseMaterial, int userData);
    virtual int addCgShaderMaterial(const char* vertexShaderProgram, const char* vertexShaderEntryPointName,
        const char* pixelShaderProgram, const char* pixelShaderEntryPointName,
        ox::video::IShaderConstantSetCallBack* callback, ox::video::E_MATERIAL_TYPE baseMaterial, bool flag,
        int userData);
    virtual int addCgShaderMaterialFromFiles(const char* vertexShaderProgramFileName,
        const char* vertexShaderEntryPointName, const char* pixelShaderProgramFileName,
        const char* pixelShaderEntryPointName, ox::video::IShaderConstantSetCallBack* callback,
        ox::video::E_MATERIAL_TYPE baseMaterial, bool flag, int userData);
    virtual int addCgShaderMaterialFromFiles(ox::io::IReadFile* vertexShaderProgram,
        const char* vertexShaderEntryPointName, ox::io::IReadFile* pixelShaderProgram,
        const char* pixelShaderEntryPointName, ox::video::IShaderConstantSetCallBack* callback,
        ox::video::E_MATERIAL_TYPE baseMaterial, bool flag, int userData);

    // IPostProcessingServices
    virtual void allocatePPSurfaces(unsigned int count);
    virtual void freePPSurfaces();
    virtual void captureScreenBuffer(unsigned int index);
    virtual void captureScreenBuffer(unsigned int index, const ox::core::CRect<int>& area);
    virtual ox::video::ITexture* getCapturedBuffer(unsigned int index);
    virtual void runPPShader(int materialType);
    virtual void runPPShader(int materialType, ox::core::CRect<int>& destRect, ox::core::CRect<int>* sourceRect,
        ox::core::CRect<int>* clipRect, unsigned int sizeTextureStage);
    virtual void setInputTexture(int stage, ox::video::ITexture* texture);

    //! Draws source part of a post-processing surface of surfaceSize into dest.
    virtual void drawPPImage(const ox::core::CRect<int>& destRect, const ox::core::CRect<int>& sourceRect,
        const ox::core::CDimension2d<int>& surfaceSize, const ox::core::CRect<int>* clipRect,
        const ox::core::CDimension2d<int>* textureSize);
    //! Starts a new 2d batch when texture or the blend flags differ from the current batch, or when
    //! quadCount more quads would not fit; returns whether it did.
    virtual bool renderStatusChanged(ox::video::ITexture* texture, bool alphaChannel, bool flag2, int quadCount)
    {
        return false;
    }
    //! Draws and empties the current 2d batch.
    virtual void flush2dRendering() {}
    //! Recomputes the 2d projection values from the render screen size and an offset.
    virtual void update2dViewValues(int offsetX, int offsetY);
    //! The cached sprite package for filename, or null; never loads.
    virtual ox::video::ISpritePackage* findSpritePackage(const char* filename);
    //! The cached particle package for filename, or null; never loads.
    virtual ox::video::IParticlePackage* findParticlePackage(const char* filename);

protected:
    //! Creates a texture of this driver from an image; the caller owns the returned reference.
    virtual ox::video::ITexture* createDeviceDependentTexture(ox::video::IImage* surface);

    void deleteAllTextures();
    ox::video::ITexture* findTexture(const char* filename);
    ox::video::ITexture* loadTextureFromFile(ox::io::IReadFile* file);
    void addTexture(ox::video::ITexture* texture, const char* filename);
    bool checkPrimitiveCount(int vertexCount);
    int addAndDropMaterialRenderer(ox::video::IMaterialRenderer* renderer);
    void deleteMaterialRenders();

    //! A cached texture under its lower-cased name.
    struct SSurface
    {
        SSurface() : Surface(0) {}

        bool operator<(const SSurface& other) const
        {
            return Filename < other.Filename;
        }

        ox::core::CString<char> Filename;
        ox::video::ITexture* Surface;
    };

    //! A material renderer and its name (the E_MATERIAL_TYPE name for the built-in types).
    struct SMaterialRenderer
    {
        ox::core::CString<char> Name;
        ox::video::IMaterialRenderer* Renderer;
    };

    //! A cached sprite package under its lower-cased file name.
    struct SSprites
    {
        SSprites() : Package(0) {}

        bool operator<(const SSprites& other) const
        {
            return Filename < other.Filename;
        }

        ox::core::CString<char> Filename;
        ox::video::ISpritePackage* Package;
    };

    //! A cached particle package under its lower-cased file name.
    struct SParticles
    {
        SParticles() : Package(0) {}

        bool operator<(const SParticles& other) const
        {
            return Filename < other.Filename;
        }

        ox::core::CString<char> Filename;
        ox::video::IParticlePackage* Package;
    };

    // The 2d batch, filled and drawn by the derived driver.
    ox::video::ITexture* Current2DTexture;
    int Current2DQuadCount;
    //! Room for MAX_2D_QUADS quads of two triangles; no recovered code reads it.
    unsigned short Indices2D[MAX_2D_QUADS * 6];
    //! Four vertices per quad, in screen pixels.
    ox::video::S3DVertex Vertices2D[MAX_2D_QUADS * 4];
    //! Constructed but never used by the recovered code.
    S2DVertex Unused2DVertices[MAX_2D_QUADS * 4];
    bool Current2DAlphaChannel;
    bool Current2DFlag2;
    //! Batches drawn in the current frame; NumStatusSwitches keeps the last frame's count.
    int StatusSwitches;
    int NumStatusSwitches;
    //! 2 / render width and 2 / render height, and the pixel offset of the 2d origin; see
    //! update2dViewValues.
    float InvHalfWidth;
    float InvHalfHeight;
    int ViewOffsetX;
    int ViewOffsetY;

    std::vector<SSurface> Textures;
    std::vector<ox::video::IImageLoader*> SurfaceLoader;
    std::vector<ox::video::SLight> Lights;
    std::vector<SMaterialRenderer> MaterialRenderers;
    std::vector<SSprites> SpritePackages;
    std::vector<SParticles> ParticlePackages;
    ox::io::IFileSystem* FileSystem;

    ox::core::CRect<int> ViewPort;
    //! The size the game renders at; 2d coordinates are in this space.
    ox::core::CDimension2d<int> ScreenSize;
    //! The size of the window or screen; equals ScreenSize unless setRenderScreenSize changed it.
    ox::core::CDimension2d<int> PhysicalScreenSize;
    CFPSCounter FPSCounter;

    unsigned int PrimitivesDrawn;
    unsigned int TextureCreationFlags;

    bool LinearFog;
    float FogStart;
    float FogEnd;
    float FogDensity;
    bool PixelFog;
    bool RangeFog;
    ox::video::SColor FogColor;

    ox::video::SExposedVideoData ExposedData;
    CGcontext CgContext;

    //! Post-processing surfaces; the first NumPPSurfaces are allocated.
    ox::video::ITexture* PPSurfaces[8];
    unsigned int NumPPSurfaces;
    //! The textures of stages 0 and 1 of the post-processing material.
    ox::video::ITexture* InputTextures[2];
    int Unknown431b0;
    //! Red, yellow and green; not read by the recovered code.
    ox::video::SColor DebugColors[3];
    //! Set by the derived driver's useMaterialShaderFor2D.
    bool UseMaterialShaderFor2D;
    bool ForcePointSampling;
};

//! Creates the null driver.
ox::video::IVideoDriver* createNullDriver(ox::io::IFileSystem* io, const ox::core::CDimension2d<int>& screenSize);

} // end namespace video
} // end namespace daisy

#endif
