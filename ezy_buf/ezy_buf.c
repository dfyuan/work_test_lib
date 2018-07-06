#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/err.h>
#include "ezy_buf.h"


static dev_t ezybuf_devt = 0;
static int ezybuf_minor = 0;
static struct class *ezybuf_class = NULL;
static struct ezybuf_hops hops_default;

static unsigned long ezybuf_alloc_contig(unsigned int buf_size)
{
	void *mem = 0;
	u32 size = PAGE_SIZE << (get_order(buf_size));

	mem = (void *)__get_free_pages(GFP_KERNEL | GFP_DMA, get_order(buf_size));
	if (mem) {
		unsigned long adr = (unsigned long)mem;
		while (size > 0) {
			SetPageReserved(virt_to_page(adr));
			adr += PAGE_SIZE;
			size -= PAGE_SIZE;
		}
	}
	return (unsigned long)mem;
}

static void ezybuf_free_contig(unsigned long addr,unsigned int buf_size)
{
	unsigned int size, adr;

	if (!addr)
		return;
	adr = addr;
	size = PAGE_SIZE << (get_order(buf_size));
	while (size > 0) {
		ClearPageReserved(virt_to_page(adr));
		adr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	free_pages(addr, get_order(buf_size));
}

static void ezybuf_free_buffers(struct ezybuf_queue *q)
{
	int i;
	struct ezybuf_buffer *eb;

	if (q->buffers) {
		for (i = 0; i < q->buf_cnt; i++) {
			eb = q->buffers[i];
			if (eb) {
				if (eb->memory == EZY_MEMORY_MMAP)
					ezybuf_free_contig(eb->vaddr, eb->size);
				kfree(eb);
				q->buffers[i] = NULL;
			}
		}

		kfree(q->buffers);
		q->buffers = NULL;
	}
	return;
}

static void *ezybuf_alloc(unsigned int size)
{
	struct ezybuf_buffer *eb;

	eb = kzalloc(size, GFP_KERNEL);

	if (NULL != eb) {
		eb->state = STATE_EZBUF_PREPARED;
		INIT_LIST_HEAD(&eb->empty);
		INIT_LIST_HEAD(&eb->full);
		init_waitqueue_head(&eb->done);
	}

	return eb;
}

static int ezybuf_reqbuf(struct ezybuf_queue *q, struct ezy_reqbufs *rbufs)
{
	int i;
	int retval = 0;
	struct ezybuf_buffer *eb = NULL;

	mutex_lock(&q->lock);

	if (rbufs->count > EZY_MAX_BUFFERS)
		return -ENOBUFS;

	if (rbufs->size > EZY_MAX_SIZE)
		return -EFAULT;

	if (!q->buffers) {
		q->buffers = kzalloc(sizeof(*q->buffers) * rbufs->count, GFP_KERNEL);
	}

	if (!q->buffers)
		return -ENOMEM;

	q->buf_cnt = 0;

	for (i = 0; i < rbufs->count; i++) {
		eb = q->buffers[i] = ezybuf_alloc(q->msize);
		if (!eb)
			goto free_mem;

		eb->index 	 = i;
		eb->memory   = rbufs->memory;
		eb->size 	 = rbufs->size;
		eb->headlen = EZY_HEAD_LEN;
		eb->buf_type = rbufs->buf_type;

		switch (eb->memory) {
		case EZY_MEMORY_MMAP:
			eb->vaddr = ezybuf_alloc_contig(eb->size);
			if (!eb->vaddr)
				goto free_mem;
			eb->phyaddr   = virt_to_phys((void *)eb->vaddr);
			eb->bytesused = eb->size;
			break;
		case EZY_MEMORY_USERPTR:

			eb->phyaddr    = 0;
			eb->vaddr 	  = 0;
			eb->bytesused = 0;
			break;
		default:
			printk("qbuf: wrong memory type\n");
			goto free_mem;
		}

		q->buf_cnt++;
	}

	mutex_unlock(&q->lock);

	return 0;

free_mem:
	ezybuf_free_buffers(q);
	mutex_unlock(&q->lock);
	return -ENOBUFS;
}

static int ezybuf_querybuf(struct ezybuf_queue *q, struct ezy_buffer *buf)
{
	struct ezybuf_buffer *eb;

	if (buf->index >= q->buf_cnt) {
		return -EFAULT;
	}

	mutex_lock(&q->lock);

	eb = q->buffers[buf->index];

	if (eb->memory == EZY_MEMORY_MMAP) {
		buf->phyaddr = eb->phyaddr;
		buf->size = eb->size;
		buf->headlen = eb->headlen;
	} else {
		buf->phyaddr = eb->phyaddr;
		buf->size = eb->size;
		buf->headlen = 0;
	}
	mutex_unlock(&q->lock);

	return 0;
}


static int ezybuf_qbuf_user(struct ezybuf_queue *q, struct ezybuf_buffer *eb)
{
	list_add_tail(&eb->full, &q->full);
	eb->state = STATE_EZBUF_QUEUED;

	return 0;
}

static int ezybuf_qbuf(struct ezybuf_queue *q, struct ezy_buffer *buf)
{
	struct ezybuf_buffer *eb = NULL;
	int retval = 0;
	unsigned long flags;

	mutex_lock(&q->lock);

	if (buf->index >= q->buf_cnt)
		goto done;
	spin_lock_irqsave(&q->irqlock, flags);
	eb = q->buffers[buf->index];
	spin_unlock_irqrestore(&q->irqlock, flags);

	if ((eb->state == STATE_EZBUF_QUEUED) || (eb->state == STATE_EZBUF_ACTIVE)) {
		printk("state == STATE_EZBUF_QUEUED || state == STATE_EZBUF_ACTIVE\n");
		goto done;
	}

	if (buf->bytesused > (eb->size - eb->headlen)) {
		eb->bytesused = eb->size - eb->headlen;
		printk("bytesused big than eb size\n");
	} else {
		eb->bytesused = buf->bytesused;
	}

	spin_lock_irqsave(&q->irqlock, flags);
	list_add_tail(&eb->empty, &q->empty);
	spin_unlock_irqrestore(&q->irqlock, flags);
	if (q->streaming) {
		spin_lock_irqsave(&q->irqlock, flags);
		retval = ezybuf_qbuf_user(q, eb);
		spin_unlock_irqrestore(&q->irqlock, flags);
	}
done:
	mutex_unlock(&q->lock);
	return retval;
}

static int ezybuf_waiton(struct ezybuf_buffer *eb, int non_blocking, int intr)
{
	int retval = 0;
	DECLARE_WAITQUEUE(wait, current);

	add_wait_queue(&eb->done, &wait);
	while (eb->state == STATE_EZBUF_ACTIVE || eb->state == STATE_EZBUF_QUEUED) {
		if (non_blocking) {
			retval = -EAGAIN;
			break;
		}

		set_current_state(intr  ? TASK_INTERRUPTIBLE : TASK_UNINTERRUPTIBLE);
		if (eb->state == STATE_EZBUF_ACTIVE || eb->state == STATE_EZBUF_QUEUED)
			schedule();

		set_current_state(TASK_RUNNING);

		if (intr && signal_pending(current)) {
			printk("ezybuf waiton: -EINTR\n");
			retval = -EINTR;
			break;
		}
	}
	remove_wait_queue(&eb->done, &wait);
	return retval;
}

static int ezybuf_dqbuf(struct ezybuf_queue *q, struct ezy_buffer *buf, int non_blocking)
{
	struct ezybuf_buffer *eb;
	int retval = -EBUSY;
	unsigned long flags;
	mutex_lock(&q->lock);

	if (!q->streaming) {
		printk("q->streaming is not ok\n");
		goto done;
	}

	if (list_empty(&q->empty)) {
		goto done;
	}

	spin_lock_irqsave(&q->irqlock, flags);
	eb = list_entry(q->empty.next, struct ezybuf_buffer, empty);
	spin_unlock_irqrestore(&q->irqlock, flags);
	retval = ezybuf_waiton(eb, non_blocking, 1);
	if (retval < 0) {
		goto done;
	}

	switch (eb->state) {
	case STATE_EZBUF_ERROR:
		eb->state = STATE_EZBUF_IDLE;
		break;
	case STATE_EZBUF_DONE:
		eb->state = STATE_EZBUF_IDLE;
		break;
	default:
		printk("dqbuf: state invalid\n");
		retval = -EINVAL;
		goto done;
	}

	list_del(&eb->empty);

	memset(buf, 0, sizeof(*buf));
	buf->index = eb->index;
	buf->phyaddr = eb->phyaddr;
	buf->size = eb->size;
	buf->bytesused = eb->bytesused;
	buf->headlen = eb->headlen;

done:
	mutex_unlock(&q->lock);
	return retval;

}

struct ezybuf_buffer *ezybuf_dqbuf_user(struct ezybuf_queue *q)
{
	struct ezybuf_buffer *eb = NULL;
	unsigned long flags;

	spin_lock_irqsave(&q->irqlock, flags);

	if (list_empty(&q->full)) {
		goto done;
	}

	eb = list_entry(q->full.next, struct ezybuf_buffer, full);

	list_del(&eb->full);
	eb->state = STATE_EZBUF_ACTIVE;

done:
	spin_unlock_irqrestore(&q->irqlock, flags);

	return eb;
}

static int ezybuf_streamon(struct ezybuf_queue *q)
{
	struct ezybuf_buffer *eb = NULL;
	struct list_head *list;
	unsigned long flags = 0;
	int retval = -EBUSY;

	mutex_lock(&q->lock);

	if (q->streaming)
		goto done;

	spin_lock_irqsave(&q->irqlock, flags);
	list_for_each(list, &q->empty) {
		eb = list_entry(list, struct ezybuf_buffer, empty);
		if (eb->state == STATE_EZBUF_PREPARED) {
			retval = ezybuf_qbuf_user(q, eb);
		}
	}
	spin_unlock_irqrestore(&q->irqlock, flags);
	q->streaming = 1;

done:
	mutex_unlock(&q->lock);
	return retval;
}


static int ezybuf_streamoff(struct ezybuf_queue *q)
{
	struct ezybuf_buffer *eb = NULL;
	unsigned long flags;
	int retval = 0;
	int i;
	mdelay(100);
	printk(" wait qbuf && dqbuf to finish %s,%d \n", __func__, __LINE__);
	mutex_lock(&q->lock);
	if (!q->streaming)
		goto done;

	spin_lock_irqsave(&q->irqlock, flags);
	INIT_LIST_HEAD(&q->empty);
	INIT_LIST_HEAD(&q->full);

	for (i = 0; i < q->buf_cnt; i++) {
		eb = q->buffers[i];
		eb->state = STATE_EZBUF_PREPARED;
	}
	spin_unlock_irqrestore(&q->irqlock, flags);
	q->streaming = 0;

done:
	mutex_unlock(&q->lock);
	return retval;
}


void ezybuf_wakeup(struct ezybuf_buffer *eb,  int err)
{
	if (eb == NULL)
		return;

	eb->state = err ? STATE_EZBUF_ERROR : STATE_EZBUF_DONE;
	wake_up_interruptible(&eb->done);

	return;
}

static void ezybuf_delete(struct kref *kref)
{
	struct ezybuf_queue *q = container_of(kref, struct ezybuf_queue, kref);
	mutex_lock(&q->lock);
	if (!q->streaming)
		goto done;
	INIT_LIST_HEAD(&q->empty);
	INIT_LIST_HEAD(&q->full);
done:
	mutex_unlock(&q->lock);
}

static inline void ezybuf_usrpriv_get(struct file *filp, struct ezybuf_queue *q)
{
	q->priv_fops = filp->private_data;
	filp->private_data = q;
}

static inline void ezy_usrpriv_put(struct file *filp, struct ezybuf_queue *q)
{
	q->priv_fops = NULL;
}

static inline void ezy_usrpriv_set(struct file *dstfilp, struct file *srcfilp, struct ezybuf_queue *q)
{
	dstfilp->private_data = q->priv_fops;
}

static int ezybuf_open(struct inode *inode, struct file *filp)
{
	int retval = 0;

	struct ezybuf_queue *q = container_of(inode->i_cdev, struct ezybuf_queue, c_dev);

	if (!q->streaming) {
		INIT_LIST_HEAD(&q->empty);
		INIT_LIST_HEAD(&q->full);
	}

	if (q->fops->open) {
		retval = q->fops->open(inode, filp);
	}

	ezybuf_usrpriv_get(filp, q);

	kref_get(&q->kref);

	return retval;

}

static int ezybuf_release(struct inode *inode, struct file *filp)
{
	struct file file;
	struct ezybuf_queue *q = filp->private_data;
	int retval = 0;

	ezy_usrpriv_set(&file, filp, q);

	if (q->fops->release)
		retval = q->fops->release(inode, &file);

	if (kref_put(&q->kref, ezybuf_delete)) {
		ezy_usrpriv_put(filp, q);
		filp->private_data = NULL;
	}

	return SUCESS;
}

static int ezybuf_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct file file;
	struct ezybuf_queue *q = filp->private_data;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	int i;
	int flag = 0;

	for (i = 0; i < q->buf_cnt; i++) {
		if (q->buffers[i]->memory == EZY_MEMORY_USERPTR)
			return -EAGAIN;

		if (offset == q->buffers[i]->phyaddr) {
			printk("ezybuf_mmap break addr:%ld\n", offset);
			flag = 1;
			break;
		}
	}

	if (!flag) {
		if (q->fops->mmap) {
			ezy_usrpriv_set(&file, filp, q);
			return q->fops->mmap(&file, vma);
		}

		return -EAGAIN;
	}

	if (!q->cache) {
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	} else {
		vma->vm_page_prot =	pgprot_noncached(vma->vm_page_prot); //add by dfyuan for forbiden no Cache and buffer 20170623
	}

	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
		return -EAGAIN;
	}

	return SUCESS;

}

static long ezybuf_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct file file;
	struct ezybuf_queue *q = filp->private_data;
	int retval = 0;

	if (_IOC_TYPE(cmd) != EZY_IOC_BASE) {
		if (q->fops->unlocked_ioctl) {
			ezy_usrpriv_set(&file, filp, q);
			retval = q->fops->unlocked_ioctl(&file, cmd, arg);
		}
		return retval;
	}

	if (_IOC_NR(cmd) > EZY_IOC_MAXNR) {
		printk("Bad command Value EZY\n");
		return -1;
	}
	if (_IOC_DIR(cmd) & _IOC_READ)
		retval = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		retval = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));

	if (retval) {
		printk("access denied\n");
		return -1;
	}

	switch (cmd) {
	case EZY_REQBUF:
		retval = ezybuf_reqbuf(q, (struct ezy_reqbufs *) arg);
		break;

	case EZY_QUERYBUF:
		retval = ezybuf_querybuf(q, (struct ezy_buffer *) arg);
		break;

	case EZY_QBUF:
		retval = ezybuf_qbuf(q, (struct ezy_buffer *) arg);
		break;

	case EZY_DQBUF:
		retval = ezybuf_dqbuf(q, (struct ezy_buffer *)arg, (filp->f_flags & O_NONBLOCK) ? 1 : 0);
		break;

	case EZY_STREAMON:
		retval = ezybuf_streamon(q);
		break;

	case EZY_STREAMOFF:
		retval = ezybuf_streamoff(q);
		break;

	default:
		break;

	}

	return retval;
}

static ssize_t ezybuf_read(struct file *filp, char __user *bufer, size_t count, loff_t *ppos)
{
	struct file file;
	struct ezybuf_queue *q = filp->private_data;
	int retval = 0;

	if (q->fops->read) {
		ezy_usrpriv_set(&file, filp, q);
		retval = q->fops->read(&file, bufer, count, ppos);
	}

	return retval;
}

static ssize_t ezybuf_write(struct file *filp, const char *bufer, size_t count, loff_t *ppos)
{
	struct file file;
	struct ezybuf_queue *q = filp->private_data;
	int retval = 0;

	if (q->fops->write) {
		ezy_usrpriv_set(&file, filp, q);
		retval = q->fops->write(&file, bufer, count, ppos);
	}

	return retval;
}

static struct file_operations ezybuf_fops = {
	.owner 	 		= THIS_MODULE,
	.open 	 		= ezybuf_open,
	.release 		= ezybuf_release,
	.unlocked_ioctl = ezybuf_ioctl,
	.read	 		= ezybuf_read,
	.write	 		= ezybuf_write,
	.mmap 	 		= ezybuf_mmap,
};

int ezybuf_queue_init(struct ezybuf_queue *q, struct file_operations *fops, unsigned int msize, unsigned int cache, char *name, void *priv)
{
	int retval;
	unsigned long flags;

	if (!q || !name || !fops || !msize || (msize < sizeof(struct ezybuf_buffer)))
		return -EINVAL;

	memset(q, 0, sizeof(*q));
	q->fops = fops;
	q->priv = priv;
	q->msize = msize;
	q->cache = cache;
	strncpy(q->name, name, sizeof(q->name));

	spin_lock_init(&q->irqlock);
	mutex_init(&q->lock);
	kref_init(&q->kref);

	INIT_LIST_HEAD(&q->empty);
	INIT_LIST_HEAD(&q->full);

	cdev_init(&q->c_dev, &ezybuf_fops);
	q->c_dev.owner = THIS_MODULE;
	q->c_dev.ops = &ezybuf_fops;

	spin_lock_irqsave(&q->irqlock, flags);
	q->id = ezybuf_minor;
	ezybuf_minor++;
	spin_unlock_irqrestore(&q->irqlock, flags);

	if (q->id >= EZYBUF_DEV_COUNT) {
		printk("too many ezybuf device\n");
		return -EINVAL;
	}

	retval = cdev_add(&q->c_dev, MKDEV(MAJOR(ezybuf_devt), q->id), 1);
	if (retval) {
		printk("Error %d adding ..error no:", retval);
		return retval;
	}

	q->ezybuf_dev = device_create(ezybuf_class, NULL, MKDEV(MAJOR(ezybuf_devt), q->id), "%s", name);
	if (q->ezybuf_dev == NULL) {
		printk("could not register device class\n");
		cdev_del(&q->c_dev);
		return -EINVAL;
	}

	return 0;
}

void ezybuf_queue_cancel(struct ezybuf_queue *q)
{
	device_destroy(ezybuf_class, MKDEV(MAJOR(ezybuf_devt), q->id));
	cdev_del(&q->c_dev);
	ezybuf_delete(&q->kref);
	memset(q, 0, sizeof(*q));
}

static int drvbuf_waiton(struct drvbuf_buffer *db, int intr)
{
	int retval = 0;
	DECLARE_WAITQUEUE(wait, current);

	add_wait_queue(&db->done, &wait);

	while (db->state == STATE_CMD_BLOCK) {
		set_current_state(intr  ? TASK_INTERRUPTIBLE : TASK_UNINTERRUPTIBLE);

		if (db->state == STATE_CMD_BLOCK)
			schedule();

		set_current_state(TASK_RUNNING);
		if (intr && signal_pending(current)) {
			printk("drvbuf_waiton: -EINTR\n");
			retval = -EINTR;
			break;
		}
	}
	remove_wait_queue(&db->done, &wait);

	db->state = STATE_CMD_BLOCK;

	return retval;
}


static void *drvbuf_alloc(unsigned int size)
{
	struct drvbuf_buffer *db;

	db = kzalloc(size, GFP_KERNEL);
	if (NULL != db) {
		INIT_LIST_HEAD(&db->empty);
		INIT_LIST_HEAD(&db->full);
		init_waitqueue_head(&db->done);
		db->state = STATE_CMD_BLOCK;
	}

	return db;
}

static void drvbuf_free(struct drvbuf_queue *q)
{
	int i;
	struct drvbuf_buffer *db;

	if (q->buffers) {
		for (i = 0; i < q->buf_cnt; i++) {
			db = q->buffers[i];
			if (db) {
				kfree(db);
				q->buffers[i] = NULL;
			}
		}
		kfree(q->buffers);
		q->buffers = NULL;
	}
	return;
}

void drvbuf_wakeup(struct drvbuf_buffer *db)
{
	db->state = STATE_CMD_NONBLOCK;
	wake_up_interruptible(&db->done);
}

void drvbuf_qempty(struct drvbuf_queue *q, struct drvbuf_buffer *db)
{
	if (q == NULL || db == NULL)
		return;

	spin_lock(&q->emptylock);
	list_add_tail(&db->empty, &q->empty);
	spin_unlock(&q->emptylock);
}


void drvbuf_qfull(struct drvbuf_queue *q, struct drvbuf_buffer *db)
{
	if (q == NULL || db == NULL)
		return;

	spin_lock(&q->fulllock);
	list_add_tail(&db->full, &q->full);
	spin_unlock(&q->fulllock);
}


struct drvbuf_buffer * drvbuf_dqempty(struct drvbuf_queue *q)
{
	struct drvbuf_buffer *db = NULL;

	if (q == NULL)
		return;

	spin_lock(&q->emptylock);
	if (list_empty(&q->empty)) {
		goto done;
	}

	if (((q->empty).next) != NULL) {
		db = list_entry(q->empty.next, struct drvbuf_buffer, empty);
		if (db == NULL || (&(db->empty)) == NULL) {
			goto done;
		}

		list_del(&db->empty);
	}

done:
	spin_unlock(&q->emptylock);
	return db;
}

struct drvbuf_buffer * drvbuf_dqfull(struct drvbuf_queue *q)
{
	int retval = 0;
	struct drvbuf_buffer *db = NULL;

	spin_lock(&q->fulllock);
	if (!list_empty(&q->full)) {
		db = list_entry(q->full.next, struct drvbuf_buffer, full);
		list_del(&db->full);
	}
	spin_unlock(&q->fulllock);

	if (db) {
		if (q->nonblock == DRVBUF_Q_BLOCK) {
			retval = drvbuf_waiton(db, 1);
			if (retval < 0)
				db = NULL;
		}
	}

	return db;
}

int drvbuf_queue_init(struct drvbuf_queue *q, unsigned int buf_cnt, unsigned int msize, unsigned int nonblock)
{
	int i;
	struct drvbuf_buffer *db;

	memset(q, 0, sizeof(*q));

	if (!buf_cnt || !msize)
		return -EINVAL;

	if (buf_cnt > EZY_MAX_BUFFERS)
		return -ENOBUFS;

	if (!q->buffers)
		q->buffers = kzalloc(sizeof(*q->buffers) * buf_cnt, GFP_KERNEL);

	if (!q->buffers)
		return -ENOMEM;

	INIT_LIST_HEAD(&q->empty);
	INIT_LIST_HEAD(&q->full);
	spin_lock_init(&q->emptylock);
	spin_lock_init(&q->fulllock);

	q->buf_cnt = 0;
	for (i = 0; i < buf_cnt; i++) {
		db = q->buffers[i] = drvbuf_alloc(msize);
		if (!db)
			goto free_mem;

		q->buf_cnt++;
		drvbuf_qempty(q, db);
		drvbuf_qfull(q, db);
	}

	q->msize = msize;
	q->nonblock = nonblock;

	return 0;

free_mem:
	drvbuf_free(q);
	return -ENOBUFS;
}

void drvbuf_queue_cancel(struct drvbuf_queue *q)
{
	drvbuf_free(q);
	memset(q, 0, sizeof(*q));

	return;
}
static int __init ezybuf_init(void)
{
	int retval;

	ezybuf_class = class_create(THIS_MODULE, "ezybuf-dev");
	if (!ezybuf_class)
		return -EIO;

	retval = alloc_chrdev_region(&ezybuf_devt, 0, EZYBUF_DEV_COUNT, "ezybuf");
	if (retval < 0) {
		printk("Module intialization failed could not register character device");
		class_destroy(ezybuf_class);
		return -ENODEV;
	}

	printk("ezybuf_init OK, Built on "__DATE__ " " __TIME__ ".\n");
	return 0;
}

static void __exit ezybuf_uninit(void)
{
	class_destroy(ezybuf_class);
	unregister_chrdev_region(ezybuf_devt, EZYBUF_DEV_COUNT);
	printk("ezybuf_uninit\n");
}

EXPORT_SYMBOL_GPL(ezybuf_queue_init);
EXPORT_SYMBOL_GPL(ezybuf_queue_cancel);
EXPORT_SYMBOL_GPL(ezybuf_dqbuf_user);
EXPORT_SYMBOL_GPL(ezybuf_wakeup);
EXPORT_SYMBOL_GPL(drvbuf_queue_init);
EXPORT_SYMBOL_GPL(drvbuf_queue_cancel);
EXPORT_SYMBOL_GPL(drvbuf_qempty);
EXPORT_SYMBOL_GPL(drvbuf_qfull);
EXPORT_SYMBOL_GPL(drvbuf_dqempty);
EXPORT_SYMBOL_GPL(drvbuf_dqfull);
EXPORT_SYMBOL_GPL(drvbuf_wakeup);

module_init(ezybuf_init);
module_exit(ezybuf_uninit);
MODULE_DESCRIPTION("helper module to manage continue buffers, easy to use");
MODULE_AUTHOR("dfyuan@optical.com> [Sunny]");
MODULE_LICENSE("GPL");

