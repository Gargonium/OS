#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#define handle_error_en(en, msg) do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

void* thread_function() {

    while (1) {
        printf("Hi! This is my line!\n");
    }
    
    return NULL;
}

int main() {

    pthread_t thread;
    int err = pthread_create(&thread, NULL, thread_function, NULL);
    if (err != 0) {
        handle_error_en(err, "pthread_create");
    }

    err = pthread_cancel(thread);
    if (err != 0) {
        handle_error_en(err, "pthread_cancel");
    }

    printf("Main done\n");
    
    pthread_exit(0);
}