// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef DAISY_GUI_CGUIDETACHABLEFRAME_H
#define DAISY_GUI_CGUIDETACHABLEFRAME_H

#include <vector>
#include "CGUIWindow.h"
#include "ox/gui/IGUIDetachableFrame.h"

namespace ox {
namespace gui {
class IGUIButton;
class IGUIPopupMenu;
}
}

namespace daisy {
namespace gui {

//! An entry that a detachable frame adds to its options menu.
struct SFrameMenuOption
{
    ox::core::CString<wchar_t> Text;
    int ID;
};

//! Oxeye's frame that the user can drag and resize, which snaps to the layout groups next to it, can be
//! locked in place from its options menu and fades out when the mouse leaves it. The game's screens are
//! built in its content area.
class CGUIDetachableFrame : public ox::gui::IGUIDetachableFrame
{
public:
    //! A fade delay of 0 hides a fadeable frame as soon as the mouse leaves it.
    CGUIDetachableFrame(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle, unsigned int fadeDelay, bool hasMenu, bool hideable);
    virtual ~CGUIDetachableFrame();

    virtual void setRelativePosition(const ox::core::CRect<int>& position);
    virtual void draw();
    virtual bool OnEvent(const ox::event::SEvent& event);
    virtual ox::core::CRect<int> getContentArea();

    virtual void setAnimations(ox::video::ISpritePackage* package, const char* animation);
    virtual void setPopupMenuTitleFont(ox::gui::IGUIFont* font);
    virtual void setPopupMenuItemsFont(ox::gui::IGUIFont* font);
    virtual void addMenuOption(const wchar_t* text, int id);
    virtual void setMenuVisible(bool visible);
    virtual void setResizable(bool resizable, const ox::core::CDimension2d<int>& minimumSize);
    virtual void setLocked(bool locked);
    virtual void setFadeable(bool fadeable);
    virtual void setLockable(bool lockable);
    virtual void setHideable(bool hideable);
    virtual void stopDragging();

private:
    //! Moves the menu and close buttons into the top corners.
    void updateButtonPositions();
    //! Stretches the edge and background rectangles to the frame size.
    void updateAnimationRects();
    //! Moves a dragged rectangle back inside a layout group.
    void lockToItemInside(ox::gui::IGUIElement* item, ox::core::CRect<int>& rect, bool& lockedX, bool& lockedY);
    //! Snaps a dragged rectangle to the edges of a nearby layout group.
    void snapToItem(ox::gui::IGUIElement* item, ox::core::CRect<int>& rect, bool& snappedX, bool& snappedY);

    ox::core::CPosition2d<int> DragStart;
    //! The absolute position of the frame when the drag started.
    ox::core::CPosition2d<int> DragStartPosition;
    bool Dragging;
    bool Resizing;
    bool Resizable;
    ox::core::CDimension2d<int> MinimumSize;

    ox::video::ISpriteAnimationState* Animations[EWP_COUNT];
    ox::core::CRect<int> AnimationRects[EWP_COUNT];
    ox::video::ISpriteAnimationState* InnerAnimations[EWIP_COUNT];
    ox::core::CRect<int> InnerAnimationRects[EWIP_COUNT];
    ox::video::ISpriteAnimationState* SizeDragHandle;
    ox::core::CRect<int> SizeDragHandleRect;

    //! When the mouse last moved over the frame, 0 before it first did.
    unsigned int LastHoverTime;
    unsigned int FadeDelay;
    //! Set while a fadeable frame is drawn without its frame.
    bool FadedOut;
    bool Locked;
    bool Lockable;
    bool Fadeable;
    bool Hideable;
    bool MenuVisible;

    ox::gui::IGUIButton* MenuButton;
    ox::gui::IGUIButton* CloseButton;
    ox::gui::IGUIFont* PopupMenuTitleFont;
    ox::gui::IGUIFont* PopupMenuItemsFont;
    ox::gui::IGUIPopupMenu* PopupMenu;
    std::vector<SFrameMenuOption> MenuOptions;
};

} // end namespace gui
} // end namespace daisy

#endif
