#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#define handle_error_en(en, msg) do { printf(" "); errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)


void* thread_function() {

    printf("TID: %ld\n", pthread_self());

    int detach_error = pthread_detach(pthread_self());

    if (detach_error != 0) {
        handle_error_en(detach_error, "pthread_detach");
    }
    
    pthread_exit(NULL);
}

int main() {

    while(1) {
        pthread_t thread;

        int create_error = pthread_create(&thread, NULL, thread_function, NULL);

        if (create_error != 0) {
            handle_error_en(create_error, "pthread_create");
            // Может быть пробовать повторно запускать при ошибке
        }
        sleep(0.1);
    }
    
    return 0;
}