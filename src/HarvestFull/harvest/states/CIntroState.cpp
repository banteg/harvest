// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CAlienEntity.h"
#include "CIntroState.h"
#include "CLoadingScreen.h"
#include "harvest/settings/CSystemConfig.h"
#include "ox/IOxDevice.h"
#include "ox/audio/IAudioDriver.h"
#include "ox/core/CBasic.h"
#include "ox/core/CHiddenInt.h"
#include "ox/core/CRect.h"
#include "ox/gui/IGUIElement.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUISkin.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/IVideoDriver.h"

namespace harvest {

ox::core::CHiddenInt g_myRegKey;

namespace states {

//! Where the splash layers start and end their slide.
const ox::core::CPosition2d<int> START_LAYER_POSITIONS[5] = {
    ox::core::CPosition2d<int>(460, 340), ox::core::CPosition2d<int>(0, 191), ox::core::CPosition2d<int>(0, -10),
    ox::core::CPosition2d<int>(0, 350), ox::core::CPosition2d<int>(316, 400)};
const ox::core::CPosition2d<int> END_LAYER_POSITIONS[5] = {
    ox::core::CPosition2d<int>(449, 357), ox::core::CPosition2d<int>(-89, 191), ox::core::CPosition2d<int>(-89, 17),
    ox::core::CPosition2d<int>(-109, 558), ox::core::CPosition2d<int>(216, 686)};

CIntroState::CIntroState()
    : Font(0), LoadingScreen(0), InitStep(0), FadeState(0), Phase(0), SplashPackage(0), SkipElement(0)
{
    for (int i = 0; i < 256; ++i)
        Keys[i] = false;
    for (int i = 0; i < SPRITE_COUNT; ++i)
        Sprites[i] = 0;
}

CIntroState::~CIntroState()
{
    if (LoadingScreen)
        delete LoadingScreen;
    if (SplashPackage && Driver)
        Driver->removeSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestSplash.dat");
    if (SkipElement)
        SkipElement->remove();
}

int CIntroState::firstInit(ox::IOxDevice* device)
{
    if (CGameState::firstInit(device) != 0)
        return 1;

    Device->setWindowCaption(L"Harvest - (c) Oxeye Game Studio");
    LoadingScreen = new CLoadingScreen();
    if (LoadingScreen->init(Driver) == 1)
        return 1;

    Device->setEventReceiver(this);
    return 0;
}

int CIntroState::secondInit()
{
    switch (InitStep++)
    {
    case 0:
        Font = GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/boldFont.fnt");
        if (!Font)
            return 1;
        GUIEnvironment->getSkin()->setFont(Font);
        GUIEnvironment->getSkin()->setColor((ox::gui::EGUI_DEFAULT_COLOR)17, ox::video::SColor(0x80000020));
        break;
    case 1:
        GUIEnvironment->getSkin()->setSpritePackage(
            Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true));
        GUIEnvironment->getSkin()->setColor((ox::gui::EGUI_DEFAULT_COLOR)8, ox::video::SColor(0xffffffff));
        break;
    case 2:
        FadeCounter.setDelay(1.0f);
        LayerCounter.setDelay(7.0f);
        break;
    case 3:
        ScreenSize = Driver->getScreenSize();
        break;
    case 4:
        SplashPackage = Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestSplash.dat", true);
        if (!SplashPackage)
            return 1;
        break;
    case 5:
        if (ScreenSize.Height > 600)
        {
            Sprites[SPRITE_BORDER_ABOVE] = SplashPackage->addNewAnimationState("SplashBorderAbove");
            Sprites[SPRITE_BORDER_BENEATH] = SplashPackage->addNewAnimationState("SplashBorderBeneath");
        }
        break;
    case 6:
        Sprites[SPRITE_OXEYE_SPLASH] = SplashPackage->addNewAnimationState("OxeyeSplash");
        break;
    case 20:
        for (int i = 0; i < SPRITE_COUNT; ++i)
            if (Sprites[i])
                SpriteSizes[i] = Sprites[i]->getFrameSize(0);
        break;
    case 22:
        Phase = 2;
        FadeState = 1;
        if (AudioDriver)
            AudioDriver->playSound("aah.ogg", 1.0f, 0.0f, 1.0f);
        break;
    // Idle steps, which give the loading screen time to show.
    case 7: case 8: case 9: case 10: case 11: case 12: case 13: case 14: case 15: case 16:
    case 17: case 18: case 19: case 21: case 23: case 24: case 25: case 26: case 27: case 28:
    case 29: case 30: case 31: case 32: case 33: case 34: case 35: case 36:
        break;
    default:
        NextState = 0;
        return 0;
    }
    return 2;
}

int CIntroState::updateState(float time)
{
    if (!Device->run() || !Driver)
        return 1;

    float frameDelta = ox::core::clamp(time, 0.0f, 0.06f);
    if (FadeState == 2 || Phase != 2)
        LayerCounter.updateCounter(frameDelta);
    else if (FadeCounter.updateCounter(frameDelta))
    {
        if (FadeState == 0)
        {
            FadeState = 1;
            FadeCounter.setDelay(1.0f);
            if (AudioDriver)
                AudioDriver->playSound("aah.ogg", 1.0f, 0.0f, 1.0f);
        }
        else if (FadeState == 1)
            FadeState = 2;
        else
            NextState = 3;
    }

    float progress = LayerCounter.getProgress();
    for (int i = 0; i < 5; ++i)
    {
        LayerPositions[i].X = (int)((END_LAYER_POSITIONS[i].X - START_LAYER_POSITIONS[i].X) * progress) +
            START_LAYER_POSITIONS[i].X;
        LayerPositions[i].Y = (int)((END_LAYER_POSITIONS[i].Y - START_LAYER_POSITIONS[i].Y) * progress) +
            START_LAYER_POSITIONS[i].Y;
    }
    return NextState;
}

bool CIntroState::OnEvent(const ox::event::SEvent& event)
{
    switch (event.EventType)
    {
    case ox::event::EET_GUI_EVENT:
        event.GUIEvent.Caller->getID();
        break;
    case ox::event::EET_MOUSE_INPUT_EVENT:
        if (event.MouseInput.Event != ox::event::EMIE_LMOUSE_LEFT_UP)
            break;
        if (Phase != 0)
        {
            if (FadeState == 0 && FadeCounter.getProgress() > 0)
            {
                FadeState = 3;
                FadeCounter.setDelay(FadeCounter.getInvertedProgress());
                FadeCounter.overrideStartingValue(1.0f);
            }
            else if (FadeState == 1 && FadeCounter.getProgress() > 0)
            {
                FadeState = 3;
                FadeCounter.setDelay(FadeCounter.getProgress());
                FadeCounter.overrideStartingValue(1.0f);
            }
            else if (FadeState == 2)
            {
                FadeCounter.setDelay(1.0f);
                FadeState = 3;
            }
        }
        else
        {
            Phase = 2;
            if (SkipElement)
                SkipElement->setVisible(false);
        }
        return true;
    case ox::event::EET_KEY_INPUT_EVENT:
        if (event.KeyInput.Event == ox::event::EKIE_KEY_PRESSED_DOWN)
        {
            Keys[event.KeyInput.Key] = true;
            if (Phase != 0)
            {
                if (FadeState == 0 && FadeCounter.getProgress() > 0)
                {
                    FadeState = 3;
                    FadeCounter.setDelay(FadeCounter.getInvertedProgress());
                    FadeCounter.overrideStartingValue(1.0f);
                }
                else if (FadeState == 1 && FadeCounter.getProgress() > 0)
                {
                    FadeState = 3;
                    FadeCounter.setDelay(FadeCounter.getProgress());
                    FadeCounter.overrideStartingValue(1.0f);
                }
                else if (FadeState == 2)
                {
                    FadeCounter.setDelay(1.0f);
                    FadeState = 3;
                }
            }
            else
            {
                Phase = 2;
                if (SkipElement)
                    SkipElement->setVisible(false);
            }
        }
        else if (event.KeyInput.Event == ox::event::EKIE_KEY_LEFT_UP)
            Keys[event.KeyInput.Key] = false;
        break;
    case ox::event::EET_USER_EVENT:
        if (event.UserEvent.UserData1 == 42)
            GUIEnvironment->addMessageBox(L"",
                settings::gp_systemConfig->getLocalizedText(L"license:invalid").c_str(), true, ox::gui::EMBF_OK, 0,
                -1);
        break;
    default:
        break;
    }
    return false;
}

void CIntroState::renderFirst()
{
}

void CIntroState::render()
{
    ox::video::SColor background =
        Phase != 0 && FadeState != 0 && Phase != 1 ? ox::video::SColor(0xffffffff) : ox::video::SColor(0xff000000);
    Driver->beginScene(true, true, background);

    if (Phase == 2 && Sprites[SPRITE_OXEYE_SPLASH] && FadeState != 0)
    {
        ox::core::CPosition2d<int> position((ScreenSize.Width - SpriteSizes[SPRITE_OXEYE_SPLASH].X) / 2,
            (ScreenSize.Height - SpriteSizes[SPRITE_OXEYE_SPLASH].Y) / 2);
        if (position.Y < -66)
            position.Y = -66;
        Sprites[SPRITE_OXEYE_SPLASH]->draw(position, 0, ox::video::SColor(0xffffffff));
    }

    GUIEnvironment->drawAll();

    if (FadeState != 2 && Phase == 2)
    {
        int alpha;
        if (FadeState == 1)
            alpha = (int)(FadeCounter.getInvertedProgress() * 255.0f);
        else
            alpha = (int)(FadeCounter.getProgress() * 255.0f);
        Driver->draw2DRectangle(ox::video::SColor(alpha << 24 | 0xffffff),
            ox::core::CRect<int>(0, 0, ScreenSize.Width, ScreenSize.Height), 0);
    }

    Driver->endScene();
}

} // end namespace states
} // end namespace harvest
