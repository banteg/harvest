// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_STATES_CSHUTTLERACESTATE_H
#define HARVEST_STATES_CSHUTTLERACESTATE_H

#include "harvest/entity/CEntityManager.h"
#include "harvest/gui/CStoryScreen.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/CRect.h"
#include "ox/core/CString.h"
#include "ox/game/CGameState.h"
#include "ox/video/IParticleEngineCallback.h"

namespace ox {
namespace gui {
class IGUIFont;
} // end namespace gui
namespace video {
class ISpriteAnimationState;
} // end namespace video
} // end namespace ox

namespace harvest {
namespace entity {
class CShuttleEntity;
} // end namespace entity
namespace states {

class CLoadingScreen;

//! The hidden two-player shuttle race on a split screen.
class CShuttleRaceState : public ox::game::CGameState, public game::IScenario,
                          public ox::video::IParticleEngineCallback
{
public:
    CShuttleRaceState();
    virtual ~CShuttleRaceState();

    virtual bool OnEvent(const ox::event::SEvent& event);
    virtual int firstInit(ox::IOxDevice* device);
    virtual void renderFirst();
    //! Loads one step per call; returns 2 while loading, 1 on failure and 0 when done.
    virtual int secondInit();
    //! Returns the state to switch to, or 0 to stay.
    virtual int updateState(float time);
    virtual void render();

    virtual void addParticleEntity(ox::video::IParticleState* state, const ox::core::CVector3d<float>& position);
    virtual const char* getOnDieMarkerAtPos(const ox::core::CVector3d<float>& position) { return 0; }
    virtual void playParticleSound(const char* name, const ox::core::CVector3d<float>& position);
    virtual int getDoodadSeed() const;
    virtual void applyInitialExpansions(game::CWorld* world);

    //! The world position under a point of the left view.
    ox::core::CPosition2d<float> getWorldPos(ox::core::CPosition2d<int> position);

private:
    enum ERaceState
    {
        RACE_COUNTDOWN,
        RACE_RUNNING,
        RACE_FINISHED
    };

    enum
    {
        SPRITE_CHECKPOINT,
        SPRITE_LOGO,
        SPRITE_COUNT
    };

    int NextState;
    bool Keys[256];
    ox::gui::IGUIFont* Font;
    ox::gui::IGUIFont* SmallFont;
    ox::gui::IGUIFont* LargeFont;
    ox::core::CDimension2d<int> ScreenSize;
    ox::core::CDimension2d<float> ScreenSizeF;
    CLoadingScreen* LoadingScreen;
    unsigned int InitStep;
    //! The camera of each player's half of the screen.
    ox::core::CPosition2d<float> ViewPositions[2];
    //! The world area each view shows, with a margin; particles outside both are dropped.
    ox::core::CRect<float> ViewRects[2];
    ox::core::CPosition2d<int> MousePosition;
    ox::video::ISpriteAnimationState* Sprites[SPRITE_COUNT];
    entity::CShuttleEntity* Shuttles[2];
    bool Finished[2];
    //! Points from a shuttle towards its next checkpoint.
    entity::SEnergyBeam GuideBeam;
    float TimeScale;
    int RaceState;
    float Countdown;
    bool ShowMessage;
    ox::core::CString<wchar_t> Message;
    ox::core::CString<wchar_t> SubMessage;
    //! Stepped through 0-27 with tab; nothing reads it.
    int DebugIndex;
};

} // end namespace states
} // end namespace harvest

#endif
