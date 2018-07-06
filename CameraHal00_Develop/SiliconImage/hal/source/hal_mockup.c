/******************************************************************************
 *
 * Copyright 2010, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
/**
 * @file hal_api.c
 *
 * @brief    This source file implements the mockup version of the HAL.
 *
 *****************************************************************************/

#include <ebase/trace.h>
#include <common/align.h>
#include "hal_api.h" // includes "hal_mockup.h" as well

#if defined ( HAL_MOCKUP )

//#include "ionalloc.h"

#include "cameraIonMgr.h"
//#include <../include/cameraIonMgr.h>
#include "camera_mem.h"


CREATE_TRACER(HAL_NOTICE0, "HAL-MOCKUP: ", TRACE_NOTICE0, 1);
CREATE_TRACER(HAL_NOTICE1, "HAL-MOCKUP: ", TRACE_NOTICE1, 1);
CREATE_TRACER(HAL_INFO , "HAL-MOCKUP: ", INFO, 1);
CREATE_TRACER(HAL_ERROR, "HAL-MOCKUP: ", ERROR, 1);


/******************************************************************************
 * local macro definitions
 *****************************************************************************/
#define NUM_I2C                  6
#define NUM_EXTDEV               3

/******************************************************************************
 * local type definitions
 *****************************************************************************/
typedef struct HalCamConfig_s
{
    uint32_t      dev_id;

    bool_t        configured;      //!< Mark whether this config was set.
    bool_t        power_lowact;    //!< Power on is low-active.
    bool_t        reset_lowact;    //!< Reset is low-active.
    //bool_t negedge;         //!< Capture data on negedge.

    uint32_t      clkin_frequence;
} HalCamConfig_t;


typedef struct HalMemMap_s
{
    ulong_t mem_address;    //!< Hardware memory address.
    uint32_t        byte_size;      //!< Size of mapped buffer.
    HalMapMemType_t mapping_type;   //!< How the buffer is mapped.
} HalMemMap_t;

struct MemBlockInfo_s{
    List node;
	//old mode
    camera_ionbuf_t data;
	//new mode
	cam_mem_info_t* new_data;
};

typedef struct HalMemManager_s
{
	//old mode
    camera_ionbuf_dev_t *ion_device;
	//new mode
	cam_mem_handle_t* cam_mem_handle;
    List halMemList;
}HalMemManager_t;

typedef struct pthread_fake_s {
	#if defined(ANDROID_5_X)
	void* nothing[2];
	#else
    void* nothing[8];
	#endif
    int pid;
} pthread_fake_t;

typedef struct HalDrvMemInfo_s
{
    uint32_t        *base;
    uint32_t        size;
} HalDrvMemInfo_t;

typedef struct HalDrvInfo_s
{
    int camsys_fd;

    HalDrvMemInfo_t    i2cmem;
    HalDrvMemInfo_t    regmem;

    uint32_t           mipi_lanes;
} HalDrvInfo_t;

/******************************************************************************
 * HalContext_t
 *****************************************************************************/
typedef struct HalContext_s
{
    osMutex             modMutex;               //!< common short term mutex; e.g. for read-modify-write accesses
    uint32_t            refCount;               //!< internal ref count

    osMutex             iicMutex[NUM_I2C];      //!< transaction locks for I2C controller 1..NUM_I2C

    HalCamConfig_t      camConfig[NUM_EXTDEV];  //!< configuration for CAM1; set at runtime

    HalMemManager_t     memMng;
    HalDrvInfo_t        drvInfo;
	uint32_t  sensorpowerupseq;
	uint32_t  vcmpowerupseq;	
	uint32_t avdd_delay;
	uint32_t dovdd_delay;
	uint32_t dvdd_delay;
	uint32_t vcmvdd_delay;
	uint32_t pwr_delay;
	uint32_t vcmpwr_delay;
	uint32_t rst_delay;
	uint32_t pwrdn_delay;
	uint32_t vcmpwrdn_delay;
	uint32_t clkin_delay;	
	uint32_t gInitialized;
	cam_mem_ops_t* ops;
} HalContext_t;


/******************************************************************************
 * local variable declarations
 *****************************************************************************/
static HalContext_t gHalContext[2]  = { {.refCount = 0}, {.refCount = 0} };
static bool_t       gInitialized = false;
bool_t				gIsNewIon    = false;
/******************************************************************************
 * local function prototypes
 *****************************************************************************/
static int32_t halIsrHandler( void *pArg );

static RESULT HalLockContext( HalContext_t *pHalCtx );
static RESULT HalUnlockContext( HalContext_t *pHalCtx );


/******************************************************************************
 * API functions; see header file for detailed comment.
 *****************************************************************************/

/******************************************************************************
 * HalNumOfCams()
 *****************************************************************************/
RESULT HalNumOfCams( HalHandle_t HalHandle, uint32_t *no )
{
    RESULT result = RET_SUCCESS;

    if ( no )
    {
        *no = 0;
    }
    else
    {
        result = RET_NULL_POINTER;
    }
   
    return ( result );
}


/******************************************************************************
 * HalOpen()
 *****************************************************************************/
HalHandle_t HalOpen( char* dev_filename , HalPara_t *para)
{
    HalContext_t *pHalCtx = NULL;
    RESULT result = RET_SUCCESS;
    RESULT lres;
    OSLAYER_STATUS osStatus = OSLAYER_OK;
    int32_t currI2c = -1,i,camsys_fd=0,err=0;
    uint32_t *base=NULL;
    char  camsys_devname[32];
    camsys_querymem_t qmem;
	
    //TODO: need a global mutex for manipulating gInitialized
    // just one instance allowed
    if (dev_filename == NULL) 
    {
        TRACE( HAL_ERROR, "%s(%d): dev_filename is NULL",__FUNCTION__,__LINE__);
        return NULL;
    }
    
    // 'create' context
    if(!strcmp(dev_filename,"/dev/camsys_marvin1"))
		pHalCtx = &gHalContext[0];
	else
		pHalCtx = &gHalContext[1];
	pHalCtx->gInitialized = true;
    memset( pHalCtx, 0, sizeof(HalContext_t) );
    pHalCtx->refCount = 1;

    // open board
    // n.a.
    // initialize context
    if ( OSLAYER_OK != osMutexInit( &pHalCtx->modMutex ) )
    {
        TRACE( HAL_ERROR, "%s(%d):Can't initialize modMutex\n" ,__FUNCTION__,__LINE__);
        goto cleanup_2;
    }

    for ( currI2c = 0; currI2c < NUM_I2C; currI2c++ )
    {
        if ( OSLAYER_OK != osMutexInit( &pHalCtx->iicMutex[currI2c] ) )
        {
            TRACE( HAL_ERROR, "%s(%d):Can't initialize iicMutex #%d\n",__FUNCTION__,__LINE__, currI2c );
            goto cleanup_3;
        }
    }

    for ( i = 0; i < NUM_EXTDEV; i++ )
    {
        pHalCtx->camConfig[i].configured = false;    
    }

	if (!para->mem_ops) {
	    //open ion device
	    pHalCtx->memMng.ion_device = (camera_ionbuf_dev_t*)malloc(sizeof(camera_ionbuf_dev_t));
	    if(pHalCtx->memMng.ion_device == NULL){
	        TRACE( HAL_ERROR, "Can't alloc memory for ion device\n" );
	        goto cleanup_2;
	    }

	    if( camera_ion_open(getpagesize(), pHalCtx->memMng.ion_device) < 0)
	    {
	        TRACE( HAL_ERROR, "%s(%d): Can't open ion device\n" ,__FUNCTION__,__LINE__);
	        goto cleanup_3;
	    }
	} else {
		cam_mem_ops_t* ops = (cam_mem_ops_t*)(para->mem_ops);
		pHalCtx->memMng.cam_mem_handle = ops->init(
			1, //iommu
			CAM_MEM_FLAG_HW_WRITE | CAM_MEM_FLAG_HW_READ | CAM_MEM_FLAG_SW_WRITE | CAM_MEM_FLAG_SW_READ,
			0 //physical continuous
			);
		if (pHalCtx->memMng.cam_mem_handle == NULL) {
	        TRACE( HAL_ERROR, "Can't get memory alloc handle\n" );
	        goto cleanup_2;
		}
		pHalCtx->ops = ops;
	}

	if(!strcmp(dev_filename,"/dev/camsys_marvin1")){
		camsys_fd = open(dev_filename, O_RDWR);
		if (camsys_fd < 0) {
			TRACE(HAL_ERROR,"Open (%s) failed, error=(%s),try to open /dev/camsys_marvin \n", dev_filename,strerror(errno));
			camsys_fd = open("/dev/camsys_marvin", O_RDWR);
			if (camsys_fd<0)
			{
				TRACE( HAL_ERROR, "%s(%d):Open %s failed!, error=(%s)\n",__FUNCTION__,__LINE__,"/dev/camsys_marvin",strerror(errno));
				goto cleanup_3;
			}
		}	 
	}else{    
	    camsys_fd = open(dev_filename, O_RDWR);
	    if (camsys_fd<0)
	    {
	        TRACE( HAL_ERROR, "%s(%d):Open %s failed!",__FUNCTION__,__LINE__,dev_filename);
	        goto cleanup_3;
	    }
	}

    //query iommu is enabled ?
    if (pHalCtx->memMng.ion_device)
    {
        int iommu_enabled = 0;
    	err = ioctl(camsys_fd, CAMSYS_QUREYIOMMU, &iommu_enabled);
        if(err<0) {
            pHalCtx->memMng.ion_device->iommu_enabled = false;
            TRACE( HAL_ERROR,"CAMSYS_QUREYIOMMU failed !!!!\n"); 
        }else{
            pHalCtx->memMng.ion_device->iommu_enabled = (iommu_enabled == 1) ? true:false;

        }
		pHalCtx->memMng.ion_device->camsys_fd = camsys_fd;
    } else
		pHalCtx->memMng.cam_mem_handle->camsys_fd = camsys_fd;

    //init mem list
    ListInit(&(pHalCtx->memMng.halMemList));

    //zyc add
    pHalCtx->drvInfo.camsys_fd = camsys_fd;

    // mmap register 
    qmem.mem_type = CamSys_Mmap_RegisterMem;
    err = ioctl(camsys_fd, CAMSYS_QUREYMEM, &qmem);
    if (err<0) {
        TRACE( HAL_ERROR, "CamSys_Mmap_RegisterMem query failed\n");
        goto cleanup_3;
    } else {        
        base = (unsigned int*)mmap(0,qmem.mem_size,
                        PROT_READ|PROT_WRITE, MAP_SHARED,
                        camsys_fd,qmem.mem_offset);
        if (base == MAP_FAILED) {
            TRACE( HAL_ERROR, "%s register memory mmap failed!\n",camsys_devname);
          
        } else {
            pHalCtx->drvInfo.regmem.base = base;
            pHalCtx->drvInfo.regmem.size = qmem.mem_size;
            TRACE( HAL_NOTICE1, "regmem.base = 0x%x,regmem.size = 0x%x!",base,qmem.mem_size);
        }        
    }

    
    // mmap i2c memory
    qmem.mem_type = CamSys_Mmap_I2cMem;
    err = ioctl(camsys_fd, CAMSYS_QUREYMEM, &qmem);
    if (err<0) {
        TRACE( HAL_ERROR, "CamSys_Mmap_I2cMem query failed\n");
        goto cleanup_3;
    } else {
        base = (unsigned int*)mmap(0,qmem.mem_size,
                        PROT_READ|PROT_WRITE, MAP_SHARED,
                        camsys_fd,qmem.mem_offset);
        if (base == MAP_FAILED) {
            TRACE( HAL_ERROR, "%s i2c memory mmap failed!\n",camsys_devname);
        } else {
            pHalCtx->drvInfo.i2cmem.base = base;
            pHalCtx->drvInfo.i2cmem.size = qmem.mem_size;
        }
    }

    pHalCtx->drvInfo.mipi_lanes = para->mipi_lanes;
	gIsNewIon = para->is_new_ion;
	pHalCtx->sensorpowerupseq = para->sensorpowerupseq;
	pHalCtx->vcmpowerupseq = para->vcmpowerupseq;
	pHalCtx->avdd_delay = para->avdd_delay;
	pHalCtx->dvdd_delay = para->dvdd_delay;
	pHalCtx->dovdd_delay = para->dovdd_delay;
	pHalCtx->pwr_delay = para->pwr_delay;
	pHalCtx->pwrdn_delay = para->pwrdn_delay;
	pHalCtx->rst_delay = para->rst_delay;
	pHalCtx->clkin_delay = para->clkin_delay;
	pHalCtx->vcmvdd_delay = para->vcmvdd_delay;
	pHalCtx->vcmpwr_delay = para->vcmpwr_delay;
	pHalCtx->vcmpwrdn_delay = para->vcmpwrdn_delay;
	
    HalSetPower(pHalCtx, HAL_DEVID_MARVIN, 1);
    HalSetReset(pHalCtx, HAL_DEVID_MARVIN, 0);

    // success 
    return (HalHandle_t)pHalCtx;

    // failure cleanup
cleanup_3: // cleanup some mutexes

    if (camsys_fd>0)
    {
        if (pHalCtx->drvInfo.regmem.base)
        {
            munmap(pHalCtx->drvInfo.regmem.base,pHalCtx->drvInfo.regmem.size);
            pHalCtx->drvInfo.regmem.base = NULL;
            pHalCtx->drvInfo.regmem.size = 0;
        }

        if (pHalCtx->drvInfo.i2cmem.base)
        {
            munmap(pHalCtx->drvInfo.i2cmem.base,pHalCtx->drvInfo.i2cmem.size);
            pHalCtx->drvInfo.i2cmem.base = NULL;
            pHalCtx->drvInfo.i2cmem.size = 0;
        }

        close(camsys_fd);
        camsys_fd = -1;
    }

    if (pHalCtx->memMng.ion_device) 
        camera_ion_close(pHalCtx->memMng.ion_device);
	
	if (pHalCtx->memMng.cam_mem_handle) {
		cam_mem_ops_t* ops = (cam_mem_ops_t*)(para->mem_ops);
		ops->deInit(pHalCtx->memMng.cam_mem_handle);
		pHalCtx->ops = NULL;
	}

    while ( currI2c > 0 )
    {
        osMutexDestroy( &pHalCtx->iicMutex[--currI2c] );
    }

    osMutexDestroy( &pHalCtx->modMutex );

cleanup_2: // close board
    // n.a.
    if (pHalCtx->memMng.ion_device) {
        free(pHalCtx->memMng.ion_device);
    }

cleanup_1: // 'destroy' context
    memset( pHalCtx, 0, sizeof(HalContext_t) );
    pHalCtx->refCount = 0;

cleanup_0: // uninitialized again
    gInitialized = false; //TODO: need a global mutex for manipulating gInitialized

    return NULL;
}


/******************************************************************************
 * HalClose()
 *****************************************************************************/
RESULT HalClose( HalHandle_t HalHandle )
{
    RESULT result = RET_SUCCESS;
    OSLAYER_STATUS osStatus;
    int32_t currI2c = -1,i;

    if (HalHandle == NULL)
    {
        TRACE( HAL_ERROR, "%s(%d): HalHandle is NULL\n",__FUNCTION__,__LINE__);
        return RET_NULL_POINTER;
    }

    if ((HalHandle != (HalHandle_t)&gHalContext[0]) && (HalHandle != (HalHandle_t)&gHalContext[1]))
    {
        return RET_WRONG_HANDLE;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        TRACE( HAL_ERROR, "%s(%d): refCount is invaldate(0)\n",__FUNCTION__,__LINE__);
        return RET_WRONG_STATE;
    }

    if (pHalCtx->drvInfo.camsys_fd<=0)
    {
        TRACE( HAL_ERROR, "pHalCtx is error for haven't camsys device!\n" );
        return RET_WRONG_HANDLE;
    }

    //TODO: need a mutex for manipulating pHalCtx->refCount
    if (pHalCtx->refCount == 1)
    {
        RESULT lres;
        // get common device mask
        uint32_t dev_mask = HAL_DEVID_MARVIN;

        for (i=0; i<NUM_EXTDEV; i++)
        {
            if (pHalCtx->camConfig[i].configured)
            {
                dev_mask |= (1<<(i+24));
            }
        } 

        // power off all devices
        lres = HalSetPower( HalHandle, dev_mask, false );
        if (RET_SUCCESS != lres)
        {
            TRACE( HAL_ERROR, "PowerOff of devices (0x%08x) failed\n", dev_mask );
            UPDATE_RESULT( result, lres );
        }

        // cleanup context
        for ( currI2c = 0; currI2c < NUM_I2C; currI2c++ )
        {
            osStatus = osMutexDestroy( &pHalCtx->iicMutex[currI2c] );
            UPDATE_RESULT( result, (OSLAYER_OK == osStatus) ? RET_SUCCESS : RET_FAILURE );
        }

        osStatus = osMutexDestroy( &pHalCtx->modMutex );
        UPDATE_RESULT( result, (OSLAYER_OK == osStatus) ? RET_SUCCESS : RET_FAILURE );

        // close board
        // n.a.
        if (pHalCtx->drvInfo.camsys_fd>0)
        {
            if (pHalCtx->drvInfo.regmem.base)
            {
                munmap(pHalCtx->drvInfo.regmem.base,pHalCtx->drvInfo.regmem.size);
                pHalCtx->drvInfo.regmem.base = NULL;
                pHalCtx->drvInfo.regmem.size = 0;
            }

            if (pHalCtx->drvInfo.i2cmem.base)
            {
                munmap(pHalCtx->drvInfo.i2cmem.base,pHalCtx->drvInfo.i2cmem.size);
                pHalCtx->drvInfo.i2cmem.base = NULL;
                pHalCtx->drvInfo.i2cmem.size = 0;
            }
            
            close(pHalCtx->drvInfo.camsys_fd);
            pHalCtx->drvInfo.camsys_fd = -1;
        }
        
        //close ion device
        if (pHalCtx->memMng.ion_device){
            camera_ion_close(pHalCtx->memMng.ion_device);
            free(pHalCtx->memMng.ion_device);
        }

		if (pHalCtx->memMng.cam_mem_handle) {
			cam_mem_ops_t* ops = pHalCtx->ops;
			ops->deInit(pHalCtx->memMng.cam_mem_handle);
			pHalCtx->ops = NULL;
		}

        // 'destroy' context
        memset( pHalCtx, 0, sizeof(HalContext_t) );
        pHalCtx->refCount = 0;

        // not initialized again
        gInitialized = false; //TODO: need a global mutex for manipulating gInitialized
    }
    else
    {
        //TODO: need a mutex for manipulating pHalCtx->refCount
        pHalCtx->refCount--;
    }

    return result;
}


/******************************************************************************
 * HalAddRef()
 *****************************************************************************/
RESULT HalAddRef( HalHandle_t HalHandle )
{
    if (HalHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        return RET_WRONG_STATE;
    }

    //TODO: need a mutex for manipulating pHalCtx->refCount
    pHalCtx->refCount++;

    return RET_SUCCESS;
}


/******************************************************************************
 * HalDelRef()
 *****************************************************************************/
RESULT HalDelRef( HalHandle_t HalHandle )
{
    // just forward to HalClose; it takes care of everything
    return HalClose( HalHandle );
}


/******************************************************************************
 * HalSetCamConfig()
 *****************************************************************************/
RESULT HalSetCamConfig( HalHandle_t HalHandle, uint32_t dev_mask, bool_t power_lowact, bool_t reset_lowact, bool_t pclk_negedge )
{
    RESULT result = RET_SUCCESS;
    RESULT lres;
    uint32_t i;

    if (HalHandle == NULL)
    {
        TRACE( HAL_ERROR, "%s(%d): HalHandle is NULL\n",__FUNCTION__,__LINE__);
        return RET_NULL_POINTER;
    }

    // check for valid devices
    if (dev_mask & ~(HAL_DEVID_EXTERNAL))
    {
        TRACE( HAL_ERROR, "%s(%d): dev_mask is invalidate\n",__FUNCTION__,__LINE__,dev_mask);
        return RET_INVALID_PARM;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        TRACE( HAL_ERROR, "%s(%d): refCount is invaldate(0)\n",__FUNCTION__,__LINE__);
        return RET_WRONG_STATE;
    }

    if (pHalCtx->drvInfo.camsys_fd<=0)
    {
        TRACE( HAL_ERROR, "pHalCtx is error for haven't camsys device!\n" );
        return RET_WRONG_HANDLE;
    }
    
    // update config(s)
    //NOTE: select clock edges for all CAM devices at once as well
    //TODO: need a mutex around the following block:
    {
        // build external devices & high-active external devices masks
        // duplicate & modify this 'if'-block for further external devices 
        for (i=0; i<NUM_EXTDEV; i++)
        {
            if (dev_mask & (1<<(i+24)))
            {
            	pHalCtx->camConfig[i].dev_id = (1<<(i+24));
                pHalCtx->camConfig[i].configured   = true;
                pHalCtx->camConfig[i].power_lowact = power_lowact;
                pHalCtx->camConfig[i].reset_lowact = reset_lowact;
            }
        }
        
    }

    // set default state (power down, reset active)
    lres = HalSetPower( HalHandle, dev_mask, false );
    UPDATE_RESULT( result, lres );
    lres = HalSetReset( HalHandle, dev_mask, true );
    UPDATE_RESULT( result, lres );

    return result;
}

/******************************************************************************
 * HalSetCamPhyConfig()
 *****************************************************************************/
RESULT HalSetCamPhyConfig( HalHandle_t HalHandle, uint32_t dev_mask, bool_t power_lowact, bool_t reset_lowact )
{
    RESULT result = RET_SUCCESS;
    

    return result;
}

/******************************************************************************
 * HalSetReset()
 *****************************************************************************/
RESULT HalSetReset( HalHandle_t HalHandle, uint32_t dev_mask, bool_t activate )
{
    RESULT result = RET_SUCCESS;
    uint32_t i,dev_mask_active;
    int32_t err;
    camsys_sysctrl_t sysctl;
    
    if (HalHandle == NULL)
    {
        TRACE( HAL_ERROR, "%s(%d): HalHandle is NULL\n",__FUNCTION__,__LINE__);
        return RET_NULL_POINTER;
    }

    // check for valid devices
    if (dev_mask & ~HAL_DEVID_ALL)
    {
        TRACE( HAL_ERROR, "%s(%d): dev_mask is invalidate\n",__FUNCTION__,__LINE__,dev_mask);
        return RET_INVALID_PARM;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        TRACE( HAL_ERROR, "%s(%d): refCount is invaldate(0)\n",__FUNCTION__,__LINE__);
        return RET_WRONG_STATE;
    }

    if (pHalCtx->drvInfo.camsys_fd<=0)
    {
        TRACE( HAL_ERROR, "pHalCtx is error for haven't camsys device!\n" );
        return RET_WRONG_HANDLE;
    }
    
    // first process all internal devices
    //...

    if (dev_mask & HAL_DEVID_INTERNAL) {
        sysctl.dev_mask = (dev_mask & HAL_DEVID_INTERNAL);
        sysctl.ops = CamSys_Rst;
        sysctl.on = (activate)?1:0;

        err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_SYSCTRL, &sysctl);
        if (err<0)
        {
            UPDATE_RESULT(result, RET_FAILURE);
            TRACE(HAL_ERROR,"Intdev(0x%x) CamSys_Rst failed\n",sysctl.dev_mask);
        }
    }

    
    // then process all external devices
    //TODO: need a mutex around the following block:
    {
        dev_mask_active = 0;
        // build external devices & high-active external devices masks
        // duplicate & modify this 'if'-block for further external devices
        for(i=0; i<NUM_EXTDEV; i++)
        {
            if (dev_mask & (1<<i))
            {
                if (!pHalCtx->camConfig[i].configured) 
                {
                    UPDATE_RESULT( result, RET_FAILURE );
                } else {
                    dev_mask_active |= (1<<(i+24));
                }
            }
        }

        sysctl.dev_mask = dev_mask_active;
        sysctl.ops = CamSys_Rst;
        sysctl.on = activate;
        err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_SYSCTRL, &sysctl);
        if (err<0)
        {
            UPDATE_RESULT(result, RET_FAILURE);
        }
    }

    return result;
}

RESULT HalSensorPwrSeq(HalHandle_t HalHandle, uint32_t dev_mask, bool_t activate)
{
    RESULT result = RET_SUCCESS;
    int32_t i,j,powerup_type,err;
	camsys_sysctrl_t sysctl;
	HalContext_t *pHalCtx = (HalContext_t *)HalHandle;

 // process external devices only
 //TODO: need a mutex around the following block:
 {
	 // build external devices & high-active external devices masks
	 // duplicate & modify this 'if'-block for further external devices
	 if (dev_mask & HAL_DEVID_EXTERNAL)
	 {
		 for (i=0; i<SENSOR_PWRSEQ_CNT; i++) {
		 
			  if (activate)
				  powerup_type = POWERSEQ_GET(pHalCtx->sensorpowerupseq,i);
			  else
				  powerup_type = POWERSEQ_GET(pHalCtx->sensorpowerupseq,(SENSOR_PWRSEQ_CNT-1-i));

			  switch (powerup_type)
			  {
				  case SENSOR_PWRSEQ_AVDD:
				  {
					  sysctl.dev_mask = (dev_mask & HAL_DEVID_EXTERNAL);
					  sysctl.ops = CamSys_Avdd;
					  sysctl.on = (activate)?1:0;
					  err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_SYSCTRL, &sysctl);
					  if (err<0) {
						  TRACE(HAL_ERROR,"CamSys_Avdd on failed!\n");
					  }
					  usleep(pHalCtx->avdd_delay);
					  break;
				  }
				  case SENSOR_PWRSEQ_DOVDD:
				  {
					  sysctl.dev_mask = (dev_mask & HAL_DEVID_EXTERNAL);
					  sysctl.ops = CamSys_Dovdd;
					  sysctl.on = (activate)?1:0;
					  err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_SYSCTRL, &sysctl);
					  if (err<0) {
						  TRACE(HAL_ERROR,"CamSys_Dovdd on failed!\n");
					  }
					  usleep(pHalCtx->dovdd_delay);
					  break;
				  } 			  
				  case SENSOR_PWRSEQ_DVDD:
				  {
					  sysctl.dev_mask = (dev_mask & HAL_DEVID_EXTERNAL);
					  sysctl.ops = CamSys_Dvdd;
					  sysctl.on = (activate)?1:0;
					  err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_SYSCTRL, &sysctl);
					  if (err<0) {
						  TRACE(HAL_ERROR,"CamSys_Dvdd on failed!\n");
					  }
					  usleep(pHalCtx->dvdd_delay);
					  break;
				  }		  
				  case SENSOR_PWRSEQ_PWR:
				  {
					  sysctl.dev_mask = (dev_mask & HAL_DEVID_EXTERNAL);
					  sysctl.ops = CamSys_PwrEn;
					  sysctl.on = (activate)?1:0;
					  err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_SYSCTRL, &sysctl);
					  if (err<0) {
						  TRACE(HAL_ERROR,"CamSys_PwrEn on failed!\n");
					  }
					  usleep(pHalCtx->pwr_delay);
					  break;
				  }
				  case SENSOR_PWRSEQ_RST:
				  {
					  sysctl.dev_mask = (dev_mask & HAL_DEVID_EXTERNAL);
					  sysctl.ops = CamSys_Rst;
					  sysctl.on = (activate)?0:1;
					  err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_SYSCTRL, &sysctl);
					  if (err<0) {
						  TRACE(HAL_ERROR,"CamSys_Rst on failed!\n");
					  }
					  usleep(pHalCtx->pwr_delay);
					  break;
				  }
				  case SENSOR_PWRSEQ_PWRDN:
				  {
					  sysctl.dev_mask = (dev_mask & HAL_DEVID_EXTERNAL);
					  sysctl.ops = CamSys_PwrDn;
					  sysctl.on = (activate)?0:1;
					  err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_SYSCTRL, &sysctl);
					  if (err<0) {
						  TRACE(HAL_ERROR,"CamSys_PwrDn on failed!\n");
					  }
					  usleep(pHalCtx->pwrdn_delay);
					  break;
				  }
				  case SENSOR_PWRSEQ_CLKIN:
				  {
					  for (j=0; j<NUM_EXTDEV; j++)
					  { 			   
						  if (pHalCtx->camConfig[j].dev_id & dev_mask)
						  {
							  sysctl.dev_mask = pHalCtx->camConfig[j].dev_id;
							  sysctl.on = (activate)?pHalCtx->camConfig[j].clkin_frequence:0;
							  
							  sysctl.ops = CamSys_ClkIn;
					  
							  err = ioctl(pHalCtx->drvInfo.camsys_fd,CAMSYS_SYSCTRL, &sysctl);
							  if (err<0) 
							  {
								  UPDATE_RESULT(result, RET_FAILURE);
								  TRACE(HAL_ERROR,"Extdev(0x%x) CamSys_ClkIn failed\n",pHalCtx->camConfig[j].dev_id);
							  }
						  }
					  }
					  usleep(pHalCtx->clkin_delay);
					  break;
				  }
				  default:
					  break;
			  }
			  
		  } 
	 }		 
 }
 TRACE( HAL_INFO, "HalSensorPwrSeq out ,result = %d\n",result);

 return result;
	
}


RESULT HalVcmPwrSeq(HalHandle_t HalHandle, uint32_t dev_mask, bool_t activate)
{
    RESULT result = RET_SUCCESS;
    int32_t i, powerup_type,err;
	camsys_sysctrl_t sysctl;
	HalContext_t *pHalCtx = (HalContext_t *)HalHandle;

 // process external devices only
 //TODO: need a mutex around the following block:
 {
	 // build external devices & high-active external devices masks
	 // duplicate & modify this 'if'-block for further external devices
	 if (dev_mask & HAL_DEVID_EXTERNAL)
	 {
		 for (i=0; i<VCM_PWRSEQ_CNT; i++) {
		 
			  if (activate)
				  powerup_type = POWERSEQ_GET(pHalCtx->vcmpowerupseq,(VCM_PWRSEQ_CNT-1-i));
			  else
				  powerup_type = POWERSEQ_GET(pHalCtx->vcmpowerupseq,i);

			  switch (powerup_type)
			  {
				  case VCM_PWRSEQ_VDD:
				  {
					  sysctl.dev_mask = (dev_mask & HAL_DEVID_EXTERNAL);
					  sysctl.ops = CamSys_Afvdd;
					  sysctl.on = (activate)?1:0;
					  err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_SYSCTRL, &sysctl);
					  if (err<0) {
						  TRACE(HAL_ERROR,"CamSys_Afvdd on failed!\n");
					  }
					  usleep(pHalCtx->vcmvdd_delay);
					  break;
				  }
				  case VCM_PWRSEQ_PWR:
				  {
					  sysctl.dev_mask = (dev_mask & HAL_DEVID_EXTERNAL);
					  sysctl.ops = CamSys_AfPwr;
					  sysctl.on = (activate)?1:0;
					  err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_SYSCTRL, &sysctl);
					  if (err<0) {
						  TRACE(HAL_ERROR,"CamSys_AfPwr on failed!\n");
					  }
					  usleep(pHalCtx->vcmpwr_delay);
					  break;
				  }
				  case VCM_PWRSEQ_PWRDN:
				  {
					  sysctl.dev_mask = (dev_mask & HAL_DEVID_EXTERNAL);
					  sysctl.ops = CamSys_AfPwrDn;
					  sysctl.on = (activate)?0:1;
					  err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_SYSCTRL, &sysctl);
					  if (err<0) {
						  TRACE(HAL_ERROR,"CamSys_AfPwr on failed!\n");
					  }
					  usleep(pHalCtx->vcmpwrdn_delay);
					  break;
				  }

				  default:
					  break;
			  }
			  
		  } 
	 }		 
 }
 TRACE( HAL_INFO, "HalSensorPwrSeq out ,result = %d\n",result);

 return result;
	
}

/******************************************************************************
 * HalSetPower()
 *****************************************************************************/
RESULT HalSetPower( HalHandle_t HalHandle, uint32_t dev_mask, bool_t activate )
{
    RESULT result = RET_SUCCESS;
    camsys_sysctrl_t sysctl;
    int32_t err,i;
    
    if (HalHandle == NULL)
    {
        TRACE( HAL_ERROR, "%s(%d): HalHandle is NULL\n",__FUNCTION__,__LINE__);
        return RET_NULL_POINTER;
    }

    // check for valid devices
    if (dev_mask & ~HAL_DEVID_ALL)
    {
        TRACE( HAL_ERROR, "%s(%d): dev_mask is invalidate\n",__FUNCTION__,__LINE__,dev_mask);
        return RET_INVALID_PARM;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        TRACE( HAL_ERROR, "%s(%d): refCount is invaldate(0)\n",__FUNCTION__,__LINE__);
        return RET_WRONG_STATE;
    }

    if (pHalCtx->drvInfo.camsys_fd<=0)
    {
        TRACE( HAL_ERROR, "pHalCtx is error for haven't camsys device!\n" );
        return RET_WRONG_HANDLE;
    }    

    if (dev_mask & HAL_DEVID_INTERNAL) {
        sysctl.dev_mask = (dev_mask & HAL_DEVID_INTERNAL);
        sysctl.ops = CamSys_ClkIn;
        if (pHalCtx->drvInfo.mipi_lanes>0 && pHalCtx->drvInfo.mipi_lanes<=4)
            sysctl.on = (activate)?pHalCtx->drvInfo.mipi_lanes:0;
        else
            sysctl.on = (activate)?1:0;

        err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_SYSCTRL, &sysctl);
        if (err<0)
        {
            UPDATE_RESULT(result, RET_FAILURE);
            TRACE(HAL_ERROR,"Intdev(0x%x) CamSys_ClkIn failed\n",sysctl.dev_mask);
        }
    }
    
#if 0
    // process external devices only
    //TODO: need a mutex around the following block:
    {
        // build external devices & high-active external devices masks
        // duplicate & modify this 'if'-block for further external devices
        if (dev_mask & HAL_DEVID_EXTERNAL)
        {
            sysctl.dev_mask = (dev_mask & HAL_DEVID_EXTERNAL);
            sysctl.on = (activate)?1:0;

            for (sysctl.ops=CamSys_Avdd; sysctl.ops<=CamSys_Afvdd; sysctl.ops++)
            {
                err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_SYSCTRL, &sysctl);
                if (err<0)
                {
                    UPDATE_RESULT(result, RET_FAILURE);
                    TRACE(HAL_ERROR,"Extdev(0x%x) CamSys_Pwr(%d) failed\n",sysctl.dev_mask,sysctl.ops);
                }
            }


            for (i=0; i<NUM_EXTDEV; i++)
            {                
                if (pHalCtx->camConfig[i].dev_id & dev_mask)
                {
                    sysctl.dev_mask = pHalCtx->camConfig[i].dev_id;
                    sysctl.on = (activate)?pHalCtx->camConfig[i].clkin_frequence:0;
					
                    sysctl.ops = CamSys_ClkIn;

                    err = ioctl(pHalCtx->drvInfo.camsys_fd,CAMSYS_SYSCTRL, &sysctl);
                    if (err<0) 
                    {
                        UPDATE_RESULT(result, RET_FAILURE);
                        TRACE(HAL_ERROR,"Extdev(0x%x) CamSys_ClkIn failed\n",pHalCtx->camConfig[i].dev_id);
                    }
                }
            }

            //1. set power en
            usleep(10);
            sysctl.dev_mask = (dev_mask & HAL_DEVID_EXTERNAL);
    	    sysctl.ops = CamSys_PwrEn;
    	    sysctl.on = (activate)?1:0;
    	    err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    	    if (err<0) {
    	        TRACE(HAL_ERROR,"Extdev(0x%x) CamSys_PwrDn failed\n",sysctl.dev_mask);
    	    } 
            //2. set power down
            usleep(10);
            sysctl.dev_mask = (dev_mask & HAL_DEVID_EXTERNAL);
    	    sysctl.ops = CamSys_PwrDn;
    	    sysctl.on = (activate)?0:1;
    	    err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    	    if (err<0) {
    	        TRACE(HAL_ERROR,"Extdev(0x%x) CamSys_PwrDn failed\n",sysctl.dev_mask);
    	    } 
            //3. set reset power
            usleep(10);
            sysctl.dev_mask = (dev_mask & HAL_DEVID_EXTERNAL);
    	    sysctl.ops = CamSys_Rst;
    	    sysctl.on = (activate)?0:1;
    	    err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    	    if (err<0) {
    	        TRACE(HAL_ERROR,"Extdev(0x%x) CamSys_Rst failed\n",sysctl.dev_mask);
    	    } 

    	    //4. set VCM power
            usleep(1000);
            sysctl.dev_mask = (dev_mask & HAL_DEVID_EXTERNAL);
    	    sysctl.ops = CamSys_AfPwr;
    	    sysctl.on = (activate)?1:0;
    	    err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    	    if (err<0) {
    	        TRACE(HAL_ERROR,"Extdev(0x%x) CamSys_AfPwr failed\n",sysctl.dev_mask);
    	    } 

    	    sysctl.dev_mask = (dev_mask & HAL_DEVID_EXTERNAL);
    	    sysctl.ops = CamSys_AfPwrDn;
    	    sysctl.on = (activate)?0:1;
    	    err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    	    if (err<0) {
    	        TRACE(HAL_ERROR,"Extdev(0x%x) CamSys_AfPwrDn failed\n",sysctl.dev_mask);
    	    }
        }       
    }
#else

	HalSensorPwrSeq(HalHandle, dev_mask, activate);
	HalVcmPwrSeq(HalHandle, dev_mask, activate);
#endif	
    TRACE( HAL_INFO, "CamSys power on out ,result = %d\n",result);

    return result;
}


/******************************************************************************
 * HalSetClock()
 *****************************************************************************/
RESULT HalSetClock( HalHandle_t HalHandle, uint32_t dev_mask, uint32_t frequency )
{
    RESULT result = RET_SUCCESS;
    int32_t i;
    
    if (HalHandle == NULL)
    {
        TRACE( HAL_ERROR, "%s(%d): HalHandle is NULL\n",__FUNCTION__,__LINE__);
        return RET_NULL_POINTER;
    }

    // check for valid devices
    if (dev_mask & ~(HAL_DEVID_EXTERNAL))
    {
        TRACE( HAL_ERROR, "%s(%d): dev_mask is invalidate\n",__FUNCTION__,__LINE__,dev_mask);
        return RET_INVALID_PARM;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        TRACE( HAL_ERROR, "%s(%d): refCount is invaldate(0)\n",__FUNCTION__,__LINE__);
        return RET_WRONG_STATE;
    }

    if (pHalCtx->drvInfo.camsys_fd<=0)
    {
        TRACE( HAL_ERROR, "pHalCtx is error for haven't camsys device!\n" );
        return RET_WRONG_HANDLE;
    } 

    // set frequencies for all devices individually
    if (dev_mask & HAL_DEVID_EXTERNAL)
    {
        for (i=0; i<NUM_EXTDEV; i++)
        {
            if (pHalCtx->camConfig[i].dev_id & dev_mask)
            {
                pHalCtx->camConfig[i].clkin_frequence = frequency;
            }
        }

    }

    return result;
}


/******************************************************************************
 * HalAllocMemory()
 *****************************************************************************/
uint32_t HalAllocMemory( HalHandle_t HalHandle, uint32_t byte_size )
{
    if ( HalHandle == NULL)
    {
        TRACE( HAL_ERROR, "%s(%d): HalHandle is NULL",__FUNCTION__,__LINE__ );
        return RET_NULL_POINTER;
    }


    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        TRACE( HAL_ERROR, "%s(%d): refCount is invaldate(0)\n",__FUNCTION__,__LINE__);
        return RET_WRONG_STATE;
    }
    
    if ((pHalCtx->memMng.ion_device==NULL) && (pHalCtx->ops == NULL))
    {
        TRACE( HAL_ERROR, "havn't memory alloc device!\n" );
        return RET_WRONG_HANDLE;
    }
    //alloc a new block
    struct MemBlockInfo_s* block = (struct MemBlockInfo_s*)malloc(sizeof(struct MemBlockInfo_s));
    if(!block){
        TRACE( HAL_ERROR, "Can't malloc mem\n" );
        goto failed_alloc;
    }
    
    memset(block,0,sizeof(struct MemBlockInfo_s));
    ListInit(&(block->node));

	if (pHalCtx->ops) {
		block->new_data = pHalCtx->ops->alloc(pHalCtx->memMng.cam_mem_handle,byte_size);
		if (!block->new_data) {
	        TRACE( HAL_ERROR, "Can't malloc size(0x%x) from mem ops\n",byte_size );
	        goto failed_alloc;
		}
		//iommu
		if (pHalCtx->ops->iommu_map) {
			if (pHalCtx->ops->iommu_map(pHalCtx->memMng.cam_mem_handle,block->new_data)) {
				TRACE( HAL_ERROR, "mem ops iommumap failed\n",byte_size );
				pHalCtx->ops->free(pHalCtx->memMng.cam_mem_handle,block->new_data);
				goto failed_alloc;
			}

		}
		//add to tail of list
		ListAddTail(&pHalCtx->memMng.halMemList, (void*)block);
		
		TRACE( HAL_INFO, "malloc 0x%x bytes,iommu mapped %d,mmu_addr 0x%x,vir_addr 0x%x,phy_addr 0x%x,block(%p)\n",
			byte_size,
			block->new_data->iommu_maped,block->new_data->mmu_addr,
			block->new_data->vir_addr,block->new_data->phy_addr,
			block);
		if (block->new_data->iommu_maped)
			return block->new_data->mmu_addr;
		else
			return block->new_data->phy_addr;
	} else {
	    //alloc from ion
	    if(pHalCtx->memMng.ion_device->alloc(pHalCtx->memMng.ion_device, byte_size, &block->data) !=0)
	    {
	        TRACE( HAL_ERROR, "Can't malloc size(0x%x) from ion\n",byte_size );
	        goto failed_alloc;
	    }
		//add to tail of list
		ListAddTail(&pHalCtx->memMng.halMemList, (void*)block);
		
		TRACE( HAL_INFO, " malloc size(0x%x@0x%x),block(%p) from ion successful\n",byte_size,block->data.phy_addr, block);
		return block->data.phy_addr;
	}

failed_alloc:
	if (block)
	    free(block);
    return RET_NULL_POINTER;

}


static int searchListFun(List *node, void *key){
    ulong_t mem_phy_addr = (ulong_t)key;
    struct MemBlockInfo_s* blockInfo_p = (struct MemBlockInfo_s*)node;
    
    if((mem_phy_addr - blockInfo_p->data.phy_addr) < blockInfo_p->data.size){
   //     TRACE(HAL_INFO,"block mem phy addr = 0x%x,find addr = 0x%x",  blockInfo_p->data->phys,mem_phy_addr);
        return 1;
    }else{
        return 0;
    }
}

static int searchListFun_for_newdata(List *node, void *key){
    ulong_t mem_phy_addr = (ulong_t)key;
    struct MemBlockInfo_s* blockInfo_p = (struct MemBlockInfo_s*)node;
	
    if (blockInfo_p->new_data->iommu_maped) {
		if((mem_phy_addr - blockInfo_p->new_data->mmu_addr) < blockInfo_p->new_data->size){
			//     TRACE(HAL_INFO,"block mem phy addr = 0x%x,find addr = 0x%x",  blockInfo_p->data->phys,mem_phy_addr);
			return 1;
		}else{
			return 0;
		}
    } else {

		 if((mem_phy_addr - blockInfo_p->new_data->phy_addr) < blockInfo_p->new_data->size){
		//	   TRACE(HAL_INFO,"block mem phy addr = 0x%x,find addr = 0x%x",  blockInfo_p->data->phys,mem_phy_addr);
			 return 1;
		 }else{
			 return 0;
		 }
	}
}
/******************************************************************************
 * HalFreeMemory()
 *****************************************************************************/
RESULT HalFreeMemory( HalHandle_t HalHandle, ulong_t mem_address )
{

    if ( HalHandle == NULL )
    {
        TRACE( HAL_ERROR, "%s(%d): HalHandleis NULL",__FUNCTION__,__LINE__ );
        return RET_NULL_POINTER;
    }
    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        TRACE( HAL_ERROR, "%s(%d): refCount is invaldate(0)\n",__FUNCTION__,__LINE__);
        return RET_WRONG_STATE;
    }
    
    if ((pHalCtx->memMng.ion_device==NULL) && (pHalCtx->ops == NULL))
    {
        TRACE( HAL_ERROR, "havn't memory alloc device!\n" );
        return RET_WRONG_HANDLE;
    }
    
    List* p_item = NULL;

	if (pHalCtx->ops == NULL) {
	    //remove the item
	    p_item = ListSearch(&pHalCtx->memMng.halMemList, &searchListFun, (void *)mem_address);
	    if(!p_item){
	        TRACE( HAL_ERROR, "line:%d,have not find the ion block,phy addr = 0x%x\n",__LINE__,mem_address );
	        return RET_SUCCESS;
	    }else{
	        struct MemBlockInfo_s* p_block = (struct MemBlockInfo_s*)p_item;
	        if(pHalCtx->memMng.ion_device->free(pHalCtx->memMng.ion_device, &(p_block->data)) != 0){
	            TRACE( HAL_ERROR, "failed free ion block,phy_addr = 0x%x\n",mem_address );
	        }else{
	             //something wrong with ListRemoveItem,zyc
	            //ListRemoveItem(&pHalCtx->memMng.halMemList, &searchListFun, (void *)mem_address);
	            free(p_block);
	            TRACE( HAL_NOTICE1, " free ion buffer(phy=0x%x) successful\n",mem_address );
	        }
	    }
	}else {
	    //remove the item
	    p_item = ListSearch(&pHalCtx->memMng.halMemList, &searchListFun_for_newdata, (void *)mem_address);
	    if(!p_item){
	        TRACE( HAL_ERROR, "line:%d,have not find the  block,phy addr = 0x%x\n",__LINE__,mem_address );
	        return RET_SUCCESS;
	    }else{
	        struct MemBlockInfo_s* p_block = (struct MemBlockInfo_s*)p_item;
			if (p_block->new_data->iommu_maped) {
				pHalCtx->ops->iommu_unmap(pHalCtx->memMng.cam_mem_handle,p_block->new_data);
			}
			//should remove before free
			ListRemoveItem(&pHalCtx->memMng.halMemList, &searchListFun_for_newdata, (void *)mem_address);			
	        if( pHalCtx->ops->free(pHalCtx->memMng.cam_mem_handle,p_block->new_data) != 0){
	            TRACE( HAL_ERROR, "failed free memops block,phy_addr = 0x%x\n",mem_address );
				//free failed, add again
				ListAddTail(&pHalCtx->memMng.halMemList, (void*)p_block);
	        }else{
	             //something wrong with ListRemoveItem,zyc
	            //ListRemoveItem(&pHalCtx->memMng.halMemList, &searchListFun, (void *)mem_address);
	            free(p_block);
	            TRACE( HAL_ERROR, " free memops buffer(phy=0x%x) successful\n",mem_address );
	        }
	    }

	}


    return RET_SUCCESS;
}


/******************************************************************************
 * HalReadMemory()
 *****************************************************************************/
RESULT HalReadMemory( HalHandle_t HalHandle, ulong_t mem_address, uint8_t* p_read_buffer, uint32_t byte_size )
{
    if (HalHandle == NULL)
    {
        TRACE( HAL_ERROR, "%s(%d): HalHandle or p_read_buffer is NULL",__FUNCTION__,__LINE__ );
        return RET_NULL_POINTER;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        TRACE( HAL_ERROR, "%s(%d): refCount is invaldate(0)\n",__FUNCTION__,__LINE__);
        return RET_WRONG_STATE;
    }


    //find the item
    List* p_item = NULL;
	
	if (pHalCtx->ops != NULL) {
	    p_item = ListSearch(&pHalCtx->memMng.halMemList, &searchListFun_for_newdata, (void *)mem_address);
	    if(!p_item){
	        TRACE( HAL_ERROR, "line:%d,have not find the memops block,phy addr = 0x%x\n",__LINE__,mem_address );
	        return -1;
	    }else{
	        struct MemBlockInfo_s* p_block = (struct MemBlockInfo_s*)p_item;
	        //compute virt addr
	        if (p_block->new_data->iommu_maped)
	        	memcpy( p_read_buffer, (void*)((ulong_t)p_block->new_data->vir_addr+((ulong_t)mem_address - p_block->new_data->mmu_addr)), byte_size );
			else
	        	memcpy( p_read_buffer, (void*)((ulong_t)p_block->new_data->vir_addr+((ulong_t)mem_address - p_block->new_data->phy_addr)), byte_size );
	    }
	} else {
			 p_item = ListSearch(&pHalCtx->memMng.halMemList, &searchListFun, (void *)mem_address);
			 if(!p_item){
				 TRACE( HAL_ERROR, "line:%d,have not find the ion block,phy addr = 0x%x\n",__LINE__,mem_address );
				 return -1;
			 }else{
				 struct MemBlockInfo_s* p_block = (struct MemBlockInfo_s*)p_item;
				 //compute virt addr
				 memcpy( p_read_buffer, (void*)((ulong_t)p_block->data.vir_addr+((ulong_t)mem_address - p_block->data.phy_addr)), byte_size );
			 }
	}


  //  memcpy( p_read_buffer, (void*)mem_address, byte_size );

    return RET_SUCCESS;
}


/******************************************************************************
 * HalWriteMemory()
 *****************************************************************************/
RESULT HalWriteMemory( HalHandle_t HalHandle, ulong_t mem_address, uint8_t* p_write_buffer, uint32_t byte_size )
{
    if (HalHandle == NULL)
    {
        TRACE( HAL_ERROR, "%s(%d): HalHandle or p_write_buffer is NULL",__FUNCTION__,__LINE__ );
        return RET_NULL_POINTER;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        TRACE( HAL_ERROR, "%s(%d): refCount is invaldate(0)\n",__FUNCTION__,__LINE__);
        return RET_WRONG_STATE;
    }

    //find the item
    List* p_item = NULL;
	
	if (pHalCtx->ops == NULL) {
	    p_item = ListSearch(&pHalCtx->memMng.halMemList, &searchListFun, (void *)mem_address);
	    if(!p_item){
	        TRACE( HAL_ERROR, "line:%d,have not find the ion block,phy addr = 0x%x\n",__LINE__,mem_address );
	        return -1;
	    }else{
	        struct MemBlockInfo_s* p_block = (struct MemBlockInfo_s*)p_item;
	        memcpy((void*)((ulong_t)p_block->data.vir_addr+ ((ulong_t)mem_address - p_block->data.phy_addr)),p_write_buffer, byte_size );
	    }
	} else {

		p_item = ListSearch(&pHalCtx->memMng.halMemList, &searchListFun_for_newdata, (void *)mem_address);
		if(!p_item){
			TRACE( HAL_ERROR, "line:%d,have not find the memops block,phy addr = 0x%x\n",__LINE__,mem_address );
			return -1;
		}else{
			struct MemBlockInfo_s* p_block = (struct MemBlockInfo_s*)p_item;
			
	        if (p_block->new_data->iommu_maped)
				memcpy((void*)((ulong_t)p_block->new_data->vir_addr+ ((ulong_t)mem_address - p_block->new_data->mmu_addr)),p_write_buffer, byte_size );
			else
				memcpy((void*)((ulong_t)p_block->new_data->vir_addr+ ((ulong_t)mem_address - p_block->new_data->phy_addr)),p_write_buffer, byte_size );
		}
	}
//    memcpy( (void*)mem_address, p_write_buffer, byte_size );

    return RET_SUCCESS;
}

//type: 0: 
RESULT HalGetMemoryMapFd( HalHandle_t HalHandle, ulong_t mem_address,int *fd )
{
    if ( HalHandle == NULL )
    {
        TRACE( HAL_ERROR, "%s(%d): HalHandle is NULL",__FUNCTION__,__LINE__ );
        return RET_NULL_POINTER;
    }
    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        TRACE( HAL_ERROR, "%s(%d): refCount is invaldate(0)\n",__FUNCTION__,__LINE__);
        return RET_WRONG_STATE;
    }
    //find the item
    List* p_item = NULL;
	
	if (pHalCtx->ops == NULL) {
	    p_item = ListSearch(&pHalCtx->memMng.halMemList, &searchListFun, (void *)mem_address);
	    if(!p_item){
	        TRACE( HAL_ERROR, "line:%d,have not find the ion block,phy addr = 0x%x\n",__LINE__,mem_address );
	        return -1;
	    }else{
	        struct MemBlockInfo_s* p_block = (struct MemBlockInfo_s*)p_item;
	        if(pHalCtx->memMng.ion_device->iommu_enabled){
	            *fd  =  p_block->data.map_fd;
	        }else{
	            *fd  = mem_address;
	        }
	        return RET_SUCCESS;
	    }
	} else {
	    p_item = ListSearch(&pHalCtx->memMng.halMemList, &searchListFun_for_newdata, (void *)mem_address);
	    if(!p_item){
	        TRACE( HAL_ERROR, "line:%d,have not find the memops block,phy addr = 0x%x\n",__LINE__,mem_address );
	        return -1;
	    }else{
	        struct MemBlockInfo_s* p_block = (struct MemBlockInfo_s*)p_item;
	        if(p_block->new_data->iommu_maped){
	            *fd  =  p_block->new_data->fd;
	        }else{
	            *fd  = mem_address;
	        }
	        return RET_SUCCESS;
	    }
	}
}

/******************************************************************************
 * HalMapMemory()
 *****************************************************************************/
RESULT HalMapMemory( HalHandle_t HalHandle, ulong_t mem_address, uint32_t byte_size, HalMapMemType_t mapping_type, void **pp_mapped_buf )
{
    RESULT result = RET_SUCCESS;
    HalMemMap_t *pMapData;
    void *p_mapped_buffer;

    if ( HalHandle == NULL )
    {
        TRACE( HAL_ERROR, "%s(%d): HalHandle is NULL",__FUNCTION__,__LINE__ );
        return RET_NULL_POINTER;
    }

    

    if (byte_size == 0)
    {
        return RET_OUTOFRANGE;
    }

    switch (mapping_type)
    {
        case HAL_MAPMEM_READWRITE:
        case HAL_MAPMEM_READONLY:
        case HAL_MAPMEM_WRITEONLY:
            break;
        default:
            return RET_INVALID_PARM;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        TRACE( HAL_ERROR, "%s(%d): refCount is invaldate(0)\n",__FUNCTION__,__LINE__);
        return RET_WRONG_STATE;
    }

#if 0
    // allocate mapping descriptor & local buffer
    pMapData = malloc( byte_size + sizeof(HalMemMap_t) );
    if (pMapData == NULL)
    {
        return RET_OUTOFMEM;
    }

    // calc local buffer address from mapping descriptor
    p_mapped_buffer = (void*)(pMapData+1);

    // store buffer data in descriptor
    pMapData->mem_address  = mem_address;
    pMapData->byte_size    = byte_size;
    pMapData->mapping_type = mapping_type;

    // 'map' buffer by reading it into local buffer?
    if (mapping_type != HAL_MAPMEM_WRITEONLY)
    {
        result = HalReadMemory( HalHandle, pMapData->mem_address, p_mapped_buffer, pMapData->byte_size );
        if (result != RET_SUCCESS)
        {
            // release mapping descriptor
            free(pMapData);
            return result;
        }
    }

    // return mapped buffer
    *pp_mapped_buf = p_mapped_buffer;
#else
    //find the item
    List* p_item = NULL;
	if (pHalCtx->ops == NULL) {

	    p_item = ListSearch(&pHalCtx->memMng.halMemList, &searchListFun, (void *)mem_address);
	    if(!p_item){
	        TRACE( HAL_ERROR, "line:%d,have not find the ion block,phy addr = 0x%x\n",__LINE__,mem_address );
	        return -1;
	    }else{
	        struct MemBlockInfo_s* p_block = (struct MemBlockInfo_s*)p_item;
	        //compute virt addr
	        p_mapped_buffer =  (void*)((ulong_t)p_block->data.vir_addr+((ulong_t)mem_address - p_block->data.phy_addr));
	    }
	    *pp_mapped_buf = p_mapped_buffer;
	} else {
	    p_item = ListSearch(&pHalCtx->memMng.halMemList, &searchListFun_for_newdata, (void *)mem_address);
	    if(!p_item){
	        TRACE( HAL_ERROR, "line:%d,have not find the memops block,phy addr = 0x%x\n",__LINE__,mem_address );
	        return -1;
	    }else{
	        struct MemBlockInfo_s* p_block = (struct MemBlockInfo_s*)p_item;
	        //compute virt addr
	        if (p_block->new_data->iommu_maped) {
				p_mapped_buffer =  (void*)((ulong_t)p_block->new_data->vir_addr+((ulong_t)mem_address - p_block->new_data->mmu_addr));
			} else
	        	p_mapped_buffer =  (void*)((ulong_t)p_block->new_data->vir_addr+((ulong_t)mem_address - p_block->new_data->phy_addr));
	    }
	    *pp_mapped_buf = p_mapped_buffer;
	}

#endif
    return result;
}


/******************************************************************************
 * HalUnMapMemory()
 *****************************************************************************/
RESULT HalUnMapMemory( HalHandle_t HalHandle, void* p_mapped_buf )
{
    RESULT result = RET_SUCCESS;
    HalMemMap_t *pMapData;

    if ( (HalHandle == NULL) || (p_mapped_buf == NULL) )
    {
        TRACE( HAL_ERROR, "%s(%d): HalHandle or p_mapped_buf is NULL",__FUNCTION__,__LINE__ );
        return RET_NULL_POINTER;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        TRACE( HAL_ERROR, "%s(%d): refCount is invaldate(0)\n",__FUNCTION__,__LINE__);
        return RET_WRONG_STATE;
    }
#if 0
    // get mapping descriptor from mapped address
    pMapData = ((HalMemMap_t*)(p_mapped_buf))-1;

    // 'unmap' buffer by writing it back into local buffer?
    if (pMapData->mapping_type != HAL_MAPMEM_READONLY)
    {
        result = HalWriteMemory( HalHandle, pMapData->mem_address, p_mapped_buf, pMapData->byte_size );
    }

    // release mapping descriptor & local buffer
    free(pMapData);
#endif
    return result;
}


/******************************************************************************
 * HalReadI2CMem()
 *****************************************************************************/
RESULT HalReadI2CMem
(
    HalHandle_t HalHandle,
    uint8_t     bus_num,
    uint16_t    slave_addr,
    uint32_t    reg_address,
    uint8_t     reg_addr_size,
    uint8_t     *p_read_buffer,
    uint32_t    byte_size
)
{
    RESULT result = RET_SUCCESS;
    OSLAYER_STATUS osStatus;

    if ( (HalHandle == NULL) || (p_read_buffer == NULL) )
    {
        TRACE( HAL_ERROR, "%s(%d): HalHandle or p_write_buffer is NULL",__FUNCTION__,__LINE__ );
        return RET_NULL_POINTER;
    }

    if ( (bus_num > NUM_I2C) || (reg_addr_size > 4) )
    {
        TRACE( HAL_ERROR, "%s(%d): bus_num(%d) or reg_addr_size(%d) is invalidate",__FUNCTION__,__LINE__,
            bus_num, reg_addr_size);
        return RET_INVALID_PARM;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        TRACE( HAL_ERROR, "%s(%d): refCount is invaldate(0)\n",__FUNCTION__,__LINE__);
        return RET_WRONG_STATE;
    }

    if (pHalCtx->drvInfo.camsys_fd<=0)
    {
        TRACE( HAL_ERROR, "%s(%d): pHalCtx is error for haven't camsys device!\n",__FUNCTION__,__LINE__);
        return RET_WRONG_HANDLE;
    }

    // lock controller
    osStatus = osMutexLock( &pHalCtx->iicMutex[bus_num] );
    if (OSLAYER_OK != osStatus)
    {
        TRACE( HAL_ERROR, "Can't lock I2C bus #%d\n", bus_num );
        return RET_FAILURE;
    }

    // access controller
    {
        camsys_i2c_info_t i2cinfo;
        int32_t err=0;
        uint32_t i;
        
        i2cinfo.bus_num = bus_num;
        i2cinfo.slave_addr = slave_addr;
        i2cinfo.reg_addr = reg_address;
        i2cinfo.reg_size = reg_addr_size;
        i2cinfo.speed   = 100000;
        if (byte_size > 4)
        {
            i2cinfo.val_size = 4;
            TRACE( HAL_ERROR, "I2c bus isn't support read %d bytes\n", byte_size );
        } else { 
            i2cinfo.val_size = byte_size;
        }
        err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_I2CRD, &i2cinfo);
        if (err<0)
        {
            TRACE( HAL_ERROR, "I2c bus #%d read failed\n", bus_num );
            UPDATE_RESULT( result, RET_FAILURE );
        } else {
            
            for (i=0; i<i2cinfo.val_size; i++) 
            {
                *p_read_buffer++ = (i2cinfo.val>>((i2cinfo.val_size-1-i)*8))&0xff;
            }
        }

    }

    // unlock controller again
    osStatus = osMutexUnlock( &pHalCtx->iicMutex[bus_num] );
    if (OSLAYER_OK != osStatus)
    {
        TRACE( HAL_ERROR, "Can't unlock I2C bus #%d\n", bus_num );
        UPDATE_RESULT( result, RET_FAILURE );
    }

    return result;
}


/******************************************************************************
 * HalWriteI2CMem()
 *****************************************************************************/
RESULT HalWriteI2CMem
(
    HalHandle_t HalHandle,
    uint8_t     bus_num,
    uint16_t    slave_addr,
    uint32_t    reg_address,
    uint8_t     reg_addr_size,
    uint8_t     *p_write_buffer,
    uint32_t    byte_size
)
{
    RESULT result = RET_SUCCESS;
    OSLAYER_STATUS osStatus;

    if ( (HalHandle == NULL) || (p_write_buffer == NULL) )
    {
        TRACE( HAL_ERROR, "%s(%d): HalHandle or p_write_buffer is NULL",__FUNCTION__,__LINE__ );
        return RET_NULL_POINTER;
    }

    if ( (bus_num > NUM_I2C) || (reg_addr_size > 4) )
    {
        TRACE( HAL_ERROR ,"%s(%d): bus_num(%d) or reg_addr_size(%d) is invalidate",__FUNCTION__,__LINE__,
            bus_num, reg_addr_size);
        return RET_INVALID_PARM;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        TRACE( HAL_ERROR, "%s(%d): refCount is invaldate(0)\n",__FUNCTION__,__LINE__);
        return RET_WRONG_STATE;
    }

    if (pHalCtx->drvInfo.camsys_fd<=0)
    {
        TRACE( HAL_ERROR, "%s(%d): pHalCtx is error for haven't camsys device!\n",__FUNCTION__,__LINE__);
        return RET_WRONG_HANDLE;
    }

    
    // lock controller
    osStatus = osMutexLock( &pHalCtx->iicMutex[bus_num] );
    if (OSLAYER_OK != osStatus)
    {
        TRACE( HAL_ERROR, "Can't lock I2C bus #%d\n", bus_num );
        return RET_FAILURE;
    }

    // access controller
    {
        camsys_i2c_info_t i2cinfo;
        int32_t err=0;
        uint32_t *i2cbase,i;
        
        i2cinfo.bus_num = bus_num;
        i2cinfo.slave_addr = slave_addr;
        i2cinfo.reg_addr = reg_address;
        i2cinfo.reg_size = reg_addr_size;
        i2cinfo.speed   = 300000;
        if (byte_size > 4)
        {
            i2cinfo.i2cbuf_directly = 1;
            i2cinfo.i2cbuf_bytes = byte_size+reg_addr_size;   
            TRACE(HAL_ERROR, "Can't support >4 bytes write by i2c!!");
            
        } else { 
            i2cinfo.val = 0;
            i2cinfo.val_size = byte_size;
            i2cinfo.i2cbuf_directly = 0;
            i2cinfo.i2cbuf_bytes = 0;
            for (i=0; i<byte_size; i++)
            {
                i2cinfo.val |= *p_write_buffer++;
                if (i+1<byte_size)
                    i2cinfo.val <<= 8;
            }
        }
        err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_I2CWR, &i2cinfo);
        if (err<0)
        {
            TRACE( HAL_ERROR, "I2c bus #%d write failed\n", bus_num );
            UPDATE_RESULT( result, RET_FAILURE );
        }
    }

    // unlock controller again
    osStatus = osMutexUnlock( &pHalCtx->iicMutex[bus_num] );
    if (OSLAYER_OK != osStatus)
    {
        TRACE( HAL_ERROR, "Can't unlock I2C bus #%d\n", bus_num );
        UPDATE_RESULT( result, RET_FAILURE );
    }

    return result;
}

//zyh@rock-chips.com: v0.0x20.0
/******************************************************************************
 * HalWriteI2CMem_Rate() : add rate set
 *****************************************************************************/
RESULT HalWriteI2CMem_Rate
(
    HalHandle_t HalHandle,
    uint8_t     bus_num,
    uint16_t    slave_addr,
    uint32_t    reg_address,
    uint8_t     reg_addr_size,
    uint8_t     *p_write_buffer,
    uint32_t    byte_size,
    uint32_t    rate
)
{
    RESULT result = RET_SUCCESS;
    OSLAYER_STATUS osStatus;

    if ( (HalHandle == NULL) || (p_write_buffer == NULL) )
    {
        TRACE( HAL_ERROR, "%s(%d): HalHandle or p_write_buffer is NULL",__FUNCTION__,__LINE__ );
        return RET_NULL_POINTER;
    }

    if ( (bus_num > NUM_I2C) || (reg_addr_size > 4) )
    {
        TRACE( HAL_ERROR ,"%s(%d): bus_num(%d) or reg_addr_size(%d) is invalidate",__FUNCTION__,__LINE__,
            bus_num, reg_addr_size);
        return RET_INVALID_PARM;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        TRACE( HAL_ERROR, "%s(%d): refCount is invaldate(0)\n",__FUNCTION__,__LINE__);
        return RET_WRONG_STATE;
    }

    if (pHalCtx->drvInfo.camsys_fd<=0)
    {
        TRACE( HAL_ERROR, "%s(%d): pHalCtx is error for haven't camsys device!\n",__FUNCTION__,__LINE__);
        return RET_WRONG_HANDLE;
    }

    
    // lock controller
    osStatus = osMutexLock( &pHalCtx->iicMutex[bus_num] );
    if (OSLAYER_OK != osStatus)
    {
        TRACE( HAL_ERROR, "Can't lock I2C bus #%d\n", bus_num );
        return RET_FAILURE;
    }

    // access controller
    {
        camsys_i2c_info_t i2cinfo;
        int32_t err=0;
        uint32_t *i2cbase,i;
        
        i2cinfo.bus_num = bus_num;
        i2cinfo.slave_addr = slave_addr;
        i2cinfo.reg_addr = reg_address;
        i2cinfo.reg_size = reg_addr_size;
        i2cinfo.speed   = rate*1000;
        if (byte_size > 4)
        {
            i2cinfo.i2cbuf_directly = 1;
            i2cinfo.i2cbuf_bytes = byte_size+reg_addr_size;   
            TRACE(HAL_ERROR, "Can't support >4 bytes write by i2c!!");
            
        } else { 
            i2cinfo.val = 0;
            i2cinfo.val_size = byte_size;
            i2cinfo.i2cbuf_directly = 0;
            i2cinfo.i2cbuf_bytes = 0;
            for (i=0; i<byte_size; i++)
            {
                i2cinfo.val |= *p_write_buffer++;
                if (i+1<byte_size)
                    i2cinfo.val <<= 8;
            }
        }
        err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_I2CWR, &i2cinfo);
        if (err<0)
        {
            TRACE( HAL_ERROR, "I2c bus #%d write failed\n", bus_num );
            UPDATE_RESULT( result, RET_FAILURE );
        }
    }

    // unlock controller again
    osStatus = osMutexUnlock( &pHalCtx->iicMutex[bus_num] );
    if (OSLAYER_OK != osStatus)
    {
        TRACE( HAL_ERROR, "Can't unlock I2C bus #%d\n", bus_num );
        UPDATE_RESULT( result, RET_FAILURE );
    }

    return result;
}


/******************************************************************************
 * HalConnectIrq()
 *****************************************************************************/
RESULT HalConnectIrq
(
    HalHandle_t HalHandle,
    HalIrqCtx_t *pIrqCtx,
    uint32_t    int_src,
    osIsrFunc   IsrFunction,
    osDpcFunc   DpcFunction,
    void*       pContext
)
{
    int32_t err=0;

    if ( (!HalHandle) || (!pIrqCtx) )
    {
        TRACE( HAL_ERROR, "%s(%d): HalHandle or pIrqCtx is NULL \n",__FUNCTION__,__LINE__ );
        return ( RET_NULL_POINTER );
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        TRACE( HAL_ERROR, "%s(%d): refCount is invaldate(0)\n",__FUNCTION__,__LINE__);
        return RET_WRONG_STATE;
    }

    if (pHalCtx->drvInfo.camsys_fd<=0)
    {
        TRACE( HAL_ERROR, "%s(%d): pHalCtx is error for haven't camsys device!\n",__FUNCTION__,__LINE__ );
        return RET_WRONG_HANDLE;
    }

    if( HalAddRef( HalHandle ) != RET_SUCCESS )
    {
        return ( RET_FAILURE );
    }

    
    pIrqCtx->HalHandle = HalHandle;

    osInterrupt *pOsIrq = &pIrqCtx->OsIrq;
    
    pOsIrq->IsrFunc   = IsrFunction;
    pOsIrq->DpcFunc   = DpcFunction;
    pOsIrq->irq_num   = int_src;
    pOsIrq->p_context = pContext;

    if( osMutexInit( &pOsIrq->isr_access_lock ) != OSLAYER_OK )
    {
        HalDelRef( pIrqCtx->HalHandle );
        TRACE( HAL_ERROR, "%s(%d): isr_access_lock init failed!\n ",__FUNCTION__,__LINE__);
        return ( RET_FAILURE );
    }

	if( osEventInit( &pOsIrq->isr_event, 1, 0 ) != OSLAYER_OK )
	{
	    osMutexDestroy( &pOsIrq->isr_access_lock );
        HalDelRef( pIrqCtx->HalHandle );
        TRACE( HAL_ERROR, "%s(%d): isr_event init failed!\n ",__FUNCTION__,__LINE__);
        return ( RET_FAILURE );
	}

	if( osEventInit( &pOsIrq->isr_exit_event, 1, 0 ) != OSLAYER_OK )
	{
	    osMutexDestroy( &pOsIrq->isr_access_lock );
		osEventDestroy( &pOsIrq->isr_event );
        HalDelRef( pIrqCtx->HalHandle );
        TRACE( HAL_ERROR, "%s(%d): isr_exit_event init failed!\n ",__FUNCTION__,__LINE__);
        return ( RET_FAILURE );
	}

    {
        camsys_irqcnnt_t irqcnnt;
        pthread_fake_t *pthread_fake=NULL;
        
        err = osThreadCreate(&pOsIrq->isr_thread, halIsrHandler, pIrqCtx); 
        if (err != OSLAYER_OK) 
        {
            osMutexDestroy( &pOsIrq->isr_access_lock );
    		osEventDestroy( &pOsIrq->isr_event );
    		osEventDestroy( &pOsIrq->isr_exit_event );
            HalDelRef( pIrqCtx->HalHandle );
            TRACE( HAL_ERROR, "%s(%d): create isr_thread  failed!\n ",__FUNCTION__,__LINE__);
            return ( RET_FAILURE );

        } else {
            pthread_fake = (pthread_fake_t*)pOsIrq->isr_thread.handle;
        }

        irqcnnt.pid = pthread_fake->pid;
        irqcnnt.timeout = 10;              //10ms
        irqcnnt.mis = pIrqCtx->misRegAddress;
        irqcnnt.icr = pIrqCtx->icrRegAddress;

        err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_IRQCONNECT, &irqcnnt);
        if (err < 0) 
        {
            osMutexDestroy( &pOsIrq->isr_access_lock );
    		osEventDestroy( &pOsIrq->isr_event );
    		osEventDestroy( &pOsIrq->isr_exit_event );
            HalDelRef( pIrqCtx->HalHandle );
            osThreadClose(&pOsIrq->isr_thread);
            TRACE( HAL_ERROR, "%s(%d): ioctl CAMSYS_IRQCONNECT  failed!\n ",__FUNCTION__,__LINE__);
            return ( RET_FAILURE );
        }
    }
	(void)osThreadSetPriority( &pOsIrq->isr_thread, OSLAYER_THREAD_PRIO_HIGHEST );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * HalDisconnectIrq()
 *****************************************************************************/
RESULT HalDisconnectIrq
(
    HalIrqCtx_t *pIrqCtx
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    if (pIrqCtx == NULL)
    {
        TRACE( HAL_ERROR, "pIrqCtx is NULL\n" );
        return RET_NULL_POINTER;
    }
    //TODO: add proper error checking below

    osInterrupt *pOsIrq = &pIrqCtx->OsIrq;
    
    (void)osEventSignal( &pOsIrq->isr_exit_event );

    (void)osEventSignal( &pOsIrq->isr_event );

    {
        int32_t err = 0;
        camsys_irqcnnt_t irqcnnt;
        pthread_fake_t *pthread_fake=NULL;
        HalContext_t *pHalCtx = (HalContext_t *)pIrqCtx->HalHandle;

        pthread_fake = (pthread_fake_t*)pOsIrq->isr_thread.handle;
        irqcnnt.pid = pthread_fake->pid;
        err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_IRQDISCONNECT, &irqcnnt);
        if ( err == 0 )
        {

            (void)osThreadClose( &pOsIrq->isr_thread );
        }
        UPDATE_RESULT( result, (err == 0) ? RET_SUCCESS : RET_FAILURE );
    }    
    (void)osEventDestroy( &pOsIrq->isr_exit_event );
    (void)osEventDestroy( &pOsIrq->isr_event );

    /* destroy mutex */
    (void)osMutexDestroy( &pOsIrq->isr_access_lock );

    pOsIrq->IsrFunc   = NULL;
    pOsIrq->DpcFunc   = NULL;
    pOsIrq->irq_num   = 0;
    pOsIrq->p_context = NULL;

    lres = HalDelRef( pIrqCtx->HalHandle );
    UPDATE_RESULT( result, lres );

    return result;
}

/******************************************************************************
 * Local functions
 *****************************************************************************/
static int32_t halIsrHandler( void *pArg )
{
    HalIrqCtx_t *pIrqCtx = (HalIrqCtx_t *)pArg;
    HalContext_t *pHalCtx = (HalContext_t *)pIrqCtx->HalHandle;
    uint32_t result;

    while( osEventTimedWait( &(pIrqCtx->OsIrq.isr_exit_event), 0) == OSLAYER_TIMEOUT )
    {

        /* check again if the ISR thread should have been quit by now */
        if ( osEventTimedWait( &(pIrqCtx->OsIrq.isr_exit_event), 0) != OSLAYER_TIMEOUT )
        {
            break;
        }

        {

            camsys_irqsta_t irqsta;
            int32_t err;


            err = ioctl( pHalCtx->drvInfo.camsys_fd, CAMSYS_IRQWAIT, &irqsta);
            if( err != 0 )
            {
                /* no interrupt has been signaled,
                 * maybe we shall terminate simply look at the exit event again */

                continue;
            } else {
                pIrqCtx->misValue = irqsta.mis;

            }
        }

        if ( ( pIrqCtx->OsIrq.DpcFunc )
          && ( (pIrqCtx->OsIrq.DpcFunc)( (void *)pIrqCtx ) != 0 ) )
        {
            return ( -1 );
        }
	}

    return ( 0 );
}

/******************************************************************************
 * HalLockContext
 *****************************************************************************/
static RESULT HalLockContext( HalContext_t *pHalCtx )
{
    // enforce validity of context
    DCT_ASSERT( pHalCtx && pHalCtx->refCount );

    // lock context
    if( osMutexLock( &pHalCtx->modMutex ) != OSLAYER_OK )
    {
        TRACE( HAL_ERROR, "Locking context failed.\n" );
        return ( RET_FAILURE );
    }

    return RET_SUCCESS;
}

/******************************************************************************
 * HalUnlockContext
 *****************************************************************************/
static RESULT HalUnlockContext( HalContext_t *pHalCtx )
{
    // enforce validity of context
    DCT_ASSERT( pHalCtx && pHalCtx->refCount );

    // unlock context
    if( osMutexUnlock( &pHalCtx->modMutex ) != OSLAYER_OK )
    {
        TRACE( HAL_ERROR, "Unlocking context failed.\n" );
        return ( RET_FAILURE );
    }

    return RET_SUCCESS;
}



/******************************************************************************
 * HalReadReg()
 *****************************************************************************/
uint32_t HalReadReg( HalHandle_t HalHandle, ulong_t reg_address )
{
    (void) HalHandle;
    DCT_ASSERT(HalHandle != NULL);

    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;    
    DCT_ASSERT(pHalCtx->drvInfo.regmem.base != NULL);
    return pHalCtx->drvInfo.regmem.base[reg_address>>2];
}


/******************************************************************************
 * HalWriteReg()
 *****************************************************************************/
void HalWriteReg( HalHandle_t HalHandle, ulong_t reg_address, uint32_t value )
{
    (void) HalHandle;
    DCT_ASSERT(HalHandle != NULL);

    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;    
    DCT_ASSERT(pHalCtx->drvInfo.regmem.base != NULL);
    pHalCtx->drvInfo.regmem.base[reg_address>>2] = value;
	   
 //	TRACE( HAL_ERROR, "write:0x%x, readback:0x%x.\n",value,pHalCtx->drvInfo.regmem.base[reg_address>>2] );
}

RESULT HalPhyCtrl(HalHandle_t HalHandle,uint32_t dev_mask,uint32_t on,uint32_t data_en_bit,uint32_t bit_rate)
{
    RESULT result = RET_SUCCESS;

    camsys_sysctrl_t sysctl;    
    camsys_mipiphy_t mipiphy;
    int err = 0;

    DCT_ASSERT(HalHandle != NULL);

    // check for valid devices
    if (dev_mask & ~HAL_DEVID_ALL)
    {
        TRACE( HAL_ERROR, "%s(%d): dev_mask is invalidate\n",__FUNCTION__,__LINE__,dev_mask);
        return RET_INVALID_PARM;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        TRACE( HAL_ERROR, "%s(%d): refCount is invaldate(0)\n",__FUNCTION__,__LINE__);
        return RET_WRONG_STATE;
    }

    if (pHalCtx->drvInfo.camsys_fd<=0)
    {
        TRACE( HAL_ERROR, "pHalCtx is error for haven't camsys device!\n" );
        return RET_WRONG_HANDLE;
    }    

   // dev_mask = HAL_DEVID_CAM_1B;
   // data_en_bit = 2;
   // bit_rate = 328;
    
    if (dev_mask & HAL_DEVID_EXTERNAL)
    {
        sysctl.dev_mask = dev_mask;
        sysctl.ops = CamSys_Phy;
        sysctl.on = on;

        if(data_en_bit == 1)
            mipiphy.data_en_bit = data_en_bit;
        else if(data_en_bit == 2)
            mipiphy.data_en_bit = 0x3;
        else if(data_en_bit == 4)
            mipiphy.data_en_bit = 0xf;
        else
            TRACE( HAL_ERROR,"mipi lane number is erro :%d\n",data_en_bit);
        mipiphy.bit_rate = bit_rate;

        memcpy(sysctl.rev,&mipiphy, sizeof(camsys_mipiphy_t));
        
        err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_SYSCTRL, &sysctl);
        if (err<0) {
            TRACE( HAL_ERROR,"CamSys_Phy control failed\n");
            UPDATE_RESULT(result, RET_FAILURE);
           
        }
        
    }else{
            TRACE( HAL_ERROR,"dev_mask 0x%x(0x%x) is erro,CamSys_Phy control failed\n",dev_mask,HAL_DEVID_EXTERNAL);
            UPDATE_RESULT(result, RET_FAILURE);
    }

    return result;

}

RESULT HalFlashTrigCtrl(HalHandle_t HalHandle,uint32_t dev_mask,uint32_t on, int mode)
{
    RESULT result = RET_SUCCESS;

    camsys_sysctrl_t sysctl;   
    int err = 0;

    DCT_ASSERT(HalHandle != NULL);

    // check for valid devices
    if (dev_mask & ~HAL_DEVID_ALL)
    {
        TRACE( HAL_ERROR, "%s(%d): dev_mask is invalidate\n",__FUNCTION__,__LINE__,dev_mask);
        return RET_INVALID_PARM;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        TRACE( HAL_ERROR, "%s(%d): refCount is invaldate(0)\n",__FUNCTION__,__LINE__);
        return RET_WRONG_STATE;
    }

    if (pHalCtx->drvInfo.camsys_fd<=0)
    {
        TRACE( HAL_ERROR, "pHalCtx is error for haven't camsys device!\n" );
        return RET_WRONG_HANDLE;
    }    

    //if (dev_mask & HAL_DEVID_EXTERNAL)
    {
        sysctl.dev_mask = dev_mask;
        sysctl.ops = CamSys_Flash_Trigger;
        sysctl.on = on;
        sysctl.rev[0] = mode;

        err = ioctl(pHalCtx->drvInfo.camsys_fd, CAMSYS_SYSCTRL, &sysctl);
        if (err<0) {
            TRACE( HAL_ERROR,"CamSys_Flash_Trigger control failed\n");
            UPDATE_RESULT(result, RET_FAILURE);
           
        }
    }/*else{
            TRACE( HAL_ERROR,"dev_mask 0x%x(0x%x) is erro,CamSys_Flash_Trigger control failed\n",dev_mask,HAL_DEVID_EXTERNAL);
            UPDATE_RESULT(result, RET_FAILURE);
    }*/

    return result;
    
}



#endif // defined ( HAL_MOCKUP )
