// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of ox::game::CGameMain; member names are inferred.

#ifndef OX_GAME_CGAMEMAIN_H
#define OX_GAME_CGAMEMAIN_H

namespace ox {
class IOxDevice;
namespace game {

class CGameState;

//! The main loop: owns the device and runs the current game state.
class CGameMain
{
public:
    CGameMain();
    virtual ~CGameMain();

    //! Deletes the current state.
    virtual void clear();
    //! Called every frame before the state is updated.
    virtual void updateMain(float time);
    //! Creates the device and the first state; returns non-zero on failure.
    virtual int init() = 0;
    //! Runs one frame.
    virtual void update();
    //! Creates the state with the given id.
    virtual CGameState* stateFactory(int state) = 0;
    virtual bool checkFiles(char** files);

    void shutdown();
    void setState(int state);
    CGameState* getState();
    bool isRunning();
#ifdef HARVEST_PORT
    //! Whether the current state is still loading (the port loads one step per update).
    bool isLoading();
#endif

protected:
    float getNewTimeStep();
#ifdef HARVEST_PORT
    //! Runs one renderFirst/secondInit step of the loading state.
    void loadStep();
    //! Logs the state's error message and stops the game.
    void failState();
#endif

    bool Running;
    CGameState* State;
    IOxDevice* Device;
    bool SleepWhenInactive;
    double LastTime;
#ifdef HARVEST_PORT
    bool Loading;
#endif
};

} // end namespace game
} // end namespace ox

#endif
