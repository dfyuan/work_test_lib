/******************************************************************************
 *
 * Copyright 2012, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
/**
 * @file cameric_isp_vs.c
 *
 * @brief
 *   Implementation of the video stabilization measurement driver.
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
 * Is the hardware Video Stabilization Measurement module available ?
 *****************************************************************************/
#if defined(MRV_VSM_VERSION)

/******************************************************************************
 * Video Stabilization module is available.
 *****************************************************************************/

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_ISP_VSM_DRV_INFO  , "CAMERIC-ISP-VSM-DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_ISP_VSM_DRV_WARN  , "CAMERIC-ISP-VSM-DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_ISP_VSM_DRV_ERROR , "CAMERIC-ISP-VSM-DRV: ", ERROR   , 1 );

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
 * CamerIcIspVsmInit()
 *****************************************************************************/
RESULT CamerIcIspVsmInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_VSM_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pIspVsmContext = ( CamerIcIspVsmContext_t *)malloc( sizeof(CamerIcIspVsmContext_t) );
    if ( ctx->pIspVsmContext == NULL )
    {
        TRACE( CAMERIC_ISP_VSM_DRV_ERROR,  "%s: Can't allocate CamerIC ISP VSM context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pIspVsmContext, 0, sizeof(CamerIcIspVsmContext_t) );

    ctx->pIspVsmContext->enabled    = BOOL_FALSE;

    TRACE( CAMERIC_ISP_VSM_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspVsmRelease()
 *****************************************************************************/
RESULT CamerIcIspVsmRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_VSM_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspVsmContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pIspVsmContext, 0, sizeof(CamerIcIspVsmContext_t) );
    free ( ctx->pIspVsmContext );
    ctx->pIspVsmContext = NULL;

    TRACE( CAMERIC_ISP_VSM_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspVsmSignal()
 *****************************************************************************/
void CamerIcIspVsmSignal
(
    CamerIcDrvHandle_t handle,
    ulong_t            currFrameId
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_VSM_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspVsmContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return;
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t delta_h_raw, delta_v_raw;

        /* read raw register content from isp_vsm_delta_h and isp_vsm_delta_v */
        delta_h_raw = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_vsm_delta_h) );
        delta_v_raw = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_vsm_delta_v) );

        /* both raw values contain a signed 12 bit number in bits [11:0], which
         * we now undertake sign extension before we store it in our context
         * structure */
        delta_h_raw = (delta_h_raw & ISP_VSM_DELTA_H_MASK) >> ISP_VSM_DELTA_H_SHIFT;
        delta_v_raw = (delta_v_raw & ISP_VSM_DELTA_V_MASK) >> ISP_VSM_DELTA_V_SHIFT;

        /* check highest bit of delta_h slice in register and apply sign
         * extension if applicable */
        if ( delta_h_raw & ((ISP_VSM_DELTA_H_MASK + 1) >> 1) ) {
            delta_h_raw = delta_h_raw | ~ISP_VSM_DELTA_H_MASK;
        }
        /* check highest bit of delta_v slice in register and apply sign
         * extension if applicable */
        if ( delta_v_raw & ((ISP_VSM_DELTA_V_MASK + 1) >> 1) ) {
            delta_v_raw = delta_v_raw | ~ISP_VSM_DELTA_V_MASK;
        }

        ctx->pIspVsmContext->eventData.frameId = currFrameId;
        ctx->pIspVsmContext->eventData.DisplVec.delta_h = (int16_t) (delta_h_raw & 0xFFFF);
        ctx->pIspVsmContext->eventData.DisplVec.delta_v = (int16_t) (delta_v_raw & 0xFFFF);

        TRACE( CAMERIC_ISP_VSM_DRV_INFO, "displ vec: (%+d,%+d)\n",
            ctx->pIspVsmContext->eventData.DisplVec.delta_h,
            ctx->pIspVsmContext->eventData.DisplVec.delta_v);

        if ( ctx->pIspVsmContext->EventCb.func != NULL )
        {
            ctx->pIspVsmContext->EventCb.func( CAMERIC_ISP_EVENT_VSM,
                                              (void *)(&ctx->pIspVsmContext->eventData),
                                              ctx->pIspVsmContext->EventCb.pUserContext );
        }
    }

    TRACE( CAMERIC_ISP_VSM_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );
}



/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspVsmRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspVsmRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void 			    *pUserContext
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_VSM_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspVsmContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)  
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( ctx->pIspVsmContext->EventCb.func != NULL )
    {
        return ( RET_BUSY );
    }

    if ( func == NULL )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        ctx->pIspVsmContext->EventCb.func          = func;
        ctx->pIspVsmContext->EventCb.pUserContext  = pUserContext;
    }

    TRACE( CAMERIC_ISP_VSM_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspVsmDeRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspVsmDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_VSM_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspVsmContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)  
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    ctx->pIspVsmContext->EventCb.func          = NULL;
    ctx->pIspVsmContext->EventCb.pUserContext  = NULL;

    TRACE( CAMERIC_ISP_VSM_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}

/******************************************************************************
 * CamerIcIspVsmEnable()
 *****************************************************************************/
RESULT CamerIcIspVsmEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;

    TRACE( CAMERIC_ISP_VSM_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspVsmContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_vsm_mode;
        uint32_t isp_imsc;

        /* enable measuring module and interrupt */

        isp_vsm_mode = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_vsm_mode) );

        REG_SET_SLICE( isp_vsm_mode, ISP_VSM_MEAS_EN, 1 );
        REG_SET_SLICE( isp_vsm_mode, ISP_VSM_MEAS_IRQ_ENABLE, 1 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_vsm_mode), isp_vsm_mode);

        isp_imsc = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc) );
        isp_imsc |= MRV_ISP_IMSC_VSM_END_MASK;
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc), isp_imsc );

        ctx->pIspVsmContext->enabled = BOOL_TRUE;
    }

    TRACE( CAMERIC_ISP_VSM_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspVsmDisable()
 *****************************************************************************/
RESULT CamerIcIspVsmDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_VSM_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspVsmContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_vsm_mode;
        uint32_t isp_imsc;

        /* enable measuring module and interrupt */

        isp_vsm_mode = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_vsm_mode) );

        REG_SET_SLICE( isp_vsm_mode, ISP_VSM_MEAS_EN, 0 );
        REG_SET_SLICE( isp_vsm_mode, ISP_VSM_MEAS_IRQ_ENABLE, 0 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_vsm_mode), isp_vsm_mode);

        isp_imsc = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc) );
        isp_imsc &= ~MRV_ISP_IMSC_VSM_END_MASK;
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc), isp_imsc );

        ctx->pIspVsmContext->enabled = BOOL_FALSE;
    }

    TRACE( CAMERIC_ISP_VSM_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspVsmIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspVsmIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_VSM_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspVsmContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pIsEnabled )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsEnabled = ctx->pIspVsmContext->enabled;

    TRACE( CAMERIC_ISP_VSM_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspVsmSetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspVsmSetMeasuringWindow
(
    CamerIcDrvHandle_t  handle,
    CamerIcWindow_t    *pMeasureWin,
    uint8_t             horSegments,
    uint8_t             verSegments
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    uint16_t value;

    TRACE( CAMERIC_ISP_VSM_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspVsmContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pMeasureWin == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    if ( (pMeasureWin->hOffset > ISP_VSM_H_OFFSET_MAX) ||
         (pMeasureWin->vOffset > ISP_VSM_V_OFFSET_MAX) )
    {
        return ( RET_OUTOFRANGE );
    }

    if ( (pMeasureWin->width > ISP_VSM_H_SIZE_MAX) ||
         (pMeasureWin->height > ISP_VSM_V_SIZE_MAX) )
    {
        return ( RET_OUTOFRANGE );
    }

    if ( (pMeasureWin->width & 1) ||
         (pMeasureWin->height & 1) ) {
        return ( RET_WRONG_CONFIG );
    }

    if ( (horSegments < ISP_VSM_H_SEGMENTS_MIN) ||
         (horSegments > ISP_VSM_H_SEGMENTS_MAX) ||
         (verSegments < ISP_VSM_V_SEGMENTS_MIN) ||
         (verSegments > ISP_VSM_V_SEGMENTS_MAX) )
    {
        return ( RET_OUTOFRANGE );
    }

    if ( (pMeasureWin->width  <= 8*horSegments) ||
         (pMeasureWin->height <= 8*verSegments) )
    {
        return ( RET_OUTOFRANGE );
    }

    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_vsm_h_offs;
        uint32_t isp_vsm_v_offs;
        uint32_t isp_vsm_h_size;
        uint32_t isp_vsm_v_size;
        uint32_t isp_vsm_h_segments;
        uint32_t isp_vsm_v_segments;

        isp_vsm_h_offs = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_vsm_h_offs) );
        isp_vsm_v_offs = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_vsm_v_offs) );
        isp_vsm_h_size = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_vsm_h_size) );
        isp_vsm_v_size = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_vsm_v_size) );
        isp_vsm_h_segments = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_vsm_h_segments) );
        isp_vsm_v_segments = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_vsm_v_segments) );

        REG_SET_SLICE( isp_vsm_h_offs, ISP_VSM_H_OFFSET, pMeasureWin->hOffset );
        REG_SET_SLICE( isp_vsm_v_offs, ISP_VSM_V_OFFSET, pMeasureWin->vOffset );
        REG_SET_SLICE( isp_vsm_h_size, ISP_VSM_H_SIZE, (pMeasureWin->width >> 1) );
        REG_SET_SLICE( isp_vsm_v_size, ISP_VSM_V_SIZE, (pMeasureWin->height >> 1) );
        REG_SET_SLICE( isp_vsm_h_segments, ISP_VSM_H_SEGMENTS, horSegments );
        REG_SET_SLICE( isp_vsm_v_segments, ISP_VSM_V_SEGMENTS, verSegments );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_vsm_h_offs), isp_vsm_h_offs );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_vsm_v_offs), isp_vsm_v_offs );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_vsm_h_size), isp_vsm_h_size );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_vsm_v_size), isp_vsm_v_size );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_vsm_h_segments), isp_vsm_h_segments );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_vsm_v_segments), isp_vsm_v_segments );

        ctx->pIspVsmContext->MeasureWin = *pMeasureWin;

        ctx->pIspVsmContext->horSegments   = horSegments;
        ctx->pIspVsmContext->verSegments   = verSegments;

    }

    TRACE( CAMERIC_ISP_VSM_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspVsmGetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspVsmGetMeasuringWindow
(
    CamerIcDrvHandle_t  handle,
    CamerIcWindow_t    *pMeasureWin,
    uint8_t            *pHorSegments,
    uint8_t            *pVerSegments
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    uint16_t value;

    TRACE( CAMERIC_ISP_VSM_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspVsmContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (pMeasureWin == NULL) || (pHorSegments == NULL) || (pVerSegments == NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    *pMeasureWin  = ctx->pIspVsmContext->MeasureWin;
    *pHorSegments = ctx->pIspVsmContext->horSegments;
    *pVerSegments = ctx->pIspVsmContext->verSegments;

    TRACE( CAMERIC_ISP_VSM_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



#else  /* #if defined(MRV_VSM_VERSION)  */


/******************************************************************************
 * Vsm module is not available.
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 *
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspVsmRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspVsmRegisterEventCb
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
 * CamerIcIspVsmDeRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspVsmDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspVsmEnable()
 *****************************************************************************/
RESULT CamerIcIspVsmEnable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspVsmDisable()
 *****************************************************************************/
RESULT CamerIcIspVsmDisable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspVsmIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspVsmIsEnabled
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
 * CamerIcIspVsmSetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspVsmSetMeasuringWindow
(
    CamerIcDrvHandle_t  handle,
    CamerIcWindow_t    *pMeasureWin,
    uint8_t             horSegments,
    uint8_t             verSegments
)
{
    (void)handle;
    (void)pMeasureWin;
    (void)horSegments;
    (void)verSegments;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspVsmGetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspVsmGetMeasuringWindow
(
    CamerIcDrvHandle_t  handle,
    CamerIcWindow_t    *pMeasureWin,
    uint8_t            *pHorSegments,
    uint8_t            *pVerSegments
)
{
    (void)handle;
    (void)pMeasureWin;
    (void)pHorSegments;
    (void)pVerSegments;
    return ( RET_NOTSUPP );
}


#endif /* #if defined(MRV_VSM_VERSION)  */

