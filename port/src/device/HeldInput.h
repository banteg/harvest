#ifndef PORT_DEVICE_HELDINPUT_H
#define PORT_DEVICE_HELDINPUT_H

#include <SDL3/SDL_events.h>
#include <string>

namespace port {

//! Input kept across SDL event pumps while a game state loads.
class HeldInput
{
public:
    explicit HeldInput(const SDL_Event& event) : Event(event)
    {
        if (Event.type == SDL_EVENT_TEXT_INPUT)
        {
            Text = event.text.text;
            Event.text.text = 0;
        }
    }

    SDL_Event getEvent() const
    {
        SDL_Event event = Event;
        if (event.type == SDL_EVENT_TEXT_INPUT)
            event.text.text = Text.c_str();
        return event;
    }

private:
    SDL_Event Event;
    std::string Text;
};

} // end namespace port

#endif
