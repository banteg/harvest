// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_STATES_CLOADINGSCREEN_H
#define HARVEST_STATES_CLOADINGSCREEN_H

namespace ox {
class IOxDevice;
namespace video {
class IVideoDriver;
class ISpritePackage;
class ISpriteAnimationState;
} // end namespace video
} // end namespace ox

namespace harvest {
namespace states {

//! The spinning Oxeye logo and version line shown while a state loads.
class CLoadingScreen
{
public:
    CLoadingScreen();
    virtual ~CLoadingScreen();

    //! Returns 1 if the menu sprite package cannot be loaded.
    int init(ox::video::IVideoDriver* driver);
    //! Draws a frame at most every 100 milliseconds.
    void render(ox::IOxDevice* device, ox::video::IVideoDriver* driver);

private:
    ox::video::ISpritePackage* Package;
    ox::video::ISpriteAnimationState* Logo;
    float Rotation;
    unsigned int LastRenderTime;
};

} // end namespace states
} // end namespace harvest

#endif
