// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: the virtual interface follows the Mac vtable (names from daisy::video::CVideoNull) up to
// draw2DLineFloat; the slots after it are not declared. Return types follow Irrlicht 0.7
// include/IVideoDriver.h where it has the function and are not verified otherwise.

#ifndef OX_VIDEO_IVIDEODRIVER_H
#define OX_VIDEO_IVIDEODRIVER_H

#include "../IUnknown.h"
#include "../core/CDimension2d.h"
#include "../core/CPosition2d.h"
#include "../core/CRect.h"
#include "../core/CVector2d.h"
#include "../core/CVector3d.h"
#include "SColor.h"

namespace ox {
namespace io {
class IFileSystem;
class IReadFile;
} // end namespace io

namespace core {
class CMatrix4;
template <class T> class CTriangle3d;
template <class T> class CAabbox3d;
} // end namespace core

namespace scene {
class IMeshBuffer;
} // end namespace scene

namespace video {

class IImage;
class IImageLoader;
class IMaterialRenderer;
struct SLight;
class SColorf;
class IParticlePackage;
class ISpritePackage;
class ITexture;
struct S3DVertex;
struct S3DVertex2TCoords;
struct SColorArray;
struct SMaterial;

//! Features a driver can be asked about. The values are the ones CVideoOpenGL::queryFeature
//! switches on; the list matches Irrlicht 1.3.
enum E_VIDEO_DRIVER_FEATURE
{
    EVDF_RENDER_TO_TARGET = 0,
    EVDF_HARDWARE_TL,
    EVDF_MULTITEXTURE,
    EVDF_BILINEAR_FILTER,
    EVDF_MIP_MAP,
    EVDF_MIP_MAP_AUTO_UPDATE,
    EVDF_STENCIL_BUFFER,
    EVDF_VERTEX_SHADER_1_1,
    EVDF_VERTEX_SHADER_2_0,
    EVDF_VERTEX_SHADER_3_0,
    EVDF_PIXEL_SHADER_1_1,
    EVDF_PIXEL_SHADER_1_2,
    EVDF_PIXEL_SHADER_1_3,
    EVDF_PIXEL_SHADER_1_4,
    EVDF_PIXEL_SHADER_2_0,
    EVDF_PIXEL_SHADER_3_0,
    EVDF_ARB_VERTEX_PROGRAM_1,
    EVDF_ARB_FRAGMENT_PROGRAM_1,
    EVDF_ARB_GLSL,
    EVDF_HLSL,
    EVDF_TEXTURE_NPOT,
    EVDF_FRAMEBUFFER_OBJECT
};

//! Geometry transformation states, as in Irrlicht 0.7.
enum E_TRANSFORMATION_STATE
{
    ETS_VIEW = 0,
    ETS_WORLD,
    ETS_PROJECTION,
    ETS_COUNT
};

//! Flags for texture creation, as in Irrlicht 0.7. The 16/32 bit and quality/speed pairs are
//! mutually exclusive: setting one clears the other three.
enum E_TEXTURE_CREATION_FLAG
{
    ETCF_ALWAYS_16_BIT = 0x00000001,
    ETCF_ALWAYS_32_BIT = 0x00000002,
    ETCF_OPTIMIZED_FOR_QUALITY = 0x00000004,
    ETCF_OPTIMIZED_FOR_SPEED = 0x00000008,
    ETCF_CREATE_MIP_MAPS = 0x00000010
};

//! Texture color formats. Partial: only the ones CGUIFont reads; the 32 bit value is the one the
//! binaries compare against.
enum ECOLOR_FORMAT
{
    ECF_A1R5G5B5 = 0,
    ECF_A8R8G8B8 = 0x8101800
};

//! Performs the 2d and 3d drawing and owns textures, sprite packages and particle packages.
class IVideoDriver : public IUnknown
{
public:
    virtual bool beginScene(bool backBuffer, bool zBuffer, SColor color) = 0;
    virtual void clearScreen(bool zBuffer, SColor color) = 0;
    virtual bool endScene() = 0;
    virtual bool queryFeature(E_VIDEO_DRIVER_FEATURE feature) = 0;
    virtual void setTransform(E_TRANSFORMATION_STATE state, const core::CMatrix4& mat) = 0;
    virtual core::CMatrix4 getTransform(E_TRANSFORMATION_STATE state) = 0;
    virtual void setMaterial(const SMaterial& material) = 0;
    virtual void useMaterialShaderFor2D(bool enabled) = 0;
    virtual void flushRender() = 0;

    virtual ITexture* getTexture(const char* filename) = 0;
    virtual ITexture* getTexture(io::IReadFile* file) = 0;
    virtual ITexture* addTexture(const core::CDimension2d<int>& size, const char* name, ECOLOR_FORMAT format) = 0;
    virtual ITexture* addTexture(const char* name, IImage* image) = 0;
    virtual void removeTexture(ITexture* texture) = 0;
    virtual void removeTexture(const char* name) = 0;
    virtual void removeAllTextures() = 0;
    virtual int getNumTextures() = 0;

    virtual ISpritePackage* getSpritePackage(const char* filename, bool load) = 0;
    virtual void removeSpritePackage(const char* filename) = 0;
    virtual void removeAllSpritePackages() = 0;
    virtual IParticlePackage* getParticlePackage(const char* filename) = 0;
    virtual void removeParticlePackage(const char* filename) = 0;
    virtual void removeAllParticlePackages() = 0;

    virtual void makeColorKeyTexture(ITexture* texture, SColor color) = 0;
    virtual void makeColorKeyTexture(ITexture* texture, core::CPosition2d<int> colorKeyPixelPos) = 0;
    virtual ITexture* createRenderTargetTexture(const core::CDimension2d<int>& size) = 0;
    virtual ITexture* createScreenTexture(const core::CDimension2d<int>& size) = 0;
    //! Returns true on success; the Linux OpenGL driver does not support render targets and
    //! returns true without doing anything.
    virtual bool setRenderTarget(ITexture* texture, bool clearBackBuffer, bool clearZBuffer, SColor color) = 0;
    virtual void setViewPort(const core::CRect<int>& area) = 0;
    virtual const core::CRect<int>& getViewPort() const = 0;

    virtual void drawIndexedTriangleList(const S3DVertex* vertices, int vertexCount,
        const unsigned short* indexList, int triangleCount) = 0;
    virtual void drawIndexedTriangleList(const S3DVertex2TCoords* vertices, int vertexCount,
        const unsigned short* indexList, int triangleCount) = 0;
    virtual void drawIndexedTriangleFan(const S3DVertex* vertices, int vertexCount,
        const unsigned short* indexList, int triangleCount) = 0;
    virtual void drawIndexedTriangleFan(const S3DVertex2TCoords* vertices, int vertexCount,
        const unsigned short* indexList, int triangleCount) = 0;
    virtual void draw3DLine(const core::CVector3d<float>& start, const core::CVector3d<float>& end,
        SColor color) = 0;
    virtual void draw3DTriangle(const core::CTriangle3d<float>& triangle, SColor color) = 0;
    virtual void draw3DBox(core::CAabbox3d<float> box, SColor color) = 0;

    virtual void draw2DImage(ITexture* texture, const core::CPosition2d<int>& destPos) = 0;
    virtual void draw2DImage(ITexture* texture, const core::CPosition2d<int>& destPos,
        const core::CRect<int>& sourceRect, const core::CRect<int>* clipRect, SColor color,
        bool useAlphaChannelOfTexture) = 0;
    virtual void drawScaled2DImage(ITexture* texture, const core::CPosition2d<float>& destPos,
        const core::CRect<int>& sourceRect, float scale, SColor color, bool useAlphaChannelOfTexture) = 0;
    virtual void draw2DImage(ITexture* texture, const core::CPosition2d<int>& destPos,
        const core::CRect<int>& sourceRect, const core::CRect<int>* clipRect, SColor* colors,
        bool useAlphaChannelOfTexture) = 0;
    virtual void draw2DImage(ITexture* texture, const core::CPosition2d<int>& corner1,
        const core::CPosition2d<int>& corner2, const core::CPosition2d<int>& corner3,
        const core::CPosition2d<int>& corner4, const core::CRect<int>& sourceRect, SColor* colors,
        bool useAlphaChannelOfTexture) = 0;
    virtual void draw2DImage(ITexture* texture, const core::CPosition2d<float>& corner1,
        const core::CPosition2d<float>& corner2, const core::CPosition2d<float>& corner3,
        const core::CPosition2d<float>& corner4, const core::CRect<int>& sourceRect,
        const SColorArray* colors, bool useAlphaChannelOfTexture) = 0;
    virtual void draw2DRectangle(SColor color, const core::CRect<int>& pos, const core::CRect<int>* clip) = 0;
    virtual void draw2DTriangleList(ITexture* texture, core::CPosition2d<float>* positions,
        core::CPosition2d<float>* textureCoords, SColor* colors, int* indices, int vertexCount,
        int triangleCount) = 0;
    virtual void draw2DLine(const core::CPosition2d<int>& start, const core::CPosition2d<int>& end,
        SColor color) = 0;
    virtual void draw2DLineFloat(const core::CPosition2d<float>& start, const core::CPosition2d<float>& end,
        SColor color) = 0;
    virtual void draw2DBezier(const core::CVector2d<float>& start, const core::CVector2d<float>& control1,
        const core::CVector2d<float>& control2, const core::CVector2d<float>& end, float step,
        SColor color) = 0;
    virtual void draw2DHermite(const core::CVector2d<float>& start, const core::CVector2d<float>& tangent1,
        const core::CVector2d<float>& end, const core::CVector2d<float>& tangent2, float step,
        SColor color) = 0;
    virtual void drawStencilShadowVolume(const core::CVector3d<float>* triangles, int count,
        bool zfail) = 0;
    virtual void drawStencilShadow(bool clearStencilBuffer, SColor leftUpEdge, SColor rightUpEdge,
        SColor leftDownEdge, SColor rightDownEdge) = 0;
    virtual void drawMeshBuffer(scene::IMeshBuffer* meshBuffer) = 0;
    virtual void setFog(SColor color, bool linearFog, float start, float end, float density,
        bool pixelFog, bool rangeFog) = 0;
    //! The size of the screen or render window.
    virtual core::CDimension2d<int> getScreenSize() = 0;
    virtual core::CDimension2d<int> getPhysicalScreenSize() = 0;
    virtual int getFPS() = 0;
    virtual int getNumStatusSwitches() = 0;
    virtual int getPrimitiveCountDrawn() = 0;
    virtual void deleteAllDynamicLights() = 0;
    virtual void addDynamicLight(const SLight& light) = 0;
    virtual void setAmbientLight(const SColorf& color) = 0;
    virtual int getMaximalDynamicLightAmount() = 0;
    virtual int getDynamicLightCount() = 0;
    virtual const SLight& getDynamicLight(int index) = 0;
    virtual const wchar_t* getName() = 0;
    virtual void addExternalImageLoader(IImageLoader* loader) = 0;
    virtual int getMaximalPrimitiveCount() = 0;
    virtual void setTextureCreationFlag(E_TEXTURE_CREATION_FLAG flag, bool enabled) = 0;
    virtual bool getTextureCreationFlag(E_TEXTURE_CREATION_FLAG flag) = 0;
    virtual IImage* createImageFromFile(const char* filename) = 0;
    virtual IImage* createImageFromFile(io::IReadFile* file) = 0;
    virtual IImage* createImageFromData(ECOLOR_FORMAT format, const core::CDimension2d<int>& size,
        void* data) = 0;
    virtual void OnResize(const core::CDimension2d<int>& size) = 0;
    virtual int addMaterialRenderer(IMaterialRenderer* renderer, const char* name) = 0;
    virtual void setMaterialRendererName(int index, const char* name) = 0;
    virtual IMaterialRenderer* getMaterialRenderer(int index) = 0;
    // Provisional: the exposed data's type is not recovered yet.
    virtual void* getExposedVideoData() = 0;
    virtual int getDriverType() = 0;
    virtual bool isFullscreen() = 0;
    virtual bool setFullscreen(bool fullscreen) = 0;
    virtual void setScissorRect(core::CRect<int>* rect) = 0;
    virtual void setRenderScreenSize(int width, int height) = 0;
    virtual void setForcePointSampling(bool force) = 0;
    // Provisional: the service types are not recovered yet.
    virtual void* getGPUProgrammingServices() = 0;
    virtual void* getPostProcessingServices() = 0;
    virtual io::IFileSystem* getFileSystem() = 0;
    //! Saves the screen as a JPEG into directory, under a generated name when name is null.
    virtual bool saveJpegScreenshot(const char* directory, const char* name) = 0;
};

} // end namespace video
} // end namespace ox

#endif
