#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#define HEAP_USE 1

#define handle_error_en(en, msg) do { printf(" "); errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

typedef struct {
    int num;
    char* str;
}myType;

void* thread_function(void* arg) {

    printf("myVar from created thread:\nnum: %d\nstr: %s\n", ((myType*)arg)->num, ((myType*)arg)->str);
    if (HEAP_USE) {
        if (arg != NULL) {
            free(arg);
        }
    }
    
    return NULL;
}

int main() {

    myType myVar;
    myType* myVarPtr;

    if (HEAP_USE) {
        myVarPtr = (myType*)malloc(sizeof(myType));

        if (myVarPtr == NULL) {
            perror("Cannot allocate memory\n");
            return 0;
        }

        myVarPtr->num = 42;
        myVarPtr->str = "Heap";
    } else {
        myVar.num = 52;
        myVar.str = "Stack";
    }

    pthread_attr_t attr;

    int err = pthread_attr_init(&attr);

    if (err != 0) {
        handle_error_en(err, "pthread_attr_init");
    }

    err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (err != 0) {
        handle_error_en(err, "pthread_attr_setdetachstate");
    }

    pthread_t thread;

    if (HEAP_USE) {
        err = pthread_create(&thread, &attr, thread_function, myVarPtr);
    } else {
        err = pthread_create(&thread, &attr, thread_function, &myVar);
    }

    if (err != 0) {
        handle_error_en(err, "pthread_create");
    }

    err = pthread_attr_destroy(&attr);

    if (err != 0) {
        handle_error_en(err, "pthread_attr_destroy");
    }

    printf("Main done\n");
    
    pthread_exit(0);
    // return 0;
}
