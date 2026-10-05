// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Layout and native methods are reconstructed; byte matching remains partial.

#ifndef HARVEST_GAME_CWORLD_H
#define HARVEST_GAME_CWORLD_H

#include "ox/TArray.h"
#include "ox/algo/CRand.h"
#include "ox/core/CHiddenInt.h"
#include "ox/core/CDimension2d.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/CRect.h"
#include "ox/core/CVector2d.h"
#include "ox/core/CVector3d.h"

namespace ox {
namespace video { class IVideoDriver; }
namespace gui { class IGUIFont; }
}

namespace harvest {
namespace game {

// Every game unit initializes this constant at startup, so its initializer is not a constant
// expression to GCC; an inline function call reproduces that. The name and the function are ours.
inline float getWorldGridOffset()
{
    return 4096.0f;
}

//! Added to world coordinates to index the entity grid.
static const float WORLD_GRID_OFFSET = getWorldGridOffset();

//! A scenery obstacle: its sprite bounds limit the elliptical collision test.
struct SDoodad
{
    int Type;
    ox::core::CPosition2d<float> Position;
    ox::core::CRect<float> Bounds;
};

struct SWindPuff;

//! The planet surface the game is played on.
class IScenario;
class CWorld
{
public:
    CWorld(int gameMode, int planet);
    virtual ~CWorld();
    bool initializeWorld(ox::video::IVideoDriver* driver, const ox::core::CDimension2d<int>& size);
    void initializeNewGame(IScenario* scenario);
    bool readAndInitialize(ox::io::IReadFile* file, int version, ox::video::IVideoDriver* driver,
        const ox::core::CDimension2d<int>& size);
    void update(float frameDelta, const ox::core::CRect<float>& area);
    void renderBackground(const ox::core::CPosition2d<float>& position, ox::gui::IGUIFont* font,
        ox::core::CRect<int>* clip);
    void renderEdgeShades(const ox::core::CPosition2d<float>& position);

    void changeViewSize(const ox::core::CDimension2d<int>& size);
    bool worldChangesSizeInThisGameMode() const;
    void constrainViewPos(ox::core::CPosition2d<float>& position);
    bool checkCollisionWithDoodad(const SDoodad* doodad, const ox::core::CPosition2d<float>& position);
    ox::core::CPosition2d<float> findRendezvousPoint(const ox::core::CPosition2d<float>& position,
        const ox::core::CVector2d<float>& movement);
    void createDoodad(const ox::core::CPosition2d<float>& position, int type);
    void placeDoodads(const ox::core::CRect<float>& area, int count);
    void recreateDoodadGrid();
    bool write(ox::io::IWriteFile* file);
    ox::core::CRect<float> expandWorld(int direction, float boundary, bool populate);
    void expandWorldFromCurrent(int direction, bool populate);

    //! Index of the planet: 0, 1 or 2.
    int getPlanet() const;
    int getGameMode() const;

    //! The area the player may currently build in.
    const ox::core::CRect<float>& getActualGameFieldSize() const;

    bool hasWorldExpandedAtLeastOnce();

    bool mayPlaceObjectHere(const ox::core::CPosition2d<float>& position, bool building);
    const ox::core::CRect<float>& getVisibleGameFieldSize() const;
    float getCollisionTangent(const ox::core::CPosition2d<float>& position);
    bool mayMoveHere(const ox::core::CPosition2d<float>& position);

    //! Adds the wind at a position over the frame to speed.
    void applyWind(const ox::core::CVector3d<float>& position, ox::core::CVector2d<float>& speed,
        float frameDelta) const;

private:
    // Names are ours; the native members and their cross-platform layout are verified.
    ox::video::IVideoDriver* VideoDriver;
    ox::core::CRect<float> VisibleGameField;
    ox::core::CRect<float> ActualGameField;
    ox::core::CRect<float> TargetGameField;
    bool InitialWorld;
    ox::core::CDimension2d<float> ViewSize;

public:
    //! Read directly by particles, which only feel wind on planet 1; see getPlanet.
    int Planet;
    //! Set directly when a saved game is read.
    int GameMode;

private:
    ox::video::ISpriteAnimationState* GroundSprites[2];
    ox::video::ISpriteAnimationState* DoodadSprites[23];
    ox::core::CDimension2d<int> DoodadSizes[23];
    float DoodadCollisionRadii[23];
    ox::algo::CRand Random;
    ox::TArray<SDoodad*> Doodads;
    ox::core::CRect<float> DoodadGridArea;
    int DoodadGridWidth;
    int DoodadGridHeight;
    ox::TArray<SDoodad*>* DoodadGrid;
    float WindClock;
    ox::TArray<SWindPuff*> WindPuffs;
    ox::core::CRect<float> WindArea;
};

extern CWorld* gp_world;
//! The player's minerals, and a negated copy that catches tampering.
extern ox::core::CHiddenInt* gp_mineralAmount;
extern ox::core::CHiddenInt* gp_negatedMineralAmount;

} // end namespace game
} // end namespace harvest

#endif
