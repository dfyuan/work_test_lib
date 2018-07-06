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
 * @file cameric_mipi.c
 *
 * @brief
 *   Implementation of CamerIC MIPI Module.
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include "mrv_all_bits.h"

#include "cameric_drv_cb.h"
#include "cameric_drv.h"


/******************************************************************************
 * Is the hardware MIPI module available ?
 *****************************************************************************/
#if defined(MRV_MIPI_VERSION)

/******************************************************************************
 * MIPI Module is available.
 *****************************************************************************/

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_MIPI_DRV_INFO  , "CAMERIC_MIPI_DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_MIPI_DRV_WARN  , "CAMERIC_MIPI_DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_MIPI_DRV_ERROR , "CAMERIC_MIPI_DRV: ", ERROR   , 1 );

CREATE_TRACER( CAMERIC_MIPI_IRQ_INFO  , "CAMERIC-MIPI-IRQ: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_MIPI_IRQ_ERROR , "CAMERIC-MIPI-IRQ: ", ERROR   , 1 );


#define MAX_NUMBER_OF_MIPI_LANES     4


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
 * CamerIcMipiIrq()
 *****************************************************************************/
static int32_t CamerIcMipiIrq
(
    void *pArg
)
{
    CamerIcDrvContext_t *pDrvCtx;       /* CamerIc Driver Context */
    HalIrqCtx_t         *pHalIrqCtx;    /* HAL context */
    static int cnt = 0;

    TRACE( CAMERIC_MIPI_IRQ_INFO, "%s: (enter) \n", __FUNCTION__ );

    /* get IRQ context from args */
    pHalIrqCtx = (HalIrqCtx_t*)(pArg);
    DCT_ASSERT( (pHalIrqCtx != NULL) );

    pDrvCtx = (CamerIcDrvContext_t *)(pHalIrqCtx->OsIrq.p_context);
    DCT_ASSERT( (pDrvCtx != NULL) );
    DCT_ASSERT( (pDrvCtx->pMipiContext != NULL) );

    if ( pHalIrqCtx->misValue & MRV_MIPI_MIS_SYNC_FIFO_OVFLW_MASK )
    {
        pHalIrqCtx->misValue &= ~( MRV_MIPI_MIS_SYNC_FIFO_OVFLW_MASK );
    }

    if ( pHalIrqCtx->misValue & MRV_MIPI_MIS_ERR_SOT_MASK )
    {
        pHalIrqCtx->misValue &= ~( MRV_MIPI_MIS_ERR_SOT_MASK );
    }

    if ( pHalIrqCtx->misValue & MRV_MIPI_MIS_ERR_SOT_SYNC_MASK )
    {
        pHalIrqCtx->misValue &= ~( MRV_MIPI_MIS_ERR_SOT_SYNC_MASK );
    }

    if ( pHalIrqCtx->misValue & MRV_MIPI_MIS_ERR_EOT_SYNC_MASK )
    {
        pHalIrqCtx->misValue &= ~( MRV_MIPI_MIS_ERR_EOT_SYNC_MASK );
    }

    if ( pHalIrqCtx->misValue & MRV_MIPI_MIS_ERR_CONTROL_MASK )
    {
        pHalIrqCtx->misValue &= ~( MRV_MIPI_MIS_ERR_EOT_SYNC_MASK );
    }

    if ( pHalIrqCtx->misValue & MRV_MIPI_MIS_ERR_PROTOCOL_MASK )
    {
        pHalIrqCtx->misValue &= ~( MRV_MIPI_MIS_ERR_PROTOCOL_MASK );
    }

    if ( pHalIrqCtx->misValue & MRV_MIPI_MIS_ERR_ECC2_MASK )
    {
        pHalIrqCtx->misValue &= ~( MRV_MIPI_MIS_ERR_ECC2_MASK );
    }

    if ( pHalIrqCtx->misValue & MRV_MIPI_MIS_ERR_ECC1_MASK )
    {
        pHalIrqCtx->misValue &= ~( MRV_MIPI_MIS_ERR_ECC1_MASK );
    }

    if ( pHalIrqCtx->misValue & MRV_MIPI_MIS_ERR_CS_MASK )
    {
        pHalIrqCtx->misValue &= ~( MRV_MIPI_MIS_ERR_CS_MASK );
    }

    if ( pHalIrqCtx->misValue & MRV_MIPI_MIS_FRAME_END_MASK )
    {
        pHalIrqCtx->misValue &= ~( MRV_MIPI_MIS_FRAME_END_MASK );
    }

    if ( pHalIrqCtx->misValue & MRV_MIPI_MIS_ADD_DATA_OVFLW_MASK )
    {
        pHalIrqCtx->misValue &= ~( MRV_MIPI_MIS_ADD_DATA_OVFLW_MASK );
    }

    if ( pHalIrqCtx->misValue & MRV_MIPI_MIS_ADD_DATA_FILL_LEVEL_MASK )
    {
        pHalIrqCtx->misValue &= ~( MRV_MIPI_MIS_ADD_DATA_FILL_LEVEL_MASK );
    }

    if (  pHalIrqCtx->misValue != 0U )
    {
        TRACE( CAMERIC_MIPI_IRQ_ERROR, "%s: 0x%08x\n", __FUNCTION__, pHalIrqCtx->misValue );
    }

    TRACE( CAMERIC_MIPI_IRQ_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( 0 );
}


/******************************************************************************
 * CamerIcMipiReset()
 *****************************************************************************/
static RESULT CamerIcMipiReset
(
    CamerIcDrvContext_t *ctx,
    const int32_t       idx
)
{
    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pMipiContext[idx] == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t vi_ircl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_ircl) );
        switch ( idx )
        {
            case 0:
                {
                    REG_SET_SLICE( vi_ircl, MRV_VI_MIPI_SOFT_RST, 1 );
                    break;
                }
#if MIPI_ITF_ARR_SIZE > 1
            case 1:
                {
                    REG_SET_SLICE( vi_ircl, MRV_VI_MIPI2_SOFT_RST, 1 );
                    break;
                }
#endif /* #if defined(MIPI_ITF_ARR_SIZE > 1) */
            default:
                {
                    return ( RET_INVALID_PARM );
                }
        }
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_ircl), vi_ircl );

        switch ( idx )
        {
            case 0:
                {
                    REG_SET_SLICE( vi_ircl, MRV_VI_MIPI_SOFT_RST, 0 );
                    break;
                }
#if MIPI_ITF_ARR_SIZE > 1
            case 1:
                {
                    REG_SET_SLICE( vi_ircl, MRV_VI_MIPI2_SOFT_RST, 0 );
                    break;
                }
#endif /* #if defined(MIPI_ITF_ARR_SIZE > 1) */
            default:
                {
                    return ( RET_INVALID_PARM );
                }
        }
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_ircl), vi_ircl );
    }

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcMipiInitHw()
 *****************************************************************************/
static RESULT CamerIcMipiInitHw
(
    CamerIcDrvContext_t *ctx,
    const int32_t       idx
)
{
    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pMipiContext[idx] == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t mipi_ctrl                = 0UL;
        uint32_t mipi_imsc                = 0UL;
        uint32_t mipi_icr                 = 0UL;
        uint32_t mipi_img_data_sel        = 0UL;
        uint32_t mipi_add_data_sel        = 0UL;
        uint32_t mipi_add_data_fill_level = 0UL;

        mipi_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_ctrl) );
        REG_SET_SLICE( mipi_ctrl, MRV_MIPI_ERR_SOT_SYNC_HS_SKIP,   1 );
        REG_SET_SLICE( mipi_ctrl, MRV_MIPI_ERR_SOT_HS_SKIP,        0 );
        REG_SET_SLICE( mipi_ctrl, MRV_MIPI_NUM_LANES,              3 );
        REG_SET_SLICE( mipi_ctrl, MRV_MIPI_SHUTDOWN_LANE,          0xf );
        REG_SET_SLICE( mipi_ctrl, MRV_MIPI_FLUSH_FIFO,             1 );
        REG_SET_SLICE( mipi_ctrl, MRV_MIPI_OUTPUT_ENA,             0 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_ctrl), mipi_ctrl );

        mipi_imsc = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_imsc) );
        REG_SET_SLICE( mipi_imsc, MRV_MIPI_IMSC_ALL_IRQS, 0 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_imsc), mipi_imsc );

        mipi_icr = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_icr) );
        REG_SET_SLICE( mipi_icr, MRV_MIPI_ICR_ALL_IRQS, 0xFFFFFFFFUL );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_icr), mipi_icr );

        mipi_img_data_sel = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_img_data_sel) );
        REG_SET_SLICE( mipi_img_data_sel, MRV_MIPI_VIRTUAL_CHANNEL, 0 );
        REG_SET_SLICE( mipi_img_data_sel, MRV_MIPI_DATA_TYPE,       0 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_img_data_sel), mipi_img_data_sel );

        mipi_add_data_sel = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_add_data_sel_1) );
        REG_SET_SLICE( mipi_add_data_sel, MRV_MIPI_ADD_DATA_VC_1,      3 );
        REG_SET_SLICE( mipi_add_data_sel, MRV_MIPI_ADD_DATA_TYPE_1, 0x3F );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_add_data_sel_1), mipi_add_data_sel );

        mipi_add_data_sel = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_add_data_sel_2) );
        REG_SET_SLICE( mipi_add_data_sel, MRV_MIPI_ADD_DATA_VC_2,      3 );
        REG_SET_SLICE( mipi_add_data_sel, MRV_MIPI_ADD_DATA_TYPE_2, 0x3F );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_add_data_sel_2), mipi_add_data_sel );

        mipi_add_data_sel = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_add_data_sel_3) );
        REG_SET_SLICE( mipi_add_data_sel, MRV_MIPI_ADD_DATA_VC_3,      3 );
        REG_SET_SLICE( mipi_add_data_sel, MRV_MIPI_ADD_DATA_TYPE_3, 0x3F );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_add_data_sel_3), mipi_add_data_sel );

        mipi_add_data_sel = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_add_data_sel_4) );
        REG_SET_SLICE( mipi_add_data_sel, MRV_MIPI_ADD_DATA_VC_4,      3 );
        REG_SET_SLICE( mipi_add_data_sel, MRV_MIPI_ADD_DATA_TYPE_4, 0x3F );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_add_data_sel_4), mipi_add_data_sel );

        mipi_add_data_sel = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_add_data_fill_level) );
        REG_SET_SLICE( mipi_add_data_fill_level, MRV_MIPI_ADD_DATA_FILL_LEVEL, 0);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_add_data_fill_level), mipi_add_data_fill_level );
    }

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver Internal API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcMipiEnableClock()
 *****************************************************************************/
RESULT CamerIcMipiEnableClock
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->HalHandle == NULL) || (ctx->pMipiContext[idx] == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t vi_iccl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_iccl) );

        switch ( idx )
        {
            case 0:
                {
                    REG_SET_SLICE( vi_iccl, MRV_VI_MIPI_CLK_ENABLE, 1 );
                    break;
                }
#if MIPI_ITF_ARR_SIZE > 1
            case 1:
                {
                    REG_SET_SLICE( vi_iccl, MRV_VI_MIPI2_CLK_ENABLE, 1 );
                    break;
                }
#endif /* #if defined(MIPI_ITF_ARR_SIZE > 1) */
            default:
                {
                    return ( RET_INVALID_PARM );
                }
        }

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_iccl), vi_iccl );
    }

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcMipiDisableClock()
 *****************************************************************************/
RESULT CamerIcMipiDisableClock
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t vi_iccl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_iccl) );

        switch ( idx )
        {
            case 0:
                {
                    REG_SET_SLICE( vi_iccl, MRV_VI_MIPI_CLK_ENABLE, 0 );
                    break;
                }
#if MIPI_ITF_ARR_SIZE > 1
            case 1:
                {
                    REG_SET_SLICE( vi_iccl, MRV_VI_MIPI2_CLK_ENABLE, 0 );
                    break;
                }
#endif /* #if defined(MIPI_ITF_ARR_SIZE > 1) */
            default:
                {
                    return ( RET_INVALID_PARM );
                }
        }

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_iccl), vi_iccl );
    }

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcMipiInit()
 *****************************************************************************/
RESULT CamerIcMipiInit
(
    CamerIcDrvHandle_t handle,
    const int32_t      idx
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check instance index */
    if ( (0 > idx) && ((MIPI_ITF_ARR_SIZE - 1) < idx) )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pMipiContext[idx] = ( CamerIcMipiContext_t *)malloc( sizeof(CamerIcMipiContext_t) );
    if (  ctx->pMipiContext[idx] == NULL )
    {
        TRACE( CAMERIC_MIPI_DRV_ERROR,  "%s: Can't allocate CamerIC MIPI context.\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pMipiContext[idx], 0, sizeof(CamerIcMipiContext_t) );

    /* set instance index */
    ctx->pMipiContext[idx]->idx = idx;

    result = CamerIcMipiEnableClock( ctx, idx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAMERIC_MIPI_DRV_ERROR,  "%s: Failed to enable clock for CamerIC MIPI module.\n",  __FUNCTION__ );
        return ( result );
    }

    result = CamerIcMipiReset( ctx, idx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAMERIC_MIPI_DRV_ERROR,  "%s: Reseting CamerIC MIPI module failed.\n",  __FUNCTION__ );
        return ( result );
    }

    result = CamerIcMipiInitHw( ctx, idx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAMERIC_MIPI_DRV_ERROR,  "%s: Initializing CamerIC MIPI module failed.\n",  __FUNCTION__ );
        return ( result );
    }

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * CamerIcMipiRelease()
 *****************************************************************************/
RESULT CamerIcMipiRelease
(
    CamerIcDrvHandle_t handle,
    const int32_t      idx
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check instance index */
    if ( (0 > idx) && ((MIPI_ITF_ARR_SIZE - 1) < idx) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pMipiContext[idx] == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pMipiContext[idx], 0, sizeof(CamerIcMipiContext_t) );
    free ( ctx->pMipiContext[idx] );
    ctx->pMipiContext[idx] = NULL;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcMipiStart()
 *****************************************************************************/
RESULT CamerIcMipiStart
(
    CamerIcDrvHandle_t handle,
    const int32_t      idx
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;
    RESULT result;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check instance index */
    if ( (0 > idx) && ((MIPI_ITF_ARR_SIZE - 1) < idx) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pMipiContext[idx] == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t mipi_imsc = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_imsc) );

        /* register interrupt handler for the MIPI at hal */
        ctx->pMipiContext[idx]->HalIrqCtx.misRegAddress = ( (ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_mis );
        ctx->pMipiContext[idx]->HalIrqCtx.icrRegAddress = ( (ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_icr );
#if 0 //needn't deal with mipi irq,zyc
        result = HalConnectIrq( ctx->HalHandle, &ctx->pMipiContext[idx]->HalIrqCtx, 0, NULL, &CamerIcMipiIrq, ctx );
        DCT_ASSERT( (result == RET_SUCCESS) );
#endif
	}

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcMipiStop()
 *****************************************************************************/
RESULT CamerIcMipiStop
(
    CamerIcDrvHandle_t handle,
    const int32_t      idx
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check instance index */
    if ( (0 > idx) && ((MIPI_ITF_ARR_SIZE - 1) < idx) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pMipiContext[idx] == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        RESULT result = RET_SUCCESS;

        /* stop interrupts of CamerIc MI Module */
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_imsc), 0UL );
#if 0 //needn't deal with mipi irq,zyc

        /* disconnect IRQ handler */
        result = HalDisconnectIrq( &ctx->pMipiContext[idx]->HalIrqCtx );
        DCT_ASSERT( (result == RET_SUCCESS) );
#endif
    }

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcMipiRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcMipiRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx,
    CamerIcEventFunc_t  func,
    void                *pUserContext
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check instance index */
    if ( (0 > idx) && ((MIPI_ITF_ARR_SIZE - 1) < idx) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pMipiContext[idx] == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( ctx->pMipiContext[idx]->EventCb.func != NULL )
    {
        return ( RET_BUSY );
    }

    if ( func == NULL )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        ctx->pMipiContext[idx]->EventCb.func          = func;
        ctx->pMipiContext[idx]->EventCb.pUserContext  = pUserContext;
    }

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcMipiDeRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcMipiDeRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check instance index */
    if ( (0 > idx) && ((MIPI_ITF_ARR_SIZE - 1) < idx) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pMipiContext[idx] == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    ctx->pMipiContext[idx]->EventCb.func          = NULL;
    ctx->pMipiContext[idx]->EventCb.pUserContext  = NULL;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcMipiEnable()
 *****************************************************************************/
RESULT CamerIcMipiEnable
(
    CamerIcDrvHandle_t handle,
    const int32_t      idx
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check instance index */
    if ( (0 > idx) && ((MIPI_ITF_ARR_SIZE - 1) < idx) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (ctx == NULL) || (ctx->pMipiContext[idx] == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t mipi_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_ctrl) );
        REG_SET_SLICE( mipi_ctrl, MRV_MIPI_OUTPUT_ENA, 1 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_ctrl), mipi_ctrl );

        ctx->pMipiContext[idx]->enabled = BOOL_TRUE;
    }

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcMipiDisable()
 *****************************************************************************/
RESULT CamerIcMipiDisable
(
    CamerIcDrvHandle_t handle,
    const int32_t      idx
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check instance index */
    if ( (0 > idx) && ((MIPI_ITF_ARR_SIZE - 1) < idx) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (ctx == NULL) || (ctx->pMipiContext[idx] == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t mipi_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_ctrl) );
        REG_SET_SLICE( mipi_ctrl, MRV_MIPI_OUTPUT_ENA, 0 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_ctrl), mipi_ctrl );

        ctx->pMipiContext[idx]->enabled = BOOL_FALSE;
    }

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcMipiIsEnabled()
 *****************************************************************************/
RESULT CamerIcMipiIsEnabled
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx,
    bool_t              *pIsEnabled
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check instance index */
    if ( (0 > idx) && ((MIPI_ITF_ARR_SIZE - 1) < idx) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (ctx == NULL) || (ctx->pMipiContext[idx] == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pIsEnabled )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsEnabled = ctx->pMipiContext[idx]->enabled;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcMipiGetNumberOfLanes()
 *****************************************************************************/
RESULT CamerIcMipiGetNumberOfLanes
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx,
    uint32_t            *no_lanes
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check instance index */
    if ( (0 > idx) && ((MIPI_ITF_ARR_SIZE - 1) < idx) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (ctx == NULL) || (ctx->pMipiContext[idx] == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == no_lanes )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        *no_lanes = ctx->pMipiContext[idx]->no_lanes;
    }

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcMipiSetNumberOfLanes()
 *****************************************************************************/
RESULT CamerIcMipiSetNumberOfLanes
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx,
    const uint32_t      no_lanes
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check instance index */
    if ( (0 > idx) && ((MIPI_ITF_ARR_SIZE - 1) < idx) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (ctx == NULL) || (ctx->pMipiContext[idx] == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (no_lanes == 0) || (no_lanes > MAX_NUMBER_OF_MIPI_LANES) )
    {
        return ( RET_OUTOFRANGE );
    }

    if ( ctx->pMipiContext[idx]->enabled == BOOL_TRUE )
    {
        return ( RET_BUSY );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t mipi_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_ctrl) );
        REG_SET_SLICE( mipi_ctrl, MRV_MIPI_NUM_LANES, (no_lanes - 1U) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_ctrl), mipi_ctrl );

        ctx->pMipiContext[idx]->no_lanes = no_lanes;
    }

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcMipiSetVirtualChannelAndDataType()
 *****************************************************************************/
RESULT CamerIcMipiSetVirtualChannelAndDataType
(
    CamerIcDrvHandle_t          handle,
    const int32_t               idx,
    const MipiVirtualChannel_t  vc,
    const MipiDataType_t        dt
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check instance index */
    if ( (0 > idx) && ((MIPI_ITF_ARR_SIZE - 1) < idx) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (ctx == NULL) || (ctx->pMipiContext[idx] == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (vc > MIPI_VIRTUAL_CHANNEL_MAX) || (dt > MIPI_DATA_TYPE_MAX) )
    {
        return ( RET_OUTOFRANGE );
    }

    if ( dt == MIPI_DATA_TYPE_RAW_14 )
    {
        return ( RET_NOTSUPP );
    }

    if ( ctx->pMipiContext[idx]->enabled == BOOL_TRUE )
    {
        return ( RET_BUSY );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t mipi_img_data_sel = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_img_data_sel) );
        REG_SET_SLICE( mipi_img_data_sel, MRV_MIPI_VIRTUAL_CHANNEL_SEL, vc );
        REG_SET_SLICE( mipi_img_data_sel, MRV_MIPI_DATA_TYPE_SEL, dt );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_img_data_sel), mipi_img_data_sel );

        ctx->pMipiContext[idx]->virtual_channel  = vc;
        ctx->pMipiContext[idx]->data_type        = dt;
    }

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcMipiSetCompressionSchemeAndPredictorBlock()
 *****************************************************************************/
RESULT CamerIcMipiSetCompressionSchemeAndPredictorBlock
(
    CamerIcDrvHandle_t                  handle,
    const int32_t                       idx,
    const MipiDataCompressionScheme_t   cs,
    const MipiPredictorBlock_t          pred
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check instance index */
    if ( (0 > idx) && ((MIPI_ITF_ARR_SIZE - 1) < idx) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (ctx == NULL) || (ctx->pMipiContext[idx] == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( cs >= MIPI_DATA_COMPRESSION_SCHEME_MAX )
    {
        return ( RET_OUTOFRANGE );
    }

    if ( ctx->pMipiContext[idx]->enabled == BOOL_TRUE)
    {
        return ( RET_BUSY );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t mipi_compressed_mode = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_compressed_mode) );

        switch ( cs )
        {
            case MIPI_DATA_COMPRESSION_SCHEME_12_8_12:
                {
                    REG_SET_SLICE( mipi_compressed_mode, MRV_MIPI_COMP_SCHEME, MRV_MIPI_COMP_SCHEME_12_8_12 );
                    break;
                }

            case MIPI_DATA_COMPRESSION_SCHEME_12_7_12:
                {
                    REG_SET_SLICE( mipi_compressed_mode, MRV_MIPI_COMP_SCHEME, MRV_MIPI_COMP_SCHEME_12_7_12 );
                    break;
                }

            case MIPI_DATA_COMPRESSION_SCHEME_12_6_12:
                {
                    REG_SET_SLICE( mipi_compressed_mode, MRV_MIPI_COMP_SCHEME, MRV_MIPI_COMP_SCHEME_12_6_12 );
                    break;
                }

            case MIPI_DATA_COMPRESSION_SCHEME_10_8_10:
                {
                    REG_SET_SLICE( mipi_compressed_mode, MRV_MIPI_COMP_SCHEME, MRV_MIPI_COMP_SCHEME_10_8_10 );
                    break;
                }

            case MIPI_DATA_COMPRESSION_SCHEME_10_7_10:
                {
                    REG_SET_SLICE( mipi_compressed_mode, MRV_MIPI_COMP_SCHEME, MRV_MIPI_COMP_SCHEME_10_7_10 );
                    break;
                }

            case MIPI_DATA_COMPRESSION_SCHEME_10_6_10:
                {
                    REG_SET_SLICE( mipi_compressed_mode, MRV_MIPI_COMP_SCHEME, MRV_MIPI_COMP_SCHEME_10_6_10 );
                    break;
                }

            default:
                {
                    return ( RET_OUTOFRANGE );
                }
        }

        switch ( pred )
        {
            case MIPI_PREDICTOR_BLOCK_1:
                {
                    REG_SET_SLICE( mipi_compressed_mode, MRV_MIPI_PREDICTOR_SEL, MRV_MIPI_PREDICTOR_SEL_1 );
                    break;
                }

            case MIPI_PREDICTOR_BLOCK_2:
                {
                    REG_SET_SLICE( mipi_compressed_mode, MRV_MIPI_PREDICTOR_SEL, MRV_MIPI_PREDICTOR_SEL_2 );
                }

            default:
                {
                    return ( RET_OUTOFRANGE );
                }
        }

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_compressed_mode), mipi_compressed_mode );

        ctx->pMipiContext[idx]->compression_scheme   = cs;
        ctx->pMipiContext[idx]->predictor_block      = pred;
    }

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcMipiEnableCompressedMode()
 *****************************************************************************/
RESULT CamerIcMipiEnableCompressedMode
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check instance index */
    if ( (0 > idx) && ((MIPI_ITF_ARR_SIZE - 1) < idx) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (ctx == NULL) || (ctx->pMipiContext[idx] == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->pMipiContext[idx]->enabled == BOOL_TRUE )
    {
        return ( RET_BUSY );
    }

    if ( (ctx->pMipiContext[idx]->compression_scheme > MIPI_DATA_COMPRESSION_SCHEME_NONE)
            && (ctx->pMipiContext[idx]->compression_scheme < MIPI_DATA_COMPRESSION_SCHEME_MAX)
            && (ctx->pMipiContext[idx]->predictor_block > MIPI_PREDICTOR_BLOCK_INVALID)
            && (ctx->pMipiContext[idx]->predictor_block < MIPI_PREDICTOR_BLOCK_MAX) )
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t mipi_compressed_mode = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_compressed_mode) );

        REG_SET_SLICE( mipi_compressed_mode, MRV_MIPI_COMPRESS_EN, 1 );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_compressed_mode), mipi_compressed_mode );

        ctx->pMipiContext[idx]->compression_enabled = BOOL_TRUE;
    }
    else
    {
        return ( RET_WRONG_CONFIG );
    }

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcMipiDisableCompressedMode()
 *****************************************************************************/
RESULT CamerIcMipiDisableCompressedMode
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check instance index */
    if ( (0 > idx) && ((MIPI_ITF_ARR_SIZE - 1) < idx) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (ctx == NULL) || (ctx->pMipiContext[idx] == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->pMipiContext[idx]->enabled == BOOL_TRUE )
    {
        return ( RET_BUSY );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t mipi_compressed_mode = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_compressed_mode) );

        REG_SET_SLICE( mipi_compressed_mode, MRV_MIPI_COMPRESS_EN, 0 );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mipi[idx].mipi_compressed_mode), mipi_compressed_mode );

        ctx->pMipiContext[idx]->compression_enabled = BOOL_FALSE;
    }

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcMipiIsEnabledCompressedMode()
 *****************************************************************************/
RESULT CamerIcMipiIsEnabledCompressedMode
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx,
    bool_t              *pIsEnabled
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check instance index */
    if ( (0 > idx) && ((MIPI_ITF_ARR_SIZE - 1) < idx) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (ctx == NULL) || (ctx->pMipiContext[idx] == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsEnabled == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsEnabled = ctx->pMipiContext[idx]->compression_enabled;

    TRACE( CAMERIC_MIPI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


#else  /* #if defined(MRV_MIPI_VERSION) */

/******************************************************************************
 * MIPI Module is not available.
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 * 
 *****************************************************************************/

/******************************************************************************
 * CamerIcMipiRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcMipiRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx,
    CamerIcEventFunc_t  func,
    void                *pUserContext
)
{
    (void)handle;
    (void)idx;
    (void)func;
    (void)pUserContext;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcMipiDeRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcMipiDeRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx
)
{
    (void)handle;
    (void)idx;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcMipiEnable()
 *****************************************************************************/
RESULT CamerIcMipiEnable
(
    CamerIcDrvHandle_t handle,
    const int32_t      idx
)
{
    (void)handle;
    (void)idx;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcMipiDisable()
 *****************************************************************************/
RESULT CamerIcMipiDisable
(
    CamerIcDrvHandle_t handle,
    const int32_t      idx
)
{
    (void)handle;
    (void)idx;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcMipiIsEnabled()
 *****************************************************************************/
RESULT CamerIcMipiIsEnabled
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx,
    bool_t              *pIsEnabled
)
{
    (void)handle;
    (void)idx;
    (void)pIsEnabled;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcMipiGetNumberOfLanes()
 *****************************************************************************/
RESULT CamerIcMipiGetNumberOfLanes
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx,
    uint32_t            *no_lanes
)
{
    (void)handle;
    (void)idx;
    (void)no_lanes;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcMipiSetNumberOfLanes()
 *****************************************************************************/
RESULT CamerIcMipiSetNumberOfLanes
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx,
    const uint32_t      no_lanes
)
{
    (void)handle;
    (void)idx;
    (void)no_lanes;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcMipiSetVirtualChannelAndDataType()
 *****************************************************************************/
RESULT CamerIcMipiSetVirtualChannelAndDataType
(
    CamerIcDrvHandle_t          handle,
    const int32_t               idx,
    const MipiVirtualChannel_t  vc,
    const MipiDataType_t        dt
)
{
    (void)handle;
    (void)idx;
    (void)vc;
    (void)dt;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcMipiSetCompressionSchemeAndPredictorBlock()
 *****************************************************************************/
RESULT CamerIcMipiSetCompressionSchemeAndPredictorBlock
(
    CamerIcDrvHandle_t                  handle,
    const int32_t                       idx,
    const MipiDataCompressionScheme_t   cs,
    const MipiPredictorBlock_t          pred
)
{
    (void)handle;
    (void)idx;
    (void)cs;
    (void)pred;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcMipiEnableCompressedMode()
 *****************************************************************************/
RESULT CamerIcMipiEnableCompressedMode
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx
)
{
    (void)handle;
    (void)idx;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcMipiDisableCompressedMode()
 *****************************************************************************/
RESULT CamerIcMipiDisableCompressedMode
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx
)
{
    (void)handle;
    (void)idx;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcMipiIsEnabledCompressedMode()
 *****************************************************************************/
RESULT CamerIcMipiIsEnabledCompressedMode
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx,
    bool_t              *pIsEnabled
)
{
    (void)handle;
    (void)idx;
    (void)pIsEnabled;
    return ( RET_NOTSUPP );
}


#endif /* #if defined(MRV_MIPI_VERSION) */

