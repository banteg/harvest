// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
#ifndef DAISY_NET_SOCKETOBJECT_H
#define DAISY_NET_SOCKETOBJECT_H
#include "../../ox/core/CCriticalSection.h"
#include "../../ox/core/CThread.h"
namespace daisy {
namespace net {
class CWinsockNetworkDevice;
class SocketObject
{
public:
    SocketObject(CWinsockNetworkDevice* device, int id);
    ~SocketObject();
    void Disconnect();
    static void thrReceiveThread(void* parameter);
    void reportThreadStarted(bool** running);
    void reportThreadStopped();
    bool Connect(const char* address, int port);
    bool isThreadStarted();
    int Bind(int port);
    int Listen();
    bool Accept(SocketObject& client);
    int Recv(char* data, int size, int flags);
    int Send(char* data, int size, int flags);
    ox::core::CCriticalSection Lock;
    int Socket;
    ox::core::CThread* ReceiveThread;
    int ReceiveSize;
    bool ThreadStarted;
    bool** Running;
    CWinsockNetworkDevice* Device;
    int Id;
};
} // end namespace net
} // end namespace daisy
#endif
