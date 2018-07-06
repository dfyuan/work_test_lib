#ifndef _CAMERA_ION_BUF_H_
#define _CAMERA_ION_BUF_H_

#include <ebase/types.h>

typedef struct camera_ionbuf_s
{
    void* ion_hdl;
    int   map_fd;
    unsigned long vir_addr;
    unsigned long phy_addr;
    size_t size;
    unsigned int share_id;

}camera_ionbuf_t;

typedef struct camera_ionbuf_dev_s
{
    int client_fd;
	int camsys_fd;
	bool_t iommu_enabled;
    unsigned long align;
    int (*alloc)(struct camera_ionbuf_dev_s *dev, size_t size, camera_ionbuf_t *data);
    int (*free)(struct camera_ionbuf_dev_s *dev, camera_ionbuf_t *data);
    int (*map)(struct camera_ionbuf_dev_s *dev, int share_id, size_t size, camera_ionbuf_t *data);
    int (*unmap)(struct camera_ionbuf_dev_s *dev, camera_ionbuf_t *data);
    int (*share)(struct camera_ionbuf_dev_s *dev, camera_ionbuf_t *data, unsigned int *share_id);

}camera_ionbuf_dev_t;


/*

camera_ion_open()
*/

 int camera_ion_open(unsigned long align, camera_ionbuf_dev_t *dev);

/*
camera_ion_close()
*/

int camera_ion_close(camera_ionbuf_dev_t *dev);

#endif
