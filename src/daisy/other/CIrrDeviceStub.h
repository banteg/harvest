// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CIrrDeviceStub.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Member layout follows the Linux 1.18 constructor (sizeof is 0x120); the virtual order follows the
// Mac 1.18 vtable.

#ifndef DAISY_CIRRDEVICESTUB_H
#define DAISY_CIRRDEVICESTUB_H

#include <map>
#include <vector>
#include "daisy/include/IDaisyDevice.h"
#include "daisy/other/CTimer.h"
#include "daisy/other/CLogger.h"
#include "daisy/video/CVideoModeList.h"
#include "ox/core/CBasic.h"
#include "ox/core/CPosition2d.h"
#include "ox/event/IEventReceiver.h"

namespace ox {
namespace io { class IFileSystem; }
namespace gui { class IGUIEnvironment; class ICursorControl; }
namespace scene { class ISceneManager; }
namespace audio { class IAudioDriver; }
namespace input { class IJoystickDriver; }
namespace net { class INetworkDevice; }
class IOSOperator;
class IThreadPool;
} // end namespace ox

namespace daisy {

//! A network device created by name with createNetworkDevice.
struct SNamedNetDevice
{
    ox::core::CString<char> Name;
    ox::net::INetworkDevice* Device;
};

//! The platform-independent part of the device: owns the subsystems and routes input events.
class CIrrDeviceStub : public IDaisyDevice
{
public:
    CIrrDeviceStub(const wchar_t* version, ox::event::IEventReceiver* receiver);
    virtual ~CIrrDeviceStub();

    virtual ox::video::IVideoDriver* getVideoDriver();
    //! The stub cannot switch modes and returns false.
    virtual bool setFullscreenMode(bool fullscreen) { return false; }
    virtual ox::io::IFileSystem* getFileSystem();
    virtual ox::gui::IGUIEnvironment* getGUIEnvironment();
    virtual ox::scene::ISceneManager* getSceneManager();
    virtual ox::gui::ICursorControl* getCursorControl();
    virtual ox::event::ILogger* getLogger();
    virtual ox::video::IVideoModeList* getVideoModeList();
    virtual ox::IOSOperator* getOSOperator();
    virtual ox::ITimer* getTimer();
    virtual void moveWindow(int x, int y) {}
    virtual const wchar_t* getVersion();
    virtual void setEventReceiver(ox::event::IEventReceiver* receiver);
    virtual void setAcceptsDragAndDrop(ox::event::E_DRAG_TYPE type, bool accept);
    virtual bool getAcceptsDragAndDrop(ox::event::E_DRAG_TYPE type);
    virtual void setAcceptsDragAndDropFileType(const char* extension, bool accept);
    virtual bool getAcceptsDragAndDropFileType(const char* extension);
    virtual void setResizeAble(bool resize);
    virtual void createGUIAndScene();
    virtual ox::net::INetworkDevice* createNetworkDevice(const char* name, int type);
    virtual ox::net::INetworkDevice* getNetworkDevice(const char* name);
    virtual void removeNetworkDevice(const char* name);
    virtual void pollNetworkDevices();
    //! The audio driver, created on first use by createAudioDriver.
    virtual ox::audio::IAudioDriver* getAudioDriver();
    //! The joystick driver, created on first use by createJoystickDriver.
    virtual ox::input::IJoystickDriver* getJoystickDriver();
    virtual void initThreadPool(unsigned int threads);
    virtual ox::IThreadPool* getThreadPool();

    virtual ox::audio::IAudioDriver* createAudioDriver() = 0;
    virtual ox::input::IJoystickDriver* createJoystickDriver() = 0;

protected:
    //! Offers an input event to the GUI, then to the user receiver, then to the scene manager;
    //! the first one that returns true consumes it.
    void postEventFromUser(ox::event::SEvent event);

    //! Logs a warning and returns false when the application was built for another engine version.
    bool checkVersion(const wchar_t* version);

    //! Counts successive clicks: a press continues the series when it is the same button as the
    //! last press, at most 250 ms later and at most 10 pixels away on each axis; otherwise the
    //! series restarts. Returns the number of clicks in the series, 2 for a double click.
    int getClickCount(int button, const ox::core::CPosition2d<int>& pos)
    {
        if (button != LastClickButton || Timer->getTime() - LastClickTime > 250
            || ox::core::abs_(LastClickPos.X - pos.X) > 10 || ox::core::abs_(LastClickPos.Y - pos.Y) > 10)
        {
            ClickCount = 0;
            LastClickButton = button;
            LastClickPos = pos;
        }
        LastClickTime = Timer->getTime();
        return ++ClickCount;
    }

    ox::io::IFileSystem* FileSystem;
    ox::video::IVideoDriver* VideoDriver;
    ox::gui::IGUIEnvironment* GUIEnvironment;
    ox::scene::ISceneManager* SceneManager;
    CTimer* Timer;
    ox::gui::ICursorControl* CursorControl;
    video::CVideoModeList VideoModeList;
    ox::event::IEventReceiver* UserReceiver;
    CLogger* Logger;
    ox::IOSOperator* Operator;
    std::vector<SNamedNetDevice> NetworkDevices;
    ox::audio::IAudioDriver* AudioDriver;
    ox::input::IJoystickDriver* JoystickDriver;
    bool AcceptsDragAndDrop[3];
    std::map<ox::core::CString<char>, bool> AcceptedDragAndDropFileTypes;
    ox::IThreadPool* ThreadPool;
    unsigned int LastClickTime;
    int LastClickButton;
    ox::core::CPosition2d<int> LastClickPos;
    int ClickCount;
};

} // end namespace daisy

#endif
