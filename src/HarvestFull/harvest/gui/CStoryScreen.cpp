// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include "harvest/game/CWorld.h"
#include "CStoryScreen.h"
#include "harvest/ECustomEvents.h"
#include "ox/IOxDevice.h"
#include "ox/audio/IAudioDriver.h"
#include "ox/core/CBasic.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUILayout.h"
#include "ox/gui/IGUIStaticText.h"
#include "ox/video/IVideoDriver.h"

namespace harvest {
namespace gui {

CStoryScreen::CStoryScreen(ox::IOxDevice* device)
    : Device(device), Window(0), State(STATE_HIDDEN), Fade(0), CloseWhenDone(false), TopBorder(0),
      BottomBorder(0)
{
    GUIEnvironment = device->getGUIEnvironment();
    Driver = device->getVideoDriver();
    Dialogues[0] = 0;
    Dialogues[1] = 0;
}

CStoryScreen::~CStoryScreen()
{
    if (Window)
        Window->remove();
    for (unsigned int i = 0; i < 2; ++i)
        delete Dialogues[i];
}

void CStoryScreen::update(float time)
{
    if (!isVisible())
        return;
    if (Blackness.isStarted())
        Blackness.updateCounter(time);
    if (State == STATE_OPEN)
    {
        Fade = 1.0f;
        float delta = -time;
        for (int i = 0; i < 2; ++i)
        {
            if (Dialogues[i])
            {
                Dialogues[i]->Time += delta;
                if (Dialogues[i]->Time <= 0)
                {
                    delete Dialogues[i];
                    Dialogues[i] = 0;
                    activateBorderText(i);
                }
            }
        }
        if (CloseWhenDone && !Dialogues[0] && !Dialogues[1])
        {
            State = STATE_CLOSING;
            if (TopBorder && BottomBorder)
            {
                TopBorder->setVisible(false);
                BottomBorder->setVisible(false);
            }
        }
    }
    else if (State == STATE_OPENING)
    {
        Fade = ox::core::min_(Fade + time * (1.0f / 3), 1.0f);
        if (Fade >= 1.0f)
        {
            State = STATE_OPEN;
            if (TopBorder && BottomBorder)
            {
                TopBorder->setVisible(true);
                BottomBorder->setVisible(true);
                activateBorderText(0);
                activateBorderText(1);
            }
        }
    }
    else if (State == STATE_CLOSING)
    {
        Fade = ox::core::max_(Fade - time * (1.0f / 3), 0.0f);
        if (Fade <= 0.0f)
        {
            State = STATE_HIDDEN;
            setVisible(false);
        }
    }
}

bool CStoryScreen::isVisible()
{
    if (Window)
        return Window->isVisible();
    return false;
}

void CStoryScreen::activateBorderText(int index)
{
    if (!Dialogues[index])
    {
        Border[index]->setVisible(false);
        return;
    }
    NameText[index]->setText(Dialogues[index]->Name.c_str());
    DialogueText[index]->setText(Dialogues[index]->Text.c_str());
    DialogueText[index]->setParagraphIcon("", 0, true);
    DialogueText[index]->activateProgressiveReveal(0);
    Portrait[index]->setParagraphIcon(Dialogues[index]->Portrait.c_str(),
        Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/ingame.dat", false), true);
    TextGroup[index]->sortRiver(true, 0, 2, false);
    Border[index]->sortRiver(true, 2, 2, false);
    Border[index]->centerOnParent();
    Border[index]->setVisible(true);
    if (Device && Device->getAudioDriver() && State == STATE_OPEN && Dialogues[index]->Sound.size() > 0)
        Device->getAudioDriver()->playVoice(Dialogues[index]->Sound.c_str());
}

void CStoryScreen::setVisible(bool visible)
{
    if (visible)
    {
        if (!Window)
        {
            Window = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 300, 300), 0);
            Window->setID(ID_WINDOW);
            Window->setReportOnDraw(true);
            TopBorder = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 300, 300), Window);
            TopBorder->setID(ID_TOP_BORDER);
            TopBorder->setReportOnDraw(true);
            BottomBorder = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 300, 300), Window);
            BottomBorder->setID(ID_BOTTOM_BORDER);
            BottomBorder->setReportOnDraw(true);
            createBorderText(0, TopBorder);
            createBorderText(1, BottomBorder);
        }
        ox::core::CDimension2d<int> screen = Driver->getScreenSize();
        Window->setRelativePosition(ox::core::CRect<int>(0, 0, screen.Width, screen.Height));
        int x = (screen.Width - 800) / 2;
        TopBorder->setRelativePosition(ox::core::CRect<int>(x, 0, x + 800, 120));
        BottomBorder->setRelativePosition(ox::core::CRect<int>(x, screen.Height - 120, x + 800, screen.Height));
        Window->setVisible(true);
        TopBorder->setVisible(false);
        BottomBorder->setVisible(false);
    }
    else if (Window)
        Window->setVisible(false);
}

void CStoryScreen::displayBlackness(float delay)
{
    Blackness.setDelay(delay);
}

void CStoryScreen::displayDialogueText(CDialogueItemInfo* info)
{
    int index = !info->Alternate;
    delete Dialogues[index];
    Dialogues[index] = new CDialogueItemInfo(info);
    if (!isVisible())
        setVisible(true);
    if (State != STATE_OPEN)
        State = STATE_OPENING;
    activateBorderText(index);
    CloseWhenDone = false;
}

bool CStoryScreen::OnEvent(const ox::event::SEvent& event)
{
    bool result = false;
    switch (event.EventType)
    {
    case ox::event::EET_GUI_EVENT:
    {
        int id = event.GUIEvent.Caller->getID();
        if (event.GUIEvent.EventType == ox::gui::EGET_ELEMENT_DRAWN && id == ID_WINDOW)
        {
            if (Blackness.isStarted())
            {
                float progress = Blackness.getInvertedProgress();
                int alpha = 255;
                if (progress < 0.5f)
                    alpha = ox::core::clamp((int)(progress * 511.0f), 0, 255);
                if (alpha > 0)
                {
                    ox::core::CRect<int> rect = event.GUIEvent.Caller->getAbsolutePosition();
                    Driver->draw2DRectangle(ox::video::SColor(alpha, 0, 0, 0), rect, 0);
                }
            }
            if (State != STATE_HIDDEN)
            {
                ox::core::CRect<int> rect = event.GUIEvent.Caller->getAbsolutePosition();
                int bottom = rect.LowerRightCorner.Y;
                rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y + (int)(Fade * 120.0f);
                Driver->draw2DRectangle(ox::video::SColor(0xff000000), rect, 0);
                rect.LowerRightCorner.Y = bottom;
                rect.UpperLeftCorner.Y = bottom - (int)(120.0f * Fade);
                Driver->draw2DRectangle(ox::video::SColor(0xff000000), rect, 0);
            }
            result = true;
        }
        break;
    }
    case ox::event::EET_MOUSE_INPUT_EVENT:
        result = true;
        if (event.MouseInput.Event != ox::event::EMIE_LMOUSE_LEFT_UP &&
            event.MouseInput.Event != ox::event::EMIE_LMOUSE_PRESSED_DOWN)
            result = State == STATE_OPEN;
        break;
    case ox::event::EET_KEY_INPUT_EVENT:
        if (event.KeyInput.Event == ox::event::EKIE_KEY_PRESSED_DOWN &&
            (event.KeyInput.Key == ox::KEY_SPACE || event.KeyInput.Key == ox::KEY_RETURN))
        {
            sendCustomEvent(ECE_SKIP_STORY);
            result = true;
        }
        break;
    default:
        break;
    }
    return result;
}

void CStoryScreen::createBorderText(int index, ox::gui::IGUIElement* parent)
{
    ox::gui::IGUIFont* font = GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/smallFont.fnt");
    Border[index] = (ox::gui::IGUILayout*)GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 500, 100), parent);
    Portrait[index] = GUIEnvironment->addStaticText(L" ", 100, Border[index], font, -1, L"");
    TextGroup[index] = (ox::gui::IGUILayout*)GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 500, 100),
        Border[index]);
    NameText[index] = GUIEnvironment->addStaticText(L" ", 400, TextGroup[index], 0, -1, L"");
    DialogueText[index] = GUIEnvironment->addStaticText(L" ", 400, TextGroup[index], 0, -1, L"");
    DialogueText[index]->LayoutFlags = "br";
    Border[index]->centerOnParent();
    Border[index]->setVisible(false);
}

} // end namespace gui
} // end namespace harvest
