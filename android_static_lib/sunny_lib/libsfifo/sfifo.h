#ifndef SFIFO_H_
#define SFIFO_H_

struct sfifo_list_des_s {
	int sfifo_num;

	struct sfifo_s *head;
	struct sfifo_s *tail;

	pthread_mutex_t lock_mutex;
	pthread_cond_t cond;
};

struct sfifo_des_s {
	int sfifo_init;

	unsigned int sfifos_num;
	unsigned int sfifos_active_max_num;

	struct sfifo_list_des_s free_list;
	struct sfifo_list_des_s active_list;
};

struct sfifo_s {
	unsigned char *buffer;
	unsigned int size;
	struct sfifo_s *next;
};

extern struct sfifo_des_s *sfifo_init(int sfifo_num, int sfifo_buffer_size, int sfifo_active_max_num);

/* productor */
extern struct sfifo_s* sfifo_get_free_buf(struct sfifo_des_s *sfifo_des_p);
extern int sfifo_put_free_buf(struct sfifo_s *sfifo, struct sfifo_des_s *sfifo_des_p);

/* consumer */
extern struct sfifo_s* sfifo_get_active_buf(struct sfifo_des_s *sfifo_des_p);
extern int sfifo_put_active_buf(struct sfifo_s *sfifo, struct sfifo_des_s *sfifo_des_p);

#endif