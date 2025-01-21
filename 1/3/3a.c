#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#define handle_error_en(en, msg) do { printf(" "); errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

typedef struct {
    int num;
    char* str;
}myType;

void* thread_function(void* arg) {

    printf("myVar from created thread:\nnum: %d\nstr: %s\n", ((myType*)arg)->num, ((myType*)arg)->str);

    return NULL;
}

int main() {

    myType myVar;

    myVar.num = 52;
    myVar.str = "Second";

    pthread_t thread;
    int err = pthread_create(&thread, NULL, thread_function, &myVar);
    if (err != 0) {
        handle_error_en(err, "pthread_create");
    }

    err = pthread_join(thread, NULL);
    if (err != 0) {
        handle_error_en(err, "pthread_join");
    }
    
    printf("Main done\n");
    
    return 0;
}
