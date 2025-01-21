#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

// Глобальная переменная
int global_var = 1;


void *mythread(void *arg) {
    // Локальная переменная
    int local_var = 2;

    // Локальная статическая переменная
    static int local_static_var = 3;

    // Локальная константная переменная
    const int local_const_var = 4;

    pthread_t self = pthread_self();
    printf("mythread [%d %d %d %ld]: Hello from mythread!\n", 
           getpid(), getppid(), gettid(), self);

    // Вывод адресов переменных
    printf("Addresses in thread [%ld]:\n", self);
    printf("Global variable address: %p\n", (void*)&global_var);
    printf("Local variable address: %p\n", (void*)&local_var);
    printf("Local static variable address: %p\n", (void*)&local_static_var);
    printf("Local const variable address: %p\n", (void*)&local_const_var);

    sleep(100);
    
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
