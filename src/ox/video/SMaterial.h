// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/SMaterial.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::video namespace; not the original source. The Linux amd64 offsets of
// the textures (0x18) and of all twelve flags (0x28..0x33) are verified against CVideoOpenGL's
// setBasicRenderStates; the Oxeye material has no MaterialTypeParam before the textures.

#ifndef OX_VIDEO_SMATERIAL_H
#define OX_VIDEO_SMATERIAL_H

#include "SColor.h"

namespace ox {
namespace video {

class ITexture;

//! Material types, as in Irrlicht 0.7.
enum E_MATERIAL_TYPE
{
    EMT_SOLID = 0,
    EMT_SOLID_2_LAYER,
    EMT_LIGHTMAP,
    EMT_LIGHTMAP_ADD,
    EMT_LIGHTMAP_M2,
    EMT_LIGHTMAP_M4,
    EMT_LIGHTMAP_LIGHTING,
    EMT_LIGHTMAP_LIGHTING_M2,
    EMT_LIGHTMAP_LIGHTING_M4,
    EMT_SPHERE_MAP,
    EMT_REFLECTION_2_LAYER,
    EMT_TRANSPARENT_ADD_COLOR,
    EMT_TRANSPARENT_ALPHA_CHANNEL,
    EMT_TRANSPARENT_VERTEX_ALPHA,
    EMT_TRANSPARENT_REFLECTION_2_LAYER,
    EMT_FORCE_32BIT = 0x7fffffff
};

//! Maximal number of textures of a material.
const int MATERIAL_MAX_TEXTURES = 2;

//! Material flags: Irrlicht 0.7's, with Oxeye's front face flag and texture mirroring added.
enum E_MATERIAL_FLAG
{
    EMF_WIREFRAME = 0,
    EMF_GOURAUD_SHADING,
    EMF_LIGHTING,
    EMF_ZBUFFER,
    EMF_ZWRITE_ENABLE,
    EMF_BACK_FACE_CULLING,
    EMF_FRONT_FACE_CCW,
    EMF_BILINEAR_FILTER,
    EMF_TRILINEAR_FILTER,
    EMF_FOG_ENABLE,
    EMF_TEXTURE_MIRROR_U,
    EMF_TEXTURE_MIRROR_V,
    EMF_MATERIAL_FLAG_COUNT
};

//! Material of a mesh buffer or scene node.
struct SMaterial
{
    //! The defaults are the values the Linux and Mac CGUIMeshViewer and CVideoOpenGL constructors
    //! store; the textures are cleared after the flags.
    SMaterial()
        : MaterialType(EMT_SOLID), AmbientColor(0xffffffff), DiffuseColor(0xffffffff), EmissiveColor(0),
          SpecularColor(0), Shininess(0.0f)
    {
        Wireframe = false;
        GouraudShading = true;
        Lighting = true;
        ZBuffer = true;
        ZWriteEnable = true;
        BackfaceCulling = true;
        FrontFaceCCW = false;
        BilinearFilter = true;
        TrilinearFilter = false;
        FogEnable = false;
        TextureMirrorU = false;
        TextureMirrorV = false;
        Texture1 = 0;
        Texture2 = 0;
    }

    E_MATERIAL_TYPE MaterialType;
    SColor AmbientColor;
    SColor DiffuseColor;
    SColor EmissiveColor;
    SColor SpecularColor;
    float Shininess;

    union
    {
        struct
        {
            ITexture* Texture1;
            ITexture* Texture2;
        };
        ITexture* Textures[MATERIAL_MAX_TEXTURES];
    };

    union
    {
        struct
        {
            bool Wireframe;
            bool GouraudShading;
            bool Lighting;
            bool ZBuffer;
            bool ZWriteEnable;
            bool BackfaceCulling;
            //! Oxeye: front faces wind counterclockwise (glFrontFace(GL_CCW)) instead of the
            //! Direct3D-style clockwise default, so back face culling removes the other side.
            bool FrontFaceCCW;
            //! Linear magnification filter; nearest when off.
            bool BilinearFilter;
            bool TrilinearFilter;
            bool FogEnable;
            //! Oxeye: GL_MIRRORED_REPEAT instead of GL_REPEAT along u (s) and v (t).
            bool TextureMirrorU;
            bool TextureMirrorV;
        };
        bool Flags[EMF_MATERIAL_FLAG_COUNT];
    };

    //! True if anything the video driver applies differs; defined in SMaterialInline.h.
    bool operator!=(const SMaterial& other) const;
};

} // end namespace video
} // end namespace ox

#endif
