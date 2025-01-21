#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

// Глобальная переменная
int global_var = 100;

void *mythread(void *arg) {
    // Локальная переменная
    int local_var = 300;

    pid_t tid = syscall(SYS_gettid);

    // Вывод информации о потоке и переменных
    printf("Thread TID: %d\n", tid);
    printf("Global variable address: %p\n", (void*)&global_var);
    printf("Local variable address: %p\n", (void*)&local_var);

    sleep(100);

    return NULL;
}

int main() {
    pthread_t threads[5];
    int err;

    printf("Main thread PID: %d\n", getpid());
    printf("Global variable address: %p\n\n", (void*)&global_var);

    // Создаем 5 потоков
    for (int i = 0; i < 5; i++) {
        err = pthread_create(&threads[i], NULL, mythread, NULL);
        if (err != 0) {
            printf("main: pthread_create() failed for thread %d\n", i);
            return -1;
        }
    }

    // Ожидаем завершения всех потоков
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
