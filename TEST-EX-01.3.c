


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