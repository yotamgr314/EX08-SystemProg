Events* currEvent = shared_event_queue->eventHead;
while(currEvent->nextEvent != NULL && currEvent->nextEvent->timeout_ms < newEvent->timeout_ms)
{
    currEvent = currEvent->nextEvent;
}
newEvent->nextEvent = currEvent->nextEvent;
currEvent->nextEvent = newEvent;

