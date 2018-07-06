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
 * @file    cameric_isp_elawb.c
 *
 * @brief   This file implements the driver external and internal API.
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/list.h>
#include <common/return_codes.h>
#include <common/picture_buffer.h>

#include "mrv_all_bits.h"

#include "cameric_drv_cb.h"
#include "cameric_drv.h"


/******************************************************************************
 * Is the hardware eliptic WB measuring module available ?
 *****************************************************************************/
#if defined(MRV_ELAWB_VERSION)

/******************************************************************************
 * eliptic WB measuring module is available.
 *****************************************************************************/


/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_ISP_ELAWB_DRV_INFO  , "CAMERIC-ISP-ELAWB-DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_ISP_ELAWB_DRV_WARN  , "CAMERIC-ISP-ELAWB-DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_ISP_ELAWB_DRV_ERROR , "CAMERIC-ISP-ELAWB-DRV: ", ERROR   , 1 );


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
 * Implementation of Driver Internal API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspElAwbInit()
 *****************************************************************************/
RESULT CamerIcIspElAwbInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pIspElAwbContext = ( CamerIcIspElAwbContext_t *)malloc( sizeof(CamerIcIspElAwbContext_t) );
    if (  ctx->pIspElAwbContext == NULL )
    {
        TRACE( CAMERIC_ISP_ELAWB_DRV_ERROR,  "%s: Can't allocate CamerIC ISP HIST context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pIspElAwbContext, 0, sizeof(CamerIcIspElAwbContext_t) );

    ctx->pIspElAwbContext->enabled      = BOOL_FALSE;
    ctx->pIspElAwbContext->MedianFilter = BOOL_FALSE;

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAwbRelease()
 *****************************************************************************/
RESULT CamerIcIspElAwbRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspElAwbContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pIspElAwbContext, 0, sizeof(CamerIcIspElAwbContext_t) );
    free ( ctx->pIspElAwbContext);
    ctx->pIspElAwbContext = NULL;

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspElAwbSignal()
 *****************************************************************************/
void CamerIcIspElAwbSignal
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspElAwbContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return;
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        if ( ctx->pIspElAwbContext->EventCb.func != NULL )
        {
            ctx->pIspElAwbContext->EventCb.func( CAMERIC_ISP_EVENT_AWB,
                    (void *)(NULL), ctx->pIspElAwbContext->EventCb.pUserContext );
        }
    }

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );
}



/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspElAwbRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspElAwbRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void                *pUserContext
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspElAwbContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)  
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( ctx->pIspElAwbContext->EventCb.func != NULL )
    {
        return ( RET_BUSY );
    }

    if ( func == NULL )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        ctx->pIspElAwbContext->EventCb.func          = func;
        ctx->pIspElAwbContext->EventCb.pUserContext  = pUserContext;
    }

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspElAwbDeRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspElAwbDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspElAwbContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)  
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    ctx->pIspElAwbContext->EventCb.func          = NULL;
    ctx->pIspElAwbContext->EventCb.pUserContext  = NULL;

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspElAwbEnable()
 *****************************************************************************/
RESULT CamerIcIspElAwbEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_hist_prop;

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspElAwbContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t awb_meas_mode;
        uint32_t isp_imsc;

        /* enable measuring module */
        awb_meas_mode = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_mode) );
        REG_SET_SLICE( awb_meas_mode, ISP_AWB_MEAS_IRQ_ENABLE, ISP_AWB_MEAS_IRQ_ENABLE_ON ); 
        REG_SET_SLICE( awb_meas_mode, ISP_AWB_MEAS_EN, ISP_AWB_MEAS_EN_ON ); 
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_mode), awb_meas_mode);

        /* enable interrupt */
        isp_imsc = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc) );
        isp_imsc |= MRV_ISP_IMSC_AWB_DONE_MASK;
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc), isp_imsc );

        ctx->pIspElAwbContext->enabled = BOOL_TRUE;
    }

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspElAwbDisable()
 *****************************************************************************/
RESULT CamerIcIspElAwbDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspElAwbContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t awb_meas_mode;
        uint32_t isp_imsc;

        /* disable measuring module */
        awb_meas_mode = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_mode) );
        REG_SET_SLICE( awb_meas_mode, ISP_AWB_MEAS_IRQ_ENABLE, ISP_AWB_MEAS_IRQ_ENABLE_OFF ); 
        REG_SET_SLICE( awb_meas_mode, ISP_AWB_MEAS_EN, ISP_AWB_MEAS_EN_OFF ); 
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_mode), awb_meas_mode);

        /* disable interrupt */
        isp_imsc = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc) );
        isp_imsc &= ~MRV_ISP_IMSC_AWB_DONE_MASK;
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc), isp_imsc );

        ctx->pIspElAwbContext->enabled = BOOL_FALSE;
    }

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspElAwbIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspElAwbIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspElAwbContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pIsEnabled )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsEnabled = ctx->pIspElAwbContext->enabled;

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspElAwbMedianFilter()
 *****************************************************************************/
RESULT CamerIcIspElAwbMedianFilter
(
    CamerIcDrvHandle_t  handle,
    const bool_t        on
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->pIspElAwbContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t awb_meas_mode;
        uint32_t isp_imsc;

        /* enable measuring module */
        awb_meas_mode = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_mode) );
        REG_SET_SLICE( awb_meas_mode, ISP_AWB_PRE_FILT_EN,
                        ((BOOL_TRUE == on) ? ISP_AWB_PRE_FILT_EN_ON : ISP_AWB_PRE_FILT_EN_OFF) ); 
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_mode), awb_meas_mode);

        ctx->pIspElAwbContext->MedianFilter = on;
    }

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspElAwbMedianFilterEnabled()
 *****************************************************************************/
RESULT CamerIcIspElAwbMedianFilterEnabled
(
    CamerIcDrvHandle_t  handle,
    bool_t              *pOn
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->pIspElAwbContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pOn != NULL )
    {
        *pOn = ctx->pIspElAwbContext->MedianFilter;
    }
    else
    {
        result = RET_INVALID_PARM;
    }

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamerIcIspElAwbSetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspElAwbSetMeasuringWindow
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

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspElAwbContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (ISP_AWB_H_OFFSET_MAX < x)
            || (ISP_AWB_V_OFFSET_MAX < y) )
    {
        return ( RET_OUTOFRANGE );
    }

    if ( (ISP_AWB_H_SIZE_MAX < width) 
            || (ISP_AWB_V_SIZE_MAX < height) )
    {
        return ( RET_OUTOFRANGE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_h_offs), x );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_v_offs), y );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_h_size), width );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_v_size), height );

        ctx->pIspElAwbContext->Window.hOffset = x;
        ctx->pIspElAwbContext->Window.vOffset = y;
        ctx->pIspElAwbContext->Window.width   = width;
        ctx->pIspElAwbContext->Window.height  = height;
    }

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspElAwbGetMeasuringEllipse()
 *****************************************************************************/
RESULT CamerIcIspElAwbGetMeasuringEllipse
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspElAwbElipseId_t id,
    uint16_t                        *x,
    uint16_t                        *y,
    uint16_t                        *a1,
    uint16_t                        *a2,
    uint16_t                        *a3,
    uint16_t                        *a4,
    uint32_t                        *r_max_sqr
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspElAwbContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (x==NULL) || (y==NULL) || (a1==NULL) || (a2==NULL) || (a3==NULL) || (a4==NULL) || (r_max_sqr==NULL) )
    {
        return ( RET_INVALID_PARM );
    }

    if ( (id > CAMERIC_ISP_AWB_ELIPSIS_ID_INVALID)
            && (id < CAMERIC_ISP_AWB_ELIPSIS_ID_MAX) )
    {
        CamerIcIspElAwbElipse_t *pEllipsis = &ctx->pIspElAwbContext->Elipsis[id];

        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint16_t x_         = 0U;
        uint16_t y_         = 0U;
        uint16_t a1_        = 0U;
        uint16_t a2_        = 0U;
        uint16_t a3_        = 0U;
        uint16_t a4_        = 0U;
        uint32_t r_max_sqr_ = 0U;

        switch ( id ) 
        {
            case CAMERIC_ISP_AWB_ELIPSIS_ID_1:
                {
                    x_  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip1_cen_x) );
                    y_  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip1_cen_y) );
                    
                    a1_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip1_a1) );
                    a2_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip1_a2) );
                    a3_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip1_a3) );
                    a4_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip1_a4) );
                    
                    r_max_sqr_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip1_rmax) );
                    break;
                }

            case CAMERIC_ISP_AWB_ELIPSIS_ID_2:
                {
                    x_  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip2_cen_x) );
                    y_  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip2_cen_y) );
                    
                    a1_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip2_a1) );
                    a2_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip2_a2) );
                    a3_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip2_a3) );
                    a4_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip2_a4) );
                    
                    r_max_sqr_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip2_rmax) );
                    break;
                }

            case CAMERIC_ISP_AWB_ELIPSIS_ID_3:
                {
                    x_  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip3_cen_x) );
                    y_  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip3_cen_y) );
                    
                    a1_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip3_a1) );
                    a2_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip3_a2) );
                    a3_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip3_a3) );
                    a4_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip3_a4) );
                    
                    r_max_sqr_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip3_rmax) );
                    break;
                }

            case CAMERIC_ISP_AWB_ELIPSIS_ID_4:
                {
                    x_  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip4_cen_x) );
                    y_  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip4_cen_y) );
                    
                    a1_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip4_a1) );
                    a2_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip4_a2) );
                    a3_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip4_a3) );
                    a4_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip4_a4) );
                    
                    r_max_sqr_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip4_rmax) );
                    break;
                }

            case CAMERIC_ISP_AWB_ELIPSIS_ID_5:
                {
                    x_  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip5_cen_x) );
                    y_  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip5_cen_y) );
                    
                    a1_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip5_a1) );
                    a2_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip5_a2) );
                    a3_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip5_a3) );
                    a4_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip5_a4) );
                    
                    r_max_sqr_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip5_rmax) );
                    break;
                }

            case CAMERIC_ISP_AWB_ELIPSIS_ID_6:
                {
                    x_  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip6_cen_x) );
                    y_  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip6_cen_y) );
                    
                    a1_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip6_a1) );
                    a2_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip6_a2) );
                    a3_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip6_a3) );
                    a4_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip6_a4) );
                    
                    r_max_sqr_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip6_rmax) );
                    break;
                }

            case CAMERIC_ISP_AWB_ELIPSIS_ID_7:
                {
                    x_  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip7_cen_x) );
                    y_  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip7_cen_y) );
                    
                    a1_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip7_a1) );
                    a2_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip7_a2) );
                    a3_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip7_a3) );
                    a4_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip7_a4) );
                    
                    r_max_sqr_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip7_rmax) );
                    break;
                }

            case CAMERIC_ISP_AWB_ELIPSIS_ID_8:
                {
                    x_  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip8_cen_x) );
                    y_  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip8_cen_y) );
                    
                    a1_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip8_a1) );
                    a2_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip8_a2) );
                    a3_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip8_a3) );
                    a4_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip8_a4) );
                    
                    r_max_sqr_ = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip8_rmax) );
                    break;
                }

            default:
                {
                    return ( RET_OUTOFRANGE );
                }
        }

        *x   = x_;
        *y   = y_;
        
        *a1  = a1_;
        *a2  = a2_;
        *a3  = a3_;
        *a4  = a4_;

        *r_max_sqr = r_max_sqr_;
    }

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspElAwbSetMeasuringEllipse()
 *****************************************************************************/
RESULT CamerIcIspElAwbSetMeasuringEllipse
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspElAwbElipseId_t id,
    const uint16_t                  x,
    const uint16_t                  y,
    const uint16_t                  a1,
    const uint16_t                  a2,
    const uint16_t                  a3,
    const uint16_t                  a4,
    const uint32_t                  r_max_sqr
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspElAwbContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (id > CAMERIC_ISP_AWB_ELIPSIS_ID_INVALID)
            && (id < CAMERIC_ISP_AWB_ELIPSIS_ID_MAX) )
    {
        CamerIcIspElAwbElipse_t *pEllipsis = &ctx->pIspElAwbContext->Elipsis[id];

        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        switch ( id ) 
        {
            case CAMERIC_ISP_AWB_ELIPSIS_ID_1:
                {
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip1_cen_x), x );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip1_cen_y), y );
                    
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip1_a1), a1 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip1_a2), a2 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip1_a3), a3 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip1_a4), a4 );
                    
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip1_rmax), r_max_sqr );
                    break;
                }

            case CAMERIC_ISP_AWB_ELIPSIS_ID_2:
                {
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip2_cen_x), x );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip2_cen_y), y );
                    
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip2_a1), a1 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip2_a2), a2 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip2_a3), a3 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip2_a4), a4 );
                    
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip2_rmax), r_max_sqr );
                    break;
                }

            case CAMERIC_ISP_AWB_ELIPSIS_ID_3:
                {
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip3_cen_x), x );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip3_cen_y), y );
                    
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip3_a1), a1 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip3_a2), a2 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip3_a3), a3 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip3_a4), a4 );
                    
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip3_rmax), r_max_sqr );
                    break;
                }

            case CAMERIC_ISP_AWB_ELIPSIS_ID_4:
                {
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip4_cen_x), x );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip4_cen_y), y );
                    
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip4_a1), a1 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip4_a2), a2 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip4_a3), a3 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip4_a4), a4 );
                    
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip4_rmax), r_max_sqr );
                    break;
                }

            case CAMERIC_ISP_AWB_ELIPSIS_ID_5:
                {
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip5_cen_x), x );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip5_cen_y), y );
                    
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip5_a1), a1 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip5_a2), a2 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip5_a3), a3 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip5_a4), a4 );
                    
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip5_rmax), r_max_sqr );
                    break;
                }

            case CAMERIC_ISP_AWB_ELIPSIS_ID_6:
                {
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip6_cen_x), x );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip6_cen_y), y );
                    
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip6_a1), a1 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip6_a2), a2 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip6_a3), a3 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip6_a4), a4 );
                    
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip6_rmax), r_max_sqr );
                    break;
                }

            case CAMERIC_ISP_AWB_ELIPSIS_ID_7:
                {
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip7_cen_x), x );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip7_cen_y), y );
                    
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip7_a1), a1 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip7_a2), a2 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip7_a3), a3 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip7_a4), a4 );
                    
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip7_rmax), r_max_sqr );
                    break;
                }

            case CAMERIC_ISP_AWB_ELIPSIS_ID_8:
                {
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip7_cen_x), x );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip7_cen_y), y );
                    
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip7_a1), a1 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip7_a2), a2 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip7_a3), a3 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip7_a4), a4 );
                    
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->awb_meas_ellip8_rmax), r_max_sqr );
                    break;
                }

            default:
                {
                    return ( RET_OUTOFRANGE );
                }
        }

        pEllipsis->x = x;
        pEllipsis->y = y;
        
        pEllipsis->a1 = a1;
        pEllipsis->a2 = a2;
        pEllipsis->a3 = a3;
        pEllipsis->a4 = a4;

        pEllipsis->r_max_sqr = r_max_sqr;
    }

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspElAwbGetGains()
 *****************************************************************************/
RESULT CamerIcIspElAwbGetGains
(
    CamerIcDrvHandle_t  handle,
    CamerIcGains_t      *pGains
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspElAwbContext == NULL) || (ctx->HalHandle == NULL) )
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

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAwbSetGains()
 *****************************************************************************/
RESULT CamerIcIspElAwbSetGains
(
    CamerIcDrvHandle_t      handle,
    const CamerIcGains_t    *pGains
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspElAwbContext == NULL) || (ctx->HalHandle == NULL) )
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

    TRACE( CAMERIC_ISP_ELAWB_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


#else  /* #if defined(MRV_ELAWB_VERSION) */

/******************************************************************************
 * eliptic WB measuring module is not available.
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 * 
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspElAwbRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspElAwbRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void                *pUserContext
)
{
    (void)handle;
    (void)func;
    (void)pUserContext;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspElAwbDeRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspElAwbDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspElAwbEnable()
 *****************************************************************************/
RESULT CamerIcIspElAwbEnable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspElAwbDisable()
 *****************************************************************************/
RESULT CamerIcIspElAwbDisable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspElAwbIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspElAwbIsEnabled
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
 * CamerIcIspElAwbMedianFilter()
 *****************************************************************************/
RESULT CamerIcIspElAwbMedianFilter
(
    CamerIcDrvHandle_t  handle,
    const bool_t        on
)
{
    (void)handle;
    (void)on;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspElAwbMedianFilterEnabled()
 *****************************************************************************/
RESULT CamerIcIspElAwbMedianFilterEnabled
(
    CamerIcDrvHandle_t  handle,
    bool_t              *pOn
)
{
    (void)handle;
    (void)pOn;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspElAwbSetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspElAwbSetMeasuringWindow
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
 * CamerIcIspElAwbGetMeasuringEllipse()
 *****************************************************************************/
RESULT CamerIcIspElAwbGetMeasuringEllipse
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspElAwbElipseId_t id,
    uint16_t                        *x,
    uint16_t                        *y,
    uint16_t                        *a1,
    uint16_t                        *a2,
    uint16_t                        *a3,
    uint16_t                        *a4,
    uint32_t                        *r_max_sqr
)
{
    (void)handle;
    (void)id;
    (void)x;
    (void)y;
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    (void)r_max_sqr;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspElAwbSetMeasuringEllipse()
 *****************************************************************************/
RESULT CamerIcIspElAwbSetMeasuringEllipse
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspElAwbElipseId_t id,
    const uint16_t                  x,
    const uint16_t                  y,
    const uint16_t                  a1,
    const uint16_t                  a2,
    const uint16_t                  a3,
    const uint16_t                  a4,
    const uint32_t                  r_max_sqr
)
{
    (void)handle;
    (void)id;
    (void)x;
    (void)y;
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    (void)r_max_sqr;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspElAwbGetGains()
 *****************************************************************************/
RESULT CamerIcIspElAwbGetGains
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
RESULT CamerIcIspElAwbSetGains
(
    CamerIcDrvHandle_t      handle,
    const CamerIcGains_t    *pGains
)
{
    (void)handle;
    (void)pGains;
    return ( RET_NOTSUPP );
}

#endif /* #if defined(MRV_ELAWB_VERSION) */

