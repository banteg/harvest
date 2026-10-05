// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_ENTITY_CBUILDABLEITEMS_H
#define HARVEST_ENTITY_CBUILDABLEITEMS_H

#include <vector>
#include "ox/core/CHiddenInt.h"
#include "ox/entity/COxEntity.h"
#include "ox/core/CRect.h"
#include "ox/core/CString.h"

namespace ox {
class IOxDevice;
namespace gui {
class IGUILayout;
} // end namespace gui
namespace io {
class IFileList;
class IFileSystem;
} // end namespace io
namespace video {
class ISpriteAnimationState;
class IVideoDriver;
} // end namespace video
} // end namespace ox

namespace harvest {
namespace entity {

class CEntity;

//! A building the player can build, standard or creative.
struct SBuildingInfoItem
{
    enum
    {
        SPRITE_BUTTON_NORMAL,
        SPRITE_BUTTON_HIGHLIGHTED,
        SPRITE_BUTTON_CHECKED,
        //! The building drawn at the cursor while placing it.
        SPRITE_PREVIEW,
        SPRITE_COUNT
    };

    //! The building id ("SPARKPRODUCER", a creative building's name).
    ox::core::CString<char> EntityId;
    //! Whether the player may build it; scripts switch it with setBuildingEnabled.
    bool Enabled;
    int EntityType;
    ox::core::CString<wchar_t> Name;
    ox::core::CString<wchar_t> Description;
    ox::core::CHiddenInt MineralCost;
    //! Sparks a construction site needs.
    ox::core::CHiddenInt SparkCost;
    float CollisionSize;
    ox::video::ISpriteAnimationState* Sprites[SPRITE_COUNT];
    //! The sprite package of a creative building.
    ox::core::CString<char> SpritePackage;
    //! The animation of the finished creative building.
    ox::core::CString<char> BuildingSpriteName;
    //! The animation of the construction site.
    ox::core::CString<char> SpriteName;
    //! The build menu button the item is drawn on.
    ox::gui::IGUILayout* ButtonLayout;
};

//! The buildings the player can build, standard and creative.
class CBuildableItems
{
public:
    CBuildableItems();
    virtual ~CBuildableItems();

    void eraseAll();
    //! Adds the built-in buildings once.
    void loadStandardBuildings(ox::video::IVideoDriver* driver);
    void addBuilding(ox::video::IVideoDriver* driver, const char* spritePackage, const char* entityId, bool enabled,
        int entityType, const char* buildingSprite, const char* constructionSprite, const wchar_t* name,
        const wchar_t* description, int mineralCost, int sparkCost, float collisionSize, const char* buttonSprite,
        const char* previewSprite);
    //! Adds the creative buildings of the game and user sandbox directories once.
    void loadCreativeBuildings(ox::IOxDevice* device);
    //! Adds the buildings of the .txt files of a directory, named from the .cfg files of the current language.
    void loadCreativeBuildingList(ox::io::IFileSystem* fileSystem, ox::video::IVideoDriver* driver,
        const char* directory, ox::io::IFileList* buildingFiles, ox::io::IFileList* textFiles);
    void setItemButtonLayout(int index, ox::gui::IGUILayout* layout);
    void renderButtonLayouts(const ox::core::CPosition2d<int>& mouse, int selected, ox::core::CRect<int> clip);
    //! The next (or previous) enabled item with a button, wrapping around.
    int changeConstructionSelection(int index, bool forward);
    void addSpecialUpgrade(const char* entityId, const char* name, const char* description, int mineralCost,
        int sparkCost, float collisionSize, const char* sprite);
    int getIndexForEntityType(int entityType);
    int getIndexForEntityId(const char* entityId);
    SBuildingInfoItem* getBuildingInfoByEntityId(const char* entityId);
    int getNumBuildings();
    SBuildingInfoItem* getBuildingInfo(int index);
    ox::video::ISpriteAnimationState* getPreviewSprite(int index);
    //! The building id ("SPARKPRODUCER", a creative building's name) of an item.
    const char* getEntityId(int index);
    const char* getEntityIdForEntityInstance(CEntity* entity);
    int getEntityType(int index);
    int getEntityMineralCost(int index);
    float getEntityRadius(int index);
    bool isPointWithinButton(int index, ox::core::CPosition2d<int> position);

private:
    bool StandardBuildingsLoaded;
    bool CreativeBuildingsLoaded;
    std::vector<SBuildingInfoItem*> Items;
};

extern CBuildableItems* gp_buildableItems;

} // end namespace entity
} // end namespace harvest

#endif
