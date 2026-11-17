// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
#include "SocketObject.h"
#include "CWinsockNetworkDevice.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
namespace daisy {
namespace net {
SocketObject::SocketObject(CWinsockNetworkDevice* device, int id)
{
    Device = device;
    Id = id;
    ReceiveThread = 0;
    ThreadStarted = false;
    Running = 0;
    Socket = -1;
}
SocketObject::~SocketObject()
{
    Disconnect();
}
void SocketObject::Disconnect()
{
    if (Running && *Running)
    {
        **Running = false;
        Running = 0;
    }
    if (Socket != -1)
    {
        close(Socket);
        Socket = -1;
    }
    if (ReceiveThread)
        delete ReceiveThread;
    ReceiveThread = 0;
}
void SocketObject::thrReceiveThread(void* parameter)
{
    SocketObject* socket = (SocketObject*)parameter;
    int handle = socket->Socket;
    CWinsockNetworkDevice* device = socket->Device;
    int id = socket->Id;
    unsigned char* buffer = new unsigned char[65536];
    bool* running = new bool(true);
    socket->reportThreadStarted(&running);
    int buffered = 0;
    while (*running)
    {
        int received = recv(handle, buffer + buffered, 65536 - buffered, 0);
        int size = buffered + received;
        if (received == 0)
            received = -1;
        buffered = 0;
        int position = 0;
        while (position < size && *running)
        {
            int consumed = device->notifyReceive(socket, buffer + position, size - position);
            if (consumed < 0)
            {
                buffered = size - position;
                memcpy(buffer, buffer + position, buffered);
                break;
            }
            position += consumed;
            if (consumed == 0)
            {
                buffered = size - position;
                memcpy(buffer, buffer + position, buffered);
                break;
            }
        }
        if (received == -1)
            break;
    }
    if (*running)
        socket->reportThreadStopped();
    if (device && *running)
        device->notifySocketClosed(id);
    delete running;
    running = 0;
    delete [] buffer;
}
void SocketObject::reportThreadStarted(bool** running)
{
    Lock.enter();
    ThreadStarted = true;
    Running = running;
    Lock.leave();
}
void SocketObject::reportThreadStopped()
{
    Lock.enter();
    Running = 0;
    Lock.leave();
}
bool SocketObject::isThreadStarted()
{
    Lock.enter();
    bool started = ThreadStarted;
    Lock.leave();
    return started;
}
bool SocketObject::Connect(const char* address, int port)
{
    sockaddr_in endpoint = {};
    endpoint.sin_family = AF_INET;
    endpoint.sin_addr.s_addr = inet_addr(address);
    if (endpoint.sin_addr.s_addr == INADDR_NONE)
    {
        hostent* host = gethostbyname(address);
        if (!host)
            return false;
        endpoint.sin_addr = *(in_addr*)host->h_addr_list[0];
    }
    endpoint.sin_port = htons(port);
    Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (Socket == -1)
        return false;
    if (connect(Socket, (sockaddr*)&endpoint, sizeof(endpoint)) == -1)
    {
        Disconnect();
        return false;
    }
    ThreadStarted = false;
    ReceiveThread = new ox::core::CThread(thrReceiveThread, this);
    while (!isThreadStarted())
        ox::core::CThread::sleep(1);
    return true;
}
int SocketObject::Bind(int port)
{
    Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (Socket == -1)
        return false;
    sockaddr_in endpoint = {};
    endpoint.sin_family = AF_INET;
    endpoint.sin_addr.s_addr = INADDR_ANY;
    endpoint.sin_port = htons(port);
    if (bind(Socket, (sockaddr*)&endpoint, sizeof(endpoint)) == -1)
    {
        Disconnect();
        return false;
    }
    return true;
}
int SocketObject::Listen()
{
    return listen(Socket, 32);
}
bool SocketObject::Accept(SocketObject& client)
{
    sockaddr_in endpoint;
    socklen_t length = sizeof(endpoint);
    client.Socket = accept(Socket, (sockaddr*)&endpoint, &length);
    if (client.Socket == -1)
        return false;
    client.ThreadStarted = false;
    ReceiveThread = new ox::core::CThread(thrReceiveThread, &client);
    while (!client.ThreadStarted)
        ox::core::CThread::sleep(1);
    return true;
}
int SocketObject::Recv(char* data, int size, int flags)
{
    return recv(Socket, data, size, flags);
}
int SocketObject::Send(char* data, int size, int flags)
{
    return send(Socket, data, size, flags);
}
} // end namespace net
} // end namespace daisy
