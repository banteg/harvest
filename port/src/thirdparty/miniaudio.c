// miniaudio's implementation with stb_vorbis for Ogg Vorbis, the game's music and sound format.
// This is the arrangement miniaudio documents: stb_vorbis's header, miniaudio, then stb_vorbis's
// implementation. Users include <miniaudio.h>.

#define STB_VORBIS_HEADER_ONLY
#include <extras/stb_vorbis.c>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#undef STB_VORBIS_HEADER_ONLY
#include <extras/stb_vorbis.c>
