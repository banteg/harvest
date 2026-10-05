// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what CPlayState uses is declared; the tail keeps the Linux object size.

#ifndef HARVEST_GUI_CGUIINFOLINES_H
#define HARVEST_GUI_CGUIINFOLINES_H

namespace ox {
class IOxDevice;
namespace video { class ISpritePackage; }
} // end namespace ox

namespace harvest {
namespace gui {

//! The message lines shown at the top of the game screen.
class CGuiInfoLines
{
public:
    CGuiInfoLines(ox::IOxDevice* device);
    virtual ~CGuiInfoLines();

    void update(float frameDelta, int width);
    //! Adds a plain line of text.
    void addInfoLine(const wchar_t* text);
    //! Adds a message with a title and a portrait animation of the package.
    void addInfoLine(const wchar_t* title, const wchar_t* text, ox::video::ISpritePackage* package,
        const char* portrait);

private:
    // Not recovered yet; keeps the Linux object size of 96 bytes.
    char Unrecovered[96 - sizeof(void*)];
};

} // end namespace gui
} // end namespace harvest

#endif
