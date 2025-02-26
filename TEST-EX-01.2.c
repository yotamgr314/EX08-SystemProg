#define _POSIX_C_SOURCE 200112L // to use CLOCK_REALTIME constant

#include <stdio.h>
#include <pthread.h> // for pthread API
#include <time.h> // Time-related functions and types e.g., clock_gettime, struct timespec
#include <unistd.h> // for sleep()
#include <sys/time.h>
#include <errno.h>

typedef struct Events {

    int timeout_ms;
    void *(* callback_func)(void*);
    void* arg;
    struct Events* nextEvent

}Events;


typedef struct Shared_event_queue {

Events* eventHead;
pthread_mutex_t queue_mutex;
pthread_cond_t new_event_cond;

}Shared_event_queue;


// sample callback func

void sample_bacllback(void * arg)
{
    char * message = (char*)arg;
    printf("timer expired: %s\n", message);
}



void add_event (Shared_event_queue* shared_event_queue, int timeout_ms, void(*callback_func)(void *),void *arg)
{
    Events* newEvent = (Events*)malloc(sizeof(Events));

    newEvent->timeout_ms = timeout_ms;
    newEvent->callback_func = callback_func;
    newEvent->arg = arg;
    newEvent->nextEvent = NULL;

/* 
    // insert the new event to the shared_event_queue in its right order.
    // first lock the shared event_queue recource.
    pthread_mutex_lock(&shared_event_queue->queue_mutex);
 */

    if(shared_event_queue->eventHead == NULL) // if there queue is empty
    {
        shared_event_queue->eventHead = newEvent;
    }

    Events* currEvent = shared_event_queue->eventHead;
    while(currEvent->nextEvent != NULL && currEvent->nextEvent->timeout_ms < newEvent->timeout_ms)
    {
        currEvent = currEvent->nextEvent;
    }
    newEvent->nextEvent = currEvent->nextEvent;
    currEvent->nextEvent = newEvent;

    pthread_cond_signal(&shared_event_queue->new_event_cond);

}


int main () {

    Shared_event_queue shared_event_queue = {NULL,PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};

    pthread_t consumerThreads;



    return 0;

}