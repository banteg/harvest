// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IrrlichtDevice.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest; not the original source. Virtual order and parameter types follow the
// Mac 1.18 vtable of daisy::CIrrDeviceMacOSX; return types other than createNetworkDevice's are
// provisional.

#ifndef OX_IOXDEVICE_H
#define OX_IOXDEVICE_H

#include "IUnknown.h"
#include "TArray.h"
#include "core/CString.h"

namespace ox {

namespace core { template <class T> class CDimension2d; }
namespace video
{
    class IVideoDriver;
    class IVideoModeList;
    class IImage;
    //! The video drivers, as in Irrlicht 0.7.
    enum E_DRIVER_TYPE
    {
        EDT_NULL = 0,
        EDT_SOFTWARE,
        EDT_DIRECTX8,
        EDT_DIRECTX9,
        EDT_OPENGL
    };
}
namespace io { class IFileSystem; }
namespace gui { class IGUIEnvironment; class ICursorControl; }
namespace scene { class ISceneManager; }
namespace audio { class IAudioDriver; }
namespace input { class IJoystickDriver; }
namespace net { class INetworkDevice; }
namespace event
{
    class IEventReceiver;
    class ILogger;
    // enumerators not recovered yet
    enum E_DRAG_TYPE {};
}
class IOSOperator;
class ITimer;
class IThreadPool;

//! The engine device: window, subsystems and platform services.
class IOxDevice : public IUnknown
{
public:
    virtual ~IOxDevice() {}

    virtual bool createDeviceWindow(const core::CDimension2d<int>& windowSize, unsigned int bits,
                                    bool fullscreen, bool stencilBuffer, bool vsync,
                                    unsigned int antiAlias) = 0;
    virtual bool createUserSelectedDeviceWindow(const TArray<core::CString<wchar_t> >* languages,
                                                unsigned int flags) = 0;
    virtual int getSelectedLanguageIndex() = 0;
    virtual bool setVideoDriver(video::E_DRIVER_TYPE type) = 0;
    virtual bool run() = 0;
    virtual bool swapBuffers() = 0;
    virtual video::IVideoDriver* getVideoDriver() = 0;
    //! Returns false when the device already is in the requested mode.
    virtual bool setFullscreenMode(bool fullscreen) = 0;
    virtual void resizeDeviceWindow(const core::CDimension2d<int>& size) = 0;
    virtual io::IFileSystem* getFileSystem() = 0;
    virtual gui::IGUIEnvironment* getGUIEnvironment() = 0;
    virtual scene::ISceneManager* getSceneManager() = 0;
    virtual gui::ICursorControl* getCursorControl() = 0;
    virtual event::ILogger* getLogger() = 0;
    virtual video::IVideoModeList* getVideoModeList() = 0;
    virtual IOSOperator* getOSOperator() = 0;
    virtual ITimer* getTimer() = 0;
    virtual void setWindowCaption(const wchar_t* text) = 0;
    virtual void moveWindow(int x, int y) = 0;
    virtual bool isWindowActive() = 0;
    virtual void closeDevice() = 0;
    virtual const wchar_t* getVersion() = 0;
    virtual void setEventReceiver(event::IEventReceiver* receiver) = 0;
    virtual void setAcceptsDragAndDrop(event::E_DRAG_TYPE type, bool accept) = 0;
    virtual bool getAcceptsDragAndDrop(event::E_DRAG_TYPE type) = 0;
    virtual void setAcceptsDragAndDropFileType(const char* extension, bool accept) = 0;
    virtual bool getAcceptsDragAndDropFileType(const char* extension) = 0;
    virtual void setResizeAble(bool resize) = 0;
    virtual void createGUIAndScene() = 0;
    virtual net::INetworkDevice* createNetworkDevice(const char* name, int type) = 0;
    virtual net::INetworkDevice* getNetworkDevice(const char* name) = 0;
    virtual void removeNetworkDevice(const char* name) = 0;
    virtual void pollNetworkDevices() = 0;
    virtual audio::IAudioDriver* getAudioDriver() = 0;
    virtual input::IJoystickDriver* getJoystickDriver() = 0;
    virtual void initThreadPool(unsigned int threads) = 0;
    virtual IThreadPool* getThreadPool() = 0;
};

//! Creates the engine device with the given video driver.
extern "C" IOxDevice* createDevice(video::E_DRIVER_TYPE driverType, event::IEventReceiver* receiver,
                                   const wchar_t* version);

} // end namespace ox

#endif
