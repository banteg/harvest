// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IEventReceiver.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::event namespace; not the original source. Partial: only the GUI, mouse,
// key, network and user events are recovered, the mouse and key ones only as far as they are read.

#ifndef OX_EVENT_IEVENTRECEIVER_H
#define OX_EVENT_IEVENTRECEIVER_H

#include "../Keycodes.h"

namespace ox {
namespace gui {

class IGUIElement;

//! GUI events, in SEvent::GUIEvent.EventType.
enum EGUI_EVENT_TYPE
{
    EGET_BUTTON_CLICKED = 3,
    EGET_TEXT_BUTTON_CLICKED = 4,
    EGET_CHECKBOX_CHANGED = 5,
    //! Sent by a check box when a click toggled it.
    EGET_CHECKBOX_TOGGLED = 8,
    //! The yes button of a message box was pressed.
    EGET_MESSAGEBOX_YES = 15,
    //! An element that reports drawing has been drawn.
    EGET_ELEMENT_DRAWN = 26
};

} // end namespace gui

namespace event {

enum EEVENT_TYPE
{
    //! An event of the GUI, in SEvent::GUIEvent.
    EET_GUI_EVENT = 0,
    //! A mouse input event, in SEvent::MouseInput.
    EET_MOUSE_INPUT_EVENT = 1,
    //! A keyboard input event, in SEvent::KeyInput.
    EET_KEY_INPUT_EVENT = 2,
    //! A network device event.
    EET_NETWORK_EVENT = 5,
    //! A device event, in SEvent::DeviceEvent.
    EET_DEVICE_EVENT = 6,
    //! A game-defined event, in SEvent::UserEvent.
    EET_USER_EVENT = 7
};

//! Mouse input events, in SEvent::MouseInput.Event.
enum EMOUSE_INPUT_EVENT
{
    EMIE_LMOUSE_PRESSED_DOWN = 0,
    EMIE_LMOUSE_LEFT_UP = 3,
    EMIE_MOUSE_MOVED = 6
};

//! Keyboard input events, in SEvent::KeyInput.Event.
enum EKEY_INPUT_EVENT
{
    EKIE_KEY_PRESSED_DOWN = 0,
    EKIE_KEY_LEFT_UP = 1,
    //! The fullscreen toggle combination was pressed.
    EKIE_TOGGLE_FULLSCREEN = 4
};

//! Device events, in SEvent::DeviceEvent.Type.
enum EDEVICE_EVENT_TYPE
{
    //! The window switched between fullscreen and windowed mode.
    EDE_FULLSCREEN_TOGGLED = 3
};

//! Network device events, in SEvent::NetworkEvent.Type.
enum ENETWORK_EVENT_TYPE
{
    ENET_CONNECTED = 2,
    ENET_CONNECTION_FAILED = 3,
    ENET_DATA_RECEIVED = 6,
    ENET_DISCONNECTED = 8,
    //! Sent by CHTTPConnectionHandler with the downloaded content as Data.
    ENET_HTTP_DONE = 9,
    //! Sent by CHTTPConnectionHandler with an error text as Data.
    ENET_HTTP_ERROR = 10
};

struct SEvent
{
    EEVENT_TYPE EventType;
    union
    {
        struct
        {
            gui::IGUIElement* Caller;
            gui::EGUI_EVENT_TYPE EventType;
        } GUIEvent;

        struct
        {
            int X;
            int Y;
            float Wheel;
            // Not recovered yet.
            int Reserved[2];
            EMOUSE_INPUT_EVENT Event;
        } MouseInput;

        struct
        {
            wchar_t Char;
            EKEY_CODE Key;
            EKEY_INPUT_EVENT Event;
            bool Shift;
            bool Control;
        } KeyInput;

        struct
        {
            EDEVICE_EVENT_TYPE Type;
        } DeviceEvent;

        struct
        {
            int Type;
            int ClientId;
            //! Data size, or a negative error code with ENET_HTTP_ERROR.
            int Size;
            int Reserved;
            char* Data;
        } NetworkEvent;

        struct
        {
            int UserData1;
            int UserData2;
            int UserData3;
            void* UserPointer;
        } UserEvent;

        // Other event structs are not recovered yet. The Linux amd64 SEvent is 48 bytes, and
        // CHTTPConnectionHandler::OnEvent keeps several on the stack, so the union keeps that size.
        void* unrecovered[5];
    };
};

class CEventSubscriberList;

//! Interface of an object which can receive events.
class IEventReceiver
{
public:
    IEventReceiver()
        : SubscriberList(0)
    {
    }

    //! Called if an event happened. Returns true if the event was processed.
    virtual bool OnEvent(const SEvent& event) = 0;

    virtual ~IEventReceiver();

    void subscribe(CEventSubscriberList* list);

private:
    CEventSubscriberList* SubscriberList;
};

//! Forwards events to its subscribers. Partial: the subscriber storage is not recovered yet.
class CEventSubscriberList : public IEventReceiver
{
public:
    CEventSubscriberList();

    virtual bool OnEvent(const SEvent& event);

    virtual ~CEventSubscriberList();

    void addSubscriber(IEventReceiver* receiver);
    void removeSubscriber(IEventReceiver* receiver);
    //! Sends the event to the subscribers on the next update.
    void postDelayedEvent(const SEvent& event);
};

//! The subscriber list that game events are posted to.
extern CEventSubscriberList* gp_subscriberList;

} // end namespace event
} // end namespace ox

#endif
