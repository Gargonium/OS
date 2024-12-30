#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define handle_error_en(en, msg) do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

void sigint_handler(int signo) {
    printf("Thread 2: Received SIGINT (номер %d).\n", signo);
}

void* thread1_func() {
    sigset_t set;
    int err = sigfillset(&set); 
    if (err != 0) {
        handle_error("Thread1. sigfillset");
    }
    err = pthread_sigmask(SIG_BLOCK, &set, NULL);
    if (err != 0) {
        handle_error_en(err, "Thread3. pthread_sigmask");
    }

    printf("Thread 1: All signals blocked.\n");

    while(1) {
        sleep(1);
    }

    return NULL;
}

void* thread2_func() {

    sigset_t set;
    int err = sigemptyset(&set);         
    if (err != 0) {
        handle_error("Thread3. sigemptyset");
    }
    err = sigaddset(&set, SIGQUIT);
    if (err != 0) {
        handle_error("Thread3. sigaddset");
    }
    err = pthread_sigmask(SIG_BLOCK, &set, NULL); 
    if (err != 0) {
        handle_error_en(err, "Thread3. pthread_sigmask");
    }

    printf("Thread 2: Waiting for signal SIGINT (Ctrl+C)...\n");

    void* errSignal = signal(SIGINT, sigint_handler);
    if (errSignal == SIG_ERR) {
        handle_error("Thread 2. Signal");
    }

    while(1) {
        sleep(1);
    }

    return NULL;
}

void* thread3_func() {
    int signo;

    sigset_t set;
    int err = sigemptyset(&set);        
    if (err != 0) {
        handle_error("Thread3. sigemptyset");
    }
    err = sigaddset(&set, SIGQUIT);    
    if (err != 0) {
        handle_error("Thread3. sigaddset");
    }
    err = pthread_sigmask(SIG_BLOCK, &set, NULL); 
    if (err != 0) {
        handle_error_en(err, "Thread3. pthread_sigmask");
    }

    printf("Thread 3: Waiting for signal SIGQUIT (Ctrl+\\)...\n");

    err = sigwait(&set, &signo);        
    if (err !=0 ) {
        handle_error("Thread3. sigwait");
    }
    printf("Thread 3: Received SIGQUIT (%d).\n", signo);

    printf("Exiting...\n");
    exit(0);
    pthread_exit(0);
}

int main() {
    pthread_t thread1, thread2, thread3;

    printf("%d\n", getpid());

    int err = pthread_create(&thread1, NULL, thread1_func, NULL);
    if (err != 0) {
        handle_error_en(err, "pthread_create1");
    }

    err = pthread_create(&thread2, NULL, thread2_func, NULL);
    if (err != 0) {
        handle_error_en(err, "pthread_create2");
    }

    err = pthread_create(&thread3, NULL, thread3_func, NULL);
    if (err != 0) {
        handle_error_en(err, "pthread_create2");
    }

    pthread_exit(0);
}
