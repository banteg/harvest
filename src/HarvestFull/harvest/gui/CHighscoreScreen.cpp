// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include <cstdlib>
#include <ctime>
#include "harvest/game/CWorld.h"
#include "CHighscoreScreen.h"
#include "harvest/ECustomEvents.h"
#include "harvest/settings/CHarvestProfile.h"
#include "harvest/settings/CProfileManager.h"
#include "harvest/settings/CSystemConfig.h"
#include "ox/IOxDevice.h"
#include "ox/TArray.h"
#include "ox/algo/CBase64url.h"
#include "ox/core/CRect.h"
#include "ox/core/CStringFunctions.h"
#include "ox/gui/IGUIButton.h"
#include "ox/gui/IGUICheckBox.h"
#include "ox/gui/IGUIEditBox.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIFont.h"
#include "ox/gui/IGUIImage.h"
#include "ox/gui/IGUILayout.h"
#include "ox/gui/IGUIListBox.h"
#include "ox/gui/IGUIModalScreen.h"
#include "ox/gui/IGUIStaticText.h"
#include "ox/io/CHelpIO.h"
#include "ox/io/CMemReadFile.h"
#include "ox/io/CMemWriteFile.h"
#include "ox/net/CHTTPConnectionHandler.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/IVideoDriver.h"

namespace harvest {
namespace gui {

//! Where the status line is centered in the highscore table.
static const ox::core::CRect<int> STATUS_LABEL_AREA(295, 391, 711, 415);

static const char* const HIGHSCORE_SPRITE_NAMES[] =
{
    "HighscoresAllBackground",
    "HighscoresTopBackground",
    "HighscoresTopSelector",
    "PromoteBackground",
    "PromoteScore",
    "PromoteMinerals",
    "PromoteModeBackground",
    "TinyPlanetIcon1",
    "TinyPlanetIcon2",
    "TinyPlanetIcon3",
    "TinyPlanetTotal",
    "PlanetIcon1",
    "PlanetIcon2",
    "PlanetIcon3",
    "PlanetIconTotal",
    "ModeBtnSmallNormal",
    "ModeBtnSmallWave",
    "ModeBtnSmallInsane",
    "ModeBtnSmallRush",
    "ModeIconNormal",
    "ModeIconWave",
    "ModeIconInsane",
    "ModeIconRush",
    "BtnPromoteNormal"
};

//! Indices into HIGHSCORE_SPRITE_NAMES.
enum
{
    SPRITE_ALL_BACKGROUND,
    SPRITE_TOP_BACKGROUND,
    SPRITE_TOP_SELECTOR,
    SPRITE_PROMOTE_BACKGROUND,
    SPRITE_PROMOTE_SCORE,
    SPRITE_PROMOTE_MINERALS,
    SPRITE_PROMOTE_MODE_BACKGROUND,
    SPRITE_TINY_PLANET_ICON,
    SPRITE_PLANET_ICON = SPRITE_TINY_PLANET_ICON + 4,
    SPRITE_MODE_BUTTON = SPRITE_PLANET_ICON + 4,
    SPRITE_MODE_ICON = SPRITE_MODE_BUTTON + 4,
    SPRITE_COUNT = SPRITE_MODE_ICON + 5
};

static const wchar_t* const GAME_MODE_NAMES[] =
{
    L"gamemode:normal",
    L"gamemode:wave",
    L"gamemode:insane",
    L"gamemode:rush",
    L"gamemode:creative"
};

static const wchar_t* const HIGHSCORE_PLANET_NAMES[] =
{
    L"highscores:planets0",
    L"highscores:planets1",
    L"highscores:planets2",
    L"highscores:planets3",
    L"highscores:planets4"
};

static const wchar_t* const HIGHSCORE_SORT_MODES[] =
{
    L"highscores:sort0",
    L"highscores:sort1",
    L"highscores:sort2",
    L"highscores:sort3",
    L"highscores:sort4",
    L"highscores:sort5"
};

static const wchar_t* const HIGHSCORE_TYPE_NAMES[] =
{
    L"highscores:players",
    L"highscores:peers",
    L"highscores:groups",
    L"highscores:table"
};

bool CHighscoreScreen::isVisible()
{
    if (Window)
        return Window->isVisible();
    return false;
}

ox::gui::IGUILayout* CHighscoreScreen::createNewPopupWindow()
{
    if (Popup)
    {
        Popup->remove();
        Popup = 0;
    }
    Popup = GUIEnvironment->addModalScreen();
    return GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 200, 100), Popup, -1);
}

void CHighscoreScreen::sortAndMovePopup(ox::gui::IGUILayout* popup, ox::gui::IGUIElement* button, bool above)
{
    popup->sortRiver(true, 5, 5, false);
    if (above)
        popup->moveTo(ox::core::CPosition2d<int>(button->getAbsolutePosition().UpperLeftCorner.X,
            button->getAbsolutePosition().UpperLeftCorner.Y - 10 - popup->getAbsolutePosition().getHeight()));
    else
        popup->moveTo(ox::core::CPosition2d<int>(button->getAbsolutePosition().UpperLeftCorner.X,
            button->getAbsolutePosition().LowerRightCorner.Y + 10));

    ox::core::CRect<int> rect = popup->getAbsolutePosition();
    if (rect.LowerRightCorner.X > Driver->getScreenSize().Width)
        popup->moveTo(ox::core::CPosition2d<int>(
            rect.UpperLeftCorner.X + Driver->getScreenSize().Width - rect.LowerRightCorner.X,
            rect.UpperLeftCorner.Y));
}

void CHighscoreScreen::createStatusString(const wchar_t* text)
{
    if (StatusText)
    {
        StatusText->remove();
        StatusText = 0;
    }
    if (ListFrame)
    {
        StatusText = GUIEnvironment->addStaticText(text, STATUS_LABEL_AREA, false, true, ListFrame, -1, L"");
        StatusText->packSize();
        ox::core::CRect<int> rect = StatusText->getRelativePosition();
        StatusText->moveTo(ox::core::CPosition2d<int>(
            (STATUS_LABEL_AREA.getWidth() - rect.getWidth()) / 2 + STATUS_LABEL_AREA.UpperLeftCorner.X,
            (STATUS_LABEL_AREA.getHeight() - rect.getHeight()) / 2 + STATUS_LABEL_AREA.UpperLeftCorner.Y));
    }
}

ox::core::CString<wchar_t> CHighscoreScreen::parseRelativeTimeFormat(const ox::core::CString<char>& seconds)
{
    time(0);
    int age = atoi(seconds.c_str());
    if (age < 60)
        return settings::gp_systemConfig->getLocalizedText(L"highscores:secondsAgo", age);
    if (age < 120)
        return settings::gp_systemConfig->getLocalizedText(L"highscores:lastMinute");
    if (age < 3600)
        return settings::gp_systemConfig->getLocalizedText(L"highscores:minutesAgo", age / 60);
    if (age < 7200)
        return settings::gp_systemConfig->getLocalizedText(L"highscores:lastHour");
    if (age < 86400)
        return settings::gp_systemConfig->getLocalizedText(L"highscores:hoursAgo", age / 3600);
    if (age < 172800)
        return settings::gp_systemConfig->getLocalizedText(L"highscores:yesterday");
    if (age < 1209600)
        return settings::gp_systemConfig->getLocalizedText(L"highscores:daysAgo", age / 86400);
    if (age < 31536000)
        return settings::gp_systemConfig->getLocalizedText(L"highscores:weeksAgo", age / 604800);
    if (age < 63072000)
        return settings::gp_systemConfig->getLocalizedText(L"highscores:lastYear");
    return settings::gp_systemConfig->getLocalizedText(L"highscores:yearsAgo", age / 31536000);
}

void CHighscoreScreen::createLoadBlock()
{
    LoadBlock = GUIEnvironment->addModalScreen();
    ox::gui::IGUILayout* frame = GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 30, 30), LoadBlock, -1);
    GUIEnvironment->addStaticText(settings::gp_systemConfig->getLocalizedText(L"highscores:loading").c_str(),
        "br center", frame, 0, -1);
    frame->sortRiver(true, 20, 20, false);
    frame->centerOnParent();
}

ox::core::CString<char> CHighscoreScreen::createBase64ForUCS2(const ox::core::CString<wchar_t>& text)
{
    ox::io::CMemWriteFile* file = new ox::io::CMemWriteFile();
    ox::io::CHelpIO::writeWideString(file, text, true);
    ox::io::CMemReadFile data(file->getData(), file->getSize(), false);
    ox::core::CString<char> result;
    ox::algo::CBase64url::encode(result, &data);
    delete file;
    return ox::core::CString<char>(result);
}

void CHighscoreScreen::loadSprites()
{
    if (SpritesLoaded)
        return;

    ox::video::ISpritePackage* package =
        Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true);
    if (package)
    {
        for (int i = 0; i < SPRITE_COUNT; ++i)
        {
            Sprites[i] = package->addNewAnimationState(HIGHSCORE_SPRITE_NAMES[i]);
            if (Sprites[i])
                SpriteSizes[i] = Sprites[i]->getFrameSize(0);
        }
    }
    SpritesLoaded = true;
}

CHighscoreScreen::CHighscoreScreen(ox::IOxDevice* device)
    : Device(device), Ready(true), GameMode(0), Planet(0), Unknown38(0), SortMode(0), ExactName(false),
      ExactGroup(false), Offset(0), SummaryCategory(0), ShownSummaryCategory(-1), SpritesLoaded(false), Window(0),
      Background(0), TypeFrame(0), ListFrame(0), StatusText(0), SummaryFrame(0), LoadBlock(0), Popup(0),
      RankEditBox(0), FilterEditBox(0), PlanetList(0), Connection(0)
{
    GUIEnvironment = device->getGUIEnvironment();
    Driver = device->getVideoDriver();
    for (int i = 0; i < 20; ++i)
        for (int j = 0; j < 7; ++j)
            Cells[i][j] = 0;
    for (int i = 0; i < SPRITE_COUNT; ++i)
        Sprites[i] = 0;
    for (int i = 0; i < 3; ++i)
        SummaryLoaded[i] = false;
    // Clears past the end of each row, as in both builds.
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            for (int k = 0; k < 4; ++k)
                PromoteButtons[i][j][k] = 0;
    for (int i = 0; i < 4; ++i)
        TypeButtons[i] = 0;
}

CHighscoreScreen::~CHighscoreScreen()
{
    if (Popup)
        Popup->remove();
    if (Window)
        Window->remove();
    if (LoadBlock)
    {
        LoadBlock->remove();
        LoadBlock = 0;
    }
    if (Connection)
        delete Connection;
    for (int i = 0; i < SPRITE_COUNT; ++i)
        if (Sprites[i])
            Sprites[i]->remove();
}

void CHighscoreScreen::createSummaryPage()
{
    if (ShownSummaryCategory == SummaryCategory)
    {
        SummaryFrame->setVisible(true);
        ListFrame->setVisible(false);
        SelectedType = ID_TYPE_BUTTON + ShownSummaryCategory;
        return;
    }

    ShownSummaryCategory = SummaryCategory;
    SelectedType = ID_TYPE_BUTTON + SummaryCategory;
    SummaryFrame->removeAllChildren();

    const int PLAYET_Y_POS[4] = {77, 114, 151, 40};
    ox::gui::IGUIFont* font = GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/smallFont.fnt");
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            for (int k = 0; k < 4; ++k)
            {
                PromoteButtons[i][j][k] = 0;
                if (SummaryPages[ShownSummaryCategory].Entries[i][j][k].Name.size() > 0)
                {
                    int x = 131 + i * 157;
                    int y = 22 + j * 195;
                    ox::gui::IGUIButton* button = GUIEnvironment->addButton(
                        ox::core::CRect<int>(x, y + PLAYET_Y_POS[k], x + 50, y + PLAYET_Y_POS[k] + 30), SummaryFrame,
                        ID_PROMOTE + i * 8 + j * 4 + k, 0);
                    button->setAnimations(
                        Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true),
                        "BtnPromote", true);
                    button->setReportOnDraw(2);
                    PromoteButtons[i][j][k] = button;

                    ox::core::CRect<int> textArea(33, 4, 125, 29);
                    ox::core::CString<wchar_t> text = SummaryPages[ShownSummaryCategory].Entries[i][j][k].Name;
                    text.append(L"\n(");
                    text.append(SummaryPages[ShownSummaryCategory].Entries[i][j][k].Score);
                    text.append(L")");
                    ox::gui::IGUIStaticText* label =
                        GUIEnvironment->addStaticText(text.c_str(), textArea, false, true, button, -1, L"");
                    label->setOverrideFont(font);
                    label->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_TOP);
                }
            }
        }
    }
    SummaryFrame->setVisible(true);
    ListFrame->setVisible(false);
}

ox::core::CString<wchar_t> CHighscoreScreen::createUCS2FromBase64UTF2(const ox::core::CString<char>& text)
{
    ox::io::CMemWriteFile* file = new ox::io::CMemWriteFile();
    ox::algo::CBase64url::decode(file, text);
    ox::core::CString<wchar_t> result = L"";
    for (int i = 0; i < file->getSize(); ++i)
    {
        // UTF-8 sequences of up to three bytes, decoded to UCS-2
        if (((unsigned char)file->getData()[i] & 0xe0) == 0xc0 && i < file->getSize() - 1)
        {
            result.append((wchar_t)(unsigned short)(((unsigned char)file->getData()[i + 1] & 0x3f) +
                (((unsigned char)file->getData()[i] & 0x1f) << 6)));
            ++i;
        }
        else if (((unsigned char)file->getData()[i] & 0xf0) == 0xe0 && i < file->getSize() - 2)
        {
            result.append((wchar_t)(unsigned short)(((unsigned char)file->getData()[i + 2] & 0x3f) +
                (((unsigned char)file->getData()[i + 1] & 0x3f) << 6) +
                (((unsigned char)file->getData()[i] & 0xf) << 12)));
            i += 2;
        }
        else if (file->getData()[i])
            result.append((wchar_t)(unsigned char)file->getData()[i]);
    }
    delete file;
    return result;
}

void CHighscoreScreen::loadHighscores(int offset)
{
    if (!Ready)
        return;

    Ready = false;
    if (LoadBlock)
    {
        LoadBlock->remove();
        LoadBlock = 0;
    }
    createLoadBlock();
    if (!Connection)
    {
        Connection = new ox::net::CHTTPConnectionHandler();
        Connection->init(Device, this);
    }
    Offset = offset;

    ox::core::CString<char> url = "/highscores/harvest/getHighScores.php?version=1";
    url.append("&mode=");
    url.append(GameMode);
    url.append("&planet=");
    url.append(Planet);
    url.append("&offset=");
    url.append(offset);
    url.append("&orderMode=");
    url.append(SortMode);
    if (ExactName)
        url.append("&exactName=1");
    if (ExactGroup)
        url.append("&exactGroup=1");
    if (NameFilter.size() > 0)
    {
        url.append("&nameFilter=");
        url.append(createBase64ForUCS2(NameFilter));
    }
    if (GroupFilter.size() > 0)
    {
        url.append("&groupFilter=");
        url.append(createBase64ForUCS2(GroupFilter));
    }
    SummaryMode = false;
    Connection->doGet(url, L"www.oxeyegames.com", 80);
}

void CHighscoreScreen::parseSummaryPage(const ox::core::CString<char>& page)
{
    static ox::core::CString<char> rowDelimiter = "<br>";
    static ox::core::CString<char> colDelimiter = "|";

    ox::TArray<ox::core::CString<char> > rows;
    ox::core::splitString(rows, page, rowDelimiter);
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            for (int k = 0; k < 4; ++k)
            {
                int row = i * 8 + j * 4 + k;
                if (row >= (int)rows.size())
                    continue;

                ox::TArray<ox::core::CString<char> > columns;
                ox::core::splitString(columns, rows[row], colDelimiter);
                if (SummaryCategory != 2 && columns.size() == 3)
                {
                    SummaryPages[SummaryCategory].Entries[i][j][k].Name = createUCS2FromBase64UTF2(columns[0]);
                    SummaryPages[SummaryCategory].Entries[i][j][k].Group = createUCS2FromBase64UTF2(columns[1]);
                    SummaryPages[SummaryCategory].Entries[i][j][k].Score = ox::core::CString<wchar_t>(columns[2].c_str());
                }
                else if (SummaryCategory == 2 && columns.size() == 2)
                {
                    // the groups summary names only the group
                    SummaryPages[SummaryCategory].Entries[i][j][k].Name = createUCS2FromBase64UTF2(columns[0]);
                    SummaryPages[SummaryCategory].Entries[i][j][k].Group = SummaryPages[SummaryCategory].Entries[i][j][k].Name;
                    SummaryPages[SummaryCategory].Entries[i][j][k].Score = ox::core::CString<wchar_t>(columns[1].c_str());
                }
            }
        }
    }
    SummaryLoaded[SummaryCategory] = true;
}


void CHighscoreScreen::update(float frameDelta)
{
    Lock.enter();
    if (Response.size() > 0)
    {
        if (Response == ox::core::CString<char>("Error"))
        {
            Response = "";
            if (LoadBlock)
            {
                LoadBlock->remove();
                LoadBlock = 0;
            }
            Ready = true;
        }
        else
        {
            if (SummaryMode)
                parseSummaryPage(Response);
            else
                parseHighscoreString(Response);
            Response = "";
            Ready = true;
            if (LoadBlock)
            {
                LoadBlock->remove();
                LoadBlock = 0;
            }
            if (SummaryMode)
                createSummaryPage();
        }
    }
    Lock.leave();
}

void CHighscoreScreen::loadSummaryPage(int category)
{
    if (SummaryLoaded[category])
    {
        SummaryCategory = category;
        createSummaryPage();
        return;
    }
    if (!Ready)
        return;

    Ready = false;
    createLoadBlock();
    if (!Connection)
    {
        Connection = new ox::net::CHTTPConnectionHandler();
        Connection->init(Device, this);
    }

    ox::core::CString<char> url = "/highscores/harvest/getSummaryPage.php?version=1";
    url.append("&category=");
    url.append(category);
    ox::core::CString<wchar_t> group = settings::gp_profileManager->getCurrentProfile()->getPlayerGroup();
    if (group.size() > 0)
    {
        url.append("&peerGroup=");
        url.append(createBase64ForUCS2(group));
    }
    SummaryMode = true;
    SummaryCategory = category;
    Connection->doGet(url, L"www.oxeyegames.com", 80);
}

void CHighscoreScreen::setVisible(bool visible)
{
    if (visible)
    {
        if (!Window)
        {
            loadSprites();
            Window = GUIEnvironment->addModalScreen();
            Window->setID(ID_SCREEN);
            Background = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 800, 600), Window);
            TypeFrame = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 5, SpriteSizes[SPRITE_TOP_BACKGROUND].X,
                SpriteSizes[SPRITE_TOP_BACKGROUND].Y + 5), Background);
            TypeFrame->setReportOnDraw(1);
            TypeFrame->setID(ID_TYPE_FRAME);
            TypeFrame->LayoutFlags = "br center";

            int top = (SpriteSizes[SPRITE_TOP_BACKGROUND].Y - SpriteSizes[SPRITE_TOP_SELECTOR].Y) / 2;
            int width = SpriteSizes[SPRITE_TOP_SELECTOR].X;
            int left = (SpriteSizes[SPRITE_TOP_BACKGROUND].X - width * 4) / 2;
            for (int i = 0; i < 4; ++i)
            {
                TypeButtons[i] = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(left, top,
                    SpriteSizes[SPRITE_TOP_SELECTOR].X + left, SpriteSizes[SPRITE_TOP_SELECTOR].Y + top), TypeFrame);
                TypeButtons[i]->setID(ID_TYPE_BUTTON + i);
                TypeButtons[i]->setReportOnDraw(2);
                TypeNames[i] = settings::gp_systemConfig->getLocalizedText(HIGHSCORE_TYPE_NAMES[i]);
                left += width;
            }

            int margin = (800 - SpriteSizes[SPRITE_ALL_BACKGROUND].X) / 2;
            ListFrame = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(margin,
                margin + SpriteSizes[SPRITE_TOP_BACKGROUND].Y + 5, SpriteSizes[SPRITE_ALL_BACKGROUND].X + margin,
                SpriteSizes[SPRITE_ALL_BACKGROUND].Y + margin + SpriteSizes[SPRITE_TOP_BACKGROUND].Y + 5), Background);
            ListFrame->setReportOnDraw(1);
            ListFrame->setID(ID_LIST_FRAME);
            ListFrame->LayoutFlags = "br center";
            ListFrame->setVisible(false);

            int summaryTop = margin + 5 + SpriteSizes[SPRITE_TOP_BACKGROUND].Y;
            int summaryWidth = SpriteSizes[SPRITE_PROMOTE_SCORE].X + SpriteSizes[SPRITE_PROMOTE_BACKGROUND].X;
            int summaryLeft = (800 - summaryWidth) / 2;
            SummaryFrame = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(summaryLeft, summaryTop,
                summaryLeft + summaryWidth, SpriteSizes[SPRITE_ALL_BACKGROUND].Y + summaryTop), Background);
            SummaryFrame->setReportOnDraw(1);
            SummaryFrame->setID(ID_SUMMARY_FRAME);
            SummaryFrame->LayoutFlags = "br center";
            StatusText = 0;

            ox::gui::IGUIButton* back = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), Background, ID_BACK,
                settings::gp_systemConfig->getLocalizedText(L"menu:back").c_str());
            back->LayoutFlags = "br";
            back->setOverrideFont(GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/boldFont.fnt"));
            back->centerOnParent();
            back->moveTo(ox::core::CPosition2d<int>(back->getRelativePosition().UpperLeftCorner.X,
                SpriteSizes[SPRITE_ALL_BACKGROUND].Y + SpriteSizes[SPRITE_TOP_BACKGROUND].Y + margin * 2 + 5));
        }
        Background->centerOnParent();
        Window->setVisible(true);
        loadSummaryPage(0);
    }
    else if (Window)
        Window->setVisible(false);
}

void CHighscoreScreen::parseHighscoreString(const ox::core::CString<char>& page)
{
    static ox::core::CString<char> rowDelimiter = "<br>";
    static ox::core::CString<char> colDelimiter = "|";

    ox::TArray<ox::core::CString<char> > rows;
    ox::core::splitString(rows, page, rowDelimiter);
    ListFrame->removeAllChildren();
    StatusText = 0;
    for (int i = 0; i < 20; ++i)
        for (int j = 0; j < 7; ++j)
            Cells[i][j] = 0;

    const ox::core::CRect<int> TEXT_AREA_COORDS[7] =
    {
        ox::core::CRect<int>(16, 63, 54, 77),
        ox::core::CRect<int>(58, 63, 202, 77),
        ox::core::CRect<int>(206, 63, 276, 77),
        ox::core::CRect<int>(280, 63, 350, 77),
        ox::core::CRect<int>(354, 63, 443, 77),
        ox::core::CRect<int>(447, 63, 591, 77),
        ox::core::CRect<int>(595, 63, 713, 77)
    };
    ox::video::ISpritePackage* package =
        Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true);

    for (int i = 0; i < (int)rows.size() && i < 20; ++i)
    {
        int rowOffset = i * 15;
        ox::TArray<ox::core::CString<char> > columns;
        ox::core::splitString(columns, rows[i], colDelimiter);
        if (columns.size() != 7)
            continue;
        ox::video::SColor color = 0xffffffff;
        if (i == 0 && atoi(columns[0].c_str()) == 1)
            color = 0xffffffc0;

        for (int j = 0; j < 7; ++j)
        {
            ox::core::CString<wchar_t> text = columns[j].c_str();
            if (j == 1 || j == 5)
                text = createUCS2FromBase64UTF2(columns[j]);
            else if (j == 6)
                text = parseRelativeTimeFormat(columns[j]);

            if (j == 4)
            {
                int planet = atoi(columns[j].c_str());
                int sprite = SPRITE_TINY_PLANET_ICON + planet;
                if ((unsigned int)planet > 4)
                    sprite = SPRITE_TINY_PLANET_ICON;
                const ox::core::CPosition2d<int>& size = SpriteSizes[sprite];
                int left = (TEXT_AREA_COORDS[j].getWidth() - size.X) / 2 + TEXT_AREA_COORDS[j].UpperLeftCorner.X;
                int top = (TEXT_AREA_COORDS[j].getHeight() - size.Y) / 2 + TEXT_AREA_COORDS[j].UpperLeftCorner.Y +
                    rowOffset;
                GUIEnvironment->addImage(ox::core::CRect<int>(left, top, left + size.X, top + size.Y), ListFrame, -1,
                    0)->setAnimation(HIGHSCORE_SPRITE_NAMES[sprite], package);
            }
            else
            {
                ox::gui::IGUIStaticText* cell = GUIEnvironment->addStaticText(text.c_str(),
                    ox::core::CRect<int>(TEXT_AREA_COORDS[j].UpperLeftCorner.X,
                        TEXT_AREA_COORDS[j].UpperLeftCorner.Y + rowOffset, TEXT_AREA_COORDS[j].LowerRightCorner.X,
                        TEXT_AREA_COORDS[j].LowerRightCorner.Y + rowOffset),
                    false, false, ListFrame, -1, L"");
                cell->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER);
                cell->setOverrideColor(color);
                Cells[i][j] = cell;
            }
        }
    }

    const char* BUTTON_SPRITE_NAMES[10] = {"BtnOrdinal", "BtnName", "BtnScore", "BtnScore", "BtnPlanets1",
        "BtnName", "BtnUpdate", "BtnScoreModes", "BtnPageUp", "BtnPageDown"};
    const wchar_t* BUTTON_CAPTIONS[10] = {L"Rank", L"Name", L"Score", L"Credits", 0, L"Group", L"Last Update", 0,
        0, 0};
    const int BUTTON_X[10] = {16, 58, 206, 280, 354, 447, 595, 16, 207, 250};
    const int BUTTON_Y[10] = {39, 39, 39, 39, 39, 39, 39, 389, 389, 389};
    const char* PLANET_BTN_SPRITES[5] = {"BtnPlanets1", "BtnPlanets2", "BtnPlanets3", "BtnPlanetsEither",
        "BtnPlanetsTotal"};
    for (int i = 0; i < 10; ++i)
    {
        const char* sprite = BUTTON_SPRITE_NAMES[i];
        if (i == 4)
            sprite = PLANET_BTN_SPRITES[Planet];
        ox::gui::IGUIButton* button = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 10, 10), ListFrame,
            ID_LIST_BUTTON + i, BUTTON_CAPTIONS[i]);
        button->setAnimations(package, sprite, true);
        button->moveTo(ox::core::CPosition2d<int>(BUTTON_X[i], BUTTON_Y[i]));
        if (i == 7)
            button->setReportOnDraw(2);
    }

    ox::core::CString<wchar_t> mode = settings::gp_systemConfig->getLocalizedText(GAME_MODE_NAMES[GameMode]);
    ox::core::CString<wchar_t> planet = settings::gp_systemConfig->getLocalizedText(HIGHSCORE_PLANET_NAMES[Planet]);
    ox::core::CString<wchar_t> sort = settings::gp_systemConfig->getLocalizedText(HIGHSCORE_SORT_MODES[SortMode]);
    if (NameFilter.size() > 0)
    {
        if (GroupFilter.size() > 0)
            createStatusString(settings::gp_systemConfig->getLocalizedText(L"highscores:infoBothFilter",
                mode.c_str(), planet.c_str(), sort.c_str(), NameFilter.c_str(), GroupFilter.c_str()).c_str());
        else
            createStatusString(settings::gp_systemConfig->getLocalizedText(L"highscores:infoNameFilter",
                mode.c_str(), planet.c_str(), sort.c_str(), NameFilter.c_str()).c_str());
    }
    else if (GroupFilter.size() > 0)
        createStatusString(settings::gp_systemConfig->getLocalizedText(L"highscores:infoGroupFilter",
            mode.c_str(), planet.c_str(), sort.c_str(), GroupFilter.c_str()).c_str());
    else
        createStatusString(settings::gp_systemConfig->getLocalizedText(L"highscores:infoNormal",
            mode.c_str(), planet.c_str(), sort.c_str()).c_str());

    SummaryFrame->setVisible(false);
    ListFrame->setVisible(true);
    SelectedType = ID_TYPE_BUTTON + 3;
}

bool CHighscoreScreen::OnEvent(const ox::event::SEvent& event)
{
    bool result = false;
    switch (event.EventType)
    {
    case ox::event::EET_GUI_EVENT:
    {
        int id = event.GUIEvent.Caller->getID();
        switch (event.GUIEvent.EventType)
        {
        case ox::gui::EGET_MODAL_SCREEN_BLOCKED:
            if (id == ID_SCREEN)
                OnEvent(static_cast<ox::gui::IGUIModalScreen*>(event.GUIEvent.Caller)->getLastBlockedEvent());
            result = true;
            break;
        case ox::gui::EGET_ELEMENT_DRAWN:
            switch (id)
            {
            case ID_TYPE_FRAME:
                if (TypeFrame && Sprites[SPRITE_TOP_BACKGROUND])
                    Sprites[SPRITE_TOP_BACKGROUND]->draw(TypeFrame->getAbsolutePosition().UpperLeftCorner, 0,
                        0xffffffff);
                result = true;
                break;
            case ID_TYPE_BUTTON:
            case ID_TYPE_BUTTON + 1:
            case ID_TYPE_BUTTON + 2:
            case ID_TYPE_BUTTON + 3:
                if (Sprites[SPRITE_TOP_SELECTOR])
                {
                    ox::core::CRect<int> rect = event.GUIEvent.Caller->getAbsolutePosition();
                    if (id == SelectedType)
                        Sprites[SPRITE_TOP_SELECTOR]->draw(ox::core::CPosition2d<int>(
                            (rect.UpperLeftCorner.X + rect.LowerRightCorner.X) / 2,
                            (rect.UpperLeftCorner.Y + rect.LowerRightCorner.Y) / 2), 0, 0xffffffff);
                    ox::video::SColor color = 0xc0ffffff;
                    const ox::core::CPosition2d<int>& mouse = GUIEnvironment->getMousePosition();
                    if (rect.UpperLeftCorner.X <= mouse.X && rect.UpperLeftCorner.Y <= mouse.Y &&
                        mouse.X < rect.LowerRightCorner.X && mouse.Y < rect.LowerRightCorner.Y)
                        color = 0xf0ffffff;
                    GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/largeFont.fnt")->draw(
                        TypeNames[id - ID_TYPE_BUTTON].c_str(), rect, color, ox::gui::EFHA_CENTER,
                        ox::gui::EFVA_CENTER, 0);
                }
                result = true;
                break;
            case ID_SUMMARY_FRAME:
                if (SummaryFrame && Sprites[SPRITE_PROMOTE_BACKGROUND])
                {
                    ox::core::CPosition2d<int> position = SummaryFrame->getAbsolutePosition().UpperLeftCorner;
                    Sprites[SPRITE_PROMOTE_SCORE]->draw(ox::core::CPosition2d<int>(position.X, position.Y + 40), 0,
                        0xffffffff);
                    Sprites[SPRITE_PROMOTE_BACKGROUND]->draw(
                        ox::core::CPosition2d<int>(position.X + 139, position.Y + 50), 0, 0xffffffff);
                    Sprites[SPRITE_PROMOTE_MINERALS]->draw(ox::core::CPosition2d<int>(position.X, position.Y + 235),
                        0, 0xffffffff);
                    Sprites[SPRITE_PROMOTE_BACKGROUND]->draw(
                        ox::core::CPosition2d<int>(position.X + 139, position.Y + 245), 0, 0xffffffff);
                    for (int i = 0; i < 4; ++i)
                    {
                        Sprites[SPRITE_PROMOTE_MODE_BACKGROUND]->draw(
                            ox::core::CPosition2d<int>(position.X + 120 + i * 157, position.Y + 40), 0, 0xffffffff);
                        Sprites[SPRITE_PROMOTE_MODE_BACKGROUND]->draw(
                            ox::core::CPosition2d<int>(position.X + 120 + i * 157, position.Y + 235), 0, 0xffffffff);
                        Sprites[SPRITE_MODE_ICON + i]->draw(ox::core::CPosition2d<int>(
                            (SpriteSizes[SPRITE_PROMOTE_MODE_BACKGROUND].X - SpriteSizes[SPRITE_MODE_ICON + i].X) / 2 +
                                position.X + 120 + i * 157,
                            position.Y + 35 - SpriteSizes[SPRITE_MODE_ICON + i].Y), 0, 0xffffffff);
                    }
                }
                result = true;
                break;
            case ID_LIST_FRAME:
                if (Sprites[SPRITE_ALL_BACKGROUND])
                    Sprites[SPRITE_ALL_BACKGROUND]->draw(ListFrame->getAbsolutePosition().UpperLeftCorner, 0,
                        0xffffffff);
                result = true;
                break;
            case ID_LIST_BUTTON + 7:
            {
                int sprite = SPRITE_MODE_BUTTON + GameMode;
                if (Sprites[sprite])
                {
                    ox::core::CRect<int> rect = event.GUIEvent.Caller->getAbsolutePosition();
                    ox::core::CPosition2d<int> position((rect.UpperLeftCorner.X + rect.LowerRightCorner.X) / 2,
                        (rect.UpperLeftCorner.Y + rect.LowerRightCorner.Y) / 2);
                    position.X -= SpriteSizes[sprite].X / 2;
                    position.Y -= SpriteSizes[sprite].Y / 2;
                    Sprites[sprite]->draw(position, 0, 0xffffffff);
                }
                result = true;
                break;
            }
            default:
                if (id >= ID_PROMOTE && id < ID_PROMOTE + 32)
                {
                    Sprites[SPRITE_PLANET_ICON + (id - ID_PROMOTE) % 4]->draw(
                        ox::core::CPosition2d<int>(event.GUIEvent.Caller->getAbsolutePosition().UpperLeftCorner), 0,
                        0xffffffff);
                    result = true;
                }
                break;
            }
            break;
        case ox::gui::EGET_BUTTON_CLICKED:
            switch (id)
            {
            case ID_BACK:
                setVisible(false);
                sendCustomEvent(ECE_CONTINUE_GAME);
                result = true;
                break;
            case ID_LIST_BUTTON:
            {
                ox::gui::IGUILayout* popup = createNewPopupWindow();
                GUIEnvironment->addStaticText(
                    settings::gp_systemConfig->getLocalizedText(L"highscores:jumpToRank").c_str(), "br center",
                    popup, 0, -1);
                RankEditBox = GUIEnvironment->addEditBox(L"", ox::core::CRect<int>(0, 0, 100, 20), true, popup, -1);
                RankEditBox->LayoutFlags = "br left";
                RankEditBox->setMax(7);
                RankEditBox->setAssociatedButton(ID_POPUP_RANK);
                GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 100, 20), popup, ID_POPUP_RANK,
                    settings::gp_systemConfig->getLocalizedText(L"highscores:apply").c_str());
                GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 100, 20), popup, ID_POPUP_CANCEL,
                    settings::gp_systemConfig->getLocalizedText(L"highscores:cancel").c_str())->LayoutFlags =
                    "br right";
                sortAndMovePopup(popup, event.GUIEvent.Caller, false);
                result = true;
                break;
            }
            case ID_LIST_BUTTON + 1:
            {
                ox::gui::IGUILayout* popup = createNewPopupWindow();
                GUIEnvironment->addStaticText(
                    settings::gp_systemConfig->getLocalizedText(L"highscores:filterName").c_str(), "br center",
                    popup, 0, -1);
                FilterEditBox =
                    GUIEnvironment->addEditBox(NameFilter.c_str(), ox::core::CRect<int>(0, 0, 200, 20), true, popup, -1);
                FilterEditBox->LayoutFlags = "br left";
                FilterEditBox->setMax(20);
                FilterEditBox->setAssociatedButton(ID_POPUP_NAME);
                GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 100, 20), popup, ID_POPUP_NAME,
                    settings::gp_systemConfig->getLocalizedText(L"highscores:apply").c_str());
                ExactCheckBox = GUIEnvironment->addCheckBox(ExactName, ox::core::CRect<int>(0, 0, 200, 20), popup, -1,
                    settings::gp_systemConfig->getLocalizedText(L"highscores:exactMatch").c_str());
                ExactCheckBox->LayoutFlags = "br";
                GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 100, 20), popup, ID_POPUP_CANCEL,
                    settings::gp_systemConfig->getLocalizedText(L"highscores:cancel").c_str())->LayoutFlags =
                    "br right";
                sortAndMovePopup(popup, event.GUIEvent.Caller, false);
                result = true;
                break;
            }
            case ID_LIST_BUTTON + 2:
                SortMode = SortMode == 0;
                loadHighscores(0);
                result = true;
                break;
            case ID_LIST_BUTTON + 3:
                SortMode = (SortMode == 2) | 2;
                loadHighscores(0);
                result = true;
                break;
            case ID_LIST_BUTTON + 4:
            {
                ox::gui::IGUILayout* popup = createNewPopupWindow();
                GUIEnvironment->addStaticText(
                    settings::gp_systemConfig->getLocalizedText(L"highscores:selectPlanet").c_str(), "br center",
                    popup, 0, -1);
                PlanetList = GUIEnvironment->addListBox(ox::core::CRect<int>(0, 0, 250, 100), popup, -1, false);
                PlanetList->LayoutFlags = "br left";
                PlanetList->setSelectable(true);
                PlanetList->addTextItem(settings::gp_systemConfig->getLocalizedText(L"highscores:planets0").c_str(), 0,
                    0xffffffff, true, true);
                PlanetList->addTextItem(settings::gp_systemConfig->getLocalizedText(L"highscores:planets1").c_str(), 0,
                    0xffffffff, true, true);
                PlanetList->addTextItem(settings::gp_systemConfig->getLocalizedText(L"highscores:planets2").c_str(), 0,
                    0xffffffff, true, true);
                PlanetList->addTextItem(settings::gp_systemConfig->getLocalizedText(L"highscores:planets3").c_str(), 0,
                    0xffffffff, true, true);
                PlanetList->addTextItem(settings::gp_systemConfig->getLocalizedText(L"highscores:planets4").c_str(), 0,
                    0xffffffff, true, true);
                GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 100, 20), popup, ID_POPUP_CANCEL,
                    settings::gp_systemConfig->getLocalizedText(L"highscores:cancel").c_str())->LayoutFlags =
                    "br right";
                GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 100, 20), popup, ID_POPUP_PLANET,
                    settings::gp_systemConfig->getLocalizedText(L"highscores:apply").c_str());
                sortAndMovePopup(popup, event.GUIEvent.Caller, false);
                result = true;
                break;
            }
            case ID_LIST_BUTTON + 5:
            {
                ox::gui::IGUILayout* popup = createNewPopupWindow();
                GUIEnvironment->addStaticText(
                    settings::gp_systemConfig->getLocalizedText(L"highscores:filterGroup").c_str(), "br center",
                    popup, 0, -1);
                FilterEditBox = GUIEnvironment->addEditBox(GroupFilter.c_str(), ox::core::CRect<int>(0, 0, 200, 20),
                    true, popup, -1);
                FilterEditBox->LayoutFlags = "br left";
                FilterEditBox->setMax(20);
                FilterEditBox->setAssociatedButton(ID_POPUP_GROUP);
                GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 100, 20), popup, ID_POPUP_GROUP,
                    settings::gp_systemConfig->getLocalizedText(L"highscores:apply").c_str());
                ExactCheckBox = GUIEnvironment->addCheckBox(ExactGroup, ox::core::CRect<int>(0, 0, 200, 20), popup,
                    -1, settings::gp_systemConfig->getLocalizedText(L"highscores:exactMatch").c_str());
                ExactCheckBox->LayoutFlags = "br";
                GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 100, 20), popup, ID_POPUP_CANCEL,
                    settings::gp_systemConfig->getLocalizedText(L"highscores:cancel").c_str())->LayoutFlags =
                    "br right";
                sortAndMovePopup(popup, event.GUIEvent.Caller, false);
                result = true;
                break;
            }
            case ID_LIST_BUTTON + 6:
                SortMode = (SortMode == 4) | 4;
                loadHighscores(0);
                result = true;
                break;
            case ID_LIST_BUTTON + 7:
            {
                ox::gui::IGUILayout* popup = createNewPopupWindow();
                GUIEnvironment->addStaticText(
                    settings::gp_systemConfig->getLocalizedText(L"highscores:selectMode").c_str(), "br center",
                    popup, 0, -1);
                ModeList = GUIEnvironment->addListBox(ox::core::CRect<int>(0, 0, 250, 120), popup, -1, false);
                ModeList->LayoutFlags = "br left";
                ModeList->setSelectable(true);
                for (int i = 0; i < 4; ++i)
                    GUIEnvironment->addImage(ox::core::CRect<int>(0, 0, 40, 40), ModeList->getListParent(), -1, 0)
                        ->setAnimation(HIGHSCORE_SPRITE_NAMES[SPRITE_MODE_BUTTON + i],
                            Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true));
                ModeList->sortItems(false);
                GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 100, 20), popup, ID_POPUP_CANCEL,
                    settings::gp_systemConfig->getLocalizedText(L"highscores:cancel").c_str())->LayoutFlags =
                    "br right";
                GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 100, 20), popup, ID_POPUP_MODE,
                    settings::gp_systemConfig->getLocalizedText(L"highscores:apply").c_str());
                sortAndMovePopup(popup, event.GUIEvent.Caller, true);
                result = true;
                break;
            }
            case ID_LIST_BUTTON + 8:
            {
                int offset = Offset - 20;
                if (offset < 0)
                    offset = 0;
                loadHighscores(offset);
                result = true;
                break;
            }
            case ID_LIST_BUTTON + 9:
                loadHighscores(Offset + 20);
                result = true;
                break;
            case ID_POPUP_CANCEL:
                if (Popup)
                {
                    Popup->remove();
                    Popup = 0;
                }
                result = true;
                break;
            case ID_POPUP_RANK:
                if (Popup && RankEditBox)
                {
                    int rank = wcstol(RankEditBox->getText(), 0, 10);
                    Popup->remove();
                    Popup = 0;
                    RankEditBox = 0;
                    int offset = rank - 9;
                    if (offset < 0)
                        offset = 0;
                    loadHighscores(offset);
                }
                result = true;
                break;
            case ID_POPUP_NAME:
                if (Popup && FilterEditBox && ExactCheckBox)
                {
                    NameFilter = FilterEditBox->getText();
                    ExactName = ExactCheckBox->isChecked();
                    Popup->remove();
                    Popup = 0;
                    FilterEditBox = 0;
                    ExactCheckBox = 0;
                    loadHighscores(0);
                }
                result = true;
                break;
            case ID_POPUP_GROUP:
                if (Popup && FilterEditBox && ExactCheckBox)
                {
                    GroupFilter = FilterEditBox->getText();
                    ExactGroup = ExactCheckBox->isChecked();
                    Popup->remove();
                    Popup = 0;
                    FilterEditBox = 0;
                    ExactCheckBox = 0;
                    loadHighscores(0);
                }
                result = true;
                break;
            case ID_POPUP_PLANET:
                if (Popup && PlanetList)
                {
                    Planet = PlanetList->getSelected();
                    Popup->remove();
                    Popup = 0;
                    PlanetList = 0;
                    loadHighscores(0);
                }
                result = true;
                break;
            case ID_POPUP_MODE:
                if (Popup && ModeList)
                {
                    GameMode = ModeList->getSelected();
                    if (GameMode > 3)
                        GameMode = 3;
                    else if (GameMode < 0)
                        GameMode = 0;
                    Popup->remove();
                    Popup = 0;
                    ModeList = 0;
                    loadHighscores(0);
                }
                result = true;
                break;
            default:
                if (id >= ID_PROMOTE && id < ID_PROMOTE + 32)
                {
                    int index = id - ID_PROMOTE;
                    Planet = index % 4;
                    if (Planet == 3)
                        Planet = 4;
                    int scoreType = index / 4 % 2;
                    SortMode = scoreType ? 2 : 0;
                    NameFilter = L"";
                    ExactName = false;
                    GameMode = index / 8;
                    if (ShownSummaryCategory)
                    {
                        GroupFilter = SummaryPages[ShownSummaryCategory].Entries[GameMode][scoreType][Planet].Group;
                        ExactGroup = true;
                    }
                    else
                    {
                        GroupFilter = L"";
                        ExactGroup = false;
                    }
                    loadHighscores(0);
                }
                break;
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
            for (int i = 0; i < 4; ++i)
            {
                if (TypeButtons[i] && TypeButtons[i]->getAbsolutePosition().isPointInside(mouse))
                {
                    if (i == 3)
                        loadHighscores(0);
                    else
                        loadSummaryPage(i);
                    break;
                }
            }
            result = true;
            break;
        case ox::event::EMIE_LMOUSE_LEFT_UP:
            result = true;
            break;
        default:
            break;
        }
        break;
    }
    case ox::event::EET_NETWORK_EVENT:
        if (event.NetworkEvent.Type == ox::event::ENET_HTTP_DONE)
        {
            Lock.enter();
            Response = event.NetworkEvent.Data;
            Lock.leave();
            result = true;
        }
        else if (event.NetworkEvent.Type == ox::event::ENET_HTTP_ERROR)
        {
            Lock.enter();
            Response = "Error";
            Lock.leave();
            result = true;
        }
        break;
    default:
        break;
    }
    return result;
}

} // end namespace gui
} // end namespace harvest
