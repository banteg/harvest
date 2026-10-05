// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_STATES_CINTROSTATE_H
#define HARVEST_STATES_CINTROSTATE_H

#include "ox/algo/CTimeCounter.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CPosition2d.h"
#include "ox/game/CGameState.h"

namespace ox {
namespace gui {
class IGUIElement;
class IGUIFont;
} // end namespace gui
namespace video {
class ISpritePackage;
class ISpriteAnimationState;
} // end namespace video
} // end namespace ox

namespace harvest {
namespace states {

class CLoadingScreen;

//! The Oxeye splash screen shown at startup.
class CIntroState : public ox::game::CGameState
{
public:
    CIntroState();
    virtual ~CIntroState();

    virtual bool OnEvent(const ox::event::SEvent& event);
    virtual int firstInit(ox::IOxDevice* device);
    virtual void renderFirst();
    //! Loads one step per call; returns 2 while loading, 1 on failure and 0 when done.
    virtual int secondInit();
    //! Returns the state to switch to, or 0 to stay.
    virtual int updateState(float time);
    virtual void render();

private:
    enum
    {
        SPRITE_BORDER_ABOVE,
        SPRITE_BORDER_BENEATH,
        SPRITE_OXEYE_SPLASH,
        SPRITE_COUNT
    };

    int NextState;
    bool Keys[256];
    ox::gui::IGUIFont* Font;
    ox::core::CDimension2d<int> ScreenSize;
    CLoadingScreen* LoadingScreen;
    unsigned int InitStep;
    int FadeState;
    int Phase;
    ox::algo::CTimeCounter FadeCounter;
    ox::algo::CTimeCounter LayerCounter;
    ox::video::ISpritePackage* SplashPackage;
    ox::video::ISpriteAnimationState* Sprites[SPRITE_COUNT];
    ox::core::CPosition2d<int> SpriteSizes[SPRITE_COUNT];
    ox::core::CPosition2d<int> LayerPositions[5];
    ox::gui::IGUIElement* SkipElement;
};

} // end namespace states
} // end namespace harvest

#endif
