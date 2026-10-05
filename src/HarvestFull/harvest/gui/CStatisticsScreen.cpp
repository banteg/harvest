// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include <math.h>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CHarvestEntity.h"
#include "CStatisticsScreen.h"
#include "harvest/CHarvestFullMain.h"
#include "harvest/game/CStatistics.h"
#include "harvest/game/CThreatLevel.h"
#include "harvest/settings/CHarvestProfile.h"
#include "harvest/settings/CProfileManager.h"
#include "harvest/settings/CSystemConfig.h"
#include "ox/IOxDevice.h"
#include "ox/audio/IAudioDriver.h"
#include "ox/core/CBasic.h"
#include "ox/core/CStringFunctions.h"
#include "ox/gui/IGUIButton.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIFont.h"
#include "ox/gui/IGUILayout.h"
#include "ox/gui/IGUISkin.h"
#include "ox/gui/IGUIStaticText.h"
#include "ox/net/CHTTPConnectionHandler.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/IVideoDriver.h"

namespace harvest {
namespace gui {

const char* const STAT_SELECTION_BUTTON_SPRITES[] = {"BtnStatisticsCredits", "BtnStatisticsBuildings",
    "BtnStatisticsMined", "BtnStatisticsKills", "BtnStatisticsLaser", "BtnStatisticsMissile",
    "BtnStatisticsBomb", "BtnStatisticsOverall"};

const wchar_t* const STAT_SELECTION_BUTTON_POPUPS[] = {L"statistics:statTotalCredits",
    L"statistics:statNumBuildings", L"statistics:statEarnedCredits", L"statistics:statKilledAliens",
    L"statistics:statTowerDamage", L"statistics:statMissileDamage", L"statistics:statBombDamage",
    L"statistics:overallStatistics"};

//! The info lines of the game totals, by harvest::game::CStatistics game statistic.
const wchar_t* const STAT_TOTALS[] = {L"statistics:levelReached", L"statistics:totalMinerals",
    L"statistics:totalKills", L"statistics:totalBuildings", L"statistics:totalTowerDamage",
    L"statistics:totalTurretDamage", L"statistics:totalBombDamage"};

const wchar_t* const STAT_SELECTION_GRAPH_POPUPS[] = {L"statistics:statTotalCreditsGraph",
    L"statistics:statNumBuildingsGraph", L"statistics:statEarnedCreditsGraph",
    L"statistics:statKilledAliensGraph", L"statistics:statTowerDamageGraph",
    L"statistics:statMissileDamageGraph", L"statistics:statBombDamageGraph"};

const char* const STAT_SPRITE_NAMES[] = {"StatisticsTop", "StatisticsBottom", "StatisticsDiagramBackground",
    "StatisticsOverallBackground", "StatisticsDiagramBar", "StatisticsPlanetIcon1", "StatisticsPlanetIcon2",
    "StatisticsPlanetIcon3", "ModeBtnSmallNormal", "ModeBtnSmallWave", "ModeBtnSmallInsane",
    "ModeBtnSmallRush", "ModeBtnSmallCreative", "StatisticsScore", "StatisticsCredits"};

CStatisticsScreen::CStatisticsScreen(ox::IOxDevice* device)
    : Device(device), SpritesLoaded(false), GraphType(GRAPH_OVERALL), Window(0), Frame(0), GraphPanel(0),
      InfoPanel(0), InfoScroll(0), Connection(0)
{
    GUIEnvironment = device->getGUIEnvironment();
    Driver = device->getVideoDriver();
    for (int i = 0; i < SPRITE_COUNT; ++i)
        Sprites[i] = 0;
}

CStatisticsScreen::~CStatisticsScreen()
{
    if (Window)
        Window->remove();
    for (int i = 0; i < SPRITE_COUNT; ++i)
        if (Sprites[i])
            Sprites[i]->remove();
    eraseGraphs();
    delete Connection;
}

void CStatisticsScreen::eraseGraphs()
{
    for (unsigned int i = 0; i < Graphs.size(); ++i)
        delete Graphs[i];
    Graphs.clear();
}

bool CStatisticsScreen::OnEvent(const ox::event::SEvent& event)
{
    bool result = false;
    switch (event.EventType)
    {
    case ox::event::EET_GUI_EVENT:
    {
        int id = event.GUIEvent.Caller->getID();
        switch (event.GUIEvent.EventType)
        {
        case ox::gui::EGET_BUTTON_CLICKED:
            if ((unsigned int)(id - ID_GRAPH_BUTTON) <= GRAPH_OVERALL)
            {
                int type = id - ID_GRAPH_BUTTON;
                if (GraphType != type && game::gp_statistics->getAllLevelStats().size() >= 2)
                {
                    GraphType = type;
                    if (type == GRAPH_OVERALL)
                    {
                        if (InfoPanel)
                            InfoPanel->setVisible(true);
                        if (GraphPanel)
                            GraphPanel->setVisible(false);
                    }
                    else
                        createGraphs(type);
                }
                result = true;
                break;
            }
            switch (id)
            {
            case ID_PAGE_UP:
                if (InfoScroll >= INFO_PAGE_LINES)
                {
                    InfoScroll -= INFO_PAGE_LINES;
                    updateTopString();
                }
                result = true;
                break;
            case ID_PAGE_DOWN:
                if (InfoScroll + INFO_PAGE_LINES < (int)InfoStrings.size())
                {
                    InfoScroll += INFO_PAGE_LINES;
                    updateTopString();
                }
                result = true;
                break;
            case ID_BACK:
                setVisible(false);
                result = true;
                break;
            }
            break;
        case ox::gui::EGET_MESSAGEBOX_YES:
            if (id == ID_SUBMIT_HIGHSCORE)
                submitHighscores();
            break;
        case ox::gui::EGET_ELEMENT_DRAWN:
            if (id == ID_GRAPH)
            {
                ox::core::CPosition2d<int> position =
                    event.GUIEvent.Caller->getAbsolutePosition().UpperLeftCorner;
                ox::core::CPosition2d<int> mouse = GUIEnvironment->getMousePosition();
                ox::core::CString<wchar_t> hoverText;
                for (unsigned int i = 0; i < Graphs.size(); ++i)
                {
                    if (Graphs[i]->Alpha <= 0)
                        break;
                    ox::video::SColor color(ox::core::clamp((int)(Graphs[i]->Alpha * 255.0f), 0, 255),
                        (i & 1) ? 0xc0 : 0, 0xc0, 0x40);
                    renderGraph(position, Graphs[i]->Rect, color);
                    if (Graphs[i]->Rect.isPointInside(mouse))
                        hoverText = Graphs[i]->Text;
                }
                for (int i = 0; i < 6; ++i)
                    Font->draw(AxisLabels[i].c_str(), AxisLabelRects[i], ox::video::SColor(0xffffffff),
                        ox::gui::EFHA_LEFT, ox::gui::EFVA_TOP, 0);
                if (hoverText.size() > 0)
                {
                    ox::core::CDimension2d<int> size = Font->getDimension(hoverText.c_str());
                    ox::core::CRect<int> rect(mouse.X - 13, mouse.Y + 37, mouse.X + size.Width - 7,
                        mouse.Y + size.Height + 43);
                    if (rect.LowerRightCorner.X > 800)
                    {
                        rect.UpperLeftCorner.X -= rect.LowerRightCorner.X - 800;
                        rect.LowerRightCorner.X = 800;
                    }
                    Driver->draw2DRectangle(ox::video::SColor(0xa0000000), rect, 0);
                    rect.UpperLeftCorner.X += 3;
                    rect.UpperLeftCorner.Y += 3;
                    Font->draw(hoverText.c_str(), rect, ox::video::SColor(0xffffffff), ox::gui::EFHA_LEFT,
                        ox::gui::EFVA_TOP, 0);
                }
                result = true;
            }
            else if (id == ID_WINDOW)
            {
                ox::core::CPosition2d<int> origin = event.GUIEvent.Caller->getAbsolutePosition().UpperLeftCorner;
                ox::core::CPosition2d<int> position = origin;
                if (Sprites[SPRITE_TOP])
                {
                    Sprites[SPRITE_TOP]->draw(position, 0, ox::video::SColor(0xffffffff));
                    position.Y += SpriteSizes[SPRITE_TOP].Y;
                }
                if (GraphType != GRAPH_OVERALL)
                {
                    if (Sprites[SPRITE_DIAGRAM_BACKGROUND])
                    {
                        Sprites[SPRITE_DIAGRAM_BACKGROUND]->draw(position, 0, ox::video::SColor(0xffffffff));
                        position.Y += SpriteSizes[SPRITE_DIAGRAM_BACKGROUND].Y;
                    }
                }
                else
                {
                    if (Sprites[SPRITE_OVERALL_BACKGROUND])
                    {
                        Sprites[SPRITE_OVERALL_BACKGROUND]->draw(position, 0, ox::video::SColor(0xffffffff));
                        position.Y += SpriteSizes[SPRITE_DIAGRAM_BACKGROUND].Y;
                    }
                    if (Sprites[SPRITE_SCORE])
                        Sprites[SPRITE_SCORE]->draw(ox::core::CPosition2d<int>(origin.X + 222, origin.Y + 85), 0,
                            ox::video::SColor(0xffffffff));
                    if (Sprites[SPRITE_CREDITS])
                        Sprites[SPRITE_CREDITS]->draw(ox::core::CPosition2d<int>(origin.X + 473, origin.Y + 85), 0,
                            ox::video::SColor(0xffffffff));
                    if (Sprites[SPRITE_PLANET_ICON + g_scenarioResultPlanet])
                        Sprites[SPRITE_PLANET_ICON + g_scenarioResultPlanet]->draw(
                            ox::core::CPosition2d<int>(origin.X + 64, origin.Y + 85), 0,
                            ox::video::SColor(0xffffffff));
                    int mode = g_scenarioResultGameMode;
                    if (Sprites[SPRITE_MODE_ICON + mode])
                        Sprites[SPRITE_MODE_ICON + mode]->draw(
                            ox::core::CPosition2d<int>(origin.X + (86 - SpriteSizes[SPRITE_MODE_ICON + mode].X) / 2 + 83,
                                origin.Y + (41 - SpriteSizes[SPRITE_MODE_ICON + mode].Y) / 2 + 65),
                            0, ox::video::SColor(0xffffffff));
                }
                if (Sprites[SPRITE_BOTTOM])
                    Sprites[SPRITE_BOTTOM]->draw(position, 0, ox::video::SColor(0xffffffff));
                result = true;
            }
            break;
        default:
            break;
        }
        break;
    }
    case ox::event::EET_NETWORK_EVENT:
        switch (event.NetworkEvent.Type)
        {
        case ox::event::ENET_HTTP_DONE:
        {
            ox::core::CString<char> answer(event.NetworkEvent.Data);
            if (answer == "success")
            {
                if (Device && Device->getAudioDriver())
                    Device->getAudioDriver()->playSound("upload.ogg", 1.0f, 0.0f, 1.0f);
                addInfoString(settings::gp_systemConfig->getLocalizedText(L"statistics:highscoreSuccess"));
            }
            else if (answer == "version")
                addInfoString(settings::gp_systemConfig->getLocalizedText(L"statistics:highscoreVersion"));
            result = true;
            break;
        }
        case ox::event::ENET_HTTP_ERROR:
            if (Device)
                Device->getAudioDriver();
            addInfoString(settings::gp_systemConfig->getLocalizedText(L"statistics:highscoreFailure"));
            result = true;
            break;
        default:
            break;
        }
        break;
    case ox::event::EET_MOUSE_INPUT_EVENT:
        switch (event.MouseInput.Event)
        {
        case ox::event::EMIE_LMOUSE_PRESSED_DOWN:
        case ox::event::EMIE_LMOUSE_LEFT_UP:
            result = true;
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

void CStatisticsScreen::setVisible(bool visible)
{
    if (visible)
    {
        Font = GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/boldFont.fnt");
        if (!Window)
        {
            loadSprites();
            ox::core::CString<wchar_t> minerals;
            ox::core::CString<wchar_t> level = L"-";
            if (game::gp_statistics)
            {
                minerals = ox::core::CString<wchar_t>(game::gp_statistics->getGameStatValue(1));
                game::CHighscoreInfo* info = game::CHighscoreInfo::getCurrentHighscoreInfo();
                if (info)
                {
                    int mode = info->getGameMode();
                    if (mode == game::EGM_WAVE || mode == game::EGM_RUSH)
                        level = ox::core::CStringFunctions::millisecondsToWide(info->getPlayTime(), true);
                    else
                        level = ox::core::CString<wchar_t>(info->getHighestLevel());
                }
            }

            Window = GUIEnvironment->addModalScreen();
            ox::gui::IGUILayout* layout = (ox::gui::IGUILayout*)GUIEnvironment->addLayoutGroup(
                ox::core::CRect<int>(0, 0, 800, 600), Window);
            ox::core::CRect<int> rect(0, 0, SpriteSizes[SPRITE_DIAGRAM_BACKGROUND].X,
                SpriteSizes[SPRITE_DIAGRAM_BACKGROUND].Y + SpriteSizes[SPRITE_TOP].Y + SpriteSizes[SPRITE_BOTTOM].Y);
            Frame = GUIEnvironment->addLayoutGroup(rect, layout);
            Frame->setReportOnDraw(true);
            Frame->setID(ID_WINDOW);
            Frame->LayoutFlags = "center br";

            const int BUTTON_X[] = {16, 104, 192, 280, 368, 456, 544, 632};
            const int BUTTON_Y[] = {389, 389, 389, 389, 389, 389, 389, 389};
            ox::video::SColor popupColor(0xffffffff);
            game::gp_statistics->getAllLevelStats();
            for (int i = 0; i <= GRAPH_OVERALL; ++i)
            {
                GraphButtons[i] = GUIEnvironment->addButton(
                    ox::core::CRect<int>(BUTTON_X[i], BUTTON_Y[i], BUTTON_X[i] + 90, BUTTON_Y[i] + 20), Frame,
                    ID_GRAPH_BUTTON + i, 0);
                GraphButtons[i]->setAnimations(GUIEnvironment->getSkin()->getSpritePackage(),
                    STAT_SELECTION_BUTTON_SPRITES[i], true);
                GUIEnvironment->addHoverDescription(GraphButtons[i],
                    settings::gp_systemConfig->getLocalizedText(STAT_SELECTION_BUTTON_POPUPS[i]).c_str(), &popupColor);
            }

            rect.LowerRightCorner.Y -= SpriteSizes[SPRITE_BOTTOM].Y;
            InfoPanel = GUIEnvironment->addLayoutGroup(rect, Frame);
            ox::gui::IGUIStaticText* text = GUIEnvironment->addStaticText(level.c_str(),
                ox::core::CRect<int>(264, 63, 417, 108), false, false, InfoPanel, -1, 0);
            text->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER);
            text->setOverrideFont(GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/largeFont.fnt"));
            text = GUIEnvironment->addStaticText(minerals.c_str(), ox::core::CRect<int>(520, 63, 673, 108), false,
                false, InfoPanel, -1, 0);
            text->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER);
            text->setOverrideFont(GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/largeFont.fnt"));

            ox::core::CString<wchar_t> bestLevel;
            ox::core::CString<wchar_t> bestMinerals(settings::gp_profileManager->getCurrentProfile()->getLocalScore(
                1, g_scenarioResultGameMode, g_scenarioResultPlanet));
            if (g_scenarioResultGameMode == game::EGM_WAVE || g_scenarioResultGameMode == game::EGM_RUSH)
                bestLevel = ox::core::CStringFunctions::millisecondsToWide(
                    settings::gp_profileManager->getCurrentProfile()->getLocalScore(
                        2, g_scenarioResultGameMode, g_scenarioResultPlanet) * 0.001f, true);
            else
                bestLevel = ox::core::CString<wchar_t>(settings::gp_profileManager->getCurrentProfile()->getLocalScore(
                    0, g_scenarioResultGameMode, g_scenarioResultPlanet));
            settings::gp_profileManager->getCurrentProfile()->getLocalScore(
                1, g_scenarioResultGameMode, g_scenarioResultPlanet);
            text = GUIEnvironment->addStaticText(bestLevel.c_str(), ox::core::CRect<int>(264, 115, 417, 136), false,
                false, InfoPanel, -1, 0);
            text->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER);
            text = GUIEnvironment->addStaticText(bestMinerals.c_str(), ox::core::CRect<int>(520, 114, 673, 136),
                false, false, InfoPanel, -1, 0);
            text->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER);
            text = GUIEnvironment->addStaticText(
                settings::gp_systemConfig->getLocalizedText(L"statistics:localBest").c_str(),
                ox::core::CRect<int>(42, 114, 174, 136), false, false, InfoPanel, -1, 0);
            text->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER);

            GraphPanel = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(49, 67, 646, 328), Frame);
            GraphPanel->setID(ID_GRAPH);
            GraphPanel->setReportOnDraw(true);
            GraphPanel->setVisible(false);

            const ox::TArray<game::SEventLog>& logs = game::gp_statistics->getLogs();
            for (unsigned int i = 0; i < logs.size(); ++i)
            {
                ox::core::CString<wchar_t> time = ox::core::CStringFunctions::millisecondsToWide(logs[i].Time, true);
                switch (logs[i].Type)
                {
                case 0:
                    addInfoString(settings::gp_systemConfig->getLocalizedText(L"statistics:logWave", time.c_str(),
                        logs[i].Value));
                    break;
                case 1:
                {
                    ox::core::CString<wchar_t> award =
                        settings::gp_systemConfig->getLocalizedText(settings::ACHIEVEMENT_NAMES[logs[i].Value]);
                    addInfoString(settings::gp_systemConfig->getLocalizedText(L"statistics:logAward", time.c_str(),
                        award.c_str()));
                    break;
                }
                }
            }
            for (int i = 1; i < 7; ++i)
                addInfoString(settings::gp_systemConfig->getLocalizedText(STAT_TOTALS[i],
                    game::gp_statistics->getGameStatValue(i)));

            if ((int)InfoStrings.size() > INFO_PAGE_LINES)
            {
                ox::gui::IGUIButton* up = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), InfoPanel,
                    ID_PAGE_UP, 0);
                up->setAnimations(Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true),
                    "BtnPageUp", true);
                ox::gui::IGUIButton* down = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), InfoPanel,
                    ID_PAGE_DOWN, 0);
                down->setAnimations(Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true),
                    "BtnPageDown", true);
                int width = up->getRelativePosition().getWidth();
                up->moveTo(ox::core::CPosition2d<int>(695 - width, 368 - width * 2));
                down->moveTo(ox::core::CPosition2d<int>(695 - width, 368 - width));
            }

            ox::gui::IGUIButton* back = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), layout,
                ID_BACK, settings::gp_systemConfig->getLocalizedText(L"menu:back").c_str());
            back->LayoutFlags = "br";
            back->setOverrideFont(GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/boldFont.fnt"));
            layout->sortRiver(false, 5, 5, false);
            layout->centerOnParent();
        }
        Window->setVisible(true);
        if (game::CHighscoreInfo::getCurrentHighscoreInfo())
        {
            ox::core::CString<wchar_t> title =
                settings::gp_systemConfig->getLocalizedText(L"statistics:highscoreTitle");
            ox::core::CString<wchar_t> question =
                settings::gp_systemConfig->getLocalizedText(L"statistics:highscoreQuestion");
            GUIEnvironment->addMessageBox(title.c_str(), question.c_str(), true,
                ox::gui::EMBF_YES | ox::gui::EMBF_NO, 0, ID_SUBMIT_HIGHSCORE);
        }
    }
    else if (Window)
        Window->setVisible(false);
}

void CStatisticsScreen::createGraphs(int type)
{
    eraseGraphs();
    if (GraphPanel && game::gp_statistics && !game::gp_statistics->getAllLevelStats().empty())
    {
        const ox::TArray<game::SLevelStats*>& stats = game::gp_statistics->getAllLevelStats();
        GraphFrame = GraphPanel->getAbsolutePosition();

        float maxValue = 0;
        for (unsigned int i = 0; i < stats.size() - 1; ++i)
            maxValue = ox::core::max_(stats[i]->Stats[type], maxValue);
        float top = floorf(ox::core::max_(maxValue * 1.1f, 4.0f) + 0.5f);
        float values[6];
        values[0] = 0;
        values[1] = floorf(top * 0.2f + 0.5f);
        values[2] = floorf(top * 0.4f + 0.5f);
        values[3] = floorf(top * 0.6f + 0.5f);
        values[4] = floorf(top * 0.8f + 0.5f);
        values[5] = top;
        for (int i = 0; i < 6; ++i)
        {
            AxisLabels[i] = ox::core::CString<wchar_t>((int)values[i]);
            AxisLabelSizes[i] = Font->getDimension(AxisLabels[i].c_str());
        }

        GraphArea = GraphFrame;
        float scale = (float)(GraphArea.LowerRightCorner.Y - GraphArea.UpperLeftCorner.Y) / top;
        for (int i = 0; i < 6; ++i)
        {
            int y = GraphArea.LowerRightCorner.Y - AxisLabelSizes[i].Height / 2 + (int)(values[i] * -scale);
            AxisLabelRects[i] = ox::core::CRect<int>(GraphArea.LowerRightCorner.X + 4, y,
                GraphArea.LowerRightCorner.X + 4 + AxisLabelSizes[i].Width, y + AxisLabelSizes[i].Height);
        }

        float step = (float)(GraphArea.LowerRightCorner.X - GraphArea.UpperLeftCorner.X);
        if (stats.size() > 1)
            step /= (float)(stats.size() - 1);
        int lastX = GraphArea.UpperLeftCorner.X;
        for (unsigned int i = 0; i < stats.size() - 1; ++i)
        {
            int x = (int)((i + 1) * step) + GraphArea.UpperLeftCorner.X;
            if (x != lastX)
            {
                int height = (int)(scale * stats[i]->Stats[type]);
                if (height > 0)
                {
                    SGraphColumn* column = new SGraphColumn;
                    column->Level = i;
                    column->Alpha = 0;
                    column->Value = stats[i]->Stats[type];
                    column->Rect = ox::core::CRect<int>(lastX, GraphArea.LowerRightCorner.Y - height, x,
                        GraphArea.LowerRightCorner.Y);
                    ox::core::CString<wchar_t> time = ox::core::CStringFunctions::millisecondsToWide(stats[i]->Time, true);
                    column->Text = settings::gp_systemConfig->getLocalizedText(L"statistics:gameTime", time.c_str());
                    column->Text += L", ";
                    column->Text += settings::gp_systemConfig->getLocalizedText(STAT_SELECTION_GRAPH_POPUPS[type],
                        column->Value);
                    Graphs.push_back(column);
                }
            }
            lastX = x;
        }
        InfoPanel->setVisible(false);
        GraphPanel->setVisible(true);
    }
}

void CStatisticsScreen::updateTopString()
{
    for (int i = 0; i < (int)InfoStrings.size(); ++i)
        InfoStrings[i]->setVisible(i >= InfoScroll && i < InfoScroll + INFO_PAGE_LINES);
}

void CStatisticsScreen::renderGraph(const ox::core::CPosition2d<int>& position, const ox::core::CRect<int>& clip,
    ox::video::SColor& color)
{
    if (clip.LowerRightCorner.X < position.X)
        return;
    ox::core::CRect<int> rect(position.X, position.Y, position.X + SpriteSizes[SPRITE_DIAGRAM_BAR].X,
        position.Y + SpriteSizes[SPRITE_DIAGRAM_BAR].Y);
    while (rect.UpperLeftCorner.X < clip.LowerRightCorner.X)
    {
        if (rect.isRectCollided(clip))
            Sprites[SPRITE_DIAGRAM_BAR]->draw(rect.UpperLeftCorner, &clip, color);
        rect.UpperLeftCorner.X += SpriteSizes[SPRITE_DIAGRAM_BAR].X;
        rect.LowerRightCorner.X += SpriteSizes[SPRITE_DIAGRAM_BAR].X;
    }
}

void CStatisticsScreen::submitHighscores()
{
    if (!game::CHighscoreInfo::getCurrentHighscoreInfo())
        return;
    if (!Connection)
    {
        Connection = new ox::net::CHTTPConnectionHandler();
        Connection->init(Device, this);
    }
    ox::core::CString<char> file = "/highscores/harvest/submitHighScores.php";
    file += "?s=";
    file += game::CHighscoreInfo::getCurrentHighscoreInfo()->getHighscoreString();
    file += "&v=";
    file += "3";
    file += "&i=";
    file += "drm";
    Connection->doGet(file, L"www.oxeyegames.com", 80);
}

void CStatisticsScreen::addInfoString(const ox::core::CString<wchar_t>& text)
{
    if (!InfoPanel)
        return;
    int index = (int)InfoStrings.size();
    int y = index % INFO_PAGE_LINES * 15;
    ox::gui::IGUIStaticText* line = GUIEnvironment->addStaticText(text.c_str(),
        ox::core::CRect<int>(35, y + 144, 695, y + 159), false, false, InfoPanel, -1, 0);
    if (index & 1)
        line->setOverrideColor(ox::video::SColor(0xffd0d0e0));
    if (index < InfoScroll || index >= InfoScroll + INFO_PAGE_LINES)
        line->setVisible(false);
    InfoStrings.push_back(line);
}

void CStatisticsScreen::loadSprites()
{
    if (SpritesLoaded)
        return;
    ox::video::ISpritePackage* package =
        Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true);
    if (package)
    {
        for (int i = 0; i < SPRITE_COUNT; ++i)
        {
            Sprites[i] = package->addNewAnimationState(STAT_SPRITE_NAMES[i]);
            if (Sprites[i])
                SpriteSizes[i] = Sprites[i]->getFrameSize(0);
        }
    }
    SpritesLoaded = true;
}

bool CStatisticsScreen::isVisible()
{
    if (Window)
        return Window->isVisible();
    return false;
}

void CStatisticsScreen::update(float time)
{
    if (game::gp_statistics && !Graphs.empty())
    {
        float fade = 15.0f * time;
        Graphs[0]->Alpha += fade;
        for (unsigned int i = 1; i < Graphs.size(); ++i)
        {
            if (Graphs[i - 1]->Alpha < 2.0f)
                Graphs[i]->Alpha = ox::core::clamp(Graphs[i]->Alpha + time * 10.0f, 0.0f,
                    Graphs[i - 1]->Alpha * 0.5f);
            else
                Graphs[i]->Alpha += fade;
        }
    }
}

} // end namespace gui
} // end namespace harvest
