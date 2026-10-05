// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CBuildableItems.h"
#include "harvest/entity/CConstructionEntity.h"
#include "harvest/entity/CCreativeEntity.h"
#include "harvest/settings/CSystemConfig.h"
#include "ox/IOxDevice.h"
#include "ox/game/CConfigBlock.h"
#include "ox/game/CTextLocalization.h"
#include "ox/gui/IGUILayout.h"
#include "ox/io/IFileList.h"
#include "ox/io/IFileSystem.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/IVideoDriver.h"

namespace harvest {
namespace entity {

CBuildableItems* gp_buildableItems = 0;

CBuildableItems::CBuildableItems()
    : StandardBuildingsLoaded(false), CreativeBuildingsLoaded(false)
{
    gp_buildableItems = this;
}

CBuildableItems::~CBuildableItems()
{
    eraseAll();
    gp_buildableItems = 0;
}

void CBuildableItems::eraseAll()
{
    for (unsigned int i = 0; i < Items.size(); ++i)
    {
        for (int j = 0; j < SBuildingInfoItem::SPRITE_COUNT; ++j)
            if (Items[i]->Sprites[j])
                Items[i]->Sprites[j]->remove();
        delete Items[i];
    }
    Items.clear();
}

void CBuildableItems::loadStandardBuildings(ox::video::IVideoDriver* driver)
{
    if (StandardBuildingsLoaded)
        return;

    // The descriptions name the hotkey of each building.
    addBuilding(driver, "$GAME_RESOURCES$/harvestClientData/gfx/ingame.dat", "SPARKPRODUCER", true, 0, "",
        "BoxSparkProducer", settings::gp_systemConfig->getLocalizedText(L"build:solar").c_str(),
        settings::gp_systemConfig->getLocalizedText(L"buildpopups:solar", L"S").c_str(), 50, 15, 25.0f,
        "BuildSparkProducer", "SparkProducer");
    addBuilding(driver, "$GAME_RESOURCES$/harvestClientData/gfx/ingame.dat", "SPARKMOVER", true, 1, "",
        "BoxSparkMover", settings::gp_systemConfig->getLocalizedText(L"build:energy").c_str(),
        settings::gp_systemConfig->getLocalizedText(L"buildpopups:energy", L"E").c_str(), 2, 5, 8.0f,
        "BuildSparkMover", "SparkMover");
    addBuilding(driver, "$GAME_RESOURCES$/harvestClientData/gfx/ingame.dat", "HARVESTER", true, 4, "",
        "BoxMineralGatherer", settings::gp_systemConfig->getLocalizedText(L"build:harvester").c_str(),
        settings::gp_systemConfig->getLocalizedText(L"buildpopups:harvester", L"R").c_str(), 5, 10, 15.0f,
        "BuildMineralGatherer", "MineralGatherer");
    addBuilding(driver, "$GAME_RESOURCES$/harvestClientData/gfx/ingame.dat", "DEFENSETOWER", true, 7, "",
        "BoxDefenceTower", settings::gp_systemConfig->getLocalizedText(L"build:defense").c_str(),
        settings::gp_systemConfig->getLocalizedText(L"buildpopups:defense", L"D").c_str(), 15, 10, 20.0f,
        "BuildDefenseTower", "DefenceTower0000");
    addBuilding(driver, "$GAME_RESOURCES$/harvestClientData/gfx/ingame.dat", "MISSILETURRET", true, 8, "",
        "BoxMissileTower", settings::gp_systemConfig->getLocalizedText(L"build:missileTurret").c_str(),
        settings::gp_systemConfig->getLocalizedText(L"buildpopups:missileTurret", L"T").c_str(), 30, 10, 18.0f,
        "BuildMissileTower", "MissileTower");
    addBuilding(driver, "$GAME_RESOURCES$/harvestClientData/gfx/ingame.dat", "EAGLEUPGRADE", false, 13, "",
        "BoxEagle", settings::gp_systemConfig->getLocalizedText(L"build:eagle").c_str(),
        settings::gp_systemConfig->getLocalizedText(L"buildpopups:eagle", L"C").c_str(), 50, 100, 18.0f,
        "BuildMissileTower", "MissileTower");
    addBuilding(driver, "$GAME_RESOURCES$/harvestClientData/gfx/ingame.dat", "TEMPESTUPGRADE", false, 14, "",
        "BoxTempest", settings::gp_systemConfig->getLocalizedText(L"build:tempest").c_str(),
        settings::gp_systemConfig->getLocalizedText(L"buildpopups:tempest", L"V").c_str(), 40, 100, 18.0f,
        "BuildMissileTower", "MissileTower");

    StandardBuildingsLoaded = true;
}

void CBuildableItems::addBuilding(ox::video::IVideoDriver* driver, const char* spritePackage, const char* entityId,
    bool enabled, int entityType, const char* buildingSprite, const char* constructionSprite, const wchar_t* name,
    const wchar_t* description, int mineralCost, int sparkCost, float collisionSize, const char* buttonSprite,
    const char* previewSprite)
{
    ox::video::ISpritePackage* package = 0;
    if (driver)
        package = driver->getSpritePackage(spritePackage, false);

    SBuildingInfoItem* item = new SBuildingInfoItem;
    item->EntityId = entityId;
    item->Enabled = enabled;
    item->EntityType = entityType;
    item->Name = name;
    item->Description = description;
    item->CollisionSize = collisionSize;
    item->SparkCost.setValue(sparkCost);
    item->MineralCost.setValue(mineralCost);
    item->ButtonLayout = 0;
    item->SpritePackage = spritePackage;
    item->BuildingSpriteName = buildingSprite;
    item->SpriteName = constructionSprite;
    for (int i = 0; i < SBuildingInfoItem::SPRITE_COUNT; ++i)
        item->Sprites[i] = 0;

    if (package)
    {
        ox::core::CString<char> animation;
        animation = buttonSprite;
        animation.append(ox::core::CString<char>("Normal"));
        item->Sprites[SBuildingInfoItem::SPRITE_BUTTON_NORMAL] = package->addNewAnimationState(animation);
        animation = buttonSprite;
        animation.append(ox::core::CString<char>("Highlighted"));
        item->Sprites[SBuildingInfoItem::SPRITE_BUTTON_HIGHLIGHTED] = package->addNewAnimationState(animation);
        animation = buttonSprite;
        animation.append(ox::core::CString<char>("CheckedNormal"));
        item->Sprites[SBuildingInfoItem::SPRITE_BUTTON_CHECKED] = package->addNewAnimationState(animation);
        item->Sprites[SBuildingInfoItem::SPRITE_PREVIEW] =
            package->addNewAnimationState(ox::core::CString<char>(previewSprite));
    }

    Items.push_back(item);
}

void CBuildableItems::loadCreativeBuildings(ox::IOxDevice* device)
{
    if (CreativeBuildingsLoaded)
        return;

    ox::io::IFileSystem* fileSystem = device->getFileSystem();
    ox::video::IVideoDriver* driver = device->getVideoDriver();

    ox::io::IFileList* buildingFiles = fileSystem->createFileList("*.txt",
        "$GAME_RESOURCES$/harvestClientData/sandbox/", (ox::io::EFileList)1);
    ox::io::IFileList* textFiles = fileSystem->createFileList("*.cfg",
        "$GAME_RESOURCES$/harvestClientData/sandbox/", (ox::io::EFileList)1);
    loadCreativeBuildingList(fileSystem, driver, "$GAME_RESOURCES$/harvestClientData/sandbox/", buildingFiles,
        textFiles);
    buildingFiles->drop();
    textFiles->drop();

    buildingFiles = fileSystem->createFileList("*.txt", "$HARVEST_USERDATA$/sandbox", (ox::io::EFileList)1);
    textFiles = fileSystem->createFileList("*.cfg", "$HARVEST_USERDATA$/sandbox", (ox::io::EFileList)1);
    loadCreativeBuildingList(fileSystem, driver, "$HARVEST_USERDATA$/sandbox", buildingFiles, textFiles);
    buildingFiles->drop();
    textFiles->drop();

    CreativeBuildingsLoaded = true;
}

void CBuildableItems::loadCreativeBuildingList(ox::io::IFileSystem* fileSystem, ox::video::IVideoDriver* driver,
    const char* directory, ox::io::IFileList* buildingFiles, ox::io::IFileList* textFiles)
{
    ox::core::CString<char> language = settings::gp_systemConfig->getCurrentLanguageName().c_str();

    // The text files of the current language, named like "english.cfg".
    std::vector<ox::game::CTextLocalization*> localizations;
    for (int i = 0; i < textFiles->getFileCount(); ++i)
    {
        ox::core::CString<char> filename = textFiles->getFileName(i);
        if (filename.startsWith(language))
        {
            ox::game::CTextLocalization* localization = new ox::game::CTextLocalization(fileSystem);
            ox::core::CString<char> path = directory;
            path.append(filename);
            if (localization->read(path.c_str()) == true)
                localizations.push_back(localization);
            else if (localization)
                delete localization;
        }
    }
    unsigned int localizationCount = localizations.size();

    for (int i = 0; i < buildingFiles->getFileCount(); ++i)
    {
        ox::game::CConfiguration* config = new ox::game::CConfiguration(fileSystem);
        ox::core::CString<char> path = directory;
        path.append(ox::core::CString<char>(buildingFiles->getFileName(i)));
        if (config->read(path.c_str()) == true)
        {
            std::vector<ox::game::CConfigBlock*> blocks = config->getBlocks();
            for (unsigned int j = 0; j < blocks.size(); ++j)
            {
                ox::game::CConfigBlock* block = blocks[j];
                ox::core::CString<wchar_t> blockName = block->getBlockName();
                // A block missing an attribute is skipped; the file name was meant for an error message.
                if (!block->attributeExists(L"name") || !block->attributeExists(L"description") ||
                    !block->attributeExists(L"package") || !block->attributeExists(L"buildingSprite") ||
                    !block->attributeExists(L"buttonSprite") || !block->attributeExists(L"constructionSprite") ||
                    !block->attributeExists(L"radius") || !block->attributeExists(L"minerals") ||
                    !block->attributeExists(L"energy") || !block->attributeExists(L"radius"))
                    buildingFiles->getFileName(i);
                else
                {
                    ox::core::CString<wchar_t> value;
                    block->getAttribute(L"package", value);
                    ox::core::CString<char> package = "$GAME_RESOURCES$/harvestClientData/sandbox/";
                    package.append(ox::core::CString<char>(value.c_str()));
                    block->getAttribute(L"buildingSprite", value);
                    ox::core::CString<char> buildingSprite = value.c_str();
                    block->getAttribute(L"constructionSprite", value);
                    ox::core::CString<char> constructionSprite = value.c_str();
                    block->getAttribute(L"buttonSprite", value);
                    ox::core::CString<char> buttonSprite = value.c_str();

                    // The name and description from the language files win over the building file.
                    ox::core::CString<wchar_t> name;
                    ox::core::CString<wchar_t> description;
                    block->getAttribute(L"name", name);
                    block->getAttribute(L"description", description);
                    for (unsigned int k = 0; k < localizationCount; ++k)
                    {
                        ox::game::CConfigBlock* text = localizations[k]->getBlock(blockName);
                        if (text && text->attributeExists(L"name") == true &&
                            text->attributeExists(L"description") == true)
                        {
                            text->getAttribute(L"name", name);
                            text->getAttribute(L"description", description);
                            break;
                        }
                    }

                    int minerals = block->getAttributeAsInt(L"minerals");
                    int energy = block->getAttributeAsInt(L"energy");
                    float radius = block->getAttributeAsFloat(L"radius");
                    addBuilding(driver, package.c_str(), ox::core::CString<char>(blockName.c_str()).c_str(), true,
                        16, buildingSprite.c_str(), constructionSprite.c_str(), name.c_str(), description.c_str(),
                        minerals, energy, radius, buttonSprite.c_str(), buildingSprite.c_str());
                }
            }
        }
        if (config)
            delete config;
    }

    for (unsigned int i = 0; i < localizationCount; ++i)
        if (localizations[i])
            delete localizations[i];
}

void CBuildableItems::setItemButtonLayout(int index, ox::gui::IGUILayout* layout)
{
    if (index >= 0 && index < (int)Items.size())
        Items[index]->ButtonLayout = layout;
}

void CBuildableItems::renderButtonLayouts(const ox::core::CPosition2d<int>& mouse, int selected,
    ox::core::CRect<int> clip)
{
    bool mouseInside = clip.UpperLeftCorner.X <= mouse.X && clip.UpperLeftCorner.Y <= mouse.Y &&
        clip.LowerRightCorner.X > mouse.X && clip.LowerRightCorner.Y > mouse.Y;
    for (int i = 0; i < (int)Items.size(); ++i)
    {
        SBuildingInfoItem* item = Items[i];
        if (!item->ButtonLayout || !item->Enabled)
            continue;

        ox::core::CRect<int> button = item->ButtonLayout->getAbsolutePosition();
        ox::core::CPosition2d<int> center = button.getCenter();
        center.Y -= 2;
        if (i == selected && item->Sprites[SBuildingInfoItem::SPRITE_BUTTON_CHECKED])
            item->Sprites[SBuildingInfoItem::SPRITE_BUTTON_CHECKED]->draw(center, &clip,
                ox::video::SColor(0xffffffff));
        else if (mouseInside && button.isPointInside(mouse) &&
            item->Sprites[SBuildingInfoItem::SPRITE_BUTTON_HIGHLIGHTED])
            item->Sprites[SBuildingInfoItem::SPRITE_BUTTON_HIGHLIGHTED]->draw(center, &clip,
                ox::video::SColor(0xffffffff));
        else if (item->Sprites[SBuildingInfoItem::SPRITE_BUTTON_NORMAL])
            item->Sprites[SBuildingInfoItem::SPRITE_BUTTON_NORMAL]->draw(center, &clip,
                ox::video::SColor(0xffffffff));
    }
}

int CBuildableItems::changeConstructionSelection(int index, bool forward)
{
    if (forward)
    {
        ++index;
        int count = Items.size();
        for (;; ++index)
        {
            if (index >= count)
                index = 0;
            if (Items[index]->ButtonLayout && Items[index]->Enabled)
                return index;
        }
    }
    --index;
    while (true)
    {
        if (index < 0)
            index = Items.size() - 1;
        else
        {
            if (Items[index]->ButtonLayout && Items[index]->Enabled)
                return index;
            --index;
        }
    }
}

void CBuildableItems::addSpecialUpgrade(const char* entityId, const char* name, const char* description,
    int mineralCost, int sparkCost, float collisionSize, const char* sprite)
{
    ox::core::CString<wchar_t> wideName = name;
    ox::core::CString<wchar_t> wideDescription = description;
    addBuilding(0, "$GAME_RESOURCES$/harvestClientData/gfx/ingame.dat", entityId, false, 16, sprite,
        "BoxSparkProducer", wideName.c_str(), wideDescription.c_str(), mineralCost, sparkCost, collisionSize, "",
        sprite);
}

int CBuildableItems::getIndexForEntityType(int entityType)
{
    for (int i = 0; i < (int)Items.size(); ++i)
        if (Items[i]->EntityType == entityType)
            return i;
    return 0;
}

int CBuildableItems::getIndexForEntityId(const char* entityId)
{
    for (int i = 0; i < (int)Items.size(); ++i)
        if (Items[i]->EntityId == ox::core::CString<char>(entityId))
            return i;
    return -1;
}

SBuildingInfoItem* CBuildableItems::getBuildingInfoByEntityId(const char* entityId)
{
    for (int i = 0; i < (int)Items.size(); ++i)
        if (Items[i]->EntityId == ox::core::CString<char>(entityId))
            return Items[i];
    return 0;
}

int CBuildableItems::getNumBuildings()
{
    return Items.size();
}

SBuildingInfoItem* CBuildableItems::getBuildingInfo(int index)
{
    if (index >= 0 && index < (int)Items.size())
        return Items[index];
    return 0;
}

ox::video::ISpriteAnimationState* CBuildableItems::getPreviewSprite(int index)
{
    if (index >= 0 && index < (int)Items.size())
        return Items[index]->Sprites[SBuildingInfoItem::SPRITE_PREVIEW];
    return 0;
}

const char* CBuildableItems::getEntityId(int index)
{
    if (index >= 0 && index < (int)Items.size())
        return Items[index]->EntityId.c_str();
    return 0;
}

const char* CBuildableItems::getEntityIdForEntityInstance(CEntity* entity)
{
    int index = getIndexForEntityType(entity->getEntityType());
    if (entity->getEntityType() == 3)
        return ((CConstructionEntity*)entity)->getBuildingId();
    if (entity->getEntityType() == 16)
        return ((CCreativeEntity*)entity)->getBuildingId();
    if (index < 0)
        return "";
    return getEntityId(index);
}

int CBuildableItems::getEntityType(int index)
{
    if (index >= 0 && index < (int)Items.size())
        return Items[index]->EntityType;
    return 0;
}

int CBuildableItems::getEntityMineralCost(int index)
{
    if (index >= 0 && index < (int)Items.size())
        return Items[index]->MineralCost.getValue();
    return 0;
}

float CBuildableItems::getEntityRadius(int index)
{
    if (index >= 0 && index < (int)Items.size())
        return Items[index]->CollisionSize;
    return 1.0f;
}

bool CBuildableItems::isPointWithinButton(int index, ox::core::CPosition2d<int> position)
{
    if (index >= 0 && index < (int)Items.size() && Items[index]->ButtonLayout)
        return Items[index]->ButtonLayout->getAbsolutePosition().isPointInside(position);
    return false;
}

} // end namespace entity
} // end namespace harvest
