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
 * @file    cameric_isp_awb.c
 *
 * @brief   This file implements the driver external and internal API.
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
 * Is White Balance module available ?
 *****************************************************************************/
#if defined(MRV_AWB_VERSION)

/******************************************************************************
 * White Balance Module is available.
 *****************************************************************************/

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_ISP_AWB_DRV_INFO  , "CAMERIC-ISP-AWB-DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_ISP_AWB_DRV_WARN  , "CAMERIC-ISP-AWB-DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_ISP_AWB_DRV_ERROR , "CAMERIC-ISP-AWB-DRV: ", ERROR   , 1 );


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
 * CamerIcIspAwbInit()
 *****************************************************************************/
RESULT CamerIcIspAwbInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pIspAwbContext = ( CamerIcIspAwbContext_t *)malloc( sizeof(CamerIcIspAwbContext_t) );
    if (  ctx->pIspAwbContext == NULL )
    {
        TRACE( CAMERIC_ISP_AWB_DRV_ERROR,  "%s: Can't allocate CamerIC ISP HIST context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pIspAwbContext, 0, sizeof(CamerIcIspAwbContext_t) );

    ctx->pIspAwbContext->enabled    = BOOL_FALSE;
    ctx->pIspAwbContext->mode       = CAMERIC_ISP_AWB_MEASURING_MODE_RGB;

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAwbRelease()
 *****************************************************************************/
RESULT CamerIcIspAwbRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspAwbContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pIspAwbContext, 0, sizeof(CamerIcIspAwbContext_t) );
    free ( ctx->pIspAwbContext );
    ctx->pIspAwbContext = NULL;

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAwbSignal()
 *****************************************************************************/
void CamerIcIspAwbSignal
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspAwbContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return;
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        CamerIcAwbMeasuringResult_t *pMeasResult = &ctx->pIspAwbContext->MeasResult;

        uint32_t isp_awb_mean = (uint32_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_mean) );

        pMeasResult->MeanY__G     = (uint8_t)REG_GET_SLICE( isp_awb_mean, MRV_ISP_AWB_MEAN_Y__G );
        pMeasResult->MeanCb__B    = (uint8_t)REG_GET_SLICE( isp_awb_mean, MRV_ISP_AWB_MEAN_CB__B );
        pMeasResult->MeanCr__R    = (uint8_t)REG_GET_SLICE( isp_awb_mean, MRV_ISP_AWB_MEAN_CR__R );
        pMeasResult->NoWhitePixel = (uint32_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_white_cnt) );

        if ( ctx->pIspAwbContext->EventCb.func != NULL )
        {
            ctx->pIspAwbContext->EventCb.func( CAMERIC_ISP_EVENT_AWB,
                    (void *)(pMeasResult), ctx->pIspAwbContext->EventCb.pUserContext );
        }
    }

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );
}



/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspAwbRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspAwbRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void                *pUserContext
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspAwbContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)  
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( ctx->pIspAwbContext->EventCb.func != NULL )
    {
        return ( RET_BUSY );
    }

    if ( func == NULL )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        ctx->pIspAwbContext->EventCb.func          = func;
        ctx->pIspAwbContext->EventCb.pUserContext  = pUserContext;
    }

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAwbDeRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspAwbDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspAwbContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)  
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    ctx->pIspAwbContext->EventCb.func          = NULL;
    ctx->pIspAwbContext->EventCb.pUserContext  = NULL;

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAwbEnable()
 *****************************************************************************/
RESULT CamerIcIspAwbEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_hist_prop;

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspAwbContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_awb_prop;
        uint32_t isp_imsc;

        /* enable measuring module */
        isp_awb_prop = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_prop) );
        REG_SET_SLICE( isp_awb_prop, MRV_ISP_AWB_MODE, MRV_ISP_AWB_MODE_MEAS ); 
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_prop), isp_awb_prop);

        /* enable interrupt */
        isp_imsc = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc) );
        isp_imsc |= MRV_ISP_IMSC_AWB_DONE_MASK;
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc), isp_imsc );

        ctx->pIspAwbContext->enabled = BOOL_TRUE;
    }

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAwbDisable()
 *****************************************************************************/
RESULT CamerIcIspAwbDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspAwbContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_awb_prop;
        uint32_t isp_imsc;

        /* disable measuring module */
        isp_awb_prop = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_prop) );
        REG_SET_SLICE( isp_awb_prop, MRV_ISP_AWB_MODE, MRV_ISP_AWB_MODE_NOMEAS ); 
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_prop), isp_awb_prop);

        /* disable interrupt */
        isp_imsc = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc) );
        isp_imsc &= ~MRV_ISP_IMSC_AWB_DONE_MASK;
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc), isp_imsc );

        ctx->pIspAwbContext->enabled = BOOL_FALSE;
    }

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAwbIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspAwbIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspAwbContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pIsEnabled )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsEnabled = ctx->pIspAwbContext->enabled;

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAwbSetMeasuringMode()
 *****************************************************************************/
RESULT CamerIcIspAwbSetMeasuringMode
(
    CamerIcDrvHandle_t                  handle,
    const CamerIcIspAwbMeasuringMode_t  mode,
    const CamerIcAwbMeasuringConfig_t   *pMeasConfig
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspAwbContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pMeasConfig == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    if (  (mode > CAMERIC_ISP_AWB_MEASURING_MODE_INVALID) && (mode < CAMERIC_ISP_AWB_MEASURING_MODE_MAX) )
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_awb_prop   = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_prop) );
        uint32_t isp_awb_thresh = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_thresh) );
        uint32_t isp_awb_ref    = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_ref) );
        uint32_t isp_awb_frames = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_frames) );

        switch ( mode )
        {
            case CAMERIC_ISP_AWB_MEASURING_MODE_YCBCR:
                {
                    REG_SET_SLICE( isp_awb_prop, MRV_ISP_AWB_MEAS_MODE, MRV_ISP_AWB_MEAS_MODE_YCBCR ); 
                    if ( pMeasConfig->MaxY == 0U )
                    {
                        REG_SET_SLICE( isp_awb_prop, MRV_ISP_AWB_MAX_EN, MRV_ISP_AWB_MAX_EN_DISABLE );
                    }
                    else
                    {
                        REG_SET_SLICE( isp_awb_prop, MRV_ISP_AWB_MAX_EN, MRV_ISP_AWB_MAX_EN_ENABLE );
                    }
                    break;
                }
            case CAMERIC_ISP_AWB_MEASURING_MODE_RGB:
                {
                    REG_SET_SLICE( isp_awb_prop, MRV_ISP_AWB_MAX_EN, MRV_ISP_AWB_MAX_EN_DISABLE );
                    REG_SET_SLICE( isp_awb_prop, MRV_ISP_AWB_MEAS_MODE, MRV_ISP_AWB_MEAS_MODE_RGB ); 
                    break;
                }
            default:
                {
                    TRACE( CAMERIC_ISP_AWB_DRV_ERROR,  "%s: invalid ISP AWB measuring mode selected (%d)\n",  __FUNCTION__, mode );
                    return ( RET_OUTOFRANGE );
                }
        }
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_prop), isp_awb_prop );

        /* set thresholds */
        REG_SET_SLICE( isp_awb_thresh, MRV_ISP_AWB_MAX_Y, pMeasConfig->MaxY );
        REG_SET_SLICE( isp_awb_thresh, MRV_ISP_AWB_MIN_Y__MAX_G, pMeasConfig->MinY_MaxG );
        REG_SET_SLICE( isp_awb_thresh, MRV_ISP_AWB_MAX_CSUM, pMeasConfig->MaxCSum );
        REG_SET_SLICE( isp_awb_thresh, MRV_ISP_AWB_MIN_C, pMeasConfig->MinC );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_thresh), isp_awb_thresh );
        
        REG_SET_SLICE( isp_awb_ref, MRV_ISP_AWB_REF_CR__MAX_R, pMeasConfig->RefCr_MaxR );
        REG_SET_SLICE( isp_awb_ref, MRV_ISP_AWB_REF_CB__MAX_B, pMeasConfig->RefCb_MaxB );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_ref), isp_awb_ref );

        isp_awb_frames = 0;
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_frames), isp_awb_frames );
 
        ctx->pIspAwbContext->mode = mode;
    }
    else
    {
        return ( RET_OUTOFRANGE );
    }

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );
     
    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAwbSetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspAwbSetMeasuringWindow
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

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspAwbContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    if ( (MRV_ISP_AWB_H_OFFS_MAX < x)
            || (MRV_ISP_AWB_V_OFFS_MAX < y) )
    {
        return ( RET_OUTOFRANGE );
    }

    if ( (MRV_ISP_AWB_H_SIZE_MAX < width) 
            || (MRV_ISP_AWB_V_SIZE_MAX < height) )
    {
        return ( RET_OUTOFRANGE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_h_offs), (MRV_ISP_AWB_H_OFFS_MASK & x) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_v_offs), (MRV_ISP_AWB_V_OFFS_MASK & y) );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_h_size), (MRV_ISP_AWB_H_SIZE_MASK & width) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_v_size), (MRV_ISP_AWB_V_SIZE_MASK & height) );

        ctx->pIspAwbContext->Window.hOffset = x;
        ctx->pIspAwbContext->Window.vOffset = y;
        ctx->pIspAwbContext->Window.width   = width;
        ctx->pIspAwbContext->Window.height  = height;
    }

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAwbGetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspAwbGetMeasuringWindow
(
    CamerIcDrvHandle_t  handle,   
    CamerIcWindow_t   *pWindow    
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;
    

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspAwbContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    if (pWindow != NULL)
        *pWindow = ctx->pIspAwbContext->Window;

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
   
}


/******************************************************************************
 * CamerIcIspAwbGetGains()
 *****************************************************************************/
RESULT CamerIcIspAwbGetGains
(
    CamerIcDrvHandle_t  handle,
    CamerIcGains_t      *pGains
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspAwbContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_INVALID )  
    {
        return ( RET_WRONG_STATE );
    }

    if ( pGains != NULL )
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap  = (MrvAllRegister_t *)ctx->base;

        uint32_t data = 0UL;

        /* read red and blue gains */
        data            = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_gain_rb) );
        pGains->Red     = REG_GET_SLICE( data, MRV_ISP_AWB_GAIN_R );
        pGains->Blue    = REG_GET_SLICE( data, MRV_ISP_AWB_GAIN_B );

        /* read greenR and greenB gains */
        data            = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_gain_g) );
        pGains->GreenR  = REG_GET_SLICE( data, MRV_ISP_AWB_GAIN_GR );
        pGains->GreenB  = REG_GET_SLICE( data, MRV_ISP_AWB_GAIN_GB );
    }
    else
    {
        return ( RET_INVALID_PARM );
    }

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAwbSetGains()
 *****************************************************************************/
RESULT CamerIcIspAwbSetGains
(
    CamerIcDrvHandle_t      handle,
    const CamerIcGains_t    *pGains
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_INVALID )  
    {
        return ( RET_WRONG_STATE );
    }

    if ( pGains != NULL )
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t data = 0UL;

        /* write red and blue gains */
        REG_SET_SLICE( data, MRV_ISP_AWB_GAIN_R, pGains->Red );
        REG_SET_SLICE( data, MRV_ISP_AWB_GAIN_B, pGains->Blue );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_gain_rb), data);
        
        data = 0UL;

        /* write greenR and greenB gains */
        REG_SET_SLICE( data, MRV_ISP_AWB_GAIN_GR, pGains->GreenR );
        REG_SET_SLICE( data, MRV_ISP_AWB_GAIN_GB, pGains->GreenB );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_awb_gain_g), data);
    }
    else
    {
        return ( RET_INVALID_PARM );
    }

    TRACE( CAMERIC_ISP_AWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


#else  /* if defined(MRV_AWB_VERSION) */

/******************************************************************************
 * White Balance Module is not available.
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 * 
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspAwbRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspAwbRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void                *pUserContext
)
{
    (void)handle;
    (void)func;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspAwbDeRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspAwbDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspAwbEnable()
 *****************************************************************************/
RESULT CamerIcIspAwbEnable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspAwbDisable()
 *****************************************************************************/
RESULT CamerIcIspAwbDisable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspAwbIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspAwbIsEnabled
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
 * CamerIcIspAwbSetMeasuringMode()
 *****************************************************************************/
RESULT CamerIcIspAwbSetMeasuringMode
(
    CamerIcDrvHandle_t                  handle,
    const CamerIcIspAwbMeasuringMode_t  mode,
    const CamerIcAwbMeasuringConfig_t   *pMeasConfig
)
{
    (void)handle;
    (void)mode;
    (void)pMeasConfig;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspAwbSetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspAwbSetMeasuringWindow
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

/******************************************************************************
 * CamerIcIspAwbGetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspAwbGetMeasuringWindow
(
    CamerIcDrvHandle_t  handle,   
    CamerIcWindow_t   *pWindow    
)
{
	(void)handle;
	(void)pWindow;
	return ( RET_NOTSUPP );
}

/******************************************************************************
 * CamerIcIspAwbGetGains()
 *****************************************************************************/
RESULT CamerIcIspAwbGetGains
(
    CamerIcDrvHandle_t  handle,
    CamerIcGains_t      *pGains
)
{
    (void)handle;
    (void)pGains;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspAwbSetGains()
 *****************************************************************************/
RESULT CamerIcIspAwbSetGains
(
    CamerIcDrvHandle_t      handle,
    const CamerIcGains_t    *pGains
)
{
    (void)handle;
    (void)pGains;
    return ( RET_NOTSUPP );
}

#endif /* if defined(MRV_AWB_VERSION) */

