// No game data or window. Exercise input deferred across frames and the offline HTTP device.
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include "device/HeldInput.h"
#include "harvest/gui/CProfileScreen.h"
#include "ox/IOxDevice.h"
#include "ox/net/CHTTPConnectionHandler.h"

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "Runtime check failed: %s\n", #c); return 1; } } while (0)

class ErrorReceiver : public ox::event::IEventReceiver
{
public:
    ErrorReceiver() : Errors(0) {}
    bool OnEvent(const ox::event::SEvent& event)
    {
        if (event.EventType == ox::event::EET_NETWORK_EVENT && event.NetworkEvent.Type == ox::event::ENET_HTTP_ERROR &&
            std::strcmp(event.NetworkEvent.Data, "Unable to connect") == 0)
            ++Errors;
        return true;
    }
    int Errors;
};

int main()
{
    std::vector<port::HeldInput> held;
    SDL_Event event = {};
    char text[] = "Zoë";
    event.type = SDL_EVENT_TEXT_INPUT;
    event.text.text = text;
    held.push_back(port::HeldInput(event));
    std::memset(text, '!', sizeof(text) - 1);
    event.type = SDL_EVENT_KEY_UP;
    event.key.key = SDLK_RETURN;
    held.push_back(port::HeldInput(event));
    CHECK(std::strcmp(held[0].getEvent().text.text, "Zoë") == 0);
    CHECK(held[1].getEvent().type == SDL_EVENT_KEY_UP && held[1].getEvent().key.key == SDLK_RETURN);

    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    ox::IOxDevice* device = ox::createDevice(ox::video::EDT_NULL, 0, L"0.7");
    {
        // The profile editor always trims the optional group, including when it is blank.
        harvest::gui::CProfileScreen screen(device);
        ox::core::CString<wchar_t> group;
        screen.stripIllegalCharacters(group);
        CHECK(group.size() == 0);
        group = L"   ";
        screen.stripIllegalCharacters(group);
        CHECK(group.size() == 0);
        group = L" Space;Crew\\  ";
        screen.stripIllegalCharacters(group);
        CHECK(group == L"Space_Crew\\");
    }
    {
        ErrorReceiver receiver;
        ox::net::CHTTPConnectionHandler request;
        request.init(device, &receiver);
        CHECK(request.doGet("/scores", L"offline", 80));
        CHECK(!request.doGet("/scores", L"offline", 80));
        CHECK(receiver.Errors == 0);
        device->pollNetworkDevices();
        CHECK(receiver.Errors == 1);
        // A cancelled request cannot complete or interfere with a later attempt.
        CHECK(request.doGet("/scores", L"offline", 80));
        request.disconnect();
        CHECK(request.doGet("/scores", L"offline", 80));
        {
            ox::net::CHTTPConnectionHandler closed;
            closed.init(device, &receiver);
            CHECK(closed.doGet("/scores", L"offline", 80));
        }
        device->pollNetworkDevices();
        CHECK(receiver.Errors == 2);
    }
    device->drop();
    SDL_Quit();
    return 0;
}
