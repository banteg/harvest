// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
#ifndef DAISY_NET_CWINSOCKNETWORKDEVICE_H
#define DAISY_NET_CWINSOCKNETWORKDEVICE_H
#include "../../ox/net/INetworkDevice.h"
#include "../../ox/event/IEventReceiver.h"
#include "../../ox/algo/CSimplePress.h"
#include "SocketObject.h"
#include <vector>
namespace daisy {
namespace net {
struct SClientData
{
    SClientData(CWinsockNetworkDevice* device, int id)
        : Socket(device, id), UserData(0) {}
    SocketObject Socket;
    ox::core::CString<wchar_t> Name;
    void* UserData;
    unsigned int Key;
};
class CWinsockNetworkDevice : public ox::net::INetworkDevice
{
public:
    CWinsockNetworkDevice(bool raw, const char* application);
    virtual ~CWinsockNetworkDevice();
    virtual int createHost(ox::event::IEventReceiver* receiver, const ox::net::SServerInfo& info);
    virtual int joinHost(ox::event::IEventReceiver* receiver, const ox::net::SServerInfo& info);
    virtual void disconnect();
    virtual const char* getLocalIp();
    virtual int sendMessage(int client, void* data, int size, int flags);
    virtual int broadcastMessage(void* data, int size, int flags, int except);
    virtual void decryptPacket(const void* data, int client);
    virtual void kickClient(int client);
    virtual void pollDevice();
    SClientData* findClient(int id);
    void removeClient(int id);
    SocketObject* getLocalSocket();
    SocketObject* getNewClientSocket();
    int notifyReceive(SocketObject* socket, unsigned char* data, int size);
    void notifySocketClosed(int id);
    void notifyNewClient(int id);
    void notifyServerUp();
private:
    ox::core::CString<char> LocalIp;
    bool Raw;
    bool Disconnecting;
    bool SocketClosed;
    SocketObject* LocalSocket;
    std::vector<SClientData*> Clients;
    SClientData* NewClient;
    bool Server;
    unsigned int Key;
    int NextClientId;
    int MaxClients;
    ox::core::CString<wchar_t> ServerName;
    ox::core::CString<char> Password;
    ox::core::CString<char> Application;
    ox::core::CThread* ListenThread;
    ox::algo::CSimplePress Compressor;
    unsigned char* ReceiveBuffer;
    ox::event::IEventReceiver* Receiver;
};
} // end namespace net
} // end namespace daisy
#endif
