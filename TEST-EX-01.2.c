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

int flag = 1;

void *consumer_timer_thread_func(void* arg)
{
    Shared_event_queue* Shared_event__queue = (Shared_event_queue*)arg;

    while(flag)
    {
        pthread_mutex_lock(&Shared_event__queue->queue_mutex); // locking the shared queue recource.

        while(Shared_event__queue->eventHead == NULL)
        {
            pthread_cond_wait(&Shared_event__queue->new_event_cond, &Shared_event__queue->queue_mutex); // waiting for a signal that new event has been added to the events_queue
        }

        // if we are here it means the thread as woken up, unlocked the mutex, and also the events head is stil not empty.
        // check is the time of the head_event has expired, if yes execute it, if no go to sleep for efresh time.

        // calculate the expire time by current time + event head expire time, and trasforming it into seconds and nano seconds.
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        now.tv_sec += Shared_event__queue->eventHead->timeout_ms / 1000;
        now.tv_nsec += (Shared_event__queue->eventHead->timeout_ms % 1000) * 1000000;
        if (now.tv_nsec >= 1000000000) {
            now.tv_sec++;
            now.tv_nsec -= 1000000000;
        }


        int res = pthread_cond_timedwait(&Shared_event__queue->new_event_cond,&Shared_event__queue->queue_mutex,&now); // sends the thread to sleep al tnai or signal of the expiration time, the mutex will get unlocked while it sleeps, and locked while it wakes up automaticly.
        if(res == ETIMEDOUT) // meaning the thread as slept for the exact expiration time of the event, the mutex will get unlocked
        {
            Events* timed_out_head_event =  Shared_event__queue->eventHead;

            Shared_event__queue->eventHead = timed_out_head_event->nextEvent;// update the shared_event_queue head to the new head of the queueu.
            // execute the callback function of the expired timed head event.
            timed_out_head_event->callback_func;

            free(timed_out_head_event);

        } else {
            pthread_mutex_unlock(&Shared_event__queue->queue_mutex);
        }

    }
    return NULL;
    
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

    pthread_create(&consumerThreads,NULL,&consumer_timer_thread_func, &shared_event_queue);


    add_event (&shared_event_queue,2000,sample_bacllback,"hello event1");\

    // add one before closest timeout
    add_event (&shared_event_queue,1000,sample_bacllback,"hello event2");

    // add one after closest timeout
    add_event (&shared_event_queue,3000,sample_bacllback,"hello event3");


    pthread_join(consumerThreads, NULL);


    return 0;



}