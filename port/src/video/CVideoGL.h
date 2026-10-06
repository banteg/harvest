// The port's renderer: the original Linux driver daisy::video::CVideoOpenGL
// (src/daisy/video/OpenGL/CVideoOpenGL.*) on OpenGL ES 3.0 / WebGL 2, or OpenGL 3.3 core on the
// desktop. It derives from the recovered CVideoNull like the original and issues the original's
// OpenGL calls in the original's order: what core OpenGL lacks (texture environments, the alpha
// test, lighting, fog, matrices, immediate mode) goes through CFixedFunction, which keeps that
// state as OpenGL 1.x would and applies it in a shader. The rules it keeps are in
// docs/port/renderer.md.

#ifndef PORT_VIDEO_CVIDEOGL_H
#define PORT_VIDEO_CVIDEOGL_H

#include "daisy/video/Null/CVideoNull.h"
#include "ox/core/CMatrix4.h"
#include "ox/video/IMaterialRendererServices.h"
#include "ox/video/SMaterial.h"
#include "video/CFixedFunction.h"
#include <vector>

namespace ox {
class IOxDevice;
} // end namespace ox

namespace port {
namespace video {

class CVideoGL : public daisy::video::CVideoNull, public ox::video::IMaterialRendererServices
{
public:
    //! The device's OpenGL context must be current; endScene presents through device->swapBuffers().
    CVideoGL(const ox::core::CDimension2d<int>& screenSize, ox::IOxDevice* device, ox::io::IFileSystem* io);
    virtual ~CVideoGL();

    //! False when the OpenGL entry points or the fixed-function program are missing.
    bool isValid() const { return Valid; }

    virtual bool beginScene(bool backBuffer, bool zBuffer, ox::video::SColor color);
    virtual void clearScreen(bool zBuffer, ox::video::SColor color);
    virtual bool endScene();
    virtual bool queryFeature(ox::video::E_VIDEO_DRIVER_FEATURE feature);
    virtual void setTransform(ox::video::E_TRANSFORMATION_STATE state, const ox::core::CMatrix4& mat);
    virtual ox::core::CMatrix4 getTransform(ox::video::E_TRANSFORMATION_STATE state);
    virtual void setMaterial(const ox::video::SMaterial& material);

    //! Records the flag; the OpenGL driver never reads it.
    virtual void useMaterialShaderFor2D(bool enabled) { UseMaterialShaderFor2D = enabled; }
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
    //! Records the file names, which select the GLSL translations, then opens and reads the files as
    //! the original does (a missing file still fails with -1).
    virtual int addCgShaderMaterialFromFiles(const char* vertexShaderProgramFileName,
        const char* vertexShaderEntryPointName, const char* pixelShaderProgramFileName,
        const char* pixelShaderEntryPointName, ox::video::IShaderConstantSetCallBack* callback,
        ox::video::E_MATERIAL_TYPE baseMaterial, bool precompiled, int userData);
    //! Compiles the GLSL translations of the Cg files whose names addCgShaderMaterialFromFiles
    //! recorded (the Cg text itself is not compiled); -1 when there is no translation.
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

    //! Binds texture to a texture unit (or disables the unit for 0); false if the texture belongs to
    //! another driver.
    bool setTexture(int stage, ox::video::ITexture* texture);
    //! Disables the units from fromStage on.
    bool disableTextures(int fromStage = 0);
    //! glActiveTexture (the original's extGlActiveTextureARB).
    void activeTexture(int unit);
    //! The emulated fixed-function state, for the material renderers.
    CFixedFunction& fixedFunction() { return FixedFunction; }

private:
    enum E_RENDER_MODE
    {
        ERM_NONE = 0,
        ERM_2D,
        ERM_3D
    };

    virtual ox::video::ITexture* createDeviceDependentTexture(ox::video::IImage* surface);

    void switch2dRendering(ox::video::ITexture* texture, bool useAlphaChannel, bool linearFilter, int quads);
    void setRenderStates3DMode();
    void setRenderStates2DMode(bool alpha, bool texture, bool alphaChannel, bool linearFilter);
    void createMaterialRenderers();

    //! Loads modelview = view * world, as glLoadMatrixf(GL_MODELVIEW) did.
    void loadModelView();
    //! Loads the projection with element 12 negated, as the original did.
    void loadProjection();
    //! Draws vertices with the current 3D state (the body of the drawIndexedTriangle* functions).
    void draw3D(gl::GLenum mode, const SVertexArrays& arrays, const unsigned short* indexList, int indexCount);
    //! Draws an untextured 2D primitive of up to four vertices in the 2D mapping, one colour.
    void drawPrimitive2D(gl::GLenum mode, const ox::core::CPosition2d<float>* positions, int count,
        ox::video::SColor color);
    //! Writes the frame to PendingScreenshot (saveJpegScreenshot defers to the end of the frame).
    void writeScreenshot();
    //! Reads DrawableSize from the window of the current OpenGL context.
    void updateDrawableSize();
    //! A rectangle in screen units (y down) as glViewport/glScissor's x, y, width and height in
    //! drawable pixels (y up).
    void toDrawable(const ox::core::CRect<int>& rect, gl::GLint out[4]) const;
    //! The 2D texture coordinate inset in texels: half a drawable pixel at 1:1, so 0.5 (the
    //! original's half texel) when the drawable has the screen size and 0.25 at twice the size.
    ox::core::CPosition2d<float> getTexelInset() const;

    ox::IOxDevice* Device;
    bool Valid;
    //! The size in pixels of what OpenGL draws into: the screen size times the display's scale.
    ox::core::CDimension2d<int> DrawableSize;
    CFixedFunction FixedFunction;

    ox::core::CMatrix4 Matrizes[ox::video::ETS_COUNT];
    E_RENDER_MODE CurrentRenderMode;
    ox::video::ITexture* CurrentTexture[CFixedFunction::MAX_UNITS];
    bool ResetRenderStates;
    bool Transformation3DChanged;
    bool ClampTexture;
    bool Fullscreen;

    ox::video::SMaterial Material;
    ox::video::SMaterial LastMaterial;
    int LastSetLight;
    int MaxTextureUnits;

    //! The Cg file names addCgShaderMaterialFromFiles is loading.
    const char* PendingVertexShaderFile;
    const char* PendingPixelShaderFile;

    //! The path saveJpegScreenshot chose; written at the next endScene, before presenting.
    ox::core::CString<char> PendingScreenshot;
};

} // end namespace video
} // end namespace port

#endif
