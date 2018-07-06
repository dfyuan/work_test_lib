#ifndef	__EZY_BUF_H__
#define	__EZY_BUF_H__

#ifdef __KERNEL__
#include <asm/io.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/kdev_t.h>
#include <asm/page.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/tty.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/proc_fs.h>
#include <linux/sysctl.h>
#include <linux/list.h>
#endif

#define EZY_HEAD_LEN		20

enum ezy_memory {
	EZY_MEMORY_MMAP             = 1,
	EZY_MEMORY_USERPTR          = 2,
};

/* To allocate the memory*/
struct ezy_reqbufs {
	int buf_type;	/* type of frame buffer */
	int size;		/* size of the frame bufferto be allocated */
	int count;		/* number of frame buffer to be allocated */
	enum ezy_memory memory;
} ;

/* assed for quering the buffer to get physical address*/
struct ezy_buffer {
	int index;					/* buffer index number, 0 -> N-1 */
	int buf_type;				/* buffer type, input or output */
	unsigned long headlen;
	unsigned long phyaddr;	/* physical     address of the buffer,
				  				 used in the mmap() system call */
	int size;
	int bytesused;	// actuall used size
};

/* ioctls definition */
#pragma		pack(1)
#define		EZY_IOC_BASE			       'E'
#define		EZY_IOC_MAXNR					6

/*Ioctl options which are to be passed while calling the ioctl*/
#define	EZY_REQBUF		_IOWR(EZY_IOC_BASE,1, struct ezy_reqbufs *)
#define	EZY_QUERYBUF	_IOWR(EZY_IOC_BASE,2, struct ezy_buffer *)
#define	EZY_QBUF		_IOWR(EZY_IOC_BASE,3, struct ezy_buffer *)
#define	EZY_DQBUF		_IOWR(EZY_IOC_BASE,4, struct ezy_buffer *)
#define	EZY_STREAMON	_IOWR(EZY_IOC_BASE,5, struct ezy_buffer *)
#define	EZY_STREAMOFF	_IOWR(EZY_IOC_BASE,6, struct ezy_buffer *)

#pragma	pack()
/* End of ioctls */

#ifdef __KERNEL__

/* Defines and Constants*/
#define	EZY_MAX_BUFFERS						128
#define	EZY_MAX_SIZE						(1920*1680*2)

/* macro for bit set and clear */
#define	BITSET(variable,bit)		((variable)| (1<<bit))
#define	BITRESET(variable,bit)		((variable)& (~(0x00000001<<(bit))))

#define	SUCESS						0
#define	ZERO						0

#define EBUF_ERROR		1
#define EBUF_DONE		0

#define CDEV_NAME_SIZE		16

#define EZYBUF_DEV_COUNT	16


enum ezybuf_state {
	STATE_EZBUF_NEEDS_INIT = 0,
	STATE_EZBUF_PREPARED   = 1,
	STATE_EZBUF_QUEUED     = 2,
	STATE_EZBUF_ACTIVE     = 3,
	STATE_EZBUF_DONE       = 4,
	STATE_EZBUF_ERROR      = 5,
	STATE_EZBUF_IDLE       = 6,
};

struct ezybuf_buffer {
	struct list_head empty;
	struct list_head full;
	wait_queue_head_t done;
	enum ezybuf_state state;
	int index;					/* buffer index number, 0 -> N-1 */
	int buf_type;				/* buffer type, input or output */

	unsigned long headlen;
	unsigned long phyaddr;/* physical start address of the buffer,
				  				    used in the mmap() system call */
	enum ezy_memory memory;
	unsigned long vaddr;		// kernel virtual start addr
	int size;
	int bytesused;				// actuall used size

	unsigned char *private;
};

struct ezybuf_queue;

struct ezybuf_hops {
	int (*reqbuf)(struct ezybuf_queue *q, void *priv, struct ezybuf_buffer *eb);
	int (*querybuf)(struct ezybuf_queue *q, void *priv, struct ezybuf_buffer *eb);
	int (*qbuf)(struct ezybuf_queue *q, void *priv, struct ezybuf_buffer *eb);
	int (*dqbuf)(struct ezybuf_queue *q, void *priv, struct ezybuf_buffer *eb);
	int (*streamon)(struct ezybuf_queue *q, void *priv);
	int (*streamoff)(struct ezybuf_queue *q, void *priv, struct ezybuf_buffer *eb);
};

struct ezybuf_queue {
	void *priv_fops;
	void *priv;

	char name[CDEV_NAME_SIZE];

	int id;
	int buf_cnt;
	int streaming;

	unsigned int msize;
	unsigned int cache;

	spinlock_t irqlock;

	struct cdev c_dev;
	struct class_device *ezybuf_dev;
	struct file_operations *fops;
	struct ezybuf_buffer **buffers;
	struct list_head empty;
	struct list_head full;
	struct mutex lock;
	struct kref kref;

};


void ezybuf_wakeup(struct ezybuf_buffer *eb,  int err);
void ezybuf_queue_cancel(struct ezybuf_queue *q);
struct ezybuf_buffer *ezybuf_dqbuf_user(struct ezybuf_queue *q);
int ezybuf_queue_init(struct ezybuf_queue *q, struct file_operations *fops, unsigned int msize, unsigned int cache, char *name, void *priv);



/*******************************************************************
 buffer queue no app call
********************************************************************/
#define DRVBUF_Q_BLOCK		0
#define DRVBUF_Q_NONBLOCK	1

enum drvbuf_state {
	STATE_CMD_BLOCK,
	STATE_CMD_NONBLOCK,
};

struct drvbuf_buffer {
	struct list_head empty;
	struct list_head full;
	enum drvbuf_state state;
	wait_queue_head_t done;
};

struct drvbuf_queue {
	struct list_head empty;
	struct list_head full;
	struct drvbuf_buffer **buffers;
	unsigned int buf_cnt;	// actual buffers
	unsigned int msize;
	unsigned int nonblock;
	spinlock_t emptylock;
	spinlock_t fulllock;
};

int drvbuf_queue_init(struct drvbuf_queue *q,unsigned int buf_cnt,unsigned int msize,unsigned int nonblock);
void drvbuf_queue_cancel(struct drvbuf_queue *q);
void drvbuf_qempty(struct drvbuf_queue *q, struct drvbuf_buffer *db);
void drvbuf_qfull(struct drvbuf_queue *q, struct drvbuf_buffer *db);
struct drvbuf_buffer * drvbuf_dqempty(struct drvbuf_queue *q);
struct drvbuf_buffer * drvbuf_dqfull(struct drvbuf_queue *q);
void drvbuf_wakeup(struct drvbuf_buffer *db);
#endif
#endif
