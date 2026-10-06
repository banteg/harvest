// The built-in material renderers: the original's COpenGLMaterialRenderer.h
// (src/daisy/video/OpenGL/) call for call, with the fixed-function calls going to CFixedFunction
// and blending to OpenGL. CVideoGL registers them in E_MATERIAL_TYPE order, so CVideoNull names
// them "solid" ... "trans_reflection_2layer".
//
// Blend modes (the incoming fragment is the texture modulated by the vertex colour, except for
// SOLID, which uses GL_DECAL):
// - SOLID, LIGHTMAP*, SPHERE_MAP, REFLECTION_2_LAYER: no blending, no alpha test.
// - SOLID_2_LAYER: binds both textures and leaves blending and the texture environment as they are.
// - TRANSPARENT_ADD_COLOR, TRANSPARENT_VERTEX_ALPHA: (ONE, ONE_MINUS_SRC_COLOR), no alpha test,
//   depth writes off.
// - TRANSPARENT_ALPHA_CHANNEL: the same blend function, plus the alpha test alpha > 0.
// - TRANSPARENT_REFLECTION_2_LAYER: the same blend function with sphere mapping.

#ifndef PORT_VIDEO_MATERIALRENDERERS_H
#define PORT_VIDEO_MATERIALRENDERERS_H

#include "video/CVideoGL.h"
#include "ox/video/IMaterialRenderer.h"

namespace port {
namespace video {

class CMaterialRenderer : public ox::video::IMaterialRenderer
{
public:
    CMaterialRenderer(CVideoGL* driver)
        : Driver(driver), FF(driver->fixedFunction())
    {
    }

protected:
    CVideoGL* Driver;
    CFixedFunction& FF;
};

//! One texture in GL_DECAL mode on the active unit: where the texture is opaque its colour replaces
//! the lit vertex colour.
class CMaterialRenderer_SOLID : public CMaterialRenderer
{
public:
    CMaterialRenderer_SOLID(CVideoGL* d)
        : CMaterialRenderer(d) {}

    virtual void OnSetMaterial(ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
        bool resetAllRenderstates, ox::video::IMaterialRendererServices* services)
    {
        if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
        {
            Driver->disableTextures(1);
            Driver->setTexture(0, material.Texture1);
            FF.setTexEnvMode(ETEM_DECAL);
            gl::glDisable(gl::GL_BLEND);
            FF.setAlphaTest(false);
        }

        Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
    }
};

//! Two textures; the texture environments are left as they are (the scattering shaders use this as
//! their base material).
class CMaterialRenderer_SOLID_2_LAYER : public CMaterialRenderer
{
public:
    CMaterialRenderer_SOLID_2_LAYER(CVideoGL* d)
        : CMaterialRenderer(d) {}

    virtual void OnSetMaterial(ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
        bool resetAllRenderstates, ox::video::IMaterialRendererServices* services)
    {
        Driver->disableTextures(2);
        Driver->setTexture(1, material.Texture2);
        Driver->setTexture(0, material.Texture1);

        Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
    }
};

//! The blend of the three transparent renderers that share it: (ONE, ONE_MINUS_SRC_COLOR) with
//! unit 1 off and GL_MODULATE, depth writes off; alphaTest adds the alpha test alpha > 0.
class CMaterialRenderer_TRANSPARENT : public CMaterialRenderer
{
public:
    CMaterialRenderer_TRANSPARENT(CVideoGL* d, bool alphaTest)
        : CMaterialRenderer(d), AlphaTest(alphaTest) {}

    virtual void OnSetMaterial(ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
        bool resetAllRenderstates, ox::video::IMaterialRendererServices* services)
    {
        if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
        {
            FF.setAlphaTest(AlphaTest);
            if (AlphaTest)
                FF.setAlphaRef(0.0f);

            Driver->setTexture(1, 0);
            Driver->activeTexture(0);

            gl::glBlendFunc(gl::GL_ONE, gl::GL_ONE_MINUS_SRC_COLOR);
            FF.setTexEnvMode(ETEM_MODULATE);
            gl::glEnable(gl::GL_BLEND);
        }

        material.ZWriteEnable = false;
        services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
    }

    virtual bool isTransparent() { return true; }

private:
    bool AlphaTest;
};

//! Texture 1 times texture 2 (the light map), scaled by 1, 2 or 4 for the _M2 and _M4 types, or
//! added for LIGHTMAP_ADD.
class CMaterialRenderer_LIGHTMAP : public CMaterialRenderer
{
public:
    CMaterialRenderer_LIGHTMAP(CVideoGL* d)
        : CMaterialRenderer(d) {}

    virtual void OnSetMaterial(ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
        bool resetAllRenderstates, ox::video::IMaterialRendererServices* services)
    {
        if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
        {
            // diffuse map
            gl::glDisable(gl::GL_BLEND);
            FF.setAlphaTest(false);

            Driver->activeTexture(0);
            FF.setTexEnvMode(ETEM_COMBINE);
            FF.setCombineRgb(ECF_COMBINE_REPLACE);

            // light map
            Driver->activeTexture(1);
            if (material.MaterialType == ox::video::EMT_LIGHTMAP_ADD)
                FF.setTexEnvMode(ETEM_ADD);
            else
            {
                FF.setTexEnvMode(ETEM_COMBINE);
                FF.setCombineRgb(ECF_COMBINE_MODULATE);
            }

            FF.setCombineSource(0, ECS_PREVIOUS);
            FF.setCombineSource(1, ECS_TEXTURE);

            if (material.MaterialType == ox::video::EMT_LIGHTMAP_M4)
                FF.setRgbScale(4.0f);
            else if (material.MaterialType == ox::video::EMT_LIGHTMAP_M2)
                FF.setRgbScale(2.0f);
            else
                FF.setRgbScale(1.0f);
        }

        services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
    }
};

//! Sphere-mapped texture coordinates; reflection adds unit 1 off with GL_DECAL around it, and the
//! transparent version blends like TRANSPARENT_ADD_COLOR with depth writes off.
class CMaterialRenderer_SPHERE_MAP : public CMaterialRenderer
{
public:
    enum E_KIND
    {
        EK_SPHERE_MAP,
        EK_REFLECTION_2_LAYER,
        EK_TRANSPARENT_REFLECTION_2_LAYER
    };

    CMaterialRenderer_SPHERE_MAP(CVideoGL* d, E_KIND kind)
        : CMaterialRenderer(d), Kind(kind) {}

    virtual void OnSetMaterial(ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
        bool resetAllRenderstates, ox::video::IMaterialRendererServices* services)
    {
        if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
        {
            disableSecondUnit();

            if (Kind == EK_TRANSPARENT_REFLECTION_2_LAYER)
            {
                gl::glBlendFunc(gl::GL_ONE, gl::GL_ONE_MINUS_SRC_COLOR);
                FF.setTexEnvMode(ETEM_MODULATE);
                gl::glEnable(gl::GL_BLEND);
            }
            else
            {
                FF.setTexEnvMode(ETEM_MODULATE);
                gl::glDisable(gl::GL_BLEND);
            }
            FF.setAlphaTest(false);

            FF.setTexGenSphereMap(true);
        }

        if (Kind == EK_TRANSPARENT_REFLECTION_2_LAYER)
            material.ZWriteEnable = false;
        services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
    }

    virtual void OnUnsetMaterial()
    {
        if (Kind == EK_REFLECTION_2_LAYER)
            disableSecondUnit();

        FF.setTexGenSphereMap(false);
    }

    virtual bool isTransparent() { return Kind == EK_TRANSPARENT_REFLECTION_2_LAYER; }

private:
    //! Unit 1 off with GL_DECAL, leaving unit 0 active.
    void disableSecondUnit()
    {
        Driver->activeTexture(1);
        FF.setTexture2D(false);
        FF.setTexEnvMode(ETEM_DECAL);
        Driver->activeTexture(0);
    }

    E_KIND Kind;
};

} // end namespace video
} // end namespace port

#endif
