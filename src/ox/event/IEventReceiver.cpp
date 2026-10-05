// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "IEventReceiver.h"
// The native unit has an iostream static initializer, as do the other ox units.
#include <iostream> // IWYU pragma: keep

namespace ox {
namespace event {

CEventSubscriberList* gp_subscriberList = 0;

IEventReceiver::~IEventReceiver()
{
    if (SubscriberList)
        gp_subscriberList->removeSubscriber(this);
}

void CEventSubscriberList::removeSubscriber(IEventReceiver* receiver)
{
    for (std::vector<IEventReceiver*>::iterator it = Subscribers.begin(); it != Subscribers.end(); ++it)
    {
        if (*it == receiver)
        {
            Subscribers.erase(it);
            return;
        }
    }
}

void IEventReceiver::subscribe(CEventSubscriberList* list)
{
    SubscriberList = list;
    list->addSubscriber(this);
}

void CEventSubscriberList::addSubscriber(IEventReceiver* receiver)
{
    Subscribers.push_back(receiver);
}

CEventSubscriberList::CEventSubscriberList()
    : CurrentDelayedEvents(0)
{
}

CEventSubscriberList::~CEventSubscriberList()
{
}

bool CEventSubscriberList::OnEvent(const SEvent& event)
{
    for (std::vector<IEventReceiver*>::iterator it = Subscribers.begin(); it != Subscribers.end(); ++it)
    {
        if ((*it)->OnEvent(event))
            return true;
    }
    return false;
}

void CEventSubscriberList::postDelayedEvent(const SEvent& event)
{
    DelayedEventsLock.enter();
    DelayedEvents[CurrentDelayedEvents].push_back(event);
    DelayedEventsLock.leave();
}

void CEventSubscriberList::executeDelayedEvents()
{
    DelayedEventsLock.enter();
    int current = CurrentDelayedEvents;
    CurrentDelayedEvents = current ^ 1;
    DelayedEventsLock.leave();

    std::vector<SEvent>& events = DelayedEvents[current];
    if (!events.empty())
    {
        for (unsigned int i = 0; i < events.size(); ++i)
            OnEvent(events[i]);
        events.clear();
    }
}

} // end namespace event
} // end namespace ox
