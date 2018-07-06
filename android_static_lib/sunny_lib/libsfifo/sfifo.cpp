#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#include <utils/Log.h>

#include "sfifo.h"

//#define CONFIG_COND_FREE 1
#define CONFIG_COND_ACTIVE 1

#define MAX_SFIFO_NUM	32

struct sfifo_des_s sfifo_des[MAX_SFIFO_NUM];

struct sfifo_s* sfifo_get_free_buf(struct sfifo_des_s *sfifo_des_p)
{
	static long empty_count = 0;
	struct sfifo_s *sfifo = NULL;

	pthread_mutex_lock(&(sfifo_des_p->free_list.lock_mutex));
#ifdef CONFIG_COND_FREE
	while (sfifo_des_p->free_list.head == NULL) {
		pthread_cond_wait(&(sfifo_des_p->free_list.cond), &(sfifo_des_p->free_list.lock_mutex));
	}
#else
	if (sfifo_des_p->free_list.head == NULL) {
		if (empty_count++ % 120 == 0) {
			ALOGD("free list empty\n");
		}
		goto EXIT;
	}
#endif
	sfifo = sfifo_des_p->free_list.head;
	sfifo_des_p->free_list.head = sfifo->next;

EXIT:
	pthread_mutex_unlock(&(sfifo_des_p->free_list.lock_mutex));

	return sfifo;
}

int sfifo_put_free_buf(struct sfifo_s *sfifo, struct sfifo_des_s *sfifo_des_p)
{
	int send_cond = 0;

	pthread_mutex_lock(&(sfifo_des_p->free_list.lock_mutex));
	if (sfifo_des_p->free_list.head == NULL) {
		sfifo_des_p->free_list.head = sfifo;
		sfifo_des_p->free_list.tail = sfifo;
		sfifo_des_p->free_list.tail->next = NULL;
		send_cond = 1;
	} else {
		sfifo_des_p->free_list.tail->next = sfifo;
		sfifo_des_p->free_list.tail = sfifo;
		sfifo_des_p->free_list.tail->next = NULL;
	}
	pthread_mutex_unlock(&(sfifo_des_p->free_list.lock_mutex));
#ifdef CONFIG_COND_FREE
	if (send_cond) {
		pthread_cond_signal(&(sfifo_des_p->free_list.cond));
	}
#endif
	return 0;
}

struct sfifo_s* sfifo_get_active_buf(struct sfifo_des_s *sfifo_des_p)
{
	struct sfifo_s *sfifo = NULL;

	pthread_mutex_lock(&(sfifo_des_p->active_list.lock_mutex));
#ifdef CONFIG_COND_ACTIVE
	while (sfifo_des_p->active_list.head == NULL) {
		//pthread_cond_timedwait(&(sfifo_des_p->active_list.cond), &(sfifo_des_p->active_list.lock_mutex), &outtime);
		pthread_cond_wait(&(sfifo_des_p->active_list.cond), &(sfifo_des_p->active_list.lock_mutex));
	}
#else
	if (sfifo_des_p->active_list.head == NULL) {
		ALOGD("active list empty\n");
		goto EXIT;
	}
#endif

	sfifo = sfifo_des_p->active_list.head;
	sfifo_des_p->active_list.head = sfifo->next;

EXIT:
	pthread_mutex_unlock(&(sfifo_des_p->active_list.lock_mutex));

	return sfifo;
}

int sfifo_put_active_buf(struct sfifo_s *sfifo, struct sfifo_des_s *sfifo_des_p)
{
	int send_cond = 0;

	pthread_mutex_lock(&(sfifo_des_p->active_list.lock_mutex));
	if (sfifo_des_p->active_list.head == NULL) {
		sfifo_des_p->active_list.head = sfifo;
		sfifo_des_p->active_list.tail = sfifo;
		sfifo_des_p->active_list.tail->next = NULL;
		send_cond = 1;
	} else {
		sfifo_des_p->active_list.tail->next = sfifo;
		sfifo_des_p->active_list.tail = sfifo;
		sfifo_des_p->active_list.tail->next = NULL;
	}
	pthread_mutex_unlock(&(sfifo_des_p->active_list.lock_mutex));
#ifdef CONFIG_COND_ACTIVE
	if (send_cond) {
		pthread_cond_signal(&(sfifo_des_p->active_list.cond));
	}
#endif
	return 0;
}

int dump_sfifo_list(struct sfifo_list_des_s *list)
{
	struct sfifo_s *sfifo = NULL;
	sfifo = list->head;
	do {
		ALOGD("dump : %x\n", sfifo->buffer[0]);
		usleep(500 * 1000);
	} while (sfifo->next != NULL && (sfifo = sfifo->next));

	return 0;
}

struct sfifo_des_s *sfifo_init(int sfifo_num, int sfifo_buffer_size, int sfifo_active_max_num)
{
	int i = 0;
	struct sfifo_s *sfifo;

	struct sfifo_des_s *sfifo_des_p;
	sfifo_des_p = (struct sfifo_des_s *)malloc(sizeof(struct sfifo_des_s));

	sfifo_des_p->sfifos_num = sfifo_num;
	sfifo_des_p->sfifos_active_max_num = sfifo_active_max_num;

	sfifo_des_p->free_list.sfifo_num = 0;
	sfifo_des_p->free_list.head = NULL;
	sfifo_des_p->free_list.tail = NULL;
	pthread_mutex_init(&sfifo_des_p->free_list.lock_mutex, NULL);
	pthread_cond_init(&sfifo_des_p->free_list.cond, NULL);

	sfifo_des_p->active_list.sfifo_num = 0;
	sfifo_des_p->active_list.head = NULL;
	sfifo_des_p->active_list.tail = NULL;
	pthread_mutex_init(&sfifo_des_p->active_list.lock_mutex, NULL);
	pthread_cond_init(&sfifo_des_p->active_list.cond, NULL);

	for (i = 0; i < sfifo_num; i++) {
		sfifo = (struct sfifo_s *)malloc(sizeof(struct sfifo_s));
		sfifo->buffer = (unsigned char *)malloc(sfifo_buffer_size);
		ALOGD("sfifo_init: %x\n", sfifo->buffer);
		memset(sfifo->buffer, i, sfifo_buffer_size);
		sfifo->size = sfifo_buffer_size;
		sfifo->next = NULL;
		sfifo_put_free_buf(sfifo, sfifo_des_p);
	}

	return sfifo_des_p;
}

