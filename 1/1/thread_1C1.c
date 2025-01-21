#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

void *mythread(void *arg) {
    pthread_t self = pthread_self();
    printf("mythread [%d %d %d %ld]: Hello from mythread!\n", 
           getpid(), getppid(), gettid(), self);

    pthread_t *created_thread = (pthread_t *)arg;
    if (pthread_equal(*created_thread, self)) {
        printf("Thread ID from pthread_create is equal to pthread_self()\n\n");
    } else {
        printf("Thread ID from pthread_create is NOT equal to pthread_self()\n\n");
    }

    return NULL;
}

int main() {
    pthread_t threads[5];
    int err;

    printf("main [%d %d %d]: Hello from main!\n\n", 
           getpid(), getppid(), gettid());

    // Создаем 5 потоков
    for (int i = 0; i < 5; i++) {
        err = pthread_create(&threads[i], NULL, mythread, &threads[i]);
        if (err != 0) {
            printf("main: pthread_create() failed for thread %d: %s\n", i, strerror(err));
            return -1;
        }
    }

    for (int i = 0; i < 5; i++) {
        err = pthread_join(threads[i], NULL);
        if (err != 0) {
            printf("main: pthread_join() failed for thread %d: %s\n", i, strerror(err));
            return -1;
        }
    }

    return 0;
}
