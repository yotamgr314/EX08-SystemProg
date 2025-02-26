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



int main () {

    Shared_event_queue shared_event_queue = {NULL,PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
    pthread_t consumerThreads[3];

    for(int i = 0; i<3; i++)
    {
        pthread_create(&consumerThreads[i],NULL,&consumer_timer_thread_func, &shared_event_queue);
    }


    for(int i = 0; i<3; i++)
    {
        pthread_join(consumerThreads[i],NULL);
    }

    return 0;



}