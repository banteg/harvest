// The GUI's built-in font image (src/daisy/gui/BuildInFont.h). The original's 8310 bytes at Linux
// 0x868fe0 are Irrlicht 0.7's BuildInFont.h byte for byte; that header stores them as 32-bit words,
// so they are unpacked here in little-endian order.

#include "daisy/gui/BuildInFont.h"

namespace {
typedef int s32;
// Irrlicht's header defines its arrays in irr::gui; keep them private to this file.
#define irr irrlicht_font
#include "../../../third_party/irrlicht-0.7/source/Irrlicht/BuildInFont.h"
#undef irr
} // end anonymous namespace

namespace daisy {
namespace gui {

int BuildInFontDataSize = 8310;
unsigned char BuildInFontData[8310];

namespace {

//! Fills BuildInFontData before main; the GUI environment reads it when it is created.
struct UnpackBuildInFont
{
    UnpackBuildInFont()
    {
        for (int i = 0; i < BuildInFontDataSize; ++i)
        {
            unsigned int word = (unsigned int)irrlicht_font::gui::BuildInFontData[i / 4];
            BuildInFontData[i] = (unsigned char)(word >> (i % 4 * 8));
        }
    }
} unpackBuildInFont;

} // end anonymous namespace

} // end namespace gui
} // end namespace daisy
