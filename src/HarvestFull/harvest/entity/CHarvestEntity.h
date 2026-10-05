// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_ENTITY_CHARVESTENTITY_H
#define HARVEST_ENTITY_CHARVESTENTITY_H

#include "ox/TArray.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/CRect.h"
#include "ox/core/CString.h"
#include "ox/core/CVector3d.h"
#include "ox/entity/COxEntity.h"
#include "ox/video/SColor.h"

namespace ox {
namespace audio {
class IAudioDriver;
} // end namespace audio

namespace gui {
class IGUIFont;
} // end namespace gui

namespace io {
class IReadFile;
class IWriteFile;
} // end namespace io

namespace video {
class IParticlePackage;
class IParticleState;
class ISpriteAnimationState;
class ISpritePackage;
class IVideoDriver;
} // end namespace video
} // end namespace ox

namespace harvest {
namespace entity {

class CSparkEntity;

static const ox::video::SColor ENERGY_PROGRESS_COLOR(255, 224, 152, 76);

//! Minerals it costs to build each entity type; entities sell for half.
static const int ENTITY_MINERAL_COSTS[] =
{
    50, 2, 0, 0, 5, 0, 0, 15, 30, 0, 0, 0, 0, 50, 40, 0, 0, 0, 0, 0, 0
};

//! The localization keys of the entity type names.
static const wchar_t* const ENTITY_KEY_NAMES[] =
{
    L"build:solar", L"build:energy", L"build:spark", L"build:construction", L"build:harvester",
    L"build:minerals", L"build:alien", L"build:defense", L"build:missileTurret", L"build:missile",
    L"build:particle", L"build:bombBuilding", L"build:bomb", L"build:eagle", L"build:tempest",
    L"build:tempestBlast", L"build:creative", L"build:dropship", L"build:dropshipBullet",
    L"build:shuttleRace", L"build:specialEffect"
};

//! Base of the game's entities. The entity types, as returned by getEntityType, are
//!  0 spark producer, 1 spark mover, 2 spark, 3 construction, 4 mineral gatherer, 5 minerals,
//!  6 alien, 7 defense tower, 8, 13 and 14 missile turrets, 9 missile, 10 particle,
//! 15 tempest blast, 16 creative building and 20 special effect.
class CEntity : public ox::entity::COxEntity
{
public:
    //! Entities with an id above 0 are placed in the entity manager's grid, sparks excepted.
    CEntity(int id, int type, float x, float y);
    virtual ~CEntity();

    virtual int getEntityType();
    //! Updates the logic, then the sprite. Returns nonzero when the entity is done.
    virtual int update(float frameDelta);
    virtual bool addToRenderList(const ox::core::CRect<float>& visibleArea);

    virtual void killEntity() { Killed = true; }
    virtual bool isKilled() { return Killed; }
    virtual void notifyRemoved();

    virtual int getRenderLayer() const { return 1; }

    virtual void writeEntityData(ox::io::IWriteFile* file) = 0;
    virtual void readEntityData(ox::io::IReadFile* file, int version) = 0;

    virtual void updateSprite(float frameDelta);
    virtual int updateLogic(float frameDelta) = 0;

    virtual void render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort) = 0;
    virtual void renderSprite(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort,
        ox::video::ISpriteAnimationState* sprite);
    virtual void renderSpriteFixed(const ox::core::CPosition2d<float>& camera,
        const ox::core::CRect<int>& viewPort, ox::video::ISpriteAnimationState* sprite);
    virtual void renderGroundLayer(const ox::core::CPosition2d<float>& camera,
        const ox::core::CRect<int>& viewPort) = 0;
    //! Draws the energy line to a linked entity in range; only spark movers and (when asked)
    //! minerals have one.
    virtual void renderEnergyLine(const ox::core::CPosition2d<float>& camera,
        const ox::core::CRect<int>& viewPort, const ox::core::CPosition2d<float>& target, bool minerals);
    //! Draws a progress bar from 0 to 1 below the entity.
    virtual void renderSelfProgress(const ox::core::CPosition2d<float>& camera,
        const ox::core::CRect<int>& viewPort, float progress, ox::video::SColor color);
    virtual ox::video::ISpriteAnimationState* getCurrentDisplaySprite();

    //! Takes a spark that arrived; returns the id the spark goes on to, -1 to send it back to its
    //! source, or 0 when the spark is used up.
    virtual int onSpark(CSparkEntity* spark) { return 0; }
    virtual bool wantsSpark() { return false; }
    virtual bool acceptsSparkFrom(int id) { return true; }
    virtual float getSparkHeight() { return 11.0f; }

    virtual ox::core::CString<wchar_t> getInfoString();
    virtual ox::core::CString<wchar_t> getMiniStatString();
    virtual ox::core::CString<wchar_t> getOperatorString();

    virtual float getCollisionSize() const { return 1.0f; }
    virtual int getSellValue() const { return ENTITY_MINERAL_COSTS[Type] >> 1; }

    virtual void handleRightClickAction(const ox::core::CPosition2d<float>& position) {}
    virtual void handleDoubleClickSelection() {}
    virtual bool handleSelectionDraggedToEntity(CEntity* entity) { return false; }
    virtual void handleSelectionDraggedToNothing() {}

    //! The id of the next entity in range that wants a spark, cycling with index.
    int findSparkTarget(int excludeId, int& index);
    //! Refills targets with the entities in spark range when the buildings have changed.
    void updateSparkTargets(ox::TArray<ox::entity::COxEntity*>& targets);
    //! The id of the next target, cycling with index, that wants a spark from this entity.
    int selectSparkTarget(const ox::TArray<ox::entity::COxEntity*>& targets, int excludeId, int& index);

    //! Writes the entity header and data; particles are not saved.
    void writeEntity(ox::io::IWriteFile* file);
    //! Reads an entity written by writeEntity, or returns 0 for an unknown type.
    static CEntity* readNextEntity(ox::io::IReadFile* file, int version);

    static ox::video::ISpritePackage* gp_spritePackage;
    static ox::video::IParticlePackage* gp_particlePackage;
    static ox::video::IVideoDriver* gp_videoDriver;
    static ox::audio::IAudioDriver* gp_audioDriver;
    static ox::gui::IGUIFont* gp_alienChantFont;
    static ox::core::CDimension2d<float> g_screenSizeF;
    static ox::core::CPosition2d<float> g_screenCenterPos;

protected:
    int Type;
    ox::video::SColor Color;
};

//! A particle of the particle package, moved by the wind on the second planet.
class CParticleEntity : public CEntity
{
public:
    CParticleEntity(float x, float y, float z, ox::core::CVector3d<float>* speed, const char* particleName);
    CParticleEntity(ox::video::IParticleState* state, const ox::core::CVector3d<float>& position);
    virtual ~CParticleEntity();

    //! Ground particles render below everything else.
    virtual int getRenderLayer() const;

    virtual void writeEntityData(ox::io::IWriteFile* file) {}
    virtual void readEntityData(ox::io::IReadFile* file, int version) {}

    virtual int updateLogic(float frameDelta);
    virtual void render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort);
    virtual void renderGroundLayer(const ox::core::CPosition2d<float>& camera,
        const ox::core::CRect<int>& viewPort) {}

    virtual int onSpark(CSparkEntity* spark) { return 0; }
    virtual bool wantsSpark() { return false; }

private:
    ox::video::IParticleState* ParticleState;
};

//! A sprite animation drawn for a number of frames, either rotated and scaled at a point or
//! stretched onto four corners.
class CSpecialEffectEntity : public CEntity
{
public:
    CSpecialEffectEntity(float x, float y, float z, ox::video::ISpriteAnimationState* sprite, float scale,
        float rotation, ox::video::SColor color);
    //! The corners are relative to y, which the effect is sorted by.
    CSpecialEffectEntity(const ox::core::CPosition2d<float>& corner1, const ox::core::CPosition2d<float>& corner2,
        const ox::core::CPosition2d<float>& corner3, const ox::core::CPosition2d<float>& corner4, float y,
        ox::video::ISpriteAnimationState* sprite, ox::video::SColor color);
    virtual ~CSpecialEffectEntity();

    virtual bool addToRenderList(const ox::core::CRect<float>& visibleArea) { return true; }

    virtual void writeEntityData(ox::io::IWriteFile* file) {}
    virtual void readEntityData(ox::io::IReadFile* file, int version) {}

    virtual int updateLogic(float frameDelta);
    virtual void render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort);
    //! Effects below the ground (a negative height) render in the ground layer.
    virtual void renderGroundLayer(const ox::core::CPosition2d<float>& camera,
        const ox::core::CRect<int>& viewPort);

    virtual int onSpark(CSparkEntity* spark) { return 0; }
    virtual bool wantsSpark() { return false; }

private:
    ox::video::ISpriteAnimationState* Sprite;
    //! Frames left to draw.
    int Frames;
    float Scale;
    float Rotation;
    ox::core::CPosition2d<float> Corner1;
    ox::core::CPosition2d<float> Corner2;
    ox::core::CPosition2d<float> Corner3;
    ox::core::CPosition2d<float> Corner4;
    bool FreeShape;
};

} // end namespace entity
} // end namespace harvest

#endif
