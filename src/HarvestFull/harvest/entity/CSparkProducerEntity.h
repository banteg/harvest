// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_ENTITY_CSPARKPRODUCERENTITY_H
#define HARVEST_ENTITY_CSPARKPRODUCERENTITY_H

#include "CBuildingEntity.h"

namespace harvest {
namespace entity {

//! The solar collector: sends a spark to a building that wants one every one and a half seconds.
//! Once its power has run out it sends them more slowly.
class CSparkProducerEntity : public CBuildingEntity
{
public:
    CSparkProducerEntity(float x, float y);
    virtual ~CSparkProducerEntity();

    virtual void writeEntityData(ox::io::IWriteFile* file);
    virtual void readEntityData(ox::io::IReadFile* file, int version);
    virtual void updateSprite(float frameDelta);
    virtual int updateLogic(float frameDelta);
    virtual void render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort);
    virtual void renderGroundLayer(const ox::core::CPosition2d<float>& camera,
        const ox::core::CRect<int>& viewPort) {}
    virtual ox::video::ISpriteAnimationState* getCurrentDisplaySprite();

    //! Passes a spark on to the next building that wants one.
    virtual int onSpark(CSparkEntity* spark);

    virtual ox::core::CString<wchar_t> getInfoString();
    virtual ox::core::CString<wchar_t> getMiniStatString();

    virtual float getCollisionSize() const { return 25.0f; }

    bool isExpired() const { return Expired; }

private:
    ox::video::ISpriteAnimationState* Sprite;
    //! Seconds until the next spark.
    float SparkTimer;
    //! Cycles through the buildings in range, see findSparkTarget.
    int SparkIndex;
    //! The power left; nothing in this unit drains it.
    float Power;
    bool Expired;
};

} // end namespace entity
} // end namespace harvest

#endif
