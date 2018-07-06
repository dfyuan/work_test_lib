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
 * @file cameric_isp_cac.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <math.h>

#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>

#include "mrv_all_bits.h"

#include "cameric_drv_cb.h"
#include "cameric_drv.h"


/******************************************************************************
 * Is the hardware CAC module available ?
 *****************************************************************************/
#if defined(MRV_CAC_VERSION)

/******************************************************************************
 * Chromatic Aberation Correction Module is available.
 *****************************************************************************/


/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_ISP_CAC_DRV_INFO  , "CAMERIC-ISP-CAC-DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_ISP_CAC_DRV_WARN  , "CAMERIC-ISP-CAC-DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_ISP_CAC_DRV_ERROR , "CAMERIC-ISP-CAC-DRV: ", ERROR   , 1 );

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

/*****************************************************************************/
/**
 * @brief       This function calculates normalization shift parameter 
 *              and factor
 *
 * @param[in]   DistMax maximum distance from the center
 * @param[out]  pCacXNs pointer to normalization shift (output)
 * @param[out]  pCacXNf pointer to normalization factor (output)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 *
 *****************************************************************************/
static RESULT CamerIcIspCacCalcNsNf
(
    const uint32_t  DistMax,
    uint32_t        *pCacXNs,
    uint32_t        *pCacXNf
)
{

    if ( (pCacXNs == NULL) || (pCacXNf == NULL) )
    {
        return ( RET_INVALID_PARM );
    }

    /* calculation: ns = 7 - number of leading zeros of bin(ulDistMax[11:0]) */
    if ( DistMax > 0x7FFU )
    {
        *pCacXNs = 7U;
    }
    else if ( DistMax > 0x3FFU )
    {
        *pCacXNs = 6U;
    }
    else if ( DistMax > 0x1FFU )
    {
        *pCacXNs = 5U;
    }
    else if ( DistMax > 0x0FFU )
    {
        *pCacXNs = 4U;
    }
    else if ( DistMax > 0x07FU ) 
    {
        *pCacXNs = 3U;
    }
    else if ( DistMax > 0x03FU )
    {
        *pCacXNs = 2U;
    }
    else if ( DistMax > 0x01FU )
    {
        *pCacXNs = 1U;
    }
    else
    {
        *pCacXNs = 0U; // ulDistMax < 0x20
    }

    /* normalization factor */
    /* nf = ( 2^(ns+9) - 1 ) / DistMax */
    /* (long values required since 2^(7+9) = 0x10000) */
    *pCacXNf = ( (1UL << (*pCacXNs + 9U)) - 1UL) / DistMax;

    if ( *pCacXNf > MRV_CAC_X_NF_MAX )
    {
        *pCacXNf = MRV_CAC_X_NF_MAX;
    }

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver Internal API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspCacInit()
 *****************************************************************************/
RESULT CamerIcIspCacInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_CAC_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pIspCacContext = ( CamerIcIspCacContext_t *)malloc( sizeof(CamerIcIspCacContext_t) );
    if (  ctx->pIspCacContext == NULL )
    {
        TRACE( CAMERIC_ISP_CAC_DRV_ERROR,  "%s: Can't allocate CamerIC ISP CAC context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pIspCacContext, 0, sizeof(CamerIcIspCacContext_t) );

    ctx->pIspCacContext->enabled = BOOL_FALSE;

    TRACE( CAMERIC_ISP_CAC_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspCacRelease()
 *****************************************************************************/
RESULT CamerIcIspCacRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_CAC_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspCacContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pIspCacContext, 0, sizeof(CamerIcIspCacContext_t) );
    free ( ctx->pIspCacContext );
    ctx->pIspCacContext = NULL;

    TRACE( CAMERIC_ISP_CAC_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspCacConfig()
 *****************************************************************************/
RESULT CamerIcIspCacConfig
(
    CamerIcDrvHandle_t      handle,
    CamerIcIspCacConfig_t   *pConfig
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_CAC_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspCacContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pConfig == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    if ( ctx->pIspCacContext->enabled == BOOL_TRUE )
    {
        return ( RET_WRONG_STATE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_cac_ctrl           = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cac_ctrl) );
        uint32_t isp_cac_count_start    = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cac_count_start) );

        uint32_t isp_cac_a              = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cac_a) );
        uint32_t isp_cac_b              = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cac_b) );
        uint32_t isp_cac_c              = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cac_c) );

        uint32_t isp_cac_x_norm         = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cac_x_norm) );
        uint32_t isp_cac_y_norm         = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cac_y_norm) );
        
        uint32_t h_cnt_start            = ( ((uint32_t)pConfig->width) >> 1 );
        uint32_t v_cnt_start            = ( ((uint32_t)pConfig->height) >> 1 );

        switch ( pConfig->hClipMode )
        {
            case CAMERIC_CAC_H_CLIPMODE_FIX4:
                {
                    REG_SET_SLICE( isp_cac_ctrl, MRV_CAC_H_CLIP_MODE, MRV_CAC_H_CLIP_MODE_FIX4 ); 
                    break;
                }
            case CAMERIC_CAC_H_CLIPMODE_DYN5:
                {
                    REG_SET_SLICE( isp_cac_ctrl, MRV_CAC_H_CLIP_MODE, MRV_CAC_H_CLIP_MODE_DYN5 ); 
                    break;
                }
            default:
                {
                    TRACE( CAMERIC_ISP_CAC_DRV_ERROR,  "%s: invalid CAC horizontal clipping mode (%d)\n",  __FUNCTION__, pConfig->hClipMode );
                    return ( RET_OUTOFRANGE );
                }
        }

        switch ( pConfig->vClipMode )
        {
            case CAMERIC_CAC_V_CLIPMODE_FIX2:
                {
                    REG_SET_SLICE( isp_cac_ctrl, MRV_CAC_V_CLIP_MODE, MRV_CAC_V_CLIP_MODE_FIX2 ); 
                    break;
                }

            case CAMERIC_CAC_V_CLIPMODE_FIX3:
                {
                    REG_SET_SLICE( isp_cac_ctrl, MRV_CAC_V_CLIP_MODE, MRV_CAC_V_CLIP_MODE_FIX3 ); 
                    break;
                }
            case CAMERIC_CAC_V_CLIBMODE_DYN4:
                {
                    REG_SET_SLICE( isp_cac_ctrl, MRV_CAC_V_CLIP_MODE, MRV_CAC_V_CLIP_MODE_DYN4 ); 
                    break;
                }
            default:
                {
                    TRACE( CAMERIC_ISP_CAC_DRV_ERROR,  "%s: invalid CAC vertical clipping mode (%d)\n",  __FUNCTION__, pConfig->hClipMode );
                    return ( RET_OUTOFRANGE );
                }
        }

        h_cnt_start += pConfig->hCenterOffset;
        v_cnt_start += pConfig->vCenterOffset;

        if ( (h_cnt_start < MRV_CAC_H_COUNT_START_MIN) || (h_cnt_start > MRV_CAC_H_COUNT_START_MAX) )
        {
            return ( RET_OUTOFRANGE );
        }

        if( (v_cnt_start < MRV_CAC_V_COUNT_START_MIN) || (v_cnt_start > MRV_CAC_V_COUNT_START_MAX) )
        {
            return ( RET_OUTOFRANGE );
        }

        REG_SET_SLICE( isp_cac_a, MRV_CAC_A_BLUE, pConfig->aBlue );
        REG_SET_SLICE( isp_cac_a, MRV_CAC_A_RED , pConfig->aRed );
        REG_SET_SLICE( isp_cac_b, MRV_CAC_B_BLUE, pConfig->bBlue );
        REG_SET_SLICE( isp_cac_b, MRV_CAC_B_RED , pConfig->bRed );
        REG_SET_SLICE( isp_cac_c, MRV_CAC_C_BLUE, pConfig->cBlue );
        REG_SET_SLICE( isp_cac_c, MRV_CAC_C_RED , pConfig->cRed );
        
        REG_SET_SLICE( isp_cac_x_norm, MRV_CAC_X_NS , pConfig->Xns );
        REG_SET_SLICE( isp_cac_x_norm, MRV_CAC_X_NF , pConfig->Xnf );
        REG_SET_SLICE( isp_cac_y_norm, MRV_CAC_Y_NS , pConfig->Yns );
        REG_SET_SLICE( isp_cac_y_norm, MRV_CAC_Y_NF , pConfig->Ynf );
        
        REG_SET_SLICE( isp_cac_count_start, MRV_CAC_H_COUNT_START, h_cnt_start ); 
        REG_SET_SLICE( isp_cac_count_start, MRV_CAC_V_COUNT_START, v_cnt_start ); 

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cac_ctrl), isp_cac_ctrl );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cac_count_start), isp_cac_count_start );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cac_a), isp_cac_a );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cac_b), isp_cac_b );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cac_c), isp_cac_c );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cac_x_norm), isp_cac_x_norm );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cac_y_norm), isp_cac_y_norm );

        ctx->pIspCacContext->width          = pConfig->width;
        ctx->pIspCacContext->height         = pConfig->height;
        ctx->pIspCacContext->hCenterOffset  = pConfig->hCenterOffset;
        ctx->pIspCacContext->vCenterOffset  = pConfig->vCenterOffset;
        ctx->pIspCacContext->hClipMode      = pConfig->hClipMode;
        ctx->pIspCacContext->vClipMode      = pConfig->vClipMode;
        ctx->pIspCacContext->aBlue          = pConfig->aBlue;
        ctx->pIspCacContext->aRed           = pConfig->aRed;
        ctx->pIspCacContext->bBlue          = pConfig->bBlue;
        ctx->pIspCacContext->bRed           = pConfig->bRed;
        ctx->pIspCacContext->cBlue          = pConfig->cBlue;
        ctx->pIspCacContext->cRed           = pConfig->cRed;

        ctx->pIspCacContext->Xns            = pConfig->Xns;
        ctx->pIspCacContext->Xnf            = pConfig->Xnf;

        ctx->pIspCacContext->Yns            = pConfig->Yns;
        ctx->pIspCacContext->Ynf            = pConfig->Ynf;
    }

    TRACE( CAMERIC_ISP_CAC_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspCacGetConfig()
 *****************************************************************************/
RESULT CamerIcIspCacGetConfig
(
    CamerIcDrvHandle_t      handle,
    CamerIcIspCacConfig_t   *pConfig
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_CAC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspCacContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pConfig )
    {
        return ( RET_NULL_POINTER );
    }

//FIXME read from register
    pConfig->width          = ctx->pIspCacContext->width;
    pConfig->height         = ctx->pIspCacContext->height;
    pConfig->hCenterOffset  = ctx->pIspCacContext->hCenterOffset;
    pConfig->vCenterOffset  = ctx->pIspCacContext->vCenterOffset;
    pConfig->hClipMode      = ctx->pIspCacContext->hClipMode;
    pConfig->vClipMode      = ctx->pIspCacContext->vClipMode;
    pConfig->aBlue          = ctx->pIspCacContext->aBlue;
    pConfig->aRed           = ctx->pIspCacContext->aRed;
    pConfig->bBlue          = ctx->pIspCacContext->bBlue;
    pConfig->bRed           = ctx->pIspCacContext->bRed;
    pConfig->cBlue          = ctx->pIspCacContext->cBlue;
    pConfig->cRed           = ctx->pIspCacContext->cRed;

    pConfig->Xns            = ctx->pIspCacContext->Xns;
    pConfig->Xnf            = ctx->pIspCacContext->Xnf;

    pConfig->Yns            = ctx->pIspCacContext->Yns;
    pConfig->Ynf            = ctx->pIspCacContext->Ynf;

    TRACE( CAMERIC_ISP_CAC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspCacEnable()
 *****************************************************************************/
RESULT CamerIcIspCacEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_hist_prop;

    TRACE( CAMERIC_ISP_CAC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspCacContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_cac_ctrl;
           
        /* enable measuring module */
        isp_cac_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cac_ctrl) );
        REG_SET_SLICE( isp_cac_ctrl, MRV_CAC_CAC_EN, 1 ); 
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cac_ctrl), isp_cac_ctrl);

        ctx->pIspCacContext->enabled = BOOL_TRUE;
    }

    TRACE( CAMERIC_ISP_CAC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspCacDisable()
 *****************************************************************************/
RESULT CamerIcIspCacDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_CAC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspCacContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_cac_ctrl;
           
        /* disable measuring module */
        isp_cac_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cac_ctrl) );
        REG_SET_SLICE( isp_cac_ctrl, MRV_CAC_CAC_EN, 0 ); 
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cac_ctrl), isp_cac_ctrl);

        ctx->pIspCacContext->enabled = BOOL_FALSE;
    }

    TRACE( CAMERIC_ISP_CAC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspCacIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspCacIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_CAC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspCacContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pIsEnabled )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsEnabled = ctx->pIspCacContext->enabled;

    TRACE( CAMERIC_ISP_CAC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


#else  /* #if defined(MRV_CAC_VERSION) */

/******************************************************************************
 * Chromatic Aberation Correction Module is not available.
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 * 
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspCacConfig()
 *****************************************************************************/
RESULT CamerIcIspCacConfig
(
    CamerIcDrvHandle_t      handle,
    CamerIcIspCacConfig_t   *pConfig
)
{
    (void)handle;
    (void)pConfig;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspCacEnable()
 *****************************************************************************/
RESULT CamerIcIspCacEnable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}

/******************************************************************************
 * CamerIcIspCacDisable()
 *****************************************************************************/
RESULT CamerIcIspCacDisable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspCacIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspCacIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
)
{
    (void)handle;
    (void)pIsEnabled;
    return ( RET_NOTSUPP );
}


#endif /* #if defined(MRV_CAC_VERSION) */

