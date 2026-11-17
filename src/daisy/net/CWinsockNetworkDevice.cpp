// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
#include "CWinsockNetworkDevice.h"
#include "../../ox/net/CVariablePacket.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
namespace daisy {
namespace net {
using namespace ox::event;
using ox::net::CVariablePacketBuilder;
using ox::net::CVariablePacketParser;
static void listenThread(void* parameter)
{
    CWinsockNetworkDevice* device = (CWinsockNetworkDevice*)parameter;
    ox::core::CThread::sleep(500);
    device->notifyServerUp();
    while (device->getLocalSocket()->Listen() != -1)
    {
        SocketObject* client = device->getNewClientSocket();
        if (device->getLocalSocket()->Accept(*client))
            device->notifyNewClient(client->Id);
        ox::core::CThread::sleep(2000);
    }
}
CWinsockNetworkDevice::CWinsockNetworkDevice(bool raw, const char* application)
    : Raw(raw), Disconnecting(false), SocketClosed(false), NewClient(0), Server(false),
      Key(0), NextClientId(1), MaxClients(1), Application(application), ListenThread(0),
      ReceiveBuffer(0), Receiver(0)
{
    LocalSocket = new SocketObject(this, 1000);
}
CWinsockNetworkDevice::~CWinsockNetworkDevice()
{
    disconnect();
    delete LocalSocket;
}
void CWinsockNetworkDevice::disconnect()
{
    Disconnecting = true;
    if (LocalSocket->Socket != -1 && !Server && !Raw)
    {
        short packet[2] = {0x4004, 4};
        if (sendMessage(1000, packet, 4, 0) > 0)
        {
            shutdown(LocalSocket->Socket, 2);
            int retries = 0;
            while (!SocketClosed)
            {
                ox::core::CThread::sleep(10);
                if (SocketClosed)
                    break;
                if (++retries == 20)
                    break;
            }
        }
    }
    LocalSocket->Disconnect();
    for (unsigned int i = 0; i < Clients.size(); ++i)
    {
        SClientData* client = Clients[i];
        if (client)
        {
            Clients[i] = 0;
            client->Socket.Disconnect();
            delete client;
        }
    }
    Clients.clear();
    if (ListenThread)
    {
        delete ListenThread;
        ListenThread = 0;
    }
    delete NewClient;
    NewClient = 0;
    delete [] ReceiveBuffer;
    ReceiveBuffer = 0;
}
const char* CWinsockNetworkDevice::getLocalIp()
{
    if (LocalIp.size() <= 0)
    {
        char hostname[80];
        if (gethostname(hostname, 80) != -1)
        {
            hostent* host = gethostbyname(hostname);
            if (host)
            {
                in_addr address;
                memcpy(&address, host->h_addr_list[0], sizeof(address));
                LocalIp = inet_ntoa(address);
            }
        }
    }
    return LocalIp.c_str();
}
int CWinsockNetworkDevice::createHost(IEventReceiver* receiver, const ox::net::SServerInfo& info)
{
    if (Raw)
        return -1;
    if (!LocalSocket->Bind(info.Port))
        return -1;
    Receiver = receiver;
    ListenThread = new ox::core::CThread(listenThread, this);
    MaxClients = info.MaxClients;
    Server = true;
    ServerName = info.Name;
    Password = info.Password;
    Disconnecting = false;
    SocketClosed = false;
    return 0;
}
void CWinsockNetworkDevice::notifyServerUp()
{
    SEvent event;
    event.EventType = EET_NETWORK_EVENT;
    event.NetworkEvent.Type = 0;
    event.NetworkEvent.ClientId = 1000;
    event.NetworkEvent.Size = 0;
    event.NetworkEvent.Data = (char*)Application.c_str();
    Receiver->OnEvent(event);
}
int CWinsockNetworkDevice::joinHost(IEventReceiver* receiver, const ox::net::SServerInfo& info)
{
    Receiver = receiver;
    if (!receiver)
        return -1;
    Server = false;
    Disconnecting = false;
    SocketClosed = false;
    ServerName = info.Name;
    Password = info.Password;
    ox::core::CString<char> address(info.Address.c_str());
    if (!LocalSocket->Connect(address.c_str(), info.Port))
    {
        SEvent event;
        event.EventType = EET_NETWORK_EVENT;
        event.NetworkEvent.Type = 3;
        event.NetworkEvent.ClientId = 0;
        event.NetworkEvent.Size = 0;
        event.NetworkEvent.Data = 0;
        Receiver->OnEvent(event);
        return -1;
    }
    if (Raw)
    {
        SEvent event;
        event.EventType = EET_NETWORK_EVENT;
        event.NetworkEvent.Type = 2;
        event.NetworkEvent.ClientId = 0;
        event.NetworkEvent.Size = 0;
        event.NetworkEvent.Data = 0;
        event.unrecovered[3] = 0;
        Receiver->OnEvent(event);
    }
    return 0;
}
int CWinsockNetworkDevice::sendMessage(int client, void* data, int size, int flags)
{
    if (!data)
        return -1;
    int length;
    if (flags & 1)
        length = strlen((char*)data);
    else
    {
        if (Raw)
            return -1;
        length = ((short*)data)[1];
        if (flags & 2)
        {
            unsigned int key = Key;
            if (client != 1000)
            {
                SClientData* found = findClient(client);
                if (!found)
                    return -1;
                key = found->Key;
            }
            unsigned char* bytes = (unsigned char*)data + 4;
            for (int i = 0; i < length - 4; ++i)
            {
                switch (i % 4)
                {
                case 0: bytes[i] ^= key >> 24; break;
                case 1: bytes[i] ^= key >> 16; break;
                case 2: bytes[i] ^= key >> 8; break;
                case 3: bytes[i] ^= key; key = (key >> 1) | (key << 31); break;
                }
            }
        }
    }
    if (client == 1000)
        return LocalSocket->Send((char*)data, length, 0);
    SClientData* found = findClient(client);
    if (found)
    {
        if (found->Socket.Send((char*)data, length, 0) != length)
            removeClient(client);
    }
    return -1;
}
SClientData* CWinsockNetworkDevice::findClient(int id)
{
    for (unsigned int i = 0; i < Clients.size(); ++i)
        if (Clients[i]->Socket.Id == id)
            return Clients[i];
    return 0;
}
void CWinsockNetworkDevice::removeClient(int id)
{
    for (std::vector<SClientData*>::iterator it = Clients.begin(); it != Clients.end(); ++it)
    {
        SClientData* client = *it;
        if (client->Socket.Id == id)
        {
            *it = 0;
            client->Socket.Disconnect();
            delete client;
            Clients.erase(it);
            return;
        }
    }
}
int CWinsockNetworkDevice::broadcastMessage(void* data, int size, int flags, int except)
{
    for (unsigned int i = 0; i < Clients.size(); ++i)
        if (Clients[i]->Socket.Id != except)
            sendMessage(Clients[i]->Socket.Id, data, size, flags);
    return 0;
}
void CWinsockNetworkDevice::decryptPacket(const void* data, int client)
{
    if (!data || Raw)
        return;
    unsigned int key = Key;
    int length = ((const short*)data)[1];
    if (client != 1000)
    {
        SClientData* found = findClient(client);
        if (!found)
            return;
        key = found->Key;
    }
    unsigned char* bytes = (unsigned char*)data + 4;
    for (int i = 0; i < length - 4; ++i)
    {
        switch (i % 4)
        {
        case 0: bytes[i] ^= key >> 24; break;
        case 1: bytes[i] ^= key >> 16; break;
        case 2: bytes[i] ^= key >> 8; break;
        case 3: bytes[i] ^= key; key = (key >> 1) | (key << 31); break;
        }
    }
}
void CWinsockNetworkDevice::kickClient(int client)
{
    removeClient(client);
}
SocketObject* CWinsockNetworkDevice::getLocalSocket()
{
    return LocalSocket;
}
SocketObject* CWinsockNetworkDevice::getNewClientSocket()
{
    delete NewClient;
    NewClient = new SClientData(this, NextClientId++);
    return &NewClient->Socket;
}
int CWinsockNetworkDevice::notifyReceive(SocketObject* socket, unsigned char* data, int size)
{
    if (Raw)
    {
        if (size > 0 && data)
        {
            SEvent event;
            event.EventType = EET_NETWORK_EVENT;
            event.NetworkEvent.Type = 6;
            event.NetworkEvent.ClientId = socket->Id;
            event.NetworkEvent.Size = size;
            event.NetworkEvent.Data = (char*)data;
            Receiver->OnEvent(event);
            return size;
        }
        return 0;
    }
    if ((unsigned int)size <= 3 || !data)
        return 0;
    short length = ((short*)data)[1];
    if (size < length)
        return 0;
    if ((unsigned short)length <= 3)
        return 0;
    if (length > 16384)
        return 0;
    if (!(((short*)data)[0] & 0x4000))
    {
        if (!Receiver)
            return length;
        SEvent event;
        event.EventType = EET_NETWORK_EVENT;
        event.NetworkEvent.Type = 6;
        event.NetworkEvent.ClientId = socket->Id;
        event.NetworkEvent.Size = length;
        event.NetworkEvent.Data = (char*)data;
        Receiver->OnEvent(event);
        return ((short*)data)[1];
    }
    CVariablePacketParser packet(data);
    switch (((short*)data)[0])
    {
    case 0x4000:
        if (!Server)
        {
            int id = packet.fetchInt();
            int status = packet.fetchInt();
            int key = packet.fetchInt();
            if (status == 0)
            {
                CVariablePacketBuilder response(0x4001);
                response.appendWideString(ServerName);
                response.appendString(Password);
                response.sendPacket(this, 1000, 0);
            }
            else if (status == 1)
            {
                Key = key;
                SEvent event;
                event.EventType = EET_NETWORK_EVENT;
                event.NetworkEvent.Type = 2;
                event.NetworkEvent.ClientId = 0;
                event.NetworkEvent.Size = id;
                event.NetworkEvent.Data = 0;
                event.unrecovered[3] = 0;
                Receiver->OnEvent(event);
            }
            else if (status == 2)
            {
                SEvent event;
                event.EventType = EET_NETWORK_EVENT;
                event.NetworkEvent.Type = 4;
                event.NetworkEvent.ClientId = 0;
                event.NetworkEvent.Size = 0;
                event.NetworkEvent.Data = 0;
                event.unrecovered[3] = 0;
                Receiver->OnEvent(event);
                disconnect();
            }
            else if (status == 3)
            {
                SEvent event;
                event.EventType = EET_NETWORK_EVENT;
                event.NetworkEvent.Type = 5;
                event.NetworkEvent.ClientId = 0;
                event.NetworkEvent.Size = 0;
                event.NetworkEvent.Data = 0;
                event.unrecovered[3] = 0;
                Receiver->OnEvent(event);
                disconnect();
            }
            else if (status == 4)
            {
                SEvent event;
                event.EventType = EET_NETWORK_EVENT;
                event.NetworkEvent.Type = 3;
                event.NetworkEvent.ClientId = 0;
                event.NetworkEvent.Size = 0;
                event.NetworkEvent.Data = 0;
                event.unrecovered[3] = 0;
                Receiver->OnEvent(event);
                disconnect();
            }
        }
        break;
    case 0x4001:
        if (Server)
        {
            SClientData* client = findClient(socket->Id);
            ox::core::CString<wchar_t> name;
            ox::core::CString<char> password;
            packet.fetchWideString(name);
            packet.fetchString(password);
            if (client)
            {
                client->Name = name;
                CVariablePacketBuilder response(0x4000);
                response.appendInt(socket->Id);
                if (Password != password)
                {
                    response.appendInt(3);
                    response.appendInt(0);
                    response.sendPacket(this, socket->Id, 0);
                    removeClient(socket->Id);
                }
                else if (client->Name.size() <= 0)
                {
                    response.appendInt(4);
                    response.appendInt(0);
                    response.sendPacket(this, socket->Id, 0);
                    removeClient(socket->Id);
                }
                else
                {
                    client->Key = (socket->Id * 337) | 0x24909445;
                    response.appendInt(1);
                    response.appendInt(client->Key);
                    response.sendPacket(this, socket->Id, 0);
                    SEvent event;
                    event.EventType = EET_NETWORK_EVENT;
                    event.NetworkEvent.Type = 1;
                    event.NetworkEvent.ClientId = socket->Id;
                    event.NetworkEvent.Size = socket->Id;
                    event.NetworkEvent.Data = (char*)client->Name.c_str();
                    event.unrecovered[3] = 0;
                    Receiver->OnEvent(event);
                }
            }
        }
        break;
    case 0x4002:
        if (!Server)
        {
            short response[2] = {0x4003, 4};
            sendMessage(1000, response, 4, 0);
        }
        break;
    case 0x4005:
        {
            if (!ReceiveBuffer)
                ReceiveBuffer = new unsigned char[32768];
            int remaining = Compressor.decompress(ReceiveBuffer, data + 4, ((short*)data)[1] - 4);
            unsigned char* position = ReceiveBuffer;
            while (remaining > 0)
            {
                int consumed = notifyReceive(socket, position, remaining);
                if (consumed == 0)
                    break;
                remaining -= consumed;
                position += consumed;
            }
        }
        break;
    }
    return ((short*)data)[1];
}
void CWinsockNetworkDevice::notifySocketClosed(int id)
{
    SEvent event;
    event.EventType = EET_NETWORK_EVENT;
    event.NetworkEvent.ClientId = id;
    event.NetworkEvent.Size = 0;
    event.NetworkEvent.Data = 0;
    if (!Server)
        event.NetworkEvent.Type = 8;
    else
    {
        for (std::vector<SClientData*>::iterator it = Clients.begin(); it != Clients.end(); ++it)
        {
            SClientData* client = *it;
            if (client && client->Socket.Id == id)
            {
                event.NetworkEvent.Type = 7;
                event.unrecovered[3] = client->UserData;
                delete client;
                *it = 0;
                Clients.erase(it);
                break;
            }
        }
    }
    if (!Disconnecting)
        Receiver->OnEvent(event);
    SocketClosed = true;
}
void CWinsockNetworkDevice::notifyNewClient(int id)
{
    if (NewClient && NewClient->Socket.Id == id)
    {
        Clients.push_back(NewClient);
        NewClient = 0;
    }
    struct
    {
        short Type;
        short Length;
        int Id;
        int Status;
        int Key;
    } packet;
    packet.Type = 0x4000;
    packet.Length = 16;
    packet.Id = id;
    if (MaxClients > (int)Clients.size())
    {
        packet.Status = 0;
        sendMessage(id, &packet, 16, 0);
    }
    else
    {
        packet.Status = 2;
        sendMessage(id, &packet, 16, 0);
        removeClient(id);
    }
}
void CWinsockNetworkDevice::pollDevice()
{
}
} // end namespace net
} // end namespace daisy
