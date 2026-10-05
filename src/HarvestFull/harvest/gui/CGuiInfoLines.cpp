// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include "harvest/game/CWorld.h"
#include "CGuiInfoLines.h"
#include "ox/IOxDevice.h"
#include "ox/core/CString.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIFont.h"
#include "ox/gui/IGUISkin.h"
#include "ox/gui/IGUIStaticText.h"
#include "ox/gui/IGUIWindow.h"

namespace harvest {
namespace gui {

CGuiInfoLines::CGuiInfoLines(ox::IOxDevice* device)
    : Device(device), SmallFontHeight(0), FirstUpdate(true)
{
    GUIEnvironment = device->getGUIEnvironment();
    Driver = Device->getVideoDriver();
    SmallFont = GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/smallFont.fnt");
    BoldFont = GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/boldFont.fnt");
    if (SmallFont)
        SmallFontHeight = SmallFont->getDimension(L"A").Height;
    if (BoldFont)
        BoldFontHeight = BoldFont->getDimension(L"A").Height;
    Container = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 50, 50), 0);
}

CGuiInfoLines::~CGuiInfoLines()
{
    for (std::list<SInfoLine*>::iterator it = InfoLines.begin(); it != InfoLines.end(); ++it)
        delete *it;
    if (Container)
        Container->remove();
}

void CGuiInfoLines::update(float time, int bottom)
{
    bool removed = false;
    std::list<SInfoLine*>::iterator it = InfoLines.begin();
    while (it != InfoLines.end())
    {
        (*it)->TimeLeft -= time;
        if ((*it)->TimeLeft <= 0 ||
            ((*it)->Element->getAbsolutePosition().UpperLeftCorner.Y <= 199 && !FirstUpdate))
        {
            Container->removeChild((*it)->Element);
            delete *it;
            it = InfoLines.erase(it);
            removed = true;
        }
        else
            ++it;
    }
    if (removed)
        Container->sortRiver(true, 1, 1, false);
    Container->moveTo(ox::core::CPosition2d<int>(10, bottom - Container->getRelativePosition().getHeight()));
    FirstUpdate = false;
}

void CGuiInfoLines::addInfoLine(const wchar_t* text)
{
    if (!SmallFont)
        return;
    ox::gui::IGUIStaticText* line = GUIEnvironment->addStaticText(text, "br", Container, SmallFont, -1);
    line->setOverrideColor(ox::video::SColor(0xffffff00));
    SInfoLine* infoLine = new SInfoLine;
    infoLine->Duration = ox::core::CString<wchar_t>(text).size() * 0.05f + 5.0f;
    infoLine->TimeLeft = infoLine->Duration;
    infoLine->Element = line;
    InfoLines.push_back(infoLine);
    Container->sortRiver(true, 1, 1, false);
}

void CGuiInfoLines::addInfoLine(const wchar_t* title, const wchar_t* text, ox::video::ISpritePackage* sprites,
    const char* icon)
{
    if (!BoldFont || !sprites)
        return;
    ox::gui::IGUIWindow* frame =
        static_cast<ox::gui::IGUIWindow*>(GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 50, 50), Container, -1));
    frame->LayoutFlags = "br";
    frame->setAnimations(GUIEnvironment->getSkin()->getSpritePackage(), "Black");
    GUIEnvironment->addStaticText(L" ", 100, frame, SmallFont, -1, L"")->setParagraphIcon(icon, sprites, true);
    ox::gui::IGUILayout* group = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 50, 50), frame);
    GUIEnvironment->addStaticText(title, "", group, BoldFont, -1);
    GUIEnvironment->addStaticText(text, 200, group, SmallFont, -1, L"")->LayoutFlags = "br";
    group->sortRiver(true, 1, 1, false);
    frame->sortRiver(true, 5, 1, false);
    SInfoLine* infoLine = new SInfoLine;
    infoLine->TimeLeft = 15.0f;
    infoLine->Element = frame;
    InfoLines.push_back(infoLine);
    Container->sortRiver(true, 1, 1, false);
}

} // end namespace gui
} // end namespace harvest
