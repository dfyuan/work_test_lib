#ifndef	__EZY_BUF_H__
#define	__EZY_BUF_H__

#ifdef __KERNEL__
/* include Linux files */
#include <asm/io.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>	/*     printk() */
#include <linux/slab.h>		/*     kmalloc() */
#include <linux/fs.h>		/*     everything... */
#include <linux/errno.h>	/*     error codes     */
#include <linux/types.h>	/*     size_t */
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

#endif				/* end of #ifdef __KERNEL__ */


//add
#define EZY_HEAD_LEN		20 //must big than 12

enum ezy_memory {
	EZY_MEMORY_MMAP             = 1,
	EZY_MEMORY_USERPTR          = 2,
//	ezy_MEMORY_OVERLAY          = 3,
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
#pragma	pack(1)


#pragma	pack()
/* End of ioctls */


#ifdef __KERNEL__

/* Defines and Constants*/
#define	EZY_MAX_BUFFERS						128
#define	EZY_MAX_SIZE						(1920*1680*2)

#define	MAX_CHANNELS						16
#define	MAX_PRIORITY						5
#define	MIN_PRIORITY						0
#define	DEFAULT_PRIORITY					3
#define	MAX_IMAGE_WIDTH						1280
#define	MAX_IMAGE_WIDTH_HIGH					640
#define	MAX_IMAGE_HEIGHT					960
#define	MAX_INPUT_BUFFERS					8
#define	MAX_OUTPUT_BUFFERS					8
#define	FREE_BUFFER							0
#define	ALIGNMENT							16
#define	CHANNEL_BUSY						1
#define	CHANNEL_FREE						0
#define	PIXEL_EVEN							2
#define	RATIO_MULTIPLIER					256


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

#define UVC_HLE		12//must be (sizeof(struct uvc_stream_header))	/* header length  12 */


struct ezybuf_buffer {
	struct list_head stream;
	struct list_head queue;
	wait_queue_head_t done;
	enum ezybuf_state state;
	int index;					/* buffer index number, 0 -> N-1 */
	int buf_type;				/* buffer type, input or output */
	//modify
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


/*  ezybuf_queue contain all info */
struct ezybuf_queue {
	struct list_head stream;
	struct list_head dma_queue;

	struct ezybuf_buffer **buffers;
	int buf_cnt;	// actual buffers
	unsigned int msize;
	unsigned int cache;
	int streaming;
	struct mutex lock;
	spinlock_t irqlock;

	struct kref kref;

	struct file_operations *fops;	// file ops
	void *priv_fops;
	struct ezybuf_hops *hops;		// hook ops
	void *priv;

	char name[CDEV_NAME_SIZE];
	int id;
	struct class_device *ezybuf_dev;
	struct cdev c_dev;
};


struct ezybuf_buffer *ezybuf_dqbuf_user(struct ezybuf_queue *q);
void ezybuf_wakeup(struct ezybuf_buffer *eb,  int err);
int ezybuf_queue_init(struct ezybuf_queue *q,
                      struct file_operations *fops,
                      struct ezybuf_hops *hops,
                      unsigned int msize,
                      unsigned int cache,
                      char *name,
                      void *priv);

void ezybuf_queue_cancel(struct ezybuf_queue *q);



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

int drvbuf_queue_init(struct drvbuf_queue *q,
                      unsigned int buf_cnt,
                      unsigned int msize,
                      unsigned int nonblock);
void drvbuf_queue_cancel(struct drvbuf_queue *q);
void drvbuf_qempty(struct drvbuf_queue *q, struct drvbuf_buffer *db);
void drvbuf_qfull(struct drvbuf_queue *q, struct drvbuf_buffer *db);
struct drvbuf_buffer * drvbuf_dqempty(struct drvbuf_queue *q);
struct drvbuf_buffer * drvbuf_dqfull(struct drvbuf_queue *q);
void drvbuf_wakeup(struct drvbuf_buffer *db);


#endif				/* end of #ifdef __KERNEL__ */

#endif				/* End of #ifndef __EZY_BUF_H__ */
