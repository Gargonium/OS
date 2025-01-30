#define _GNU_SOURCE
#include <pthread.h>
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
        handle_error("Cannot allocate memory for a queue");
    }

    q->first = NULL;
    q->last = NULL;
    q->max_count = max_count;
    q->count = 0;

    q->add_attempts = q->get_attempts = 0;
    q->add_count = q->get_count = 0;
    err = pthread_mutex_init(&q->lock, NULL);
    if (err != 0) {
        handle_error_en(err, "Failed to initialize the mutex");
    }
    err = pthread_cond_init(&q->not_empty, NULL);
    if (err != 0) {
        handle_error_en(err, "Failed to initialize the not_empty condition variable");
    }
    err = pthread_cond_init(&q->not_full, NULL);
    if (err != 0) {
        handle_error_en(err, "Failed to initialize the not_full condition variable");
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

    pthread_mutex_lock(&(*q)->lock);

    pthread_cond_broadcast(&(*q)->not_empty);
    pthread_cond_broadcast(&(*q)->not_full);

    pthread_mutex_unlock(&(*q)->lock);

    pthread_cancel((*q)->qmonitor_tid);
    pthread_join((*q)->qmonitor_tid, NULL);

    pthread_mutex_destroy(&(*q)->lock);
    pthread_cond_destroy(&(*q)->not_empty);
    pthread_cond_destroy(&(*q)->not_full);

    while ((*q)->first) {
        qnode_t* tmp = (*q)->first;
        (*q)->first = (*q)->first->next;
        free(tmp);
    }

    free(*q);
    *q = NULL;
}


int blocking_queue_add(blocking_queue_t *q, int val) {
    pthread_mutex_lock(&q->lock);

    q->add_attempts++;

    assert(q->count <= q->max_count);

    while (q->count == q->max_count) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }

    qnode_t *new = malloc(sizeof(qnode_t));
    if (!new) {
        printf("Cannot allocate memory for a new node\n");
        pthread_mutex_unlock(&q->lock);
        abort();
    }

    new->val = val;
    new->next = NULL;

    if (!q->first)
        q->first = q->last = new;
    else {
        q->last->next = new;
        q->last = q->last->next;
    }

    q->count++;
    q->add_count++;

    pthread_cond_signal(&q->not_empty);

    pthread_mutex_unlock(&q->lock);

    return 1;
}

int blocking_queue_get(blocking_queue_t *q, int *val) {
    pthread_mutex_lock(&q->lock);

    q->get_attempts++;

    assert(q->count >= 0);

    while (q->count == 0) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }

    qnode_t *tmp = q->first;

    *val = tmp->val;
    q->first = q->first->next;

    free(tmp);
    q->count--;
    q->get_count++;

    pthread_cond_signal(&q->not_full);

    pthread_mutex_unlock(&q->lock);

    return 1;
}

void blocking_queue_print_stats(blocking_queue_t *q) {
    printf("queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld %ld %ld)\n",
           q->count,
           q->add_attempts, q->get_attempts, q->add_attempts - q->get_attempts,
           q->add_count, q->get_count, q->add_count - q->get_count);
}