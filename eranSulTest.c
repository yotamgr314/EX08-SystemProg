// REMEBER THAT IN THE EXAM ERAN SAID WE CAN ASSUME struct timeSpec is just sturct int that 
// represent the expiration time!.
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>



//Struct for a timeout event
typedef struct TimeoutEvent
{
    int timeout_ms; // Timeout in milliseconds
    void (*callback)(void *); // Callback function
    void *arg; // Argument for the callback function
    struct TimeoutEvent *next; // Next event in the queue
}TimeoutEvent;

// Struct for the timer queue
typedef struct TimerQueue
{
    TimeoutEvent *head;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} TimerQueue;


// Timer thread function
void *timer_thread(void *arg)
{
    TimerQueue *queue = (TimerQueue *)arg;
    
    while (1) {
        pthread_mutex_lock(&queue->mutex);
        
        while (queue->head == NULL) {
            pthread_cond_wait(&queue->cond, &queue->mutex)
        }
        

        int res = pthread_cond_timedwait(&queue->cond, &queue->mutex, &queue->ts);
        
        if (res == ETIMEDOUT) 
        {
            TimeoutEvent *event = queue->head;
            queue->head = event->next;
            
            // Perform the callback
            event->callback(event->arg);
            free(event);
        }
            pthread_mutex_unlock(&queue->mutex);
     }

}

    

// Function to add a new timeout event to the queue
void add_message(struct timespec expiration, callback_t callback, void* data) {
    TimerQueue * msg = (TimerQueue *)malloc(sizeof(message_t));
    msg->expiration_time = expiration;
    msg->callback = callback;
    msg->data = data;
    msg->next = NULL;

    pthread_mutex_lock(&timer_list.mutex);

    // Insert the message into the sorted list by expiration time
    message_t* current = timer_list.head;
    while (current->next && current-> next->expiration_time < expiration ){
        current =  current->next;
   }
    msg->next = current  ->next 
   current -> next = msg;

    pthread_cond_signal(&timer_list.cond);
    pthread_mutex_unlock(&timer_list.mutex);
}

// Sample callback function
void sample_callback(void *arg){
    char *message = (char *)arg;
    printf("Timer expired: %s\n", message);
}


//Main function to demonstrate usage
int main() {
    TimerQueue queue = {NULL, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
    pthread_t timer_thread_id;

    pthread_create(&timer_thread_id, NULL, timer_thread, &queue);

    add_timeout_event(&queue, 2000, sample_callback, "Event 1");
// add one before closest timeout
    add_timeout_event(&queue, 1000, sample_callback, "Event 2");
// add one after closest timeout
    add_timeout_event(&queue, 3000, sample_callback, "Event 3");

    pthread_join(timer_thread_id, NULL);

    return 0;
}
