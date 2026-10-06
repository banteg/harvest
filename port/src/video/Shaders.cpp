// Embeds the GLSL files in shaders/ with #embed (a C23 feature clang accepts in every language
// mode), so each stays a plain GLSL file.

#include "video/Shaders.h"

#pragma clang diagnostic ignored "-Wc23-extensions"

namespace port {
namespace video {

const char FixedVertexShader[] = {
#embed "shaders/fixed.vert"
    , 0};
const char FixedFragmentShader[] = {
#embed "shaders/fixed.frag"
    , 0};

namespace {

const char ScatterAtmoVertex[] = {
#embed "shaders/scatterAtmoCG.vert"
    , 0};
const char ScatterAtmoPixel[] = {
#embed "shaders/scatterAtmoCG.frag"
    , 0};
const char ScatterGroundVertex[] = {
#embed "shaders/scatterGroundCG.vert"
    , 0};
const char ScatterGroundPixel[] = {
#embed "shaders/scatterGroundCG.frag"
    , 0};
const char ScatterGroundSimpleVertex[] = {
#embed "shaders/scatterGroundSCG.vert"
    , 0};
const char ScatterGroundSimplePixel[] = {
#embed "shaders/scatterGroundSCG.frag"
    , 0};

} // end anonymous namespace

const SCgTranslation CgTranslations[] = {
    {"scatterAtmoCG.vsh", true, ScatterAtmoVertex},
    {"scatterAtmoCG.psh", false, ScatterAtmoPixel},
    {"scatterGroundCG.vsh", true, ScatterGroundVertex},
    {"scatterGroundCG.psh", false, ScatterGroundPixel},
    {"scatterGroundSCG.vsh", true, ScatterGroundSimpleVertex},
    {"scatterGroundSCG.psh", false, ScatterGroundSimplePixel},
    {0, false, 0},
};

const SSamplerUnit CgSamplerUnits[] = {
    {"ps_DiffSpec", 0},
    {"ps_NormGlow", 1},
    {0, 0},
};

} // end namespace video
} // end namespace port
