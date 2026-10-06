// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// PROVISIONAL: written for CVideoOpenGL while CVideoNull is recovered in parallel. Only the layout
// (Linux amd64 offsets in the comments), the virtual order (Mac 1.18 vtable of
// daisy::video::CVideoNull) and the members CVideoOpenGL uses are recovered; the full header of the
// CVideoNull unit replaces this one.

#ifndef DAISY_VIDEO_NULL_CVIDEONULL_H
#define DAISY_VIDEO_NULL_CVIDEONULL_H

#include "ox/core/CString.h"
#include "ox/video/IGPUProgrammingServices.h"
#include "ox/video/IPostProcessingServices.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/S3DVertex.h"
#include "daisy/video/CgApi.h"
#include <vector>

namespace ox {
namespace io { class IFileSystem; }
namespace video { class IMaterialRenderer; }
} // end namespace ox

namespace daisy {
namespace video {

//! 2D quads collected by the draw2DImage calls and drawn with one glBegin(GL_QUADS) when the
//! texture or the blend mode changes, when the batch is full, or on flush2dRendering.
struct S2DQuadBatch
{
    enum { MAX_QUADS = 0x400 };

    ox::video::ITexture* Texture;
    int QuadCount;
    unsigned short Indices[MAX_QUADS * 6];
    //! Four vertices per quad, clockwise from the upper left corner.
    ox::video::S3DVertex Vertices[MAX_QUADS * 4];
    //! 4096 records of 28 bytes (three floats, a gap, two floats); not used by the OpenGL driver.
    char Unrecovered[0x1c000];
    //! Textured quads use alpha testing and SRC_ALPHA blending.
    bool UseAlphaChannel;
    //! Textured quads use linear filtering (unless point sampling is forced).
    bool LinearFilter;
};

//! A material renderer and its name.
struct SMaterialRenderer
{
    ox::core::CString<char> Name;
    ox::video::IMaterialRenderer* Renderer;
};

//! The device-independent part of the video driver: texture, sprite and particle package caches,
//! material renderers, the 2D quad batch and the 2D coordinate transform.
class CVideoNull : public ox::video::IVideoDriver, public ox::video::IGPUProgrammingServices,
                   public ox::video::IPostProcessingServices
{
public:
    CVideoNull(ox::io::IFileSystem* io, const ox::core::CDimension2d<int>& screenSize);
    virtual ~CVideoNull();

    virtual bool beginScene(bool backBuffer, bool zBuffer, ox::video::SColor color);
    virtual void clearScreen(bool zBuffer, ox::video::SColor color);
    virtual bool endScene();
    virtual bool queryFeature(ox::video::E_VIDEO_DRIVER_FEATURE feature);
    virtual void setTransform(ox::video::E_TRANSFORMATION_STATE state, const ox::core::CMatrix4& mat);
    virtual ox::core::CMatrix4 getTransform(ox::video::E_TRANSFORMATION_STATE state);
    virtual void setMaterial(const ox::video::SMaterial& material);
    virtual void useMaterialShaderFor2D(bool enabled);
    virtual void flushRender();
    virtual ox::video::ITexture* getTexture(const char* filename);
    virtual ox::video::ITexture* getTexture(ox::io::IReadFile* file);
    virtual ox::video::ITexture* addTexture(const ox::core::CDimension2d<int>& size, const char* name,
        ox::video::ECOLOR_FORMAT format);
    virtual ox::video::ITexture* addTexture(const char* name, ox::video::IImage* image);
    virtual void removeTexture(ox::video::ITexture* texture);
    virtual void removeTexture(const char* name);
    virtual void removeAllTextures();
    virtual int getNumTextures();
    virtual ox::video::ISpritePackage* getSpritePackage(const char* filename, bool load);
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
    virtual void draw2DTriangleList(ox::video::ITexture* texture, ox::core::CPosition2d<float>* positions,
        ox::core::CPosition2d<float>* textureCoords, ox::video::SColor* colors, int* indices, int vertexCount,
        int triangleCount);
    virtual void draw2DLine(const ox::core::CPosition2d<int>& start, const ox::core::CPosition2d<int>& end,
        ox::video::SColor color);
    virtual void draw2DLineFloat(const ox::core::CPosition2d<float>& start,
        const ox::core::CPosition2d<float>& end, ox::video::SColor color);
    virtual void draw2DBezier(const ox::core::CVector2d<float>& start,
        const ox::core::CVector2d<float>& control1, const ox::core::CVector2d<float>& control2,
        const ox::core::CVector2d<float>& end, float step, ox::video::SColor color);
    virtual void draw2DHermite(const ox::core::CVector2d<float>& start,
        const ox::core::CVector2d<float>& tangent1, const ox::core::CVector2d<float>& end,
        const ox::core::CVector2d<float>& tangent2, float step, ox::video::SColor color);
    virtual void drawStencilShadowVolume(const ox::core::CVector3d<float>* triangles, int count, bool zfail);
    virtual void drawStencilShadow(bool clearStencilBuffer, ox::video::SColor leftUpEdge,
        ox::video::SColor rightUpEdge, ox::video::SColor leftDownEdge, ox::video::SColor rightDownEdge);
    virtual void drawMeshBuffer(ox::scene::IMeshBuffer* meshBuffer);
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
    virtual const ox::video::SLight& getDynamicLight(int index);
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
    virtual ox::video::IMaterialRenderer* getMaterialRenderer(int index);
    virtual void* getExposedVideoData();
    virtual int getDriverType();
    virtual bool isFullscreen();
    virtual bool setFullscreen(bool fullscreen);
    virtual void setScissorRect(ox::core::CRect<int>* rect);
    virtual void setRenderScreenSize(int width, int height);
    virtual void setForcePointSampling(bool force);
    virtual void* getGPUProgrammingServices();
    virtual void* getPostProcessingServices();
    virtual ox::io::IFileSystem* getFileSystem();
    virtual bool saveJpegScreenshot(const char* directory, const char* name);

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
    virtual bool allocatePPSurfaces(unsigned int count);
    virtual void freePPSurfaces();
    virtual void captureScreenBuffer(unsigned int index);
    virtual void captureScreenBuffer(unsigned int index, const ox::core::CRect<int>& area);
    virtual ox::video::ITexture* getCapturedBuffer(unsigned int index);
    virtual void runPPShader(int material);
    virtual void runPPShader(int material, ox::core::CRect<int>& destRect, ox::core::CRect<int>* sourceRect,
        ox::core::CRect<int>* clipRect, unsigned int index);
    virtual void setInputTexture(int index, ox::video::ITexture* texture);

    virtual void drawPPImage(const ox::core::CRect<int>& destRect, const ox::core::CRect<int>& sourceRect,
        const ox::core::CDimension2d<int>& textureSize, const ox::core::CRect<int>* clipRect,
        ox::video::ITexture* texture);
    virtual void renderStatusChanged(ox::video::ITexture* texture, bool useAlphaChannel, bool linearFilter,
        int quads);
    //! Draws the collected 2D quads.
    virtual void flush2dRendering();
    //! Recomputes the pixel to clip space transform of the 2D functions for the screen size, with
    //! the origin moved by (x, y).
    virtual void update2dViewValues(int x, int y);
    virtual ox::video::ISpritePackage* findSpritePackage(const char* filename);
    virtual ox::video::IParticlePackage* findParticlePackage(const char* filename);
    //! Creates the driver's texture for an image; overridden by the drivers with their own textures.
    virtual ox::video::ITexture* createDeviceDependentTexture(ox::video::IImage* surface);

protected:
    void deleteAllTextures();
    void deleteMaterialRenders();
    //! False (and a log message) if a draw call has more primitives than the driver allows.
    bool checkPrimitiveCount(int vertexCount);
    void addAndDropMaterialRenderer(ox::video::IMaterialRenderer* renderer);

    // 0x28
    S2DQuadBatch Batch;
    // 0x43038: glBegin/glEnd pairs of the 2D batch
    int BatchFlushCount;
    // 0x4303c
    int StatusSwitches;
    // 0x43040: a pixel position p maps to clip space ((p.X + ViewXOffset) * ViewXFactor,
    // (ViewYOffset - p.Y) * ViewYFactor); see update2dViewValues.
    float ViewXFactor;
    float ViewYFactor;
    int ViewXOffset;
    int ViewYOffset;
    // 0x43050: texture, surface loader and other caches
    char Unrecovered43050[0x48];
    // 0x43098
    std::vector<SMaterialRenderer> MaterialRenderers;
    // 0x430b0
    char Unrecovered430b0[0x30];
    // 0x430e0
    ox::io::IFileSystem* FileSystem;
    // 0x430e8
    ox::core::CRect<int> ViewPort;
    // 0x430f8: the size the 2D functions draw for (setRenderScreenSize)
    ox::core::CDimension2d<int> ScreenSize;
    // 0x43100: the window size
    ox::core::CDimension2d<int> PhysicalScreenSize;
    // 0x43108
    char Unrecovered43108[0xc];
    // 0x43114
    int PrimitivesDrawn;
    // 0x43118: E_TEXTURE_CREATION_FLAG bits
    unsigned int TextureCreationFlags;
    // 0x4311c
    bool LinearFog;
    float FogStart;
    float FogEnd;
    float FogDensity;
    bool PixelFog;
    bool RangeFog;
    ox::video::SColor FogColor;
    // 0x43138
    char Unrecovered43138[0x18];
    // 0x43150: shared by all Cg material renderers
    CGcontext CgContext;
    // 0x43158
    char Unrecovered43158[0x69];
    // 0x431c1: nearest neighbour filtering for all 2D images
    bool ForcePointSampling;
};

} // end namespace video
} // end namespace daisy

#endif
