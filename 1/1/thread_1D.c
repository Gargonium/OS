#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

// Глобальная переменная
int global_var = 100;

void *mythread(void *arg) {
    // Локальная переменная
    int local_var = 300;

    // Вывод первоначальных значений переменных
    printf("mythread [%d %d %d %ld]: Initial values\n", 
           getpid(), getppid(), gettid(), pthread_self());
    printf("Global variable: %d, Local variable: %d\n", global_var, local_var);

    // Изменение значений переменных
    global_var += 10;
    local_var += 10;

    // Вывод изменённых значений переменных
    printf("mythread [%d %d %d %ld]: After modification\n", 
           getpid(), getppid(), gettid(), pthread_self());
    printf("Global variable: %d, Local variable: %d\n\n", global_var, local_var);

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
