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
 * @file cameric_isp_exp.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>

#include "mrv_all_bits.h"

#include "cameric_drv_cb.h"
#include "cameric_drv.h"


/******************************************************************************
 * Is the hardware Exporsure Measurement module available ?
 *****************************************************************************/
#if defined(MRV_AUTO_EXPOSURE_VERSION)

/******************************************************************************
 * Exporsure module is available.
 *****************************************************************************/

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_ISP_EXP_DRV_INFO  , "CAMERIC-ISP-EXP-DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_ISP_EXP_DRV_WARN  , "CAMERIC-ISP-EXP-DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_ISP_EXP_DRV_ERROR , "CAMERIC-ISP-EXP-DRV: ", ERROR   , 1 );

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
 * CamerIcIspExpInit()
 *****************************************************************************/
RESULT CamerIcIspExpInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pIspExpContext = ( CamerIcIspExpContext_t *)malloc( sizeof(CamerIcIspExpContext_t) );
    if (  ctx->pIspExpContext == NULL )
    {
        TRACE( CAMERIC_ISP_EXP_DRV_ERROR,  "%s: Can't allocate CamerIC ISP HIST context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pIspExpContext, 0, sizeof(CamerIcIspExpContext_t) );

    ctx->pIspExpContext->enabled    = BOOL_FALSE;
    ctx->pIspExpContext->mode       = CAMERIC_ISP_EXP_MEASURING_MODE_1;

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspExpRelease()
 *****************************************************************************/
RESULT CamerIcIspExpRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspExpContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pIspExpContext, 0, sizeof(CamerIcIspExpContext_t) );
    free ( ctx->pIspExpContext );
    ctx->pIspExpContext = NULL;

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspExpSignal()
 *****************************************************************************/
void CamerIcIspExpSignal
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspExpContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return;
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_exp_ctrl;

        ctx->pIspExpContext->Luma[ 0] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_00) );
        ctx->pIspExpContext->Luma[ 1] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_10) );
        ctx->pIspExpContext->Luma[ 2] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_20) );
        ctx->pIspExpContext->Luma[ 3] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_30) );
        ctx->pIspExpContext->Luma[ 4] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_40) );
        ctx->pIspExpContext->Luma[ 5] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_01) );
        ctx->pIspExpContext->Luma[ 6] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_11) );
        ctx->pIspExpContext->Luma[ 7] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_21) );
        ctx->pIspExpContext->Luma[ 8] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_31) );
        ctx->pIspExpContext->Luma[ 9] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_41) );
        ctx->pIspExpContext->Luma[10] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_02) );
        ctx->pIspExpContext->Luma[11] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_12) );
        ctx->pIspExpContext->Luma[12] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_22) );
        ctx->pIspExpContext->Luma[13] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_32) );
        ctx->pIspExpContext->Luma[14] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_42) );
        ctx->pIspExpContext->Luma[15] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_03) );
        ctx->pIspExpContext->Luma[16] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_13) );
        ctx->pIspExpContext->Luma[17] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_23) );
        ctx->pIspExpContext->Luma[18] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_33) );
        ctx->pIspExpContext->Luma[19] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_43) );
        ctx->pIspExpContext->Luma[20] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_04) );
        ctx->pIspExpContext->Luma[21] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_14) );
        ctx->pIspExpContext->Luma[22] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_24) );
        ctx->pIspExpContext->Luma[23] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_34) );
        ctx->pIspExpContext->Luma[24] = (uint8_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_mean_44) );

        if ( ctx->pIspExpContext->EventCb.func != NULL )
        {
            ctx->pIspExpContext->EventCb.func( CAMERIC_ISP_EVENT_MEANLUMA,
                                                    (void *)(ctx->pIspExpContext->Luma),
                                                    ctx->pIspExpContext->EventCb.pUserContext );
        }
    }

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );
}



/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspExpRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspExpRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void 			    *pUserContext
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspExpContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)  
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( ctx->pIspExpContext->EventCb.func != NULL )
    {
        return ( RET_BUSY );
    }

    if ( func == NULL )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        ctx->pIspExpContext->EventCb.func          = func;
        ctx->pIspExpContext->EventCb.pUserContext  = pUserContext;
    }

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspExpDeRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspExpDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspExpContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)  
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    ctx->pIspExpContext->EventCb.func          = NULL;
    ctx->pIspExpContext->EventCb.pUserContext  = NULL;

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}

/******************************************************************************
 * CamerIcIspExpEnable()
 *****************************************************************************/
RESULT CamerIcIspExpEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_hist_prop;

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspExpContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_exp_ctrl;
        uint32_t isp_imsc;
           
        /* enable measuring module */
        isp_exp_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_ctrl) );
        REG_SET_SLICE( isp_exp_ctrl, MRV_AE_EXP_START, 1 ); 
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_ctrl), isp_exp_ctrl);

        /* enable interrupt */
        isp_imsc = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc) );
        isp_imsc |= MRV_ISP_IMSC_EXP_END_MASK;
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc), isp_imsc );

        ctx->pIspExpContext->enabled = BOOL_TRUE;
    }

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspExpDisable()
 *****************************************************************************/
RESULT CamerIcIspExpDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspExpContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_exp_ctrl;
        uint32_t isp_imsc;
           
        /* disable measuring module */
        isp_exp_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_ctrl) );
        REG_SET_SLICE( isp_exp_ctrl, MRV_AE_EXP_START, 0 ); 
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_ctrl), isp_exp_ctrl);

        /* disable interrupt */
        isp_imsc = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc) );
        isp_imsc &= ~MRV_ISP_IMSC_EXP_END_MASK;
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc), isp_imsc );

        ctx->pIspExpContext->enabled = BOOL_FALSE;
    }

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspExpIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspExpIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspExpContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pIsEnabled )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsEnabled = ctx->pIspExpContext->enabled;

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspExpSetMeasuringMode()
 *****************************************************************************/
RESULT CamerIcIspExpSetMeasuringMode
(
    CamerIcDrvHandle_t  		        handle,
    const CamerIcIspExpMeasuringMode_t  mode	
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspExpContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if (  (mode > CAMERIC_ISP_EXP_MEASURING_MODE_INVALID) && (mode < CAMERIC_ISP_EXP_MEASURING_MODE_MAX) )
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_exp_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_ctrl) );

        switch ( mode )
        {
            case CAMERIC_ISP_EXP_MEASURING_MODE_1:
                {
                    REG_SET_SLICE( isp_exp_ctrl, MRV_AE_EXP_MEAS_MODE, MRV_AE_EXP_MEAS_MODE_0 ); 
                    break;
                }
            case CAMERIC_ISP_EXP_MEASURING_MODE_2:
                {
                    REG_SET_SLICE( isp_exp_ctrl, MRV_AE_EXP_MEAS_MODE, MRV_AE_EXP_MEAS_MODE_1 ); 
                    break;
                }
            default:
                {
                    TRACE( CAMERIC_ISP_EXP_DRV_ERROR,  "%s: invalid ISP EXPOSURE measuring mode selected (%d)\n",  __FUNCTION__, mode );
                    return ( RET_OUTOFRANGE );
                }
        }
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_ctrl), isp_exp_ctrl);

        ctx->pIspExpContext->mode = mode;
    }
    else
    {
        return ( RET_OUTOFRANGE );
    }

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );
     
    return ( RET_SUCCESS );
}
/******************************************************************************
 * CamerIcIspExpGetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspExpGetMeasuringWindow
(
    CamerIcDrvHandle_t  handle,
    CamerIcWindow_t   *pWindow,
    CamerIcWindow_t   *pGrid
    
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;
    

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspExpContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    if (pWindow != NULL)
        *pWindow = ctx->pIspExpContext->Window;
    if (pGrid != NULL)
        *pGrid = ctx->pIspExpContext->Grid;
    
    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}

/******************************************************************************
 * CamerIcIspExpSetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspExpSetMeasuringWindow
(
    CamerIcDrvHandle_t  handle,
    const uint16_t      x,
    const uint16_t      y,
    const uint16_t      width,
    const uint16_t      height
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    uint16_t value;

    uint16_t GridHeight;
    uint16_t GridWidth;

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (enter)  (%d,%d,%d,%d)\n", __FUNCTION__,
        x,y,width,height);

    if ( (ctx == NULL) || (ctx->pIspExpContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (MRV_AE_ISP_EXP_H_OFFSET_MAX < x)
            || (MRV_AE_ISP_EXP_V_OFFSET_MAX < y) )
    {
        return ( RET_OUTOFRANGE );
    }

    GridWidth   = (uint16_t)( (width / 5) - 1 );
    GridHeight  = (uint16_t)( (height / 5) - 1 );

    if ( (MRV_AE_ISP_EXP_H_SIZE_MAX < GridWidth) 
            || (MRV_AE_ISP_EXP_V_SIZE_MAX < GridHeight) )
    {
        return ( RET_OUTOFRANGE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_h_offset), (MRV_AE_ISP_EXP_H_OFFSET_MASK & x) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_v_offset), (MRV_AE_ISP_EXP_V_OFFSET_MASK & y) );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_h_size), (MRV_AE_ISP_EXP_H_SIZE_MASK & GridWidth) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_exp_v_size), (MRV_AE_ISP_EXP_V_SIZE_MASK & GridHeight) );

        ctx->pIspExpContext->Grid.hOffset = 0;
        ctx->pIspExpContext->Grid.vOffset = 0;
        ctx->pIspExpContext->Grid.width   = GridWidth;
        ctx->pIspExpContext->Grid.height  = GridHeight;

        ctx->pIspExpContext->Window.hOffset = x;
        ctx->pIspExpContext->Window.vOffset = y;
        ctx->pIspExpContext->Window.width   = width;
        ctx->pIspExpContext->Window.height  = height;
    }

    TRACE( CAMERIC_ISP_EXP_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


#else  /* #if defined(MRV_AUTO_EXPOSURE_VERSION)  */


/******************************************************************************
 * Exporsure module is not available.
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 *
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspExpRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspExpRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void 			    *pUserContext
)
{
    (void)handle;
    (void)func;
    (void)pUserContext;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspExpDeRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspExpDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspExpEnable()
 *****************************************************************************/
RESULT CamerIcIspExpEnable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspExpDisable()
 *****************************************************************************/
RESULT CamerIcIspExpDisable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspExpIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspExpIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
)
{
    (void)handle;
    (void)pIsEnabled;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspExpSetMeasuringMode()
 *****************************************************************************/
RESULT CamerIcIspExpSetMeasuringMode
(
    CamerIcDrvHandle_t  		        handle,
    const CamerIcIspExpMeasuringMode_t  mode	
)
{
    (void)handle;
    (void)mode;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspExpSetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspExpSetMeasuringWindow
(
    CamerIcDrvHandle_t  handle,
    const uint16_t      x,
    const uint16_t      y,
    const uint16_t      width,
    const uint16_t      height
)
{
    (void)handle;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    return ( RET_NOTSUPP );
}


#endif /* #if defined(MRV_AUTO_EXPOSURE_VERSION)  */

