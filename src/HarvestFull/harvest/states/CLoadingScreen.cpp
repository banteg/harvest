// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include "CLoadingScreen.h"
#include "ox/IOxDevice.h"
#include "ox/ITimer.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIFont.h"
#include "ox/gui/IGUISkin.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/IVideoDriver.h"

namespace harvest {
namespace states {

CLoadingScreen::CLoadingScreen()
    : Package(0), Logo(0), Rotation(0), LastRenderTime(0)
{
}

CLoadingScreen::~CLoadingScreen()
{
    if (Logo)
        Logo->remove();
}

int CLoadingScreen::init(ox::video::IVideoDriver* driver)
{
    Package = driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true);
    if (!Package)
        return 1;
    Logo = Package->addNewAnimationState("OxeyeLoading");
    return 0;
}

void CLoadingScreen::render(ox::IOxDevice* device, ox::video::IVideoDriver* driver)
{
    unsigned int time = device->getTimer()->getTime();
    if (time >= LastRenderTime + 100)
    {
        if (Logo)
        {
            ox::core::CDimension2d<int> screenSize = driver->getScreenSize();
            driver->beginScene(true, true, ox::video::SColor(0xff000000));
            Logo->drawRotated(ox::core::CPosition2d<float>(screenSize.Width * 0.5f, screenSize.Height * 0.5f),
                Rotation, 1.0f, ox::video::SColor(0xffffffff));

            if (device->getGUIEnvironment() && device->getGUIEnvironment()->getSkin() &&
                device->getGUIEnvironment()->getSkin()->getFont())
            {
                ox::core::CString<wchar_t> text = L"Harvest: Massive Encounter";
                text.append(ox::core::CString<wchar_t>(L" - "));
                text.append(ox::core::CString<wchar_t>("v1.18"));
                text.append(ox::core::CString<wchar_t>(L", (c) Oxeye Game Studio 2007-2008"));
                device->getGUIEnvironment()->getSkin()->getFont()->draw(text.c_str(),
                    ox::core::CRect<int>(0, screenSize.Height - 20, screenSize.Width, screenSize.Height),
                    ox::video::SColor(0xffffffff), ox::gui::EFHA_CENTER, ox::gui::EFVA_TOP, 0);
            }

            driver->endScene();
        }
        LastRenderTime = time;
    }
    Rotation += 0.1745329f;
}

} // end namespace states
} // end namespace harvest
