#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#define handle_error_en(en, msg) do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

void cleaner(void* arg) {
    if (arg != NULL) {
        free(arg);
    }
    printf("Hello from cleaner!\n");
}

void* thread_function() {
    char* hw = (char*)malloc(sizeof(char) * strlen("hello world"));
    if (hw == NULL) {
        perror("Cannot allocate memory\n");
        return 0;
    }
    memcpy(hw, "hello world", strlen("hello world"));

    while (1) {
        pthread_cleanup_push(cleaner, hw);
        printf("%s\n", hw);
        pthread_cleanup_pop(0);
    }
}

int main() {

    pthread_t thread;
    int err = pthread_create(&thread, NULL, thread_function, NULL);
    if (err != 0) {
        handle_error_en(err, "pthread_create");
    }

    //sleep(1);

    err = pthread_cancel(thread);
    if (err != 0) {
        handle_error_en(err, "pthread_cancel");
    }

    printf("Main done\n");
    
    pthread_exit(0);
}