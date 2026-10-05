// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_ENTITY_CCONSTRUCTIONENTITY_H
#define HARVEST_ENTITY_CCONSTRUCTIONENTITY_H

#include "CBuildingEntity.h"
#include "ox/core/CBasic.h"

namespace harvest {
namespace entity {

//! A construction site. Sparks build it up; when it has all it needs it is replaced by its building.
class CConstructionEntity : public CBuildingEntity
{
public:
    //! buildingId names the buildable item; without one the site is set up by readEntityData.
    CConstructionEntity(float x, float y, const char* buildingId);
    virtual ~CConstructionEntity();

    virtual void writeEntityData(ox::io::IWriteFile* file);
    virtual void readEntityData(ox::io::IReadFile* file, int version);
    virtual void updateSprite(float frameDelta);
    //! Done when the building has replaced it.
    virtual int updateLogic(float frameDelta);
    virtual void render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort);
    virtual void renderGroundLayer(const ox::core::CPosition2d<float>& camera,
        const ox::core::CRect<int>& viewPort) {}
    virtual ox::video::ISpriteAnimationState* getCurrentDisplaySprite();

    //! Takes a spark while it needs one, else sends it back.
    virtual int onSpark(CSparkEntity* spark);
    virtual bool wantsSpark();

    virtual ox::core::CString<wchar_t> getInfoString();

    virtual float getCollisionSize() const { return CollisionSize; }
    virtual int getSellValue() const { return MineralCost; }

    //! Calls the free spark movers in range to the site, or releases them again.
    virtual void handleDoubleClickSelection();

    const char* getBuildingId();
    bool haveMoversBeenCalled() const { return MoversCalled; }
    //! Sets how far the construction is, from 0 to 1; a finished site still needs its last spark.
    void setProgress(float progress)
    {
        if (progress >= 1.0f)
            Sparks = SparksNeeded;
        else
            Sparks = ox::core::clamp((int)(SparksNeeded * progress), 0, SparksNeeded - 1);
    }

private:
    //! Takes the entity type, costs, name and sprite from the buildable item.
    void setEntityInfo();
    //! Replaces the sprite; a package file loads a creative building's own sprite package.
    void setSprite(const char* packageFile, const char* spriteName);

    ox::core::CString<char> BuildingId;
    ox::core::CString<wchar_t> Name;
    //! The entity type of the building under construction.
    int EntityType;
    int Sparks;
    int SparksNeeded;
    int MineralCost;
    //! Whether spark movers have been called to the site.
    bool MoversCalled;
    ox::video::ISpriteAnimationState* Sprite;
    float CollisionSize;
};

} // end namespace entity
} // end namespace harvest

#endif
