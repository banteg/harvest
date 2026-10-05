// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGPUProgrammingServices.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::video namespace; not the original source. The virtual order follows
// the Mac 1.18 vtable of daisy::video::CVideoNull, which adds the Cg shader functions.

#ifndef OX_VIDEO_IGPUPROGRAMMINGSERVICES_H
#define OX_VIDEO_IGPUPROGRAMMINGSERVICES_H

#include "SMaterial.h"

namespace ox {
namespace io { class IReadFile; }
namespace video {

class IShaderConstantSetCallBack;

//! Vertex shader versions; the enumerators are not recovered.
enum E_VERTEX_SHADER_TYPE
{
};

//! Pixel shader versions; the enumerators are not recovered.
enum E_PIXEL_SHADER_TYPE
{
};

//! Creates shader-based materials. Each function returns the new material type, or -1 on failure.
class IGPUProgrammingServices
{
public:
    virtual int addHighLevelShaderMaterial(const char* vertexShaderProgram, const char* vertexShaderEntryPointName,
        E_VERTEX_SHADER_TYPE vsCompileTarget, const char* pixelShaderProgram, const char* pixelShaderEntryPointName,
        E_PIXEL_SHADER_TYPE psCompileTarget, IShaderConstantSetCallBack* callback, E_MATERIAL_TYPE baseMaterial,
        int userData) = 0;
    virtual int addHighLevelShaderMaterialFromFiles(const char* vertexShaderProgramFile,
        const char* vertexShaderEntryPointName, E_VERTEX_SHADER_TYPE vsCompileTarget,
        const char* pixelShaderProgramFile, const char* pixelShaderEntryPointName, E_PIXEL_SHADER_TYPE psCompileTarget,
        IShaderConstantSetCallBack* callback, E_MATERIAL_TYPE baseMaterial, int userData) = 0;
    virtual int addHighLevelShaderMaterialFromFiles(io::IReadFile* vertexShaderProgram,
        const char* vertexShaderEntryPointName, E_VERTEX_SHADER_TYPE vsCompileTarget,
        io::IReadFile* pixelShaderProgram, const char* pixelShaderEntryPointName, E_PIXEL_SHADER_TYPE psCompileTarget,
        IShaderConstantSetCallBack* callback, E_MATERIAL_TYPE baseMaterial, int userData) = 0;
    virtual int addShaderMaterial(const char* vertexShaderProgram, const char* pixelShaderProgram,
        IShaderConstantSetCallBack* callback, E_MATERIAL_TYPE baseMaterial, int userData) = 0;
    virtual int addShaderMaterialFromFiles(io::IReadFile* vertexShaderProgram, io::IReadFile* pixelShaderProgram,
        IShaderConstantSetCallBack* callback, E_MATERIAL_TYPE baseMaterial, int userData) = 0;
    virtual int addShaderMaterialFromFiles(const char* vertexShaderProgramFileName,
        const char* pixelShaderProgramFileName, IShaderConstantSetCallBack* callback, E_MATERIAL_TYPE baseMaterial,
        int userData) = 0;
    virtual int addCgShaderMaterial(const char* vertexShaderProgram, const char* vertexShaderEntryPointName,
        const char* pixelShaderProgram, const char* pixelShaderEntryPointName, IShaderConstantSetCallBack* callback,
        E_MATERIAL_TYPE baseMaterial, bool flag, int userData) = 0;
    //! The meaning of flag is not recovered.
    virtual int addCgShaderMaterialFromFiles(const char* vertexShaderProgramFileName,
        const char* vertexShaderEntryPointName, const char* pixelShaderProgramFileName,
        const char* pixelShaderEntryPointName, IShaderConstantSetCallBack* callback, E_MATERIAL_TYPE baseMaterial,
        bool flag, int userData) = 0;
    virtual int addCgShaderMaterialFromFiles(io::IReadFile* vertexShaderProgram, const char* vertexShaderEntryPointName,
        io::IReadFile* pixelShaderProgram, const char* pixelShaderEntryPointName, IShaderConstantSetCallBack* callback,
        E_MATERIAL_TYPE baseMaterial, bool flag, int userData) = 0;
};

} // end namespace video
} // end namespace ox

#endif
