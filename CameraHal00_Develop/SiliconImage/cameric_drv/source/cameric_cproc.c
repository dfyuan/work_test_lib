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
 * @cond    cameric_cproc
 *
 * @file    cameric_cproc.c
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
 * Is the hardware Color processing module available ?
 *****************************************************************************/
#if defined(MRV_COLOR_PROCESSING_VERSION)

/******************************************************************************
 * Color Processing Module is available.
 *****************************************************************************/

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_CPROC_DRV_INFO  , "CAMERIC_CPROC_DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_CPROC_DRV_WARN  , "CAMERIC_CPROC_DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_CPROC_DRV_ERROR , "CAMERIC_CPROC_DRV: ", ERROR   , 1 );

/******************************************************************************
 * local type definitions
 *****************************************************************************/

/******************************************************************************
 * local variable declarations
 *****************************************************************************/

/******************************************************************************
 * local function prototypes
 *****************************************************************************/

/******************************************************************************/
/**
 * @brief   This functions resets the CamerIC color processing module.
 *
 * @param   ctx                 Driver contecxt for getting HalHandle to access
 *                              hardware
 * @param   stay_in_reset       if TRUE than the IE hardware module stays in
 *                              reset state
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
static RESULT CamerIcCprocReset
(
    CamerIcDrvContext_t *ctx,
    const bool_t        stay_in_reset
)
{
    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t vi_ircl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_ircl) );
        REG_SET_SLICE( vi_ircl, MRV_VI_CP_SOFT_RST, 1 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_ircl), vi_ircl );

        if ( BOOL_FALSE == stay_in_reset )
        {
            REG_SET_SLICE( vi_ircl, MRV_VI_CP_SOFT_RST, 0 );
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_ircl), vi_ircl );
        }
    }

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver Internal API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcCprocIsAvailable()
 *****************************************************************************/
RESULT CamerIcCprocIsAvailable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcCprocEnableClock()
 *****************************************************************************/
RESULT CamerIcCprocEnableClock
(
    CamerIcDrvHandle_t  handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->HalHandle == NULL) || (ctx->pCprocContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t vi_iccl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_iccl) );

        REG_SET_SLICE( vi_iccl, MRV_VI_CP_CLK_ENABLE, 1 );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_iccl), vi_iccl );

        result = CamerIcCprocReset( ctx, BOOL_FALSE );
    }

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/******************************************************************************
 * CamerIcCprocDisableClock()
 *****************************************************************************/
RESULT CamerIcCprocDisableClock
(
    CamerIcDrvHandle_t  handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t vi_iccl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_iccl) );

        REG_SET_SLICE( vi_iccl, MRV_VI_CP_CLK_ENABLE, 0 );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_iccl), vi_iccl );

        result = CamerIcCprocReset( ctx, BOOL_TRUE );
    }

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/******************************************************************************
 * CamerIcCprocInit()
 *****************************************************************************/
RESULT CamerIcCprocInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pCprocContext = ( CamerIcCprocContext_t *)malloc( sizeof(CamerIcCprocContext_t) );
    if (  ctx->pCprocContext == NULL )
    {
        TRACE( CAMERIC_CPROC_DRV_ERROR,  "%s: Can't allocate CamerIC MIPI context.\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pCprocContext, 0, sizeof(CamerIcCprocContext_t) );

    result = CamerIcCprocEnableClock( ctx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAMERIC_CPROC_DRV_ERROR,  "%s: Failed to enable clock for CamerIC MIPI module.\n",  __FUNCTION__ );
        return ( result );
    }

    ctx->pCprocContext->enabled = BOOL_FALSE;

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/******************************************************************************
 * CamerIcCprocRelease()
 *****************************************************************************/
RESULT CamerIcCprocRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pCprocContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pCprocContext, 0, sizeof(CamerIcCprocContext_t) );
    free ( ctx->pCprocContext );
    ctx->pCprocContext = NULL;

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcCprocEnable()
 *****************************************************************************/
RESULT CamerIcCprocEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pCprocContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t cproc_ctrl         = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_ctrl) );
        uint32_t cproc_contrast     = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_contrast) );
        uint32_t cproc_brightness   = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_brightness) );
        uint32_t cproc_saturation   = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_saturation) );
        uint32_t cproc_hue          = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_hue) );

        switch ( ctx->pCprocContext->config.ChromaOut )
        {
            case CAMERIC_CPROC_CHROM_RANGE_OUT_BT601:
                {
                    REG_SET_SLICE( cproc_ctrl, MRV_CPROC_CPROC_C_OUT_RANGE, MRV_CPROC_CPROC_C_OUT_RANGE_BT601 );
                    break;
                }

            case CAMERIC_CPROC_CHROM_RANGE_OUT_FULL_RANGE:
                {
                    REG_SET_SLICE( cproc_ctrl, MRV_CPROC_CPROC_C_OUT_RANGE, MRV_CPROC_CPROC_C_OUT_RANGE_FULL );
                    break;
                }

            default:
                {
                    return ( RET_NOTSUPP );
                }
        }

        switch ( ctx->pCprocContext->config.LumaIn )
        {
            case CAMERIC_CPROC_LUM_RANGE_IN_BT601:
                {
                    REG_SET_SLICE( cproc_ctrl, MRV_CPROC_CPROC_Y_IN_RANGE, MRV_CPROC_CPROC_Y_IN_RANGE_BT601 );
                    break;
                }

            case CAMERIC_CPROC_LUM_RANGE_IN_FULL_RANGE:
                {
                    REG_SET_SLICE( cproc_ctrl, MRV_CPROC_CPROC_Y_IN_RANGE, MRV_CPROC_CPROC_Y_IN_RANGE_FULL );
                    break;
                }

            default:
                {
                    return ( RET_NOTSUPP );
                }
        }

        switch ( ctx->pCprocContext->config.LumaOut )
        {
            case CAMERIC_CPROC_LUM_RANGE_OUT_BT601:
                {
                    REG_SET_SLICE( cproc_ctrl, MRV_CPROC_CPROC_Y_OUT_RANGE, MRV_CPROC_CPROC_Y_OUT_RANGE_BT601 );
                    break;
                }

            case CAMERIC_CPROC_LUM_RANGE_OUT_FULL_RANGE:
                {
                    REG_SET_SLICE( cproc_ctrl, MRV_CPROC_CPROC_Y_OUT_RANGE, MRV_CPROC_CPROC_Y_OUT_RANGE_FULL );
                    break;
                }

            default:
                {
                    return ( RET_NOTSUPP );
                }
        }

        REG_SET_SLICE( cproc_contrast, MRV_CPROC_CPROC_CONTRAST, ctx->pCprocContext->config.contrast );
        REG_SET_SLICE( cproc_brightness, MRV_CPROC_CPROC_BRIGHTNESS, ctx->pCprocContext->config.brightness );
        REG_SET_SLICE( cproc_saturation, MRV_CPROC_CPROC_SATURATION, ctx->pCprocContext->config.saturation );
        REG_SET_SLICE( cproc_hue, MRV_CPROC_CPROC_HUE, ctx->pCprocContext->config.hue );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_contrast), cproc_contrast);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_brightness), cproc_brightness);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_saturation), cproc_saturation);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_hue), cproc_hue);

        REG_SET_SLICE( cproc_ctrl, MRV_CPROC_CPROC_ENABLE, MRV_CPROC_CPROC_ENABLE_PROCESS );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_ctrl), cproc_ctrl);

        ctx->pCprocContext->enabled = BOOL_TRUE;
    }

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcCprocDisable()
 *****************************************************************************/
RESULT CamerIcCprocDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pCprocContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t cproc_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_ctrl) );

        REG_SET_SLICE( cproc_ctrl, MRV_CPROC_CPROC_ENABLE, MRV_CPROC_CPROC_ENABLE_BYPASS );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_ctrl), cproc_ctrl);

        ctx->pCprocContext->enabled = BOOL_FALSE;
    }

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcCprocIsEnabled()
 *****************************************************************************/
RESULT CamerIcCprocIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pCprocContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pIsEnabled )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsEnabled = ctx->pCprocContext->enabled;

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcCprocConfigure()
 *****************************************************************************/
RESULT CamerIcCprocConfigure
(
    CamerIcDrvHandle_t      handle,
    CamerIcCprocConfig_t    *pConfig
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pCprocContext == NULL) || (ctx->HalHandle == NULL) )
    {
        TRACE( CAMERIC_CPROC_DRV_INFO, "%s: wrong handle \n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

    if ( (pConfig->ChromaOut <= CAMERIC_CPROC_CHROM_RANGE_OUT_INVALID)
            || (pConfig->ChromaOut >= CAMERIC_CPROC_CHROM_RANGE_OUT_MAX) )
    {
        TRACE( CAMERIC_CPROC_DRV_INFO, "%s: unsupported chroma out-range\n", __FUNCTION__);
        return ( RET_NOTSUPP );
    }

    if ( (pConfig->LumaOut <= CAMERIC_CPROC_LUM_RANGE_OUT_INVALID)
            || (pConfig->LumaOut >= CAMERIC_CPROC_LUM_RANGE_OUT_MAX) )
    {
        TRACE( CAMERIC_CPROC_DRV_INFO, "%s: unsupported luma out-range\n", __FUNCTION__);
        return ( RET_NOTSUPP );
    }

    if ( (pConfig->LumaIn <= CAMERIC_CPROC_LUM_RANGE_IN_INVALID)
            || (pConfig->LumaIn >= CAMERIC_CPROC_LUM_RANGE_IN_MAX) )
    {
        TRACE( CAMERIC_CPROC_DRV_INFO, "%s: unsupported luma in-range\n", __FUNCTION__);
        return ( RET_NOTSUPP );
    }

    MEMCPY( &ctx->pCprocContext->config, pConfig, sizeof( CamerIcCprocConfig_t ) );

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcCprocGetContrast()
 *****************************************************************************/
RESULT CamerIcCprocGetContrast
(
    CamerIcDrvHandle_t  handle,
    uint8_t             *contrast
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pCprocContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == contrast )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t cproc_contrast = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_contrast) );

        *contrast = cproc_contrast;
    }

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcCprocSetContrast()
 *****************************************************************************/
RESULT CamerIcCprocSetContrast
(
    CamerIcDrvHandle_t  handle,
    const uint8_t       contrast
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pCprocContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t cproc_contrast = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_contrast) );

        REG_SET_SLICE( cproc_contrast, MRV_CPROC_CPROC_CONTRAST, contrast );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_contrast), cproc_contrast);
    }

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcCprocGetBrightness()
 *****************************************************************************/
RESULT CamerIcCprocGetBrightness
(
    CamerIcDrvHandle_t  handle,
    uint8_t             *brightness
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pCprocContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == brightness )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t cproc_brightness = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_brightness) );

        *brightness = cproc_brightness;
    }

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcCprocSetBrightness()
 *****************************************************************************/
RESULT CamerIcCprocSetBrightness
(
    CamerIcDrvHandle_t  handle,
    const uint8_t       brightness
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pCprocContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t cproc_brightness = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_brightness) );

        REG_SET_SLICE( cproc_brightness, MRV_CPROC_CPROC_BRIGHTNESS, brightness );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_brightness), cproc_brightness);
    }

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcCprocGetSaturation()
 *****************************************************************************/
RESULT CamerIcCprocGetSaturation
(
    CamerIcDrvHandle_t  handle,
    uint8_t             *saturation
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pCprocContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == saturation )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t cproc_saturation = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_saturation) );

        *saturation = cproc_saturation;
    }

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcCprocSetSaturation()
 *****************************************************************************/
RESULT CamerIcCprocSetSaturation
(
    CamerIcDrvHandle_t  handle,
    const uint8_t       saturation
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pCprocContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t cproc_saturation = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_saturation) );

        REG_SET_SLICE( cproc_saturation, MRV_CPROC_CPROC_SATURATION, saturation );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_saturation), cproc_saturation );
    }

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcCprocGetHue()
 *****************************************************************************/
RESULT CamerIcCprocGetHue
(
    CamerIcDrvHandle_t  handle,
    uint8_t             *hue
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pCprocContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == hue )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t cproc_hue = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_hue) );

        *hue = cproc_hue;
    }

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcCprocSetHue()
 *****************************************************************************/
RESULT CamerIcCprocSetHue
(
    CamerIcDrvHandle_t  handle,
    const uint8_t       hue
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pCprocContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t cproc_hue = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_hue) );

        REG_SET_SLICE( cproc_hue, MRV_CPROC_CPROC_HUE, hue );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cproc_hue), cproc_hue );
    }

    TRACE( CAMERIC_CPROC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


#else  /* #if defined(MRV_COLOR_PROCESSING_VERSION) */

/******************************************************************************
 * Color Processing Module is not available.
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 * 
 *****************************************************************************/

/******************************************************************************
 * CamerIcCprocIsAvailable()
 *****************************************************************************/
RESULT CamerIcCprocIsAvailable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcCprocEnable()
 *****************************************************************************/
RESULT CamerIcCprocEnable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcCprocDisable()
 *****************************************************************************/
RESULT CamerIcCprocDisable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcCprocIsEnabled()
 *****************************************************************************/
RESULT CamerIcCprocIsEnabled
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
 * CamerIcCprocConfigure()
 *****************************************************************************/
RESULT CamerIcCprocConfigure
(
    CamerIcDrvHandle_t      handle,
    CamerIcCprocConfig_t    *pConfig
)
{
    (void)handle;
    (void)pConfig;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcCprocGetContrast()
 *****************************************************************************/
RESULT CamerIcCprocGetContrast
(
    CamerIcDrvHandle_t  handle,
    uint8_t             *contrast
)
{
    (void)handle;
    (void)contrast;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcCprocSetContrast()
 *****************************************************************************/
RESULT CamerIcCprocSetContrast
(
    CamerIcDrvHandle_t  handle,
    const uint8_t       contrast
)
{
    (void)handle;
    (void)contrast;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcCprocGetBrightness()
 *****************************************************************************/
RESULT CamerIcCprocGetBrightness
(
    CamerIcDrvHandle_t  handle,
    uint8_t             *brightness
)
{
    (void)handle;
    (void)brightness;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcCprocSetBrightness()
 *****************************************************************************/
RESULT CamerIcCprocSetBrightness
(
    CamerIcDrvHandle_t  handle,
    const uint8_t       brightness
)
{
    (void)handle;
    (void)brightness;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcCprocGetSaturation()
 *****************************************************************************/
RESULT CamerIcCprocGetSaturation
(
    CamerIcDrvHandle_t  handle,
    uint8_t             *saturation
)
{
    (void)handle;
    (void)saturation;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcCprocSetSaturation()
 *****************************************************************************/
RESULT CamerIcCprocSetSaturation
(
    CamerIcDrvHandle_t  handle,
    const uint8_t       saturation
)
{
    (void)handle;
    (void)saturation;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcCprocGetHue()
 *****************************************************************************/
RESULT CamerIcCprocGetHue
(
    CamerIcDrvHandle_t  handle,
    uint8_t             *hue
)
{
    (void)handle;
    (void)hue;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcCprocSetHue()
 *****************************************************************************/
RESULT CamerIcCprocSetHue
(
    CamerIcDrvHandle_t  handle,
    const uint8_t       hue
)
{
    (void)handle;
    (void)hue;
    return ( RET_NOTSUPP );
}


#endif /* #if defined(MRV_COLOR_PROCESSING_VERSION) */

