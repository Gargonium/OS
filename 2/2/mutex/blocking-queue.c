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

    blocking_queue_t *q = malloc(sizeof(blocking_queue_t));
    if (!q) {
        printf("Cannot allocate memory for a queue\n");
        abort();
    }

    q->first = NULL;
    q->last = NULL;
    q->max_count = max_count;
    q->count = 0;

    q->add_attempts = q->get_attempts = 0;
    q->add_count = q->get_count = 0;

    int err = pthread_mutex_init(&q->lock, NULL);

    if (err != 0) {
        handle_error_en(err, "Failed to initialize the spinlock");
    }

    err = pthread_create(&q->qmonitor_tid, NULL, qmonitor, q);
    if (err != 0) {
        handle_error_en(err, "blocking_queue_init: pthread_create() failed");
    }

    return q;
}

void blocking_queue_destroy(blocking_queue_t **q) {
    pthread_mutex_lock(&(*q)->lock);

    if (*q == NULL) {
        return;
    }

    while ((*q)->first) {
        qnode_t* tmp = (*q)->first;
        (*q)->first = (*q)->first->next;
        free(tmp);
    }

    pthread_cancel((*q)->qmonitor_tid);
    int err = pthread_join((*q)->qmonitor_tid, NULL);
    if (err != 0) {
        handle_error_en(err, "blocking_queue_destroy pthread_join");
    }

    pthread_mutex_t lock = (*q)->lock;
    pthread_mutex_destroy(&(*q)->lock);

    free(*q);
    *q = NULL;

    pthread_mutex_unlock(&lock);
}

int blocking_queue_add(blocking_queue_t *q, int val) {

    pthread_mutex_lock(&q->lock);

    q->add_attempts++;

    assert(q->count <= q->max_count);
    
    if (q->count == q->max_count) {
        pthread_mutex_unlock(&q->lock);
        return 0;
    }

    qnode_t *new = malloc(sizeof(qnode_t));
    if (new == NULL) {
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

    pthread_mutex_unlock(&q->lock);

    return 1;
}

int blocking_queue_get(blocking_queue_t *q, int *val) {

    pthread_mutex_lock(&q->lock);

    q->get_attempts++;

    assert(q->count >= 0);

    if (q->count == 0) {
        pthread_mutex_unlock(&q->lock);
        return 0;
    }

    qnode_t *tmp = q->first;

    q->first = q->first->next;

    q->count--;
    q->get_count++;

    pthread_mutex_unlock(&q->lock);

    *val = tmp->val;
    free(tmp);

    return 1;
}

void blocking_queue_print_stats(blocking_queue_t *q) {
    printf("queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld %ld %ld)\n",
           q->count,
           q->add_attempts, q->get_attempts, q->add_attempts - q->get_attempts,
           q->add_count, q->get_count, q->add_count - q->get_count);
}