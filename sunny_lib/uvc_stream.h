#ifndef __VIDEO_STREAM_H__
#define __VIDEO_STREAM_H__
#include "uvc_ctrl.h"
#include "typedef.h"


#define		EZY_IOC_BASE			       'E'
#define		EZY_IOC_MAXNR					6

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

#define	EZY_REQBUF		_IOWR(EZY_IOC_BASE,1, struct ezy_reqbufs *)
#define	EZY_QUERYBUF	_IOWR(EZY_IOC_BASE,2, struct ezy_buffer *)
#define	EZY_QBUF		_IOWR(EZY_IOC_BASE,3, struct ezy_buffer *)
#define	EZY_DQBUF		_IOWR(EZY_IOC_BASE,4, struct ezy_buffer *)
#define	EZY_STREAMON	_IOWR(EZY_IOC_BASE,5, struct ezy_buffer *)
#define	EZY_STREAMOFF	_IOWR(EZY_IOC_BASE,6, struct ezy_buffer *)


#define IMAGE_SIZE			(640*480*3/2+HEAD_LEN)

#define EZY_BUF_Q			0
#define EZY_BUF_NQ			1

#define EZY_BUF_NBLOCK		1
#define EZY_BUF_BLOCK		0

enum EzyMemory {
	MEMORY_MMAP             = 1,
	MEMORY_USERPTR          = 2,
};

struct EzyBuf {
	void *start;
	unsigned long headlen;
	unsigned long phyaddr;
	int index;
	int size;
	int bytesused;
};

typedef struct {
	int fd;
	enum EzyMemory memory;
	struct EzyBuf **ezyBufs;
	int numBufs;
} EZY_BufHndl;

int uvcstream_init(void);
int uvcstream_on(void);
int uvcstream_off();
bool is_uvcstreamon(void);
int set_device_mode(int mode);
int get_device_mode(void);
void uvcstream_uninit(void);
unsigned int write_video_buffer(unsigned char* pBuffer, unsigned int nBufferLen);


#endif
