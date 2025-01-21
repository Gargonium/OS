#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#define handle_error_en(en, msg) do { printf(" "); errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

void* thread_function() {

    printf("TID: %ld\n", pthread_self());
    
    return NULL;
}

int main() {

    pthread_attr_t attr;

    int attr_init_error = pthread_attr_init(&attr);

    if (attr_init_error != 0) {
        handle_error_en(attr_init_error, "pthread_attr_init");
    }

    int setdetach_error = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (setdetach_error != 0) {
        handle_error_en(setdetach_error, "pthread_attr_setdetachstate");
    }

    int attr_destroy_error;
    while(1) {
        pthread_t thread;

        int create_error = pthread_create(&thread, &attr, thread_function, NULL);

        if (create_error != 0) {

            attr_destroy_error = pthread_attr_destroy(&attr);
            if (attr_destroy_error != 0) {
                handle_error_en(attr_destroy_error, "pthread_attr_destroy in while");
            }

            handle_error_en(create_error, "pthread_create");
        }
        sleep(0.1);
    }

    attr_destroy_error = pthread_attr_destroy(&attr);

    if (attr_destroy_error != 0) {
        handle_error_en(attr_destroy_error, "pthread_attr_destroy");
    }
    
    return 0;
}