// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CAlienEntity.h"
#include "CPriorityScreen.h"
#include "harvest/ECustomEvents.h"
#include "harvest/game/CThreatLevel.h"
#include "harvest/settings/CAlienPriorities.h"
#include "harvest/settings/CHarvestProfile.h"
#include "harvest/settings/CProfileManager.h"
#include "harvest/settings/CSystemConfig.h"
#include "ox/IOxDevice.h"
#include "ox/gui/ICursorControl.h"
#include "ox/gui/IGUIButton.h"
#include "ox/gui/IGUICheckBox.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIFont.h"
#include "ox/gui/IGUILayout.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/IVideoDriver.h"

namespace harvest {
namespace settings {

//! Localization keys of the five weapon types the priorities are for.
static const wchar_t* const PRIORITY_KEY_NAMES[] =
{
    L"prio:defense",
    L"prio:defenseLinked",
    L"prio:missile",
    L"prio:eagle",
    L"prio:tempest"
};

} // end namespace settings

namespace gui {

//! A color per alien type; not used by the priority screen.
static const ox::video::SColor ALIEN_COLORS[] =
{
    ox::video::SColor(0xffffff9d),
    ox::video::SColor(0xfffae7fb),
    ox::video::SColor(0xffce4d37),
    ox::video::SColor(0xff3a362d),
    ox::video::SColor(0xff957d41),
    ox::video::SColor(0xffb7b353),
    ox::video::SColor(0xffff92bb),
    ox::video::SColor(0xffffffff),
    ox::video::SColor(0xff7d6b39),
    ox::video::SColor(0xff),
    ox::video::SColor(0xff),
    ox::video::SColor(0xff),
    ox::video::SColor(0xff),
    ox::video::SColor(0xff)
};

//! The game command that toggles the priority screen.
static const int COMMAND_PRIORITY_SCREEN = 20;

//! A rectangle given by its upper left corner and its size, initialized in that order like Irrlicht's
//! rect(position, dimension) constructor; CRect lacks it because adding it changes other units' code.
struct SSizedRect : public ox::core::CRect<int>
{
    SSizedRect(const ox::core::CPosition2d<int>& position, const ox::core::CDimension2d<int>& size)
    {
        UpperLeftCorner = position;
        LowerRightCorner = ox::core::CPosition2d<int>(position.X + size.Width, position.Y + size.Height);
    }
};

static const char* const INGAME_SPRITES = "$GAME_RESOURCES$/harvestClientData/gfx/ingame.dat";
static const char* const BOLD_FONT = "$GAME_RESOURCES$/harvestClientData/gfx/boldFont.fnt";

CPriorityScreen::CPriorityScreen(ox::IOxDevice* device)
    : Device(device), ThreatLevel(0), Window(0), Frame(0), Dragging(false), DraggedBox(0), DragX(0), DragY(0),
      Font(0), TooltipX(0), TooltipY(0)
{
    GUIEnvironment = device->getGUIEnvironment();
    Driver = device->getVideoDriver();
    for (int i = 0; i < 14; ++i)
        AlienIcons[i] = 0;
    for (int i = 0; i < ES_COUNT; ++i)
        Sprites[i] = 0;
    for (int i = 0; i < 5; ++i)
    {
        PriorityDisplays[i] = 0;
        BuildingIcons[i] = 0;
        TargetClosestBoxes[i] = 0;
        HoldFireBoxes[i] = 0;
    }
}

CPriorityScreen::~CPriorityScreen()
{
    if (Window)
        Window->remove();
    for (int i = 0; i < ES_COUNT; ++i)
        if (Sprites[i])
            Sprites[i]->remove();
    for (int i = 0; i < 14; ++i)
        if (AlienIcons[i])
            AlienIcons[i]->remove();
    for (int i = 0; i < 5; ++i)
        if (BuildingIcons[i])
            BuildingIcons[i]->remove();
    emptyPriorityBoxes();
}

void CPriorityScreen::emptyPriorityBoxes()
{
    for (int i = 0; i < 5; ++i)
    {
        for (unsigned int j = 0; j < PriorityBoxes[i].size(); ++j)
            delete PriorityBoxes[i][j];
        PriorityBoxes[i].clear();
    }
}

bool CPriorityScreen::OnEvent(const ox::event::SEvent& event)
{
    switch (event.EventType)
    {
    case ox::event::EET_GUI_EVENT:
    {
        int id = event.GUIEvent.Caller->getID();
        switch (event.GUIEvent.EventType)
        {
        case ox::gui::EGET_BUTTON_CLICKED:
            switch (id)
            {
            case ID_CONFIRM:
                savePrioritiesByGUI();
                setVisible(false, 0);
                sendCustomEvent(ECE_CONTINUE_GAME);
                return true;
            case ID_CANCEL:
                setVisible(false, 0);
                sendCustomEvent(ECE_CONTINUE_GAME);
                return true;
            }
            break;
        case ox::gui::EGET_CHECKBOX_TOGGLED:
            for (int i = 0; i < 5; ++i)
            {
                if (TargetClosestBoxes[i] && event.GUIEvent.Caller == TargetClosestBoxes[i])
                {
                    settings::g_attackPriorities[i].setIfRangeIsImportant(TargetClosestBoxes[i]->isChecked());
                    return true;
                }
                if (HoldFireBoxes[i] && event.GUIEvent.Caller == HoldFireBoxes[i])
                {
                    settings::g_attackPriorities[i].setHoldFire(HoldFireBoxes[i]->isChecked());
                    return true;
                }
            }
            break;
        case ox::gui::EGET_ELEMENT_DRAWN:
            if (id == ID_FRAME)
            {
                ox::core::CRect<int> screen = GUIEnvironment->getRootGUIElement()->getAbsolutePosition();
                Driver->draw2DRectangle(ox::video::SColor(0x80000020), screen, 0);
                ox::core::CRect<int> rect = event.GUIEvent.Caller->getAbsolutePosition();
                int x = rect.UpperLeftCorner.X;
                int y = rect.UpperLeftCorner.Y;
                if (Sprites[ES_LEFT_BACKGROUND])
                    Sprites[ES_LEFT_BACKGROUND]->draw(ox::core::CPosition2d<int>(x, y + 41), 0,
                        ox::video::SColor(0xffffffff));
                if (Sprites[ES_RIGHT_BACKGROUND])
                    Sprites[ES_RIGHT_BACKGROUND]->draw(ox::core::CPosition2d<int>(x + 61, y + 51), 0,
                        ox::video::SColor(0xffffffff));
                return true;
            }
            else if (id >= ID_PRIORITY_DISPLAY && id < ID_PRIORITY_DISPLAY + 5)
            {
                ox::core::CRect<int> rect = event.GUIEvent.Caller->getAbsolutePosition();
                int x = rect.UpperLeftCorner.X;
                int y = rect.UpperLeftCorner.Y;
                if (Sprites[ES_DISPLAY_BACKGROUND])
                    Sprites[ES_DISPLAY_BACKGROUND]->draw(ox::core::CPosition2d<int>(x - 6, y - 60), 0,
                        ox::video::SColor(0xffffffff));
                ox::core::CString<wchar_t> text(L"");
                int index = id - ID_PRIORITY_DISPLAY;
                if (BuildingIcons[index])
                    BuildingIcons[index]->draw(ox::core::CPosition2d<int>(x + 18, y - 56), 0,
                        ox::video::SColor(0xffffffff));

                int hovered = -1;
                if (!Dragging)
                {
                    for (int j = (int)PriorityBoxes[index].size() - 1; j >= 0; --j)
                    {
                        if (PriorityBoxes[index][j]->Rect.isPointInside(GUIEnvironment->getMousePosition()))
                        {
                            ShowTooltip = true;
                            TooltipText = entity::getAlienName(PriorityBoxes[index][j]->AlienType);
                            TooltipX = PriorityBoxes[index][j]->Rect.UpperLeftCorner.X;
                            TooltipY = PriorityBoxes[index][j]->Rect.UpperLeftCorner.Y;
                            hovered = j;
                            break;
                        }
                    }
                }

                for (int j = 0; j < (int)PriorityBoxes[index].size(); ++j)
                {
                    int frame;
                    if (Dragging && PriorityBoxes[index][j] == DraggedBox)
                    {
                        SPriorityBox* box = PriorityBoxes[index][j];
                        if (Sprites[ES_ARROW_UP] && box->Priority < 3)
                            Sprites[ES_ARROW_UP]->draw(ox::core::CPosition2d<int>(box->Rect.UpperLeftCorner.X + 18,
                                box->Rect.UpperLeftCorner.Y - 11), 0, ox::video::SColor(0xffffffff));
                        frame = ES_ICON_FRAME_PRESSED;
                        if (Sprites[ES_ARROW_DOWN] && PriorityBoxes[index][j]->Priority > 0)
                            Sprites[ES_ARROW_DOWN]->draw(
                                ox::core::CPosition2d<int>(PriorityBoxes[index][j]->Rect.UpperLeftCorner.X + 18,
                                    PriorityBoxes[index][j]->Rect.UpperLeftCorner.Y + 53),
                                0, ox::video::SColor(0xffffffff));
                    }
                    else
                    {
                        frame = ES_ICON_FRAME_NORMAL;
                        if (hovered == j)
                            frame = ES_ICON_FRAME_HIGHLIGHTED;
                    }
                    if (Sprites[frame])
                        Sprites[frame]->draw(PriorityBoxes[index][j]->Rect.UpperLeftCorner, 0,
                            ox::video::SColor(0xffffffff));

                    int alienType = PriorityBoxes[index][j]->AlienType;
                    if (AlienIcons[alienType])
                    {
                        ox::video::SColor color;
                        if (ThreatLevel && ThreatLevel->alienIsPresentAtThisLevel(alienType))
                            color = ox::video::SColor(0xffffffff);
                        else
                            color = ox::video::SColor(0xa8000000);
                        SPriorityBox* box = PriorityBoxes[index][j];
                        AlienIcons[box->AlienType]->draw(ox::core::CPosition2d<int>(box->Rect.UpperLeftCorner.X + 1,
                            box->Rect.UpperLeftCorner.Y + 1), 0, color);
                    }
                }
                return true;
            }
            else if (id == ID_WINDOW)
            {
                if (ShowTooltip)
                {
                    ShowTooltip = false;
                    if (TooltipText.size() > 0 && Font)
                    {
                        ox::core::CDimension2d<int> size = Font->getDimension(TooltipText.c_str());
                        SSizedRect textRect(
                            ox::core::CPosition2d<int>(TooltipX + (52 - size.Width) / 2, TooltipY + 52), size);
                        ox::core::CRect<int> background(textRect.UpperLeftCorner.X - 2, textRect.UpperLeftCorner.Y - 2,
                            textRect.LowerRightCorner.X + 2, textRect.LowerRightCorner.Y + 2);
                        Driver->draw2DRectangle(ox::video::SColor(0x80000000), background, 0);
                        Font->draw(TooltipText.c_str(), textRect, ox::video::SColor(0xffffffff), ox::gui::EFHA_LEFT,
                            ox::gui::EFVA_TOP, 0);
                    }
                }
            }
            break;
        }
        break;
    }
    case ox::event::EET_MOUSE_INPUT_EVENT:
    {
        ox::core::CPosition2d<int> mouse(event.MouseInput.X, event.MouseInput.Y);
        switch (event.MouseInput.Event)
        {
        case ox::event::EMIE_LMOUSE_PRESSED_DOWN:
            for (int i = 0; i < 5; ++i)
            {
                for (int j = (int)PriorityBoxes[i].size() - 1; j >= 0; --j)
                {
                    SPriorityBox* box = PriorityBoxes[i][j];
                    if (box->Rect.isPointInside(mouse))
                    {
                        DraggedBox = box;
                        Dragging = true;
                        DragX = mouse.X;
                        DragY = mouse.Y;
                        Device->getCursorControl()->setVisible(false);
                        break;
                    }
                }
            }
            break;
        case ox::event::EMIE_LMOUSE_LEFT_UP:
            if (Dragging)
                Device->getCursorControl()->setVisible(true);
            DraggedBox = 0;
            Dragging = false;
            break;
        case ox::event::EMIE_MOUSE_MOVED:
            if (Dragging && DraggedBox)
            {
                // Dragging a box up by more than 30 pixels raises its priority, down lowers it.
                int dy = mouse.Y - DragY;
                if (dy < -30)
                {
                    if (DraggedBox->Priority < 3)
                    {
                        ++DraggedBox->Priority;
                        updatePriorityBoxPositions();
                    }
                    DragX = mouse.X;
                    DragY = mouse.Y;
                }
                else if (dy > 30)
                {
                    if (DraggedBox->Priority > 0)
                    {
                        --DraggedBox->Priority;
                        updatePriorityBoxPositions();
                    }
                    DragX = mouse.X;
                    DragY = mouse.Y;
                }
            }
            break;
        }
        return true;
    }
    case ox::event::EET_KEY_INPUT_EVENT:
        if (event.KeyInput.Event == ox::event::EKIE_KEY_PRESSED_DOWN)
        {
            if (settings::gp_profileManager->getCurrentProfile()->getCommandForKey(event.KeyInput.Key) ==
                COMMAND_PRIORITY_SCREEN)
            {
                setVisible(false, 0);
                sendCustomEvent(ECE_CONTINUE_GAME);
            }
        }
        else if (event.KeyInput.Event == ox::event::EKIE_KEY_LEFT_UP)
        {
            if (event.KeyInput.Key == ox::KEY_ESCAPE)
            {
                setVisible(false, 0);
                sendCustomEvent(ECE_CONTINUE_GAME);
            }
        }
        return true;
    default:
        break;
    }
    return false;
}

void CPriorityScreen::savePrioritiesByGUI()
{
    for (int i = 0; i < 5; ++i)
    {
        for (unsigned int j = 0; j < PriorityBoxes[i].size(); ++j)
            settings::g_attackPriorities[i].setAlienPriority(PriorityBoxes[i][j]->AlienType,
                PriorityBoxes[i][j]->Priority);
        if (TargetClosestBoxes[i])
            settings::g_attackPriorities[i].setIfRangeIsImportant(TargetClosestBoxes[i]->isChecked());
        if (HoldFireBoxes[i])
            settings::g_attackPriorities[i].setHoldFire(HoldFireBoxes[i]->isChecked());
        if (settings::gp_profileManager && settings::gp_profileManager->getCurrentProfile())
        {
            settings::gp_profileManager->getCurrentProfile()->setAttackPriority(i,
                settings::g_attackPriorities[i].prioritiesToString());
            settings::gp_profileManager->getCurrentProfile()->setAttackRangeMatters(i,
                settings::g_attackPriorities[i].RangeIsImportant);
        }
    }
}

void CPriorityScreen::setVisible(bool visible, game::CThreatLevel* threatLevel)
{
    ThreatLevel = threatLevel;
    if (visible)
    {
        if (!Frame || !Window)
        {
            loadSprites();
            Font = GUIEnvironment->getFont(BOLD_FONT);
            int width = SpriteSizes[ES_LEFT_BACKGROUND].Width + SpriteSizes[ES_RIGHT_BACKGROUND].Width;
            int height = SpriteSizes[ES_LEFT_BACKGROUND].Height + 91;

            Window = GUIEnvironment->addLayoutGroup(GUIEnvironment->getRootGUIElement()->getRelativePosition(), 0);
            Window->setID(ID_WINDOW);
            Window->setReportOnDraw(2);
            Frame = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, width, height), Window);
            Frame->setID(ID_FRAME);
            Frame->setReportOnDraw(1);

            ox::core::CString<wchar_t> targetClosest =
                settings::gp_systemConfig->getLocalizedText(L"prio:targetClosest");
            ox::core::CString<wchar_t> holdFire = settings::gp_systemConfig->getLocalizedText(L"prio:holdFire");
            for (int i = 0; i < 5; ++i)
            {
                int x = 64 + i * 135;
                ox::gui::IGUIElement* column = GUIEnvironment->addLayoutGroup(
                    SSizedRect(ox::core::CPosition2d<int>(x, 0), SpriteSizes[ES_DISPLAY_BACKGROUND]), Frame);
                if (i == 0)
                    column->LayoutFlags = "br";
                ox::gui::IGUIElement* title =
                    GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(24, 4, 107, 41), column);
                ox::core::CString<wchar_t> name =
                    settings::gp_systemConfig->getLocalizedText(settings::PRIORITY_KEY_NAMES[i]);
                ox::video::SColor color(0xffffffff);
                GUIEnvironment->addHoverDescription(title,
                    settings::gp_systemConfig->getLocalizedText(L"prio:prioFor", name.c_str()).c_str(), &color);

                ox::gui::IGUIElement* display =
                    GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(6, 60, 124, 276), column);
                display->setID(ID_PRIORITY_DISPLAY + i);
                display->setReportOnDraw(2);

                TargetClosestBoxes[i] = GUIEnvironment->addCheckBox(false, ox::core::CRect<int>(6, 279, 106, 309),
                    column, ID_PRIORITY_DISPLAY + i, L"");
                TargetClosestBoxes[i]->setAnimations(Driver->getSpritePackage(INGAME_SPRITES, false),
                    "BtnTargetClosest");
                GUIEnvironment->addHoverDescription(TargetClosestBoxes[i], targetClosest.c_str(), &color);
                HoldFireBoxes[i] = GUIEnvironment->addCheckBox(false, ox::core::CRect<int>(66, 279, 106, 309),
                    column, ID_PRIORITY_DISPLAY + i, L"");
                HoldFireBoxes[i]->setAnimations(Driver->getSpritePackage(INGAME_SPRITES, false), "BtnHoldFire");
                GUIEnvironment->addHoverDescription(HoldFireBoxes[i], holdFire.c_str(), &color);
                PriorityDisplays[i] = display;
            }

            ox::gui::IGUILayout* buttons = (ox::gui::IGUILayout*)GUIEnvironment->addLayoutGroup(
                ox::core::CRect<int>(0, SpriteSizes[ES_DISPLAY_BACKGROUND].Height + 5, width,
                    SpriteSizes[ES_DISPLAY_BACKGROUND].Height + 45), Frame);
            ox::core::CString<wchar_t> text = settings::gp_systemConfig->getLocalizedText(L"prio:confirm");
            ox::gui::IGUIButton* button =
                GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), buttons, ID_CONFIRM, text.c_str());
            button->LayoutFlags = "center";
            button->setOverrideFont(GUIEnvironment->getFont(BOLD_FONT));
            text = settings::gp_systemConfig->getLocalizedText(L"prio:cancel");
            button = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), buttons, ID_CANCEL, text.c_str());
            button->setOverrideFont(GUIEnvironment->getFont(BOLD_FONT));
            buttons->sortRiver(false, 10, 2, false);
        }
        Window->setRelativePosition(GUIEnvironment->getRootGUIElement()->getRelativePosition());
        Frame->centerOnParent();
        createPriorityBoxes();
        updatePriorityBoxPositions();
        Window->setVisible(true);
    }
    else if (Window)
    {
        Window->setVisible(false);
    }
}

void CPriorityScreen::updatePriorityBoxPositions()
{
    for (int i = 0; i < 5; ++i)
    {
        if (TargetClosestBoxes[i])
            TargetClosestBoxes[i]->setChecked(settings::g_attackPriorities[i].RangeIsImportant);
        if (HoldFireBoxes[i])
            HoldFireBoxes[i]->setChecked(settings::g_attackPriorities[i].HoldFire);
        if (PriorityDisplays[i])
        {
            ox::core::CRect<int> rect = PriorityDisplays[i]->getAbsolutePosition();
            // The boxes of a priority row are spread evenly over the display, a lone one centered.
            int width = rect.LowerRightCorner.X - 4 - rect.UpperLeftCorner.X;
            int count[4];
            int placed[4];
            for (int k = 0; k < 4; ++k)
            {
                count[k] = 0;
                placed[k] = 0;
            }
            for (unsigned int j = 0; j < PriorityBoxes[i].size(); ++j)
                ++count[PriorityBoxes[i][j]->Priority];
            for (unsigned int j = 0; j < PriorityBoxes[i].size(); ++j)
            {
                SPriorityBox* box = PriorityBoxes[i][j];
                int priority = box->Priority;
                int x;
                if (count[priority] > 1)
                    x = rect.UpperLeftCorner.X + 3 + (width - 52) / (count[priority] - 1) * placed[priority];
                else
                    x = rect.UpperLeftCorner.X + 3 + (width - 52) / 2;
                ++placed[priority];
                int y = rect.LowerRightCorner.Y - 54 - priority * 54;
                box->Rect = ox::core::CRect<int>(x, y, x + 52, y + 52);
            }
        }
    }
}

void CPriorityScreen::loadSprites()
{
    if (!Driver || Sprites[ES_LEFT_BACKGROUND])
        return;
    ox::video::ISpritePackage* package = Driver->getSpritePackage(INGAME_SPRITES, false);
    if (!package)
        return;

    Sprites[ES_LEFT_BACKGROUND] = package->addNewAnimationState("PriorityGuiLeftBackground(en)");
    Sprites[ES_RIGHT_BACKGROUND] = package->addNewAnimationState("PriorityGuiRightBackground");
    Sprites[ES_DISPLAY_BACKGROUND] = package->addNewAnimationState("PriorityDisplayBackground(en)");
    Sprites[ES_ICON_FRAME_NORMAL] = package->addNewAnimationState("PriorityIconFrameNormal");
    Sprites[ES_ICON_FRAME_HIGHLIGHTED] = package->addNewAnimationState("PriorityIconFrameHighlighted");
    Sprites[ES_ICON_FRAME_PRESSED] = package->addNewAnimationState("PriorityIconFramePressed");
    Sprites[ES_ARROW_UP] = package->addNewAnimationState("PriorityArrowUp");
    Sprites[ES_ARROW_DOWN] = package->addNewAnimationState("PriorityArrowDown");

    AlienIcons[0] = package->addNewAnimationState("PriorityIconMilky");
    AlienIcons[1] = package->addNewAnimationState("PriorityIconShielder");
    AlienIcons[2] = package->addNewAnimationState("PriorityIconTiny");
    AlienIcons[3] = package->addNewAnimationState("PriorityIconSummoner");
    AlienIcons[4] = package->addNewAnimationState("PriorityIconLooker");
    AlienIcons[5] = package->addNewAnimationState("PriorityIconHarvester");
    AlienIcons[6] = package->addNewAnimationState("PriorityIconStealer");
    AlienIcons[7] = package->addNewAnimationState("PriorityIconBrain");
    AlienIcons[8] = package->addNewAnimationState("PriorityIconMega");

    BuildingIcons[0] = package->addNewAnimationState("PriorityBuildingLaser");
    BuildingIcons[1] = package->addNewAnimationState("PriorityBuildingConnected");
    BuildingIcons[2] = package->addNewAnimationState("PriorityBuildingMissile");
    BuildingIcons[3] = package->addNewAnimationState("PriorityBuildingEagle");
    BuildingIcons[4] = package->addNewAnimationState("PriorityBuildingTempest");

    for (int i = 0; i < ES_COUNT; ++i)
        if (Sprites[i])
            SpriteSizes[i] = Sprites[i]->getFrameSize(0);
}

void CPriorityScreen::createPriorityBoxes()
{
    if (!game::gp_world)
        return;
    emptyPriorityBoxes();
    int planet = game::gp_world->getPlanet();
    for (int i = 0; i < 5; ++i)
    {
        for (int j = 0; j < 14; ++j)
        {
            if (game::CThreatLevel::alienOccursOnPlanet(planet, j) == true)
            {
                int priority = settings::g_attackPriorities[i].Priorities[j];
                SPriorityBox* box = new SPriorityBox;
                box->AlienType = j;
                box->Priority = priority;
                PriorityBoxes[i].push_back(box);
            }
        }
    }
}

bool CPriorityScreen::isVisible()
{
    if (Window)
        return Window->isVisible();
    return false;
}

} // end namespace gui
} // end namespace harvest
