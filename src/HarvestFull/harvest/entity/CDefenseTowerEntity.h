// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_ENTITY_CDEFENSETOWERENTITY_H
#define HARVEST_ENTITY_CDEFENSETOWERENTITY_H

#include "CBuildingEntity.h"
#include "CEntityManager.h"
#include "ox/core/CHiddenFloat.h"
#include <vector>

namespace harvest {
namespace entity {

//! A laser tower; linked towers contribute their range and damage to the last tower in the chain.
class CDefenseTowerEntity : public CBuildingEntity
{
public:
    CDefenseTowerEntity(float x, float y);
    virtual ~CDefenseTowerEntity();

    virtual void writeEntityData(ox::io::IWriteFile* file);
    virtual void readEntityData(ox::io::IReadFile* file, int version);
    virtual void updateSprite(float frameDelta);
    virtual int updateLogic(float frameDelta);
    virtual bool addToRenderList(const ox::core::CRect<float>& visibleArea);
    virtual void render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort);
    virtual void renderGroundLayer(const ox::core::CPosition2d<float>& camera,
        const ox::core::CRect<int>& viewPort);
    virtual ox::video::ISpriteAnimationState* getCurrentDisplaySprite();
    virtual int onSpark(CSparkEntity* spark);
    virtual bool wantsSpark();
    virtual ox::core::CString<wchar_t> getInfoString();
    virtual ox::core::CString<wchar_t> getMiniStatString();
    virtual ox::core::CString<wchar_t> getOperatorString();
    virtual float getCollisionSize() const { return 20.0f; }
    virtual void handleRightClickAction(const ox::core::CPosition2d<float>& position);
    virtual void handleDoubleClickSelection();
    virtual bool handleSelectionDraggedToEntity(CEntity* entity);
    virtual void handleSelectionDraggedToNothing();

    void updateTowerLinks();
    bool rotateTowardsTarget(float frameDelta);
    bool isForwardTargetShooting();
    int getBackTargetCount(bool shooting);
    void stopShootingAndClearTarget();
    float getTotalRange();
    float getTotalDamage(bool shooting);
    int getAlienKillCount();
    void updateBackTargetCount();
    void updateForwardTargetShooting();
    void notifyRemoveBackTarget(CDefenseTowerEntity* tower);
    bool isForwardingToThis(CDefenseTowerEntity* tower);
    void notifyNewBackTarget(CDefenseTowerEntity* tower);
    bool isLinked();
    bool isLinkedTo(CDefenseTowerEntity* tower);
    //! Reverses the forwarding chain, making this its last laser.
    void makeEndLaser(CDefenseTowerEntity* tower);
    int getNumBackTargets();

private:
    ox::core::CHiddenFloat Energy;
    bool Shooting;
    bool DrawLaser;
    float SearchCooldown;
    float LaserPhase;
    float ShieldHitTime;
    int Kills;
    ox::entity::SEntityReference Target;
    SEnergyBeam Beam;
    ox::core::CVector3d<float> TargetPosition;
    float TargetHeight;
    ox::entity::SEntityReference ForwardTarget;
    std::vector<ox::entity::SEntityReference*> BackTargets;
    int BackTargetCount;
    int ShootingBackTargetCount;
    bool ForwardTargetShooting;
    ox::video::IParticleState* HitParticle;
    //! 72 directions, first idle and then firing.
    ox::video::ISpriteAnimationState* Sprites[144];
    float Rotation;
    int SpriteIndex;
};

} // end namespace entity
} // end namespace harvest

#endif
