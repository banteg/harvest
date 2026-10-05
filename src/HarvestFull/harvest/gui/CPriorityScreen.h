// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_GUI_CPRIORITYSCREEN_H
#define HARVEST_GUI_CPRIORITYSCREEN_H

#include <vector>
#include "ox/core/CDimension2d.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/CRect.h"
#include "ox/core/CString.h"
#include "ox/event/IEventReceiver.h"

namespace ox {
class IOxDevice;
namespace gui {
class IGUICheckBox;
class IGUIElement;
class IGUIEnvironment;
class IGUIFont;
} // end namespace gui
namespace video {
class ISpriteAnimationState;
class IVideoDriver;
} // end namespace video
} // end namespace ox

namespace harvest {
namespace game { class CThreatLevel; }
namespace gui {

//! An alien icon on a priority display, dragged up and down between the priority rows.
struct SPriorityBox
{
    int AlienType;
    int Priority;
    ox::core::CRect<int> Rect;
};

//! The screen where the player sets the alien priorities of the five weapon types.
class CPriorityScreen : public ox::event::IEventReceiver
{
public:
    CPriorityScreen(ox::IOxDevice* device);
    virtual ~CPriorityScreen();

    virtual bool OnEvent(const ox::event::SEvent& event);

    //! Shows or hides the screen; aliens not present at the threat level are dimmed.
    void setVisible(bool visible, game::CThreatLevel* threatLevel);
    bool isVisible();

private:
    enum
    {
        ID_CONFIRM = 90000,
        ID_CANCEL,
        ID_FRAME,
        ID_WINDOW,
        //! The priority displays of the five weapon types.
        ID_PRIORITY_DISPLAY
    };

    enum ESprite
    {
        ES_LEFT_BACKGROUND,
        ES_RIGHT_BACKGROUND,
        ES_DISPLAY_BACKGROUND,
        ES_ICON_FRAME_NORMAL,
        ES_ICON_FRAME_HIGHLIGHTED,
        ES_ICON_FRAME_PRESSED,
        ES_ARROW_UP,
        ES_ARROW_DOWN,
        ES_COUNT
    };

    void emptyPriorityBoxes();
    void savePrioritiesByGUI();
    void updatePriorityBoxPositions();
    void loadSprites();
    void createPriorityBoxes();

    ox::IOxDevice* Device;
    ox::gui::IGUIEnvironment* GUIEnvironment;
    ox::video::IVideoDriver* Driver;
    game::CThreatLevel* ThreatLevel;
    ox::gui::IGUIElement* Window;
    ox::gui::IGUIElement* Frame;
    ox::video::ISpriteAnimationState* AlienIcons[14];
    std::vector<SPriorityBox*> PriorityBoxes[5];
    ox::gui::IGUIElement* PriorityDisplays[5];
    ox::gui::IGUICheckBox* TargetClosestBoxes[5];
    ox::gui::IGUICheckBox* HoldFireBoxes[5];
    ox::video::ISpriteAnimationState* BuildingIcons[5];
    ox::video::ISpriteAnimationState* Sprites[ES_COUNT];
    ox::core::CDimension2d<int> SpriteSizes[ES_COUNT];
    bool Dragging;
    SPriorityBox* DraggedBox;
    //! The mouse position where the dragged box last changed priority.
    int DragX;
    int DragY;
    ox::gui::IGUIFont* Font;
    //! Set while the mouse hovers a box; the window draws the alien name below it.
    bool ShowTooltip;
    ox::core::CString<wchar_t> TooltipText;
    int TooltipX;
    int TooltipY;
};

} // end namespace gui
} // end namespace harvest

#endif
