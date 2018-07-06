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
 * @file cameric_isp_flt.c
 *
 * @brief
 *   Implementation of FLT Module.
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include "mrv_all_bits.h"

#include "cameric_drv_cb.h"
#include "cameric_drv.h"



/******************************************************************************
 * Is the hardware ISP Filter module available ?
 *****************************************************************************/
#if defined(MRV_FILTER_VERSION)

/******************************************************************************
 * Auto Focus Module is available.
 *****************************************************************************/

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_ISP_FLT_DRV_INFO  , "CAMERIC_ISP_FLT_DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_ISP_FLT_DRV_WARN  , "CAMERIC_ISP_FLT_DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_ISP_FLT_DRV_ERROR , "CAMERIC_ISP_FLT_DRV: ", ERROR   , 1 );


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
 * CamerIcIspFltInit()
 *****************************************************************************/
RESULT CamerIcIspFltInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_FLT_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pIspFltContext = ( CamerIcIspFltContext_t *)malloc( sizeof(CamerIcIspFltContext_t) );
    if (  ctx->pIspFltContext == NULL )
    {
        TRACE( CAMERIC_ISP_FLT_DRV_ERROR,  "%s: Can't allocate CamerIC ISP FLT context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pIspFltContext, 0, sizeof( CamerIcIspFltContext_t ) );

    /* initialize mode control & luminance weight function register */
    volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_filt_mode)        , 0x00000000UL);
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_filt_lum_weight)  , 0x00032040UL);

    ctx->pIspFltContext->enabled = BOOL_FALSE;

    /* initialize filter parameter register */
    CamerIcIspFltSetFilterParameter( handle, CAMERIC_ISP_FLT_DENOISE_LEVEL_2, CAMERIC_ISP_FLT_SHARPENING_LEVEL_3 );

    TRACE( CAMERIC_ISP_FLT_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspFltRelease()
 *****************************************************************************/
RESULT CamerIcIspFltRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_FLT_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspFltContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pIspFltContext, 0, sizeof( CamerIcIspFltContext_t ) );
    free ( ctx->pIspFltContext );
    ctx->pIspFltContext = NULL;

    TRACE( CAMERIC_ISP_FLT_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspFltEnable()
 *****************************************************************************/
RESULT CamerIcIspFltEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_FLT_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspFltContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_flt_mode = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_filt_mode) );
        REG_SET_SLICE( isp_flt_mode, MRV_FILT_FILT_ENABLE, 1U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_filt_mode), isp_flt_mode );

        ctx->pIspFltContext->enabled = BOOL_TRUE;
    }

    TRACE( CAMERIC_ISP_FLT_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspFltDisable()
 *****************************************************************************/
RESULT CamerIcIspFltDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_FLT_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspFltContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_flt_mode = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_filt_mode) );
        REG_SET_SLICE( isp_flt_mode, MRV_FILT_FILT_ENABLE, 0U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_filt_mode), isp_flt_mode );

        ctx->pIspFltContext->enabled = BOOL_FALSE;
    }

    TRACE( CAMERIC_ISP_FLT_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspFltIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspFltIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_FLT_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspFltContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pIsEnabled )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsEnabled = ctx->pIspFltContext->enabled;

    TRACE( CAMERIC_ISP_FLT_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspFltGetFilterParameter()
 *****************************************************************************/
RESULT CamerIcIspFltGetFilterParameter
(
    CamerIcDrvHandle_t              handle,
    CamerIcIspFltDeNoiseLevel_t     *pDeNoiseLevel,
    CamerIcIspFltSharpeningLevel_t  *pSharpeningLevel
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_FLT_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspFltContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (NULL == pDeNoiseLevel) || (NULL == pSharpeningLevel) )
    {
        return ( RET_NULL_POINTER );
    }

    *pDeNoiseLevel    = ctx->pIspFltContext->DeNoiseLevel;
    *pSharpeningLevel = ctx->pIspFltContext->SharpeningLevel;

    TRACE( CAMERIC_ISP_FLT_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspFltSetFilterParameter()
 *****************************************************************************/
RESULT CamerIcIspFltSetFilterParameter
(
    CamerIcDrvHandle_t                      handle,
    const CamerIcIspFltDeNoiseLevel_t       DeNoiseLevel,
    const CamerIcIspFltSharpeningLevel_t    SharpeningLevel
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;
    
    volatile MrvAllRegister_t *ptCamerIcRegMap;

    uint32_t isp_filt_thresh_sh0 = 0U;
    uint32_t isp_filt_thresh_sh1 = 0U;
    uint32_t isp_filt_thresh_bl0 = 0U;
    uint32_t isp_filt_thresh_bl1 = 0U;

    uint32_t isp_filt_fac_sh0 = 0U;
    uint32_t isp_filt_fac_sh1 = 0U;
    uint32_t isp_filt_fac_mid = 0U;
    uint32_t isp_filt_fac_bl0 = 0U;
    uint32_t isp_filt_fac_bl1 = 0U;

    uint32_t isp_filt_mode    = 0UL;
    
    TRACE( CAMERIC_ISP_FLT_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->pIspFltContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    isp_filt_mode = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_filt_mode) );

    switch ( DeNoiseLevel )
    {

        case CAMERIC_ISP_FLT_DENOISE_LEVEL_0:
            {
                /* NoiseReductionLevel = 0 */
                isp_filt_thresh_sh0 = (MRV_FILT_FILT_THRESH_SH0_MASK & 0);
                isp_filt_thresh_sh1 = (MRV_FILT_FILT_THRESH_SH1_MASK & 0);
                isp_filt_thresh_bl0 = (MRV_FILT_FILT_THRESH_BL0_MASK & 0);
                isp_filt_thresh_bl1 = (MRV_FILT_FILT_THRESH_BL1_MASK & 0);

                REG_SET_SLICE( isp_filt_mode, MRV_FILT_STAGE1_SELECT, 6 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_V_MODE, MRV_FILT_FILT_CHR_V_MODE_STATIC8 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_H_MODE, MRV_FILT_FILT_CHR_H_MODE_BYPASS );

                break;
            }

        case CAMERIC_ISP_FLT_DENOISE_LEVEL_1:
            {
                /* NoiseReductionLevel = 1 */
                isp_filt_thresh_sh0 = (MRV_FILT_FILT_THRESH_SH0_MASK & 18);
                isp_filt_thresh_sh1 = (MRV_FILT_FILT_THRESH_SH1_MASK & 33);
                isp_filt_thresh_bl0 = (MRV_FILT_FILT_THRESH_BL0_MASK &  8);
                isp_filt_thresh_bl1 = (MRV_FILT_FILT_THRESH_BL1_MASK &  2);

                REG_SET_SLICE( isp_filt_mode, MRV_FILT_STAGE1_SELECT, 6 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_V_MODE, MRV_FILT_FILT_CHR_V_MODE_STATIC12 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_H_MODE, MRV_FILT_FILT_CHR_H_MODE_DYN_2 );

                break;
            }

        case CAMERIC_ISP_FLT_DENOISE_LEVEL_2:
            {
                /* NoiseReductionLevel = 2 */
                isp_filt_thresh_sh0 = (MRV_FILT_FILT_THRESH_SH0_MASK & 26);
                isp_filt_thresh_sh1 = (MRV_FILT_FILT_THRESH_SH1_MASK & 44);
                isp_filt_thresh_bl0 = (MRV_FILT_FILT_THRESH_BL0_MASK & 13);
                isp_filt_thresh_bl1 = (MRV_FILT_FILT_THRESH_BL1_MASK &  5);

                REG_SET_SLICE( isp_filt_mode, MRV_FILT_STAGE1_SELECT, 4 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_V_MODE, MRV_FILT_FILT_CHR_V_MODE_STATIC12 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_H_MODE, MRV_FILT_FILT_CHR_H_MODE_DYN_2 );

                break;
            }

        case CAMERIC_ISP_FLT_DENOISE_LEVEL_3:
            {
                /* NoiseReductionLevel = 3; */
                isp_filt_thresh_sh0 = (MRV_FILT_FILT_THRESH_SH0_MASK & 36);
                isp_filt_thresh_sh1 = (MRV_FILT_FILT_THRESH_SH1_MASK & 51);
                isp_filt_thresh_bl0 = (MRV_FILT_FILT_THRESH_BL0_MASK & 23);
                isp_filt_thresh_bl1 = (MRV_FILT_FILT_THRESH_BL1_MASK & 10);

                REG_SET_SLICE( isp_filt_mode, MRV_FILT_STAGE1_SELECT, 2 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_V_MODE, MRV_FILT_FILT_CHR_V_MODE_STATIC12 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_H_MODE, MRV_FILT_FILT_CHR_H_MODE_DYN_2 );

                break;
            }

        case CAMERIC_ISP_FLT_DENOISE_LEVEL_4:
            {
                /* NoiseReductionLevel = 4; */
                isp_filt_thresh_sh0 = (MRV_FILT_FILT_THRESH_SH0_MASK & 41);
                isp_filt_thresh_sh1 = (MRV_FILT_FILT_THRESH_SH1_MASK & 67);
                isp_filt_thresh_bl0 = (MRV_FILT_FILT_THRESH_BL0_MASK & 26);
                isp_filt_thresh_bl1 = (MRV_FILT_FILT_THRESH_BL1_MASK & 15);

                REG_SET_SLICE( isp_filt_mode, MRV_FILT_STAGE1_SELECT, 3 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_V_MODE, MRV_FILT_FILT_CHR_V_MODE_STATIC12 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_H_MODE, MRV_FILT_FILT_CHR_H_MODE_DYN_2 );

                break;
            }

        case CAMERIC_ISP_FLT_DENOISE_LEVEL_5:
            {
                /* NoiseReductionLevel = 5; */
                isp_filt_thresh_sh0 = (MRV_FILT_FILT_THRESH_SH0_MASK &  75);
                isp_filt_thresh_sh1 = (MRV_FILT_FILT_THRESH_SH1_MASK & 100);
                isp_filt_thresh_bl0 = (MRV_FILT_FILT_THRESH_BL0_MASK &  50);
                isp_filt_thresh_bl1 = (MRV_FILT_FILT_THRESH_BL1_MASK &  20);

                REG_SET_SLICE( isp_filt_mode, MRV_FILT_STAGE1_SELECT, 3 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_V_MODE, MRV_FILT_FILT_CHR_V_MODE_STATIC12 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_H_MODE, MRV_FILT_FILT_CHR_H_MODE_DYN_2 );

                break;
            }

        case CAMERIC_ISP_FLT_DENOISE_LEVEL_6:
            {
                /* NoiseReductionLevel = 6; */
                isp_filt_thresh_sh0 = (MRV_FILT_FILT_THRESH_SH0_MASK &  90);
                isp_filt_thresh_sh1 = (MRV_FILT_FILT_THRESH_SH1_MASK & 120);
                isp_filt_thresh_bl0 = (MRV_FILT_FILT_THRESH_BL0_MASK &  60);
                isp_filt_thresh_bl1 = (MRV_FILT_FILT_THRESH_BL1_MASK &  26);

                REG_SET_SLICE( isp_filt_mode, MRV_FILT_STAGE1_SELECT, 2 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_V_MODE, MRV_FILT_FILT_CHR_V_MODE_STATIC12 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_H_MODE, MRV_FILT_FILT_CHR_H_MODE_DYN_2 );

                break;
            }

        case CAMERIC_ISP_FLT_DENOISE_LEVEL_7:
            {
                /* NoiseReductionLevel = 7; */
                isp_filt_thresh_sh0 = (MRV_FILT_FILT_THRESH_SH0_MASK & 120);
                isp_filt_thresh_sh1 = (MRV_FILT_FILT_THRESH_SH1_MASK & 150);
                isp_filt_thresh_bl0 = (MRV_FILT_FILT_THRESH_BL0_MASK &  80);
                isp_filt_thresh_bl1 = (MRV_FILT_FILT_THRESH_BL1_MASK &  51);

                REG_SET_SLICE( isp_filt_mode, MRV_FILT_STAGE1_SELECT, 2 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_V_MODE, MRV_FILT_FILT_CHR_V_MODE_STATIC12 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_H_MODE, MRV_FILT_FILT_CHR_H_MODE_DYN_2 );

                break;
            }

        case CAMERIC_ISP_FLT_DENOISE_LEVEL_8:
            {
                /* NoiseReductionLevel = 8; */
                isp_filt_thresh_sh0 = (MRV_FILT_FILT_THRESH_SH0_MASK & 170);
                isp_filt_thresh_sh1 = (MRV_FILT_FILT_THRESH_SH1_MASK & 200);
                isp_filt_thresh_bl0 = (MRV_FILT_FILT_THRESH_BL0_MASK & 140);
                isp_filt_thresh_bl1 = (MRV_FILT_FILT_THRESH_BL1_MASK & 100);

                REG_SET_SLICE( isp_filt_mode, MRV_FILT_STAGE1_SELECT, 2 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_V_MODE, MRV_FILT_FILT_CHR_V_MODE_STATIC12 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_H_MODE, MRV_FILT_FILT_CHR_H_MODE_DYN_2 );

                break;
            }

        case CAMERIC_ISP_FLT_DENOISE_LEVEL_9:
            {
                /* NoiseReductionLevel = 9; */
                isp_filt_thresh_sh0 = (MRV_FILT_FILT_THRESH_SH0_MASK & 250);
                isp_filt_thresh_sh1 = (MRV_FILT_FILT_THRESH_SH1_MASK & 300);
                isp_filt_thresh_bl0 = (MRV_FILT_FILT_THRESH_BL0_MASK & 180);
                isp_filt_thresh_bl1 = (MRV_FILT_FILT_THRESH_BL1_MASK & 150);

                REG_SET_SLICE( isp_filt_mode, MRV_FILT_STAGE1_SELECT,
                        ((SharpeningLevel > CAMERIC_ISP_FLT_SHARPENING_LEVEL_3) ? 2 : 1) );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_V_MODE, MRV_FILT_FILT_CHR_V_MODE_STATIC12 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_H_MODE, MRV_FILT_FILT_CHR_H_MODE_DYN_2 );

                break;
            }

        case CAMERIC_ISP_FLT_DENOISE_LEVEL_10:
            {
                /* NoiseReductionLevel = 10; extrem noise */
                isp_filt_thresh_sh0 = (MRV_FILT_FILT_THRESH_SH0_MASK & 1023);
                isp_filt_thresh_sh1 = (MRV_FILT_FILT_THRESH_SH1_MASK & 1023);
                isp_filt_thresh_bl0 = (MRV_FILT_FILT_THRESH_BL0_MASK & 1023);
                isp_filt_thresh_bl1 = (MRV_FILT_FILT_THRESH_BL1_MASK & 1023);

                REG_SET_SLICE( isp_filt_mode, MRV_FILT_STAGE1_SELECT,
                        ((SharpeningLevel > CAMERIC_ISP_FLT_SHARPENING_LEVEL_5) ? 2 : ((SharpeningLevel > CAMERIC_ISP_FLT_SHARPENING_LEVEL_3) ? 1 : 0)) );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_V_MODE, MRV_FILT_FILT_CHR_V_MODE_STATIC12 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_H_MODE, MRV_FILT_FILT_CHR_H_MODE_DYN_2 );

                break;
            }

        case CAMERIC_ISP_FLT_DENOISE_LEVEL_TEST:
            {
                /* TEST MODE (don't use) */
                isp_filt_thresh_sh0 = (MRV_FILT_FILT_THRESH_SH0_MASK & 0x03FF);
                isp_filt_thresh_sh1 = (MRV_FILT_FILT_THRESH_SH1_MASK & 0x03FF);
                isp_filt_thresh_bl0 = (MRV_FILT_FILT_THRESH_BL0_MASK & 0x03FF);
                isp_filt_thresh_bl1 = (MRV_FILT_FILT_THRESH_BL1_MASK & 0x03FF);

                REG_SET_SLICE( isp_filt_mode, MRV_FILT_STAGE1_SELECT, 0 );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_V_MODE, MRV_FILT_FILT_CHR_V_MODE_BYPASS );
                REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_CHR_H_MODE, MRV_FILT_FILT_CHR_H_MODE_BYPASS );

                break;
            }

        default:
            {
                return ( RET_OUTOFRANGE );
            }
    }

    switch ( SharpeningLevel )
    {

        case CAMERIC_ISP_FLT_SHARPENING_LEVEL_0:
            {
                /* SharpeningLevel = 0; no sharp enhancement */
                isp_filt_fac_sh0 = (MRV_FILT_FILT_FAC_SH0_MASK & 0x00000004);
                isp_filt_fac_sh1 = (MRV_FILT_FILT_FAC_SH1_MASK & 0x00000004);
                isp_filt_fac_mid = (MRV_FILT_FILT_FAC_MID_MASK & 0x00000004);
                isp_filt_fac_bl0 = (MRV_FILT_FILT_FAC_BL0_MASK & 0x00000002);
                isp_filt_fac_bl1 = (MRV_FILT_FILT_FAC_BL1_MASK & 0x00000000);

                break;
            }

        case CAMERIC_ISP_FLT_SHARPENING_LEVEL_1:
            {
                /* SharpeningLevel = 1; */
                isp_filt_fac_sh0 = (MRV_FILT_FILT_FAC_SH0_MASK & 0x00000007);
                isp_filt_fac_sh1 = (MRV_FILT_FILT_FAC_SH1_MASK & 0x00000008);
                isp_filt_fac_mid = (MRV_FILT_FILT_FAC_MID_MASK & 0x00000006);
                isp_filt_fac_bl0 = (MRV_FILT_FILT_FAC_BL0_MASK & 0x00000002);
                isp_filt_fac_bl1 = (MRV_FILT_FILT_FAC_BL1_MASK & 0x00000000);

                break;
            }

        case CAMERIC_ISP_FLT_SHARPENING_LEVEL_2:
            {
                /* SharpeningLevel = 2; */
                isp_filt_fac_sh0 = (MRV_FILT_FILT_FAC_SH0_MASK & 0x0000000A);
                isp_filt_fac_sh1 = (MRV_FILT_FILT_FAC_SH1_MASK & 0x0000000C);
                isp_filt_fac_mid = (MRV_FILT_FILT_FAC_MID_MASK & 0x00000008);
                isp_filt_fac_bl0 = (MRV_FILT_FILT_FAC_BL0_MASK & 0x00000004);
                isp_filt_fac_bl1 = (MRV_FILT_FILT_FAC_BL1_MASK & 0x00000000);

                break;
            }

        case CAMERIC_ISP_FLT_SHARPENING_LEVEL_3:
            {
                /* SharpeningLevel = 3; */
                isp_filt_fac_sh0 = (MRV_FILT_FILT_FAC_SH0_MASK & 0x0000000C);
                isp_filt_fac_sh1 = (MRV_FILT_FILT_FAC_SH1_MASK & 0x00000010);
                isp_filt_fac_mid = (MRV_FILT_FILT_FAC_MID_MASK & 0x0000000A);
                isp_filt_fac_bl0 = (MRV_FILT_FILT_FAC_BL0_MASK & 0x00000006);
                isp_filt_fac_bl1 = (MRV_FILT_FILT_FAC_BL1_MASK & 0x00000002);

                break;
            }

        case CAMERIC_ISP_FLT_SHARPENING_LEVEL_4:
            {
                /* SharpeningLevel = 4; */
                isp_filt_fac_sh0 = (MRV_FILT_FILT_FAC_SH0_MASK & 0x00000010);
                isp_filt_fac_sh1 = (MRV_FILT_FILT_FAC_SH1_MASK & 0x00000016);
                isp_filt_fac_mid = (MRV_FILT_FILT_FAC_MID_MASK & 0x0000000C);
                isp_filt_fac_bl0 = (MRV_FILT_FILT_FAC_BL0_MASK & 0x00000008);
                isp_filt_fac_bl1 = (MRV_FILT_FILT_FAC_BL1_MASK & 0x00000004);

                break;
            }

        case CAMERIC_ISP_FLT_SHARPENING_LEVEL_5:
            {
                /* SharpeningLevel = 5; */
                isp_filt_fac_sh0 = (MRV_FILT_FILT_FAC_SH0_MASK & 0x00000014);
                isp_filt_fac_sh1 = (MRV_FILT_FILT_FAC_SH1_MASK & 0x0000001B);
                isp_filt_fac_mid = (MRV_FILT_FILT_FAC_MID_MASK & 0x00000010);
                isp_filt_fac_bl0 = (MRV_FILT_FILT_FAC_BL0_MASK & 0x0000000A);
                isp_filt_fac_bl1 = (MRV_FILT_FILT_FAC_BL1_MASK & 0x00000004);

                break;
            }

        case CAMERIC_ISP_FLT_SHARPENING_LEVEL_6:
            {
                /* SharpeningLevel = 6; */
                isp_filt_fac_sh0 = (MRV_FILT_FILT_FAC_SH0_MASK & 0x0000001A);
                isp_filt_fac_sh1 = (MRV_FILT_FILT_FAC_SH1_MASK & 0x00000020);
                isp_filt_fac_mid = (MRV_FILT_FILT_FAC_MID_MASK & 0x00000013);
                isp_filt_fac_bl0 = (MRV_FILT_FILT_FAC_BL0_MASK & 0x0000000C);
                isp_filt_fac_bl1 = (MRV_FILT_FILT_FAC_BL1_MASK & 0x00000006);

                break;
            }

        case CAMERIC_ISP_FLT_SHARPENING_LEVEL_7:
            {
                /* SharpeningLevel = 7; */
                isp_filt_fac_sh0 = (MRV_FILT_FILT_FAC_SH0_MASK & 0x0000001E);
                isp_filt_fac_sh1 = (MRV_FILT_FILT_FAC_SH1_MASK & 0x00000026);
                isp_filt_fac_mid = (MRV_FILT_FILT_FAC_MID_MASK & 0x00000017);
                isp_filt_fac_bl0 = (MRV_FILT_FILT_FAC_BL0_MASK & 0x00000010);
                isp_filt_fac_bl1 = (MRV_FILT_FILT_FAC_BL1_MASK & 0x00000008);

                break;
            }

        case CAMERIC_ISP_FLT_SHARPENING_LEVEL_8:
            {
                /* SharpeningLevel = 8; */
                isp_filt_thresh_sh0 = (MRV_FILT_FILT_THRESH_SH0_MASK & 0x00000013);

                if ( (isp_filt_thresh_sh1 & MRV_FILT_FILT_THRESH_SH1_MASK) > 0x0000008A )
                {
                    isp_filt_thresh_sh1 = (MRV_FILT_FILT_THRESH_SH1_MASK & 0x0000008A);
                }

                isp_filt_fac_sh1 = (MRV_FILT_FILT_FAC_SH1_MASK & 0x0000002C);
                isp_filt_fac_sh0 = (MRV_FILT_FILT_FAC_SH0_MASK & 0x00000024);
                isp_filt_fac_mid = (MRV_FILT_FILT_FAC_MID_MASK & 0x0000001D);
                isp_filt_fac_bl0 = (MRV_FILT_FILT_FAC_BL0_MASK & 0x00000015);
                isp_filt_fac_bl1 = (MRV_FILT_FILT_FAC_BL1_MASK & 0x0000000D);

                break;
            }

        case CAMERIC_ISP_FLT_SHARPENING_LEVEL_9:
            {
                /* SharpeningLevel = 9; */
                isp_filt_thresh_sh0 = (MRV_FILT_FILT_THRESH_SH0_MASK & 0x00000013);

                if ( (isp_filt_thresh_sh1 & MRV_FILT_FILT_THRESH_SH1_MASK) > 0x0000008A )
                {
                    isp_filt_thresh_sh1 = (MRV_FILT_FILT_THRESH_SH1_MASK & 0x0000008A);
                }

                isp_filt_fac_sh1 = (MRV_FILT_FILT_FAC_SH1_MASK & 0x00000030);
                isp_filt_fac_sh0 = (MRV_FILT_FILT_FAC_SH0_MASK & 0x0000002A);
                isp_filt_fac_mid = (MRV_FILT_FILT_FAC_MID_MASK & 0x00000022);
                isp_filt_fac_bl0 = (MRV_FILT_FILT_FAC_BL0_MASK & 0x0000001A);
                isp_filt_fac_bl1 = (MRV_FILT_FILT_FAC_BL1_MASK & 0x00000014);

                break;
            }

        case CAMERIC_ISP_FLT_SHARPENING_LEVEL_10:
            {
                /* SharpeningLevel = 10; */
                isp_filt_fac_sh1 = (MRV_FILT_FILT_FAC_SH1_MASK & 0x0000003F);
                isp_filt_fac_sh0 = (MRV_FILT_FILT_FAC_SH0_MASK & 0x00000030);
                isp_filt_fac_mid = (MRV_FILT_FILT_FAC_MID_MASK & 0x00000028);
                isp_filt_fac_bl0 = (MRV_FILT_FILT_FAC_BL0_MASK & 0x00000024);
                isp_filt_fac_bl1 = (MRV_FILT_FILT_FAC_BL1_MASK & 0x00000020);

                break;
            }

        default:
            {
                return ( RET_OUTOFRANGE );
            }
    }

    if ( DeNoiseLevel > CAMERIC_ISP_FLT_DENOISE_LEVEL_7 )
    {
        if ( SharpeningLevel > CAMERIC_ISP_FLT_SHARPENING_LEVEL_7 )
        {
            isp_filt_fac_bl0 = (MRV_FILT_FILT_FAC_BL0_MASK & (isp_filt_fac_bl0 >> 1)); // * 0.50
            isp_filt_fac_bl1 = (MRV_FILT_FILT_FAC_BL1_MASK & (isp_filt_fac_bl1 >> 2)); // * 0.25
        }
        else if ( SharpeningLevel > CAMERIC_ISP_FLT_SHARPENING_LEVEL_4 )
        {
            isp_filt_fac_bl0 = (MRV_FILT_FILT_FAC_BL0_MASK & ((isp_filt_fac_bl0 * 3) >> 2)); // * 0.75
            isp_filt_fac_bl1 = (MRV_FILT_FILT_FAC_BL1_MASK & (isp_filt_fac_bl1       >> 2)); // * 0.50
        }
    }

    bool_t restart = BOOL_FALSE;
    if ( ctx->pIspFltContext->enabled )
    {
        CamerIcIspFltDisable( handle );
        restart = BOOL_TRUE;
    }

    /* set ISP filter parameter register values */
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_filt_thresh_sh0), isp_filt_thresh_sh0 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_filt_thresh_sh1), isp_filt_thresh_sh1 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_filt_thresh_bl0), isp_filt_thresh_bl0 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_filt_thresh_bl1), isp_filt_thresh_bl1 );

    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_filt_fac_sh0), isp_filt_fac_sh0 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_filt_fac_sh1), isp_filt_fac_sh1 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_filt_fac_mid), isp_filt_fac_mid );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_filt_fac_bl0), isp_filt_fac_bl0 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_filt_fac_bl1), isp_filt_fac_bl1 );

    /* set ISP filter mode register values */
    REG_SET_SLICE( isp_filt_mode, MRV_FILT_FILT_MODE, MRV_FILT_FILT_MODE_DYNAMIC );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_filt_mode), isp_filt_mode );

    ctx->pIspFltContext->DeNoiseLevel    = DeNoiseLevel;
    ctx->pIspFltContext->SharpeningLevel = SharpeningLevel;

    if ( BOOL_TRUE == restart )
    {
        CamerIcIspFltEnable( handle );
    }

    TRACE( CAMERIC_ISP_FLT_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


#else  /* #if defined(MRV_FILTER_VERSION) */

/******************************************************************************
 * Auto Focus Module is not available.
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 * 
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspFltEnable()
 *****************************************************************************/
RESULT CamerIcIspFltEnable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspFltDisable()
 *****************************************************************************/
RESULT CamerIcIspFltDisable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspFltIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspFltIsEnabled
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
 * CamerIcIspFltGetFilterParameter()
 *****************************************************************************/
RESULT CamerIcIspFltGetFilterParameter
(
    CamerIcDrvHandle_t              handle,
    CamerIcIspFltDeNoiseLevel_t     *pDeNoiseLevel,
    CamerIcIspFltSharpeningLevel_t  *pSharpeningLevel
)
{
    (void)handle;
    (void)pDeNoiseLevel;
    (void)pSharpeningLevel;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspFltSetFilterParameter()
 *****************************************************************************/
RESULT CamerIcIspFltSetFilterParameter
(
    CamerIcDrvHandle_t                      handle,
    const CamerIcIspFltDeNoiseLevel_t       DeNoiseLevel,
    const CamerIcIspFltSharpeningLevel_t    SharpeningLevel
)
{
    (void)handle;
    (void)DeNoiseLevel;
    (void)SharpeningLevel;
    return ( RET_NOTSUPP );
}

#endif /* #if defined(MRV_FILTER_VERSION) */

