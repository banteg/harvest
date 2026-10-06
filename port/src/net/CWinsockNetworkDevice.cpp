#include "net/CWinsockNetworkDevice.h"

#include "ox/event/IEventReceiver.h"

namespace daisy {
namespace net {

CWinsockNetworkDevice::CWinsockNetworkDevice(bool flag, const char* name)
{
}

CWinsockNetworkDevice::~CWinsockNetworkDevice()
{
}

int CWinsockNetworkDevice::fail(ox::event::IEventReceiver* receiver)
{
    if (receiver)
    {
        Lock.enter();
        FailedReceivers.push_back(receiver);
        Lock.leave();
    }
    return -1;
}

int CWinsockNetworkDevice::createHost(ox::event::IEventReceiver* receiver, const ox::net::SServerInfo& info)
{
    return fail(receiver);
}

int CWinsockNetworkDevice::joinHost(ox::event::IEventReceiver* receiver, const ox::net::SServerInfo& info)
{
    return fail(receiver);
}

void CWinsockNetworkDevice::disconnect()
{
}

const char* CWinsockNetworkDevice::getLocalIp()
{
    return "127.0.0.1";
}

int CWinsockNetworkDevice::sendMessage(int messageId, void* data, int size, int flags)
{
    return -1;
}

int CWinsockNetworkDevice::broadcastMessage(void* data, int size, int messageId, int flags)
{
    return -1;
}

void CWinsockNetworkDevice::decryptPacket(const void* data, int size)
{
}

void CWinsockNetworkDevice::kickClient(int client)
{
}

//! Reports the failed connections outside the lock: a receiver may connect again from its handler.
void CWinsockNetworkDevice::pollDevice()
{
    Lock.enter();
    std::vector<ox::event::IEventReceiver*> failed;
    failed.swap(FailedReceivers);
    Lock.leave();

    for (unsigned int i = 0; i < failed.size(); ++i)
    {
        ox::event::SEvent event;
        event.EventType = ox::event::EET_NETWORK_EVENT;
        event.NetworkEvent.Type = ox::event::ENET_CONNECTION_FAILED;
        event.NetworkEvent.ClientId = 0;
        event.NetworkEvent.Size = 0;
        event.NetworkEvent.Reserved = 0;
        event.NetworkEvent.Data = 0;
        failed[i]->OnEvent(event);
    }
}

} // end namespace net
} // end namespace daisy
