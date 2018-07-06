/******************************************************************************
 *
 * Copyright 2013, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
/**
 * @cond    cameric_cnr
 *
 * @file    cameric_cnr.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>

#include "mrv_all_bits.h"

#include "cameric_drv_cb.h"
#include "cameric_drv.h"


/******************************************************************************
 * Is the hardware Color noise reduction module available ?
 *****************************************************************************/
#if defined(MRV_CNR_VERSION)

/******************************************************************************
 * Color Noise Reduction Module is available.
 *****************************************************************************/

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_ISP_CNR_DRV_INFO  , "CAMERIC-ISP-CNR-DRV: ", INFO     , 0 );
CREATE_TRACER( CAMERIC_ISP_CNR_DRV_WARN  , "CAMERIC-ISP-CNR-DRV: ", WARNING  , 1 );
CREATE_TRACER( CAMERIC_ISP_CNR_DRV_ERROR , "CAMERIC-ISP-CNR-DRV: ", ERROR    , 1 );


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
 * Implementation of Driver Internal API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspCnrInit()
 *****************************************************************************/
RESULT CamerIcIspCnrInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_CNR_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pIspCnrContext = ( CamerIcIspCnrContext_t *)malloc( sizeof(CamerIcIspCnrContext_t) );
    if (  ctx->pIspCnrContext == NULL )
    {
        TRACE( CAMERIC_ISP_CNR_DRV_ERROR,  "%s: Can't allocate CamerIC ISP CNR context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pIspCnrContext, 0, sizeof(CamerIcIspCnrContext_t) );

    TRACE( CAMERIC_ISP_CNR_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspCnrRelease()
 *****************************************************************************/
RESULT CamerIcIspCnrRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_CNR_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspCnrContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pIspCnrContext, 0, sizeof(CamerIcIspCnrContext_t) );
    free ( ctx->pIspCnrContext );
    ctx->pIspCnrContext = NULL;

    TRACE( CAMERIC_ISP_CNR_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspCnrIsAvailable()
 *****************************************************************************/
RESULT CamerIcIspCnrIsAvailable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_SUCCESS );
}

/******************************************************************************
 * CamerIcIspCnrEnable()
 *****************************************************************************/
RESULT CamerIcIspCnrEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_ISP_CNR_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspCnrContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_INVALID )
    {
        return ( RET_WRONG_STATE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl) );
        REG_SET_SLICE( isp_ctrl, MRV_ISP_CNR_EN, 1U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl), isp_ctrl );
    }

    TRACE( CAMERIC_ISP_CNR_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspCnrDisable()
 *****************************************************************************/
RESULT CamerIcIspCnrDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_CNR_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspCnrContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_INVALID )
    {
        return ( RET_WRONG_STATE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl) );
        REG_SET_SLICE( isp_ctrl, MRV_ISP_CNR_EN, 0U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl), isp_ctrl );
    }

    TRACE( CAMERIC_ISP_CNR_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspCnrIsActivated()
 *****************************************************************************/
RESULT CamerIcIspCnrIsActivated
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;
   
    TRACE( CAMERIC_ISP_CNR_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspCnrContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pIsEnabled )
    {
        return ( RET_NULL_POINTER );
    }

    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_INVALID )
    {
        return ( RET_WRONG_STATE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t isp_ctrl;

        isp_ctrl        = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl) );
        *pIsEnabled = (1UL == REG_GET_SLICE( isp_ctrl, MRV_ISP_CNR_EN ) ) ? BOOL_TRUE : BOOL_FALSE;
    }

    TRACE( CAMERIC_ISP_CNR_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}

/******************************************************************************
 * CamerIcIspCnrSetLineWidth()
 *****************************************************************************/
RESULT CamerIcIspCnrSetLineWidth
(
    CamerIcDrvHandle_t  handle,
    const uint16_t      width
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_ISP_CNR_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspCnrContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_INVALID )
    {
        return ( RET_WRONG_STATE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_cnr_linesize = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cnr_linesize) );
        REG_SET_SLICE( isp_cnr_linesize, MRV_ISP_CNR_LINESIZE, width );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cnr_linesize), isp_cnr_linesize );
    }

    TRACE( CAMERIC_ISP_CNR_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}

/******************************************************************************
 * CamerIcIspCnrGetThresholds()
 *****************************************************************************/
RESULT CamerIcIspCnrGetThresholds
(
    CamerIcDrvHandle_t  handle,
    uint32_t            *pThreshold1,
    uint32_t            *pThreshold2
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_ISP_CNR_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspCnrContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_INVALID )
    {
        return ( RET_WRONG_STATE );
    }

    if ( (NULL == pThreshold1) || (NULL == pThreshold2) )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_cnr_threshold;

        isp_cnr_threshold = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cnr_threshold_c1) );
        *pThreshold1 = REG_GET_SLICE( isp_cnr_threshold, MRV_ISP_CNR_THRESHOLD_C1 );
        
        isp_cnr_threshold = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cnr_threshold_c2) );
        *pThreshold2 = REG_GET_SLICE( isp_cnr_threshold, MRV_ISP_CNR_THRESHOLD_C2 );
    }

    TRACE( CAMERIC_ISP_CNR_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}

/******************************************************************************
 * CamerIcIspCnrSetThresholds()
 *****************************************************************************/
RESULT CamerIcIspCnrSetThresholds
(
    CamerIcDrvHandle_t  handle,
    const uint32_t      threshold1,
    const uint32_t      threshold2
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_ISP_CNR_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspCnrContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_cnr_threshold;

        isp_cnr_threshold = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cnr_threshold_c1) );
        REG_SET_SLICE( isp_cnr_threshold, MRV_ISP_CNR_THRESHOLD_C1, threshold1 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cnr_threshold_c1), isp_cnr_threshold );
        
        isp_cnr_threshold = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cnr_threshold_c2) );
        REG_SET_SLICE( isp_cnr_threshold, MRV_ISP_CNR_THRESHOLD_C2, threshold2 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cnr_threshold_c2), isp_cnr_threshold );
    }

    TRACE( CAMERIC_ISP_CNR_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}

#else  /* #if defined(MRV_CNR_VERSION) */

/******************************************************************************
 * Color Noise Reduction Module is not available.
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 * 
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspCnrIsAvailable()
 *****************************************************************************/
RESULT CamerIcIspCnrIsAvailable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspCnrEnable()
 *****************************************************************************/
RESULT CamerIcIspCnrEnable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspCnrDisable()
 *****************************************************************************/
RESULT CamerIcIspCnrDisable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspCnrIsActivated()
 *****************************************************************************/
RESULT CamerIcIspCnrIsActivated
(
    CamerIcDrvHandle_t  handle,
    bool_t              *pIsEnabled
)
{
    (void)handle;
    (void)pIsEnabled;
    return ( RET_NOTSUPP );
}

/******************************************************************************
 * CamerIcIspCnrSetLineWidth()
 *****************************************************************************/
RESULT CamerIcIspCnrSetLineWidth
(
    CamerIcDrvHandle_t  handle,
    const uint16_t      width
)
{
    (void)handle;
    (void)width;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspCnrGetThresholds()
 *****************************************************************************/
RESULT CamerIcIspCnrGetThresholds
(
    CamerIcDrvHandle_t  handle,
    uint32_t            *threshold1,
    uint32_t            *threshold2
)
{
    (void)handle;
    (void)threshold1;
    (void)threshold2;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspCnrSetThresholds()
 *****************************************************************************/
RESULT CamerIcIspCnrSetThresholds
(
    CamerIcDrvHandle_t  handle,
    const uint32_t      threshold1,
    const uint32_t      threshold2
)
{
    (void)handle;
    (void)threshold1;
    (void)threshold2;
    return ( RET_NOTSUPP );
}



#endif /* #if defined(MRV_CNR_VERSION) */

