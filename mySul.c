#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// Function to unlock the mutex (wrapper for pthread_mutex_unlock)
void unlock_mutex(void *arg)
{
    pthread_mutex_unlock((pthread_mutex_t *)arg);
}

// Struct for timeout event
typedef struct TimeoutEvent
{
    int timeout_ms;            // pop timing
    void (*callback)(void *);  // a function pointer.
    void *arg;                 // arguments for the callback function.
    struct TimeoutEvent *next; // next event in the queue.
} TimeoutEvent;

// Struct for timer queue
typedef struct TimerQueue
{
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
    }
    else
    { // else - find the new event currect position in the list (based on his timeout_ms value).
        TimeoutEvent *current = queue->head;
        while (current->next && current->next->timeout_ms <= timeout_ms)
        {
            current = current->next;
        }
        event->next = current->next;
        current->next = event;
    }

    pthread_cond_signal(&queue->cond); // once the shared queue is updated whith a new event - we will randomly allert one of the consumer threads which waiting on the queue cond signal that there is a new event in the shared queue. 
    pthread_mutex_unlock(&queue->mutex);
}

// the function executed by the timer thread. It listens for timeout events in the queue and executes the associated callback functions at the right time
void *timer_thread(void *arg)
{
    TimerQueue *queue = (TimerQueue *)arg; // shared queue recource. 

    // Register cleanup handler to unlock the mutex if the thread is canceled
    pthread_cleanup_push(unlock_mutex, &queue->mutex);

    while (1)
    {
        pthread_mutex_lock(&queue->mutex); // locking the shared queue recource.

        while (!queue->head) // if there are no eventes to execute in the shared queueu recouce then wait upon a condition from the shared queue recource., the mutex is automaticly unlocked until the the thred which executed this func will get a signal.
        {                    // The while loop ensures that the thread only proceeds if two conditions are met: 1. The thread is awakened by a signal. 2. After waking up, the shared queue is not empty (i.e., queue->head is not NULL),
                             // If the queue is still empty (because another thread consumed the event) the loop continues, and the thread waits again.
            pthread_cond_wait(&queue->cond, &queue->mutex);
        }

        TimeoutEvent *event = queue->head; // if we are here it means that a new event has been added to the head of the shared queue recource, hence we will compare its time with the current time of the PC, if the efresh is > 0 we will cond_timed_wait on that efresh.
                                           /// which automaticly relase the mutex on the shared queue recource and will make the thread who executed that function continute his executation in that efresh time

        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);

        int wait_time = event->timeout_ms - (now.tv_sec * 1000 + now.tv_nsec / 1000000); // calculate the efresh. 
        if (wait_time > 0)
        {
            struct timespec ts;
            ts.tv_sec = now.tv_sec + wait_time / 1000;
            ts.tv_nsec = now.tv_nsec + (wait_time % 1000) * 1000000;
            pthread_cond_timedwait(&queue->cond, &queue->mutex, &ts); // sleep for the efresh time..
        }

        if (queue->head == event) // if after the efresh time is done the timed out event is still in the head of the shared queue recouce - then execute it ! 
        {
            queue->head = event->next;
            pthread_mutex_unlock(&queue->mutex);

            event->callback(event->arg);
            free(event);
        }
        else
        {
            pthread_mutex_unlock(&queue->mutex);
        }
    }

    pthread_cleanup_pop(0);
    return NULL;
}

// the actual callback function that the timer thread calls when a timeout event expires.
void sample_callback(void *arg)
{
    char *message = (char *)arg;
    printf("Timer expired: %s\n", message);
}

// Main function to demonstrate usage
int main()
{
    TimerQueue queue = {NULL, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER}; // Static initialization of the struct. The mutex and condition variable are initialized using static macros PTHREAD_MUTEX_INITIALIZER and PTHREAD_COND_INITIALIZER, which set them up at compile time.

    pthread_t timer_thread_ids[3];

    // Create 3 timer threads
    for (int i = 0; i < 3; i++)
    {
        // int pthread_create(pthread_t * thread, const pthread_attr_t *thread_attr, void *(*callbackFunc)(void *), void *callBackFuncArguments )
        pthread_create(&timer_thread_ids[i], NULL, timer_thread, &queue); // passing NULL means the default thread intizliation.
    }

    // נכון מאוד – במקרה של הקוד הזה, ה־main (או הקובץ main.c) משמש כ־producer thread. כלומר, הוא אחראי "לייצר" את האירועים על ידי קריאות ל־add_timeout_event שמוסיפות את האירועים לתור (TimerQueue). אירועים אלו (TimeoutEvent) מועברים לתור המשותף, והם "נצרכים" (consumed) על ידי חוטי התזמון (timer_thread), שהם ה־consumer threads.

    // Add multiple events to the queue
    add_timeout_event(&queue, 2000, sample_callback, "Event 1");
    add_timeout_event(&queue, 1000, sample_callback, "Event 2");
    add_timeout_event(&queue, 3000, sample_callback, "Event 3");
    add_timeout_event(&queue, 4000, sample_callback, "Event 4");
    add_timeout_event(&queue, 1500, sample_callback, "Event 5");

    sleep(5); // Let the threads process events for 5 seconds

    // Cancel the threads
    for (int i = 0; i < 3; i++)
    {
        pthread_cancel(timer_thread_ids[i]);
    }

    // Join the threads
    for (int i = 0; i < 3; i++)
    {
        pthread_join(timer_thread_ids[i], NULL);
    }

    return 0;
}
