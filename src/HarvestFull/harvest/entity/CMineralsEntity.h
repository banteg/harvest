// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_ENTITY_CMINERALSENTITY_H
#define HARVEST_ENTITY_CMINERALSENTITY_H

#include "CHarvestEntity.h"
#include "ox/core/CHiddenInt.h"

namespace harvest {
namespace game {
class CWorld;
} // end namespace game

namespace entity {

//! A mineral deposit that miners gather from. Sizes: 0 medium (40 minerals), 1 small (25),
//! 2 large (70).
class CMineralsEntity : public CEntity
{
public:
    CMineralsEntity(float x, float y, int size);
    virtual ~CMineralsEntity();

    virtual int getRenderLayer() const { return 0; }

    virtual void writeEntityData(ox::io::IWriteFile* file);
    virtual void readEntityData(ox::io::IReadFile* file, int version);
    virtual void updateSprite(float frameDelta);
    //! Done once it is empty.
    virtual int updateLogic(float frameDelta);
    virtual void render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort);
    virtual void renderGroundLayer(const ox::core::CPosition2d<float>& camera,
        const ox::core::CRect<int>& viewPort) {}
    virtual ox::video::ISpriteAnimationState* getCurrentDisplaySprite();

    virtual int onSpark(CSparkEntity* spark);
    virtual bool wantsSpark();

    virtual ox::core::CString<wchar_t> getInfoString();

    virtual float getCollisionSize() const { return 17.0f; }

    //! Takes up to amount minerals out; returns how many it took.
    int withdrawAmount(int amount);
    //! The miner that has claimed the deposit, or -1.
    void setHogStatus(int hoggerId);
    int getHoggerId();
    //! Sets the minerals left and resizes the deposit to match.
    void setRemainingMinerals(int amount);
    int getRemainingMinerals() { return Minerals.getValue(); }

    //! Scatters count deposits over an area in clusters of up to six.
    static void fillAreaWithMinerals(const ox::core::CRect<float>& area, int count, game::CWorld* world);

private:
    //! Picks the sprite for the size and the planet.
    void setSprite();

    ox::video::ISpriteAnimationState* Sprite;
    ox::core::CHiddenInt Minerals;
    int Size;
    int HoggerId;
};

} // end namespace entity
} // end namespace harvest

#endif
