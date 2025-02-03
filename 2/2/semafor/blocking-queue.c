#define _GNU_SOURCE
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>

#include "blocking-queue.h"

#define handle_error_en(en, msg) do { errno = en; perror(msg); abort(); } while (0)
#define handle_error(msg) do { perror(msg); abort(); } while (0)

void *qmonitor(void *arg) {
    blocking_queue_t *q = (blocking_queue_t *)arg;

    printf("qmonitor: [%d %d %d]\n", getpid(), getppid(), gettid());

    while (1) {
        blocking_queue_print_stats(q);
        sleep(1);
    }

    return NULL;
}

blocking_queue_t* blocking_queue_init(int max_count) {
    int err;

    blocking_queue_t *q = malloc(sizeof(blocking_queue_t));
    if (q == NULL) {
        printf("Cannot allocate memory for a queue\n");
        abort();
    }

    q->first = NULL;
    q->last = NULL;
    q->max_count = max_count;
    q->count = 0;

    q->add_attempts = q->get_attempts = 0;
    q->add_count = q->get_count = 0;

    err = sem_init(&q->empty, 0, max_count);
    if (err != 0) {
        handle_error("queue_init sem_init empty");
    }
    err = sem_init(&q->full, 0, 0);
    if (err != 0) {
        handle_error("queue_init sem_init full");
    }
    err = sem_init(&q->lock, 0, 1);
    if (err != 0) {
        handle_error("queue_init sem_init lock");
    }

    err = pthread_create(&q->qmonitor_tid, NULL, qmonitor, q);
    if (err != 0) {
        handle_error_en(err, "blocking_queue_init: pthread_create() failed");
    }

    return q;
}

void blocking_queue_destroy(blocking_queue_t **q) {
    if (*q == NULL) {
        return;
    }

    int err = sem_destroy(&(*q)->empty); 
    if (err != 0) {
        handle_error("queue_destroy sem_destroy empty");
    }
    err = sem_destroy(&(*q)->full);
    if (err != 0) {
        handle_error("queue_destroy sem_destroy full");
    }
    err = sem_destroy(&(*q)->lock);
    if (err != 0) {
        handle_error("queue_destroy sem_destroy lock");
    }

    pthread_cancel((*q)->qmonitor_tid);
    err = pthread_join((*q)->qmonitor_tid, NULL);
    if (err != 0) {
        handle_error_en(err, "blocking_queue_destroy _pthread_join");
    }

    while ((*q)->first) {
        qnode_t* tmp = (*q)->first;
        (*q)->first = (*q)->first->next;
        free(tmp);
    }

    free(*q);
    *q = NULL;

}

int blocking_queue_add(blocking_queue_t *q, int val) {
    sem_wait(&q->lock);
    q->add_attempts++;
    sem_post(&q->lock);

    assert(q->count <= q->max_count);

    int err = sem_wait(&q->empty);
    if (err != 0) {
        handle_error("queue_add sem_wait empty");
    }

    qnode_t *new = malloc(sizeof(qnode_t));
    if (!new) {
        printf("Cannot allocate memory for a new node\n");
        abort();
    }

    new->val = val;
    new->next = NULL;

    err = sem_wait(&q->lock);
    if (err != 0) {
        handle_error("queue_add sem_wait lock");
    }

    if (!q->first)
        q->first = q->last = new;
    else {
        q->last->next = new;
        q->last = q->last->next;
    }

    q->count++;
    q->add_count++;

    err = sem_post(&q->full);
    if (err != 0) {
        handle_error("queue_add sem_post full");
    }
    err = sem_post(&q->lock);
    if (err != 0) {
        handle_error("queue_add sem_post lock");
    }

    return 1;
}

int blocking_queue_get(blocking_queue_t *q, int *val) {
    q->get_attempts++;

    assert(q->count >= 0);

    int err = sem_wait(&q->full);
    if (err != 0) {
        handle_error("queue_get sem_wait full");
    }

    err = sem_wait(&q->lock);
    if (err != 0) {
        handle_error("queue_get sem_wait lock");
    }

    qnode_t *tmp = q->first;

    *val = tmp->val;
    q->first = q->first->next;

    free(tmp);
    q->count--;
    q->get_count++;

    err = sem_post(&q->empty);
    if (err != 0) {
        handle_error("queue_get sem_post empty");
    }

    err = sem_post(&q->lock);
    if (err != 0) {
        handle_error("queue_get sem_post lock");
    }

    return 1;
}

void blocking_queue_print_stats(blocking_queue_t *q) {
    printf("queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld %ld %ld)\n",
           q->count,
           q->add_attempts, q->get_attempts, q->add_attempts - q->get_attempts,
           q->add_count, q->get_count, q->add_count - q->get_count);
}