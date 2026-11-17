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
        FailedReceivers.push_back(receiver);
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

void CWinsockNetworkDevice::cancelPending(ox::event::IEventReceiver* receiver)
{
    for (size_t i = 0; i < FailedReceivers.size(); ++i)
        if (FailedReceivers[i] == receiver)
            FailedReceivers[i] = 0;
}

//! A callback may cancel another receiver or queue a request for the next poll.
void CWinsockNetworkDevice::pollDevice()
{
    const size_t count = FailedReceivers.size();
    for (size_t i = 0; i < count; ++i)
    {
        if (!FailedReceivers[i])
            continue;
        ox::event::SEvent event;
        event.EventType = ox::event::EET_NETWORK_EVENT;
        event.NetworkEvent.Type = ox::event::ENET_CONNECTION_FAILED;
        event.NetworkEvent.ClientId = 0;
        event.NetworkEvent.Size = 0;
        event.NetworkEvent.Reserved = 0;
        event.NetworkEvent.Data = 0;
        FailedReceivers[i]->OnEvent(event);
    }
    FailedReceivers.erase(FailedReceivers.begin(), FailedReceivers.begin() + count);
}

} // end namespace net
} // end namespace daisy
