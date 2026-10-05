// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef HARVEST_ENTITY_CBUILDABLEITEMS_H
#define HARVEST_ENTITY_CBUILDABLEITEMS_H

#include "ox/TArray.h"
#include "ox/core/CHiddenInt.h"
#include "ox/entity/COxEntity.h"
#include "ox/core/CString.h"
#include "ox/core/CRect.h"

namespace ox {
class IOxDevice;
namespace gui { class IGUILayout; }
namespace video {
class ISpriteAnimationState;
class IVideoDriver;
} // end namespace video
} // end namespace ox

namespace harvest {
namespace entity {

class CEntity;
//! A buildable building. Partial: the layout between the recovered members is not known yet.
struct SBuildingInfoItem
{
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
    unsigned char Unrecovered3[0x90 - 0x6c];
    //! The sprite package of a creative building.
    ox::core::CString<char> SpritePackage;
    //! The animation of the finished creative building.
    ox::core::CString<char> BuildingSpriteName;
    const char* SpriteName;
};

//! The buildings the player can build, standard and creative.
class CBuildableItems
{
public:
    CBuildableItems();
    virtual ~CBuildableItems();
    const char* getEntityIdForEntityInstance(CEntity* entity);
    int getIndexForEntityType(int entityType);
    int getIndexForEntityId(const char* entityId);
    SBuildingInfoItem* getBuildingInfo(int index);
    //! The building id ("SPARKPRODUCER", a creative building's name) of an item.
    const char* getEntityId(int index);
    SBuildingInfoItem* getBuildingInfoByEntityId(const char* entityId);
    int getEntityType(int index);
    int getEntityMineralCost(int index);
    //! The collision radius of an item's building.
    float getEntityRadius(int index);
    //! The next or previous enabled building after index, wrapping around.
    int changeConstructionSelection(int index, bool next);
    void loadStandardBuildings(ox::video::IVideoDriver* driver);
    void loadCreativeBuildings(ox::IOxDevice* device);
    int getNumBuildings();
    ox::video::ISpriteAnimationState* getPreviewSprite(int index);
    bool isPointWithinButton(int index, ox::core::CPosition2d<int> position);
    void renderButtonLayouts(const ox::core::CPosition2d<int>& offset, int selected, ox::core::CRect<int> clip);
    //! Places the item's build button in a layout.
    void setItemButtonLayout(int index, ox::gui::IGUILayout* layout);
    void addSpecialUpgrade(const char* entityId, const char* upgradeId, const char* name, int cost,
        int count, float value, const char* description);

private:
    // Not recovered yet.
    bool Unknown8;
    bool Unknown9;
    ox::TArray<SBuildingInfoItem*> Items;
};

extern CBuildableItems* gp_buildableItems;

} // end namespace entity
} // end namespace harvest

#endif
