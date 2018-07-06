#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>

#include "mrv_all_bits.h"

#include "cameric_drv_cb.h"
#include "cameric_drv.h"



/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_ISP_FLASH_DRV_INFO  , "CAMERIC-ISP-FLASH-DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_ISP_FLASH_DRV_NOTICE0  , "CAMERIC-ISP-FLASH-DRV: ", TRACE_NOTICE0, 1 );
CREATE_TRACER( CAMERIC_ISP_FLASH_DRV_NOTICE1  , "CAMERIC-ISP-FLASH-DRV: ", TRACE_NOTICE1, 1 );
CREATE_TRACER( CAMERIC_ISP_FLASH_DRV_WARN  , "CAMERIC-ISP-FLASH-DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_ISP_FLASH_DRV_ERROR , "CAMERIC-ISP-FLASH-DRV: ", ERROR   , 1 );

#define     FLASH_SKIP_FRAMES  (3)

/******************************************************************************
 * local type definitions
 *****************************************************************************/


/******************************************************************************
 * local variable declarations
 *****************************************************************************/


/******************************************************************************
 * local function prototypes
 *****************************************************************************/


/******************************************************************************
 * local functions
 *****************************************************************************/


/******************************************************************************
 * Implementation of Driver internal API Functions
 *****************************************************************************/
/******************************************************************************
 * CamerIcIspFlashInit()
 *****************************************************************************/
static RESULT CamerIcIspFlashCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcIspFlashIntEvent_t event,
    uint32_t para
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;
    volatile MrvAllRegister_t *ptCamerIcRegMap;
    int i = 0;
    int meanLuma = 0;

    TRACE( CAMERIC_ISP_FLASH_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pFlashContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }    

    /* pointer to register map */
    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

    switch(event)
    {
        case CAMERIC_ISP_FLASH_ON_EVENT:
              ctx->pFlashContext->flash_on_int_received = BOOL_TRUE;
              break;
        case CAMERIC_ISP_FLASH_OFF_EVENT:
              ctx->pFlashContext->flash_off_int_received= BOOL_TRUE;
              break;
        case  CAMERIC_ISP_FLASH_CAPTURE_FRAME:
              for(;i < 25;i++)
                meanLuma += ctx->pIspExpContext->Luma[i];
              meanLuma = meanLuma /25;
               
              if(ctx->pFlashContext->flash_skip_frames > 0){
                  ctx->pFlashContext->flash_skip_frames--;
                  TRACE( CAMERIC_ISP_FLASH_DRV_NOTICE1, "%s: isp flash skip frame:%d\n", __FUNCTION__,para );
              } else {
                  if(ctx->pFlashContext->flash_on_int_received &&  (!ctx->pFlashContext->flash_off_int_received)){
                      if((ctx->pFlashContext->flash_desired_frame_start == 0xffffffff) /*&& (meanLuma > 30)*/){
                          ctx->pFlashContext->flash_desired_frame_start = para;
                          TRACE( CAMERIC_ISP_FLASH_DRV_NOTICE1, "%s: isp flash got desired frame:%d , meanLuma = %d \n", __FUNCTION__,para ,meanLuma);
                      }
                    //ctx->pFlashContext->pIspFlashCb               =NULL;
                  } else if(ctx->pFlashContext->flash_on_int_received && ctx->pFlashContext->flash_off_int_received){
                      if(ctx->pFlashContext->flash_desired_frame_start == 0xffffffff)
                          ctx->pFlashContext->flash_desired_frame_start = para;
                    //ctx->pFlashContext->pIspFlashCb               =NULL;
                      TRACE( CAMERIC_ISP_FLASH_DRV_WARN, "%s: isp flash got frame out of the flash time:%d\n", __FUNCTION__ ,para);
                  }else{
                      TRACE( CAMERIC_ISP_FLASH_DRV_NOTICE1, "%s: isp flash got frame:%d\n", __FUNCTION__,para );
                  }
                    
              }
              break;
        case  CAMERIC_ISP_FLASH_STOP_EVENT:
              CamerIcIspFlashStop(ctx,true);
              break;  
        default:
              break;
    }

    TRACE( CAMERIC_ISP_FLASH_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}
 
RESULT CamerIcIspFlashInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_FLASH_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pFlashContext = ( CamerIcIspFlashContext_t *)malloc( sizeof(CamerIcIspFlashContext_t) );
    if (  ctx->pFlashContext == NULL )
    {
        TRACE( CAMERIC_ISP_FLASH_DRV_ERROR,  "%s: Can't allocate CamerIC ISP FLASH context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pFlashContext, 0, sizeof(CamerIcIspFlashContext_t) );

    
    ctx->pFlashContext->mode       = CAMERIC_ISP_FLASH_OFF;
    ctx->pFlashContext->flash_on_int_received     = BOOL_FALSE;
    ctx->pFlashContext->flash_off_int_received    = BOOL_FALSE;
    ctx->pFlashContext->flash_skip_frames         = FLASH_SKIP_FRAMES;
    ctx->pFlashContext->flash_desired_frame_start = 0xffffffff;
    ctx->pFlashContext->pIspFlashCb               =NULL;
    TRACE( CAMERIC_ISP_FLASH_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspFlashRelease()
 *****************************************************************************/
RESULT CamerIcIspFlashRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_FLASH_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pFlashContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pFlashContext, 0, sizeof(CamerIcIspFlashContext_t) );
    free ( ctx->pFlashContext );
    ctx->pFlashContext = NULL;

    TRACE( CAMERIC_ISP_FLASH_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/
 
/******************************************************************************
 * CamerIcIspFlashConfigure()
 *****************************************************************************/
RESULT CamerIcIspFlashConfigure
(
    CamerIcDrvHandle_t  handle,
    CamerIcIspFlashCfg_t *cfgFsh
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_FLASH_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pFlashContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pFlashContext->mode = cfgFsh->mode;
    ctx->pFlashContext->active_pol = cfgFsh->active_pol;
	ctx->pFlashContext->flashtype = cfgFsh->flashtype;
	ctx->pFlashContext->dev_mask  = cfgFsh->dev_mask;
    TRACE( CAMERIC_ISP_FLASH_DRV_INFO, "%s: (exit)  ctx->pFlashContext->mode: 0x%x \n", __FUNCTION__,
        ctx->pFlashContext->mode);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspFlashStart()
 *****************************************************************************/
RESULT CamerIcIspFlashStart
(
    CamerIcDrvHandle_t  handle,
    bool_t operate_now
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;
    volatile MrvAllRegister_t *ptCamerIcRegMap;

    TRACE( CAMERIC_ISP_FLASH_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pFlashContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* pointer to register map */
    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

    switch(ctx->pFlashContext->mode)
    {
        case CAMERIC_ISP_FLASH_OFF:
        {

            break;
        }
        case CAMERIC_ISP_FLASH_ON:
        {
            ctx->pFlashContext->flash_on_int_received     = BOOL_FALSE;
            ctx->pFlashContext->flash_off_int_received    = BOOL_FALSE;
            ctx->pFlashContext->flash_skip_frames         = FLASH_SKIP_FRAMES;
            ctx->pFlashContext->flash_desired_frame_start = 0xffffffff;
            ctx->pFlashContext->pIspFlashCb = CamerIcIspFlashCb;
            if (operate_now == BOOL_TRUE) {
                HalWriteReg(ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_flash_prediv), 0xffffffff);
                HalWriteReg(ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_flash_time), 0xffffffff);
                HalWriteReg(ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_flash_delay), 0);
                HalWriteReg(ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_flash_maxp), 0xffffffff);
                //0:high active;1:low active
                if(ctx->pFlashContext->flashtype == 2)
                	HalWriteReg(ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_flash_config), ((~(ctx->pFlashContext->active_pol) & 0x1)  << (MRV_FLASH_FL_POL_SHIFT)) | (0x1 << MRV_FLASH_PRELIGHT_MODE_SHIFT));
				else
					HalWriteReg(ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_flash_config), ((~(ctx->pFlashContext->active_pol) & 0x1)  << (MRV_FLASH_FL_POL_SHIFT)));

				HalFlashTrigCtrl(ctx->HalHandle,ctx->pFlashContext->dev_mask,1,ctx->pFlashContext->mode);
				
				if(ctx->pFlashContext->flashtype == 2)
					HalWriteReg(ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_flash_cmd), 0x3); 
				else
					HalWriteReg(ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_flash_cmd), 0x2);
                TRACE( CAMERIC_ISP_FLASH_DRV_NOTICE1, "flash trigger on"); 
            } else {

            }
            
            break;
        }

        case CAMERIC_ISP_FLASH_AUTO:
        {

            break;
        }

        case CAMERIC_ISP_FLASH_RED_EYE:
        {

            break;
        }

        case CAMERIC_ISP_FLASH_TORCH:
        {
            if (operate_now == BOOL_TRUE) {
    			HalFlashTrigCtrl(ctx->HalHandle,ctx->pFlashContext->dev_mask,1,ctx->pFlashContext->mode);
                TRACE( CAMERIC_ISP_FLASH_DRV_NOTICE1, "flash torch on"); 
			}
            break;
        }

        default:
        {
            TRACE( CAMERIC_ISP_FLASH_DRV_ERROR,"ctx->pFlashContext->mode: %d isn't support",
                ctx->pFlashContext->mode);
            break;
        }
       

    }

    TRACE( CAMERIC_ISP_FLASH_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}

/******************************************************************************
 * CamerIcIspFlashStop()
 *****************************************************************************/
RESULT CamerIcIspFlashStop
(
    CamerIcDrvHandle_t  handle,
    bool_t operate_now
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;
    volatile MrvAllRegister_t *ptCamerIcRegMap;

    TRACE( CAMERIC_ISP_FLASH_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pFlashContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }    

    /* pointer to register map */
    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

    switch(ctx->pFlashContext->mode)
    {
        case CAMERIC_ISP_FLASH_OFF:
        {

            break;
        }
        case CAMERIC_ISP_FLASH_ON:
        {
            if (operate_now == BOOL_TRUE) {
                HalWriteReg(ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_flash_cmd), 0x00);
                HalFlashTrigCtrl(ctx->HalHandle,ctx->pFlashContext->dev_mask,0,0);

                ctx->pFlashContext->flash_on_int_received     = BOOL_FALSE;
                ctx->pFlashContext->flash_off_int_received    = BOOL_FALSE;
                ctx->pFlashContext->flash_skip_frames         = FLASH_SKIP_FRAMES;
                ctx->pFlashContext->flash_desired_frame_start = 0xffffffff;
                ctx->pFlashContext->pIspFlashCb               =NULL;

                TRACE( CAMERIC_ISP_FLASH_DRV_NOTICE1, "flash trigger off");
            }            
            break;
        }

        case CAMERIC_ISP_FLASH_AUTO:
        {

            break;
        }

        case CAMERIC_ISP_FLASH_RED_EYE:
        {

            break;
        }

        case CAMERIC_ISP_FLASH_TORCH:
        {
            if (operate_now == BOOL_TRUE) {
                HalFlashTrigCtrl(ctx->HalHandle,ctx->pFlashContext->dev_mask,0,0);
                TRACE( CAMERIC_ISP_FLASH_DRV_NOTICE0, "flash torch off");
            }
            break;
        }

        default:
        {
            TRACE( CAMERIC_ISP_FLASH_DRV_ERROR,"ctx->pFlashContext->mode: %d isn't support",
                ctx->pFlashContext->mode);
            break;
        }
       

    }

    TRACE( CAMERIC_ISP_FLASH_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}

 

