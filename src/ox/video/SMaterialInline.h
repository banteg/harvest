// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Inline body of SMaterial::operator!=, kept out of SMaterial.h so that units which only store
// materials do not see it. The Mac build emits it as a COMDAT in CVideoOpenGL.o; Linux inlines it.

#ifndef OX_VIDEO_SMATERIALINLINE_H
#define OX_VIDEO_SMATERIALINLINE_H

#include "SMaterial.h"

namespace ox {
namespace video {

inline bool SMaterial::operator!=(const SMaterial& other) const
{
    bool sameTextures = true;
    for (int i = 0; i < MATERIAL_MAX_TEXTURES; ++i)
        sameTextures &= Textures[i] == other.Textures[i];

    return MaterialType != other.MaterialType || AmbientColor.color != other.AmbientColor.color ||
        DiffuseColor.color != other.DiffuseColor.color || EmissiveColor.color != other.EmissiveColor.color ||
        SpecularColor.color != other.SpecularColor.color || Shininess != other.Shininess ||
        Wireframe != other.Wireframe || GouraudShading != other.GouraudShading || Lighting != other.Lighting ||
        ZBuffer != other.ZBuffer || ZWriteEnable != other.ZWriteEnable || BackfaceCulling != other.BackfaceCulling ||
        FrontFaceCCW != other.FrontFaceCCW || BilinearFilter != other.BilinearFilter ||
        TrilinearFilter != other.TrilinearFilter || FogEnable != other.FogEnable ||
        TextureMirrorU != other.TextureMirrorU || TextureMirrorV != other.TextureMirrorV || !sameTextures;
}

} // end namespace video
} // end namespace ox

#endif
