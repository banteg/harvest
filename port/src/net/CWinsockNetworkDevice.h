// The port's network device. CIrrDeviceStub::createNetworkDevice creates it by this name; the port
// build includes this declaration there instead of the matching build's placeholder.

#ifndef PORT_NET_CWINSOCKNETWORKDEVICE_H
#define PORT_NET_CWINSOCKNETWORKDEVICE_H

#include <vector>
#include "ox/core/CCriticalSection.h"
#include "ox/net/INetworkDevice.h"

namespace daisy {
namespace net {

//! A network device that never connects: the original's only use is the online highscores
//! (CHTTPConnectionHandler), whose server is gone. joinHost and createHost fail at once, and the
//! next pollDevice reports ENET_CONNECTION_FAILED to the receiver, on the main thread
//! (CHTTPConnectionHandler calls joinHost from its own thread), so the highscore screens show their
//! "Unable to connect" error. Sending does nothing.
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
    //! Fails a connection attempt: the receiver hears about it on the next pollDevice.
    int fail(ox::event::IEventReceiver* receiver);

    //! Receivers owed an ENET_CONNECTION_FAILED; written by any thread, read by pollDevice.
    std::vector<ox::event::IEventReceiver*> FailedReceivers;
    ox::core::CCriticalSection Lock;
};

} // end namespace net
} // end namespace daisy

#endif
