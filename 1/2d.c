#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#define handle_error_en(en, msg) \
            do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)


void* thread_function() {

    printf("TID: %ld\n", pthread_self());

    pthread_exit(NULL);
}

int main() {

    while(1) {
        pthread_t thread;
        int create_error = pthread_create(&thread, NULL, thread_function, NULL);

        if (create_error != 0) {
            //printf("bbb\n");
            handle_error_en(create_error, "pthread_create");
        }
    }
    
    return 0;
}