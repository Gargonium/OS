#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#define handle_error_en(en, msg) \
            do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

void* thread_function() {

    printf("Hello from the new thread!\n");
    sleep(2);  
    printf("Thread is finishing execution.\n");

    return NULL;
}

int main() {

    pthread_t thread;

    int create_error = pthread_create(&thread, NULL, thread_function, NULL);

    if (create_error != 0) {
        handle_error_en(create_error, "pthread_create");
    }

    int join_error = pthread_join(thread, NULL);

    if (join_error != 0) {
        handle_error_en(join_error, "pthread_join");
    }

    printf("Main thread: The created thread has finished execution.\n");

    return 0;
}
