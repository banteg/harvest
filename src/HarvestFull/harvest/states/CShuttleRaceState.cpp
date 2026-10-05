// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include <cmath>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CShuttleEntity.h"
#include "CShuttleRaceState.h"
#include "CLoadingScreen.h"
#include "harvest/entity/CEntityManager.h"
#include "ox/IOxDevice.h"
#include "ox/audio/IAudioDriver.h"
#include "ox/core/CBasic.h"
#include "ox/core/CMath.h"
#include "ox/core/CStringFunctions.h"
#include "ox/gui/IGUIElement.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIFont.h"
#include "ox/input/IJoystickDriver.h"
#include "ox/video/IParticlePackage.h"
#include "ox/video/IParticleState.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/IVideoDriver.h"

namespace harvest {
namespace states {

int CShuttleRaceState::getDoodadSeed() const
{
    return 1;
}

bool CShuttleRaceState::OnEvent(const ox::event::SEvent& event)
{
    bool result = false;
    switch (event.EventType)
    {
    case ox::event::EET_GUI_EVENT:
        event.GUIEvent.Caller->getID();
        break;
    case ox::event::EET_MOUSE_INPUT_EVENT:
    {
        ox::core::CPosition2d<int> position(event.MouseInput.X, event.MouseInput.Y);
        if (event.MouseInput.Event == ox::event::EMIE_LMOUSE_LEFT_UP)
            result = true;
        else if (event.MouseInput.Event == ox::event::EMIE_MOUSE_MOVED)
            MousePosition = position;
        break;
    }
    case ox::event::EET_KEY_INPUT_EVENT:
        switch (event.KeyInput.Event)
        {
        case ox::event::EKIE_KEY_PRESSED_DOWN:
            Keys[event.KeyInput.Key] = true;
            break;
        case ox::event::EKIE_KEY_LEFT_UP:
            Keys[event.KeyInput.Key] = false;
            switch (event.KeyInput.Key)
            {
            case ox::KEY_KEY_N:
                NextState = 3;
                break;
            case ox::KEY_KEY_Y:
                NextState = 5;
                break;
            case ox::KEY_TAB:
                if (event.KeyInput.Shift)
                {
                    if (DebugIndex == 0)
                        DebugIndex = 27;
                    else
                        --DebugIndex;
                }
                else
                    DebugIndex = (DebugIndex + 1) % 28;
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return result;
}

ox::core::CPosition2d<float> CShuttleRaceState::getWorldPos(ox::core::CPosition2d<int> position)
{
    return ox::core::CPosition2d<float>(position.X + ViewPositions[0].X, position.Y + ViewPositions[0].Y);
}

void CShuttleRaceState::playParticleSound(const char* name, const ox::core::CVector3d<float>& position)
{
    if (AudioDriver)
        AudioDriver->playSound(name, 1.0f, 0.0f, 1.0f);
}

int CShuttleRaceState::firstInit(ox::IOxDevice* device)
{
    if (CGameState::firstInit(device) == 1)
        return 1;

    LoadingScreen = new CLoadingScreen();
    if (LoadingScreen->init(Driver) == 1)
        return 1;

    Device->setEventReceiver(this);
    return 0;
}

int CShuttleRaceState::secondInit()
{
    switch (InitStep++)
    {
    case 0:
        Font = GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/boldFont.fnt");
        if (!Font)
            return 1;
        break;
    case 1:
        SmallFont = GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/smallFont.fnt");
        LargeFont = GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/largeFont.fnt");
        break;
    case 3:
        ScreenSize = Driver->getScreenSize();
        ScreenSizeF.Width = (float)ScreenSize.Width;
        ScreenSizeF.Height = (float)ScreenSize.Height;
        break;
    case 8:
        entity::CEntity::gp_spritePackage =
            Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/ingame.dat", false);
        entity::CEntity::gp_particlePackage =
            Driver->getParticlePackage("$GAME_RESOURCES$/harvestClientData/gfx/particles.pfx");
        if (!entity::CEntity::gp_particlePackage)
            return 1;
        entity::CEntity::gp_particlePackage->setSpritePackage(
            Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/fx.dat", false));
        entity::CEntity::gp_particlePackage->setCallbackEngine(this);
        entity::CEntity::gp_particlePackage->ImportanceLevel = 2;
        entity::CEntity::gp_videoDriver = Driver;
        entity::CEntity::gp_audioDriver = AudioDriver;
        entity::CEntity::gp_alienChantFont = SmallFont;
        break;
    case 9:
        entity::gp_entityManager = new entity::CEntityManager();
        entity::g_nextEntityId = 1;
        break;
    case 10:
        game::gp_world = new game::CWorld(6, 2);
        if (!game::gp_world->initializeWorld(Driver,
                ox::core::CDimension2d<int>(ScreenSize.Width / 2, ScreenSize.Height)))
            return 1;
        game::gp_world->initializeNewGame(this);
        break;
    case 20:
        Shuttles[0] = new entity::CShuttleEntity(-1);
        Shuttles[1] = new entity::CShuttleEntity(-1);
        for (int i = 0; i < 2; ++i)
        {
            Finished[i] = false;
            const ox::core::CRect<float>& field = game::gp_world->getVisibleGameFieldSize();
            ViewPositions[i].X = (field.UpperLeftCorner.X + field.LowerRightCorner.X) * 0.5f;
            ViewPositions[i].Y = (field.UpperLeftCorner.Y + field.LowerRightCorner.Y) * 0.5f;
            ViewPositions[i].X += ScreenSizeF.Width * -0.25f;
            ViewPositions[i].Y -= ScreenSizeF.Height * 0.5f;
        }
        entity::gp_entityManager->appendEntity(Shuttles[0], 3);
        entity::gp_entityManager->appendEntity(Shuttles[1], 3);
        break;
    case 21:
        // The computer players.
        entity::gp_entityManager->appendEntity(new entity::CShuttleEntity(1), 3);
        entity::gp_entityManager->appendEntity(new entity::CShuttleEntity(2), 3);
        entity::gp_entityManager->appendEntity(new entity::CShuttleEntity(3), 3);
        entity::gp_entityManager->appendEntity(new entity::CShuttleEntity(4), 3);
        entity::gp_entityManager->appendEntity(new entity::CShuttleEntity(5), 3);
        entity::gp_entityManager->appendEntity(new entity::CShuttleEntity(6), 3);
        entity::gp_entityManager->appendEntity(new entity::CShuttleEntity(7), 3);
        entity::gp_entityManager->appendEntity(new entity::CShuttleEntity(8), 3);
        break;
    case 22:
        Sprites[SPRITE_CHECKPOINT] = entity::CEntity::gp_spritePackage->addNewAnimationState("RangeCircle");
        Sprites[SPRITE_LOGO] = Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true)
                                   ->addNewAnimationState("OxeyeLoading");
        GuideBeam.Beam = entity::CEntity::gp_spritePackage->addNewAnimationState("RangeLine");
        GuideBeam.Color = ox::video::SColor(0xc8fafafa);
        break;
    case 36:
        if (AudioDriver)
            AudioDriver->playMusic("mus_intro.ogg", 1.0f, false);
        break;
    // Idle steps, which give the loading screen time to show.
    case 2: case 4: case 5: case 6: case 7: case 11: case 12: case 13: case 14: case 15: case 16:
    case 17: case 18: case 19: case 23: case 24: case 25: case 26: case 27: case 28: case 29:
    case 30: case 31: case 32: case 33: case 34: case 35:
        break;
    default:
        NextState = 0;
        return 0;
    }
    return 2;
}

void CShuttleRaceState::applyInitialExpansions(game::CWorld* world)
{
    world->expandWorldFromCurrent(0, false);
    world->expandWorldFromCurrent(3, false);
    world->expandWorldFromCurrent(1, false);
    world->expandWorldFromCurrent(2, false);
    world->expandWorldFromCurrent(0, false);
    world->expandWorldFromCurrent(3, false);
    world->expandWorldFromCurrent(1, false);
    world->expandWorldFromCurrent(2, false);
}

void CShuttleRaceState::renderFirst()
{
    if (LoadingScreen)
        LoadingScreen->render(Device, Driver);
}

void CShuttleRaceState::addParticleEntity(ox::video::IParticleState* state, const ox::core::CVector3d<float>& position)
{
    if (!state)
        return;

    if (state->getParticleImportance() > 0)
    {
        bool visible = false;
        for (int i = 0; i < 2; ++i)
            if (ViewRects[i].isPointInside(ox::core::CPosition2d<float>(position.X, position.Y + position.Z)))
            {
                visible = true;
                break;
            }
        if (!visible)
        {
            state->remove();
            return;
        }
    }

    int count = entity::gp_entityManager->getEntityList(4).size();
    if (count >= 10000)
    {
        state->remove();
        return;
    }
    entity::gp_entityManager->appendEntity(new entity::CParticleEntity(state, position), 4);
}

CShuttleRaceState::CShuttleRaceState()
    : Font(0), LoadingScreen(0), InitStep(0), MousePosition(1, 1), GuideBeam(6.0f), TimeScale(1.0f),
      RaceState(RACE_COUNTDOWN), Countdown(3.0f), DebugIndex(0)
{
    for (int i = 0; i < 256; ++i)
        Keys[i] = false;
    for (int i = 0; i < SPRITE_COUNT; ++i)
        Sprites[i] = 0;
}

int CShuttleRaceState::updateState(float time)
{
    if (!Device->run() || !Driver)
        return 1;

    float frameDelta = ox::core::clamp(time * TimeScale, 0.0f, 0.06f);

    if (Shuttles)
    {
        // The right player: arrow keys or the second gamepad.
        float axis = JoystickDriver->getJoystickAxes(1, 0).X;
        Shuttles[1]->setMovementFlag(1, axis < -0.5f || Keys[ox::KEY_LEFT]);
        Shuttles[1]->setMovementFlag(2, axis > 0.5f || Keys[ox::KEY_RIGHT]);
        Shuttles[1]->setMovementFlag(4, JoystickDriver->isButtonPressed(1, 0) || Keys[ox::KEY_UP]);

        // The left player: W, A, D or the first gamepad.
        axis = JoystickDriver->getJoystickAxes(0, 0).X;
        Shuttles[0]->setMovementFlag(1, axis < -0.5f || Keys[ox::KEY_KEY_A]);
        Shuttles[0]->setMovementFlag(2, axis > 0.5f || Keys[ox::KEY_KEY_D]);
        Shuttles[0]->setMovementFlag(4, JoystickDriver->isButtonPressed(0, 0) || Keys[ox::KEY_KEY_W]);
    }

    switch (RaceState)
    {
    case RACE_COUNTDOWN:
        Countdown -= frameDelta;
        ShowMessage = true;
        if (Countdown > 2.0f)
            Message = L"On your marks...";
        else if (Countdown > 1.0f)
            Message = L"ready...";
        else if (Countdown > 0.0f)
            Message = L"set...";
        else
        {
            Message = L"GO!";
            RaceState = RACE_RUNNING;
        }
        break;
    case RACE_RUNNING:
    {
        Countdown -= frameDelta;
        ShowMessage = Countdown > -1.0f;
        ox::core::CRect<float> visibleArea = game::gp_world->getVisibleGameFieldSize();
        // Update in steps of at most 10 ms so fast shuttles do not miss checkpoints.
        while (frameDelta > 0)
        {
            float step = frameDelta < 0.01f ? frameDelta : 0.01f;
            if (entity::gp_entityManager)
                entity::gp_entityManager->update(step, visibleArea);
            for (int i = 0; i < 2; ++i)
                if (!Finished[i] && Shuttles[i]->getCurrentLap() > 3)
                {
                    Finished[i] = true;
                    RaceState = RACE_FINISHED;
                    AudioDriver->playSound("aah.ogg", 1.0f, 0.0f, 1.0f);
                }
            frameDelta -= step;
        }
        break;
    }
    case RACE_FINISHED:
        ShowMessage = true;
        Message = L"We have a Winner! ";
        Message.append(ox::core::CString<wchar_t>(L"Time: "));
        Message.append(ox::core::CStringFunctions::millisecondsToWide(Shuttles[0]->getTotalTime(), false));
        SubMessage = L"Press N to Quit, or Y to Play Again";
        break;
    }

    for (int i = 0; i < 2; ++i)
    {
        if (!Shuttles[i])
            continue;
        const ox::core::CVector3d<float>& shuttlePosition = Shuttles[i]->getPosition();
        ViewPositions[i].X = shuttlePosition.X;
        ViewPositions[i].Y = shuttlePosition.Y;
        ViewPositions[i].X += Shuttles[i]->getCurrentSpeed().X * 0.5f;
        ViewPositions[i].Y += Shuttles[i]->getCurrentSpeed().Y * 0.5f;
        ViewPositions[i].X += ScreenSizeF.Width * -0.25f;
        ViewPositions[i].Y -= ScreenSizeF.Height * 0.5f;
        if (game::gp_world)
            game::gp_world->constrainViewPos(ViewPositions[i]);
        ViewRects[i].UpperLeftCorner.X = ViewPositions[i].X - 100.0f;
        ViewRects[i].UpperLeftCorner.Y = ViewPositions[i].Y - 100.0f;
        ViewRects[i].LowerRightCorner.X = ScreenSize.Width * 0.5f + 200.0f + ViewRects[i].UpperLeftCorner.X;
        ViewRects[i].LowerRightCorner.Y = ScreenSize.Height + 200.0f + ViewRects[i].UpperLeftCorner.Y;
    }
    return NextState;
}

CShuttleRaceState::~CShuttleRaceState()
{
    if (LoadingScreen)
        delete LoadingScreen;
    if (game::gp_world)
        delete game::gp_world;
    game::gp_world = 0;
    if (entity::gp_entityManager)
        delete entity::gp_entityManager;
    entity::gp_entityManager = 0;
    for (int i = 0; i < SPRITE_COUNT; ++i)
        if (Sprites[i])
            Sprites[i]->remove();
}

void CShuttleRaceState::render()
{
    Driver->beginScene(true, true, ox::video::SColor(0xff000000));

    for (int i = 0; i < 2; ++i)
    {
        int halfWidth = ScreenSize.Width / 2;
        ox::core::CRect<int> viewPort(0, 0, halfWidth, ScreenSize.Height);
        if (i == 1)
        {
            viewPort.UpperLeftCorner.X = halfWidth;
            viewPort.LowerRightCorner.X = halfWidth * 2;
        }
        Driver->setScissorRect(&viewPort);

        if (game::gp_world)
            game::gp_world->renderBackground(ViewPositions[i], Font, &viewPort);

        if (Sprites[SPRITE_CHECKPOINT])
            for (int j = 0; j < 14; ++j)
            {
                ox::core::CPosition2d<float> position(
                    entity::LAP_CHECKPOINTS[j].X - ViewPositions[i].X + viewPort.UpperLeftCorner.X,
                    entity::LAP_CHECKPOINTS[j].Y - ViewPositions[i].Y + viewPort.UpperLeftCorner.Y);
                ox::video::SColor color = j == Shuttles[i]->getNextCheckPoint() ? ox::video::SColor(0xffffffff)
                                                                               : ox::video::SColor(0xff40cc40);
                Sprites[SPRITE_CHECKPOINT]->drawScaled(position, 250.0f / 255.0f, color);
            }

        if (Sprites[SPRITE_LOGO])
            Sprites[SPRITE_LOGO]->draw(
                ox::core::CPosition2d<int>(viewPort.UpperLeftCorner.X - (int)ViewPositions[i].X + 1673,
                    viewPort.UpperLeftCorner.Y - (int)ViewPositions[i].Y - 856),
                0, ox::video::SColor(0xa0ffffff));

        if (entity::gp_entityManager)
        {
            if (Shuttles[i])
            {
                // Two beam stubs: towards the next checkpoint, and from it towards the one after.
                const ox::core::CVector3d<float>& shuttlePosition = Shuttles[i]->getPosition();
                GuideBeam.Start.X = shuttlePosition.X;
                GuideBeam.Start.Y = shuttlePosition.Y;
                GuideBeam.End = entity::LAP_CHECKPOINTS[Shuttles[i]->getNextCheckPoint()];
                float angle = ox::core::CMath::getAngleIY(GuideBeam.Start, GuideBeam.End);
                GuideBeam.End.X = GuideBeam.Start.X + cos(angle) * 125.0;
                GuideBeam.End.Y = GuideBeam.Start.Y + sin(angle) * 125.0;
                entity::gp_entityManager->renderEnergyBeam(&GuideBeam, ViewPositions[i], viewPort);

                GuideBeam.Start = entity::LAP_CHECKPOINTS[Shuttles[i]->getNextCheckPoint()];
                GuideBeam.End = entity::LAP_CHECKPOINTS[(Shuttles[i]->getNextCheckPoint() + 1) % 14];
                angle = ox::core::CMath::getAngleIY(GuideBeam.Start, GuideBeam.End);
                GuideBeam.End.X = GuideBeam.Start.X + cos(angle) * 125.0;
                GuideBeam.End.Y = GuideBeam.Start.Y + sin(angle) * 125.0;
                entity::gp_entityManager->renderEnergyBeam(&GuideBeam, ViewPositions[i], viewPort);
            }
            entity::gp_entityManager->renderEntities(ViewPositions[i], viewPort);

            for (int j = 0; j < 2; ++j)
            {
                if (!Shuttles[j])
                    continue;
                const ox::core::CVector3d<float>& position = Shuttles[j]->getPosition();
                ox::core::CString<wchar_t> name(L"P");
                name.append(j + 1);
                int x = (int)(position.X - ViewPositions[i].X) + viewPort.UpperLeftCorner.X;
                int y = (int)(position.Y - ViewPositions[i].Y) + viewPort.UpperLeftCorner.Y;
                Font->draw(name.c_str(), ox::core::CRect<int>(x - 50, y - 30, x + 50, y - 10),
                    ox::video::SColor(0xffffffff), ox::gui::EFHA_CENTER, ox::gui::EFVA_TOP, 0);
            }
        }

        if (LargeFont && Font)
        {
            ox::core::CString<wchar_t> text;
            if (RaceState != RACE_COUNTDOWN)
            {
                text = L"Position: ";
                text.append(Shuttles[i]->getCurrentPlacing());
            }
            else if (i != 0)
                text = L"Up, Left, Right or Gamepad 2";
            else
                text = L"W, A, D or Gamepad 1";
            LargeFont->draw(text.c_str(),
                ox::core::CRect<int>(viewPort.UpperLeftCorner.X + 10, ScreenSize.Height - 74, ScreenSize.Width / 2,
                    ScreenSize.Height),
                ox::video::SColor(0xffffffff), ox::gui::EFHA_LEFT, ox::gui::EFVA_TOP, 0);

            text = L"Lap ";
            text.append(Shuttles[i]->getCurrentLap());
            text.append(ox::core::CString<wchar_t>(L" of 3, Lap time: "));
            text.append(ox::core::CStringFunctions::millisecondsToWide(Shuttles[i]->getCurrentLapTime(), false));
            text.append(ox::core::CString<wchar_t>(L", Total time: "));
            text.append(ox::core::CStringFunctions::millisecondsToWide(Shuttles[i]->getTotalTime(), false));
            Font->draw(text.c_str(),
                ox::core::CRect<int>(viewPort.UpperLeftCorner.X + 10, ScreenSize.Height - 37, ScreenSize.Width / 2,
                    ScreenSize.Height),
                ox::video::SColor(0xffffffff), ox::gui::EFHA_LEFT, ox::gui::EFVA_TOP, 0);
        }

        // The line between the two views.
        if (i != 0)
            Driver->draw2DLine(viewPort.UpperLeftCorner,
                ox::core::CPosition2d<int>(viewPort.UpperLeftCorner.X, viewPort.LowerRightCorner.Y),
                ox::video::SColor(0xff000000));
        else
            Driver->draw2DLine(
                ox::core::CPosition2d<int>(viewPort.LowerRightCorner.X - 1, viewPort.UpperLeftCorner.Y),
                ox::core::CPosition2d<int>(viewPort.LowerRightCorner.X - 1, viewPort.LowerRightCorner.Y),
                ox::video::SColor(0xff000000));
    }

    Driver->setScissorRect(0);

    if (!ShowMessage && Font)
    {
        ox::core::CString<wchar_t> disclaimer(L"Disclamer: This \"game\" is not a part of Harvest: Massive "
                                              L"Encounter. No support is available from Oxeye Game Studio.");
        Font->draw(disclaimer.c_str(), ox::core::CRect<int>(10, ScreenSize.Height - 20, ScreenSize.Width,
            ScreenSize.Height), ox::video::SColor(0xffffffff), ox::gui::EFHA_LEFT, ox::gui::EFVA_TOP, 0);
    }

    if (ShowMessage && LargeFont)
    {
        LargeFont->draw(Message.c_str(), ox::core::CRect<int>(0, 0, ScreenSize.Width, ScreenSize.Height),
            ox::video::SColor(0xffffffff), ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER, 0);
        if (SubMessage.size() > 0)
            Font->draw(SubMessage.c_str(), ox::core::CRect<int>(0, 80, ScreenSize.Width, ScreenSize.Height),
                ox::video::SColor(0xffffffff), ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER, 0);
    }

    Driver->endScene();
}

} // end namespace states
} // end namespace harvest
