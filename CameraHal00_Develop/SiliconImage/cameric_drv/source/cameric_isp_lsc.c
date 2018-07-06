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
 * @file cameric_isp_lsc.c
 *
 * @brief
 *   Implementation of LSC Module.
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include "mrv_all_bits.h"

#include "cameric_drv_cb.h"
#include "cameric_drv.h"


/******************************************************************************
 * Is LSC Module available ?
 *****************************************************************************/
#if defined(MRV_LSC_VERSION)

/******************************************************************************
 * Lense Shade Correction Module is available.
 *****************************************************************************/


/******************************************************************************
 * local macro definitions
 *****************************************************************************/

CREATE_TRACER( CAMERIC_ISP_LSC_DRV_INFO  , "CAMERIC_ISP_LSC_DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_ISP_LSC_DRV_WARN  , "CAMERIC_ISP_LSC_DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_ISP_LSC_DRV_ERROR , "CAMERIC_ISP_LSC_DRV: ", ERROR   , 1 );

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
 * CamerIcIspLscInit()
 *****************************************************************************/
RESULT CamerIcIspLscInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_LSC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pIspLscContext = ( CamerIcIspLscContext_t *)malloc( sizeof(CamerIcIspLscContext_t) );
    if (  ctx->pIspLscContext == NULL )
    {
        TRACE( CAMERIC_ISP_LSC_DRV_ERROR,  "%s: Can't allocate CamerIC ISP LSC context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pIspLscContext, 0, sizeof( CamerIcIspLscContext_t ) );

    ctx->pIspLscContext->enabled = BOOL_FALSE;

    TRACE( CAMERIC_ISP_LSC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspLscRelease()
 *****************************************************************************/
RESULT CamerIcIspLscRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_LSC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspLscContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pIspLscContext, 0, sizeof( CamerIcIspLscContext_t ) );
    free ( ctx->pIspLscContext );
    ctx->pIspLscContext = NULL;

    TRACE( CAMERIC_ISP_LSC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspLscEnable()
 *****************************************************************************/
RESULT CamerIcIspLscEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_LSC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspLscContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_lsc_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ctrl) );
        REG_SET_SLICE( isp_lsc_ctrl, MRV_LSC_LSC_EN, 1U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ctrl), isp_lsc_ctrl );

        ctx->pIspLscContext->enabled = BOOL_TRUE;
    }

    TRACE( CAMERIC_ISP_LSC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspLscDisable()
 *****************************************************************************/
RESULT CamerIcIspLscDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_LSC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspLscContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_lsc_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ctrl) );
        REG_SET_SLICE( isp_lsc_ctrl, MRV_LSC_LSC_EN, 0U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ctrl), isp_lsc_ctrl );

        ctx->pIspLscContext->enabled = BOOL_FALSE;
    }

    TRACE( CAMERIC_ISP_LSC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspLscIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspLscIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_LSC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspLscContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pIsEnabled )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsEnabled = ctx->pIspLscContext->enabled;

    TRACE( CAMERIC_ISP_LSC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspLscGetLenseShadeCorrection()
 *****************************************************************************/
RESULT CamerIcIspLscGetLenseShadeCorrection
(
    CamerIcDrvHandle_t      handle,
    CamerIcIspLscConfig_t   *pLscConfig
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_LSC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->pIspLscContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pLscConfig )
    {
        return ( RET_NULL_POINTER );
    }

    volatile MrvAllRegister_t *ptCamerIcRegMap  = (MrvAllRegister_t *)ctx->base;

    uint32_t isp_lsc_ctrl = 0UL;
    uint32_t data = 0UL;
    uint32_t isp_lsc_status     = 0U;
    uint32_t sram_addr          = 0U;

    uint32_t i;
    uint32_t n;

    isp_lsc_status = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_status) );
    sram_addr = ( isp_lsc_status & 0x2U ) ? 153U : 0U;  /* ( 17 * 18 ) >> 1 */

    /* clear/reset address counters */
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_r_table_addr),    sram_addr);
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_gr_table_addr),   sram_addr);
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_gb_table_addr),   sram_addr);
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_b_table_addr),    sram_addr);

    for (n = 0; n < ((CAMERIC_MAX_LSC_SECTORS + 1) * (CAMERIC_MAX_LSC_SECTORS + 1)); n += CAMERIC_MAX_LSC_SECTORS + 1) // 17 steps
    {
        /* 17 sectors with 2 values in one DWORD = 9 DWORDs (8 steps + 1 outside loop) */
        for (i = 0; i < (CAMERIC_MAX_LSC_SECTORS); i += 2)
        {
            /* read into temp variable because only single              */
            /* register access is allowed (auto-increment registers)    */
            data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_r_table_data) );
            pLscConfig->LscRDataTbl[n+i]   = REG_GET_SLICE(data, MRV_LSC_R_SAMPLE_0);
            pLscConfig->LscRDataTbl[n+i+1] = REG_GET_SLICE(data, MRV_LSC_R_SAMPLE_1);

            data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_gr_table_data) );
            pLscConfig->LscGRDataTbl[n+i]   = REG_GET_SLICE(data, MRV_LSC_GR_SAMPLE_0);
            pLscConfig->LscGRDataTbl[n+i+1] = REG_GET_SLICE(data, MRV_LSC_GR_SAMPLE_1);

            data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_gb_table_data) );
            pLscConfig->LscGBDataTbl[n+i]   = REG_GET_SLICE(data, MRV_LSC_GB_SAMPLE_0);
            pLscConfig->LscGBDataTbl[n+i+1] = REG_GET_SLICE(data, MRV_LSC_GB_SAMPLE_1);

            data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_b_table_data) );
            pLscConfig->LscBDataTbl[n+i]   = REG_GET_SLICE(data, MRV_LSC_B_SAMPLE_0);
            pLscConfig->LscBDataTbl[n+i+1] = REG_GET_SLICE(data, MRV_LSC_B_SAMPLE_1);
        }

        data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_r_table_data) );
        pLscConfig->LscRDataTbl[n+CAMERIC_MAX_LSC_SECTORS] = REG_GET_SLICE(data, MRV_LSC_R_SAMPLE_0);

        data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_gr_table_data) );
        pLscConfig->LscGRDataTbl[n+CAMERIC_MAX_LSC_SECTORS] = REG_GET_SLICE(data, MRV_LSC_GR_SAMPLE_0);

        data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_gb_table_data) );
        pLscConfig->LscGBDataTbl[n+CAMERIC_MAX_LSC_SECTORS] = REG_GET_SLICE(data, MRV_LSC_GB_SAMPLE_0);

        data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_b_table_data) );
        pLscConfig->LscBDataTbl[n+CAMERIC_MAX_LSC_SECTORS] = REG_GET_SLICE(data, MRV_LSC_B_SAMPLE_0);
    }

    /* read x size tables */
    data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xsize_01) );
    pLscConfig->LscXSizeTbl[0] = REG_GET_SLICE(data, MRV_LSC_X_SECT_SIZE_0);
    pLscConfig->LscXSizeTbl[1] = REG_GET_SLICE(data, MRV_LSC_X_SECT_SIZE_1);
    data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xsize_23) );
    pLscConfig->LscXSizeTbl[2] = REG_GET_SLICE(data, MRV_LSC_X_SECT_SIZE_2);
    pLscConfig->LscXSizeTbl[3] = REG_GET_SLICE(data, MRV_LSC_X_SECT_SIZE_3);
    data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xsize_45) );
    pLscConfig->LscXSizeTbl[4] = REG_GET_SLICE(data, MRV_LSC_X_SECT_SIZE_4);
    pLscConfig->LscXSizeTbl[5] = REG_GET_SLICE(data, MRV_LSC_X_SECT_SIZE_5);
    data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xsize_67) );
    pLscConfig->LscXSizeTbl[6] = REG_GET_SLICE(data, MRV_LSC_X_SECT_SIZE_6);
    pLscConfig->LscXSizeTbl[7] = REG_GET_SLICE(data, MRV_LSC_X_SECT_SIZE_7);

    /* read y size tables */
    data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ysize_01) );
    pLscConfig->LscYSizeTbl[0] = REG_GET_SLICE(data, MRV_LSC_Y_SECT_SIZE_0);
    pLscConfig->LscYSizeTbl[1] = REG_GET_SLICE(data, MRV_LSC_Y_SECT_SIZE_1);
    data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ysize_23) );
    pLscConfig->LscYSizeTbl[2] = REG_GET_SLICE(data, MRV_LSC_Y_SECT_SIZE_2);
    pLscConfig->LscYSizeTbl[3] = REG_GET_SLICE(data, MRV_LSC_Y_SECT_SIZE_3);
    data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ysize_45) );
    pLscConfig->LscYSizeTbl[4] = REG_GET_SLICE(data, MRV_LSC_Y_SECT_SIZE_4);
    pLscConfig->LscYSizeTbl[5] = REG_GET_SLICE(data, MRV_LSC_Y_SECT_SIZE_5);
    data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ysize_67) );
    pLscConfig->LscYSizeTbl[6] = REG_GET_SLICE(data, MRV_LSC_Y_SECT_SIZE_6);
    pLscConfig->LscYSizeTbl[7] = REG_GET_SLICE(data, MRV_LSC_Y_SECT_SIZE_7);

    /* read x grad tables */
    data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xgrad_01) );
    pLscConfig->LscXGradTbl[0] = REG_GET_SLICE(data, MRV_LSC_XGRAD_0);
    pLscConfig->LscXGradTbl[1] = REG_GET_SLICE(data, MRV_LSC_XGRAD_1);
    data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xgrad_23) );
    pLscConfig->LscXGradTbl[2] = REG_GET_SLICE(data, MRV_LSC_XGRAD_2);
    pLscConfig->LscXGradTbl[3] = REG_GET_SLICE(data, MRV_LSC_XGRAD_3);
    data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xgrad_45) );
    pLscConfig->LscXGradTbl[4] = REG_GET_SLICE(data, MRV_LSC_XGRAD_4);
    pLscConfig->LscXGradTbl[5] = REG_GET_SLICE(data, MRV_LSC_XGRAD_5);
    data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xgrad_67) );
    pLscConfig->LscXGradTbl[6] = REG_GET_SLICE(data, MRV_LSC_XGRAD_6);
    pLscConfig->LscXGradTbl[7] = REG_GET_SLICE(data, MRV_LSC_XGRAD_7);

    /* read y grad tables */
    data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ygrad_01) );
    pLscConfig->LscYGradTbl[0] = REG_GET_SLICE(data, MRV_LSC_YGRAD_0);
    pLscConfig->LscYGradTbl[1] = REG_GET_SLICE(data, MRV_LSC_YGRAD_1);
    data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ygrad_23) );
    pLscConfig->LscYGradTbl[2] = REG_GET_SLICE(data, MRV_LSC_YGRAD_2);
    pLscConfig->LscYGradTbl[3] = REG_GET_SLICE(data, MRV_LSC_YGRAD_3);
    data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ygrad_45) );
    pLscConfig->LscYGradTbl[4] = REG_GET_SLICE(data, MRV_LSC_YGRAD_4);
    pLscConfig->LscYGradTbl[5] = REG_GET_SLICE(data, MRV_LSC_YGRAD_5);
    data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ygrad_67) );
    pLscConfig->LscYGradTbl[6] = REG_GET_SLICE(data, MRV_LSC_YGRAD_6);
    pLscConfig->LscYGradTbl[7] = REG_GET_SLICE(data, MRV_LSC_YGRAD_7);

    TRACE( CAMERIC_ISP_LSC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspLscSetLenseShadeCorrection()
 *****************************************************************************/
RESULT CamerIcIspLscSetLenseShadeCorrection
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspLscConfig_t     *pLscConfig
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_LSC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->pIspLscContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL != pLscConfig )
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        bool_t restart = BOOL_FALSE;

        uint32_t i;
        uint32_t n;

        uint32_t data = 0;

        /* disable lense shade correction before programming */
        if ( ctx->pIspLscContext->enabled )
        {
            CamerIcIspLscDisable( handle );
            restart = BOOL_TRUE;
        }

        /* clear/reset address counters */
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_r_table_addr), 0);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_gr_table_addr), 0);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_gb_table_addr), 0);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_b_table_addr), 0);

        /* program data tables (table size is 9 * 17 = 153) */
        for (n = 0; n < ((CAMERIC_MAX_LSC_SECTORS + 1) * (CAMERIC_MAX_LSC_SECTORS + 1)); n += CAMERIC_MAX_LSC_SECTORS + 1) // 17 steps
        {
            /* 17 sectors with 2 values in one DWORD = 9 DWORDs (8 steps + 1 outside loop) */
            for (i = 0; i < (CAMERIC_MAX_LSC_SECTORS); i += 2)
            {
                REG_SET_SLICE(data, MRV_LSC_R_SAMPLE_0, pLscConfig->LscRDataTbl[n+i]);
                REG_SET_SLICE(data, MRV_LSC_R_SAMPLE_1, pLscConfig->LscRDataTbl[n+i+1]);
                HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_r_table_data), data);

                REG_SET_SLICE(data, MRV_LSC_GR_SAMPLE_0, pLscConfig->LscGRDataTbl[n+i]);
                REG_SET_SLICE(data, MRV_LSC_GR_SAMPLE_1, pLscConfig->LscGRDataTbl[n+i+1]);
                HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_gr_table_data), data);

                REG_SET_SLICE(data, MRV_LSC_GB_SAMPLE_0, pLscConfig->LscGBDataTbl[n+i]);
                REG_SET_SLICE(data, MRV_LSC_GB_SAMPLE_1, pLscConfig->LscGBDataTbl[n+i+1]);
                HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_gb_table_data), data);

                REG_SET_SLICE(data, MRV_LSC_B_SAMPLE_0, pLscConfig->LscBDataTbl[n+i]);
                REG_SET_SLICE(data, MRV_LSC_B_SAMPLE_1, pLscConfig->LscBDataTbl[n+i+1]);
                HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_b_table_data), data);
            }

            REG_SET_SLICE(data, MRV_LSC_R_SAMPLE_0, pLscConfig->LscRDataTbl[n+CAMERIC_MAX_LSC_SECTORS]);
            REG_SET_SLICE(data, MRV_LSC_R_SAMPLE_1, 0U);
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_r_table_data), data);

            REG_SET_SLICE(data, MRV_LSC_GR_SAMPLE_0, pLscConfig->LscGRDataTbl[n+CAMERIC_MAX_LSC_SECTORS]);
            REG_SET_SLICE(data, MRV_LSC_GR_SAMPLE_1, 0U);
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_gr_table_data), data);

            REG_SET_SLICE(data, MRV_LSC_GB_SAMPLE_0, pLscConfig->LscGBDataTbl[n+CAMERIC_MAX_LSC_SECTORS]);
            REG_SET_SLICE(data, MRV_LSC_GB_SAMPLE_1, 0U);
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_gb_table_data), data);

            REG_SET_SLICE(data, MRV_LSC_B_SAMPLE_0, pLscConfig->LscBDataTbl[n+CAMERIC_MAX_LSC_SECTORS]);
            REG_SET_SLICE(data, MRV_LSC_B_SAMPLE_1, 0U);
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_b_table_data), data);
        }

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_table_sel), 0U );

        /* program x size tables */
        data = 0;
        REG_SET_SLICE(data, MRV_LSC_X_SECT_SIZE_0, pLscConfig->LscXSizeTbl[0]);
        REG_SET_SLICE(data, MRV_LSC_X_SECT_SIZE_1, pLscConfig->LscXSizeTbl[1]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xsize_01), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_X_SECT_SIZE_2, pLscConfig->LscXSizeTbl[2]);
        REG_SET_SLICE(data, MRV_LSC_X_SECT_SIZE_3, pLscConfig->LscXSizeTbl[3]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xsize_23), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_X_SECT_SIZE_4, pLscConfig->LscXSizeTbl[4]);
        REG_SET_SLICE(data, MRV_LSC_X_SECT_SIZE_5, pLscConfig->LscXSizeTbl[5]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xsize_45), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_X_SECT_SIZE_6, pLscConfig->LscXSizeTbl[6]);
        REG_SET_SLICE(data, MRV_LSC_X_SECT_SIZE_7, pLscConfig->LscXSizeTbl[7]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xsize_67), data);

        /* program y size tables */
        data = 0;
        REG_SET_SLICE(data, MRV_LSC_Y_SECT_SIZE_0, pLscConfig->LscYSizeTbl[0]);
        REG_SET_SLICE(data, MRV_LSC_Y_SECT_SIZE_1, pLscConfig->LscYSizeTbl[1]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ysize_01), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_Y_SECT_SIZE_2, pLscConfig->LscYSizeTbl[2]);
        REG_SET_SLICE(data, MRV_LSC_Y_SECT_SIZE_3, pLscConfig->LscYSizeTbl[3]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ysize_23), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_Y_SECT_SIZE_4, pLscConfig->LscYSizeTbl[4]);
        REG_SET_SLICE(data, MRV_LSC_Y_SECT_SIZE_5, pLscConfig->LscYSizeTbl[5]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ysize_45), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_Y_SECT_SIZE_6, pLscConfig->LscYSizeTbl[6]);
        REG_SET_SLICE(data, MRV_LSC_Y_SECT_SIZE_7, pLscConfig->LscYSizeTbl[7]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ysize_67), data);

        /* program x grad tables */
        data = 0;
        REG_SET_SLICE(data, MRV_LSC_XGRAD_0, pLscConfig->LscXGradTbl[0]);
        REG_SET_SLICE(data, MRV_LSC_XGRAD_1, pLscConfig->LscXGradTbl[1]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xgrad_01), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_XGRAD_2, pLscConfig->LscXGradTbl[2]);
        REG_SET_SLICE(data, MRV_LSC_XGRAD_3, pLscConfig->LscXGradTbl[3]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xgrad_23), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_XGRAD_4, pLscConfig->LscXGradTbl[4]);
        REG_SET_SLICE(data, MRV_LSC_XGRAD_5, pLscConfig->LscXGradTbl[5]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xgrad_45), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_XGRAD_6, pLscConfig->LscXGradTbl[6]);
        REG_SET_SLICE(data, MRV_LSC_XGRAD_7, pLscConfig->LscXGradTbl[7]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xgrad_67), data);

        /* program y grad tables */
        data = 0;
        REG_SET_SLICE(data, MRV_LSC_YGRAD_0, pLscConfig->LscYGradTbl[0]);
        REG_SET_SLICE(data, MRV_LSC_YGRAD_1, pLscConfig->LscYGradTbl[1]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ygrad_01), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_YGRAD_2, pLscConfig->LscYGradTbl[2]);
        REG_SET_SLICE(data, MRV_LSC_YGRAD_3, pLscConfig->LscYGradTbl[3]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ygrad_23), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_YGRAD_4, pLscConfig->LscYGradTbl[4]);
        REG_SET_SLICE(data, MRV_LSC_YGRAD_5, pLscConfig->LscYGradTbl[5]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ygrad_45), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_YGRAD_6, pLscConfig->LscYGradTbl[6]);
        REG_SET_SLICE(data, MRV_LSC_YGRAD_7, pLscConfig->LscYGradTbl[7]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ygrad_67), data);

        if ( BOOL_TRUE == restart )
        {
            CamerIcIspLscEnable( handle );
        }
    }
    else
    {
        /* if no config diable the lsc-module */
        CamerIcIspLscDisable( handle );
    }

    TRACE( CAMERIC_ISP_LSC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspLscGetLenseShadeSectorConfig()
 *****************************************************************************/
RESULT CamerIcIspLscGetLenseShadeSectorConfig
(
    CamerIcDrvHandle_t          handle,
    CamerIcIspLscSectorConfig_t *pLscConfig
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_LSC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->pIspLscContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pLscConfig )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap  = (MrvAllRegister_t *)ctx->base;

        uint32_t data = 0;

        /* read x size tables */
        data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xsize_01) );
        pLscConfig->LscXSizeTbl[0] = REG_GET_SLICE(data, MRV_LSC_X_SECT_SIZE_0);
        pLscConfig->LscXSizeTbl[1] = REG_GET_SLICE(data, MRV_LSC_X_SECT_SIZE_1);
        data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xsize_23) );
        pLscConfig->LscXSizeTbl[2] = REG_GET_SLICE(data, MRV_LSC_X_SECT_SIZE_2);
        pLscConfig->LscXSizeTbl[3] = REG_GET_SLICE(data, MRV_LSC_X_SECT_SIZE_3);
        data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xsize_45) );
        pLscConfig->LscXSizeTbl[4] = REG_GET_SLICE(data, MRV_LSC_X_SECT_SIZE_4);
        pLscConfig->LscXSizeTbl[5] = REG_GET_SLICE(data, MRV_LSC_X_SECT_SIZE_5);
        data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xsize_67) );
        pLscConfig->LscXSizeTbl[6] = REG_GET_SLICE(data, MRV_LSC_X_SECT_SIZE_6);
        pLscConfig->LscXSizeTbl[7] = REG_GET_SLICE(data, MRV_LSC_X_SECT_SIZE_7);

        /* read y size tables */
        data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ysize_01) );
        pLscConfig->LscYSizeTbl[0] = REG_GET_SLICE(data, MRV_LSC_Y_SECT_SIZE_0);
        pLscConfig->LscYSizeTbl[1] = REG_GET_SLICE(data, MRV_LSC_Y_SECT_SIZE_1);
        data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ysize_23) );
        pLscConfig->LscYSizeTbl[2] = REG_GET_SLICE(data, MRV_LSC_Y_SECT_SIZE_2);
        pLscConfig->LscYSizeTbl[3] = REG_GET_SLICE(data, MRV_LSC_Y_SECT_SIZE_3);
        data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ysize_45) );
        pLscConfig->LscYSizeTbl[4] = REG_GET_SLICE(data, MRV_LSC_Y_SECT_SIZE_4);
        pLscConfig->LscYSizeTbl[5] = REG_GET_SLICE(data, MRV_LSC_Y_SECT_SIZE_5);
        data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ysize_67) );
        pLscConfig->LscYSizeTbl[6] = REG_GET_SLICE(data, MRV_LSC_Y_SECT_SIZE_6);
        pLscConfig->LscYSizeTbl[7] = REG_GET_SLICE(data, MRV_LSC_Y_SECT_SIZE_7);

        /* read x grad tables */
        data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xgrad_01) );
        pLscConfig->LscXGradTbl[0] = REG_GET_SLICE(data, MRV_LSC_XGRAD_0);
        pLscConfig->LscXGradTbl[1] = REG_GET_SLICE(data, MRV_LSC_XGRAD_1);
        data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xgrad_23) );
        pLscConfig->LscXGradTbl[2] = REG_GET_SLICE(data, MRV_LSC_XGRAD_2);
        pLscConfig->LscXGradTbl[3] = REG_GET_SLICE(data, MRV_LSC_XGRAD_3);
        data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xgrad_45) );
        pLscConfig->LscXGradTbl[4] = REG_GET_SLICE(data, MRV_LSC_XGRAD_4);
        pLscConfig->LscXGradTbl[5] = REG_GET_SLICE(data, MRV_LSC_XGRAD_5);
        data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xgrad_67) );
        pLscConfig->LscXGradTbl[6] = REG_GET_SLICE(data, MRV_LSC_XGRAD_6);
        pLscConfig->LscXGradTbl[7] = REG_GET_SLICE(data, MRV_LSC_XGRAD_7);

        /* read y grad tables */
        data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ygrad_01) );
        pLscConfig->LscYGradTbl[0] = REG_GET_SLICE(data, MRV_LSC_YGRAD_0);
        pLscConfig->LscYGradTbl[1] = REG_GET_SLICE(data, MRV_LSC_YGRAD_1);
        data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ygrad_23) );
        pLscConfig->LscYGradTbl[2] = REG_GET_SLICE(data, MRV_LSC_YGRAD_2);
        pLscConfig->LscYGradTbl[3] = REG_GET_SLICE(data, MRV_LSC_YGRAD_3);
        data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ygrad_45) );
        pLscConfig->LscYGradTbl[4] = REG_GET_SLICE(data, MRV_LSC_YGRAD_4);
        pLscConfig->LscYGradTbl[5] = REG_GET_SLICE(data, MRV_LSC_YGRAD_5);
        data = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ygrad_67) );
        pLscConfig->LscYGradTbl[6] = REG_GET_SLICE(data, MRV_LSC_YGRAD_6);
        pLscConfig->LscYGradTbl[7] = REG_GET_SLICE(data, MRV_LSC_YGRAD_7);
    }

    TRACE( CAMERIC_ISP_LSC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspLscSetLenseShadeSectorConfig()
 *****************************************************************************/
RESULT CamerIcIspLscSetLenseShadeSectorConfig
(
    CamerIcDrvHandle_t                  handle,
    const CamerIcIspLscSectorConfig_t   *pLscConfig
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_LSC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->pIspLscContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL != pLscConfig )
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        bool_t restart = BOOL_FALSE;

        uint32_t data = 0;

        /* disable lense shade correction before programming */
        if ( ctx->pIspLscContext->enabled )
        {
            CamerIcIspLscDisable( handle );
            restart = BOOL_TRUE;
        }

        /* program x size tables */
        data = 0;
        REG_SET_SLICE(data, MRV_LSC_X_SECT_SIZE_0, pLscConfig->LscXSizeTbl[0]);
        REG_SET_SLICE(data, MRV_LSC_X_SECT_SIZE_1, pLscConfig->LscXSizeTbl[1]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xsize_01), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_X_SECT_SIZE_2, pLscConfig->LscXSizeTbl[2]);
        REG_SET_SLICE(data, MRV_LSC_X_SECT_SIZE_3, pLscConfig->LscXSizeTbl[3]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xsize_23), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_X_SECT_SIZE_4, pLscConfig->LscXSizeTbl[4]);
        REG_SET_SLICE(data, MRV_LSC_X_SECT_SIZE_5, pLscConfig->LscXSizeTbl[5]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xsize_45), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_X_SECT_SIZE_6, pLscConfig->LscXSizeTbl[6]);
        REG_SET_SLICE(data, MRV_LSC_X_SECT_SIZE_7, pLscConfig->LscXSizeTbl[7]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xsize_67), data);

        /* program y size tables */
        data = 0;
        REG_SET_SLICE(data, MRV_LSC_Y_SECT_SIZE_0, pLscConfig->LscYSizeTbl[0]);
        REG_SET_SLICE(data, MRV_LSC_Y_SECT_SIZE_1, pLscConfig->LscYSizeTbl[1]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ysize_01), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_Y_SECT_SIZE_2, pLscConfig->LscYSizeTbl[2]);
        REG_SET_SLICE(data, MRV_LSC_Y_SECT_SIZE_3, pLscConfig->LscYSizeTbl[3]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ysize_23), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_Y_SECT_SIZE_4, pLscConfig->LscYSizeTbl[4]);
        REG_SET_SLICE(data, MRV_LSC_Y_SECT_SIZE_5, pLscConfig->LscYSizeTbl[5]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ysize_45), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_Y_SECT_SIZE_6, pLscConfig->LscYSizeTbl[6]);
        REG_SET_SLICE(data, MRV_LSC_Y_SECT_SIZE_7, pLscConfig->LscYSizeTbl[7]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ysize_67), data);

        /* program x grad tables */
        data = 0;
        REG_SET_SLICE(data, MRV_LSC_XGRAD_0, pLscConfig->LscXGradTbl[0]);
        REG_SET_SLICE(data, MRV_LSC_XGRAD_1, pLscConfig->LscXGradTbl[1]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xgrad_01), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_XGRAD_2, pLscConfig->LscXGradTbl[2]);
        REG_SET_SLICE(data, MRV_LSC_XGRAD_3, pLscConfig->LscXGradTbl[3]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xgrad_23), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_XGRAD_4, pLscConfig->LscXGradTbl[4]);
        REG_SET_SLICE(data, MRV_LSC_XGRAD_5, pLscConfig->LscXGradTbl[5]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xgrad_45), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_XGRAD_6, pLscConfig->LscXGradTbl[6]);
        REG_SET_SLICE(data, MRV_LSC_XGRAD_7, pLscConfig->LscXGradTbl[7]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_xgrad_67), data);

        /* program y grad tables */
        data = 0;
        REG_SET_SLICE(data, MRV_LSC_YGRAD_0, pLscConfig->LscYGradTbl[0]);
        REG_SET_SLICE(data, MRV_LSC_YGRAD_1, pLscConfig->LscYGradTbl[1]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ygrad_01), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_YGRAD_2, pLscConfig->LscYGradTbl[2]);
        REG_SET_SLICE(data, MRV_LSC_YGRAD_3, pLscConfig->LscYGradTbl[3]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ygrad_23), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_YGRAD_4, pLscConfig->LscYGradTbl[4]);
        REG_SET_SLICE(data, MRV_LSC_YGRAD_5, pLscConfig->LscYGradTbl[5]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ygrad_45), data);

        data = 0;
        REG_SET_SLICE(data, MRV_LSC_YGRAD_6, pLscConfig->LscYGradTbl[6]);
        REG_SET_SLICE(data, MRV_LSC_YGRAD_7, pLscConfig->LscYGradTbl[7]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_ygrad_67), data);

        if ( BOOL_TRUE == restart )
        {
            CamerIcIspLscEnable( handle );
        }
    }
    else
    {
        /* if no config diable the lsc-module */
        CamerIcIspLscDisable( handle );
    }

    TRACE( CAMERIC_ISP_LSC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspLscSetLenseShadeCorrection()
 *****************************************************************************/
RESULT CamerIcIspLscSetLenseShadeCorrectionMatrix
(
    CamerIcDrvHandle_t  handle,
    const uint16_t      *pLscRDataTbl,      /**< correction values of R color part  */
    const uint16_t      *pLscGRDataTbl,     /**< correction values of G color part  */
    const uint16_t      *pLscGBDataTbl,     /**< correction values of G color part  */
    const uint16_t      *pLscBDataTbl       /**< correction values of B color part  */
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;

    TRACE( CAMERIC_ISP_LSC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    ptCamerIcRegMap  = (MrvAllRegister_t *)ctx->base;

    if ( (pLscRDataTbl != NULL) && (pLscGRDataTbl != NULL) && (pLscGBDataTbl != NULL) && (pLscBDataTbl != NULL) )
    {
        uint32_t i;
        uint32_t n;

        uint32_t data               = 0U;
        uint32_t isp_lsc_status     = 0U;
        uint32_t isp_lsc_table_sel  = 0U;
        uint32_t sram_addr          = 0U;

        isp_lsc_status = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_status) );
        sram_addr = ( isp_lsc_status & 0x2U ) ? 0U : 153U;  /* ( 17 * 18 ) >> 1 */

        /* clear/reset address counters */
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_r_table_addr),    sram_addr);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_gr_table_addr),   sram_addr);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_gb_table_addr),   sram_addr);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_b_table_addr),    sram_addr);

        /* program data tables (table size is 9 * 17 = 153) */
        for (n = 0; n < ((CAMERIC_MAX_LSC_SECTORS + 1) * (CAMERIC_MAX_LSC_SECTORS + 1)); n += CAMERIC_MAX_LSC_SECTORS + 1) // 17 steps
        {
            /* 17 sectors with 2 values in one DWORD = 9 DWORDs (8 steps + 1 outside loop) */
            for (i = 0; i < (CAMERIC_MAX_LSC_SECTORS); i += 2)
            {
                REG_SET_SLICE(data, MRV_LSC_R_SAMPLE_0, pLscRDataTbl[n+i]);
                REG_SET_SLICE(data, MRV_LSC_R_SAMPLE_1, pLscRDataTbl[n+i+1]);
                HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_r_table_data), data);

                REG_SET_SLICE(data, MRV_LSC_GR_SAMPLE_0, pLscGRDataTbl[n+i]);
                REG_SET_SLICE(data, MRV_LSC_GR_SAMPLE_1, pLscGRDataTbl[n+i+1]);
                HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_gr_table_data), data);

                REG_SET_SLICE(data, MRV_LSC_GB_SAMPLE_0, pLscGBDataTbl[n+i]);
                REG_SET_SLICE(data, MRV_LSC_GB_SAMPLE_1, pLscGBDataTbl[n+i+1]);
                HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_gb_table_data), data);

                REG_SET_SLICE(data, MRV_LSC_B_SAMPLE_0, pLscBDataTbl[n+i]);
                REG_SET_SLICE(data, MRV_LSC_B_SAMPLE_1, pLscBDataTbl[n+i+1]);
                HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_b_table_data), data);
            }

            REG_SET_SLICE(data, MRV_LSC_R_SAMPLE_0, pLscRDataTbl[n+CAMERIC_MAX_LSC_SECTORS]);
            REG_SET_SLICE(data, MRV_LSC_R_SAMPLE_1, 0);
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_r_table_data), data);

            REG_SET_SLICE(data, MRV_LSC_GR_SAMPLE_0, pLscGRDataTbl[n+CAMERIC_MAX_LSC_SECTORS]);
            REG_SET_SLICE(data, MRV_LSC_GR_SAMPLE_1, 0);
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_gr_table_data), data);

            REG_SET_SLICE(data, MRV_LSC_GB_SAMPLE_0, pLscGBDataTbl[n+CAMERIC_MAX_LSC_SECTORS]);
            REG_SET_SLICE(data, MRV_LSC_GB_SAMPLE_1, 0);
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_gb_table_data), data);

            REG_SET_SLICE(data, MRV_LSC_B_SAMPLE_0, pLscBDataTbl[n+CAMERIC_MAX_LSC_SECTORS]);
            REG_SET_SLICE(data, MRV_LSC_B_SAMPLE_1, 0);
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_b_table_data), data);
        }

        isp_lsc_table_sel = ( isp_lsc_status & 0x2U ) ? 0U : 1U;
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_lsc_table_sel), isp_lsc_table_sel );
    }
    else
    {
        return ( RET_INVALID_PARM );
    }

    TRACE( CAMERIC_ISP_LSC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}

#else  /* #if defined(MRV_LSC_VERSION) */

/******************************************************************************
 * Lense Shade Correction Module is not available.
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 *
 *****************************************************************************/
/******************************************************************************
 * CamerIcIspLscEnable()
 *****************************************************************************/
RESULT CamerIcIspLscEnable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspLscDisable()
 *****************************************************************************/
RESULT CamerIcIspLscDisable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspLscIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspLscIsEnabled
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
 * CamerIcIspLscGetLenseShadeCorrection()
 *****************************************************************************/
RESULT CamerIcIspLscGetLenseShadeCorrection
(
    CamerIcDrvHandle_t      handle,
    CamerIcIspLscConfig_t   *pLscConfig
)
{
    (void)handle;
    (void)pLscConfig;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspLscSetLenseShadeCorrection()
 *****************************************************************************/
RESULT CamerIcIspLscSetLenseShadeCorrection
(
    CamerIcDrvHandle_t          handle,
    const CamerIcIspLscConfig_t *pLscConfig
)
{
    (void)handle;
    (void)pLscConfig;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspLscGetLenseShadeSectorConfig()
 *****************************************************************************/
RESULT CamerIcIspLscGetLenseShadeSectorConfig
(
    CamerIcDrvHandle_t          handle,
    CamerIcIspLscSectorConfig_t *pLscConfig
)
{
    (void)handle;
    (void)pLscConfig;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspLscSetLenseShadeSectorConfig()
 *****************************************************************************/
RESULT CamerIcIspLscSetLenseShadeSectorConfig
(
    CamerIcDrvHandle_t                  handle,
    const CamerIcIspLscSectorConfig_t   *pLscConfig
)
{
    (void)handle;
    (void)pLscConfig;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspLscSetLenseShadeCorrection()
 *****************************************************************************/
RESULT CamerIcIspLscSetLenseShadeCorrectionMatrix
(
    CamerIcDrvHandle_t  handle,
    const uint16_t      *pLscRDataTbl,      /**< correction values of R color part  */
    const uint16_t      *pLscGRDataTbl,     /**< correction values of G color part  */
    const uint16_t      *pLscGBDataTbl,     /**< correction values of G color part  */
    const uint16_t      *pLscBDataTbl       /**< correction values of B color part  */
)
{
    (void)handle;
    (void)pLscRDataTbl;
    (void)pLscGRDataTbl;
    (void)pLscGBDataTbl;
    (void)pLscBDataTbl;
    return ( RET_NOTSUPP );
}

#endif  /* #if defined(MRV_LSC_VERSION) */

