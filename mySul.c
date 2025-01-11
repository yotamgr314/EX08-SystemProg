#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// Function to unlock the mutex (wrapper for pthread_mutex_unlock)
void unlock_mutex(void *arg) {
    pthread_mutex_unlock((pthread_mutex_t *)arg);
}

// Struct for timeout event
typedef struct TimeoutEvent {
    int timeout_ms; // pop timing
    void (*callback)(void *); // a function pointer.
    void *arg; // arguments for the callback function.
    struct TimeoutEvent *next; // next event in the queue.
} TimeoutEvent;

// Struct for timer queue
typedef struct TimerQueue {
    TimeoutEvent *head;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} TimerQueue;

// Function to add a timeout event to its correct position in the list (based on its newEvent.timeout_ms value)
void add_timeout_event(TimerQueue *queue, int timeout_ms, void (*callback)(void *), void *arg) 
{
    TimeoutEvent *event = (TimeoutEvent *)malloc(sizeof(TimeoutEvent));
    event->timeout_ms = timeout_ms;
    event->callback = callback;
    event->arg = arg;
    event->next = NULL;

    pthread_mutex_lock(&queue->mutex);

    if (!queue->head || queue->head->timeout_ms > timeout_ms) // if the list is empty or the head of the list(which is the event with the smallest time_out value is greater than the new even.timeout valeu- insert it into the start of the list.
    {
        event->next = queue->head;
        queue->head = event;
    } else { // else - find the new event currect position in the list (based on his timeout_ms value).
        TimeoutEvent *current = queue->head;
        while (current->next && current->next->timeout_ms <= timeout_ms) 
        {
            current = current->next;
        }
        event->next = current->next;
        current->next = event;
    }

    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

// Timer thread function
void *timer_thread(void *arg) {
    TimerQueue *queue = (TimerQueue *)arg;

    // Register cleanup handler to unlock the mutex if the thread is canceled
    pthread_cleanup_push(unlock_mutex, &queue->mutex);

    while (1) {
        pthread_mutex_lock(&queue->mutex);

        while (!queue->head) {
            pthread_cond_wait(&queue->cond, &queue->mutex);
        }

        TimeoutEvent *event = queue->head;

        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);

        int wait_time = event->timeout_ms - (now.tv_sec * 1000 + now.tv_nsec / 1000000);
        if (wait_time > 0) {
            struct timespec ts;
            ts.tv_sec = now.tv_sec + wait_time / 1000;
            ts.tv_nsec = now.tv_nsec + (wait_time % 1000) * 1000000;
            pthread_cond_timedwait(&queue->cond, &queue->mutex, &ts);
        }

        if (queue->head == event) {
            queue->head = event->next;
            pthread_mutex_unlock(&queue->mutex);

            event->callback(event->arg);
            free(event);
        } else {
            pthread_mutex_unlock(&queue->mutex);
        }
    }

    pthread_cleanup_pop(0);
    return NULL;
}

// Sample callback function
void sample_callback(void *arg) {
    char *message = (char *)arg;
    printf("Timer expired: %s\n", message);
}

// Main function to demonstrate usage
int main() {
    TimerQueue queue = {NULL, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
    pthread_t timer_thread_ids[3];

    // Create 3 timer threads
    for (int i = 0; i < 3; i++) {
        pthread_create(&timer_thread_ids[i], NULL, timer_thread, &queue);
    }

    // Add multiple events to the queue
    add_timeout_event(&queue, 2000, sample_callback, "Event 1");
    add_timeout_event(&queue, 1000, sample_callback, "Event 2");
    add_timeout_event(&queue, 3000, sample_callback, "Event 3");
    add_timeout_event(&queue, 4000, sample_callback, "Event 4");
    add_timeout_event(&queue, 1500, sample_callback, "Event 5");

    sleep(5); // Let the threads process events for 5 seconds

    // Cancel the threads
    for (int i = 0; i < 3; i++) {
        pthread_cancel(timer_thread_ids[i]);
    }

    // Join the threads
    for (int i = 0; i < 3; i++) {
        pthread_join(timer_thread_ids[i], NULL);
    }

    return 0;
}
