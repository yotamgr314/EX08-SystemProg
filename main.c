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

/// NOT SURE IF THE ABOVE IS IN DIFFERENT FILES 

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






