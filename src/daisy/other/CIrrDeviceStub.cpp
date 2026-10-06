// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CIrrDeviceStub.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Daisy adds drag and drop settings, named network devices, lazily created audio and joystick
// drivers and a thread pool slot to Irrlicht's stub, and offers input events to the GUI first.

#include "CIrrDeviceStub.h"
#include <wchar.h>
#include "daisy/os.h"
#include "daisy/input/CJoystickNullDriver.h"
#include "ox/IOSOperator.h"
#include "ox/IThreadPool.h"
#include "ox/audio/IAudioDriver.h"
#include "ox/gui/ICursorControl.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/input/IJoystickDriver.h"
#include "ox/io/IFileSystem.h"
#include "ox/net/INetworkDevice.h"
#include "ox/scene/ISceneManager.h"
#include "ox/video/IVideoDriver.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

//! The engine version the device reports; applications pass the version they were built with.
#define DAISY_SDK_VERSION L"0.7"

namespace daisy {

namespace io
{
    ox::io::IFileSystem* createFileSystem();
} // end namespace io

namespace gui
{
    ox::gui::IGUIEnvironment* createGUIEnvironment(ox::io::IFileSystem* fs, ox::video::IVideoDriver* driver,
        ox::IOSOperator* op);
} // end namespace gui

namespace scene
{
    ox::scene::ISceneManager* createSceneManager(ox::video::IVideoDriver* driver, ox::io::IFileSystem* fs,
        ox::gui::ICursorControl* cursorControl);
} // end namespace scene

namespace net
{
    //! Provisional: only what the stub needs (the constructor and the Linux size, 0xc0) until the
    //! CWinsockNetworkDevice unit is recovered.
    class CWinsockNetworkDevice : public ox::net::INetworkDevice
    {
    public:
        //! createNetworkDevice passes false for type 0 and true for type 1.
        CWinsockNetworkDevice(bool flag, const char* name);
        virtual ~CWinsockNetworkDevice();

        virtual int createHost(ox::event::IEventReceiver* receiver, const ox::net::SServerInfo& info);
        virtual int joinHost(ox::event::IEventReceiver* receiver, const ox::net::SServerInfo& info);
        virtual void disconnect();
        virtual const char* getLocalIp();
        virtual int sendMessage(int messageId, void* data, int size, int flags);
        virtual int broadcastMessage(void* data, int size, int messageId, int flags);
        virtual void decryptPacket(const void* data, int size);
        virtual void kickClient(int client);
        virtual void pollDevice();

    private:
        char Unrecovered[0xc0 - sizeof(ox::net::INetworkDevice)];
    };
} // end namespace net

//! Creates the logger (it becomes os::Printer::Logger), logs "Daisy version 0.7" at ELL_NONE so it
//! always prints, warns when the application's version differs, then creates the timer and the file
//! system. The video driver, GUI, scene manager, cursor control and OS operator are left to the
//! platform device and createGUIAndScene.
CIrrDeviceStub::CIrrDeviceStub(const wchar_t* version, ox::event::IEventReceiver* receiver)
    : VideoDriver(0), Logger(0), Operator(0), AudioDriver(0), JoystickDriver(0), ThreadPool(0)
{
    UserReceiver = receiver;

    Logger = new CLogger(UserReceiver);
    os::Printer::Logger = Logger;

    ox::core::CString<wchar_t> s = L"Daisy version ";
    s += getVersion();
    os::Printer::log(s.c_str(), ox::event::ELL_NONE);

    checkVersion(version);

    // create timer
    Timer = new CTimer();

    // create filesystem
    FileSystem = io::createFileSystem();

    GUIEnvironment = 0;
    VideoDriver = 0;
    SceneManager = 0;
    CursorControl = 0;
    Operator = 0;
    for (int i = 0; i < 3; ++i)
        AcceptsDragAndDrop[i] = false;
    LastClickTime = 0;
    LastClickButton = 0;
    ClickCount = 0;
}

//! checks version of sdk and prints warning if there might be a problem
bool CIrrDeviceStub::checkVersion(const wchar_t* version)
{
    if (wcscmp(getVersion(), version))
    {
        ox::core::CString<wchar_t> w;
        w = L"Warning: The version of the Daisy (";
        w += getVersion();
        w += L") does not match the version of the application (";
        w += version;
        w += L").";
        os::Printer::log(w.c_str(), ox::event::ELL_WARNING);
        return false;
    }

    return true;
}

//! Drops the file system, GUI, video driver, scene manager, cursor control, OS operator, every
//! named network device, the timer, the logger and the audio and joystick drivers in that order,
//! then deletes the thread pool. os::Printer::Logger keeps pointing at the dropped logger.
CIrrDeviceStub::~CIrrDeviceStub()
{
    FileSystem->drop();

    if (GUIEnvironment)
        GUIEnvironment->drop();

    if (VideoDriver)
        VideoDriver->drop();

    if (SceneManager)
        SceneManager->drop();

    if (CursorControl)
        CursorControl->drop();

    if (Operator)
        Operator->drop();

    if (!NetworkDevices.empty())
        for (unsigned int i = 0; i < NetworkDevices.size(); ++i)
            NetworkDevices[i].Device->drop();

    CursorControl = 0;

    Timer->drop();

    Logger->drop();

    if (AudioDriver)
        AudioDriver->drop();

    if (JoystickDriver)
        JoystickDriver->drop();

    if (ThreadPool)
        delete ThreadPool;
}

//! Replaces the GUI environment and scene manager with new ones for the current video driver and
//! hands them the user receiver.
void CIrrDeviceStub::createGUIAndScene()
{
    if (GUIEnvironment)
        GUIEnvironment->drop();

    if (SceneManager)
        SceneManager->drop();

    // create gui environment
    GUIEnvironment = gui::createGUIEnvironment(FileSystem, VideoDriver, Operator);

    // create Scene manager
    SceneManager = scene::createSceneManager(VideoDriver, FileSystem, CursorControl);

    setEventReceiver(UserReceiver);
}

//! returns the video driver
ox::video::IVideoDriver* CIrrDeviceStub::getVideoDriver()
{
    return VideoDriver;
}

//! return file system
ox::io::IFileSystem* CIrrDeviceStub::getFileSystem()
{
    return FileSystem;
}

//! returns the gui environment
ox::gui::IGUIEnvironment* CIrrDeviceStub::getGUIEnvironment()
{
    return GUIEnvironment;
}

//! returns the scene manager
ox::scene::ISceneManager* CIrrDeviceStub::getSceneManager()
{
    return SceneManager;
}

//! \return Returns a pointer to the ITimer object. With it the current Time can be received.
ox::ITimer* CIrrDeviceStub::getTimer()
{
    return Timer;
}

//! Returns the version of the engine.
const wchar_t* CIrrDeviceStub::getVersion()
{
    return DAISY_SDK_VERSION;
}

//! \return Returns a pointer to the mouse cursor control interface.
ox::gui::ICursorControl* CIrrDeviceStub::getCursorControl()
{
    return CursorControl;
}

//! \return Returns a pointer to a list with all video modes supported by the gfx adapter.
ox::video::IVideoModeList* CIrrDeviceStub::getVideoModeList()
{
    return &VideoModeList;
}

//! Unlike Irrlicht 0.7, the GUI environment sees an event before the user receiver, and the
//! scene manager only gets what both left alone.
void CIrrDeviceStub::postEventFromUser(ox::event::SEvent event)
{
    bool absorbed = false;

    if (GUIEnvironment)
        absorbed = GUIEnvironment->postEventFromUser(event);

    if (!absorbed && UserReceiver)
        absorbed = UserReceiver->OnEvent(event);

    if (!absorbed && SceneManager)
        absorbed = SceneManager->postEventFromUser(event);
}

//! Sets a new event receiver to receive events
void CIrrDeviceStub::setEventReceiver(ox::event::IEventReceiver* receiver)
{
    UserReceiver = receiver;
    Logger->setReceiver(receiver);
    if (GUIEnvironment)
        GUIEnvironment->setUserEventReceiver(receiver);
}

//! \return Returns a pointer to the logger.
ox::event::ILogger* CIrrDeviceStub::getLogger()
{
    return Logger;
}

//! Returns the operation system opertator object.
ox::IOSOperator* CIrrDeviceStub::getOSOperator()
{
    return Operator;
}

//! Sets if the window should be resizeable in windowed mode.
void CIrrDeviceStub::setResizeAble(bool resize)
{
}

//! The flags are plain storage here; the platform device reads them when files are dropped.
void CIrrDeviceStub::setAcceptsDragAndDrop(ox::event::E_DRAG_TYPE type, bool accept)
{
    AcceptsDragAndDrop[type] = accept;
}

bool CIrrDeviceStub::getAcceptsDragAndDrop(ox::event::E_DRAG_TYPE type)
{
    return AcceptsDragAndDrop[type];
}

//! The extension is compared case-sensitively, exactly as given.
void CIrrDeviceStub::setAcceptsDragAndDropFileType(const char* extension, bool accept)
{
    AcceptedDragAndDropFileTypes[ox::core::CString<char>(extension)] = accept;
}

//! Looks the extension up with operator[], so asking about an unknown extension adds it as
//! not accepted.
bool CIrrDeviceStub::getAcceptsDragAndDropFileType(const char* extension)
{
    return AcceptedDragAndDropFileTypes[ox::core::CString<char>(extension)];
}

//! Returns the device already registered under the name, whatever its type. Otherwise creates a
//! CWinsockNetworkDevice (type 0 passes false to its constructor, type 1 true; other types
//! create nothing and return 0) and registers it under the name. The device keeps it until
//! removeNetworkDevice or the device's destructor.
ox::net::INetworkDevice* CIrrDeviceStub::createNetworkDevice(const char* name, int type)
{
    ox::net::INetworkDevice* device = getNetworkDevice(name);
    if (device)
        return device;

    switch (type)
    {
    case 0:
        device = new net::CWinsockNetworkDevice(false, name);
        break;
    case 1:
        device = new net::CWinsockNetworkDevice(true, name);
        break;
    }

    if (device)
    {
        SNamedNetDevice entry;
        entry.Name = name;
        entry.Device = device;
        NetworkDevices.push_back(entry);
    }

    return device;
}

//! The device registered under the name, or 0.
ox::net::INetworkDevice* CIrrDeviceStub::getNetworkDevice(const char* name)
{
    for (unsigned int i = 0; i < NetworkDevices.size(); ++i)
        if (NetworkDevices[i].Name == ox::core::CString<char>(name))
            return NetworkDevices[i].Device;

    return 0;
}

//! Deletes the device registered under the name (with delete, ignoring its reference count) and
//! forgets it.
void CIrrDeviceStub::removeNetworkDevice(const char* name)
{
    for (std::vector<SNamedNetDevice>::iterator it = NetworkDevices.begin(); it != NetworkDevices.end(); ++it)
    {
        if (it->Name == ox::core::CString<char>(name))
        {
            delete it->Device;
            NetworkDevices.erase(it);
            return;
        }
    }
}

//! Polls every registered network device in creation order.
void CIrrDeviceStub::pollNetworkDevices()
{
    for (std::vector<SNamedNetDevice>::iterator it = NetworkDevices.begin(); it != NetworkDevices.end(); ++it)
        it->Device->pollDevice();
}

ox::audio::IAudioDriver* CIrrDeviceStub::getAudioDriver()
{
    if (!AudioDriver)
        return createAudioDriver();

    return AudioDriver;
}

ox::input::IJoystickDriver* CIrrDeviceStub::getJoystickDriver()
{
    if (!JoystickDriver)
        return createJoystickDriver();

    return JoystickDriver;
}

//! Pure, but defined: a joystick driver without joysticks. Linux overrides it, so it is unused.
ox::input::IJoystickDriver* CIrrDeviceStub::createJoystickDriver()
{
    JoystickDriver = new input::CJoystickNullDriver();
    return JoystickDriver;
}

//! The stub has no thread pool; no 1.18 device creates one.
void CIrrDeviceStub::initThreadPool(unsigned int threads)
{
}

//! Logs an error and returns 0 while no thread pool exists.
ox::IThreadPool* CIrrDeviceStub::getThreadPool()
{
    if (!ThreadPool)
    {
        os::Printer::log("Thread pool retrieved without having been initialized!", ox::event::ELL_ERROR);
        return 0;
    }

    return ThreadPool;
}

} // end namespace daisy
