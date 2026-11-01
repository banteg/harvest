// Recovered for Harvest; not the original source. Virtual order and parameter types follow the Mac
// 1.18 vtable of daisy::net::CWinsockNetworkDevice; return types other than joinHost's are provisional.

#ifndef OX_NET_INETWORKDEVICE_H
#define OX_NET_INETWORKDEVICE_H

#include "../IUnknown.h"
#include "../core/CString.h"

namespace ox {

namespace event { class IEventReceiver; }

namespace net {

//! Server description used to host or join. Only Address and Port have confirmed meanings;
//! the other names are provisional.
struct SServerInfo
{
    core::CString<wchar_t> Name;
    core::CString<wchar_t> Address;
    int Port;
    int MaxClients;
    core::CString<wchar_t> Description;
    core::CString<char> Password;
};

//! A network connection that reports to an event receiver.
class INetworkDevice : public IUnknown
{
public:
    virtual ~INetworkDevice() {}

    virtual int createHost(event::IEventReceiver* receiver, const SServerInfo& info) = 0;
    virtual int joinHost(event::IEventReceiver* receiver, const SServerInfo& info) = 0;
    virtual void disconnect() = 0;
    virtual const char* getLocalIp() = 0;
    virtual int sendMessage(int messageId, void* data, int size, int flags) = 0;
    virtual int broadcastMessage(void* data, int size, int messageId, int flags) = 0;
    virtual void decryptPacket(const void* data, int size) = 0;
    virtual void kickClient(int client) = 0;
    virtual void pollDevice() = 0;
#ifdef HARVEST_PORT
    //! Cancels this receiver's pending offline response before it disconnects or is destroyed.
    virtual void cancelPending(event::IEventReceiver* receiver) = 0;
#endif
};

} // end namespace net
} // end namespace ox

#endif
