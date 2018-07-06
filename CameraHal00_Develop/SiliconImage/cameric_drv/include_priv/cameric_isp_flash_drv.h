#ifndef __CAMERIC_ISP_FLASH_DRV_H__
#define __CAMERIC_ISP_FLASH_DRV_H__

#include <ebase/types.h>
#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_flash_drv_api.h>



typedef struct CamerIcIspFlashContext_s
{
    CamerIcIspFlashMode_t  mode;
    CamerIcIspFlashTriggerPol_t active_pol;
    int flashtype;
    unsigned int dev_mask;
    unsigned int delay_t;                    /* us */
    unsigned int open_t;                     /* us */
    unsigned int maxp_t;                     /* us */
    

    bool    flash_on_int_received;
    bool    flash_off_int_received;
    int     flash_skip_frames;
    uint32_t     flash_desired_frame_start;
    CamerIcIspFlashEventCb pIspFlashCb;
    
} CamerIcIspFlashContext_t;


/******************************************************************************
 * CamerIcIspFlashInit()
 *****************************************************************************/
RESULT CamerIcIspFlashInit
(
    CamerIcDrvHandle_t handle
);

/******************************************************************************
 * CamerIcIspFlashRelease()
 *****************************************************************************/
RESULT CamerIcIspFlashRelease
(
    CamerIcDrvHandle_t handle
);
#endif

