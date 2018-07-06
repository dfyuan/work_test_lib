#include <sys/mman.h>
#include "cameraIonMgr.h"

#include <ebase/trace.h>
#include <sys/file.h>

#define 	ION_DMA_BUF
#ifdef ION_DMA_BUF
//#include <linux/ion.h>
#include <ion.h>
#include <rockchip_ion.h>
#include "camsys_head.h"
#include "common_head.h"
#include <hal/hal_api.h>
#include <hal/hal_mockup.h>

#else
#include "ionalloc.h"
//#define EINVAL (1)
#endif

USE_TRACER(HAL_INFO);
USE_TRACER(HAL_ERROR);



#ifdef ION_DMA_BUF
/*
camera_ion_alloc()
*/

int camera_ion_alloc(camera_ionbuf_dev_t *dev, size_t size, camera_ionbuf_t*data){
    int err = 0;
    int map_fd;
    unsigned long vir_addr = 0;
	
//    ion_user_handle_t handle = NULL;
	unsigned long handle = 0;//ugly, compatible with ion old version and new version.

    if (dev == NULL || data == NULL) {
        TRACE( HAL_ERROR,"input parameters invalidate\n");
        return -EINVAL;
    }

    if(!(dev->iommu_enabled)){
		if(gIsNewIon)
        	err = ion_alloc(dev->client_fd, size, dev->align, ION_HEAP(4), 0, &handle);
		else
			err = ion_alloc(dev->client_fd, size, dev->align, ION_HEAP(1), 0, &handle);
        ion_get_phys(dev->client_fd,handle,&(data->phy_addr));
    }else{
	    if(gIsNewIon)
	        err = ion_alloc(dev->client_fd, size, dev->align, ION_HEAP(0), 0, &handle);
		else
			err = ion_alloc(dev->client_fd, size, dev->align, ION_HEAP(3), 0, &handle);
        data->phy_addr = 0;
    }
    if (err) {
        TRACE( HAL_ERROR,"ion alloc failed\n");
        return err;
    }

    TRACE( HAL_INFO,"handle %p\n", handle);

    err = ion_share(dev->client_fd,handle,&map_fd);
    if (err) {
        TRACE( HAL_ERROR,"ion map failed\n");
        ion_free(dev->client_fd,handle);
        return err;
    }

   
    vir_addr = (unsigned long)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, map_fd, 0);
    if (vir_addr == 0) {
        TRACE( HAL_ERROR,"ion get phys failed\n");
        ion_free(dev->client_fd,handle);
        return -1;
    }
    if(dev->iommu_enabled){
        camsys_iommu_t *iommu;
        camsys_sysctrl_t sysctrl;
        
        memset(&sysctrl,0,sizeof(camsys_sysctrl_t));
        iommu = (camsys_iommu_t *)(sysctrl.rev);
        iommu->client_fd = dev->client_fd;
        iommu->map_fd = map_fd;
        sysctrl.dev_mask = HAL_DEVID_MARVIN;
        sysctrl.on = 1;
        sysctrl.ops = CamSys_IOMMU;
        err = ioctl(dev->camsys_fd, CAMSYS_SYSCTRL, &sysctrl);
        if((err < 0) /*|| (iommu->linear_addr < 0)*/){
            TRACE( HAL_ERROR,"ion IOMMU map failed,err %d linear_addr %ul,\n",err,iommu->linear_addr);
            ion_free(dev->client_fd,handle);
            return -1;
        }
        data->phy_addr = iommu->linear_addr;
        TRACE( HAL_ERROR,"PHY ADDR (0x%x),iommu addr (0x%x)",data->phy_addr ,iommu->linear_addr);
    }
        
    data->size = size;
    data->vir_addr = vir_addr;
    data->ion_hdl = (void*)handle;
    data->map_fd    =   map_fd;


    TRACE( HAL_INFO,"success\n");

    return 0;

}

/*
camera_ion_free()
*/
int camera_ion_free(camera_ionbuf_dev_t *dev, camera_ionbuf_t *data){

    int err = 0;
    long temp_handle = 0; 

    if (dev == NULL || data == NULL) {
        TRACE( HAL_ERROR,"input parameters invalidate\n");
        return -EINVAL;
    }

    if(dev->iommu_enabled){
        camsys_iommu_t *iommu;
        camsys_sysctrl_t sysctrl;
        memset(&sysctrl,0,sizeof(camsys_sysctrl_t));
        iommu = (camsys_iommu_t *)(sysctrl.rev);
        iommu->client_fd = dev->client_fd;
        iommu->map_fd = data->map_fd;
        sysctrl.dev_mask = HAL_DEVID_MARVIN;
        sysctrl.on = 0;
        sysctrl.ops = CamSys_IOMMU;
        err = ioctl(dev->camsys_fd, CAMSYS_SYSCTRL, &sysctrl);
        if(err < 0){
            TRACE( HAL_ERROR,"ion IOMMU unmap failed\n");
            return -1;
        }
    }
    
    err = munmap((void*)(data->vir_addr), data->size);
    if (err) {
        TRACE( HAL_ERROR,"munmap failed\n");
        return err;
    }

    close(data->map_fd);
	temp_handle = (long)data->ion_hdl;
    err = ion_free(dev->client_fd, (ion_user_handle_t)temp_handle);
    if (err) {
        TRACE( HAL_ERROR,"ion free failed\n");
        return err;
    }
    
    memset(data,0,sizeof(camera_ionbuf_t));


    TRACE( HAL_INFO,"success\n");

    return 0;

}


/*
camera_ion_share()
*/
int camera_ion_share(camera_ionbuf_dev_t *dev, camera_ionbuf_t *data, unsigned int *share_id){
#if 0
	int err = 0;

    if (dev == NULL || data == NULL) {
        TRACE( HAL_ERROR,"input parameters invalidate\n");
        return -EINVAL;
    }


    err = ion_get_share_id(dev->client_fd,data->map_fd,share_id);
    if (err) {
        TRACE( HAL_ERROR,"ion share failed\n");
        return err;
    }
	return 0;
#endif
	return -1;    

}


/*
camera_ion_map()
*/
int camera_ion_map(camera_ionbuf_dev_t *dev, int share_id, size_t size, camera_ionbuf_t *mapped_data){
#if 0
    int err = 0;
    int share_fd  = 0;
    struct ion_handle* handle = NULL;
    unsigned long vir_addr = 0;



    if ((dev == NULL) || (mapped_data == NULL)) {
        TRACE( HAL_ERROR,"input parameters invalidate\n");
        return -EINVAL;
    }

    err = ion_share_by_id(dev->client_fd,&share_fd,share_id);
    if (err) {
        TRACE( HAL_ERROR,"ion map failed\n");
        return err;
    }

    err = ion_import(dev->client_fd, share_fd, &handle);
    if (err) {
        TRACE( HAL_ERROR,"ion import failed\n");
        return err;
    }

    vir_addr = (unsigned long)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, share_fd, 0);
    if (vir_addr == 0) {
        TRACE( HAL_ERROR,"ion mmap failed\n");
        return -1;
    }

    ion_get_phys(dev->client_fd,handle,&(mapped_data->phy_addr));
    mapped_data->size = size;
    mapped_data->vir_addr = vir_addr;
    mapped_data->ion_hdl = (void*)handle;
    mapped_data->map_fd = share_fd;
	return 0;

#endif
	return -1;
}


/*
camera_ion_unmap()
*/
int camera_ion_unmap(camera_ionbuf_dev_t *dev, camera_ionbuf_t *data){

    int err = 0;
    long temp_handle = 0; 

    if (dev == NULL || data == NULL) {
        TRACE( HAL_ERROR,"input parameters invalidate\n");
        return -EINVAL;
    }

    err = munmap((void*)(data->vir_addr), data->size);
    if (err) {
        TRACE( HAL_ERROR,"munmap failed\n");
        return err;
    }

    close(data->map_fd);
	temp_handle = (long)data->ion_hdl;
    err = ion_free(dev->client_fd, (ion_user_handle_t)temp_handle);
    if (err) {
        TRACE( HAL_ERROR,"ion unmap failed\n");
        return err;
    }

    memset(data,0,sizeof(camera_ionbuf_t));
    TRACE( HAL_INFO,"success\n");
	return 0;

}

#else
/*
camera_ion_alloc()
*/

int camera_ion_alloc(camera_ionbuf_dev_t *dev, size_t size, camera_ionbuf_t*data){
    ion_device_t *ion_device = NULL;
    ion_buffer_t* ion_buffer = NULL;


    if (dev == NULL || data == NULL) {
        TRACE( HAL_ERROR,"input parameters invalidate\n");
        return -EINVAL;
    }

    ion_device = (ion_device_t *)dev->client_fd;


    if(ion_device->alloc(ion_device, size, _ION_HEAP_RESERVE, &ion_buffer) !=0){
        TRACE( HAL_ERROR,"ion alloc failed\n");
        return -EINVAL;

    }

    data->phy_addr = ion_buffer->phys;
    data->vir_addr = (unsigned long)ion_buffer->virt;
    data->size  =  ion_buffer->size;

    data->ion_hdl = (void*)(ion_buffer); 

    return 0;


}


/*
camera_ion_free()
*/
int camera_ion_free(camera_ionbuf_dev_t *dev, camera_ionbuf_t *data){

    ion_device_t *ion_device = NULL;
    if (dev == NULL || data == NULL) {
        TRACE( HAL_ERROR,"input parameters invalidate\n");
        return -EINVAL;
    }

    ion_device = (ion_device_t *)dev->client_fd;

    if(ion_device->free(ion_device, (ion_buffer_t*)(data->ion_hdl)) != 0){
        TRACE( HAL_ERROR,"ion free failed\n");
        return -EINVAL;
    }

    memset(data,0,sizeof(camera_ionbuf_t));

    return 0;
}


/*
camera_ion_share()
*/
int camera_ion_share(camera_ionbuf_dev_t *dev, camera_ionbuf_t *data, unsigned int *share_id){
    TRACE( HAL_ERROR,"ion DON'T support share\n");

    return 0;

}


/*
camera_ion_map()
*/
int camera_ion_map(camera_ionbuf_dev_t *dev, int share_id, size_t size, camera_ionbuf_t *mapped_data){

    TRACE( HAL_ERROR,"ion DON'T support map\n");


    return 0;

}


/*
camera_ion_unmap()
*/
int camera_ion_unmap(camera_ionbuf_dev_t *dev, camera_ionbuf_t *data){
    TRACE( HAL_ERROR,"ion DON'T support unmap\n");


    return 0;

}

#endif
/*

camera_ion_open()
*/

int camera_ion_open(unsigned long align, camera_ionbuf_dev_t *dev){

    if(dev == NULL){
        TRACE( HAL_ERROR,"ion open failed\n");
        return -EINVAL;
    }

#ifdef ION_DMA_BUF

    dev->client_fd = ion_open();
    dev->alloc = camera_ion_alloc;
    dev->free = camera_ion_free;
    dev->map = camera_ion_map;
    dev->unmap = camera_ion_unmap;
    dev->share = camera_ion_share;
    dev->align = align;
#else

    struct ion_device_t * tmp_dev =  NULL;

    if(ion_open(align, ION_NUM_MODULES, &tmp_dev) < 0){
        TRACE( HAL_ERROR,"ion open failed\n");
        return -EINVAL;

    }
    dev->client_fd = (int)tmp_dev;
    dev->alloc = camera_ion_alloc;
    dev->free = camera_ion_free;
    dev->map = NULL;
    dev->unmap = NULL;
    dev->share = NULL;
    dev->align = align;
#endif

    return 0;
}

/*
camera_ion_close()
*/

int camera_ion_close(camera_ionbuf_dev_t *dev){

    if(dev == NULL){
        TRACE( HAL_ERROR,"ion open failed\n");
        return -EINVAL;
    }

#ifdef ION_DMA_BUF
    ion_close(dev->client_fd);
#else
    ion_close((ion_device_t *)(dev->client_fd));

#endif

    return 0;

}



