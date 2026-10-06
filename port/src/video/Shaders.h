// The renderer's GLSL sources (port/src/video/shaders/), embedded at compile time. Each is a shader
// body without a #version line; CFixedFunction::createProgram prepends the one for the context.

#ifndef PORT_VIDEO_SHADERS_H
#define PORT_VIDEO_SHADERS_H

namespace port {
namespace video {

//! The fixed-function emulation.
extern const char FixedVertexShader[];
extern const char FixedFragmentShader[];

//! The GLSL translation of one of the game's Cg files in harvestClientData/gfx/shaders/.
struct SCgTranslation
{
    //! The Cg file's name, without directories; matched case-insensitively.
    const char* FileName;
    //! true for a .vsh (vertex program), false for a .psh (pixel program).
    bool Vertex;
    const char* Source;
};

//! The translations, terminated by an entry with a null FileName.
extern const SCgTranslation CgTranslations[];

//! The texture unit of each sampler the translated pixel shaders declare (Cg's TEXn semantics).
struct SSamplerUnit
{
    const char* Name;
    int Unit;
};

//! Terminated by an entry with a null Name.
extern const SSamplerUnit CgSamplerUnits[];

} // end namespace video
} // end namespace port

#endif
