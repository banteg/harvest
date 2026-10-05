// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_ENTITY_CPERIMETERBOMB_H
#define HARVEST_ENTITY_CPERIMETERBOMB_H

#include "CHarvestEntity.h"
#include "ox/core/CVector2d.h"

namespace harvest {
namespace entity {

//! The blast of a perimeter bomb, or of an overcharged energy link.
class CPerimeterBombExplosion : public CEntity
{
public:
    CPerimeterBombExplosion(float x, float y);
    void setFuse(float fuse) { Fuse = fuse; }
    virtual ~CPerimeterBombExplosion();

    virtual void writeEntityData(ox::io::IWriteFile* file);
    virtual void readEntityData(ox::io::IReadFile* file, int version);
    virtual void updateSprite(float frameDelta);
    virtual int updateLogic(float frameDelta);
    virtual void render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort);
    virtual void renderGroundLayer(const ox::core::CPosition2d<float>& camera,
        const ox::core::CRect<int>& viewPort) {}

private:
    float Fuse;
    ox::core::CVector2d<float> Speed;
    bool Moved;
    ox::video::ISpriteAnimationState* Sprite;
};

} // end namespace entity
} // end namespace harvest

#endif
