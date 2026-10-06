// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/COpenGLMaterialRenderer.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source. Oxeye's
// renderers bind their textures through CVideoOpenGL::setTexture and disableTextures, and the
// solid ones call the driver's setBasicRenderStates directly.
//
// Blend modes (the incoming fragment is the texture modulated by the vertex color, except for SOLID,
// which uses GL_DECAL):
// - SOLID, LIGHTMAP*, SPHERE_MAP, REFLECTION_2_LAYER: no blending, no alpha test.
// - SOLID_2_LAYER: binds both textures and leaves blending and the texture environment as they are.
// - TRANSPARENT_ADD_COLOR, TRANSPARENT_VERTEX_ALPHA: glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR),
//   no alpha test, depth writes off (the vertex alpha is not used, as in Irrlicht 0.7).
// - TRANSPARENT_ALPHA_CHANNEL: the same blend function, plus the alpha test alpha > 0.
// - TRANSPARENT_REFLECTION_2_LAYER: the same blend function with sphere mapping.

#ifndef DAISY_VIDEO_OPENGL_COPENGLMATERIALRENDERER_H
#define DAISY_VIDEO_OPENGL_COPENGLMATERIALRENDERER_H

#include "CVideoOpenGL.h"
#include "ox/video/IMaterialRenderer.h"

namespace daisy {
namespace video {

//! Base class of the fixed function material renderers.
class COpenGLMaterialRenderer : public ox::video::IMaterialRenderer
{
public:
    COpenGLMaterialRenderer(CVideoOpenGL* driver)
        : Driver(driver)
    {
    }

protected:
    CVideoOpenGL* Driver;
};

//! One texture in GL_DECAL mode on unit 0: where the texture is opaque its color replaces the lit
//! vertex color (Irrlicht 0.7 used GL_DECAL only to switch the second unit off).
class COpenGLMaterialRenderer_SOLID : public COpenGLMaterialRenderer
{
public:
    COpenGLMaterialRenderer_SOLID(CVideoOpenGL* d)
        : COpenGLMaterialRenderer(d) {}

    virtual void OnSetMaterial(ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
        bool resetAllRenderstates, ox::video::IMaterialRendererServices* services)
    {
        if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
        {
            Driver->disableTextures(1);
            Driver->setTexture(0, material.Texture1);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
            glDisable(GL_BLEND);
            glDisable(GL_ALPHA_TEST);
        }

        Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
    }
};

//! Two textures; the second unit's environment is left to the caller (the Cg ground shader of the
//! main menu uses this as its base material).
class COpenGLMaterialRenderer_SOLID_2_LAYER : public COpenGLMaterialRenderer
{
public:
    COpenGLMaterialRenderer_SOLID_2_LAYER(CVideoOpenGL* d)
        : COpenGLMaterialRenderer(d) {}

    virtual void OnSetMaterial(ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
        bool resetAllRenderstates, ox::video::IMaterialRendererServices* services)
    {
        Driver->disableTextures(2);
        Driver->setTexture(1, material.Texture2);
        Driver->setTexture(0, material.Texture1);

        Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
    }
};

//! Additive-style transparency: the destination is scaled by one minus the source color.
class COpenGLMaterialRenderer_TRANSPARENT_ADD_COLOR : public COpenGLMaterialRenderer
{
public:
    COpenGLMaterialRenderer_TRANSPARENT_ADD_COLOR(CVideoOpenGL* d)
        : COpenGLMaterialRenderer(d) {}

    virtual void OnSetMaterial(ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
        bool resetAllRenderstates, ox::video::IMaterialRendererServices* services)
    {
        if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
        {
            glDisable(GL_ALPHA_TEST);

            Driver->setTexture(1, 0);

            if (Driver->hasMultiTextureExtension())
                Driver->extGlActiveTextureARB(GL_TEXTURE0_ARB);

            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glEnable(GL_BLEND);
        }

        material.ZWriteEnable = false;
        services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
    }

    virtual bool isTransparent() { return true; }
};

//! Transparency from the texture's alpha channel; fragments with alpha 0 are discarded.
class COpenGLMaterialRenderer_TRANSPARENT_ALPHA_CHANNEL : public COpenGLMaterialRenderer
{
public:
    COpenGLMaterialRenderer_TRANSPARENT_ALPHA_CHANNEL(CVideoOpenGL* d)
        : COpenGLMaterialRenderer(d) {}

    virtual void OnSetMaterial(ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
        bool resetAllRenderstates, ox::video::IMaterialRendererServices* services)
    {
        if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
        {
            glEnable(GL_ALPHA_TEST);
            glAlphaFunc(GL_GREATER, 0);

            Driver->setTexture(1, 0);

            if (Driver->hasMultiTextureExtension())
                Driver->extGlActiveTextureARB(GL_TEXTURE0_ARB);

            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glEnable(GL_BLEND);
        }

        material.ZWriteEnable = false;
        services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
    }

    virtual bool isTransparent() { return true; }
};

//! Meant to blend with the vertex alpha, but sets the same blend function as TRANSPARENT_ADD_COLOR.
class COpenGLMaterialRenderer_TRANSPARENT_VERTEX_ALPHA : public COpenGLMaterialRenderer
{
public:
    COpenGLMaterialRenderer_TRANSPARENT_VERTEX_ALPHA(CVideoOpenGL* d)
        : COpenGLMaterialRenderer(d) {}

    virtual void OnSetMaterial(ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
        bool resetAllRenderstates, ox::video::IMaterialRendererServices* services)
    {
        if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
        {
            glDisable(GL_ALPHA_TEST);

            Driver->setTexture(1, 0);

            if (Driver->hasMultiTextureExtension())
                Driver->extGlActiveTextureARB(GL_TEXTURE0_ARB);

            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glEnable(GL_BLEND);
        }

        material.ZWriteEnable = false;
        services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
    }

    virtual bool isTransparent() { return true; }
};

//! Texture 1 times texture 2 (the lightmap), scaled by 1, 2 or 4 for the _M2 and _M4 types, or
//! added for LIGHTMAP_ADD.
class COpenGLMaterialRenderer_LIGHTMAP : public COpenGLMaterialRenderer
{
public:
    COpenGLMaterialRenderer_LIGHTMAP(CVideoOpenGL* d)
        : COpenGLMaterialRenderer(d) {}

    virtual void OnSetMaterial(ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
        bool resetAllRenderstates, ox::video::IMaterialRendererServices* services)
    {
        if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
        {
            // diffuse map

            if (Driver->hasMultiTextureExtension())
            {
                glDisable(GL_BLEND);
                glDisable(GL_ALPHA_TEST);

                Driver->extGlActiveTextureARB(GL_TEXTURE0_ARB);
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
                glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE);

                // lightmap

                Driver->extGlActiveTextureARB(GL_TEXTURE1_ARB);
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);

                if (material.MaterialType == ox::video::EMT_LIGHTMAP_ADD)
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
                else
                    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);

                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_PREVIOUS_EXT);
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_TEXTURE);
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_EXT, GL_TEXTURE);
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_EXT, GL_SRC_COLOR);

                if (material.MaterialType == ox::video::EMT_LIGHTMAP_M4)
                    glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, 4);
                else if (material.MaterialType == ox::video::EMT_LIGHTMAP_M2)
                    glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, 2);
                else
                    glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, 1);
            }
        }

        services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
    }
};

//! Texture coordinates generated by sphere mapping.
class COpenGLMaterialRenderer_SPHERE_MAP : public COpenGLMaterialRenderer
{
public:
    COpenGLMaterialRenderer_SPHERE_MAP(CVideoOpenGL* d)
        : COpenGLMaterialRenderer(d) {}

    virtual void OnSetMaterial(ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
        bool resetAllRenderstates, ox::video::IMaterialRendererServices* services)
    {
        if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
        {
            if (Driver->hasMultiTextureExtension())
            {
                Driver->extGlActiveTextureARB(GL_TEXTURE1_ARB);
                glDisable(GL_TEXTURE_2D);
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

                Driver->extGlActiveTextureARB(GL_TEXTURE0_ARB);
            }

            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glDisable(GL_BLEND);
            glDisable(GL_ALPHA_TEST);

            glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

            glEnable(GL_TEXTURE_GEN_S);
            glEnable(GL_TEXTURE_GEN_T);
        }

        services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
    }

    virtual void OnUnsetMaterial()
    {
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
    }
};

//! Sphere mapped reflection.
class COpenGLMaterialRenderer_REFLECTION_2_LAYER : public COpenGLMaterialRenderer
{
public:
    COpenGLMaterialRenderer_REFLECTION_2_LAYER(CVideoOpenGL* d)
        : COpenGLMaterialRenderer(d) {}

    virtual void OnSetMaterial(ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
        bool resetAllRenderstates, ox::video::IMaterialRendererServices* services)
    {
        if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
        {
            if (Driver->hasMultiTextureExtension())
            {
                Driver->extGlActiveTextureARB(GL_TEXTURE1_ARB);
                glDisable(GL_TEXTURE_2D);
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

                Driver->extGlActiveTextureARB(GL_TEXTURE0_ARB);
            }

            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glDisable(GL_BLEND);
            glDisable(GL_ALPHA_TEST);

            glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

            glEnable(GL_TEXTURE_GEN_S);
            glEnable(GL_TEXTURE_GEN_T);
        }

        services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
    }

    virtual void OnUnsetMaterial()
    {
        if (Driver->hasMultiTextureExtension())
        {
            Driver->extGlActiveTextureARB(GL_TEXTURE1_ARB);
            glDisable(GL_TEXTURE_2D);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

            Driver->extGlActiveTextureARB(GL_TEXTURE0_ARB);
        }

        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
    }
};

//! Sphere mapped reflection blended like TRANSPARENT_ADD_COLOR.
class COpenGLMaterialRenderer_TRANSPARENT_REFLECTION_2_LAYER : public COpenGLMaterialRenderer
{
public:
    COpenGLMaterialRenderer_TRANSPARENT_REFLECTION_2_LAYER(CVideoOpenGL* d)
        : COpenGLMaterialRenderer(d) {}

    virtual void OnSetMaterial(ox::video::SMaterial& material, const ox::video::SMaterial& lastMaterial,
        bool resetAllRenderstates, ox::video::IMaterialRendererServices* services)
    {
        if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
        {
            if (Driver->hasMultiTextureExtension())
            {
                Driver->extGlActiveTextureARB(GL_TEXTURE1_ARB);
                glDisable(GL_TEXTURE_2D);
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

                Driver->extGlActiveTextureARB(GL_TEXTURE0_ARB);
            }

            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glEnable(GL_BLEND);
            glDisable(GL_ALPHA_TEST);

            glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

            glEnable(GL_TEXTURE_GEN_S);
            glEnable(GL_TEXTURE_GEN_T);
        }

        material.ZWriteEnable = false;
        services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
    }

    virtual void OnUnsetMaterial()
    {
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
    }

    virtual bool isTransparent() { return true; }
};

} // end namespace video
} // end namespace daisy

#endif
