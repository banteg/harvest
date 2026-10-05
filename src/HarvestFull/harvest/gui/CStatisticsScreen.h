// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_GUI_CSTATISTICSSCREEN_H
#define HARVEST_GUI_CSTATISTICSSCREEN_H

#include "ox/TArray.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/CRect.h"
#include "ox/core/CString.h"
#include "ox/event/IEventReceiver.h"
#include "ox/video/SColor.h"

namespace ox {
class IOxDevice;
namespace gui {
class IGUIButton;
class IGUIElement;
class IGUIEnvironment;
class IGUIFont;
class IGUIStaticText;
} // end namespace gui
namespace net { class CHTTPConnectionHandler; }
namespace video {
class ISpriteAnimationState;
class IVideoDriver;
} // end namespace video
} // end namespace ox

namespace harvest {
namespace gui {

//! A bar of a statistics graph: one wave's value, fading in from left to right.
struct SGraphColumn
{
    //! The wave.
    int Level;
    float Value;
    ox::core::CRect<int> Rect;
    //! Opacity, from 0 (hidden) up; grows while the screen updates.
    float Alpha;
    //! The popup shown while the mouse is over the bar.
    ox::core::CString<wchar_t> Text;
};

//! The statistics shown after a game: wave graphs, the score, the event log and the highscore upload.
class CStatisticsScreen : public ox::event::IEventReceiver
{
public:
    CStatisticsScreen(ox::IOxDevice* device);
    virtual ~CStatisticsScreen();

    virtual bool OnEvent(const ox::event::SEvent& event);

    void setVisible(bool visible);
    bool isVisible();
    //! Fades in the graph bars.
    void update(float time);

private:
    enum
    {
        //! The graph selection buttons, one per graph type.
        ID_GRAPH_BUTTON = 6432,
        ID_GRAPH = ID_GRAPH_BUTTON + 8,
        ID_SUBMIT_HIGHSCORE,
        ID_WINDOW,
        ID_PAGE_UP,
        ID_PAGE_DOWN,
        ID_BACK
    };

    //! The graph types: the level statistics of harvest::game::SLevelStats, then the overview.
    enum
    {
        GRAPH_COUNT = 7,
        GRAPH_OVERALL = GRAPH_COUNT
    };

    //! The sprites of STAT_SPRITE_NAMES.
    enum
    {
        SPRITE_TOP,
        SPRITE_BOTTOM,
        SPRITE_DIAGRAM_BACKGROUND,
        SPRITE_OVERALL_BACKGROUND,
        SPRITE_DIAGRAM_BAR,
        SPRITE_PLANET_ICON,
        SPRITE_MODE_ICON = SPRITE_PLANET_ICON + 3,
        SPRITE_SCORE = SPRITE_MODE_ICON + 5,
        SPRITE_CREDITS,
        SPRITE_COUNT
    };

    //! The info lines shown at once; the page buttons scroll by this many.
    enum { INFO_PAGE_LINES = 14 };

    void eraseGraphs();
    void createGraphs(int type);
    void updateTopString();
    //! Tiles the bar sprite from position over the bar rectangle.
    void renderGraph(const ox::core::CPosition2d<int>& position, const ox::core::CRect<int>& clip,
        ox::video::SColor& color);
    void submitHighscores();
    void addInfoString(const ox::core::CString<wchar_t>& text);
    void loadSprites();

    ox::IOxDevice* Device;
    ox::gui::IGUIEnvironment* GUIEnvironment;
    ox::video::IVideoDriver* Driver;
    ox::gui::IGUIFont* Font;
    bool SpritesLoaded;
    int GraphType;
    ox::TArray<SGraphColumn*> Graphs;
    //! The graph panel's area, and the area the bars fill.
    ox::core::CRect<int> GraphFrame;
    ox::core::CRect<int> GraphArea;
    //! The six value labels of the graph's vertical axis.
    ox::core::CString<wchar_t> AxisLabels[6];
    ox::core::CDimension2d<int> AxisLabelSizes[6];
    ox::core::CRect<int> AxisLabelRects[6];
    ox::video::ISpriteAnimationState* Sprites[SPRITE_COUNT];
    ox::core::CPosition2d<int> SpriteSizes[SPRITE_COUNT];
    ox::gui::IGUIElement* Window;
    ox::gui::IGUIElement* Frame;
    ox::gui::IGUIButton* GraphButtons[GRAPH_COUNT + 1];
    ox::gui::IGUIElement* GraphPanel;
    ox::gui::IGUIElement* InfoPanel;
    // Not recovered yet.
    void* Unrecovered[4];
    ox::TArray<ox::gui::IGUIStaticText*> InfoStrings;
    //! The first info line shown.
    int InfoScroll;
    ox::net::CHTTPConnectionHandler* Connection;
};

} // end namespace gui
} // end namespace harvest

#endif
