
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

// Struct for a timeout event
typedef struct TimeoutEvent {
    int timeout_ms; // Timeout in milliseconds
    void (*callback)(void *); // Callback function
    void *arg; // Argument for the callback function
    struct TimeoutEvent *next; // Next event in the queue
} TimeoutEvent;

// Struct for the timer queue
typedef struct TimerQueue {
    TimeoutEvent *head;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} TimerQueue;

int flag=1;
// Timer thread function
void *timer_thread(void *arg) {
    TimerQueue *queue = (TimerQueue *)arg;
    
    while (flag) {
        pthread_mutex_lock(&queue->mutex);
        
        while (queue->head == NULL) {
            pthread_cond_wait(&queue->cond, &queue->mutex);
        }
        
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += queue->head->timeout_ms / 1000;
        ts.tv_nsec += (queue->head->timeout_ms % 1000) * 1000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }

        int res = pthread_cond_timedwait(&queue->cond, &queue->mutex, &ts);
        
        if (res == ETIMEDOUT) {
            TimeoutEvent *event = queue->head;
            queue->head = event->next;
            pthread_mutex_unlock(&queue->mutex);
            
            // Perform the callback
            event->callback(event->arg);
            free(event);
        } else {
            pthread_mutex_unlock(&queue->mutex);
        }

    }
//gracefull termination
    
    return NULL;
}

// Function to add a new timeout event to the queue
void add_timeout_event(TimerQueue *queue, int timeout_ms, void (*callback)(void *), void *arg) {
    TimeoutEvent *new_event = (TimeoutEvent *)malloc(sizeof(TimeoutEvent));
    new_event->timeout_ms = timeout_ms;
    new_event->callback = callback;
    new_event->arg = arg;
    new_event->next = NULL;

    pthread_mutex_lock(&queue->mutex);
    
    if (queue->head == NULL) {
        queue->head = new_event;
    } else {
        TimeoutEvent *current = queue->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_event;
    }

    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

// Sample callback function
void sample_callback(void *arg) {
    char *message = (char *)arg;
    printf("Timer expired: %s\n", message);
}

// Main function to demonstrate usage
int main() {
    TimerQueue queue = {NULL, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
    pthread_t timer_thread_id;

    pthread_create(&timer_thread_id, NULL, timer_thread, &queue);

    add_timeout_event(&queue, 2000, sample_callback, "Event 1");
    add_timeout_event(&queue, 1000, sample_callback, "Event 2");
    add_timeout_event(&queue, 3000, sample_callback, "Event 3");

    pthread_join(timer_thread_id, NULL);

    return 0;
}