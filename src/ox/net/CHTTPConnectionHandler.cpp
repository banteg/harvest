// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CHTTPConnectionHandler.h"
#include "INetworkDevice.h"
#include "../IOxDevice.h"
#include "../core/CThread.h"
#include "../core/CStringFunctions.h"
#include <cstdlib>
// The object has an iostream static initializer; the header that pulled it in is not identified yet.
#include <iostream>

namespace ox {
namespace net {

namespace
{
    enum E_HTTP_STATE
    {
        EHS_IDLE = 0,
        EHS_CONNECTING = 1,
        EHS_HEADER = 2,
        EHS_CHUNK_SIZE = 3,
        EHS_CHUNK_DATA = 4,
        EHS_BODY = 5
    };
}

CHTTPConnectionHandler::CHTTPConnectionHandler()
    : NetworkDevice(0), Connected(false), State(EHS_IDLE), Thread(0)
{
    Content = "";
}

CHTTPConnectionHandler::~CHTTPConnectionHandler()
{
#ifdef HARVEST_PORT
    disconnect();
#endif
    if (NetworkDevice)
        NetworkDevice->drop();

    delete Thread;
    Thread = 0;
}

void CHTTPConnectionHandler::init(IOxDevice* device, event::IEventReceiver* receiver)
{
    Receiver = receiver;
    NetworkDevice = device->createNetworkDevice("HTTPPrimDev", 1);
    NetworkDevice->grab();
}

bool CHTTPConnectionHandler::isConnected()
{
    Lock.enter();
    bool connected = Connected;
    Lock.leave();
    return connected;
}

void CHTTPConnectionHandler::disconnect()
{
    Connected = false;
    State = EHS_IDLE;
    if (NetworkDevice)
    {
#ifdef HARVEST_PORT
        NetworkDevice->cancelPending(this);
#endif
        NetworkDevice->disconnect();
    }
}

bool CHTTPConnectionHandler::doGet(const core::CString<char>& file, const core::CString<wchar_t>& host,
                                   unsigned int port)
{
    Lock.enter();
#ifdef HARVEST_PORT
    if (State != EHS_IDLE)
    {
        Lock.leave();
        return false;
    }
#else
    // returns without leaving the lock, as the original does
    if (State != EHS_IDLE)
        return false;
#endif

    State = EHS_CONNECTING;
    delete Thread;
    Thread = 0;

    Content = "";
    Buffer = "";
    ContentLength = 0;
    File = file;
    Host = core::CStringFunctions::wideToAnsi(host);
    Port = port;

#ifdef HARVEST_PORT
    // The offline network device queues its failure for the main loop. No worker is needed
    // (and the browser build has no worker threads).
    joinThread(this);
#else
    Thread = new core::CThread(joinThread, this);
#endif
    Lock.leave();
    return true;
}

void CHTTPConnectionHandler::joinThread(void* handler)
{
    CHTTPConnectionHandler* self = (CHTTPConnectionHandler*)handler;

    SServerInfo info;
    info.Address = core::CString<wchar_t>(self->Host.c_str());
    info.Port = self->Port;
    self->NetworkDevice->joinHost(self, info);
}

bool CHTTPConnectionHandler::OnEvent(const event::SEvent& event)
{
    if (event.EventType != event::EET_NETWORK_EVENT)
        return false;

    Lock.enter();
    bool closeConnection = false;

    switch (event.NetworkEvent.Type)
    {
    case event::ENET_CONNECTED:
    {
        Connected = true;
        State = EHS_HEADER;

        core::CString<char> request = "GET ";
        request += File;
        request += " HTTP/1.1\r\n";
        request += "Host: ";
        request += Host;
        request += "\r\n";
        request += "Accept: text/plain, text/html\r\n";
        request += "Connection: close\r\n\r\n";

        NetworkDevice->sendMessage(1000, (void*)request.c_str(), request.size() + 1, 1);
        break;
    }

    case event::ENET_CONNECTION_FAILED:
    {
        event::SEvent e;
        e.EventType = event::EET_NETWORK_EVENT;
        e.NetworkEvent.Type = event::ENET_HTTP_ERROR;
        e.NetworkEvent.Data = (char*)"Unable to connect";
        Receiver->OnEvent(e);
        closeConnection = true;
        break;
    }

    case event::ENET_DATA_RECEIVED:
        if (State != EHS_IDLE)
        {
            // the data is not zero-terminated: take the last byte out, terminate, and put it back
            char last = event.NetworkEvent.Data[event.NetworkEvent.Size - 1];
            event.NetworkEvent.Data[event.NetworkEvent.Size - 1] = 0;
            core::CString<char> data = event.NetworkEvent.Data;
            data.append(last);
            Buffer.append(data);

            for (;;)
            {
                if (State == EHS_HEADER)
                {
                    int end = Buffer.findNext("\r\n\r\n", 0);
                    if (end != -1)
                    {
                        core::CString<char> header = Buffer.subString(0, end);
                        Buffer = Buffer.subStringToEnd(end + 4);
                        ContentLength = getContentLength(header);

                        if (ContentLength >= 0)
                        {
                            State = EHS_BODY;
                            continue;
                        }
                        else if (ContentLength == -1)
                        {
                            State = EHS_CHUNK_SIZE;
                            continue;
                        }
                        else
                        {
                            event::SEvent e;
                            e.EventType = event::EET_NETWORK_EVENT;
                            e.NetworkEvent.Type = event::ENET_HTTP_ERROR;
                            e.NetworkEvent.Size = -ContentLength;
                            e.NetworkEvent.Data = (char*)"Error code";
                            Receiver->OnEvent(e);
                            State = EHS_IDLE;
                            closeConnection = true;
                        }
                    }
                    break;
                }
                else if (State == EHS_CHUNK_SIZE)
                {
                    if (Buffer.size() >= 3)
                    {
                        int end = Buffer.findNext("\r\n", 0);
                        if (end != -1)
                        {
                            ContentLength = strtol(Buffer.c_str(), 0, 16);
                            if (ContentLength == 0)
                            {
                                State = EHS_IDLE;
                                event::SEvent e;
                                e.EventType = event::EET_NETWORK_EVENT;
                                e.NetworkEvent.Type = event::ENET_HTTP_DONE;
                                e.NetworkEvent.Data = (char*)Content.c_str();
                                Receiver->OnEvent(e);
                                closeConnection = true;
                            }
                            else
                            {
                                Buffer = Buffer.subStringToEnd(end + 2);
                                State = EHS_CHUNK_DATA;
                                continue;
                            }
                        }
                    }
                    break;
                }
                else if (State == EHS_CHUNK_DATA)
                {
                    if (Buffer.size() >= ContentLength + 2)
                    {
                        Content.append(Buffer.subString(0, ContentLength));
                        Buffer = Buffer.subStringToEnd(ContentLength + 2);
                        State = EHS_CHUNK_SIZE;
                        continue;
                    }
                    break;
                }
                else if (State == EHS_BODY)
                {
                    if (Buffer.size() >= ContentLength)
                    {
                        State = EHS_IDLE;
                        event::SEvent e;
                        e.EventType = event::EET_NETWORK_EVENT;
                        e.NetworkEvent.Type = event::ENET_HTTP_DONE;
                        e.NetworkEvent.Data = (char*)Buffer.c_str();
                        Receiver->OnEvent(e);
                        closeConnection = true;
                    }
                    break;
                }
            }
        }
        break;

    case event::ENET_DISCONNECTED:
        if (State != EHS_IDLE)
        {
            // EventType is left unset, as in both builds
            event::SEvent e;
            e.NetworkEvent.Type = event::ENET_HTTP_ERROR;
            e.NetworkEvent.Data = (char*)"Interrupted";
            Receiver->OnEvent(e);
        }
        closeConnection = true;
        break;
    }

    Lock.leave();
    if (closeConnection)
        disconnect();
    return true;
}

int CHTTPConnectionHandler::getContentLength(core::CString<char>& header)
{
    TArray<core::CString<char> > lines;
    core::CString<char> separator = "\n";
    core::splitString(lines, header, separator);
    int count = lines.size();

    core::CString<char> key = "Content-Length:";

    // "HTTP/1.1 200 OK"
    int status = strtol(lines[0].c_str() + 9, 0, 10);
    if (status != 200)
        return -status;

    for (int i = 0; i < count; ++i)
    {
        if (lines[i].startsWith(key))
            return strtol(lines[i].subStringToEnd(key.size()).c_str(), 0, 10);
    }

    return -1;
}

} // end namespace net
} // end namespace ox
