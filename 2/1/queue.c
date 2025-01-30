#define _GNU_SOURCE
#include <pthread.h>
#include <assert.h>

#include "queue.h"

#define handle_error_en(en, msg) do { errno = en; perror(msg); abort(); } while (0)
#define handle_error(msg) do { perror(msg); abort(); } while (0)

void *qmonitor(void *arg) {
	queue_t *q = (queue_t *)arg;

	printf("qmonitor: [%d %d %d]\n", getpid(), getppid(), gettid());

	while (1) {
        queue_print_stats(q);
        sleep(1);
    }

	return NULL;
}

queue_t* queue_init(int max_count) {
	int err;

	queue_t *q = malloc(sizeof(queue_t));
	if (q == NULL) {
		handle_error("Cannot allocate memory for a queue\n");
	}

	q->first = NULL;
	q->last = NULL;
	q->max_count = max_count;
	q->count = 0;

	q->add_attempts = q->get_attempts = 0;
	q->add_count = q->get_count = 0;

	err = pthread_create(&q->qmonitor_tid, NULL, qmonitor, q);
	if (err != 0) {
		handle_error_en(err, "queue_init: pthread_create() failed: %s\n");
	}

	return q;
}

void queue_destroy(queue_t *q) {
	pthread_cancel(q->qmonitor_tid);

	/*
	если не дождается join может произойти ситуация когда структура уже уничтожена, а
	монитор продолжит к ней обращаться
	*/
    int err = pthread_join(q->qmonitor_tid, NULL);
	if (err != 0) {
		handle_error_en(err, "queue_destroy: pthread_join() qmonitor");
	}


    qnode_t *current = q->first;
    while (current) {
        qnode_t *tmp = current;
        current = current->next;
        free(tmp);
    }

    free(q);
}

int queue_add(queue_t *q, int val) {
	q->add_attempts++;

	assert(q->count <= q->max_count);

	if (q->count == q->max_count)
		return 0;

	qnode_t *new = malloc(sizeof(qnode_t));
	if (new == NULL) {
		handle_error("Cannot allocate memory for new node\n");
	}

	new->val = val;
	new->next = NULL;

	if (!q->first)
		q->first = q->last = new;
	else {
		q->last->next = new;

		/*
		если в этот момент произойдет переключение, то может возникнуть проблема. Читающий поток
		забирает все эл-ты из очереди, затем пишущий поток продолжает работать с этой точки.
		Запись в конец очереди будет некорректна, т.к. надо обновить указатель first. 
		На следующей операции записи first и last обновятся, а значение, записанное в last в прошлый раз будет
		потеряно(т.к. очередь будет пуста и мы обновим сразу и first и last). 
		Отсюда ошибка с получением значения на 1 больше ожидаемого. 
		Кроме того, счетчик будет показывать на 1 элемент больше, чем доступно в очереди, 
		поэтому в следующий цикл работы читающего потока произойдет segfault 
		*/
		q->last = q->last->next;
	}


	/* 
	возникает race condition за q->count (т.к. ++ неатомарна)
	операция ++ началась, но прежде чем переложить полученное значение из регистра в память,
	запускается другой поток, который забирает элементы из очереди. Потом снова запускается
	этот поток и записывает в память полученное значение. Получается, что фактически эл-ов
   	нет, но счетчик показывает, что есть. Из-за этого читающий поток попробует прочитать из
	NULL и произойдет segfault 
	*/
	q->count++;
	q->add_count++;

	return 1;
}

int queue_get(queue_t *q, int *val) {
	q->get_attempts++;

	assert(q->count >= 0);

	if (q->count == 0)
		return 0;

	qnode_t *tmp = q->first;

	*val = tmp->val;
	q->first = q->first->next;

	free(tmp);

	/* 
	может быть аналогичная проблема, но в обратную сторону. В очереди будут элементы, но
	в счетчик запишется, что их 0 
	*/
	q->count--;
	q->get_count++;

	return 1;
}

void queue_print_stats(queue_t *q) {
	printf("queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld %ld %ld)\n",
		q->count,
		q->add_attempts, q->get_attempts, q->add_attempts - q->get_attempts,
		q->add_count, q->get_count, q->add_count - q->get_count);
}