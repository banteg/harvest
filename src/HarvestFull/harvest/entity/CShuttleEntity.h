// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_ENTITY_CSHUTTLEENTITY_H
#define HARVEST_ENTITY_CSHUTTLEENTITY_H
#include "CHarvestEntity.h"
#include "ox/core/CVector2d.h"
namespace harvest {
namespace entity {

//! The race track: the shuttles pass the checkpoints in order.
static const ox::core::CPosition2d<float> LAP_CHECKPOINTS[14] = {
    ox::core::CPosition2d<float>(500.0f, 1580.0f),
    ox::core::CPosition2d<float>(929.0f, 1629.0f),
    ox::core::CPosition2d<float>(1331.0f, 1016.0f),
    ox::core::CPosition2d<float>(1125.0f, 730.0f),
    ox::core::CPosition2d<float>(904.0f, 379.0f),
    ox::core::CPosition2d<float>(400.0f, 334.0f),
    ox::core::CPosition2d<float>(140.0f, -81.0f),
    ox::core::CPosition2d<float>(90.0f, -575.0f),
    ox::core::CPosition2d<float>(-600.0f, -540.0f),
    ox::core::CPosition2d<float>(-620.0f, -164.0f),
    ox::core::CPosition2d<float>(-654.0f, 294.0f),
    ox::core::CPosition2d<float>(-828.0f, 809.0f),
    ox::core::CPosition2d<float>(-583.0f, 1360.0f),
    ox::core::CPosition2d<float>(50.0f, 1563.0f),
};

//! A controllable or AI-driven shuttle used in the checkpoint race game mode.
//! Member names are ours; the public method names are from the Mac symbols.
class CShuttleEntity : public CEntity
{
public:
    CShuttleEntity(int ai);
    virtual ~CShuttleEntity();
    virtual void updateSprite(float frameDelta);
    virtual int updateLogic(float frameDelta);
    virtual void render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort);
    virtual void renderGroundLayer(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort) {}
    virtual void writeEntityData(ox::io::IWriteFile* file);
    virtual void readEntityData(ox::io::IReadFile* file, int version);
    void updatePlacing();
    bool isAi();
    void updateAi();
    int getNextCheckPoint() const;
    void setMovementFlag(int flag, bool enabled);
    const ox::core::CVector2d<float>& getCurrentSpeed();
    int getNextCheckPointNoMod() const;
    int getCurrentLap() const;
    float getTimeAtLastCheckPoint() const;
    void setCurrentPlacing(int placing);
    int getCurrentPlacing() const;
    float getCurrentLapTime() const;
    float getTotalTime() const;
    void resetAllAiByMe(bool mutate);
private:
    ox::video::ISpriteAnimationState* Sprites[30];
    int SpriteIndex;
    ox::video::IParticleState* Thrust;
    float Angle;
    ox::core::CVector2d<float> Speed;
    int Ai;
    int MovementFlags;
    int NextCheckPoint;
    int Lap;
    float LapTime;
    float TotalTime;
    float LastCheckPointTime;
    int Placing;
    int ReturnState;
    ox::core::CPosition2d<float> ReturnPosition;
    ox::core::CPosition2d<float> EndTargets[14];
    ox::core::CPosition2d<float> MiddleTargets[14];
    bool UseMiddleTarget;
};
struct SBestShuttleSorter
{
    bool operator()(CShuttleEntity* first, CShuttleEntity* second) const
    {
        if (first->getNextCheckPointNoMod() != second->getNextCheckPointNoMod())
            return first->getNextCheckPointNoMod() > second->getNextCheckPointNoMod();
        return first->getTimeAtLastCheckPoint() < second->getTimeAtLastCheckPoint();
    }
};
} // end namespace entity
} // end namespace harvest
#endif
