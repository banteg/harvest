// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIWindow.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// The object also holds Oxeye's CGUIDetachableFrame, which shares the window's sprite drawing.

#include "CGUIWindow.h"
#include "CGUIDetachableFrame.h"
#include "GUIIcons.h"
#include "daisy/os.h"
#include "ox/gui/IGUIButton.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIFont.h"
#include "ox/gui/IGUILayoutInline.h"
#include "ox/gui/IGUIPopupMenu.h"
#include "ox/gui/IGUISkin.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/IVideoDriver.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

namespace {

//! Clips a rectangle against another, as Irrlicht's rect::clipAgainst.
inline void clipAgainst(ox::core::CRect<int>& rect, const ox::core::CRect<int>& other)
{
    if (other.LowerRightCorner.X < rect.LowerRightCorner.X)
        rect.LowerRightCorner.X = other.LowerRightCorner.X;
    if (other.LowerRightCorner.Y < rect.LowerRightCorner.Y)
        rect.LowerRightCorner.Y = other.LowerRightCorner.Y;

    if (other.UpperLeftCorner.X > rect.UpperLeftCorner.X)
        rect.UpperLeftCorner.X = other.UpperLeftCorner.X;
    if (other.UpperLeftCorner.Y > rect.UpperLeftCorner.Y)
        rect.UpperLeftCorner.Y = other.UpperLeftCorner.Y;
}

} // end anonymous namespace

//! constructor
CGUIWindow::CGUIWindow(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
    ox::core::CRect<int> rectangle, bool isFrame)
    : IGUIWindow(environment, parent, id, rectangle), Dragging(false), IsFrame(isFrame)
{
    // a skin with a frame background but no window background draws every window as a frame
    if (!IsFrame && Environment && Environment->getSkin() && Environment->getSkin()->getSpritePackage())
    {
        ox::video::ISpritePackage* package = Environment->getSkin()->getSpritePackage();
        if (package->animationStateExists("FrameBackground") && !package->animationStateExists("WindowBackground"))
            IsFrame = true;
    }

    if (IsFrame)
    {
        MinButton = 0;
        RestoreButton = 0;
        CloseButton = 0;
    }
    else
    {
        CloseButton = Environment->addButton(ox::core::CRect<int>(0, 0, 100, 20), this, -1, GUI_ICON_WINDOW_CLOSE);
        CloseButton->setOverrideFont(Environment->getBuiltInFont());
        CloseButton->setFixed(true);

        int buttonw = CloseButton->getRelativePosition().getWidth();
        int posx = RelativeRect.getWidth() - buttonw - 4;
        CloseButton->setRelativePosition(ox::core::CRect<int>(posx, 3, posx + buttonw, 3 + buttonw));
        posx -= buttonw + 2;

        RestoreButton = Environment->addButton(ox::core::CRect<int>(posx, 3, posx + buttonw, 3 + buttonw), this, -1,
            GUI_ICON_WINDOW_RESTORE);
        RestoreButton->setOverrideFont(Environment->getBuiltInFont());
        RestoreButton->setVisible(false);
        RestoreButton->setFixed(true);
        posx -= buttonw + 2;

        MinButton = Environment->addButton(ox::core::CRect<int>(posx, 3, posx + buttonw, 3 + buttonw), this, -1,
            GUI_ICON_WINDOW_MINIMIZE);
        MinButton->setOverrideFont(Environment->getBuiltInFont());
        MinButton->setFixed(true);
        MinButton->setVisible(false);

        MinButton->grab();
        RestoreButton->grab();
        CloseButton->grab();
    }

    for (int i = 0; i < EWP_COUNT; ++i)
        Animations[i] = 0;
    for (int i = 0; i < EWIP_COUNT; ++i)
        InnerAnimations[i] = 0;

    if (Environment && Environment->getSkin() && Environment->getSkin()->getSpritePackage())
    {
        if (IsFrame)
            setAnimations(Environment->getSkin()->getSpritePackage(), "Frame");
        else
            setAnimations(Environment->getSkin()->getSpritePackage(), "Window");
    }
}

//! destructor
CGUIWindow::~CGUIWindow()
{
    if (MinButton)
        MinButton->drop();

    if (RestoreButton)
        RestoreButton->drop();

    if (CloseButton)
        CloseButton->drop();

    for (int i = 0; i < EWP_COUNT; ++i)
        if (Animations[i])
            Animations[i]->remove();

    for (int i = 0; i < EWIP_COUNT; ++i)
        if (InnerAnimations[i])
            InnerAnimations[i]->remove();
}

void CGUIWindow::setRelativePosition(const ox::core::CRect<int>& position)
{
    IGUIElement::setRelativePosition(position);
    updateAnimationRects();

    if (CloseButton)
    {
        ox::core::CRect<int> buttonRect = CloseButton->getRelativePosition();
        int width = buttonRect.getWidth();
        int offsetY = buttonRect.UpperLeftCorner.Y - buttonRect.LowerRightCorner.Y;
        if (Animations[EWP_TOP_RIGHT])
        {
            width += Animations[EWP_TOP_RIGHT]->getFrameSize(0).Width;
            offsetY += Animations[EWP_TOP_RIGHT]->getFrameSize(0).Height;
        }

        CloseButton->moveTo(ox::core::CPosition2d<int>(RelativeRect.getWidth() - width, offsetY / 2));
    }
}

void CGUIWindow::updateAnimationRects()
{
    int width = RelativeRect.getWidth();
    int height = RelativeRect.getHeight();

    AnimationRects[EWP_TOP_RIGHT].UpperLeftCorner.X = width - AnimationRects[EWP_TOP_RIGHT].getWidth();
    AnimationRects[EWP_TOP_RIGHT].LowerRightCorner.X = width;

    AnimationRects[EWP_TOP].UpperLeftCorner.X = AnimationRects[EWP_TOP_LEFT].getWidth();
    AnimationRects[EWP_TOP].LowerRightCorner.X = AnimationRects[EWP_TOP_RIGHT].UpperLeftCorner.X;

    AnimationRects[EWP_BOTTOM_LEFT].UpperLeftCorner.Y = height - AnimationRects[EWP_BOTTOM_LEFT].getHeight();
    AnimationRects[EWP_BOTTOM_LEFT].LowerRightCorner.Y = height;

    int bottomRightWidth = AnimationRects[EWP_BOTTOM_RIGHT].getWidth();
    AnimationRects[EWP_BOTTOM_RIGHT].UpperLeftCorner.X = width - bottomRightWidth;
    AnimationRects[EWP_BOTTOM_RIGHT].UpperLeftCorner.Y = AnimationRects[EWP_BOTTOM_LEFT].UpperLeftCorner.Y;
    AnimationRects[EWP_BOTTOM_RIGHT].LowerRightCorner.X = width;
    AnimationRects[EWP_BOTTOM_RIGHT].LowerRightCorner.Y = height;

    // the bottom edge starts at the bottom right corner's width
    AnimationRects[EWP_BOTTOM] = ox::core::CRect<int>(bottomRightWidth,
        AnimationRects[EWP_BOTTOM_LEFT].UpperLeftCorner.Y, AnimationRects[EWP_BOTTOM_RIGHT].UpperLeftCorner.X,
        height);

    int sideWidth = AnimationRects[EWP_LEFT].getWidth();
    int top = AnimationRects[EWP_TOP_RIGHT].getHeight();
    AnimationRects[EWP_LEFT] =
        ox::core::CRect<int>(0, top, sideWidth, AnimationRects[EWP_BOTTOM_LEFT].UpperLeftCorner.Y);

    // the right edge is as wide as the left one
    AnimationRects[EWP_RIGHT] = ox::core::CRect<int>(width - sideWidth, top, width,
        AnimationRects[EWP_BOTTOM_LEFT].UpperLeftCorner.Y);

    AnimationRects[EWP_BACKGROUND] = ox::core::CRect<int>(sideWidth, top, width - sideWidth,
        AnimationRects[EWP_BOTTOM_LEFT].UpperLeftCorner.Y);
}

//! called if an event happened.
bool CGUIWindow::OnEvent(const ox::event::SEvent& event)
{
    if (EventReceiver && !Dragging && EventReceiver->OnEvent(event))
        return true;

    switch (event.EventType)
    {
    case ox::event::EET_GUI_EVENT:
        if (event.GUIEvent.EventType == ox::gui::EGET_ELEMENT_FOCUS_LOST)
        {
            Dragging = false;
            return true;
        }
        else if (event.GUIEvent.EventType == ox::gui::EGET_BUTTON_CLICKED)
        {
            if (event.GUIEvent.Caller == CloseButton)
            {
                setVisible(false);

                ox::event::SEvent closed;
                closed.EventType = ox::event::EET_GUI_EVENT;
                closed.GUIEvent.Caller = this;
                closed.GUIEvent.EventType = ox::gui::EGET_WINDOW_CLOSED;
                Parent->OnEvent(closed);
                return true;
            }
        }
        break;
    case ox::event::EET_MOUSE_INPUT_EVENT:
        switch (event.MouseInput.Event)
        {
        case ox::event::EMIE_LMOUSE_PRESSED_DOWN:
            DragStart.X = event.MouseInput.X;
            DragStart.Y = event.MouseInput.Y;
            if (!Environment->hasFocus(this) && !IsFrame)
            {
                // with a top edge animation, only the top edge drags the window
                if (!Animations[EWP_TOP] ||
                    AnimationRects[EWP_TOP].isPointInside(ox::core::CPosition2d<int>(
                        DragStart.X - AbsoluteRect.UpperLeftCorner.X, DragStart.Y - AbsoluteRect.UpperLeftCorner.Y)))
                {
                    Dragging = true;
                    Environment->setFocus(this);
                    if (Parent)
                        Parent->bringToFront(this);
                }
            }
            if (!IsFrame)
                return false;
            break;
        case ox::event::EMIE_LMOUSE_LEFT_UP:
            Dragging = false;
            Environment->removeFocus(this);
            if (!IsFrame)
                return false;
            break;
        case ox::event::EMIE_MOUSE_MOVED:
            if (Dragging)
            {
                move(ox::core::CPosition2d<int>(event.MouseInput.X - DragStart.X, event.MouseInput.Y - DragStart.Y));
                DragStart.X = event.MouseInput.X;
                DragStart.Y = event.MouseInput.Y;
                return true;
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return Parent ? Parent->OnEvent(event) : false;
}

//! draws the element and its children
void CGUIWindow::draw()
{
    if (!IsVisible)
        return;

    ox::gui::IGUISkin* skin = Environment->getSkin();
    ox::video::IVideoDriver* driver = Environment->getVideoDriver();

    ox::core::CRect<int> rect = AbsoluteRect;
    ox::core::CRect<int>* cl = &AbsoluteClippingRect;

    if (!Animations[EWP_BACKGROUND])
    {
        // draw body fast

        rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y + 1;
        driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_HIGH_LIGHT), rect, cl);

        rect.LowerRightCorner.Y = AbsoluteRect.LowerRightCorner.Y;
        rect.LowerRightCorner.X = rect.UpperLeftCorner.X + 1;
        driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_HIGH_LIGHT), rect, cl);

        rect.UpperLeftCorner.X = AbsoluteRect.LowerRightCorner.X - 1;
        rect.LowerRightCorner.X = AbsoluteRect.LowerRightCorner.X;
        rect.UpperLeftCorner.Y = AbsoluteRect.UpperLeftCorner.Y;
        rect.LowerRightCorner.Y = AbsoluteRect.LowerRightCorner.Y;
        driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_DARK_SHADOW), rect, cl);

        rect.UpperLeftCorner.X -= 1;
        rect.LowerRightCorner.X -= 1;
        rect.UpperLeftCorner.Y += 1;
        rect.LowerRightCorner.Y -= 1;
        driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_SHADOW), rect, cl);

        rect.UpperLeftCorner.X = AbsoluteRect.UpperLeftCorner.X;
        rect.UpperLeftCorner.Y = AbsoluteRect.LowerRightCorner.Y - 1;
        rect.LowerRightCorner.Y = AbsoluteRect.LowerRightCorner.Y;
        rect.LowerRightCorner.X = AbsoluteRect.LowerRightCorner.X;
        driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_DARK_SHADOW), rect, cl);

        rect.UpperLeftCorner.X += 1;
        rect.LowerRightCorner.X -= 1;
        rect.UpperLeftCorner.Y -= 1;
        rect.LowerRightCorner.Y -= 1;
        driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_SHADOW), rect, cl);

        rect = AbsoluteRect;
        rect.UpperLeftCorner.X += 1;
        rect.UpperLeftCorner.Y += 1;
        rect.LowerRightCorner.X -= 2;
        rect.LowerRightCorner.Y -= 2;
        driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_FACE), rect, cl);

        // draw title bar

        rect = AbsoluteRect;
        rect.UpperLeftCorner.X += 2;
        rect.UpperLeftCorner.Y += 2;
        rect.LowerRightCorner.X -= 2;
        rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y + skin->getSize(ox::gui::EGDS_WINDOW_BUTTON_WIDTH) + 2;
        driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_ACTIVE_BORDER), rect, cl);

        rect.UpperLeftCorner.X += 2;
        rect.LowerRightCorner.X -= skin->getSize(ox::gui::EGDS_WINDOW_BUTTON_WIDTH) + 5;
    }
    else
    {
        const ox::video::SColor white(0xffffffff);
        // corners
        Animations[EWP_TOP_LEFT]->draw(
            ox::core::CPosition2d<int>(rect.UpperLeftCorner.X + AnimationRects[EWP_TOP_LEFT].UpperLeftCorner.X,
                rect.UpperLeftCorner.Y + AnimationRects[EWP_TOP_LEFT].UpperLeftCorner.Y), cl, white);
        Animations[EWP_TOP_RIGHT]->draw(
            ox::core::CPosition2d<int>(rect.UpperLeftCorner.X + AnimationRects[EWP_TOP_RIGHT].UpperLeftCorner.X,
                rect.UpperLeftCorner.Y + AnimationRects[EWP_TOP_RIGHT].UpperLeftCorner.Y), cl, white);
        Animations[EWP_BOTTOM_LEFT]->draw(
            ox::core::CPosition2d<int>(rect.UpperLeftCorner.X + AnimationRects[EWP_BOTTOM_LEFT].UpperLeftCorner.X,
                rect.UpperLeftCorner.Y + AnimationRects[EWP_BOTTOM_LEFT].UpperLeftCorner.Y), cl, white);
        Animations[EWP_BOTTOM_RIGHT]->draw(
            ox::core::CPosition2d<int>(rect.UpperLeftCorner.X + AnimationRects[EWP_BOTTOM_RIGHT].UpperLeftCorner.X,
                rect.UpperLeftCorner.Y + AnimationRects[EWP_BOTTOM_RIGHT].UpperLeftCorner.Y), cl, white);

        // top and bottom edges
        ox::core::CRect<int> area;
        area = AnimationRects[EWP_TOP];
        area.UpperLeftCorner += rect.UpperLeftCorner;
        area.LowerRightCorner += rect.UpperLeftCorner;
        clipAgainst(area, *cl);
        int step = Animations[EWP_TOP]->getFrameSize(0).Width;
        for (int x = area.UpperLeftCorner.X; x < area.LowerRightCorner.X; x += step)
            Animations[EWP_TOP]->draw(ox::core::CPosition2d<int>(x, area.UpperLeftCorner.Y), &area, white);

        area = AnimationRects[EWP_BOTTOM];
        area.UpperLeftCorner += rect.UpperLeftCorner;
        area.LowerRightCorner += rect.UpperLeftCorner;
        clipAgainst(area, *cl);
        step = Animations[EWP_BOTTOM]->getFrameSize(0).Width;
        for (int x = area.UpperLeftCorner.X; x < area.LowerRightCorner.X; x += step)
            Animations[EWP_BOTTOM]->draw(ox::core::CPosition2d<int>(x, area.UpperLeftCorner.Y), &area, white);

        // left and right edges
        area = AnimationRects[EWP_LEFT];
        area.UpperLeftCorner += rect.UpperLeftCorner;
        area.LowerRightCorner += rect.UpperLeftCorner;
        clipAgainst(area, *cl);
        step = Animations[EWP_LEFT]->getFrameSize(0).Height;
        for (int y = area.UpperLeftCorner.Y; y < area.LowerRightCorner.Y; y += step)
            Animations[EWP_LEFT]->draw(ox::core::CPosition2d<int>(area.UpperLeftCorner.X, y), &area, white);

        area = AnimationRects[EWP_RIGHT];
        area.UpperLeftCorner += rect.UpperLeftCorner;
        area.LowerRightCorner += rect.UpperLeftCorner;
        clipAgainst(area, *cl);
        step = Animations[EWP_RIGHT]->getFrameSize(0).Height;
        for (int y = area.UpperLeftCorner.Y; y < area.LowerRightCorner.Y; y += step)
            Animations[EWP_RIGHT]->draw(ox::core::CPosition2d<int>(area.UpperLeftCorner.X, y), &area, white);

        // background
        area = AnimationRects[EWP_BACKGROUND];
        area.UpperLeftCorner += rect.UpperLeftCorner;
        area.LowerRightCorner += rect.UpperLeftCorner;
        clipAgainst(area, *cl);
        int stepX = Animations[EWP_BACKGROUND]->getFrameSize(0).Width;
        int stepY = Animations[EWP_BACKGROUND]->getFrameSize(0).Height;
        for (int y = area.UpperLeftCorner.Y; y < area.LowerRightCorner.Y; y += stepY)
            for (int x = area.UpperLeftCorner.X; x < area.LowerRightCorner.X; x += stepX)
                Animations[EWP_BACKGROUND]->draw(ox::core::CPosition2d<int>(x, y), &area, white);

        // inner edges over the background
        if (InnerAnimations[EWIP_TOP])
        {
            int width = InnerAnimationRects[EWIP_TOP].getWidth();
            for (int x = area.UpperLeftCorner.X; x < area.LowerRightCorner.X; x += width)
                InnerAnimations[EWIP_TOP]->draw(ox::core::CPosition2d<int>(x, area.UpperLeftCorner.Y), &area, white);
        }

        if (InnerAnimations[EWIP_BOTTOM])
        {
            int width = InnerAnimationRects[EWIP_BOTTOM].getWidth();
            int height = InnerAnimationRects[EWIP_BOTTOM].getHeight();
            for (int x = area.UpperLeftCorner.X; x < area.LowerRightCorner.X; x += width)
                InnerAnimations[EWIP_BOTTOM]->draw(ox::core::CPosition2d<int>(x, area.LowerRightCorner.Y - height),
                    &area, white);
        }

        if (InnerAnimations[EWIP_LEFT])
        {
            int height = InnerAnimationRects[EWIP_LEFT].getHeight();
            for (int y = area.UpperLeftCorner.Y; y < area.LowerRightCorner.Y; y += height)
                InnerAnimations[EWIP_LEFT]->draw(ox::core::CPosition2d<int>(area.UpperLeftCorner.X, y), &area, white);
        }

        if (InnerAnimations[EWIP_RIGHT])
        {
            int width = InnerAnimationRects[EWIP_RIGHT].getWidth();
            int height = InnerAnimationRects[EWIP_RIGHT].getHeight();
            for (int y = area.UpperLeftCorner.Y; y < area.LowerRightCorner.Y; y += height)
                InnerAnimations[EWIP_RIGHT]->draw(ox::core::CPosition2d<int>(area.LowerRightCorner.X - width, y),
                    &area, white);
        }

        // the caption goes between the top corners
        rect.UpperLeftCorner.X += AnimationRects[EWP_TOP_LEFT].getWidth() + 2;
        rect.UpperLeftCorner.Y += 2;
        rect.LowerRightCorner.X -= AnimationRects[EWP_TOP_RIGHT].getWidth() + 2;
        rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y + AnimationRects[EWP_TOP].getHeight() - 2;
    }

    if (Text.size() && !IsFrame)
    {
        ox::gui::IGUIFont* font = skin->getFont();
        if (font)
            font->draw(Text.c_str(), rect, skin->getColor(ox::gui::EGDC_ACTIVE_CAPTION), ox::gui::EFHA_LEFT,
                ox::gui::EFVA_CENTER, cl);
    }

    IGUIElement::draw();
}

//! Returns pointer to the close button
ox::gui::IGUIButton* CGUIWindow::getCloseButton()
{
    return CloseButton;
}

//! Returns pointer to the minimize button
ox::gui::IGUIButton* CGUIWindow::getMinimizeButton()
{
    return MinButton;
}

//! Returns pointer to the maximize button
ox::gui::IGUIButton* CGUIWindow::getMaximizeButton()
{
    return RestoreButton;
}

void CGUIWindow::setAnimations(ox::video::ISpritePackage* package, const char* animation)
{
    const char* WINDOW_ANIMATION_NAMES[EWP_COUNT] = {
        "Background", "TopLeft", "TopRight", "BottomLeft", "BottomRight", "Top", "Left", "Right", "Bottom"
    };
    const char* INNER_ANIMATION_NAMES[EWIP_COUNT] = { "TopInner", "LeftInner", "RightInner", "BottomInner" };

    for (int i = 0; i < EWP_COUNT; ++i)
    {
        if (Animations[i])
        {
            Animations[i]->remove();
            Animations[i] = 0;
        }

        ox::core::CString<char> name(animation);
        name.append(ox::core::CString<char>(WINDOW_ANIMATION_NAMES[i]));
        Animations[i] = package->addNewAnimationState(name.c_str());
        if (Animations[i])
        {
            ox::core::CDimension2d<int> size = Animations[i]->getFrameSize(0);
            AnimationRects[i] = ox::core::CRect<int>(0, 0, size.Width, size.Height);
        }
    }

    // grow the window by the corners so that the content area keeps its size
    RelativeRect.UpperLeftCorner.X -= AnimationRects[EWP_TOP_LEFT].getWidth();
    RelativeRect.UpperLeftCorner.Y -= AnimationRects[EWP_TOP_LEFT].getHeight();
    RelativeRect.LowerRightCorner.X += AnimationRects[EWP_BOTTOM_RIGHT].getWidth();
    RelativeRect.LowerRightCorner.Y += AnimationRects[EWP_BOTTOM_RIGHT].getHeight();
    setRelativePosition(RelativeRect);

    for (int i = 0; i < EWIP_COUNT; ++i)
    {
        if (InnerAnimations[i])
        {
            InnerAnimations[i]->remove();
            InnerAnimations[i] = 0;
        }

        ox::core::CString<char> name(animation);
        name.append(ox::core::CString<char>(INNER_ANIMATION_NAMES[i]));
        InnerAnimations[i] = package->addNewAnimationState(name.c_str());
        if (InnerAnimations[i])
        {
            ox::core::CDimension2d<int> size = InnerAnimations[i]->getFrameSize(0);
            InnerAnimationRects[i] = ox::core::CRect<int>(0, 0, size.Width, size.Height);
        }
    }
}

ox::core::CRect<int> CGUIWindow::getContentArea()
{
    return AnimationRects[EWP_BACKGROUND];
}

//! The ID of the lock option in the options menu.
static const int LOCK_MENU_ID = -387;

//! How close, in pixels, a dragged frame snaps to a layout group's edge.
static const int SNAP_DISTANCE = 14;

//! Whether an edge offset is within the snapping distance.
static bool snaps(int offset)
{
    if (offset < -SNAP_DISTANCE)
        return false;
    if (offset > SNAP_DISTANCE)
        return false;
    return true;
}

CGUIDetachableFrame::CGUIDetachableFrame(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
    ox::core::CRect<int> rectangle, unsigned int fadeDelay, bool hasMenu, bool hideable)
    : IGUIDetachableFrame(environment, parent, id, rectangle), Dragging(false), Resizing(false), Resizable(false),
      LastHoverTime(0), FadeDelay(fadeDelay), FadedOut(true), Locked(false), Lockable(true), Fadeable(true),
      Hideable(hideable), MenuVisible(true), MenuButton(0), CloseButton(0), PopupMenu(0)
{
    for (int i = 0; i < EWP_COUNT; ++i)
        Animations[i] = 0;
    for (int i = 0; i < EWIP_COUNT; ++i)
        InnerAnimations[i] = 0;
    SizeDragHandle = 0;

    if (Environment && Environment->getSkin() && Environment->getSkin()->getSpritePackage())
        setAnimations(Environment->getSkin()->getSpritePackage(), "Frame");

    if (hasMenu)
    {
        MenuButton = Environment->addButton(ox::core::CRect<int>(0, 0, 100, 20), this, -1, L"");
        MenuButton->setAnimations(Environment->getSkin()->getSpritePackage(), "PearlButton", true);
        MenuButton->setFixed(true);
    }

    if (hideable)
    {
        CloseButton = Environment->addButton(ox::core::CRect<int>(0, 0, 100, 20), this, -1, L"");
        CloseButton->setAnimations(Environment->getSkin()->getSpritePackage(), "CloseBtn", true);
        CloseButton->setFixed(true);
    }

    updateButtonPositions();

    PopupMenuTitleFont = Environment->getSkin()->getFont();
    PopupMenuItemsFont = Environment->getSkin()->getFont();
}

void CGUIDetachableFrame::updateButtonPositions()
{
    if (MenuButton)
        MenuButton->moveTo(AnimationRects[EWP_TOP_LEFT].UpperLeftCorner);

    if (CloseButton)
    {
        ox::core::CRect<int> buttonRect = CloseButton->getRelativePosition();
        CloseButton->moveTo(ox::core::CPosition2d<int>(buttonRect.UpperLeftCorner.X -
            buttonRect.LowerRightCorner.X + AnimationRects[EWP_TOP_RIGHT].LowerRightCorner.X,
            AnimationRects[EWP_TOP_RIGHT].UpperLeftCorner.Y));
    }
}

CGUIDetachableFrame::~CGUIDetachableFrame()
{
    for (int i = 0; i < EWP_COUNT; ++i)
        if (Animations[i])
            Animations[i]->remove();

    for (int i = 0; i < EWIP_COUNT; ++i)
        if (InnerAnimations[i])
            InnerAnimations[i]->remove();

    if (SizeDragHandle)
        SizeDragHandle->remove();
}

void CGUIDetachableFrame::setPopupMenuTitleFont(ox::gui::IGUIFont* font)
{
    PopupMenuTitleFont = font;
}

void CGUIDetachableFrame::setPopupMenuItemsFont(ox::gui::IGUIFont* font)
{
    PopupMenuItemsFont = font;
}

void CGUIDetachableFrame::setFadeable(bool fadeable)
{
    Fadeable = fadeable;
    if (!fadeable && !Locked)
        FadedOut = false;
}

void CGUIDetachableFrame::setLockable(bool lockable)
{
    Lockable = lockable;
    Locked = lockable;
    if (!Fadeable && !lockable)
        FadedOut = false;
}

void CGUIDetachableFrame::setHideable(bool hideable)
{
    if (CloseButton)
        CloseButton->setVisible(hideable);
    Hideable = hideable;
}

void CGUIDetachableFrame::setLocked(bool locked)
{
    if (Lockable)
        Locked = locked;
}

void CGUIDetachableFrame::setRelativePosition(const ox::core::CRect<int>& position)
{
    IGUIElement::setRelativePosition(position);
    updateAnimationRects();
    updateButtonPositions();
}

void CGUIDetachableFrame::updateAnimationRects()
{
    int width = RelativeRect.getWidth();
    int height = RelativeRect.getHeight();

    AnimationRects[EWP_TOP_RIGHT].UpperLeftCorner.X = width - AnimationRects[EWP_TOP_RIGHT].getWidth();
    AnimationRects[EWP_TOP_RIGHT].LowerRightCorner.X = width;

    AnimationRects[EWP_TOP].UpperLeftCorner.X = AnimationRects[EWP_TOP_LEFT].getWidth();
    AnimationRects[EWP_TOP].LowerRightCorner.X = AnimationRects[EWP_TOP_RIGHT].UpperLeftCorner.X;

    AnimationRects[EWP_BOTTOM_LEFT].UpperLeftCorner.Y = height - AnimationRects[EWP_BOTTOM_LEFT].getHeight();
    AnimationRects[EWP_BOTTOM_LEFT].LowerRightCorner.Y = height;

    int bottomRightWidth = AnimationRects[EWP_BOTTOM_RIGHT].getWidth();
    AnimationRects[EWP_BOTTOM_RIGHT].UpperLeftCorner.X = width - bottomRightWidth;
    AnimationRects[EWP_BOTTOM_RIGHT].UpperLeftCorner.Y = AnimationRects[EWP_BOTTOM_LEFT].UpperLeftCorner.Y;
    AnimationRects[EWP_BOTTOM_RIGHT].LowerRightCorner.X = width;
    AnimationRects[EWP_BOTTOM_RIGHT].LowerRightCorner.Y = height;

    // the bottom edge starts at the bottom right corner's width
    AnimationRects[EWP_BOTTOM] = ox::core::CRect<int>(bottomRightWidth,
        AnimationRects[EWP_BOTTOM_LEFT].UpperLeftCorner.Y, AnimationRects[EWP_BOTTOM_RIGHT].UpperLeftCorner.X,
        height);

    int sideWidth = AnimationRects[EWP_LEFT].getWidth();
    int top = AnimationRects[EWP_TOP_RIGHT].getHeight();
    AnimationRects[EWP_LEFT] =
        ox::core::CRect<int>(0, top, sideWidth, AnimationRects[EWP_BOTTOM_LEFT].UpperLeftCorner.Y);

    // the right edge is as wide as the left one
    AnimationRects[EWP_RIGHT] = ox::core::CRect<int>(width - sideWidth, top, width,
        AnimationRects[EWP_BOTTOM_LEFT].UpperLeftCorner.Y);

    AnimationRects[EWP_BACKGROUND] = ox::core::CRect<int>(sideWidth, top, width - sideWidth,
        AnimationRects[EWP_BOTTOM_LEFT].UpperLeftCorner.Y);

    ox::core::CDimension2d<int> handleSize(SizeDragHandleRect.getWidth(), SizeDragHandleRect.getHeight());
    SizeDragHandleRect = ox::core::CRect<int>(
        ox::core::CPosition2d<int>(width - handleSize.Width, height - handleSize.Height), handleSize);
}

void CGUIDetachableFrame::setResizable(bool resizable, const ox::core::CDimension2d<int>& minimumSize)
{
    Resizable = resizable;
    MinimumSize = minimumSize;
}

void CGUIDetachableFrame::addMenuOption(const wchar_t* text, int id)
{
    SFrameMenuOption option;
    option.Text = text;
    option.ID = id;
    MenuOptions.push_back(option);
}

void CGUIDetachableFrame::setMenuVisible(bool visible)
{
    if (MenuButton)
        MenuButton->setVisible(visible);
    MenuVisible = visible;
}

bool CGUIDetachableFrame::OnEvent(const ox::event::SEvent& event)
{
    if (EventReceiver && !Dragging && !Resizing && EventReceiver->OnEvent(event))
        return true;

    switch (event.EventType)
    {
    case ox::event::EET_GUI_EVENT:
        if (event.GUIEvent.EventType == ox::gui::EGET_ELEMENT_FOCUS_LOST)
        {
            Dragging = false;
            Resizing = false;
            return true;
        }
        else if (event.GUIEvent.EventType == ox::gui::EGET_BUTTON_CLICKED)
        {
            if (event.GUIEvent.Caller == MenuButton && MenuButton)
            {
                PopupMenu = Environment->addPopupMenu(MenuButton->getAbsolutePosition().LowerRightCorner, 100,
                    L"Menu Options", PopupMenuTitleFont, PopupMenuItemsFont, 0, -1);
                if (Lockable)
                    PopupMenu->addMenuOption(Locked ? L"Unlock Window" : L"Lock Window", LOCK_MENU_ID);
                for (unsigned int i = 0; i < MenuOptions.size(); ++i)
                    PopupMenu->addMenuOption(MenuOptions[i].Text.c_str(), MenuOptions[i].ID);
                PopupMenu->setSelectionParent(this);
                return true;
            }
            else if (event.GUIEvent.Caller == CloseButton && CloseButton && Hideable)
            {
                setVisible(false);

                ox::event::SEvent closed;
                closed.EventType = ox::event::EET_GUI_EVENT;
                closed.GUIEvent.Caller = this;
                closed.GUIEvent.EventType = ox::gui::EGET_WINDOW_CLOSED;
                Parent->OnEvent(closed);
                return true;
            }
        }
        else if (event.GUIEvent.EventType == ox::gui::EGET_POPUP_MENU_OPTION_CHOSEN)
        {
            if (event.GUIEvent.Caller->getID() == LOCK_MENU_ID)
            {
                Locked = !Locked;
                return true;
            }
        }
        break;
    case ox::event::EET_MOUSE_INPUT_EVENT:
        switch (event.MouseInput.Event)
        {
        case ox::event::EMIE_LMOUSE_PRESSED_DOWN:
            if (Locked)
                break;

            DragStart.X = event.MouseInput.X;
            DragStart.Y = event.MouseInput.Y;
            DragStartPosition = AbsoluteRect.UpperLeftCorner;
            if (!Environment->hasFocus(this))
            {
                if (SizeDragHandle && Resizable &&
                    SizeDragHandleRect.isPointInside(ox::core::CPosition2d<int>(
                        DragStart.X - AbsoluteRect.UpperLeftCorner.X, DragStart.Y - AbsoluteRect.UpperLeftCorner.Y)))
                {
                    FadedOut = false;
                    Resizing = true;
                    Dragging = false;
                }
                else
                {
                    FadedOut = false;
                    Dragging = true;
                }

                Environment->setFocus(this);
                if (Parent)
                    Parent->bringToFront(this);
            }
            else if (FadedOut)
                break;
            return true;
        case ox::event::EMIE_LMOUSE_LEFT_UP:
        {
            bool handled = false;
            if (event.MouseInput.Clicks > 1 && Lockable)
            {
                Locked = true;
                handled = true;
            }

            if (!FadedOut && !Locked)
                handled = true;

            if (Dragging || Resizing)
            {
                Dragging = false;
                Resizing = false;
                handled = true;
            }

            Environment->removeFocus(this);
            if (handled)
                return true;
            break;
        }
        case ox::event::EMIE_MOUSE_MOVED:
            LastHoverTime = os::Timer::getTime();
            if (MenuButton && MenuVisible)
                MenuButton->setVisible(true);
            if (CloseButton && Hideable)
                CloseButton->setVisible(true);

            if (Dragging && !Locked)
            {
                moveTo(DragStartPosition);

                int dy = event.MouseInput.Y - DragStart.Y;
                int dx = event.MouseInput.X - DragStart.X;
                bool snappedX = false;
                bool snappedY = false;
                if (Parent)
                {
                    const std::list<ox::gui::IGUIElement*>& children = Parent->getChildren();
                    std::list<ox::gui::IGUIElement*>::const_iterator it = children.begin();
                    ox::core::CRect<int> rect(RelativeRect.UpperLeftCorner.X + dx, RelativeRect.UpperLeftCorner.Y + dy,
                        RelativeRect.LowerRightCorner.X + dx, RelativeRect.LowerRightCorner.Y + dy);

                    lockToItemInside(Parent, rect, snappedX, snappedY);
                    for (; it != children.end(); ++it)
                        snapToItem(*it, rect, snappedX, snappedY);

                    if (snappedX || snappedY)
                        setRelativePosition(rect);
                }

                if (!snappedX && !snappedY)
                    move(ox::core::CPosition2d<int>(dx, dy));
                return true;
            }
            else if (Resizing && !Locked)
            {
                ox::core::CRect<int> rect(RelativeRect.UpperLeftCorner.X, RelativeRect.UpperLeftCorner.Y,
                    RelativeRect.LowerRightCorner.X + event.MouseInput.X - DragStart.X,
                    RelativeRect.LowerRightCorner.Y + event.MouseInput.Y - DragStart.Y);
                if (rect.getWidth() < MinimumSize.Width)
                    rect.LowerRightCorner.X = rect.UpperLeftCorner.X + MinimumSize.Width;
                if (rect.getHeight() < MinimumSize.Height)
                    rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y + MinimumSize.Height;
                setRelativePosition(rect);

                DragStart.X = event.MouseInput.X;
                DragStart.Y = event.MouseInput.Y;

                ox::event::SEvent resized;
                resized.EventType = ox::event::EET_GUI_EVENT;
                resized.GUIEvent.Caller = this;
                resized.GUIEvent.EventType = ox::gui::EGET_ELEMENT_RESIZED;
                Parent->OnEvent(resized);
                return true;
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return Parent ? Parent->OnEvent(event) : false;
}

void CGUIDetachableFrame::lockToItemInside(ox::gui::IGUIElement* item, ox::core::CRect<int>& rect, bool& lockedX,
    bool& lockedY)
{
    if (item->getType() != ox::gui::EGUIET_LAYOUT || item == this || !item->isVisible())
        return;

    ox::core::CRect<int> area = item->getRelativePosition();
    int offset = rect.UpperLeftCorner.X - area.UpperLeftCorner.X;

    if (!lockedX)
    {
        if (offset < 0)
        {
            rect.UpperLeftCorner.X -= offset;
            rect.LowerRightCorner.X -= offset;
            lockedX = true;
        }
        else
        {
            offset = rect.LowerRightCorner.X - area.LowerRightCorner.X;
            if (offset > 0)
            {
                rect.UpperLeftCorner.X -= offset;
                rect.LowerRightCorner.X -= offset;
                lockedX = true;
            }
        }
    }

    if (!lockedY)
    {
        offset = rect.UpperLeftCorner.Y - area.UpperLeftCorner.Y;
        if (offset < 0)
        {
            rect.UpperLeftCorner.Y -= offset;
            rect.LowerRightCorner.Y -= offset;
            lockedY = true;
        }
        else
        {
            offset = rect.LowerRightCorner.Y - area.LowerRightCorner.Y;
            if (offset > 0)
            {
                rect.UpperLeftCorner.Y -= offset;
                rect.LowerRightCorner.Y -= offset;
                lockedY = true;
            }
        }
    }
}

void CGUIDetachableFrame::snapToItem(ox::gui::IGUIElement* item, ox::core::CRect<int>& rect, bool& snappedX,
    bool& snappedY)
{
    if (item->getType() != ox::gui::EGUIET_LAYOUT || item == this || !item->isVisible())
        return;

    ox::core::CRect<int> area = item->getRelativePosition();
    bool overlapX = rect.UpperLeftCorner.X < area.LowerRightCorner.X &&
        rect.LowerRightCorner.X > area.UpperLeftCorner.X;
    bool overlapY = rect.UpperLeftCorner.Y < area.LowerRightCorner.Y &&
        rect.LowerRightCorner.Y > area.UpperLeftCorner.Y;

    if (!snappedX && overlapY)
    {
        int offset = rect.UpperLeftCorner.X - area.LowerRightCorner.X;
        if (snaps(offset))
        {
            rect.UpperLeftCorner.X -= offset;
            rect.LowerRightCorner.X -= offset;
            snappedX = true;
        }
        else
        {
            offset = rect.LowerRightCorner.X - area.UpperLeftCorner.X;
            if (snaps(offset))
            {
                rect.UpperLeftCorner.X -= offset;
                rect.LowerRightCorner.X -= offset;
                snappedX = true;
            }
        }
    }

    if (!snappedY && overlapX)
    {
        int offset = rect.UpperLeftCorner.Y - area.LowerRightCorner.Y;
        if (snaps(offset))
        {
            rect.UpperLeftCorner.Y -= offset;
            rect.LowerRightCorner.Y -= offset;
            snappedY = true;
        }
        else
        {
            offset = rect.LowerRightCorner.Y - area.UpperLeftCorner.Y;
            if (snaps(offset))
            {
                rect.UpperLeftCorner.Y -= offset;
                rect.LowerRightCorner.Y -= offset;
                snappedY = true;
            }
        }
    }
}

void CGUIDetachableFrame::draw()
{
    if (!IsVisible)
        return;

    ox::gui::IGUISkin* skin = Environment->getSkin();
    Environment->getVideoDriver();

    ox::core::CRect<int> rect = AbsoluteRect;
    unsigned int now = os::Timer::getTime();
    ox::video::SColor color(0xffffffff);

    // fade out when the mouse has been away for the fade delay
    unsigned int elapsed = now - LastHoverTime;
    if (Fadeable && LastHoverTime && elapsed >= FadeDelay &&
        !AbsoluteRect.isPointInside(Environment->getMousePosition()))
    {
        if (MenuButton)
            MenuButton->setVisible(false);
        if (CloseButton)
            CloseButton->setVisible(false);

        if (elapsed >= FadeDelay * 2)
        {
            FadedOut = true;
            IGUIElement::draw();
            return;
        }

        color = ox::video::SColor((int)((float)(FadeDelay * 2 - elapsed) * 255.0f / (float)FadeDelay), 255, 255,
            255);
    }

    if ((!LastHoverTime || Locked) && Fadeable)
    {
        FadedOut = true;
        IGUIElement::draw();
        return;
    }

    FadedOut = false;
    ox::core::CRect<int>* cl = &AbsoluteClippingRect;

    if (Animations[EWP_BACKGROUND])
    {
        // corners
        Animations[EWP_TOP_LEFT]->draw(
            ox::core::CPosition2d<int>(rect.UpperLeftCorner.X + AnimationRects[EWP_TOP_LEFT].UpperLeftCorner.X,
                rect.UpperLeftCorner.Y + AnimationRects[EWP_TOP_LEFT].UpperLeftCorner.Y), cl, color);
        Animations[EWP_TOP_RIGHT]->draw(
            ox::core::CPosition2d<int>(rect.UpperLeftCorner.X + AnimationRects[EWP_TOP_RIGHT].UpperLeftCorner.X,
                rect.UpperLeftCorner.Y + AnimationRects[EWP_TOP_RIGHT].UpperLeftCorner.Y), cl, color);
        Animations[EWP_BOTTOM_LEFT]->draw(
            ox::core::CPosition2d<int>(rect.UpperLeftCorner.X + AnimationRects[EWP_BOTTOM_LEFT].UpperLeftCorner.X,
                rect.UpperLeftCorner.Y + AnimationRects[EWP_BOTTOM_LEFT].UpperLeftCorner.Y), cl, color);
        Animations[EWP_BOTTOM_RIGHT]->draw(
            ox::core::CPosition2d<int>(rect.UpperLeftCorner.X + AnimationRects[EWP_BOTTOM_RIGHT].UpperLeftCorner.X,
                rect.UpperLeftCorner.Y + AnimationRects[EWP_BOTTOM_RIGHT].UpperLeftCorner.Y), cl, color);

        // top and bottom edges
        ox::core::CRect<int> area;
        area = AnimationRects[EWP_TOP];
        area.UpperLeftCorner += rect.UpperLeftCorner;
        area.LowerRightCorner += rect.UpperLeftCorner;
        clipAgainst(area, *cl);
        int step = Animations[EWP_TOP]->getFrameSize(0).Width;
        for (int x = area.UpperLeftCorner.X; x < area.LowerRightCorner.X; x += step)
            Animations[EWP_TOP]->draw(ox::core::CPosition2d<int>(x, area.UpperLeftCorner.Y), &area, color);

        area = AnimationRects[EWP_BOTTOM];
        area.UpperLeftCorner += rect.UpperLeftCorner;
        area.LowerRightCorner += rect.UpperLeftCorner;
        clipAgainst(area, *cl);
        step = Animations[EWP_BOTTOM]->getFrameSize(0).Width;
        for (int x = area.UpperLeftCorner.X; x < area.LowerRightCorner.X; x += step)
            Animations[EWP_BOTTOM]->draw(ox::core::CPosition2d<int>(x, area.UpperLeftCorner.Y), &area, color);

        // left and right edges
        area = AnimationRects[EWP_LEFT];
        area.UpperLeftCorner += rect.UpperLeftCorner;
        area.LowerRightCorner += rect.UpperLeftCorner;
        clipAgainst(area, *cl);
        step = Animations[EWP_LEFT]->getFrameSize(0).Height;
        for (int y = area.UpperLeftCorner.Y; y < area.LowerRightCorner.Y; y += step)
            Animations[EWP_LEFT]->draw(ox::core::CPosition2d<int>(area.UpperLeftCorner.X, y), &area, color);

        area = AnimationRects[EWP_RIGHT];
        area.UpperLeftCorner += rect.UpperLeftCorner;
        area.LowerRightCorner += rect.UpperLeftCorner;
        clipAgainst(area, *cl);
        step = Animations[EWP_RIGHT]->getFrameSize(0).Height;
        for (int y = area.UpperLeftCorner.Y; y < area.LowerRightCorner.Y; y += step)
            Animations[EWP_RIGHT]->draw(ox::core::CPosition2d<int>(area.UpperLeftCorner.X, y), &area, color);

        // background
        area = AnimationRects[EWP_BACKGROUND];
        area.UpperLeftCorner += rect.UpperLeftCorner;
        area.LowerRightCorner += rect.UpperLeftCorner;
        clipAgainst(area, *cl);
        int stepX = Animations[EWP_BACKGROUND]->getFrameSize(0).Width;
        int stepY = Animations[EWP_BACKGROUND]->getFrameSize(0).Height;
        for (int y = area.UpperLeftCorner.Y; y < area.LowerRightCorner.Y; y += stepY)
            for (int x = area.UpperLeftCorner.X; x < area.LowerRightCorner.X; x += stepX)
                Animations[EWP_BACKGROUND]->draw(ox::core::CPosition2d<int>(x, y), &area, color);

        // inner edges over the background
        if (InnerAnimations[EWIP_TOP])
        {
            int width = InnerAnimationRects[EWIP_TOP].getWidth();
            for (int x = area.UpperLeftCorner.X; x < area.LowerRightCorner.X; x += width)
                InnerAnimations[EWIP_TOP]->draw(ox::core::CPosition2d<int>(x, area.UpperLeftCorner.Y), &area, color);
        }

        if (InnerAnimations[EWIP_BOTTOM])
        {
            int width = InnerAnimationRects[EWIP_BOTTOM].getWidth();
            int height = InnerAnimationRects[EWIP_BOTTOM].getHeight();
            for (int x = area.UpperLeftCorner.X; x < area.LowerRightCorner.X; x += width)
                InnerAnimations[EWIP_BOTTOM]->draw(ox::core::CPosition2d<int>(x, area.LowerRightCorner.Y - height),
                    &area, color);
        }

        if (InnerAnimations[EWIP_LEFT])
        {
            int height = InnerAnimationRects[EWIP_LEFT].getHeight();
            for (int y = area.UpperLeftCorner.Y; y < area.LowerRightCorner.Y; y += height)
                InnerAnimations[EWIP_LEFT]->draw(ox::core::CPosition2d<int>(area.UpperLeftCorner.X, y), &area, color);
        }

        if (InnerAnimations[EWIP_RIGHT])
        {
            int width = InnerAnimationRects[EWIP_RIGHT].getWidth();
            int height = InnerAnimationRects[EWIP_RIGHT].getHeight();
            for (int y = area.UpperLeftCorner.Y; y < area.LowerRightCorner.Y; y += height)
                InnerAnimations[EWIP_RIGHT]->draw(ox::core::CPosition2d<int>(area.LowerRightCorner.X - width, y),
                    &area, color);
        }

        if (SizeDragHandle && Resizable)
            SizeDragHandle->draw(
                ox::core::CPosition2d<int>(rect.UpperLeftCorner.X + SizeDragHandleRect.UpperLeftCorner.X,
                    rect.UpperLeftCorner.Y + SizeDragHandleRect.UpperLeftCorner.Y), cl, color);

        // the caption goes between the top corners
        rect.UpperLeftCorner.X += AnimationRects[EWP_TOP_LEFT].getWidth() + 2;
        rect.UpperLeftCorner.Y += 2;
        rect.LowerRightCorner.X -= AnimationRects[EWP_TOP_RIGHT].getWidth() + 2;
        rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y + AnimationRects[EWP_TOP].getHeight() - 2;
    }

    if (Text.size())
    {
        ox::gui::IGUIFont* font = skin->getFont();
        if (font)
        {
            ox::video::SColor textColor = skin->getColor(ox::gui::EGDC_ACTIVE_CAPTION);
            textColor.setAlpha(color.getAlpha());
            font->draw(Text.c_str(), rect, textColor, ox::gui::EFHA_LEFT, ox::gui::EFVA_CENTER, cl);
        }
    }

    IGUIElement::draw();
}

void CGUIDetachableFrame::setAnimations(ox::video::ISpritePackage* package, const char* animation)
{
    const char* WINDOW_ANIMATION_NAMES[EWP_COUNT] = {
        "Background", "TopLeft", "TopRight", "BottomLeft", "BottomRight", "Top", "Left", "Right", "Bottom"
    };
    const char* INNER_ANIMATION_NAMES[EWIP_COUNT] = { "TopInner", "LeftInner", "RightInner", "BottomInner" };

    for (int i = 0; i < EWP_COUNT; ++i)
    {
        if (Animations[i])
        {
            Animations[i]->remove();
            Animations[i] = 0;
        }

        ox::core::CString<char> name(animation);
        name.append(ox::core::CString<char>(WINDOW_ANIMATION_NAMES[i]));
        Animations[i] = package->addNewAnimationState(name.c_str());
        if (Animations[i])
        {
            ox::core::CDimension2d<int> size = Animations[i]->getFrameSize(0);
            AnimationRects[i] = ox::core::CRect<int>(0, 0, size.Width, size.Height);
        }
    }

    for (int i = 0; i < EWIP_COUNT; ++i)
    {
        if (InnerAnimations[i])
        {
            InnerAnimations[i]->remove();
            InnerAnimations[i] = 0;
        }

        ox::core::CString<char> name(animation);
        name.append(ox::core::CString<char>(INNER_ANIMATION_NAMES[i]));
        InnerAnimations[i] = package->addNewAnimationState(name.c_str());
        if (InnerAnimations[i])
        {
            ox::core::CDimension2d<int> size = InnerAnimations[i]->getFrameSize(0);
            InnerAnimationRects[i] = ox::core::CRect<int>(0, 0, size.Width, size.Height);
        }
    }

    if (SizeDragHandle)
    {
        SizeDragHandle->remove();
        SizeDragHandle = 0;
    }

    {
        ox::core::CString<char> name(animation);
        name.append(ox::core::CString<char>("SizeDragHandle"));
        SizeDragHandle = package->addNewAnimationState(name.c_str());
        if (SizeDragHandle)
        {
            ox::core::CDimension2d<int> size = SizeDragHandle->getFrameSize(0);
            SizeDragHandleRect = ox::core::CRect<int>(0, 0, size.Width, size.Height);
        }
    }

    // grow the frame by the corners so that the content area keeps its size
    RelativeRect.UpperLeftCorner.X -= AnimationRects[EWP_TOP_LEFT].getWidth();
    RelativeRect.UpperLeftCorner.Y -= AnimationRects[EWP_TOP_LEFT].getHeight();
    RelativeRect.LowerRightCorner.X += AnimationRects[EWP_BOTTOM_RIGHT].getWidth();
    RelativeRect.LowerRightCorner.Y += AnimationRects[EWP_BOTTOM_RIGHT].getHeight();
    setRelativePosition(RelativeRect);
}

ox::core::CRect<int> CGUIDetachableFrame::getContentArea()
{
    return AnimationRects[EWP_BACKGROUND];
}

void CGUIDetachableFrame::stopDragging()
{
    Dragging = false;
}

} // end namespace gui
} // end namespace daisy
