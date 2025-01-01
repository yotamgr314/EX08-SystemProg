#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// Struct representing a timeout event
typedef struct TimeoutEvent {
    int timeout_ms;                  // Timeout in milliseconds
    void (*callback)(void *);        // Callback function to execute when timeout expires
    void *arg;                       // Argument to pass to the callback function
    struct TimeoutEvent *next;       // Pointer to the next event in the queue
} TimeoutEvent;


// Struct representing the timer queue
typedef struct TimerQueue {
    TimeoutEvent *head;              // Head of the linked list of events
    pthread_mutex_t mutex;           // Mutex for synchronizing access to the queue
    pthread_cond_t cond;             // Condition variable to signal threads
} TimerQueue;


// Function to add a timeout event to the queue
void add_timeout_event(TimerQueue *queue, int timeout_ms, void (*callback)(void *), void *arg) 
{
    // Allocate memory for the new event
    TimeoutEvent *event = (TimeoutEvent *)malloc(sizeof(TimeoutEvent));
    event->timeout_ms = timeout_ms;  // Set the timeout in milliseconds
    event->callback = callback;      // Set the callback function
    event->arg = arg;                // Set the argument for the callback function
    event->next = NULL;              // Initialize the next pointer to NULL

    pthread_mutex_lock(&queue->mutex); // Lock the mutex to ensure exclusive access to the queue
    if (!queue->head || queue->head->timeout_ms > timeout_ms) // NOTE : if list is empty or the new event is the earliest.
    {
        event->next = queue->head; // Insert at the head if it is the earliest event
        queue->head = event;
    } else {
        TimeoutEvent *current = queue->head;
        // Traverse the queue to find the correct position
        while (current->next && current->next->timeout_ms <= timeout_ms) 
        {
            current = current->next;
        }
        event->next = current->next; // Insert the event in the correct position
        current->next = event;
    }

    pthread_cond_signal(&queue->cond); // sends a signal to the thread that is waiting on that condition.

    pthread_mutex_unlock(&queue->mutex); // Unlock the mutex
}

// Function for the timer thread to process events
void *timer_thread(void *arg) 
{
    TimerQueue *queue = (TimerQueue *)arg; // Get the timer queue from the argument

    while (1) { // Infinite loop to continuously process events
        pthread_mutex_lock(&queue->mutex); // Lock the mutex for exclusive access

        // Wait for an event to be added if the queue is empty
        while (!queue->head) {
            pthread_cond_wait(&queue->cond, &queue->mutex);
        }

        TimeoutEvent *event = queue->head; // Get the head event

        // Get the current time
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);

        // Calculate the remaining wait time for the event
        int wait_time = event->timeout_ms - (now.tv_sec * 1000 + now.tv_nsec / 1000000);
        if (wait_time > 0) {
            struct timespec ts;
            ts.tv_sec = now.tv_sec + wait_time / 1000; // Set the wake-up time in seconds
            ts.tv_nsec = now.tv_nsec + (wait_time % 1000) * 1000000; // Set the wake-up time in nanoseconds
            pthread_cond_timedwait(&queue->cond, &queue->mutex, &ts); // Wait until the timeout
        }

        // If the head of the queue is still the same event, process it
        if (queue->head == event) {
            queue->head = event->next; // Remove the event from the queue
            pthread_mutex_unlock(&queue->mutex); // Unlock the mutex

            event->callback(event->arg); // Call the callback function with the provided argument
            free(event); // Free the memory allocated for the event
        } else {
            pthread_mutex_unlock(&queue->mutex); // Unlock the mutex if the event changed
        }
    }

    return NULL;
}

// Sample callback function
void sample_callback(void *arg) {
    char *message = (char *)arg; // Cast the argument to a string
    printf("Timer expired: %s\n", message); // Print a message when the timer expires
}

// Main function to demonstrate usage
int main() {
    // Initialize the timer queue with an empty head and initialized mutex and condition variable
    TimerQueue queue = {NULL, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
    pthread_t timer_thread_ids[3]; // Array to hold thread IDs for 3 timer threads

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

    // Wait for all threads to finish (though in this case, they run infinitely)
    for (int i = 0; i < 3; i++) {
        pthread_join(timer_thread_ids[i], NULL);
    }

    return 0;
}
