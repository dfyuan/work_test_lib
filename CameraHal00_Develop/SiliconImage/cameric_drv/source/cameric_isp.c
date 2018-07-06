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
 * @file cameric_isp.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>
#include <common/list.h>

#include "mrv_all_bits.h"

#include "cameric_drv_cb.h"
#include "cameric_drv.h"
#include "cameric_isp_drv.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_ISP_DRV_INFO  , "CAMERIC-ISP-DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_ISP_DRV_WARN  , "CAMERIC-ISP-DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_ISP_DRV_ERROR , "CAMERIC-ISP-DRV: ", ERROR   , 1 );

CREATE_TRACER( CAMERIC_ISP_IRQ_INFO  , "CAMERIC-ISP-IRQ: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_ISP_IRQ_ERROR , "CAMERIC-ISP-IRQ: ", ERROR    , 1 );

CREATE_TRACER( CAMERIC_ISP_IRQ_DEBUG , "CAMERIC-ISP-IRQ: ", INFO    , 0 );


/******************************************************************************
 * local type definitions
 *****************************************************************************/



/******************************************************************************
 * local variable declarations
 *****************************************************************************/



/******************************************************************************
 * local function prototypes
 *****************************************************************************/
static int32_t CamerIcIspIrq( void *pArg );
static uint32_t g_ispFrameNum = 0;



/******************************************************************************
 * local functions
 *****************************************************************************/

/*****************************************************************************/
/**
 *          CamerIcIspIrq()
 *
 * @brief   ISP IRQ-Handler
 *
 * @return              Return the result of the function call.
 * @retval              0
 *
 *****************************************************************************/
static int32_t CamerIcIspIrq
(
    void *pArg
)
{
    CamerIcDrvContext_t *pDrvCtx;       /* CamerIc Driver Context */
    HalIrqCtx_t         *pHalIrqCtx;    /* HAL context */

   // TRACE( CAMERIC_ISP_IRQ_ERROR, "%s: (enter) \n", __FUNCTION__ );

    /* get IRQ context from args */
    pHalIrqCtx = (HalIrqCtx_t*)(pArg);
    DCT_ASSERT( (pHalIrqCtx != NULL) );

    pDrvCtx = (CamerIcDrvContext_t *)(pHalIrqCtx->OsIrq.p_context);
    DCT_ASSERT( (pDrvCtx != NULL) );
    DCT_ASSERT( (pDrvCtx->pIspContext != NULL) );

    TRACE( CAMERIC_ISP_IRQ_INFO, "%s: (mis=%08x) \n", __FUNCTION__, pHalIrqCtx->misValue);
    if ( pHalIrqCtx->misValue & MRV_ISP_MIS_ISP_OFF_MASK )
    {
        if ( (pDrvCtx->pStopInputCompletionCb != NULL) && (pDrvCtx->pStopInputCompletionCb->func != NULL) )
        {
            pDrvCtx->pStopInputCompletionCb->func(
                            CAMERIC_ISP_COMMAND_STOP_INPUT,
                            RET_SUCCESS,
                            pDrvCtx->pStopInputCompletionCb->pParam,
                            pDrvCtx->pStopInputCompletionCb->pUserContext );

            pDrvCtx->pStopInputCompletionCb = NULL;
        }
        else if ( (pDrvCtx->pCapturingCompletionCb != NULL) && (pDrvCtx->pCapturingCompletionCb->func != NULL) )
        {
            volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)pDrvCtx->base;

            uint32_t isp_imsc = 0UL;

            pDrvCtx->pCapturingCompletionCb->func(
                            CAMERIC_ISP_COMMAND_CAPTURE_FRAMES,
                            RET_SUCCESS,
                            pDrvCtx->pCapturingCompletionCb->pParam,
                            pDrvCtx->pCapturingCompletionCb->pUserContext );

            /* disable isp-off interrupt */
            isp_imsc = HalReadReg( pDrvCtx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc) );
            isp_imsc &= ~MRV_ISP_IMSC_ISP_OFF_MASK;
            HalWriteReg( pDrvCtx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc), isp_imsc );

            pDrvCtx->pCapturingCompletionCb = NULL;
        }

        pHalIrqCtx->misValue &= ~( MRV_ISP_MIS_ISP_OFF_MASK );
    }

    if (  pHalIrqCtx->misValue & MRV_ISP_IMSC_FLASH_OFF_MASK ){
        if(pDrvCtx->pFlashContext->pIspFlashCb != NULL){
            pDrvCtx->pFlashContext->pIspFlashCb(pDrvCtx,CAMERIC_ISP_FLASH_OFF_EVENT,0);
        }
        pHalIrqCtx->misValue &= ~( MRV_ISP_IMSC_FLASH_OFF_MASK );
    }

    if (  pHalIrqCtx->misValue & MRV_ISP_IMSC_FLASH_ON_MASK ){
        if(pDrvCtx->pFlashContext->pIspFlashCb != NULL){
            pDrvCtx->pFlashContext->pIspFlashCb(pDrvCtx,CAMERIC_ISP_FLASH_ON_EVENT,0);
        }
        pHalIrqCtx->misValue &= ~( MRV_ISP_IMSC_FLASH_ON_MASK );
    }
    
	
    if (  pHalIrqCtx->misValue & MRV_ISP_MIS_DATA_LOSS_MASK )
    {
        
        pHalIrqCtx->misValue &= ~( MRV_ISP_MIS_DATA_LOSS_MASK );
		pDrvCtx->invalFrame = g_ispFrameNum+1;
		TRACE( CAMERIC_ISP_IRQ_ERROR, "\n\n\n\n\n\n\n\n\n%s: data loss first,g_ispFrameNum == %d\n\n\n\n\n\n\n\n", __FUNCTION__,g_ispFrameNum+1);
    }
	if (  pHalIrqCtx->misValue & MRV_ISP_MIS_PIC_SIZE_ERR_MASK )
    {
		volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)pDrvCtx->base;
		HalWriteReg( pDrvCtx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_err_clr), 0xffffffff );
        pHalIrqCtx->misValue &= ~(MRV_ISP_MIS_PIC_SIZE_ERR_MASK );
		pDrvCtx->invalFrame = g_ispFrameNum+1;
		TRACE( CAMERIC_ISP_IRQ_ERROR, "\n\n\n\n\n\n\n\n\n%s: pic err first,g_ispFrameNum == %d\n\n\n\n\n\n\n\n", __FUNCTION__,g_ispFrameNum+1);
    }
	
    if (  pHalIrqCtx->misValue & MRV_ISP_MIS_FRAME_MASK )
    {
         // OK, its an ugly hack, but...
        static uint64_t initTick = 0;
        static uint64_t lastTimeMs = 0;
        if (!initTick) initTick = osGetTick();
        uint32_t currTimeMs = (uint32_t)( ( ((float)(osGetTick() - initTick))/osGetFrequency() ) * 1000 );
        TRACE( CAMERIC_ISP_IRQ_INFO, "%s: -------------------------------- frame out (%d @ %d ms; dt = %d ms) --------------------------------\n", __FUNCTION__, g_ispFrameNum++, currTimeMs, currTimeMs - lastTimeMs );
        lastTimeMs = currTimeMs;
		//hkw add
		/*if (  pHalIrqCtx->misValue & MRV_ISP_MIS_DATA_LOSS_MASK )
	    {
	        TRACE( CAMERIC_ISP_IRQ_ERROR, "%s: data loss,g_ispFrameNum == %d\n", __FUNCTION__,g_ispFrameNum);
	        pHalIrqCtx->misValue &= ~( MRV_ISP_MIS_DATA_LOSS_MASK );
			pDrvCtx->invalFrame = g_ispFrameNum;
	    }*/
        if(pDrvCtx->pFlashContext->pIspFlashCb != NULL){
            pDrvCtx->pFlashContext->pIspFlashCb(pDrvCtx,CAMERIC_ISP_FLASH_CAPTURE_FRAME,g_ispFrameNum);
        }

        /* signal ISP-FRAME-OUT irq */
        if ( pDrvCtx->pIspContext->EventCb.func != NULL  )
        {
            TRACE( CAMERIC_ISP_IRQ_INFO, "%s: frame out\n", __FUNCTION__ );
            pDrvCtx->pIspContext->EventCb.func( CAMERIC_ISP_EVENT_FRAME_OUT, NULL, pDrvCtx->pIspContext->EventCb.pUserContext );
        }
        pHalIrqCtx->misValue &= ~( MRV_ISP_MIS_FRAME_MASK );
    }

#ifdef MRV_AWB_VERSION
    if (  pHalIrqCtx->misValue & MRV_ISP_MIS_AWB_DONE_MASK )
    {
        if ( (pDrvCtx->pIspAwbContext != NULL) && (pDrvCtx->pIspAwbContext->enabled == BOOL_TRUE) )
        {
            CamerIcIspAwbSignal( pDrvCtx );
        }
#ifndef MRV_ELAWB_VERSION
        pHalIrqCtx->misValue &= ~( MRV_ISP_MIS_AWB_DONE_MASK );
#endif /* MRV_ELAWB_VERSION */
    }
#endif /* MRV_AWB_VERSION */

#ifdef MRV_ELAWB_VERSION
    if (  pHalIrqCtx->misValue & MRV_ISP_MIS_AWB_DONE_MASK )
    {
        if ( (pDrvCtx->pIspElAwbContext != NULL) && (pDrvCtx->pIspElAwbContext->enabled == BOOL_TRUE) )
        {
            CamerIcIspElAwbSignal( pDrvCtx );
        }
        pHalIrqCtx->misValue &= ~( MRV_ISP_MIS_AWB_DONE_MASK );
    }
#endif /* MRV_ELAWB_VERSION */

    if (  pHalIrqCtx->misValue & MRV_ISP_MIS_FRAME_IN_MASK )
    {
        /* signal ISP-FRAME-IN irq */
        if ( pDrvCtx->pIspContext->EventCb.func != NULL  )
        {
            TRACE( CAMERIC_ISP_IRQ_INFO, "%s: frame in -> sensor v-sync\n", __FUNCTION__ );
            pDrvCtx->pIspContext->EventCb.func( CAMERIC_ISP_EVENT_FRAME_IN, NULL, NULL );
        }
        pHalIrqCtx->misValue &= ~( MRV_ISP_MIS_FRAME_IN_MASK );
    }

#ifdef MRV_AUTOFOCUS_VERSION
    if (  pHalIrqCtx->misValue & MRV_ISP_MIS_AFM_FIN_MASK )
    {
        if ( (pDrvCtx->pIspAfmContext != NULL) && (pDrvCtx->pIspAfmContext->enabled == BOOL_TRUE) )
        {
            CamerIcIspAfmSignal( CAMERIC_ISP_AFM_SIGNAL_MEASURMENT, pDrvCtx );
        }
        pHalIrqCtx->misValue &= ~( MRV_ISP_MIS_AFM_FIN_MASK );
    }

    if (  pHalIrqCtx->misValue & MRV_ISP_MIS_AFM_LUM_OF_MASK )
    {
        if ( (pDrvCtx->pIspAfmContext != NULL) && (pDrvCtx->pIspAfmContext->enabled == BOOL_TRUE) )
        {
            CamerIcIspAfmSignal( CAMERIC_ISP_AFM_SIGNAL_LUMA_OVERFLOW, pDrvCtx );
        }
        pHalIrqCtx->misValue &= ~( MRV_ISP_MIS_AFM_LUM_OF_MASK );
    }

    if (  pHalIrqCtx->misValue & MRV_ISP_MIS_AFM_SUM_OF_MASK )
    {
        if ( (pDrvCtx->pIspAfmContext != NULL) && (pDrvCtx->pIspAfmContext->enabled == BOOL_TRUE) )
        {
            CamerIcIspAfmSignal( CAMERIC_ISP_AFM_SIGNAL_SUM_OVERFLOW, pDrvCtx );
        }
        pHalIrqCtx->misValue &= ~( MRV_ISP_MIS_AFM_SUM_OF_MASK );
    }
#endif /* MRV_AUTOFOCUS_VERSION */

#ifdef MRV_HISTOGRAM_VERSION
    if ( pHalIrqCtx->misValue & MRV_ISP_MIS_HIST_MEASURE_RDY_MASK )
    {
        if ( (pDrvCtx->pIspHistContext != NULL) && (pDrvCtx->pIspHistContext->enabled == BOOL_TRUE) )
        {
            CamerIcIspHistSignal( pDrvCtx );
        }
        pHalIrqCtx->misValue &= ~( MRV_ISP_MIS_HIST_MEASURE_RDY_MASK );
    }
#endif /* MRV_HISTOGRAM_VERSION */

#ifdef MRV_AUTO_EXPOSURE_VERSION
    if (  pHalIrqCtx->misValue & MRV_ISP_MIS_EXP_END_MASK )
    {
        if ( (pDrvCtx->pIspExpContext != NULL) && (pDrvCtx->pIspExpContext->enabled == BOOL_TRUE) )
        {
            CamerIcIspExpSignal( pDrvCtx );
        }
        pHalIrqCtx->misValue &= ~( MRV_ISP_MIS_EXP_END_MASK );
    }
#endif /* MRV_AUTO_EXPOSURE_VERSION */

#ifdef MRV_VSM_VERSION
    if (  pHalIrqCtx->misValue & MRV_ISP_MIS_VSM_END_MASK )
    {
        if ( (pDrvCtx->pIspVsmContext != NULL) && (pDrvCtx->pIspVsmContext->enabled == BOOL_TRUE) )
        {
            CamerIcIspVsmSignal( pDrvCtx, CamerIcMiGetBufferId( pDrvCtx, CAMERIC_MI_PATH_MAIN ) );
        }
        pHalIrqCtx->misValue &= ~( MRV_ISP_MIS_VSM_END_MASK );
    }
#endif /* MRV_VSM_VERSION */


  //  TRACE( CAMERIC_ISP_IRQ_ERROR, "%s: (exit)\n", __FUNCTION__ );

    return ( 0 );
}



/******************************************************************************
 * Implementation of Driver Internal API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspInit()
 *****************************************************************************/
RESULT CamerIcIspInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;
	
	volatile MrvAllRegister_t *ptCamerIcRegMap;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    ctx->pIspContext = ( CamerIcIspContext_t *)malloc( sizeof(CamerIcIspContext_t) );
    if (  ctx->pIspContext == NULL )
    {
        TRACE( CAMERIC_ISP_DRV_ERROR,  "%s: Can't allocate CamerIC ISP context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pIspContext, 0, sizeof(CamerIcIspContext_t) );

	
	ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

	#if 0   /* ddl@rock-chips.com: v0.0x11.0 */
	HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_ircl), 0xffffffff );
    usleep(2000);
	HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_ircl), 0x0 );
    #else
    HalSetReset(ctx->HalHandle,HAL_DEVID_INTERNAL,1);
    usleep(2000);
    HalSetReset(ctx->HalHandle,HAL_DEVID_INTERNAL,0);
    #endif
    
    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspRelease()
 *****************************************************************************/
RESULT CamerIcIspRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pIspContext, 0, sizeof( CamerIcIspContext_t ) );
    free ( ctx->pIspContext);
    ctx->pIspContext = NULL;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspRegisterRequestCb()
 *****************************************************************************/
RESULT CamerIcIspRegisterRequestCb
(
    CamerIcDrvHandle_t      handle,
    CamerIcRequestFunc_t    func,
    void                    *pUserContext
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( ctx->pIspContext->RequestCb.func != NULL )
    {
        return ( RET_BUSY );
    }

    if ( func == NULL )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        ctx->pIspContext->RequestCb.func          = func;
        ctx->pIspContext->RequestCb.pUserContext  = pUserContext;
    }

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspDeRegisterRequestCb()
 *****************************************************************************/
RESULT CamerIcIspDeRegisterRequestCb
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    ctx->pIspContext->RequestCb.func          = NULL;
    ctx->pIspContext->RequestCb.pUserContext  = NULL;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void                *pUserContext
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->pIspContext->EventCb.func != NULL )
    {
        return ( RET_BUSY );
    }

    if ( func == NULL )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        ctx->pIspContext->EventCb.func          = func;
        ctx->pIspContext->EventCb.pUserContext  = pUserContext;
    }

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspDeRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pIspContext->EventCb.func          = NULL;
    ctx->pIspContext->EventCb.pUserContext  = NULL;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspStart()
 *****************************************************************************/
RESULT CamerIcIspStart
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;
    volatile MrvAllRegister_t *ptCamerIcRegMap;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* pointer to register map */
    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

    /* register IRQ handler */
    ctx->pIspContext->HalIrqCtx.misRegAddress = ( (ulong_t)&ptCamerIcRegMap->isp_mis );
    ctx->pIspContext->HalIrqCtx.icrRegAddress = ( (ulong_t)&ptCamerIcRegMap->isp_icr );


	ctx->invalFrame = 0;
    result = HalConnectIrq( ctx->HalHandle, &ctx->pIspContext->HalIrqCtx, 0, NULL, &CamerIcIspIrq, ctx );
    DCT_ASSERT( (result == RET_SUCCESS) );

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * CamerIcIspStop()
 *****************************************************************************/
RESULT CamerIcIspStop
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        /* stop interrupts of CamerIc ISP Module */
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc), 0UL );

        /* disconnect IRQ handler */
        result = HalDisconnectIrq( &ctx->pIspContext->HalIrqCtx );
    }

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * CamerIcIspStartCapturing()
 *****************************************************************************/
RESULT CamerIcIspStartCapturing
(
    CamerIcDrvHandle_t  handle,
    const uint32_t      numFrames
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;

    uint32_t isp_ctrl;
    uint32_t isp_imsc;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

	
    g_ispFrameNum = 0;

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->DriverState != CAMERIC_DRIVER_STATE_RUNNING )
    {
        return ( RET_WRONG_STATE );
    }

    /* range check, valid value are                     */
    /* 0                             : continuous mode  */
    /* 1 - MRV_ISP_ACQ_NR_FRAMES_MASK: counting mode    */
    if ( (numFrames > 0) && !(numFrames & MRV_ISP_ACQ_NR_FRAMES_MASK) )
    {
        return ( RET_OUTOFRANGE );
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

    /* write number of frames to capture */
    TRACE( CAMERIC_ISP_DRV_INFO, "%s: nFrames = %d (0x%08x)\n", __FUNCTION__, numFrames, (MRV_ISP_ACQ_NR_FRAMES_MASK & numFrames) );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_acq_nr_frames), (MRV_ISP_ACQ_NR_FRAMES_MASK & numFrames) );

    /* enable isp-off interrupt */
    isp_imsc = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc) );
    isp_imsc |= ( MRV_ISP_IMSC_ISP_OFF_MASK | MRV_ISP_IMSC_FRAME_MASK | MRV_ISP_IMSC_DATA_LOSS_MASK | MRV_ISP_IMSC_FRAME_IN_MASK | MRV_ISP_IMSC_PIC_SIZE_ERR_MASK)
                | ( MRV_ISP_IMSC_FLASH_CAP_MASK |MRV_ISP_IMSC_FLASH_OFF_MASK|MRV_ISP_IMSC_FLASH_ON_MASK );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc), isp_imsc );

    isp_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl) );

    //zyc for test 
   // REG_SET_SLICE(isp_ctrl, MRV_ISP_ISP_AWB_ENABLE, 0);
    
    /* frame synchronous update */
    REG_SET_SLICE(isp_ctrl, MRV_ISP_ISP_GEN_CFG_UPD, 1);
    REG_SET_SLICE(isp_ctrl, MRV_ISP_ISP_CFG_UPD, 1);

    /* enable input formatter */
    REG_SET_SLICE(isp_ctrl, MRV_ISP_ISP_INFORM_ENABLE, 1);


    REG_SET_SLICE(isp_ctrl, MRV_ISP_ISP_FLASH_MODE, 1);


    /* enable ISP */
    REG_SET_SLICE(isp_ctrl, MRV_ISP_ISP_ENABLE, 1);
    REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_CFG_UPD_PERMANENT, 1 );

    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl), isp_ctrl);

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspEnable()
 *****************************************************************************/
RESULT CamerIcIspEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_ctrl;

    TRACE( CAMERIC_ISP_DRV_ERROR, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->DriverState != CAMERIC_DRIVER_STATE_RUNNING )
    {
        return ( RET_WRONG_STATE );
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

    if ( (ctx->pCapturingCompletionCb != NULL) && (ctx->pCapturingCompletionCb->func != NULL) )
    {
        uint32_t isp_imsc = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc) );
        isp_imsc |= ( MRV_ISP_IMSC_ISP_OFF_MASK | MRV_ISP_IMSC_FRAME_MASK);
        TRACE( CAMERIC_ISP_DRV_INFO, "%s: isp_imsc: 0x%08x \n", __FUNCTION__, isp_imsc );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc), isp_imsc );
    }



    {
        uint32_t isp_imsc = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc) );
        isp_imsc |= ( MRV_ISP_IMSC_FLASH_CAP_MASK |MRV_ISP_IMSC_FLASH_OFF_MASK|MRV_ISP_IMSC_FLASH_ON_MASK);
        TRACE( CAMERIC_ISP_DRV_INFO, "%s: isp_imsc: 0x%08x \n", __FUNCTION__, isp_imsc );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc), isp_imsc );
    }
    

    isp_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl) );

    /* enable input formatter */
    REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_INFORM_ENABLE, 1 );

    /* enable ISP */
    REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_ENABLE, 1 );
    
    /* permanently send an update pulse through the ISP pipe */
    REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_CFG_UPD_PERMANENT, 1 );

    /* start ISP immediately */
    REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_CFG_UPD, 1 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl), isp_ctrl );

    TRACE( CAMERIC_ISP_DRV_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspDisable()
 *****************************************************************************/
RESULT CamerIcIspDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_ctrl;
    uint32_t isp_imsc;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->DriverState != CAMERIC_DRIVER_STATE_RUNNING )
    {
        return ( RET_WRONG_STATE );
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    isp_ctrl        = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl) );
    isp_imsc        = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc) );

    /* enable isp-off interrupt */
    isp_imsc |= ( MRV_ISP_IMSC_ISP_OFF_MASK | MRV_ISP_IMSC_FRAME_MASK );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc), isp_imsc );

    /* disable ISP */
    REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_ENABLE, 0 );

    /* disable input formatter */
    REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_INFORM_ENABLE, 0 );

    /* disable permanent update */
    REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_CFG_UPD_PERMANENT, 0 );

    /* frame synchronous update */
    REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_GEN_CFG_UPD, 1 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl), isp_ctrl );

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspSetMode()
 *****************************************************************************/
RESULT CamerIcIspSetMode
(
    CamerIcDrvHandle_t      handle,
    const CamerIcIspMode_t  mode
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_ctrl;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow mode changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    isp_ctrl        = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl) );

    switch ( mode )
    {
        case CAMERIC_ISP_MODE_RAW:
            {
                REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_MODE, MRV_ISP_ISP_MODE_RAW );
                break;
            }

        case CAMERIC_ISP_MODE_656:
            {
                REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_MODE, MRV_ISP_ISP_MODE_656 );
                break;
            }

        case CAMERIC_ISP_MODE_601:
            {
                REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_MODE, MRV_ISP_ISP_MODE_601 );
                break;
            }

        case CAMERIC_ISP_MODE_BAYER_RGB:
            {
                REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_MODE, MRV_ISP_ISP_MODE_RGB );
                break;
            }

        case CAMERIC_ISP_MODE_DATA:
            {
                REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_MODE, MRV_ISP_ISP_MODE_DATA );
                break;
            }

        case CAMERIC_ISP_MODE_RGB656:
            {
                REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_MODE, MRV_ISP_ISP_MODE_RGB656 );
                break;
            }

        case CAMERIC_ISP_MODE_RAW656:
            {
                REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_MODE, MRV_ISP_ISP_MODE_RAW656 );
                break;
            }

        default:
            {
                TRACE( CAMERIC_ISP_DRV_ERROR, "%s: unsopported mode(%d)\n", __FUNCTION__, mode );
                return ( RET_NOTSUPP );
            }
    }

    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl), isp_ctrl );

    ctx->pIspContext->IspMode = mode;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspSetAcqProperties()
 *****************************************************************************/
RESULT CamerIcIspSetAcqProperties
(
    CamerIcDrvHandle_t                  handle,
    const CamerIcIspSampleEdge_t        sampleEdge,
    const CamerIcIspPolarity_t          hSyncPol,
    const CamerIcIspPolarity_t          vSyncPol,
    const CamerIcIspBayerPattern_t      bayerPattern,
    const CamerIcIspColorSubsampling_t  subSampling,
    const CamerIcIspCCIRSequence_t      seqCCIR,
    const CamerIcIspFieldSelection_t    fieldSelection,
    const CamerIcIspInputSelection_t    inputSelection,
    const CamerIcIspLatencyFifo_t       latencyFifo
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_acq_prop;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (NULL == ctx) || (NULL == ctx->pIspContext) || (NULL == ctx->HalHandle) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow mode changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    isp_acq_prop    = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_acq_prop) );

    switch ( sampleEdge )
    {
        case CAMERIC_ISP_SAMPLE_EDGE_FALLING:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_SAMPLE_EDGE, 0);
                break;
            }

        case CAMERIC_ISP_SAMPLE_EDGE_RISING:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_SAMPLE_EDGE, 1);
                break;
            }

        default:
            {
                TRACE( CAMERIC_ISP_DRV_ERROR, "%s: unsopported triger edge (%d)\n", __FUNCTION__, sampleEdge );
                return ( RET_NOTSUPP );
            }
    }

    switch ( hSyncPol )
    {
        case CAMERIC_ISP_POLARITY_HIGH:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_HSYNC_POL, 0);
                break;
            }

        case CAMERIC_ISP_POLARITY_LOW:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_HSYNC_POL, 1);
                break;
            }

        default:
            {
                TRACE( CAMERIC_ISP_DRV_ERROR, "%s: unsopported hsync-polarity (%d)\n", __FUNCTION__, hSyncPol );
                return ( RET_NOTSUPP );
            }
    }

    switch ( vSyncPol )
    {
        case CAMERIC_ISP_POLARITY_HIGH:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_VSYNC_POL, 0);
                break;
            }

        case CAMERIC_ISP_POLARITY_LOW:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_VSYNC_POL, 1);
                break;
            }

        default:
            {
                TRACE( CAMERIC_ISP_DRV_ERROR, "%s: unsopported vsync-polarity (%d)\n", __FUNCTION__, vSyncPol );
                return ( RET_NOTSUPP );
            }
    }

    switch ( bayerPattern )
    {

        case CAMERIC_ISP_BAYER_PATTERN_RGRGGBGB:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_BAYER_PAT, MRV_ISP_BAYER_PAT_RG);
                break;
            }

        case CAMERIC_ISP_BAYER_PATTERN_GRGRBGBG:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_BAYER_PAT, MRV_ISP_BAYER_PAT_GR);
                break;
            }

        case CAMERIC_ISP_BAYER_PATTERN_GBGBRGRG:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_BAYER_PAT, MRV_ISP_BAYER_PAT_GB);
                break;
            }

        case CAMERIC_ISP_BAYER_PATTERN_BGBGGRGR:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_BAYER_PAT, MRV_ISP_BAYER_PAT_BG);
                break;
            }

        default:
            {
                TRACE( CAMERIC_ISP_DRV_ERROR, "%s: unsopported bayer pattern (%d)\n", __FUNCTION__, bayerPattern );
                return ( RET_NOTSUPP );
            }
    }

    switch ( subSampling )
    {
        case CAMERIC_ISP_CONV422_COSITED:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_CONV_422, MRV_ISP_CONV_422_CO);
                break;
            }

        case CAMERIC_ISP_CONV422_INTERLEAVED:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_CONV_422, MRV_ISP_CONV_422_INTER);
                break;
            }

        case CAMERIC_ISP_CONV422_NONCOSITED:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_CONV_422, MRV_ISP_CONV_422_NONCO);
                break;
            }

        default:
            {
                TRACE( CAMERIC_ISP_DRV_ERROR, "%s: ulConv422 not supported (%d) \n", __FUNCTION__, subSampling);
                return ( RET_NOTSUPP );
            }
    }

    switch ( seqCCIR )
    {
        case CAMERIC_ISP_CCIR_SEQUENCE_YCbYCr:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_CCIR_SEQ, MRV_ISP_CCIR_SEQ_YCBYCR);
                break;
            }

        case CAMERIC_ISP_CCIR_SEQUENCE_YCrYCb:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_CCIR_SEQ, MRV_ISP_CCIR_SEQ_YCRYCB);
                break;
            }

        case CAMERIC_ISP_CCIR_SEQUENCE_CbYCrY:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_CCIR_SEQ, MRV_ISP_CCIR_SEQ_CBYCRY);
                break;
            }

        case CAMERIC_ISP_CCIR_SEQUENCE_CrYCbY:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_CCIR_SEQ, MRV_ISP_CCIR_SEQ_CRYCBY);
                break;
            }

        default:
            {
                TRACE( CAMERIC_ISP_DRV_ERROR, "%s: CCIR sequence not supported (%d) \n", __FUNCTION__, seqCCIR );
                return ( RET_NOTSUPP );
            }
    }

    switch ( fieldSelection )
    {
        case CAMERIC_ISP_FIELD_SELECTION_BOTH:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_FIELD_SELECTION, MRV_ISP_FIELD_SELECTION_BOTH);
                break;
            }

        case CAMERIC_ISP_FIELD_SELECTION_EVEN:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_FIELD_SELECTION, MRV_ISP_FIELD_SELECTION_EVEN);
                break;
            }

        case CAMERIC_ISP_FIELD_SELECTION_ODD:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_FIELD_SELECTION, MRV_ISP_FIELD_SELECTION_ODD);
                break;
            }

        default:
            {
                TRACE( CAMERIC_ISP_DRV_ERROR, "%s: Field Selection not supported (%d)\n", __FUNCTION__, fieldSelection );
                return ( RET_NOTSUPP );
            }
    }

    switch ( inputSelection )
    {

        case CAMERIC_ISP_INPUT_12BIT:
            {
                /* 000- 12Bit external Interface */
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_INPUT_SELECTION, MRV_ISP_INPUT_SELECTION_12EXT);
                break;
            }

        case CAMERIC_ISP_INPUT_10BIT_ZZ:
            {
                /* 001- 10Bit Interface, append 2 zeroes as LSBs */
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_INPUT_SELECTION, MRV_ISP_INPUT_SELECTION_10ZERO);
                break;
            }
        case CAMERIC_ISP_INPUT_10BIT_EX:
            {
                /* 010- 10Bit Interface, append 2 MSBs as LSBs */
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_INPUT_SELECTION, MRV_ISP_INPUT_SELECTION_10MSB);
                break;
            }
        case CAMERIC_ISP_INPUT_8BIT_ZZ:
            {
                /* 011- 8Bit Interface, append 4 zeroes as LSBs */
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_INPUT_SELECTION, MRV_ISP_INPUT_SELECTION_8ZERO);
                break;
            }
        case CAMERIC_ISP_INPUT_8BIT_EX:
            {
                /* 100- 8Bit Interface, append 4 MSBs as LSBs */
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_INPUT_SELECTION, MRV_ISP_INPUT_SELECTION_8MSB);
                break;
            }
        default:
            {
                TRACE( CAMERIC_ISP_DRV_ERROR, "%s: Camera bus width not supported (%d)\n",  __FUNCTION__, inputSelection);
                return ( RET_NOTSUPP );
            }
    }

    switch ( latencyFifo )
    {
        case CAMERIC_ISP_LATENCY_FIFO_INPUT_FORMATTER:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_DMA_YUV_SELECTION, 0u );
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_DMA_RGB_SELECTION, 0u );
                break;
            }

        case CAMERIC_ISP_LATENCY_FIFO_DMA_READ_RAW:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_DMA_YUV_SELECTION, 0u );
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_DMA_RGB_SELECTION, 1u );
                break;
            }

        case CAMERIC_ISP_LATENCY_FIFO_DMA_READ_YUV:
            {
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_DMA_YUV_SELECTION, 1u );
                REG_SET_SLICE(isp_acq_prop, MRV_ISP_DMA_RGB_SELECTION, 0u );
                break;
            }

        default:
            {
                TRACE( CAMERIC_ISP_DRV_ERROR, "%s: Latency fifo setting not supported (%d)\n",  __FUNCTION__, latencyFifo);
                return ( RET_NOTSUPP );
            }
     }


    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_acq_prop), isp_acq_prop );

    ctx->pIspContext->sampleEdge        = sampleEdge;
    ctx->pIspContext->hSyncPol          = hSyncPol;
    ctx->pIspContext->vSyncPol          = vSyncPol;
    ctx->pIspContext->bayerPattern      = bayerPattern;
    ctx->pIspContext->subSampling       = subSampling;
    ctx->pIspContext->seqCCIR           = seqCCIR;
    ctx->pIspContext->fieldSelection    = fieldSelection;
    ctx->pIspContext->inputSelection    = inputSelection;
    ctx->pIspContext->latencyFifo       = latencyFifo;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return (  RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspGetAcqSampleEdge()
 *****************************************************************************/
RESULT CamerIcIspGetAcqSampleEdge
(
    CamerIcDrvHandle_t      handle,
    CamerIcIspSampleEdge_t  *pSampleEdge
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;
    
    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_acq_prop;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (NULL == ctx) || (NULL == ctx->pIspContext) || (NULL == ctx->HalHandle) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pSampleEdge )
    {
        return ( RET_INVALID_PARM );
    }

    /* read register value */
    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    isp_acq_prop    = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_acq_prop) );

    /* check register value */
    if ( !REG_GET_SLICE(isp_acq_prop, MRV_ISP_SAMPLE_EDGE) )
    {
        *pSampleEdge = CAMERIC_ISP_SAMPLE_EDGE_FALLING;
    }
    else
    {
        *pSampleEdge = CAMERIC_ISP_SAMPLE_EDGE_RISING;
    }
    
    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return (  RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspGetAcqBayerPattern()
 *****************************************************************************/
RESULT CamerIcIspGetAcqBayerPattern
(
    CamerIcDrvHandle_t          handle,
    CamerIcIspBayerPattern_t    *pBayerPattern
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;
    
    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_acq_prop;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (NULL == ctx) || (NULL == ctx->pIspContext) || (NULL == ctx->HalHandle) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pBayerPattern )
    {
        return ( RET_INVALID_PARM );
    }

    /* read register value */
    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    isp_acq_prop    = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_acq_prop) );

    /* check register value */
    switch ( REG_GET_SLICE(isp_acq_prop, MRV_ISP_BAYER_PAT) )
    {
        case MRV_ISP_BAYER_PAT_RG:
            {
                *pBayerPattern = CAMERIC_ISP_BAYER_PATTERN_RGRGGBGB;
                break;
            }

        case MRV_ISP_BAYER_PAT_GR:
            {
                *pBayerPattern = CAMERIC_ISP_BAYER_PATTERN_GRGRBGBG;
                break;
            }

        case MRV_ISP_BAYER_PAT_GB:
            {
                *pBayerPattern = CAMERIC_ISP_BAYER_PATTERN_GBGBRGRG;
                break;
            }

        case MRV_ISP_BAYER_PAT_BG:
            {
                *pBayerPattern = CAMERIC_ISP_BAYER_PATTERN_BGBGGRGR;
                break;
            }

        default:
            {
                *pBayerPattern = CAMERIC_ISP_BAYER_PATTERN_BGBGGRGR;
                break;
            }
    }

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return (  RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspSetAcqResolution()
 *****************************************************************************/
RESULT CamerIcIspSetAcqResolution
(
    CamerIcDrvHandle_t  handle,
    const uint16_t      hOffset,
    const uint16_t      vOffset,
    const uint16_t      width,
    const uint16_t      height
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)  %dx%d \n", __FUNCTION__,width,height);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow mode changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;    

    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_acq_h_offs), (hOffset & MRV_ISP_ACQ_H_OFFS_MASK) );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_acq_v_offs), (vOffset & MRV_ISP_ACQ_V_OFFS_MASK) );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_acq_h_size), (width   & MRV_ISP_ACQ_H_SIZE_MASK) );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_acq_v_size), (height  & MRV_ISP_ACQ_V_SIZE_MASK) );

    SetCamerIcWindow( &ctx->pIspContext->acqWindow, hOffset, vOffset, height, width );

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return (  RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspSetOutputFormatterResolution()
 *****************************************************************************/
RESULT CamerIcIspSetOutputFormatterResolution
(
    CamerIcDrvHandle_t  handle,
    const uint16_t      hOffset,
    const uint16_t      vOffset,
    const uint16_t      width,
    const uint16_t      height
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow mode changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_out_h_offs), (hOffset & MRV_ISP_ISP_OUT_H_OFFS_MASK) );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_out_v_offs), (vOffset & MRV_ISP_ISP_OUT_V_OFFS_MASK) );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_out_h_size), (width   & MRV_ISP_ISP_OUT_H_SIZE_MASK) );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_out_v_size), (height  & MRV_ISP_ISP_OUT_V_SIZE_MASK) );

    SetCamerIcWindow( &ctx->pIspContext->ofWindow, hOffset, vOffset, height, width );

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspSetImageStabilizationResolution()
 *****************************************************************************/
RESULT CamerIcIspSetImageStabilizationResolution
(
    CamerIcDrvHandle_t  handle,
    const uint16_t      hOffset,
    const uint16_t      vOffset,
    const uint16_t      width,
    const uint16_t      height
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow mode changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_h_offs), (hOffset & MRV_IS_IS_H_OFFS_MASK) );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_v_offs), (vOffset & MRV_IS_IS_V_OFFS_MASK) );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_h_size), (width   & MRV_IS_IS_H_SIZE_MASK) );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_v_size), (height  & MRV_IS_IS_V_SIZE_MASK) );

    SetCamerIcWindow( &ctx->pIspContext->isWindow, hOffset, vOffset, height, width );

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspGetDemosaic()
 *****************************************************************************/
RESULT CamerIcIspGetDemosaic
(
    CamerIcDrvHandle_t          handle,
    CamerIcIspDemosaicBypass_t  *pBypassMode,
    uint8_t                     *pDemosaicThreshold
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_INVALID )
    {
        return ( RET_WRONG_STATE );
    }

    if ( ( pBypassMode != NULL ) && ( pDemosaicThreshold != NULL ) )
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t isp_demosaic;

        isp_demosaic = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_demosaic) );

        *pBypassMode        = (0UL == REG_GET_SLICE( isp_demosaic, MRV_ISP_DEMOSAIC_BYPASS ) ) ? CAMERIC_ISP_DEMOSAIC_NORMAL_OPERATION : CAMERIC_ISP_DEMOSAIC_BYPASS;
        *pDemosaicThreshold = REG_GET_SLICE( isp_demosaic, MRV_ISP_DEMOSAIC_TH );
    }
    else
    {
        return ( RET_INVALID_PARM );
    }

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspSetDemosaic()
 *****************************************************************************/
RESULT CamerIcIspSetDemosaic
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspDemosaicBypass_t    BypassMode,
    const uint8_t                   DemosaicThreshold
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_demosaic;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_INVALID )
    {
        return ( RET_WRONG_STATE );
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    isp_demosaic    = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_demosaic) );

    /* set demosaic-mode */
    switch ( BypassMode )
    {
        case CAMERIC_ISP_DEMOSAIC_NORMAL_OPERATION:
            {
                REG_SET_SLICE( isp_demosaic, MRV_ISP_DEMOSAIC_BYPASS, 0U );
                break;
            }

        case CAMERIC_ISP_DEMOSAIC_BYPASS:
            {
                REG_SET_SLICE( isp_demosaic, MRV_ISP_DEMOSAIC_BYPASS, 1U );
                break;
            }

        default:
            {
                TRACE( CAMERIC_ISP_DRV_WARN, "%s: demosaicing mode not supported (%d)\n",  __FUNCTION__, BypassMode );
                return ( RET_NOTSUPP );
            }
    }

    /* set demosaic threshold */
    REG_SET_SLICE(isp_demosaic, MRV_ISP_DEMOSAIC_TH, DemosaicThreshold);

    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_demosaic), isp_demosaic);

    ctx->pIspContext->bypassMode        = BypassMode;
    ctx->pIspContext->demosaicThreshold = DemosaicThreshold;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspSetColorConversionRange()
 *****************************************************************************/
RESULT CamerIcIspSetColorConversionRange
(
    CamerIcDrvHandle_t                  handle,
    const CamerIcColorConversionRange_t YConvRange,
    const CamerIcColorConversionRange_t CrConvRange
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_ctrl;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_INVALID )
    {
        return ( RET_WRONG_STATE );
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    isp_ctrl        = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl) );

    switch ( YConvRange )
    {
        case CAMERIC_ISP_CCONV_RANGE_LIMITED_RANGE:
            {
                REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_CSM_Y_RANGE, MRV_ISP_ISP_CSM_Y_RANGE_BT601 );
                break;
            }

        case CAMERIC_ISP_CCONV_RANGE_FULL_RANGE:
            {
                REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_CSM_Y_RANGE, MRV_ISP_ISP_CSM_Y_RANGE_FULL );
                break;
            }

        default:
            {
                TRACE( CAMERIC_ISP_DRV_WARN, "%s: luminance conversion range not supported (%d)\n",  __FUNCTION__, YConvRange );
                return ( RET_NOTSUPP );
            }
    }

    switch ( CrConvRange )
    {
        case CAMERIC_ISP_CCONV_RANGE_LIMITED_RANGE:
            {
                REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_CSM_C_RANGE, MRV_ISP_ISP_CSM_C_RANGE_BT601 );
                break;
            }

        case CAMERIC_ISP_CCONV_RANGE_FULL_RANGE:
            {
                REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_CSM_C_RANGE, MRV_ISP_ISP_CSM_C_RANGE_FULL );
                break;
            }

        default:
            {
                TRACE( CAMERIC_ISP_DRV_WARN, "%s: crominance conversion range not supported (%d)\n",  __FUNCTION__, CrConvRange );
                return ( RET_NOTSUPP );
            }
    }
    
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl), isp_ctrl );

    ctx->pIspContext->YConvRange  = YConvRange;
    ctx->pIspContext->CrConvRange = CrConvRange;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspGetColorConversionCoefficients()
 *****************************************************************************/
RESULT CamerIcIspGetColorConversionCoefficients
(
    CamerIcDrvHandle_t  handle,
    CamerIc3x3Matrix_t  *pCConvCoefficients
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_INVALID )
    {
        return ( RET_WRONG_STATE );
    }

    if ( pCConvCoefficients != NULL )
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        pCConvCoefficients->Coeff[0]
            = (MRV_ISP_CC_COEFF_0_MASK & HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cc_coeff_0) ));
        pCConvCoefficients->Coeff[1]
            = (MRV_ISP_CC_COEFF_1_MASK & HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cc_coeff_1) ));
        pCConvCoefficients->Coeff[2]
            = (MRV_ISP_CC_COEFF_2_MASK & HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cc_coeff_2) ));
        pCConvCoefficients->Coeff[3]
            = (MRV_ISP_CC_COEFF_3_MASK & HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cc_coeff_3) ));
        pCConvCoefficients->Coeff[4]
            = (MRV_ISP_CC_COEFF_4_MASK & HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cc_coeff_4) ));
        pCConvCoefficients->Coeff[5]
            = (MRV_ISP_CC_COEFF_5_MASK & HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cc_coeff_5) ));
        pCConvCoefficients->Coeff[6]
            = (MRV_ISP_CC_COEFF_6_MASK & HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cc_coeff_6) ));
        pCConvCoefficients->Coeff[7]
            = (MRV_ISP_CC_COEFF_7_MASK & HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cc_coeff_7) ));
        pCConvCoefficients->Coeff[8]
            = (MRV_ISP_CC_COEFF_8_MASK & HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cc_coeff_8) ));
    }
    else
    {
        return ( RET_INVALID_PARM );
    }

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspSetColorConversionCoefficients()
 *****************************************************************************/
RESULT CamerIcIspSetColorConversionCoefficients
(
    CamerIcDrvHandle_t          handle,
    const CamerIc3x3Matrix_t    *pCConvCoefficients
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_INVALID )
    {
        return ( RET_WRONG_STATE );
    }

    if ( pCConvCoefficients != NULL )
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cc_coeff_0), 
                            (MRV_ISP_CC_COEFF_0_MASK & pCConvCoefficients->Coeff[0]) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cc_coeff_1),
                            (MRV_ISP_CC_COEFF_1_MASK & pCConvCoefficients->Coeff[1]) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cc_coeff_2),
                            (MRV_ISP_CC_COEFF_2_MASK & pCConvCoefficients->Coeff[2]) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cc_coeff_3),
                            (MRV_ISP_CC_COEFF_3_MASK & pCConvCoefficients->Coeff[3]) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cc_coeff_4),
                            (MRV_ISP_CC_COEFF_4_MASK & pCConvCoefficients->Coeff[4]) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cc_coeff_5),
                            (MRV_ISP_CC_COEFF_5_MASK & pCConvCoefficients->Coeff[5]) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cc_coeff_6),
                            (MRV_ISP_CC_COEFF_6_MASK & pCConvCoefficients->Coeff[6]) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cc_coeff_7),
                            (MRV_ISP_CC_COEFF_7_MASK & pCConvCoefficients->Coeff[7]) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_cc_coeff_8),
                            (MRV_ISP_CC_COEFF_8_MASK & pCConvCoefficients->Coeff[8]) );
    }
    else
    {
        return ( RET_INVALID_PARM );
    }

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspGetCrossTalkCoefficients()
 *****************************************************************************/
RESULT CamerIcIspGetCrossTalkCoefficients
(
    CamerIcDrvHandle_t  handle,
    CamerIc3x3Matrix_t  *pCTalkCoefficients
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_INVALID )
    {
        return ( RET_WRONG_STATE );
    }

    if ( pCTalkCoefficients!= NULL )
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t i = 0UL;

        for ( i = 0UL; i<CROSS_TALK_COEF_BLOCK_ARR_SIZE; i++ )
        {
            pCTalkCoefficients->Coeff[i]
                = ( MRV_ISP_CT_COEFF_MASK & HalReadReg(ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->cross_talk_coef_block_arr[i])) );
        }
    }
    else
    {
        return ( RET_INVALID_PARM );
    }

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspSetCrossTalkCoefficients()
 *****************************************************************************/
RESULT CamerIcIspSetCrossTalkCoefficients
(
    CamerIcDrvHandle_t          handle,
    const CamerIc3x3Matrix_t    *pCTalkCoefficients
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
       return ( RET_WRONG_HANDLE );

    }

    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_INVALID )
    {
        return ( RET_WRONG_STATE );
    }

    if ( pCTalkCoefficients != NULL )
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t i = 0UL;

        for ( i = 0UL; i<CROSS_TALK_COEF_BLOCK_ARR_SIZE; i++ )
        {
            HalWriteReg( ctx->HalHandle,
                    ((ulong_t)&ptCamerIcRegMap->cross_talk_coef_block_arr[i]),
                    (MRV_ISP_CT_COEFF_MASK & pCTalkCoefficients->Coeff[i]) );
        }
    }
    else
    {
        return ( RET_INVALID_PARM );
    }

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspGetCrossTalkOffset()
 *****************************************************************************/
RESULT CamerIcIspGetCrossTalkOffset
(
    CamerIcDrvHandle_t      handle,
    CamerIcXTalkOffset_t    *ptCrossTalkOffset
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_INVALID )
    {
        return ( RET_WRONG_STATE );
    }

    if ( ptCrossTalkOffset != NULL )
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        ptCrossTalkOffset->Red = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ct_offset_r) );
        ptCrossTalkOffset->Green = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ct_offset_g) );
        ptCrossTalkOffset->Blue = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ct_offset_b) );
    }
    else
    {
        return ( RET_INVALID_PARM );
    }

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspSetCrossTalkOffset()
 *****************************************************************************/
RESULT CamerIcIspSetCrossTalkOffset
(
    CamerIcDrvHandle_t          handle,
    const CamerIcXTalkOffset_t  *ptCrossTalkOffset
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_INVALID )
    {
        return ( RET_WRONG_STATE );
    }

    if ( ptCrossTalkOffset != NULL )
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ct_offset_r), (MRV_ISP_CT_OFFSET_R_MASK & ptCrossTalkOffset->Red) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ct_offset_g), (MRV_ISP_CT_OFFSET_G_MASK & ptCrossTalkOffset->Green) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ct_offset_b), (MRV_ISP_CT_OFFSET_B_MASK & ptCrossTalkOffset->Blue) );
    }
    else
    {
        return ( RET_INVALID_PARM );
    }

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspActivateWB()
 *****************************************************************************/
RESULT CamerIcIspActivateWB
(
    CamerIcDrvHandle_t          handle,
    const bool_t                enable
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_ctrl;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow mode changing if running */
    if ( ( ctx->DriverState != CAMERIC_DRIVER_STATE_INIT ) &&
         ( ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED ) )
    {
        return ( RET_WRONG_STATE );
    }

    ptCamerIcRegMap  = (MrvAllRegister_t *)ctx->base;
    isp_ctrl        = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl) );
    REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_AWB_ENABLE, (enable) ? 1UL : 0UL );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl), isp_ctrl );

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspIsWBActivated()
 *****************************************************************************/
RESULT CamerIcIspIsWBActivated
(
    CamerIcDrvHandle_t  handle,
    bool_t              *pIsEnabled
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_ctrl;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsEnabled == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_INVALID )
    {
        return ( RET_WRONG_STATE );
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    isp_ctrl        = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl) );
    *pIsEnabled = (1UL == REG_GET_SLICE( isp_ctrl, MRV_ISP_ISP_AWB_ENABLE ) ) ? BOOL_TRUE : BOOL_FALSE;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspGammaOutEnable()
 *****************************************************************************/
RESULT CamerIcIspGammaOutEnable
(
    CamerIcDrvHandle_t  handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl) );
        REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_GAMMA_OUT_ENABLE, 1U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl), isp_ctrl );
    }

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspGammaOutDisable()
 *****************************************************************************/
RESULT CamerIcIspGammaOutDisable
(
    CamerIcDrvHandle_t  handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_ctrl;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl) );
        REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_GAMMA_OUT_ENABLE, 0U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl), isp_ctrl );
    }

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspIsGammaOutActivated()
 *****************************************************************************/
RESULT CamerIcIspIsGammaOutActivated
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_ctrl;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsEnabled == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_INVALID )
    {
        return ( RET_WRONG_STATE );
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    isp_ctrl        = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl) );
    *pIsEnabled = (1UL == REG_GET_SLICE( isp_ctrl, MRV_ISP_ISP_GAMMA_OUT_ENABLE ) ) ? BOOL_TRUE : BOOL_FALSE;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspSetGammOutCurve()
 *****************************************************************************/
RESULT CamerIcIspSetGammOutCurve
(
    CamerIcDrvHandle_t                      handle,
    const CamerIcIspGammaSegmentationMode_t mode,
    const CamerIcIspGammaCurve_t            *pCurve
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    uint32_t isp_gamma_out_mode;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_INVALID )
    {
        return ( RET_WRONG_STATE );
    }

    if ( pCurve != NULL )
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_gamma_out_mode = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_gamma_out_mode) );

        int32_t i;

        /* check curve values before writing to registers */
        for ( i=0L; i < CAMERIC_ISP_GAMMA_CURVE_SIZE; i++ )
        {
            if ( pCurve->GammaY[i] > MRV_ISP_ISP_GAMMA_OUT_Y_MASK )
            {
                TRACE( CAMERIC_ISP_DRV_ERROR,  "%s: Gamma Correction curve value G_%d(%d) out of range (%d > %d)\n",
                            __FUNCTION__, i, pCurve->GammaY[i], pCurve->GammaY[i], MRV_ISP_ISP_GAMMA_OUT_Y_MASK  );
                return ( RET_OUTOFRANGE );
            }
        }

        switch ( mode )
        {
            case CAMERIC_ISP_SEGMENTATION_MODE_LOGARITHMIC:
                {
                    REG_SET_SLICE( isp_gamma_out_mode, MRV_ISP_EQU_SEGM, MRV_ISP_EQU_SEGM_LOG );
                    break;
                }

            case CAMERIC_ISP_SEGMENTATION_MODE_EQUIDISTANT:
                {
                    REG_SET_SLICE( isp_gamma_out_mode, MRV_ISP_EQU_SEGM,MRV_ISP_EQU_SEGM_EQU );
                    break;
                }

            default:
                {
                    return ( RET_INVALID_PARM );
                }
        }
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_gamma_out_mode), isp_gamma_out_mode );

        for ( i=0L; i < CAMERIC_ISP_GAMMA_CURVE_SIZE; i++ )
        {
             HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->gamma_out_y_block_arr[i]), (pCurve->GammaY[i] & MRV_ISP_ISP_GAMMA_OUT_Y_MASK) );
        }
    }
    else
    {
        return ( RET_INVALID_PARM );
    }

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspGetGammOutCurve()
 *****************************************************************************/
RESULT CamerIcIspGetGammOutCurve
(
    CamerIcDrvHandle_t                  handle,
    CamerIcIspGammaSegmentationMode_t   *pMode,
    CamerIcIspGammaCurve_t              *pCurve
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_INVALID )
    {
        return ( RET_WRONG_STATE );
    }

    if ( (NULL!=pCurve) && (NULL!=pMode) )
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_gamma_out_mode = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_gamma_out_mode) );
        
        MEMSET( pCurve, 0, sizeof(CamerIcIspGammaCurve_t) );

        int32_t i;

        *pMode = ( MRV_ISP_EQU_SEGM_LOG == REG_GET_SLICE( isp_gamma_out_mode, MRV_ISP_EQU_SEGM ) )
                    ? CAMERIC_ISP_SEGMENTATION_MODE_LOGARITHMIC
                    : CAMERIC_ISP_SEGMENTATION_MODE_EQUIDISTANT;

        /* check curve values before writing to registers */
        for ( i=0L; i < CAMERIC_ISP_GAMMA_CURVE_SIZE; i++ )
        {
            pCurve->GammaY[i] = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->gamma_out_y_block_arr[i]) );
        }
    }
    else
    {
        return ( RET_INVALID_PARM );
    }

    TRACE( CAMERIC_ISP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}

