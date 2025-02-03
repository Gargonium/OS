#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "blocking-queue.h"

#define handle_error_en(en, msg) do { errno = en; perror(msg); abort(); } while (0)

void *qmonitor(void *arg) {
    blocking_queue_t *q = (blocking_queue_t *)arg;
    printf("qmonitor: [%d %d]\n", getpid(), getppid());
    while (1) {
        blocking_queue_print_stats(q);
        sleep(1);
    }
    return NULL;
}

blocking_queue_t* blocking_queue_init(int max_count) {
    blocking_queue_t *q = malloc(sizeof(blocking_queue_t));
    if (!q) {
        fprintf(stderr, "Cannot allocate memory for a queue\n");
        abort();
    }

    qnode_t *dummy = malloc(sizeof(qnode_t));
    if (!dummy) {
        fprintf(stderr, "Cannot allocate memory for a dummy node\n");
        abort();
    }
    dummy->next = NULL;

    q->first = dummy;
    q->last  = dummy;

    q->max_count = max_count;
    q->count = 0;

    q->add_attempts = q->get_attempts = 0;
    q->add_count = q->get_count = 0;

    int err = pthread_create(&q->qmonitor_tid, NULL, qmonitor, q);
    if (err != 0) {
        handle_error_en(err, "blocking_queue_init: pthread_create() failed");
    }

    return q;
}

int blocking_queue_add(blocking_queue_t *q, int val) {
    __sync_fetch_and_add(&q->add_attempts, 1);

    qnode_t *node = malloc(sizeof(qnode_t));
    if (node == NULL) {
        fprintf(stderr, "Cannot allocate memory for a new node\n");
        abort();
    }
    node->val = val;
    node->next = NULL;

    abort();

    // while (1) {
    //     int cur = q->count;
    //     if (cur == q->max_count)
    //         return 0;
    //     if (__sync_bool_compare_and_swap(&q->count, cur, cur + 1))
    //         break;
    // }

    // while (1) {
    //     qnode_t *last = q->last;
    //     qnode_t *next = last->next;
    //     if (last == q->last) {
    //         if (next == NULL) {
    //             // Пытаемся присоединить node к last->next 
    //             if (__sync_bool_compare_and_swap(&last->next, NULL, node)) {
    //                 // Если успешно – продвигаем last
    //                 __sync_bool_compare_and_swap(&q->last, last, node);
    //                 // Здесь, если не получится - ничего стращного, ридер пофиксит, или мы в следующий раз
    //                 break;
    //             }
    //         } else {
    //             // Фиксим за собой last ноду
    //             __sync_bool_compare_and_swap(&q->last, last, next);
    //         }
    //     }
    // }

    __sync_fetch_and_add(&q->add_count, 1);
    return 1;
}

int blocking_queue_get(blocking_queue_t *q, int *val) {
    __sync_fetch_and_add(&q->get_attempts, 1);

    // while (1) {
    //     qnode_t *first = q->first;
    //     qnode_t *last  = q->last;
    //     qnode_t *next  = first->next;
    //     if (first == q->first) {
            
    //         if (first == last) {
    //             // Если равны, то это dummy
    //             if (next == NULL) 
    //                 return 0;   // Никто не начал добавлять элемент
    //             // Если нет, то первый cas сделан, но last ещё не обновлен. Фиксим
    //             __sync_bool_compare_and_swap(&q->last, last, next);
    //         } else {
    //             int value = next->val;
    //             // Пытаемся продвинуть head 
    //             if (__sync_bool_compare_and_swap(&q->first, first, next)) {
    //                 *val = value;
    //                 // Атомарно уменьшаем счётчик элементов
    //                 __sync_fetch_and_sub(&q->count, 1);
    //                 __sync_fetch_and_add(&q->get_count, 1);
    //                 // Освобождаем старый узел 
    //                 free(first);
    //                 return 1;
    //             }
    //         }
    //     }
    // }
}

void blocking_queue_print_stats(blocking_queue_t *q) {
    printf("queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld %ld %ld)\n",
           q->count,
           q->add_attempts, q->get_attempts, q->add_attempts - q->get_attempts,
           q->add_count, q->get_count, q->add_count - q->get_count);
}

void blocking_queue_destroy(blocking_queue_t **q) {
    
    pthread_cancel((*q)->qmonitor_tid);
    int err = pthread_join((*q)->qmonitor_tid, NULL);
    if (err != 0)
        handle_error_en(err, "blocking_queue_destroy: pthread_join");

    if (*q == NULL) {
        return;
    }

    while ((*q)->first) {
        qnode_t* tmp = (*q)->first;
        (*q)->first = (*q)->first->next;
        free(tmp);
    }

    free(*q);
    *q = NULL;
}
