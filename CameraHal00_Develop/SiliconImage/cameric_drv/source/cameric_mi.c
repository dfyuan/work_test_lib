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
 * @file cameric_mi.c
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

#include "cameric_simp_drv.h"
#include "cameric_scale_drv.h"
#include "cameric_drv_cb.h"
#include "cameric_drv.h"
#include "cameric_mi_drv.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_MI_DRV_INFO  , "CAMERIC-MI-DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_MI_DRV_WARN  , "CAMERIC-MI-DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_MI_DRV_ERROR , "CAMERIC-MI-DRV: ", ERROR   , 1 );

CREATE_TRACER( CAMERIC_MI_IRQ_INFO  , "CAMERIC-MI-IRQ: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_MI_IRQ_WARN  , "CAMERIC-MI-IRQ: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_MI_IRQ_ERROR , "CAMERIC-MI-IRQ: ", ERROR   , 1 );

#define CAMERIC_BUFFER_GAP  ( 0x0U )

//static osMutex buffer_mutex;
//static uint32_t g_miFrameNum = 0; // OK, its an ugly hack, but...

#define RKBUFFLAG "rkbufFlg"


/******************************************************************************
 * local type definitions
 *****************************************************************************/

/******************************************************************************
 * local variable declarations
 *****************************************************************************/

/******************************************************************************
 * local function prototypes
 *****************************************************************************/

/*****************************************************************************/
/**
 * @brief   This function is the CamerIc MI irq handler.
 *
 * @param   pArg    instance to the interrupt context, which is given 
 *                  during interrupt registration
 *
 * @return  Return the result of the function call.
 * @retval  0       interrupt successfully handled 
 *
 *****************************************************************************/
static int32_t CamerIcMiIrq
(
    void *pArg
);


/*****************************************************************************/
/**
 * @brief   This function checks/test the path configuration.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         configuration is ok.
 * @retval  RET_WRONG_CONFIG    wrong configuration
 * @retval  RET_INVALID_PARM    invalid parameter
 *
 *****************************************************************************/
static RESULT CamerIcMiCheckConfiguration
(
    CamerIcMiContext_t  *pMiContext
);


/*****************************************************************************/
/**
 * @brief   CamerIcSetupPictureBuffer
 *
 * @return  Return the result of the function call.
 *
 *****************************************************************************/
static RESULT CamerIcSetupPictureBuffer
(
    CamerIcDrvHandle_t      handle,
    const CamerIcMiPath_t   path,
    MediaBuffer_t           *pBuffer
);


/*****************************************************************************/
/**
 * @brief   CamerIcRequestAndSetupBuffers
 *
 * @return  Return the result of the function call.
 *
 *****************************************************************************/
static RESULT CamerIcRequestAndSetupBuffers
(
    CamerIcDrvHandle_t      handle,
    const CamerIcMiPath_t   path
);


/*****************************************************************************/
/**
 * @brief   CamerIcFlushAllBuffers
 *
 * @return  Return the result of the function call.
 *
 *****************************************************************************/
static RESULT CamerIcFlushAllBuffers
(
    const CamerIcMiPath_t   path,
    CamerIcMiContext_t      *ctx
);


/*****************************************************************************/
/**
 * @brief   Set/Program picture buffer addresses to CamerIc registers.
 *
 * @param   handle          CamerIc driver handle
 * @param   path            Path Index (main- or selfpath)
 * @param   pPicBuffer      Pointer to picture buffer
 *
 * @return                  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
static RESULT CamerIcMiSetBuffer
(
    CamerIcDrvHandle_t      handle,
    const CamerIcMiPath_t   path,
    const PicBufMetaData_t  *pPicBuffer
);


/******************************************************************************
 * local function implementation
 *****************************************************************************/

/******************************************************************************
 * CamerIcMiIrq()
 *****************************************************************************/
static int32_t CamerIcMiIrq
(
    void *pArg
)
{
    CamerIcDrvContext_t *pDrvCtx;       /* CamerIc Driver Context */
    HalIrqCtx_t         *pHalIrqCtx;    /* HAL context */

    TRACE( CAMERIC_MI_IRQ_INFO, "%s: (enter) \n", __FUNCTION__ );

    /* get IRQ context from args */
    pHalIrqCtx = (HalIrqCtx_t*)(pArg);
    DCT_ASSERT( (pHalIrqCtx != NULL) );

    pDrvCtx = (CamerIcDrvContext_t *)(pHalIrqCtx->OsIrq.p_context);
    DCT_ASSERT( (pDrvCtx != NULL) );
    DCT_ASSERT( (pDrvCtx->pMiContext != NULL) );

    volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)pDrvCtx->base;

    uint32_t numFramesToSkip = pDrvCtx->pMiContext->numFramesToSkip; /* get a copy once to use the copy multiple times "atomically" */

    TRACE( CAMERIC_MI_IRQ_INFO, "%s: (mis=%08x, skip=%d) \n", __FUNCTION__, pHalIrqCtx->misValue, numFramesToSkip);

    osMutexLock(&pDrvCtx->pMiContext->buffer_mutex);

    
    if ( pHalIrqCtx->misValue & ( MRV_MI_WRAP_MP_Y_MASK | MRV_MI_WRAP_MP_CB_MASK | MRV_MI_WRAP_MP_CR_MASK ) )
    {
        TRACE( CAMERIC_MI_IRQ_ERROR, "%s: wrap arround on MP (mis=%08x)\n", __FUNCTION__, pHalIrqCtx->misValue );
        pHalIrqCtx->misValue &= ~( MRV_MI_WRAP_MP_Y_MASK | MRV_MI_WRAP_MP_CB_MASK | MRV_MI_WRAP_MP_CR_MASK );
    }

    if ( pHalIrqCtx->misValue & ( MRV_MI_WRAP_SP_Y_MASK ) )
    {
        TRACE( CAMERIC_MI_IRQ_ERROR, "%s: wrap arround on SP(Y)\n", __FUNCTION__ );
        pHalIrqCtx->misValue &= ~( MRV_MI_WRAP_SP_Y_MASK );
    }

    if ( pHalIrqCtx->misValue & ( MRV_MI_WRAP_SP_CB_MASK ) )
    {
        TRACE( CAMERIC_MI_IRQ_ERROR, "%s: wrap arround on SP(Cb)\n", __FUNCTION__ );
        pHalIrqCtx->misValue &= ~( MRV_MI_WRAP_SP_CB_MASK );
    }

    if ( pHalIrqCtx->misValue & ( MRV_MI_WRAP_SP_CR_MASK ) )
    {
        TRACE( CAMERIC_MI_IRQ_ERROR, "%s: wrap arround on SP(Cr)\n", __FUNCTION__ );
        pHalIrqCtx->misValue &= ~( MRV_MI_WRAP_SP_CR_MASK );
    }

    if ( pHalIrqCtx->misValue & MRV_MI_FILL_MP_Y_MASK )
    {
        TRACE( CAMERIC_MI_IRQ_ERROR, "%s: fill level on MP\n", __FUNCTION__ );
        pHalIrqCtx->misValue &= ~( MRV_MI_FILL_MP_Y_MASK );
    }

    if ( pHalIrqCtx->misValue & MRV_MI_DMA_READY_MASK )
    {
        if ( (pDrvCtx->pDmaCompletionCb != NULL) && (pDrvCtx->pDmaCompletionCb->func != NULL) )
        {
            CamerIcCompletionCb_t *pCompleted = pDrvCtx->pDmaCompletionCb;

            uint32_t mi_imsc = 0UL;

            pDrvCtx->pDmaCompletionCb = NULL;

            /* disable dma interrupt */
            mi_imsc = HalReadReg( pDrvCtx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_imsc) );
            mi_imsc &= ~MRV_MI_DMA_READY_MASK;
            HalWriteReg( pDrvCtx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_imsc), mi_imsc );

            pCompleted->func( CAMERIC_MI_COMMAND_DMA_TRANSFER, RET_SUCCESS, pCompleted, pCompleted->pUserContext );
        }

        pHalIrqCtx->misValue &= ~( MRV_MI_DMA_READY_MASK );
    }

    if ( pHalIrqCtx->misValue & (MRV_MI_MP_FRAME_END_MASK | MRV_MI_SP_FRAME_END_MASK) )
    {
        CamerIcMiContext_t *pMiCtx = pDrvCtx->pMiContext;
        pMiCtx->miFrameNum++;
		TRACE( CAMERIC_MI_IRQ_INFO, "%s enter ,pDrvCtx->invalFrame == %d,miFrameNum == %d;numFramesToSkip == %d\n", __FUNCTION__,pDrvCtx->invalFrame,pMiCtx->miFrameNum,numFramesToSkip );
        // send EoF signal to super impose module 
        CamerIcSimpSignal( CAMERIC_SIMP_SIGNAL_END_OF_FRAME, pDrvCtx );
    
        if ( (pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].out_mode != CAMERIC_MI_DATAMODE_DISABLED) && (pHalIrqCtx->misValue & MRV_MI_MP_FRAME_END_MASK) )
        {
        	
			if ((pMiCtx->miFrameNum == pDrvCtx->invalFrame )  || ( numFramesToSkip != 0 ))
			//if ( numFramesToSkip != 0 ) 
            {
            REUSEBUFFER:
                if ( pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].pBuffer )
                {
                    RESULT result;

                    /* reuse buffer */
                    PicBufMetaData_t *pPicBuffer;


                    TRACE( CAMERIC_MI_IRQ_ERROR, "%s: MP buffer skipped (reused),pDrvCtx->invalFrame === %d,miFrameNum == %d;numFramesToSkip == %d\n", __FUNCTION__,pDrvCtx->invalFrame,pMiCtx->miFrameNum,numFramesToSkip );

                    MediaBuffer_t *pBuffer = pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].pShdBuffer;
                    pPicBuffer  = (PicBufMetaData_t *)pBuffer->pMetaData;

                    /* program buffer to hardware */
                    result =  CamerIcMiSetBuffer( pDrvCtx, CAMERIC_MI_PATH_MAIN, pPicBuffer );
                    DCT_ASSERT( RET_SUCCESS == result );

                    pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].pShdBuffer = pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].pBuffer;
                    pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].pBuffer = pBuffer;
                }
				
				pDrvCtx->invalFrame = 0;
				
            }
			else
            {
                uint32_t mi_mp_y_offs_cnt_shd = HalReadReg( pDrvCtx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_mp_y_offs_cnt_shd) );
                uint32_t mi_mp_cb_offs_cnt_shd = HalReadReg( pDrvCtx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_mp_cb_offs_cnt_shd) );
                uint32_t mi_mp_cr_offs_cnt_shd = HalReadReg( pDrvCtx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_mp_cr_offs_cnt_shd) );

                TRACE( CAMERIC_MI_IRQ_INFO, "%s: y-cnt = %d, %d %d\n", __FUNCTION__, mi_mp_y_offs_cnt_shd, mi_mp_cb_offs_cnt_shd, mi_mp_cr_offs_cnt_shd );

                /* last time we didn't get an empty buffer, so we set pBuf=pShdBuf  */
                /* and not delivering a full buffer until we got a second buffer    */ 
                
                if ( pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].pShdBuffer != pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].pBuffer )
                {
                    /* signal full buffer to upper software */
                    TRACE( CAMERIC_MI_IRQ_INFO, "%s: (call MainPathCtx.EventFrameEndCb [%p])\n", __FUNCTION__, pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].pShdBuffer->pBaseAddress );
                    if ( (pMiCtx->EventCb.func != NULL) && ( pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].pShdBuffer != NULL) )
                    {
                        PicBufMetaData_t * pPicBuffer = (PicBufMetaData_t *)pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].pShdBuffer->pMetaData;

                        //verify if this is a full buffer (may be wrong mi frame_end irq ?),or not ,zyc
                        void* y_addr_vir;
                        HalMapMemory( pDrvCtx->HalHandle, (ulong_t)(pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].pShdBuffer->pBaseAddress), 100, HAL_MAPMEM_READWRITE, &y_addr_vir );
                        if(strncmp(RKBUFFLAG,(char*)y_addr_vir,strlen(RKBUFFLAG)) == 0){
                            TRACE( CAMERIC_MI_IRQ_WARN,"pBuf: %p(base:%p) not filled , so reused!!",
                                pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].pShdBuffer,
                                pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].pShdBuffer->pBaseAddress);
                            goto REUSEBUFFER;    /* ddl@rock-chips.com : v0.0x30.0 */
                        }

                        if ( pPicBuffer->Type == PIC_BUF_TYPE_JPEG )
                        {
                            uint32_t mi_byte_cnt =  HalReadReg( pDrvCtx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_byte_cnt) );
                            uint32_t size   = REG_GET_SLICE( mi_byte_cnt, MRV_MI_BYTE_CNT );

                            pPicBuffer->Data.jpeg.pHeader    = NULL;
                            pPicBuffer->Data.jpeg.HeaderSize = 0;
                            pPicBuffer->Data.jpeg.DataSize   = size;
                        }

                        if(pDrvCtx->pFlashContext->mode == CAMERIC_ISP_FLASH_AUTO
                            || pDrvCtx->pFlashContext->mode == CAMERIC_ISP_FLASH_ON
                            || pDrvCtx->pFlashContext->mode == CAMERIC_ISP_FLASH_TORCH){
                            if((pDrvCtx->pFlashContext->flash_desired_frame_start != 0xffffffff)
                                && (pMiCtx->miFrameNum >= pDrvCtx->pFlashContext->flash_desired_frame_start )
                                && (pDrvCtx->pFlashContext->pIspFlashCb)) {
                                TRACE( CAMERIC_MI_IRQ_INFO, "%s: mi flash got the desired frame:%d(%d)!\n", __FUNCTION__ ,pMiCtx->miFrameNum,pDrvCtx->pFlashContext->flash_desired_frame_start);
                                pDrvCtx->pFlashContext->pIspFlashCb(pDrvCtx,CAMERIC_ISP_FLASH_STOP_EVENT,0);
                                pPicBuffer->priv =(void*)1; 
                            }else{
                                TRACE( CAMERIC_MI_IRQ_INFO, "%s: mi flash got the  frame:%d(%d)!\n", __FUNCTION__ ,pMiCtx->miFrameNum,pDrvCtx->pFlashContext->flash_desired_frame_start);
                                pPicBuffer->priv =(void*)0; 
                            }

                        }
                        pMiCtx->EventCb.func( CAMERIC_MI_EVENT_FULL_MP_BUFFER,
                                (void *)pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].pShdBuffer,
                                pMiCtx->EventCb.pUserContext );
                    }
                }
                else
                {
                    DROPBUFFER:
                    pMiCtx->EventCb.func( CAMERIC_MI_EVENT_DROPPED_MP_BUFFER,
                            (void *)NULL,
                            pMiCtx->EventCb.pUserContext );
                }

                /* request an empty buffer from upper software */
                if ( pMiCtx->RequestCb.func != NULL )
                {
                    RESULT result;
                    void *pParam;
                    void* y_addr_vir = NULL;

                    TRACE( CAMERIC_MI_IRQ_INFO, "%s: (call MainPathCtx.RequestEmptyBufferCb)\n", __FUNCTION__ );
                    result = pMiCtx->RequestCb.func( CAMERIC_MI_REQUEST_GET_EMPTY_MP_BUFFER, &pParam, pMiCtx->EventCb.pUserContext );
                    if ( RET_SUCCESS == result )
                    {
                        /* we recieved en empty buffer */
                        PicBufMetaData_t *pPicBuffer;

                        MediaBuffer_t *pBuffer = (MediaBuffer_t *)pParam;

                        TRACE( CAMERIC_MI_IRQ_INFO, "%s: MP received %p\n", __FUNCTION__, pBuffer->pBaseAddress );

                        //memset buffer
                        HalMapMemory( pDrvCtx->HalHandle, (ulong_t)(pBuffer->pBaseAddress), 100, HAL_MAPMEM_READWRITE, &y_addr_vir );
						memcpy(y_addr_vir,RKBUFFLAG,strlen(RKBUFFLAG));
                        result = CamerIcSetupPictureBuffer( pDrvCtx, CAMERIC_MI_PATH_MAIN, pBuffer );
                        DCT_ASSERT( RET_SUCCESS == result );

                        pPicBuffer  = (PicBufMetaData_t *)pBuffer->pMetaData;

                        /* program buffer to hardware */
                        result =  CamerIcMiSetBuffer( pDrvCtx, CAMERIC_MI_PATH_MAIN, pPicBuffer );
                        DCT_ASSERT(  RET_SUCCESS == result );

                        /* swap the buffers */
                        pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].pShdBuffer = pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].pBuffer;
                        pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].pBuffer = pBuffer;
                    }
                    else if ( RET_NOTAVAILABLE ==  result )
                    {
                        /* an empty buffer is not available */
                        PicBufMetaData_t *pPicBuffer;

                        TRACE( CAMERIC_MI_IRQ_INFO, "%s: MP no buffer received\n", __FUNCTION__ );

                        MediaBuffer_t *pBuffer = pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].pBuffer;
                        pPicBuffer  = (PicBufMetaData_t *)pBuffer->pMetaData;

                        /* program buffer to hardware */
                        result =  CamerIcMiSetBuffer( pDrvCtx, CAMERIC_MI_PATH_MAIN, pPicBuffer );
                        DCT_ASSERT(  RET_SUCCESS == result );

                        pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].pShdBuffer = pMiCtx->PathCtx[CAMERIC_MI_PATH_MAIN].pBuffer;
                    }
                    else
                    {
                        osMutexUnlock(&pMiCtx->buffer_mutex);
                        return ( result );
                    }
                }
            }
            

            pHalIrqCtx->misValue &= ~( MRV_MI_MP_FRAME_END_MASK );
        }

        if ( (pMiCtx->PathCtx[CAMERIC_MI_PATH_SELF].out_mode != CAMERIC_MI_DATAMODE_DISABLED) && ( pHalIrqCtx->misValue & MRV_MI_SP_FRAME_END_MASK ) )
        {
            if ( numFramesToSkip == 0 )
            {
                /* last time we didn't get an empty buffer, so we set pBuf=pShdBuf  */
                /* and not delivering a full buffer until we got a second buffer    */
                if ( pMiCtx->PathCtx[CAMERIC_MI_PATH_SELF].pShdBuffer != pMiCtx->PathCtx[CAMERIC_MI_PATH_SELF].pBuffer )
                {
                    /* signal full buffer to upper software */
                    TRACE( CAMERIC_MI_IRQ_INFO, "%s: (call SelfPathCtx.EventFrameEndCb [%p])\n", __FUNCTION__, pMiCtx->PathCtx[CAMERIC_MI_PATH_SELF].pShdBuffer->pBaseAddress );
                    if ( (pMiCtx->EventCb.func != NULL) && ( pMiCtx->PathCtx[CAMERIC_MI_PATH_SELF].pShdBuffer != NULL) )
                    {
                        pMiCtx->EventCb.func( CAMERIC_MI_EVENT_FULL_SP_BUFFER,
                                (void *)pMiCtx->PathCtx[CAMERIC_MI_PATH_SELF].pShdBuffer,
                                pMiCtx->EventCb.pUserContext );
                    }
                }
                else
                {
                    pMiCtx->EventCb.func( CAMERIC_MI_EVENT_DROPPED_SP_BUFFER,
                            (void *)NULL,
                            pMiCtx->EventCb.pUserContext );
                }

                /* request an empty buffer from upper software */
                if ( pMiCtx->RequestCb.func != NULL )
                {
                    RESULT result;
                    void *pParam;

                    TRACE( CAMERIC_MI_IRQ_INFO, "%s: (call SelfPathCtx.RequestEmptyBufferCb)\n", __FUNCTION__ );
                    result = pMiCtx->RequestCb.func( CAMERIC_MI_REQUEST_GET_EMPTY_SP_BUFFER, &pParam, pMiCtx->EventCb.pUserContext );
                    if ( RET_SUCCESS == result )
                    {
                        /* we recieved en empty buffer */
                        PicBufMetaData_t *pPicBuffer;

                        MediaBuffer_t *pBuffer = (MediaBuffer_t *)pParam;

                        TRACE( CAMERIC_MI_IRQ_INFO, "%s: SP received 0x%p\n", __FUNCTION__, pBuffer->pBaseAddress );

                        result = CamerIcSetupPictureBuffer( pDrvCtx, CAMERIC_MI_PATH_SELF, pBuffer );
                        DCT_ASSERT( RET_SUCCESS == result );

                        pPicBuffer  = (PicBufMetaData_t *)pBuffer->pMetaData;

                        /* program buffer to hardware */
                        result =  CamerIcMiSetBuffer( pDrvCtx, CAMERIC_MI_PATH_SELF, pPicBuffer );
                        DCT_ASSERT(  RET_SUCCESS == result );

                        /* swap the buffers */
                        pMiCtx->PathCtx[CAMERIC_MI_PATH_SELF].pShdBuffer = pMiCtx->PathCtx[CAMERIC_MI_PATH_SELF].pBuffer;
                        pMiCtx->PathCtx[CAMERIC_MI_PATH_SELF].pBuffer = pBuffer;
                    }
                    else if ( RET_NOTAVAILABLE ==  result )
                    {
                        /* an empty buffer is not available */
                        PicBufMetaData_t *pPicBuffer;

                        TRACE( CAMERIC_MI_IRQ_INFO, "%s: SP no buffer received\n", __FUNCTION__ );

                        MediaBuffer_t *pBuffer = pMiCtx->PathCtx[CAMERIC_MI_PATH_SELF].pBuffer;
                        pPicBuffer  = (PicBufMetaData_t *)pBuffer->pMetaData;

                        /* program buffer to hardware */
                        result =  CamerIcMiSetBuffer( pDrvCtx, CAMERIC_MI_PATH_SELF, pPicBuffer );
                        DCT_ASSERT(  RET_SUCCESS == result );

                        pMiCtx->PathCtx[CAMERIC_MI_PATH_SELF].pShdBuffer = pMiCtx->PathCtx[CAMERIC_MI_PATH_SELF].pBuffer;
                    }
                    else
                    {
                        osMutexUnlock(&pMiCtx->buffer_mutex);
                        return ( result );
                    }
                }
            }
            else
            {
                if ( pMiCtx->PathCtx[CAMERIC_MI_PATH_SELF].pBuffer )
                {
                    RESULT result;

                    /* reuse buffer */
                    PicBufMetaData_t *pPicBuffer;

                    TRACE( CAMERIC_MI_IRQ_INFO, "%s: SP buffer skipped (reused)\n", __FUNCTION__ );

                    MediaBuffer_t *pBuffer = pMiCtx->PathCtx[CAMERIC_MI_PATH_SELF].pShdBuffer;
                    pPicBuffer  = (PicBufMetaData_t *)pBuffer->pMetaData;

                    /* program buffer to hardware */
                    result =  CamerIcMiSetBuffer( pDrvCtx, CAMERIC_MI_PATH_SELF, pPicBuffer );
                    DCT_ASSERT( RET_SUCCESS == result );

                    pMiCtx->PathCtx[CAMERIC_MI_PATH_SELF].pShdBuffer = pMiCtx->PathCtx[CAMERIC_MI_PATH_SELF].pBuffer;
                    pMiCtx->PathCtx[CAMERIC_MI_PATH_SELF].pBuffer = pBuffer;
                }
            }

            pHalIrqCtx->misValue &= ~( MRV_MI_SP_FRAME_END_MASK );
        }
    }

    if (  pHalIrqCtx->misValue != 0U )
    {
        TRACE( CAMERIC_MI_IRQ_ERROR, "%s: 0x%08x\n", __FUNCTION__, pHalIrqCtx->misValue );
    }

    /* this must be last in IRQ handler, since osAtomicDecrement() may block, although for a very short time only */
    if ( numFramesToSkip != 0 )
    {
        (void) osAtomicDecrement( &pDrvCtx->pMiContext->numFramesToSkip );
    }

    TRACE( CAMERIC_MI_IRQ_INFO, "%s: (exit)\n", __FUNCTION__ );
    osMutexUnlock(&pDrvCtx->pMiContext->buffer_mutex);
    return ( 0 );
}


/******************************************************************************
 * CamerIcMiCheckConfiguration()
 *****************************************************************************/
static RESULT CamerIcMiCheckConfiguration
(
    CamerIcMiContext_t  *pMiContext
)
{
    RESULT result = RET_WRONG_CONFIG;

    /* check configured burst length */
    if ( (pMiContext->y_burstlength < CAMERIC_MI_BURSTLENGTH_4)
            || (pMiContext->y_burstlength > CAMERIC_MI_BURSTLENGTH_16) )
    {
        return ( result );
    }

    if ( (pMiContext->c_burstlength < CAMERIC_MI_BURSTLENGTH_4)
            || (pMiContext->c_burstlength > CAMERIC_MI_BURSTLENGTH_16) )
    {
        return ( result );
    }

    /* check possible combinations of path configurations */
    if ( (pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].out_mode == CAMERIC_MI_DATAMODE_DISABLED)
            && ( (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_DISABLED)
                    || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_YUV444)
                    || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_YUV422)
                    || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_YUV420)
                    || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_YUV400)
                    || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_RGB888)
                    || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_RGB666)
                    || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_RGB565)
                    || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_DPCC) ) )
    {
        /* valid setting */
        result = RET_SUCCESS;
    }
    else if ( ((pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].out_mode == CAMERIC_MI_DATAMODE_YUV422)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].out_mode == CAMERIC_MI_DATAMODE_YUV420))
                && ( (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_DISABLED)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_YUV444)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_YUV422)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_YUV420)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_YUV400)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_RGB888)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_RGB666)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_RGB565)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_DPCC) ) )
    {
        /* valid setting */
        result = RET_SUCCESS;
    }
    else if ( (pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].out_mode == CAMERIC_MI_DATAMODE_JPEG)
                && ( (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_DISABLED)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_YUV444)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_YUV422)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_YUV420)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_YUV400)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_RGB888)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_RGB666)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_RGB565)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_DPCC) ) )
    {
        /* valid setting */
        result = RET_SUCCESS;
    }
    else if ( (pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].out_mode == CAMERIC_MI_DATAMODE_DPCC)
                && ( (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_DISABLED)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_YUV444)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_YUV422)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_YUV420)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_YUV400)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_RGB888)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_RGB666)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_RGB565) ) )
    {
        /* valid setting */
        result = RET_SUCCESS;
    }
    else if ( ((pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].out_mode == CAMERIC_MI_DATAMODE_RAW8)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].out_mode == CAMERIC_MI_DATAMODE_RAW12))
                && ( (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_DISABLED)
                        || (pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_DPCC) ) )
    {
        /* valid setting */
        result = RET_SUCCESS;
    }
    else
    {
        /* invalid setting */
        return ( RET_INVALID_PARM );
    }

    return ( result );
}


/******************************************************************************
 * CamerIcRequestAndSetupBuffers()
 *****************************************************************************/
static RESULT CamerIcRequestAndSetupBuffers
(
    CamerIcDrvHandle_t      handle,
    const CamerIcMiPath_t   path
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    int32_t reqId;
    CamerIcMiContext_t  *pMiCtx;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    DCT_ASSERT( (ctx != NULL) && (ctx->pMiContext != NULL) );
    DCT_ASSERT( ((path == CAMERIC_MI_PATH_MAIN) || (path == CAMERIC_MI_PATH_SELF)) );

    pMiCtx = ctx->pMiContext;
    reqId = ( path == CAMERIC_MI_PATH_MAIN ) ? CAMERIC_MI_REQUEST_GET_EMPTY_MP_BUFFER : CAMERIC_MI_REQUEST_GET_EMPTY_SP_BUFFER;

    /* check if request callback registered */
    if ( pMiCtx->RequestCb.func != NULL )
    {
        void *pParam = NULL;

        /* call to request the shadow buffer */
        result = pMiCtx->RequestCb.func( reqId, &pParam, pMiCtx->RequestCb.pUserContext );
        DCT_ASSERT( ((result == RET_SUCCESS) && (pParam != NULL))  );

        pMiCtx->PathCtx[path].pShdBuffer = (MediaBuffer_t *)pParam;
        result = CamerIcSetupPictureBuffer( ctx, path, pMiCtx->PathCtx[path].pShdBuffer );
        DCT_ASSERT( result == RET_SUCCESS );

        TRACE( CAMERIC_MI_DRV_INFO, "%s: (got empty Buffer [%p])\n", __FUNCTION__, pMiCtx->PathCtx[path].pShdBuffer->pBaseAddress );

        /* call to request the second buffer */
        pParam = NULL;
        result = pMiCtx->RequestCb.func( reqId, &pParam, pMiCtx->RequestCb.pUserContext );
        TRACE( CAMERIC_MI_DRV_INFO, "%s: (got empty Buffer: %d)\n", __FUNCTION__, result );
        DCT_ASSERT( ((result == RET_SUCCESS) && (pParam!=NULL))  );

        pMiCtx->PathCtx[path].pBuffer = (MediaBuffer_t *)pParam;
        result = CamerIcSetupPictureBuffer( ctx, path, pMiCtx->PathCtx[path].pBuffer );
        DCT_ASSERT( result == RET_SUCCESS );

        TRACE( CAMERIC_MI_DRV_INFO, "%s: (got empty Buffer [%p])\n", __FUNCTION__, pMiCtx->PathCtx[path].pBuffer->pBaseAddress );
    }
    else
    {
        TRACE( CAMERIC_MI_DRV_ERROR, "%s: request handler not registered\n", __FUNCTION__);
        return ( RET_FAILURE );
    }

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/******************************************************************************
 * CamerIcFlushAllBuffers()
 *****************************************************************************/
static RESULT CamerIcFlushAllBuffers
(
    const CamerIcMiPath_t   path,
    CamerIcMiContext_t      *ctx
)
{
    RESULT result = RET_SUCCESS;

    int32_t evtId;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    DCT_ASSERT( (ctx != NULL) );
    DCT_ASSERT( ((path == CAMERIC_MI_PATH_MAIN) || (path == CAMERIC_MI_PATH_SELF)) );

    evtId = ( path == CAMERIC_MI_PATH_MAIN ) ? CAMERIC_MI_EVENT_FLUSHED_MP_BUFFER : CAMERIC_MI_EVENT_FLUSHED_SP_BUFFER;

    if ( (ctx != NULL)
            && (ctx->EventCb.func != NULL)
            && (ctx->PathCtx[path].out_mode != CAMERIC_MI_DATAMODE_DISABLED)
            && (ctx->PathCtx[path].pShdBuffer != NULL)
            && (ctx->PathCtx[path].pBuffer != NULL) )
    {
        if ( ctx->PathCtx[path].pShdBuffer != ctx->PathCtx[path].pBuffer )
        {
            ctx->EventCb.func( evtId, (void *)ctx->PathCtx[path].pShdBuffer, ctx->EventCb.pUserContext );
        }

        ctx->EventCb.func( evtId, (void *)ctx->PathCtx[path].pBuffer, ctx->EventCb.pUserContext );

        ctx->PathCtx[path].pShdBuffer   = NULL;
        ctx->PathCtx[path].pBuffer      = NULL;
    }

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/******************************************************************************
 * CamerIcSetupPictureBuffer()
 *****************************************************************************/
static RESULT CamerIcSetupPictureBuffer
(
    CamerIcDrvHandle_t      handle,
    const CamerIcMiPath_t   path,
    MediaBuffer_t           *pBuffer
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    PicBufMetaData_t *pPicBuffer;
    CamerIcMiDataPathContext_t *pMiPathCtx;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    DCT_ASSERT( ((ctx != NULL) && (ctx->pMiContext != NULL) && (ctx->pIspContext != NULL)) );
    DCT_ASSERT( ((path == CAMERIC_MI_PATH_MAIN) || (path == CAMERIC_MI_PATH_SELF)) );
    DCT_ASSERT( ((pBuffer != NULL) && (pBuffer->pMetaData != NULL)) );

    pMiPathCtx = &ctx->pMiContext->PathCtx[path];
    DCT_ASSERT( (pMiPathCtx != NULL) );
    pPicBuffer = (PicBufMetaData_t *)pBuffer->pMetaData;
    pPicBuffer->Align = CAMERIC_ALIGN_SIZE;

    switch ( pMiPathCtx->out_mode )
    {
        case CAMERIC_MI_DATAMODE_YUV420:
            {
                pPicBuffer->Type = PIC_BUF_TYPE_YCbCr420;

                if ( pMiPathCtx->datalayout == CAMERIC_MI_DATASTORAGE_PLANAR )
                {
                    pPicBuffer->Layout = PIC_BUF_LAYOUT_PLANAR;

                    /* luminace plane (Y) */
                    pPicBuffer->Data.YCbCr.planar.Y.pBuffer         = (uint8_t *)pBuffer->pBaseAddress;
                    pPicBuffer->Data.YCbCr.planar.Y.PicWidthPixel   = pMiPathCtx->out_width;
                    pPicBuffer->Data.YCbCr.planar.Y.PicHeightPixel  = pMiPathCtx->out_height;
                    pPicBuffer->Data.YCbCr.planar.Y.PicWidthBytes   = pMiPathCtx->out_width;

                    /* first color plane (Cb) (check alignment and addresses are in buffer) */
                    pPicBuffer->Data.YCbCr.planar.Cb.pBuffer
                        = (uint8_t *)( CAMERIC_MI_ALIGN((ulong_t)pBuffer->pBaseAddress
                                       + (pMiPathCtx->out_width * pMiPathCtx->out_height) + CAMERIC_BUFFER_GAP) );
                    pPicBuffer->Data.YCbCr.planar.Cb.PicWidthPixel   = (pMiPathCtx->out_width >> 1);
                    pPicBuffer->Data.YCbCr.planar.Cb.PicHeightPixel  = (pMiPathCtx->out_height >> 1);
                    pPicBuffer->Data.YCbCr.planar.Cb.PicWidthBytes   = (pMiPathCtx->out_width >> 1);

                    /* second color plane (Cr) (check alignment and addresses are in buffer) */
                    pPicBuffer->Data.YCbCr.planar.Cr.pBuffer
                        = (uint8_t *)( CAMERIC_MI_ALIGN((ulong_t)pPicBuffer->Data.YCbCr.planar.Cb.pBuffer
                                       + ((pMiPathCtx->out_width * pMiPathCtx->out_height)>>2) + CAMERIC_BUFFER_GAP) );
                    pPicBuffer->Data.YCbCr.planar.Cr.PicWidthPixel   = (pMiPathCtx->out_width >> 1);
                    pPicBuffer->Data.YCbCr.planar.Cr.PicHeightPixel  = (pMiPathCtx->out_height >> 1);
                    pPicBuffer->Data.YCbCr.planar.Cr.PicWidthBytes   = (pMiPathCtx->out_width >> 1);
                }
                else if ( pMiPathCtx->datalayout == CAMERIC_MI_DATASTORAGE_SEMIPLANAR )
                {
                    pPicBuffer->Layout = PIC_BUF_LAYOUT_SEMIPLANAR;

                    /* luminance plane (Y) */
                    pPicBuffer->Data.YCbCr.semiplanar.Y.pBuffer             = (uint8_t *)pBuffer->pBaseAddress;
                    pPicBuffer->Data.YCbCr.semiplanar.Y.PicWidthPixel       = pMiPathCtx->out_width;
                    pPicBuffer->Data.YCbCr.semiplanar.Y.PicHeightPixel      = pMiPathCtx->out_height;
                    pPicBuffer->Data.YCbCr.semiplanar.Y.PicWidthBytes       = pMiPathCtx->out_width;

                    /* color plane (CbCr) */
                    pPicBuffer->Data.YCbCr.semiplanar.CbCr.pBuffer
                        = (uint8_t *)( CAMERIC_MI_ALIGN((ulong_t)pBuffer->pBaseAddress 
                                       + (pMiPathCtx->out_width * pMiPathCtx->out_height) + CAMERIC_BUFFER_GAP) );
                    pPicBuffer->Data.YCbCr.semiplanar.CbCr.PicWidthPixel    = pMiPathCtx->out_width;
                    pPicBuffer->Data.YCbCr.semiplanar.CbCr.PicHeightPixel   = (pMiPathCtx->out_height >> 1);
                    pPicBuffer->Data.YCbCr.semiplanar.CbCr.PicWidthBytes    = pMiPathCtx->out_width;
                }
                else if ( pMiPathCtx->datalayout == CAMERIC_MI_DATASTORAGE_INTERLEAVED )
                {
                    pPicBuffer->Layout = PIC_BUF_LAYOUT_COMBINED;

                    pPicBuffer->Data.YCbCr.combined.pBuffer         = (uint8_t *)pBuffer->pBaseAddress;
                    pPicBuffer->Data.YCbCr.combined.PicWidthPixel   = (pMiPathCtx->out_width << 1);
                    pPicBuffer->Data.YCbCr.combined.PicHeightPixel  = (pMiPathCtx->out_height + (pMiPathCtx->out_height >> 1));
                    pPicBuffer->Data.YCbCr.combined.PicWidthBytes   = (pMiPathCtx->out_width << 1);
                }
                else
                {
                    TRACE( CAMERIC_MI_DRV_ERROR, "%s: unknown data-layout (%p, %d)\n", __FUNCTION__, pMiPathCtx, pMiPathCtx->out_mode);
                }

                break;
            }

        case CAMERIC_MI_DATAMODE_YUV422:
            {
                pPicBuffer->Type = PIC_BUF_TYPE_YCbCr422;

                if ( pMiPathCtx->datalayout == CAMERIC_MI_DATASTORAGE_PLANAR )
                {
                    pPicBuffer->Layout = PIC_BUF_LAYOUT_PLANAR;

                    /* luminace plane (Y) */
                    pPicBuffer->Data.YCbCr.planar.Y.pBuffer         = (uint8_t *)pBuffer->pBaseAddress;
                    pPicBuffer->Data.YCbCr.planar.Y.PicWidthPixel   = pMiPathCtx->out_width;
                    pPicBuffer->Data.YCbCr.planar.Y.PicHeightPixel  = pMiPathCtx->out_height;
                    pPicBuffer->Data.YCbCr.planar.Y.PicWidthBytes   = pMiPathCtx->out_width;

                    /* first color plane (Cb) (check alignment and addresses are in buffer) */
                    pPicBuffer->Data.YCbCr.planar.Cb.pBuffer
                        = (uint8_t *)( CAMERIC_MI_ALIGN((ulong_t)pBuffer->pBaseAddress 
                                       + (pMiPathCtx->out_width * pMiPathCtx->out_height) + CAMERIC_BUFFER_GAP) );
                    pPicBuffer->Data.YCbCr.planar.Cb.PicWidthPixel   = (pMiPathCtx->out_width >> 1);
                    pPicBuffer->Data.YCbCr.planar.Cb.PicHeightPixel  = (pMiPathCtx->out_height);
                    pPicBuffer->Data.YCbCr.planar.Cb.PicWidthBytes   = (pMiPathCtx->out_width >> 1);

                    /* second color plane (Cr) (check alignment and addresses are in buffer) */
                    pPicBuffer->Data.YCbCr.planar.Cr.pBuffer
                        = (uint8_t *)( CAMERIC_MI_ALIGN((ulong_t)pPicBuffer->Data.YCbCr.planar.Cb.pBuffer 
                                       + ((pMiPathCtx->out_width * pMiPathCtx->out_height)>>1) + CAMERIC_BUFFER_GAP) );
                    pPicBuffer->Data.YCbCr.planar.Cr.PicWidthPixel   = (pMiPathCtx->out_width >> 1);
                    pPicBuffer->Data.YCbCr.planar.Cr.PicHeightPixel  = (pMiPathCtx->out_height);
                    pPicBuffer->Data.YCbCr.planar.Cr.PicWidthBytes   = (pMiPathCtx->out_width >> 1);
                }
                else if ( pMiPathCtx->datalayout == CAMERIC_MI_DATASTORAGE_SEMIPLANAR )
                {
                    pPicBuffer->Layout = PIC_BUF_LAYOUT_SEMIPLANAR;

                    /* luminance plane (Y) */
                    pPicBuffer->Data.YCbCr.semiplanar.Y.pBuffer             = (uint8_t *)pBuffer->pBaseAddress;
                    pPicBuffer->Data.YCbCr.semiplanar.Y.PicWidthPixel       = pMiPathCtx->out_width;
                    pPicBuffer->Data.YCbCr.semiplanar.Y.PicHeightPixel      = pMiPathCtx->out_height;
                    pPicBuffer->Data.YCbCr.semiplanar.Y.PicWidthBytes       = pMiPathCtx->out_width;

                    /* color plane (CbCr) */
                    pPicBuffer->Data.YCbCr.semiplanar.CbCr.pBuffer
                        = (uint8_t *)( CAMERIC_MI_ALIGN((ulong_t)pBuffer->pBaseAddress 
                                       + (pMiPathCtx->out_width * pMiPathCtx->out_height) + CAMERIC_BUFFER_GAP) );
                    pPicBuffer->Data.YCbCr.semiplanar.CbCr.PicWidthPixel    = pMiPathCtx->out_width;
                    pPicBuffer->Data.YCbCr.semiplanar.CbCr.PicHeightPixel   = pMiPathCtx->out_height;
                    pPicBuffer->Data.YCbCr.semiplanar.CbCr.PicWidthBytes    = pMiPathCtx->out_width;
                }
                else if ( pMiPathCtx->datalayout == CAMERIC_MI_DATASTORAGE_INTERLEAVED )
                {
                    pPicBuffer->Layout = PIC_BUF_LAYOUT_COMBINED;

                    pPicBuffer->Data.YCbCr.combined.pBuffer         = (uint8_t *)pBuffer->pBaseAddress;
                    pPicBuffer->Data.YCbCr.combined.PicWidthPixel   = (pMiPathCtx->out_width << 1);
                    pPicBuffer->Data.YCbCr.combined.PicHeightPixel  = (pMiPathCtx->out_height);
                    pPicBuffer->Data.YCbCr.combined.PicWidthBytes   = (pMiPathCtx->out_width << 1);
                }
                else
                {
                    TRACE( CAMERIC_MI_DRV_ERROR, "%s: unknown data-layout (%p, %d)\n", __FUNCTION__, pMiPathCtx, pMiPathCtx->out_mode);
                }

                break;
            }
        
        case CAMERIC_MI_DATAMODE_YUV444:
            {
                pPicBuffer->Type = PIC_BUF_TYPE_YCbCr444;

                if ( pMiPathCtx->datalayout == CAMERIC_MI_DATASTORAGE_PLANAR )
                {
                    pPicBuffer->Layout = PIC_BUF_LAYOUT_PLANAR;

                    /* luminace plane (Y) */
                    pPicBuffer->Data.YCbCr.planar.Y.pBuffer         = (uint8_t *)pBuffer->pBaseAddress;
                    pPicBuffer->Data.YCbCr.planar.Y.PicWidthPixel   = pMiPathCtx->out_width;
                    pPicBuffer->Data.YCbCr.planar.Y.PicHeightPixel  = pMiPathCtx->out_height;
                    pPicBuffer->Data.YCbCr.planar.Y.PicWidthBytes   = pMiPathCtx->out_width;

                    /* first color plane (Cb) (check alignment and addresses are in buffer) */
                    pPicBuffer->Data.YCbCr.planar.Cb.pBuffer
                        = (uint8_t *)( CAMERIC_MI_ALIGN((ulong_t)pBuffer->pBaseAddress 
                                       + (pMiPathCtx->out_width * pMiPathCtx->out_height) + CAMERIC_BUFFER_GAP) );
                    pPicBuffer->Data.YCbCr.planar.Cb.PicWidthPixel  = pMiPathCtx->out_width;
                    pPicBuffer->Data.YCbCr.planar.Cb.PicHeightPixel = pMiPathCtx->out_height;
                    pPicBuffer->Data.YCbCr.planar.Cb.PicWidthBytes  = pMiPathCtx->out_width;

                    /* second color plane (Cr) (check alignment and addresses are in buffer) */
                    pPicBuffer->Data.YCbCr.planar.Cr.pBuffer
                        = (uint8_t *)( CAMERIC_MI_ALIGN((ulong_t)pPicBuffer->Data.YCbCr.planar.Cb.pBuffer 
                                       + (pMiPathCtx->out_width * pMiPathCtx->out_height) + CAMERIC_BUFFER_GAP) );
                    pPicBuffer->Data.YCbCr.planar.Cr.PicWidthPixel  = pMiPathCtx->out_width;
                    pPicBuffer->Data.YCbCr.planar.Cr.PicHeightPixel = pMiPathCtx->out_height;
                    pPicBuffer->Data.YCbCr.planar.Cr.PicWidthBytes  = pMiPathCtx->out_width;
                }
                else
                {
                    TRACE( CAMERIC_MI_DRV_ERROR, "%s: invalid data-layout (%p, %d)\n", __FUNCTION__, pMiPathCtx, pMiPathCtx->out_mode);
                }

                break;
            }

        case CAMERIC_MI_DATAMODE_RGB565:
            {
                pPicBuffer->Type    = PIC_BUF_TYPE_RGB565;
                pPicBuffer->Layout  = PIC_BUF_LAYOUT_COMBINED;

                pPicBuffer->Data.RGB.combined.pBuffer = (uint8_t *)pBuffer->pBaseAddress;

                switch ( pMiPathCtx->orientation )
                {
                    case CAMERIC_MI_ORIENTATION_ORIGINAL:
                    case CAMERIC_MI_ORIENTATION_VERTICAL_FLIP:
                    case CAMERIC_MI_ORIENTATION_HORIZONTAL_FLIP:
                    case CAMERIC_MI_ORIENTATION_ROTATE180:
                        {
                            pPicBuffer->Data.RGB.combined.PicWidthPixel     = pMiPathCtx->out_width;
                            pPicBuffer->Data.RGB.combined.PicHeightPixel    = pMiPathCtx->out_height;
                            pPicBuffer->Data.RGB.combined.PicWidthBytes     = (pMiPathCtx->out_width << 1);    // 32 bit per pixel

                            break;
                        }

                    case CAMERIC_MI_ORIENTATION_ROTATE90:
                    case CAMERIC_MI_ORIENTATION_ROTATE270:
                        {
                            pPicBuffer->Data.RGB.combined.PicWidthPixel     = pMiPathCtx->out_height;
                            pPicBuffer->Data.RGB.combined.PicHeightPixel    = pMiPathCtx->out_width;
                            pPicBuffer->Data.RGB.combined.PicWidthBytes     = (pMiPathCtx->out_height << 1);   // 32 bit per pixel

                            break;
                        }

                    default:
                        {
                            result = RET_NOTSUPP;
                            break;
                        }
                }

                break;
           }

        case CAMERIC_MI_DATAMODE_RGB666:
        case CAMERIC_MI_DATAMODE_RGB888:
            {
                pPicBuffer->Type    = ( pMiPathCtx->out_mode == CAMERIC_MI_DATAMODE_RGB666) ? PIC_BUF_TYPE_RGB666 : PIC_BUF_TYPE_RGB888;
                pPicBuffer->Layout  = PIC_BUF_LAYOUT_COMBINED;

                pPicBuffer->Data.RGB.combined.pBuffer = (uint8_t *)pBuffer->pBaseAddress;

                switch ( pMiPathCtx->orientation )
                {
                    case CAMERIC_MI_ORIENTATION_ORIGINAL:
                    case CAMERIC_MI_ORIENTATION_VERTICAL_FLIP:
                    case CAMERIC_MI_ORIENTATION_HORIZONTAL_FLIP:
                    case CAMERIC_MI_ORIENTATION_ROTATE180:
                        {
                            pPicBuffer->Data.RGB.combined.PicWidthPixel     = pMiPathCtx->out_width;
                            pPicBuffer->Data.RGB.combined.PicHeightPixel    = pMiPathCtx->out_height;
                            pPicBuffer->Data.RGB.combined.PicWidthBytes     = (pMiPathCtx->out_width << 2);    // 32 bit per pixel

                            break;
                        }

                    case CAMERIC_MI_ORIENTATION_ROTATE90:
                    case CAMERIC_MI_ORIENTATION_ROTATE270:
                        {
                            pPicBuffer->Data.RGB.combined.PicWidthPixel     = pMiPathCtx->out_height;
                            pPicBuffer->Data.RGB.combined.PicHeightPixel    = pMiPathCtx->out_width;
                            pPicBuffer->Data.RGB.combined.PicWidthBytes     = (pMiPathCtx->out_height << 2);   // 32 bit per pixel

                            break;
                        }

                    default:
                        {
                            result = RET_NOTSUPP;
                            break;
                        }
                }

                break;
           }

        case CAMERIC_MI_DATAMODE_RAW8:
            {
                pPicBuffer->Type                    = PIC_BUF_TYPE_RAW8;
                switch ( ctx->pIspContext->bayerPattern )
                {
                    case CAMERIC_ISP_BAYER_PATTERN_RGRGGBGB:
                    {
                        pPicBuffer->Layout = PIC_BUF_LAYOUT_BAYER_RGRGGBGB;
                        break;
                    }
                    case CAMERIC_ISP_BAYER_PATTERN_GRGRBGBG:
                    {
                        pPicBuffer->Layout = PIC_BUF_LAYOUT_BAYER_GRGRBGBG;
                        break;
                    }
                    case CAMERIC_ISP_BAYER_PATTERN_GBGBRGRG:
                    {
                        pPicBuffer->Layout = PIC_BUF_LAYOUT_BAYER_GBGBRGRG;
                        break;
                    }
                    case CAMERIC_ISP_BAYER_PATTERN_BGBGGRGR:
                    {
                        pPicBuffer->Layout = PIC_BUF_LAYOUT_BAYER_BGBGGRGR;
                        break;
                    }
                    default:
                    {
                        pPicBuffer->Layout  = PIC_BUF_LAYOUT_COMBINED;
                    }
                }

                pPicBuffer->Data.raw.pBuffer        = (uint8_t *)pBuffer->pBaseAddress;
                pPicBuffer->Data.raw.PicWidthPixel  = pMiPathCtx->out_width;
                pPicBuffer->Data.raw.PicHeightPixel = pMiPathCtx->out_height;
                pPicBuffer->Data.raw.PicWidthBytes  = pMiPathCtx->out_width;

                break;
            }

        case CAMERIC_MI_DATAMODE_RAW12:
            {
                pPicBuffer->Type                    = PIC_BUF_TYPE_RAW16;
                switch ( ctx->pIspContext->bayerPattern )
                {
                    case CAMERIC_ISP_BAYER_PATTERN_RGRGGBGB:
                    {
                        pPicBuffer->Layout = PIC_BUF_LAYOUT_BAYER_RGRGGBGB;
                        break;
                    }
                    case CAMERIC_ISP_BAYER_PATTERN_GRGRBGBG:
                    {
                        pPicBuffer->Layout = PIC_BUF_LAYOUT_BAYER_GRGRBGBG;
                        break;
                    }
                    case CAMERIC_ISP_BAYER_PATTERN_GBGBRGRG:
                    {
                        pPicBuffer->Layout = PIC_BUF_LAYOUT_BAYER_GBGBRGRG;
                        break;
                    }
                    case CAMERIC_ISP_BAYER_PATTERN_BGBGGRGR:
                    {
                        pPicBuffer->Layout = PIC_BUF_LAYOUT_BAYER_BGBGGRGR;
                        break;
                    }
                    default:
                    {
                        pPicBuffer->Layout  = PIC_BUF_LAYOUT_COMBINED;
                    }
                }

                pPicBuffer->Data.raw.pBuffer        = (uint8_t *)pBuffer->pBaseAddress;
                pPicBuffer->Data.raw.PicWidthPixel  = pMiPathCtx->out_width;
                pPicBuffer->Data.raw.PicHeightPixel = pMiPathCtx->out_height;
                pPicBuffer->Data.raw.PicWidthBytes  = (pMiPathCtx->out_width << 1);

                break;
            }

        case CAMERIC_MI_DATAMODE_JPEG:
            {
                pPicBuffer->Type                    = PIC_BUF_TYPE_JPEG;
                pPicBuffer->Layout                  = PIC_BUF_LAYOUT_COMBINED;

                pPicBuffer->Data.jpeg.pData          = (uint8_t *)pBuffer->pBaseAddress;
                pPicBuffer->Data.jpeg.PicWidthPixel  = pMiPathCtx->out_width;
                pPicBuffer->Data.jpeg.PicHeightPixel = pMiPathCtx->out_height;

                break;
            }

        case CAMERIC_MI_DATAMODE_DPCC:
            {
                pPicBuffer->Type                    = PIC_BUF_TYPE_DPCC;
                pPicBuffer->Layout                  = PIC_BUF_LAYOUT_COMBINED;

                pPicBuffer->Data.raw.pBuffer        = (uint8_t *)pBuffer->pBaseAddress;
                pPicBuffer->Data.raw.PicWidthPixel  = pMiPathCtx->out_width;
                pPicBuffer->Data.raw.PicHeightPixel = pMiPathCtx->out_height;
                pPicBuffer->Data.raw.PicWidthBytes  = pMiPathCtx->out_width;

                break;
            }

         default:
            {
                result = RET_NOTSUPP;
                break;
            }
    }

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

/******************************************************************************
 * CamerIcDisableScaling()
 *****************************************************************************/
static RESULT CamerIcDisableScaling
(
    CamerIcDrvContext_t     *ctx,
    const CamerIcMiPath_t   path
)
{
    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (path != CAMERIC_MI_PATH_MAIN) && (path != CAMERIC_MI_PATH_SELF) )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        CamerIcMiDataPathContext_t *pPathCtx = &ctx->pMiContext->PathCtx[path];

        RESULT result = RET_SUCCESS;
        uint32_t ctrl = 0U;

        if ( path == CAMERIC_MI_PATH_MAIN )
        {
            ctrl =  HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mrsz_ctrl) );

            REG_SET_SLICE( ctrl, MRV_MRSZ_AUTO_UPD, 1);

            REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_HY_ENABLE, 0 );
            REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_HC_ENABLE, 0 );
            REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_VY_ENABLE, 0 );
            REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_VC_ENABLE, 0 );

            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mrsz_ctrl), ctrl );
        }
        else
        {
            ctrl =  HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->srsz_ctrl) );

            REG_SET_SLICE( ctrl, MRV_SRSZ_CFG_UPD, 1);

            REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_HY_ENABLE, 0 );
            REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_HC_ENABLE, 0 );
            REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_VY_ENABLE, 0 );
            REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_VC_ENABLE, 0 );

            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->srsz_ctrl), ctrl );
        }
    }

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}

/******************************************************************************
 * CamerIcSetupScaling()
 *****************************************************************************/
static RESULT CamerIcSetupScaling
(
    CamerIcDrvContext_t     *ctx,
    const CamerIcMiPath_t   path,
    bool aoto_update
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (path != CAMERIC_MI_PATH_MAIN) && (path != CAMERIC_MI_PATH_SELF) )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        CamerIcMiDataPathContext_t *pPathCtx = &ctx->pMiContext->PathCtx[path];

        uint32_t scale_hy   = 0U;
        uint32_t scale_hcb  = 0U;
        uint32_t scale_hcr  = 0U;

        uint32_t scale_vy   = 0U;
        uint32_t scale_vc   = 0U;

        uint32_t ctrl = 0U;

        //zyc add
        if((pPathCtx->out_mode != CAMERIC_MI_DATAMODE_YUV420) && (pPathCtx->out_mode != CAMERIC_MI_DATAMODE_YUV422)){
            //disable scale
            result = CamerIcDisableScaling(ctx,path);
            return result;

        }

        if ( path == CAMERIC_MI_PATH_MAIN )
        {
            ctrl =  HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mrsz_ctrl) );
            ctrl &= ~MRV_MRSZ_CFG_UPD_MASK;
        }
        else
        {
            ctrl =  HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->srsz_ctrl) );
        }

        switch ( pPathCtx->in_mode )
        {
            case CAMERIC_MI_DATAMODE_YUV422:
            {
                // check horizontal down scaling ?
                if ( pPathCtx->in_width != pPathCtx->out_width )
                {
                    result = CamerIcCalcScaleFactor( pPathCtx->in_width, pPathCtx->out_width, &scale_hy );
                    if ( result != RET_SUCCESS )
                    {
                        return ( result );
                    }

                    // NOTE: YUV422 => width/2 for chroma
                    result = CamerIcCalcScaleFactor( (pPathCtx->in_width>>1), (pPathCtx->out_width>>1), &scale_hcb );
                    if ( result != RET_SUCCESS )
                    {
                        return ( result );
                    }

                    result = CamerIcCalcScaleFactor( (pPathCtx->in_width>>1), (pPathCtx->out_width>>1), &scale_hcr );
                    if ( result != RET_SUCCESS )
                    {
                        return ( result );
                    }

                    if ( path == CAMERIC_MI_PATH_MAIN )
                    {
                        uint32_t Y_mode = (pPathCtx->in_width > pPathCtx->out_width) ? MRV_MRSZ_SCALE_HY_UP_DOWNSCALE : MRV_MRSZ_SCALE_HY_UP_UPSCALE; 
                        uint32_t C_mode = (pPathCtx->in_width > pPathCtx->out_width) ? MRV_MRSZ_SCALE_HC_UP_DOWNSCALE : MRV_MRSZ_SCALE_HC_UP_UPSCALE; 

                        if(aoto_update){
                            REG_SET_SLICE( ctrl, MRV_MRSZ_AUTO_UPD, 1 );
                        }else{
                            REG_SET_SLICE( ctrl, MRV_MRSZ_CFG_UPD, 1 );
                        }

                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_HY_UP, Y_mode );
                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_HC_UP, C_mode );

                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_HY_ENABLE, 1 );
                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_HC_ENABLE, 1 );
                    }
                    else
                    {
                        uint32_t Y_mode = (pPathCtx->in_width > pPathCtx->out_width) ? MRV_SRSZ_SCALE_HY_UP_DOWNSCALE : MRV_SRSZ_SCALE_HY_UP_UPSCALE; 
                        uint32_t C_mode = (pPathCtx->in_width > pPathCtx->out_width) ? MRV_SRSZ_SCALE_HC_UP_DOWNSCALE : MRV_SRSZ_SCALE_HC_UP_UPSCALE; 

                        REG_SET_SLICE( ctrl, MRV_SRSZ_CFG_UPD, 1 );

                        REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_HY_UP, Y_mode );
                        REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_HC_UP, C_mode );

                        REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_HY_ENABLE, 1 );
                        REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_HC_ENABLE, 1 );
                    }
                }
                else
                {
                    if ( path == CAMERIC_MI_PATH_MAIN )
                    {
                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_HY_ENABLE, 0 );
                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_HC_ENABLE, 0 );
                    }
                    else
                    {
                        REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_HY_ENABLE, 0 );
                        REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_HC_ENABLE, 0 );
                    }
                }

                // check vertical down scaling ?
                if ( pPathCtx->in_height != pPathCtx->out_height )
                {
                    result = CamerIcCalcScaleFactor( pPathCtx->in_height, pPathCtx->out_height, &scale_vy );
                    if ( result != RET_SUCCESS )
                    {
                        return ( result );
                    }

                    result = CamerIcCalcScaleFactor( pPathCtx->in_height, pPathCtx->out_height, &scale_vc );
                    if ( result != RET_SUCCESS )
                    {
                        return ( result );
                    }

                    if ( path == CAMERIC_MI_PATH_MAIN )
                    {
                        uint32_t Y_mode = (pPathCtx->in_height > pPathCtx->out_height) ? MRV_MRSZ_SCALE_VY_UP_DOWNSCALE : MRV_MRSZ_SCALE_VY_UP_UPSCALE; 
                        uint32_t C_mode = (pPathCtx->in_height > pPathCtx->out_height) ? MRV_MRSZ_SCALE_VC_UP_DOWNSCALE : MRV_MRSZ_SCALE_VC_UP_UPSCALE; 

                        if(aoto_update){
                            REG_SET_SLICE( ctrl, MRV_MRSZ_AUTO_UPD, 1 );
                        }else{
                            REG_SET_SLICE( ctrl, MRV_MRSZ_CFG_UPD, 1 );
                        }

                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_VY_UP, Y_mode );
                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_VC_UP, C_mode );

                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_VY_ENABLE, 1 );
                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_VC_ENABLE, 1 );
                    }
                    else
                    {
                        uint32_t Y_mode = (pPathCtx->in_height > pPathCtx->out_height) ? MRV_SRSZ_SCALE_VY_UP_DOWNSCALE : MRV_SRSZ_SCALE_VY_UP_UPSCALE; 
                        uint32_t C_mode = (pPathCtx->in_height > pPathCtx->out_height) ? MRV_SRSZ_SCALE_VC_UP_DOWNSCALE : MRV_SRSZ_SCALE_VC_UP_UPSCALE; 

                        REG_SET_SLICE( ctrl, MRV_SRSZ_CFG_UPD, 1);

                        REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_VY_UP, Y_mode );
                        REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_VC_UP, C_mode );

                        REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_VY_ENABLE, 1 );
                        REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_VC_ENABLE, 1 );
                    }
                }
                else
                {
                    if ( path == CAMERIC_MI_PATH_MAIN )
                    {
                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_VY_ENABLE, 0 );
                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_VC_ENABLE, 0 );
                    }
                    else
                    {
                        REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_VY_ENABLE, 0 );
                        REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_VC_ENABLE, 0 );
                    }
                }

                break;
            }

            case CAMERIC_MI_DATAMODE_YUV420:
            {
                // check horizontal down scaling ?
                if ( pPathCtx->in_width != pPathCtx->out_width )
                {
                    result = CamerIcCalcScaleFactor( pPathCtx->in_width, pPathCtx->out_width, &scale_hy );
                    if ( result != RET_SUCCESS )
                    {
                        return ( result );
                    }

                    result = CamerIcCalcScaleFactor( (pPathCtx->in_width>>1), (pPathCtx->out_width>>1), &scale_hcb );
                    if ( result != RET_SUCCESS )
                    {
                        return ( result );
                    }

                    result = CamerIcCalcScaleFactor( (pPathCtx->in_width>>1), (pPathCtx->out_width>>1), &scale_hcr );
                    if ( result != RET_SUCCESS )
                    {
                        return ( result );
                    }

                    if ( path == CAMERIC_MI_PATH_MAIN )
                    {
                        uint32_t Y_mode = (pPathCtx->in_width > pPathCtx->out_width) ? MRV_MRSZ_SCALE_HY_UP_DOWNSCALE : MRV_MRSZ_SCALE_HY_UP_UPSCALE; 
                        uint32_t C_mode = (pPathCtx->in_width > pPathCtx->out_width) ? MRV_MRSZ_SCALE_HC_UP_DOWNSCALE : MRV_MRSZ_SCALE_HC_UP_UPSCALE; 

                        if(aoto_update){
                            REG_SET_SLICE( ctrl, MRV_MRSZ_AUTO_UPD, 1 );
                        }else{
                            REG_SET_SLICE( ctrl, MRV_MRSZ_CFG_UPD, 1 );
                        }

                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_HY_UP, Y_mode );
                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_HC_UP, C_mode );

                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_HY_ENABLE, 1 );
                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_HC_ENABLE, 1 );
                    }
                    else
                    {
                        uint32_t Y_mode = (pPathCtx->in_height > pPathCtx->out_height) ? MRV_SRSZ_SCALE_HY_UP_DOWNSCALE : MRV_SRSZ_SCALE_HY_UP_UPSCALE; 
                        uint32_t C_mode = (pPathCtx->in_height > pPathCtx->out_height) ? MRV_SRSZ_SCALE_HC_UP_DOWNSCALE : MRV_SRSZ_SCALE_HC_UP_UPSCALE; 

                        REG_SET_SLICE( ctrl, MRV_SRSZ_CFG_UPD, 1);

                        REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_HY_UP, Y_mode );
                        REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_HC_UP, C_mode );

                        REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_HY_ENABLE, 1 );
                        REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_HC_ENABLE, 1 );
                    }
                }
                else
                {
                    if ( path == CAMERIC_MI_PATH_MAIN )
                    {
                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_HY_ENABLE, 0 );
                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_HC_ENABLE, 0 );
                    }
                    else
                    {
                        REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_HY_ENABLE, 0 );
                        REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_HC_ENABLE, 0 );
                    }
                }

                // vertical downscaling of luminance plane ?
                if ( pPathCtx->in_height != pPathCtx->out_height )
                {
                    result = CamerIcCalcScaleFactor( pPathCtx->in_height, pPathCtx->out_height, &scale_vy );
                    if ( result != RET_SUCCESS )
                    {
                        return ( result );
                    }

                    if ( path == CAMERIC_MI_PATH_MAIN )
                    {
                        uint32_t Y_mode = (pPathCtx->in_height > pPathCtx->out_height) ? MRV_MRSZ_SCALE_VY_UP_DOWNSCALE : MRV_MRSZ_SCALE_VY_UP_UPSCALE; 

                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_VY_UP, Y_mode );
                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_VY_ENABLE, 1 );
                    }
                    else
                    {
                        uint32_t Y_mode = (pPathCtx->in_height > pPathCtx->out_height) ? MRV_SRSZ_SCALE_VY_UP_DOWNSCALE : MRV_SRSZ_SCALE_VY_UP_UPSCALE;

                        REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_VY_UP, Y_mode );
                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_VY_ENABLE, 1 );
                    }
                }
                else
                {
                    if ( path == CAMERIC_MI_PATH_MAIN )
                    {
                        REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_VY_ENABLE, 0 );
                    }
                    else
                    {
                        REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_VY_ENABLE, 0 );
                    }
                }

                // chrominance plane is always downscaled due to 420
                result = CamerIcCalcScaleFactor( pPathCtx->in_height, (pPathCtx->out_height>>1), &scale_vc );
                if ( result != RET_SUCCESS )
                {
                    return ( result );
                }

                if ( path == CAMERIC_MI_PATH_MAIN )
                {
                    uint32_t C_mode = (pPathCtx->in_height > (pPathCtx->out_height>>1)) ? MRV_MRSZ_SCALE_VC_UP_DOWNSCALE : MRV_MRSZ_SCALE_VC_UP_UPSCALE; 

                    if(aoto_update){
                        REG_SET_SLICE( ctrl, MRV_MRSZ_AUTO_UPD, 1 );
                    }else{
                        REG_SET_SLICE( ctrl, MRV_MRSZ_CFG_UPD, 1 );
                    }
                    REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_VC_UP, C_mode );
                    REG_SET_SLICE( ctrl, MRV_MRSZ_SCALE_VC_ENABLE, 1 );
                }
                else
                {
                    uint32_t C_mode = (pPathCtx->in_height > (pPathCtx->out_height>>1)) ? MRV_SRSZ_SCALE_VC_UP_DOWNSCALE : MRV_SRSZ_SCALE_VC_UP_UPSCALE; 

                    REG_SET_SLICE( ctrl, MRV_SRSZ_CFG_UPD, 1);
                    REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_VC_UP, C_mode );
                    REG_SET_SLICE( ctrl, MRV_SRSZ_SCALE_VC_ENABLE, 1 );
                }
                
                break;
            }
 
            default:
            {
                result = RET_NOTSUPP;
                break;
            }
        }

        if ( path == CAMERIC_MI_PATH_MAIN )
        {
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mrsz_scale_vc), scale_vc );
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mrsz_scale_vy), scale_vy );

            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mrsz_scale_hcr), scale_hcr );
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mrsz_scale_hcb), scale_hcb );
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mrsz_scale_hy) , scale_hy);

            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mrsz_ctrl), ctrl );
        }
        else
        {
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->srsz_scale_vc), scale_vc );
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->srsz_scale_vy), scale_vy );

            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->srsz_scale_hcr), scale_hcr );
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->srsz_scale_hcb), scale_hcb );
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->srsz_scale_hy) , scale_hy);

            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->srsz_ctrl), ctrl );
        }

    }

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/******************************************************************************
 * CamerIcMiSetBuffer()
 *****************************************************************************/
static RESULT CamerIcMiSetBuffer
(
    CamerIcDrvHandle_t      handle,
    const CamerIcMiPath_t   path,
    const PicBufMetaData_t  *pPicBuffer
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    ulong_t baseY;
    uint32_t sizeY;

    ulong_t baseCb;
    uint32_t sizeCb;

    ulong_t baseCr;
    uint32_t sizeCr;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    DCT_ASSERT( ((ctx != NULL) && (ctx->pMiContext != NULL)) );

    /* check path and picture buffer pointer */
    if ( (pPicBuffer == NULL)
            || ((path != CAMERIC_MI_PATH_MAIN) && (path != CAMERIC_MI_PATH_SELF)) )
    {
        return ( RET_INVALID_PARM );
    }


    switch ( pPicBuffer->Type )
    {
        case PIC_BUF_TYPE_RAW8:
            {
                if ( ( pPicBuffer->Layout == PIC_BUF_LAYOUT_BAYER_RGRGGBGB ) ||
                     ( pPicBuffer->Layout == PIC_BUF_LAYOUT_BAYER_GRGRBGBG ) ||
                     ( pPicBuffer->Layout == PIC_BUF_LAYOUT_BAYER_GBGBRGRG ) ||
                     ( pPicBuffer->Layout == PIC_BUF_LAYOUT_BAYER_BGBGGRGR ) ||
                     ( pPicBuffer->Layout == PIC_BUF_LAYOUT_COMBINED ) )
                {
                    baseY = (ulong_t)pPicBuffer->Data.raw.pBuffer;
                    sizeY = pPicBuffer->Data.raw.PicWidthBytes * pPicBuffer->Data.raw.PicHeightPixel;

                    baseCb = 0UL;
                    sizeCb = 0UL;

                    baseCr = 0UL;
                    sizeCr = 0UL;
                }
                else
                {
                    TRACE( CAMERIC_MI_DRV_ERROR, "%s: invalid buffer layout\n", __FUNCTION__ );
                    result = RET_FAILURE;
                }

                break;
            }

        case PIC_BUF_TYPE_RAW16:
            {
                if ( ( pPicBuffer->Layout == PIC_BUF_LAYOUT_BAYER_RGRGGBGB ) ||
                     ( pPicBuffer->Layout == PIC_BUF_LAYOUT_BAYER_GRGRBGBG ) ||
                     ( pPicBuffer->Layout == PIC_BUF_LAYOUT_BAYER_GBGBRGRG ) ||
                     ( pPicBuffer->Layout == PIC_BUF_LAYOUT_BAYER_BGBGGRGR ) ||
                     ( pPicBuffer->Layout == PIC_BUF_LAYOUT_COMBINED ) )
                {
                    baseY = (ulong_t)pPicBuffer->Data.raw.pBuffer;
                    sizeY = ( (pPicBuffer->Data.raw.PicWidthPixel * pPicBuffer->Data.raw.PicHeightPixel) << 1 );

                    baseCb = 0UL;
                    sizeCb = 0UL;

                    baseCr = 0UL;
                    sizeCr = 0UL;
                }
                else
                {
                    TRACE( CAMERIC_MI_DRV_ERROR, "%s: invalid buffer layout\n", __FUNCTION__ );
                    result = RET_FAILURE;
                }

                break;
            }

        case PIC_BUF_TYPE_JPEG:
            {
                if ( pPicBuffer->Layout == PIC_BUF_LAYOUT_COMBINED )
                {
                    baseY = (ulong_t)pPicBuffer->Data.jpeg.pData;
                    sizeY = ( pPicBuffer->Data.jpeg.PicWidthPixel * pPicBuffer->Data.jpeg.PicHeightPixel );

                    baseCb = 0UL;
                    sizeCb = 0UL;

                    baseCr = 0UL;
                    sizeCr = 0UL;
                }
                else
                {
                    TRACE( CAMERIC_MI_DRV_ERROR, "%s: invalid buffer layout\n", __FUNCTION__ );
                    result = RET_FAILURE;
                }

                break;
            }

        case PIC_BUF_TYPE_DPCC:
            {
                if ( pPicBuffer->Layout == PIC_BUF_LAYOUT_COMBINED )
                {
                    baseY = (ulong_t)pPicBuffer->Data.raw.pBuffer;
                    sizeY = ( pPicBuffer->Data.raw.PicWidthPixel * pPicBuffer->Data.raw.PicHeightPixel );

                    baseCb = 0UL;
                    sizeCb = 0UL;

                    baseCr = 0UL;
                    sizeCr = 0UL;
                }
                else
                {
                    TRACE( CAMERIC_MI_DRV_ERROR, "%s: invalid buffer layout\n", __FUNCTION__ );
                    result = RET_FAILURE;
                }

                break;
            }

        case PIC_BUF_TYPE_RGB565:
        case PIC_BUF_TYPE_RGB666:
        case PIC_BUF_TYPE_RGB888:
            {
                if ( pPicBuffer->Layout == PIC_BUF_LAYOUT_COMBINED )
                {
                    baseY = (ulong_t)pPicBuffer->Data.RGB.combined.pBuffer;
                    sizeY = ( pPicBuffer->Data.RGB.combined.PicWidthBytes * pPicBuffer->Data.RGB.combined.PicHeightPixel + CAMERIC_BUFFER_GAP );

                    baseCb = 0UL;
                    sizeCb = 0UL;

                    baseCr = 0UL;
                    sizeCr = 0UL;
                }
                else
                {
                    TRACE( CAMERIC_MI_DRV_ERROR, "%s: invalid buffer layout\n", __FUNCTION__ );
                    result = RET_FAILURE;
                }

                break;
             }

        case PIC_BUF_TYPE_YCbCr444:
        case PIC_BUF_TYPE_YCbCr422:
        case PIC_BUF_TYPE_YCbCr420:
            {
                if ( pPicBuffer->Layout == PIC_BUF_LAYOUT_PLANAR )
                {
                    baseY = (ulong_t)pPicBuffer->Data.YCbCr.planar.Y.pBuffer;
                    sizeY = (pPicBuffer->Data.YCbCr.planar.Y.PicWidthPixel * pPicBuffer->Data.YCbCr.planar.Y.PicHeightPixel + CAMERIC_BUFFER_GAP);

                    baseCb = (ulong_t)pPicBuffer->Data.YCbCr.planar.Cb.pBuffer;
                    sizeCb = (pPicBuffer->Data.YCbCr.planar.Cb.PicWidthPixel * pPicBuffer->Data.YCbCr.planar.Cb.PicHeightPixel + CAMERIC_BUFFER_GAP);

                    baseCr = (ulong_t)pPicBuffer->Data.YCbCr.planar.Cr.pBuffer;
                    sizeCr = (pPicBuffer->Data.YCbCr.planar.Cr.PicWidthPixel * pPicBuffer->Data.YCbCr.planar.Cr.PicHeightPixel + CAMERIC_BUFFER_GAP);
                }
                else if ( pPicBuffer->Layout == PIC_BUF_LAYOUT_SEMIPLANAR )
                {
                    baseY = (ulong_t)pPicBuffer->Data.YCbCr.semiplanar.Y.pBuffer;
                    sizeY = (pPicBuffer->Data.YCbCr.semiplanar.Y.PicWidthPixel * pPicBuffer->Data.YCbCr.semiplanar.Y.PicHeightPixel + CAMERIC_BUFFER_GAP);

                    baseCb = (ulong_t)pPicBuffer->Data.YCbCr.semiplanar.CbCr.pBuffer;
                    sizeCb = (pPicBuffer->Data.YCbCr.semiplanar.CbCr.PicWidthPixel * pPicBuffer->Data.YCbCr.semiplanar.CbCr.PicHeightPixel + CAMERIC_BUFFER_GAP);

                    baseCr = 0UL;
                    sizeCr = 0UL;
                }
                else if ( pPicBuffer->Layout == PIC_BUF_LAYOUT_COMBINED )
                {
                    baseY = (ulong_t)pPicBuffer->Data.YCbCr.combined.pBuffer;
                    sizeY = pPicBuffer->Data.YCbCr.combined.PicWidthPixel * pPicBuffer->Data.YCbCr.combined.PicHeightPixel + CAMERIC_BUFFER_GAP;

                    baseCb = 0UL;
                    sizeCb = 0UL;

                    baseCr = 0UL;
                    sizeCr = 0UL;
                }
                else
                {
                    TRACE( CAMERIC_MI_DRV_ERROR, "%s: invalid buffer layout\n", __FUNCTION__ );
                    result = RET_FAILURE;
                }

                break;
            }

        default:
            {
                TRACE( CAMERIC_MI_DRV_ERROR, "%s: invalid buffer type\n", __FUNCTION__ );
                result = RET_FAILURE;

                break;
            }
    }

    if ( result == RET_SUCCESS )
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        switch ( path )
        {

            case CAMERIC_MI_PATH_MAIN:
                {
                    TRACE( CAMERIC_MI_DRV_INFO, "%s: MP (0x%08x, 0x%08x, 0x%08x)\n",
                            __FUNCTION__, baseY, baseCb, baseCr);

                    TRACE( CAMERIC_MI_DRV_INFO, "%s: MP SHD (0x%08x, 0x%08x, 0x%08x)\n",
                            __FUNCTION__,
                            HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_mp_y_base_ad_shd)   ),
                            HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_mp_cb_base_ad_shd)  ),
                            HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_mp_cr_base_ad_shd)  ) );

                    TRACE( CAMERIC_MI_DRV_INFO, "%s: Byte Cnt (0x%08x)\n",
                            __FUNCTION__,  HalReadReg(ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_byte_cnt)) );

                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_mp_y_base_ad_init) , (baseY & MRV_MI_MP_Y_BASE_AD_INIT_MASK) );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_mp_y_size_init)    , (sizeY & MRV_MI_MP_Y_SIZE_INIT_MASK) );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_mp_y_offs_cnt_init), 0UL );

                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_mp_cb_base_ad_init) , (baseCb & MRV_MI_MP_CB_BASE_AD_INIT_MASK) );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_mp_cb_size_init)    , (sizeCb & MRV_MI_MP_CB_SIZE_INIT_MASK)  );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_mp_cb_offs_cnt_init), 0UL );

                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_mp_cr_base_ad_init) , (baseCr & MRV_MI_MP_CR_BASE_AD_INIT_MASK ) );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_mp_cr_size_init)    , (sizeCr & MRV_MI_MP_CR_SIZE_INIT_MASK) );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_mp_cr_offs_cnt_init), 0UL );

                    break;
                }

            case CAMERIC_MI_PATH_SELF:
                {
                    uint32_t mi_ctrl            = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_ctrl) );
                    uint32_t mi_sp_y_pic_width  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_y_pic_width) );
                    uint32_t mi_sp_y_pic_height = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_y_pic_height) );
                    uint32_t mi_sp_y_pic_size   = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_y_pic_size) );
                    uint32_t mi_sp_y_llength    = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_y_llength) );

                    /* check if picture orientation */
                    switch ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].orientation )
                    {
                        case CAMERIC_MI_ORIENTATION_ORIGINAL:
                            {
                                if ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].prepare_rotation == BOOL_TRUE )
                                {
                                    REG_SET_SLICE( mi_sp_y_pic_width, MRV_MI_SP_Y_PIC_WIDTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height );
                                    REG_SET_SLICE( mi_sp_y_llength, MRV_MI_SP_Y_LLENGTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height );
                                    REG_SET_SLICE( mi_sp_y_pic_height, MRV_MI_SP_Y_PIC_HEIGHT,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width );

                                    ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].prepare_rotation = BOOL_FALSE;
                                }
                                else
                                {
                                    REG_SET_SLICE( mi_sp_y_pic_width, MRV_MI_SP_Y_PIC_WIDTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width );
                                    REG_SET_SLICE( mi_sp_y_llength, MRV_MI_SP_Y_LLENGTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width );
                                    REG_SET_SLICE( mi_sp_y_pic_height, MRV_MI_SP_Y_PIC_HEIGHT,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height );

                                    REG_SET_SLICE( mi_ctrl, MRV_MI_ROT      , 0U );
                                    REG_SET_SLICE( mi_ctrl, MRV_MI_V_FLIP   , 0U );
                                    REG_SET_SLICE( mi_ctrl, MRV_MI_H_FLIP   , 0U );
                                }
                                break;
                            }

                        case CAMERIC_MI_ORIENTATION_HORIZONTAL_FLIP:
                            {
                                if ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].prepare_rotation == BOOL_TRUE )
                                {
                                    REG_SET_SLICE( mi_sp_y_pic_width, MRV_MI_SP_Y_PIC_WIDTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height );
                                    REG_SET_SLICE( mi_sp_y_llength, MRV_MI_SP_Y_LLENGTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height );
                                    REG_SET_SLICE( mi_sp_y_pic_height, MRV_MI_SP_Y_PIC_HEIGHT,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width );

                                    ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].prepare_rotation = BOOL_FALSE;
                                }
                                else
                                {
                                    REG_SET_SLICE( mi_sp_y_pic_width, MRV_MI_SP_Y_PIC_WIDTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width );
                                    REG_SET_SLICE( mi_sp_y_llength, MRV_MI_SP_Y_LLENGTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width );
                                    REG_SET_SLICE( mi_sp_y_pic_height, MRV_MI_SP_Y_PIC_HEIGHT,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height );

                                    REG_SET_SLICE( mi_ctrl, MRV_MI_ROT      , 0U );
                                    REG_SET_SLICE( mi_ctrl, MRV_MI_V_FLIP   , 0U );
                                    REG_SET_SLICE( mi_ctrl, MRV_MI_H_FLIP   , 1U );
                                }
                                break;
                            }

                        case CAMERIC_MI_ORIENTATION_VERTICAL_FLIP:
                            {
                                if ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].prepare_rotation == BOOL_TRUE )
                                {
                                    REG_SET_SLICE( mi_sp_y_pic_width, MRV_MI_SP_Y_PIC_WIDTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height );
                                    REG_SET_SLICE( mi_sp_y_llength, MRV_MI_SP_Y_LLENGTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height );
                                    REG_SET_SLICE( mi_sp_y_pic_height, MRV_MI_SP_Y_PIC_HEIGHT,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width );

                                    ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].prepare_rotation = BOOL_FALSE;
                                }
                                else
                                {
                                    REG_SET_SLICE( mi_sp_y_pic_width, MRV_MI_SP_Y_PIC_WIDTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width );
                                    REG_SET_SLICE( mi_sp_y_llength, MRV_MI_SP_Y_LLENGTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width );
                                    REG_SET_SLICE( mi_sp_y_pic_height, MRV_MI_SP_Y_PIC_HEIGHT,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height );

                                    REG_SET_SLICE( mi_ctrl, MRV_MI_ROT      , 0U );
                                    REG_SET_SLICE( mi_ctrl, MRV_MI_V_FLIP   , 1U );
                                    REG_SET_SLICE( mi_ctrl, MRV_MI_H_FLIP   , 0U );
                                }
                                break;
                            }

                        case CAMERIC_MI_ORIENTATION_ROTATE90:
                            {
                                if ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].prepare_rotation == BOOL_TRUE )
                                {
                                    REG_SET_SLICE( mi_sp_y_pic_width, MRV_MI_SP_Y_PIC_WIDTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width );
                                    REG_SET_SLICE( mi_sp_y_llength, MRV_MI_SP_Y_LLENGTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width );
                                    REG_SET_SLICE( mi_sp_y_pic_height, MRV_MI_SP_Y_PIC_HEIGHT,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height );

                                    ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].prepare_rotation = BOOL_FALSE;
                                }
                                else
                                {
                                    REG_SET_SLICE( mi_sp_y_pic_width, MRV_MI_SP_Y_PIC_WIDTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height );
                                    REG_SET_SLICE( mi_sp_y_llength, MRV_MI_SP_Y_LLENGTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height );
                                    REG_SET_SLICE( mi_sp_y_pic_height, MRV_MI_SP_Y_PIC_HEIGHT,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width );

                                    REG_SET_SLICE( mi_ctrl, MRV_MI_ROT      , 1U );
                                    REG_SET_SLICE( mi_ctrl, MRV_MI_V_FLIP   , 0U );
                                    REG_SET_SLICE( mi_ctrl, MRV_MI_H_FLIP   , 0U );
                                }

                                break;
                            }

                        case CAMERIC_MI_ORIENTATION_ROTATE180:
                            {
                                if ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].prepare_rotation == BOOL_TRUE )
                                {
                                    REG_SET_SLICE( mi_sp_y_pic_width, MRV_MI_SP_Y_PIC_WIDTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height );
                                    REG_SET_SLICE( mi_sp_y_llength, MRV_MI_SP_Y_LLENGTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height );
                                    REG_SET_SLICE( mi_sp_y_pic_height, MRV_MI_SP_Y_PIC_HEIGHT,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width );

                                    ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].prepare_rotation = BOOL_FALSE;
                                }
                                else
                                {
                                    REG_SET_SLICE( mi_sp_y_pic_width, MRV_MI_SP_Y_PIC_WIDTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width );
                                    REG_SET_SLICE( mi_sp_y_llength, MRV_MI_SP_Y_LLENGTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width );
                                    REG_SET_SLICE( mi_sp_y_pic_height, MRV_MI_SP_Y_PIC_HEIGHT,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height );

                                    REG_SET_SLICE( mi_ctrl, MRV_MI_ROT      , 0U );
                                    REG_SET_SLICE( mi_ctrl, MRV_MI_V_FLIP   , 1U );
                                    REG_SET_SLICE( mi_ctrl, MRV_MI_H_FLIP   , 1U );
                                }

                                break;
                            }

                        case CAMERIC_MI_ORIENTATION_ROTATE270:
                            {
                                if ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].prepare_rotation == BOOL_TRUE )
                                {
                                    REG_SET_SLICE( mi_sp_y_pic_width, MRV_MI_SP_Y_PIC_WIDTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width );
                                    REG_SET_SLICE( mi_sp_y_llength, MRV_MI_SP_Y_LLENGTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width );
                                    REG_SET_SLICE( mi_sp_y_pic_height, MRV_MI_SP_Y_PIC_HEIGHT,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height );

                                    ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].prepare_rotation = BOOL_FALSE;
                                }
                                else
                                {
                                    REG_SET_SLICE( mi_sp_y_pic_width, MRV_MI_SP_Y_PIC_WIDTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height );
                                    REG_SET_SLICE( mi_sp_y_llength, MRV_MI_SP_Y_LLENGTH,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height );
                                    REG_SET_SLICE( mi_sp_y_pic_height, MRV_MI_SP_Y_PIC_HEIGHT,
                                            ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width );

                                    REG_SET_SLICE( mi_ctrl, MRV_MI_ROT      , 1U );
                                    REG_SET_SLICE( mi_ctrl, MRV_MI_V_FLIP   , 1U );
                                    REG_SET_SLICE( mi_ctrl, MRV_MI_H_FLIP   , 1U );
                                }

                                break;
                            }


                        default:
                            {
                                return ( RET_FAILURE );
                            }
                    }

                    REG_SET_SLICE( mi_sp_y_pic_size, MRV_MI_SP_Y_PIC_SIZE,
                            ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width
                                * ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height ) );

                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_y_pic_width) , mi_sp_y_pic_width );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_y_llength)   , mi_sp_y_pic_width);
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_y_pic_height), mi_sp_y_pic_height );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_y_pic_size)  , mi_sp_y_pic_size );

                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_ctrl), mi_ctrl );

                    TRACE( CAMERIC_MI_DRV_INFO, "%s: SP (0x%08x,%d, 0x%08x,%d, 0x%08x,%d)\n",
                            __FUNCTION__, baseY, sizeY, baseCb, sizeCb, baseCr, sizeCr);

                    TRACE( CAMERIC_MI_DRV_INFO, "%s: SP SHD (0x%08x, 0x%08x, 0x%08x)\n",
                            __FUNCTION__,
                            HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_y_base_ad_shd)   ),
                            HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_cb_base_ad_shd)  ),
                            HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_cr_base_ad_shd)  ) );

                    TRACE( CAMERIC_MI_DRV_INFO, "%s: Byte Cnt (0x%08x)\n",
                            __FUNCTION__,  HalReadReg(ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_byte_cnt)) );

                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_y_base_ad_init)     , (baseY & MRV_MI_MP_Y_BASE_AD_INIT_MASK) );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_y_size_init)        , (sizeY & MRV_MI_MP_Y_SIZE_INIT_MASK) );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_y_offs_cnt_init)    , 0UL );

                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_cb_base_ad_init)    , (baseCb & MRV_MI_MP_CB_BASE_AD_INIT_MASK) );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_cb_size_init)       , (sizeCb & MRV_MI_MP_CB_SIZE_INIT_MASK)  );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_cb_offs_cnt_init)   , 0UL );

                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_cr_base_ad_init)    , (baseCr & MRV_MI_MP_CR_BASE_AD_INIT_MASK ) );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_cr_size_init)       , (sizeCr & MRV_MI_MP_CR_SIZE_INIT_MASK) );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_cr_offs_cnt_init)   , 0UL );

                    break;
                }

            default:
                {
                    TRACE( CAMERIC_MI_DRV_ERROR, "%s: unknown data-path\n", __FUNCTION__);
                    result = RET_FAILURE;
                    break;
                }
        }
    }

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/******************************************************************************
 * Implementation of Driver Internal API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcMiInit()
 *****************************************************************************/
RESULT CamerIcMiInit
(
    CamerIcDrvHandle_t  handle
)
{
    RESULT result = RET_SUCCESS;

    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    ctx->pMiContext = (CamerIcMiContext_t *)malloc( sizeof(CamerIcMiContext_t) );
    if (  ctx->pMiContext == NULL )
    {
        TRACE( CAMERIC_MI_DRV_ERROR,  "%s: Can't allocate CamerIC Mi context\n",  __FUNCTION__ );

        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pMiContext, 0, sizeof(CamerIcMiContext_t) );

    ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].in_mode = CAMERIC_MI_DATAMODE_YUV422;
    ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].out_mode = CAMERIC_MI_DATAMODE_DISABLED;
    ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].orientation = CAMERIC_MI_ORIENTATION_ORIGINAL;

    ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].in_mode = CAMERIC_MI_DATAMODE_YUV422;
    ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode = CAMERIC_MI_DATAMODE_DISABLED;
    ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].orientation = CAMERIC_MI_ORIENTATION_ORIGINAL;

    ctx->pMiContext->numFramesToSkip = 0;

    osMutexInit(&ctx->pMiContext->buffer_mutex);

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * CamerIcMiRelease()
 *****************************************************************************/
RESULT CamerIcMiRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pMiContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    osMutexDestroy(&ctx->pMiContext->buffer_mutex);
    MEMSET( ctx->pMiContext, 0, sizeof( CamerIcMiContext_t ) );
    free ( ctx->pMiContext);
    ctx->pMiContext = NULL;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcMiStart()
 *****************************************************************************/
RESULT CamerIcMiStart
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;
    RESULT result;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pMiContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
	ctx->pMiContext->miFrameNum = 0;

    result = CamerIcMiCheckConfiguration( ctx->pMiContext );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    if ( ( (ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].out_mode != CAMERIC_MI_DATAMODE_DISABLED) ||
           (ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode != CAMERIC_MI_DATAMODE_DISABLED) )  
           &&
           (ctx->pMiContext->RequestCb.func == NULL) )
    {
        return ( RET_FAILURE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t mi_imsc = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_imsc) );
        uint32_t mi_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_ctrl) );
        uint32_t mi_xtd_format_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_xtd_format_ctrl) );

        /* disable byte swap */
        if(ctx->isSwapByte && (( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].out_mode ) == CAMERIC_MI_DATAMODE_RAW12)){
            REG_SET_SLICE( mi_ctrl, MRV_MI_BYTE_SWAP, 1 );//soc camera test
        }else{
            REG_SET_SLICE( mi_ctrl, MRV_MI_BYTE_SWAP, 0);//soc camera test
        }
        REG_SET_SLICE( mi_ctrl, MRV_MI_PATH_ENABLE, 0 );

        /* register interrupt handler for the MI at hal */
        ctx->pMiContext->HalIrqCtx.misRegAddress = ( (ulong_t)&ptCamerIcRegMap->mi_mis );
        ctx->pMiContext->HalIrqCtx.icrRegAddress = ( (ulong_t)&ptCamerIcRegMap->mi_icr );

        result = HalConnectIrq( ctx->HalHandle, &ctx->pMiContext->HalIrqCtx, 0, NULL, &CamerIcMiIrq, ctx );
        DCT_ASSERT( (result == RET_SUCCESS) );

        switch ( ctx->pMiContext->y_burstlength )
        {
            case CAMERIC_MI_BURSTLENGTH_4:
                {
                    REG_SET_SLICE( mi_ctrl, MRV_MI_BURST_LEN_LUM, MRV_MI_BURST_LEN_LUM_4 );
                    break;
                }

            case CAMERIC_MI_BURSTLENGTH_8:
                {
                    REG_SET_SLICE( mi_ctrl, MRV_MI_BURST_LEN_LUM, MRV_MI_BURST_LEN_LUM_8 );
                    break;
                }

            case CAMERIC_MI_BURSTLENGTH_16:
                {
                    REG_SET_SLICE( mi_ctrl, MRV_MI_BURST_LEN_LUM, MRV_MI_BURST_LEN_LUM_16 );
                    break;
                }

            default:
                {
                    TRACE( CAMERIC_MI_DRV_ERROR, "%s: unknown dma luminance burst length (%d)\n", __FUNCTION__, (int32_t)ctx->pMiContext->y_burstlength);
                    return ( RET_INVALID_PARM );
                }
        }

        switch ( ctx->pMiContext->c_burstlength )
        {
            case CAMERIC_MI_BURSTLENGTH_4:
                {
                    REG_SET_SLICE( mi_ctrl, MRV_MI_BURST_LEN_CHROM, MRV_MI_BURST_LEN_CHROM_4 );
                    break;
                }

            case CAMERIC_MI_BURSTLENGTH_8:
                {
                    REG_SET_SLICE( mi_ctrl, MRV_MI_BURST_LEN_CHROM, MRV_MI_BURST_LEN_CHROM_8 );
                    break;
                }

            case CAMERIC_MI_BURSTLENGTH_16:
                {
                    REG_SET_SLICE( mi_ctrl, MRV_MI_BURST_LEN_CHROM, MRV_MI_BURST_LEN_CHROM_16 );
                    break;
                }

            default:
                {
                    TRACE( CAMERIC_MI_DRV_ERROR, "%s: unknown dma chrominance burst length (%d)\n", __FUNCTION__, (uint32_t)ctx->pMiContext->c_burstlength);
                    return ( RET_INVALID_PARM );
                }
        }

        /* main path enabled ? */
        if ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].out_mode != CAMERIC_MI_DATAMODE_DISABLED )
        {
            /* check MI output format versus MI input format */
            if ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].out_mode == CAMERIC_MI_DATAMODE_YUV420 )
            {
                ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].in_mode = CAMERIC_MI_DATAMODE_YUV420;
            }
            else
            {
                ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].in_mode = CAMERIC_MI_DATAMODE_YUV422;
            }

            /* request buffers for mainpath */
            result = CamerIcRequestAndSetupBuffers( ctx, CAMERIC_MI_PATH_MAIN );
            if ( result != RET_SUCCESS )
            {
                return ( result );
            }

            /* setup mi for main path */
            mi_ctrl &= ~( MRV_MI_MP_WRITE_FORMAT_MASK );
            switch ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].out_mode )
            {
                 case ( CAMERIC_MI_DATAMODE_RAW8 ):
                    {
                        REG_SET_SLICE( mi_ctrl, MRV_MI_MP_WRITE_FORMAT, MRV_MI_MP_WRITE_FORMAT_RAW_8 );
                        REG_SET_SLICE( mi_ctrl, MRV_MI_RAW_ENABLE, 1 );
                        break;
                    }

                 case ( CAMERIC_MI_DATAMODE_RAW12 ):
                    {
                        REG_SET_SLICE( mi_ctrl, MRV_MI_MP_WRITE_FORMAT, MRV_MI_MP_WRITE_FORMAT_RAW_12 );
                        REG_SET_SLICE( mi_ctrl, MRV_MI_RAW_ENABLE, 1 );
                        break;
                    }

                 case ( CAMERIC_MI_DATAMODE_JPEG ):
                    {
                        REG_SET_SLICE( mi_ctrl, MRV_MI_MP_WRITE_FORMAT, MRV_MI_MP_WRITE_FORMAT_PLANAR );
                        REG_SET_SLICE( mi_ctrl, MRV_MI_JPEG_ENABLE, 1);
                        break;
                    }

                 case ( CAMERIC_MI_DATAMODE_YUV444 ):
                 case ( CAMERIC_MI_DATAMODE_YUV422 ):
                 case ( CAMERIC_MI_DATAMODE_YUV420 ):
                 case ( CAMERIC_MI_DATAMODE_YUV400 ):
                    {
                        if ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].datalayout == CAMERIC_MI_DATASTORAGE_PLANAR )
                        {
                            REG_SET_SLICE( mi_ctrl, MRV_MI_MP_WRITE_FORMAT, MRV_MI_MP_WRITE_FORMAT_PLANAR );
                        }
                        else if ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].datalayout == CAMERIC_MI_DATASTORAGE_SEMIPLANAR )
                        {
                            REG_SET_SLICE( mi_ctrl, MRV_MI_MP_WRITE_FORMAT, MRV_MI_MP_WRITE_FORMAT_SEMIPLANAR );
                        }
                        else if ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].datalayout == CAMERIC_MI_DATASTORAGE_INTERLEAVED )
                        {
                            REG_SET_SLICE( mi_ctrl, MRV_MI_MP_WRITE_FORMAT, MRV_MI_MP_WRITE_FORMAT_INTERLEAVED );
                        }
                        else
                        {
                            return ( RET_FAILURE );
                        }

                        REG_SET_SLICE( mi_ctrl, MRV_MI_MP_ENABLE, 1);

                        break;
                    }
                 
                 case ( CAMERIC_MI_DATAMODE_DPCC ):
                    {
                        REG_SET_SLICE( mi_ctrl, MRV_MI_MP_WRITE_FORMAT, MRV_MI_MP_WRITE_FORMAT_PLANAR );
                        REG_SET_SLICE( mi_ctrl, MRV_MI_DPCC_ENABLE, 3);
                        REG_SET_SLICE( mi_ctrl, MRV_MI_MP_ENABLE, 1);
                        break;
                    }

                 default:
                    {
                        return ( RET_FAILURE );
                    }
            }

            result = CamerIcSetupScaling( ctx, CAMERIC_MI_PATH_MAIN,BOOL_FALSE);
            if ( result != RET_SUCCESS )
            {
                return ( result );
            }

            REG_SET_SLICE( mi_ctrl, MRV_MI_MP_AUTO_UPDATE, 1);
           
            /* set color channel swapping for main-path */
            REG_SET_SLICE( mi_xtd_format_ctrl, MRV_MI_NV21_MAIN,
                                (ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].swap_UV) ? 1u : 0u );

            /* enable frame end irq for  main path */
            mi_imsc |= ( MRV_MI_MP_FRAME_END_MASK ); // | MRV_MI_WRAP_MP_Y_MASK | MRV_MI_WRAP_MP_CB_MASK | MRV_MI_WRAP_MP_CR_MASK );
        }

        /* self path enabled ? */
        if ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode != CAMERIC_MI_DATAMODE_DISABLED )
        {
            uint32_t mi_sp_y_pic_width  = 0UL;
            uint32_t mi_sp_y_pic_height = 0UL;
            uint32_t mi_sp_y_pic_size   = 0UL;
            uint32_t mi_sp_y_llength    = 0UL;
    
            /* check MI output format versus MI input format */
            if ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode == CAMERIC_MI_DATAMODE_YUV420 )
            {
                ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].in_mode = CAMERIC_MI_DATAMODE_YUV420;
            }
            else
            {
                ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].in_mode = CAMERIC_MI_DATAMODE_YUV422;
            }

            /* request buffers for self-path */
            result = CamerIcRequestAndSetupBuffers( ctx, CAMERIC_MI_PATH_SELF );
            if ( result != RET_SUCCESS )
            {
                return ( result );
            }

            /* setup mi for self-path */
            mi_ctrl &= ~( MRV_MI_SP_WRITE_FORMAT_MASK );

            switch ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].in_mode )
            {
                case ( CAMERIC_MI_DATAMODE_YUV444 ):
                    {
                        REG_SET_SLICE( mi_ctrl, MRV_MI_SP_INPUT_FORMAT,  MRV_MI_SP_INPUT_FORMAT_YUV444 );
                        break;
                    }

                 case ( CAMERIC_MI_DATAMODE_YUV422 ):
                     {
                        REG_SET_SLICE( mi_ctrl, MRV_MI_SP_INPUT_FORMAT,  MRV_MI_SP_INPUT_FORMAT_YUV422 );
                        break;
                    }

                 case ( CAMERIC_MI_DATAMODE_YUV420 ):
                    {
                        REG_SET_SLICE( mi_ctrl, MRV_MI_SP_INPUT_FORMAT,  MRV_MI_SP_INPUT_FORMAT_YUV420 );
                        break;
                    }

                case ( CAMERIC_MI_DATAMODE_YUV400 ):
                    {
                        REG_SET_SLICE( mi_ctrl, MRV_MI_SP_INPUT_FORMAT,  MRV_MI_SP_INPUT_FORMAT_YUV400 );
                        break;
                    }

                default:
                    {
                        return ( RET_FAILURE );
                    }
            }

            switch ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode )
            {
                case ( CAMERIC_MI_DATAMODE_RGB888 ):
                    {
                        REG_SET_SLICE( mi_ctrl, MRV_MI_SP_OUTPUT_FORMAT, MRV_MI_SP_OUTPUT_FORMAT_RGB888 );
                        break;
                    }

                case ( CAMERIC_MI_DATAMODE_RGB666 ):
                    {
                        REG_SET_SLICE( mi_ctrl, MRV_MI_SP_OUTPUT_FORMAT, MRV_MI_SP_OUTPUT_FORMAT_RGB666 );
                        break;
                    }

                case ( CAMERIC_MI_DATAMODE_RGB565 ):
                    {
                        REG_SET_SLICE( mi_ctrl, MRV_MI_SP_OUTPUT_FORMAT, MRV_MI_SP_OUTPUT_FORMAT_RGB565 );
                        break;
                    }

                case ( CAMERIC_MI_DATAMODE_YUV444 ):
                    {
                        REG_SET_SLICE( mi_ctrl, MRV_MI_SP_OUTPUT_FORMAT, MRV_MI_SP_OUTPUT_FORMAT_YUV444 );
                        break;
                    }

                 case ( CAMERIC_MI_DATAMODE_YUV422 ):
                     {
                        REG_SET_SLICE( mi_ctrl, MRV_MI_SP_OUTPUT_FORMAT, MRV_MI_SP_OUTPUT_FORMAT_YUV422 );
                        break;
                    }

                 case ( CAMERIC_MI_DATAMODE_YUV420 ):
                    {
                        REG_SET_SLICE( mi_ctrl, MRV_MI_SP_OUTPUT_FORMAT, MRV_MI_SP_OUTPUT_FORMAT_YUV420 );
                        break;
                    }

                case ( CAMERIC_MI_DATAMODE_YUV400 ):
                    {
                        REG_SET_SLICE( mi_ctrl, MRV_MI_SP_OUTPUT_FORMAT, MRV_MI_SP_OUTPUT_FORMAT_YUV400 );
                        break;
                    }

                default:
                    {
                        return ( RET_FAILURE );
                    }
            }

            switch ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode )
            {
                case ( CAMERIC_MI_DATAMODE_RGB888 ):
                case ( CAMERIC_MI_DATAMODE_RGB666 ):
                case ( CAMERIC_MI_DATAMODE_RGB565 ):
                    {
                        /* ignore data layout in RGB mode, it's always interleaved */
                        REG_SET_SLICE( mi_ctrl, MRV_MI_SP_WRITE_FORMAT, MRV_MI_SP_WRITE_FORMAT_RGB_INTERLEAVED );
                        break;
                    }

                case ( CAMERIC_MI_DATAMODE_YUV444 ):
                case ( CAMERIC_MI_DATAMODE_YUV400 ):
                    {
                        if ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].datalayout == CAMERIC_MI_DATASTORAGE_PLANAR )
                        {
                            REG_SET_SLICE( mi_ctrl, MRV_MI_SP_WRITE_FORMAT, MRV_MI_SP_WRITE_FORMAT_PLANAR );
                        }
                        else
                        {
                            return ( RET_FAILURE );
                        }

                        break;
                    }

                case ( CAMERIC_MI_DATAMODE_YUV422 ):
                case ( CAMERIC_MI_DATAMODE_YUV420 ):
                    {
                        switch ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].datalayout )
                        {

                            case ( CAMERIC_MI_DATASTORAGE_PLANAR ):
                                {
                                    REG_SET_SLICE( mi_ctrl, MRV_MI_SP_WRITE_FORMAT, MRV_MI_SP_WRITE_FORMAT_PLANAR );
                                    break;
                                }

                            case ( CAMERIC_MI_DATASTORAGE_SEMIPLANAR ):
                                {
                                    REG_SET_SLICE( mi_ctrl, MRV_MI_SP_WRITE_FORMAT, MRV_MI_SP_WRITE_FORMAT_SEMIPLANAR );
                                    break;
                                }

                            case ( CAMERIC_MI_DATASTORAGE_INTERLEAVED ):
                                {
                                    REG_SET_SLICE( mi_ctrl, MRV_MI_SP_WRITE_FORMAT, MRV_MI_SP_WRITE_FORMAT_INTERLEAVED );
                                    break;
                                }

                            default:
                                {
                                    return ( RET_FAILURE );
                                }
                        }

                        break;
                    }

                default:
                    {
                        return ( RET_FAILURE );
                    }
            }

            result = CamerIcSetupScaling( ctx, CAMERIC_MI_PATH_SELF ,BOOL_FALSE);
            if ( result != RET_SUCCESS )
            {
                return ( result );
            }

            switch ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].orientation )
            {
                case CAMERIC_MI_ORIENTATION_ORIGINAL:
                    {
                        REG_SET_SLICE( mi_sp_y_pic_width, MRV_MI_SP_Y_PIC_WIDTH,
                                ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width );
                        REG_SET_SLICE( mi_sp_y_llength, MRV_MI_SP_Y_LLENGTH,
                                ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width );
                        REG_SET_SLICE( mi_sp_y_pic_height, MRV_MI_SP_Y_PIC_HEIGHT,
                                ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height );
                        REG_SET_SLICE( mi_sp_y_pic_size, MRV_MI_SP_Y_PIC_SIZE,
                                ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_width
                                    * ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_height ) );

                        REG_SET_SLICE( mi_ctrl, MRV_MI_ROT      , 0U );
                        REG_SET_SLICE( mi_ctrl, MRV_MI_V_FLIP   , 0U );
                        REG_SET_SLICE( mi_ctrl, MRV_MI_H_FLIP   , 0U );

                        break;
                    }
                default:
                    {
                        return ( RET_FAILURE );
                    }
            }

            REG_SET_SLICE( mi_ctrl, MRV_MI_SP_ENABLE, 1);
            REG_SET_SLICE( mi_ctrl, MRV_MI_SP_AUTO_UPDATE, 1);
            
            /* set color channel swapping for main-path */
            REG_SET_SLICE( mi_xtd_format_ctrl, MRV_MI_NV21_SELF,
                                (ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].swap_UV) ? 1u : 0u );

            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_y_pic_width) , mi_sp_y_pic_width );
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_y_llength)   , mi_sp_y_pic_width);
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_y_pic_height), mi_sp_y_pic_height );
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_sp_y_pic_size)  , mi_sp_y_pic_size );

            /* enable frame end interrupt on self path */
            mi_imsc |= ( MRV_MI_SP_FRAME_END_MASK ); //| MRV_MI_WRAP_SP_Y_MASK | MRV_MI_WRAP_SP_CB_MASK | MRV_MI_WRAP_SP_CR_MASK );
        }

        /* program buffers for both pathes to hardware */
        if ( result == RET_SUCCESS )
        {
            uint32_t mi_init = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_init) );

            int32_t i;

            /* remove update enable bits for offset and base registers */
            mi_ctrl &= ~( MRV_MI_INIT_BASE_EN_MASK | MRV_MI_INIT_OFFSET_EN_MASK );

            for ( i=0; i<2; i++ )
            {
                if ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].out_mode != CAMERIC_MI_DATAMODE_DISABLED )
                {
                    MediaBuffer_t *pBuffer       = NULL;
                    PicBufMetaData_t *pPicBuffer = NULL;

                    pBuffer = ( i == 0 )
                                ? ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].pShdBuffer
                                : ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].pBuffer;
                    if ( pBuffer != NULL )
                    {
                        pPicBuffer = (PicBufMetaData_t *)pBuffer->pMetaData;

                        /* program buffer to hardware */
                        result =  CamerIcMiSetBuffer( ctx, CAMERIC_MI_PATH_MAIN, pPicBuffer );
                        DCT_ASSERT( result == RET_SUCCESS );

                        /* we successfully programmed a buffer to marvin, so we need
                         * to enable updateing of base and offset registers */
                        mi_ctrl |= ( MRV_MI_INIT_BASE_EN_MASK | MRV_MI_INIT_OFFSET_EN_MASK );
                    }
                }

                if ( ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].out_mode != CAMERIC_MI_DATAMODE_DISABLED )
                {
                    MediaBuffer_t *pBuffer       = NULL;
                    PicBufMetaData_t *pPicBuffer = NULL;

                    pBuffer = ( i == 0 )
                                ? ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].pShdBuffer
                                : ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].pBuffer;
                    if ( pBuffer != NULL )
                    {
                        pPicBuffer = (PicBufMetaData_t *)pBuffer->pMetaData;

                        /* program buffer to hardware */
                        result =  CamerIcMiSetBuffer( ctx, CAMERIC_MI_PATH_SELF, pPicBuffer );
                        DCT_ASSERT( result == RET_SUCCESS );

                        /* we successfully programmed a buffer to marvin, so we need
                         * to enable updateing of base and offset registers */
                        mi_ctrl |= ( MRV_MI_INIT_BASE_EN_MASK | MRV_MI_INIT_OFFSET_EN_MASK );
                    }
                }

                /* update shadow registers only for the first buffer */
                if ( i == 0 )
                {
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_ctrl), mi_ctrl );

                    /* force updating of shadow registers, if enabled in mi_ctrl */
                    REG_SET_SLICE( mi_init, MRV_MI_MI_CFG_UPD, 1 );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_init), mi_init );
                }
            }
        }

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_xtd_format_ctrl), mi_xtd_format_ctrl );

        /* 3.) activate event handling by seting masked register value */
        // mi_imsc |= MRV_MI_DMA_READY_MASK;
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_imsc), mi_imsc );
    }

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcMiStop()
 *****************************************************************************/
RESULT CamerIcMiStop
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pMiContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        RESULT result = RET_SUCCESS;

        /* stop interrupts of CamerIc MI Module */
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_imsc), 0UL );

        /* disconnect IRQ handler */
        result = HalDisconnectIrq( &ctx->pMiContext->HalIrqCtx );
        DCT_ASSERT( (result == RET_SUCCESS) );

        CamerIcFlushAllBuffers( CAMERIC_MI_PATH_MAIN, ctx->pMiContext );
        CamerIcFlushAllBuffers( CAMERIC_MI_PATH_SELF, ctx->pMiContext );

        /* reset picture orientation */
        ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN].orientation = CAMERIC_MI_ORIENTATION_ORIGINAL;
        ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_SELF].orientation = CAMERIC_MI_ORIENTATION_ORIGINAL;
    }

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcMiStartLoadPicture()
 *****************************************************************************/
RESULT CamerIcMiStartLoadPicture
(
    CamerIcDrvHandle_t      handle,
    PicBufMetaData_t        *pPicBuffer,
    bool_t                  cont
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter %08x)\n", __FUNCTION__, (ulong_t)handle );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pMiContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pPicBuffer == NULL )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t mi_imsc;
        uint32_t mi_dma_ctrl;

        ulong_t baseY = 0;
        uint32_t widthY = 0;
        uint32_t heightY = 0;

        ulong_t baseCb = 0;
        ulong_t baseCr = 0;

        mi_dma_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_dma_ctrl) );

        switch ( pPicBuffer->Type )
        {
            case PIC_BUF_TYPE_RAW16:
                {
                    if ( (pPicBuffer->Layout == PIC_BUF_LAYOUT_BAYER_RGRGGBGB)
                            || (pPicBuffer->Layout == PIC_BUF_LAYOUT_BAYER_GRGRBGBG)
                            || (pPicBuffer->Layout == PIC_BUF_LAYOUT_BAYER_GBGBRGRG)
                            || (pPicBuffer->Layout == PIC_BUF_LAYOUT_BAYER_BGBGGRGR) )
                    {
                        baseY   = (ulong_t)pPicBuffer->Data.raw.pBuffer;
                        widthY  = (uint32_t)pPicBuffer->Data.raw.PicWidthPixel;
                        heightY = (uint32_t)pPicBuffer->Data.raw.PicHeightPixel;
                        baseCb = 0UL;
                        baseCr = 0UL;

                        REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_RGB_FORMAT, MRV_MI_DMA_RGB_FORMAT_16BIT_BAYER );
                        REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_READ_FORMAT, MRV_MI_DMA_READ_FORMAT_INTERLEAVED );
                        REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_INOUT_FORMAT, MRV_MI_DMA_INOUT_FORMAT_YUV422 );
                    }
                    else
                    {
                        TRACE( CAMERIC_MI_DRV_ERROR, "%s: invalid buffer layout\n", __FUNCTION__ );
                        return ( RET_INVALID_PARM );
                    }

                    break;
                }

            case PIC_BUF_TYPE_RAW8:
                {
                    if ( (pPicBuffer->Layout == PIC_BUF_LAYOUT_BAYER_RGRGGBGB)
                            || (pPicBuffer->Layout == PIC_BUF_LAYOUT_BAYER_GRGRBGBG)
                            || (pPicBuffer->Layout == PIC_BUF_LAYOUT_BAYER_GBGBRGRG)
                            || (pPicBuffer->Layout == PIC_BUF_LAYOUT_BAYER_BGBGGRGR) )
                    {
                        baseY   = (ulong_t)pPicBuffer->Data.raw.pBuffer;
                        widthY  = (uint32_t)pPicBuffer->Data.raw.PicWidthPixel;
                        heightY = (uint32_t)pPicBuffer->Data.raw.PicHeightPixel;
                        baseCb = 0UL;
                        baseCr = 0UL;

                        REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_RGB_FORMAT, MRV_MI_DMA_RGB_FORMAT_8BIT_BAYER );
                        REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_READ_FORMAT, MRV_MI_DMA_READ_FORMAT_INTERLEAVED );
                        REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_INOUT_FORMAT, MRV_MI_DMA_INOUT_FORMAT_YUV422 );
                    }
                    else
                    {
                        TRACE( CAMERIC_MI_DRV_ERROR, "%s: invalid buffer layout\n", __FUNCTION__ );
                        return ( RET_INVALID_PARM );
                    }

                    break;
                }

            case PIC_BUF_TYPE_YCbCr444:
                {
                    REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_RGB_FORMAT, MRV_MI_DMA_RGB_FORMAT_NO_DATA );
                    REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_INOUT_FORMAT, MRV_MI_DMA_INOUT_FORMAT_YUV444 );

                    if ( pPicBuffer->Layout == PIC_BUF_LAYOUT_PLANAR )
                    {
                        baseY   = (ulong_t)pPicBuffer->Data.YCbCr.planar.Y.pBuffer;
                        widthY  = (uint32_t)pPicBuffer->Data.YCbCr.planar.Y.PicWidthPixel;
                        heightY = (uint32_t)pPicBuffer->Data.YCbCr.planar.Y.PicHeightPixel;
                        baseCb  = (ulong_t)pPicBuffer->Data.YCbCr.planar.Cb.pBuffer;
                        baseCr  = (ulong_t)pPicBuffer->Data.YCbCr.planar.Cr.pBuffer;

                        REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_READ_FORMAT, MRV_MI_DMA_READ_FORMAT_PLANAR );
                    }
                    else
                    {
                        TRACE( CAMERIC_MI_DRV_ERROR, "%s: invalid buffer layout\n", __FUNCTION__ );
                        return ( RET_INVALID_PARM );
                    }

                    break;
                }

            case PIC_BUF_TYPE_YCbCr422:
                {
                    REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_RGB_FORMAT, MRV_MI_DMA_RGB_FORMAT_NO_DATA );
                    REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_INOUT_FORMAT, MRV_MI_DMA_INOUT_FORMAT_YUV422 );

                    if ( pPicBuffer->Layout == PIC_BUF_LAYOUT_PLANAR )
                    {
                        baseY   = (ulong_t)pPicBuffer->Data.YCbCr.planar.Y.pBuffer;
                        widthY  = (uint32_t)pPicBuffer->Data.YCbCr.planar.Y.PicWidthPixel;
                        heightY = (uint32_t)pPicBuffer->Data.YCbCr.planar.Y.PicHeightPixel;
                        baseCb  = (ulong_t)pPicBuffer->Data.YCbCr.planar.Cb.pBuffer;
                        baseCr  = (ulong_t)pPicBuffer->Data.YCbCr.planar.Cr.pBuffer;

                        REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_READ_FORMAT, MRV_MI_DMA_READ_FORMAT_PLANAR );
                    }
                    else if ( pPicBuffer->Layout == PIC_BUF_LAYOUT_SEMIPLANAR )
                    {
                        baseY   = (ulong_t)pPicBuffer->Data.YCbCr.semiplanar.Y.pBuffer;
                        widthY  = (uint32_t)pPicBuffer->Data.YCbCr.semiplanar.Y.PicWidthPixel;
                        heightY = (uint32_t)pPicBuffer->Data.YCbCr.semiplanar.Y.PicHeightPixel;
                        baseCb  = (ulong_t)pPicBuffer->Data.YCbCr.semiplanar.CbCr.pBuffer;
                        baseCr  = 0UL;

                        REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_READ_FORMAT, MRV_MI_DMA_READ_FORMAT_SEMIPLANAR );
                    }
                    else if ( pPicBuffer->Layout == PIC_BUF_LAYOUT_COMBINED )
                    {
                        baseY   = (ulong_t)pPicBuffer->Data.YCbCr.combined.pBuffer;
                        widthY  = (uint32_t)pPicBuffer->Data.YCbCr.combined.PicWidthPixel;
                        heightY = (uint32_t)pPicBuffer->Data.YCbCr.combined.PicHeightPixel;
                        baseCb  = 0UL;
                        baseCr  = 0UL;

                        REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_READ_FORMAT, MRV_MI_DMA_READ_FORMAT_INTERLEAVED );
                    }
                    else
                    {
                        TRACE( CAMERIC_MI_DRV_ERROR, "%s: invalid buffer layout\n", __FUNCTION__ );
                        return ( RET_INVALID_PARM );
                    }

                    break;
                }

            case PIC_BUF_TYPE_YCbCr420:
                {
                    REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_RGB_FORMAT, MRV_MI_DMA_RGB_FORMAT_NO_DATA );
                    REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_INOUT_FORMAT, MRV_MI_DMA_INOUT_FORMAT_YUV420 );

                    if ( pPicBuffer->Layout == PIC_BUF_LAYOUT_PLANAR )
                    {
                        baseY   = (ulong_t)pPicBuffer->Data.YCbCr.planar.Y.pBuffer;
                        widthY  = (uint32_t)pPicBuffer->Data.YCbCr.planar.Y.PicWidthPixel;
                        heightY = (uint32_t)pPicBuffer->Data.YCbCr.planar.Y.PicHeightPixel;
                        baseCb  = (ulong_t)pPicBuffer->Data.YCbCr.planar.Cb.pBuffer;
                        baseCr  = (ulong_t)pPicBuffer->Data.YCbCr.planar.Cr.pBuffer;

                        REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_READ_FORMAT, MRV_MI_DMA_READ_FORMAT_PLANAR );
                    }
                    else if ( pPicBuffer->Layout == PIC_BUF_LAYOUT_SEMIPLANAR )
                    {
                        baseY   = (ulong_t)pPicBuffer->Data.YCbCr.semiplanar.Y.pBuffer;
                        widthY  = (uint32_t)pPicBuffer->Data.YCbCr.semiplanar.Y.PicWidthPixel;
                        heightY = (uint32_t)pPicBuffer->Data.YCbCr.semiplanar.Y.PicHeightPixel;
                        baseCb  = (ulong_t)pPicBuffer->Data.YCbCr.semiplanar.CbCr.pBuffer;
                        baseCr  = 0UL;

                        REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_READ_FORMAT, MRV_MI_DMA_READ_FORMAT_SEMIPLANAR );
                    }
                    else
                    {
                        TRACE( CAMERIC_MI_DRV_ERROR, "%s: invalid buffer layout\n", __FUNCTION__ );
                        return ( RET_INVALID_PARM );
                    }

                    break;
                }

            case PIC_BUF_TYPE_YCbCr400:
                {
                    REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_RGB_FORMAT, MRV_MI_DMA_RGB_FORMAT_NO_DATA );
                    REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_INOUT_FORMAT, MRV_MI_DMA_INOUT_FORMAT_YUV400 );

                    if ( pPicBuffer->Layout == PIC_BUF_LAYOUT_PLANAR )
                    {
                        baseY   = (ulong_t)pPicBuffer->Data.YCbCr.planar.Y.pBuffer;
                        widthY  = (uint32_t)pPicBuffer->Data.YCbCr.planar.Y.PicWidthPixel;
                        heightY = (uint32_t)pPicBuffer->Data.YCbCr.planar.Y.PicHeightPixel;
                        baseCb  = 0UL;
                        baseCr  = 0UL;

                        REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_READ_FORMAT, MRV_MI_DMA_READ_FORMAT_PLANAR );
                    }
                    else
                    {
                        TRACE( CAMERIC_MI_DRV_ERROR, "%s: invalid buffer layout\n", __FUNCTION__ );
                        return ( RET_INVALID_PARM );
                    }

                    break;
                }


            default:
                {
                    TRACE( CAMERIC_MI_DRV_ERROR, "%s: unknown picture type (%d)\n", __FUNCTION__, (int32_t)pPicBuffer->Type);
                    return ( RET_INVALID_PARM );
                }
        }

        switch ( ctx->pMiContext->y_burstlength )
        {
            case CAMERIC_MI_BURSTLENGTH_4:
                {
                    REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_BURST_LEN_LUM, MRV_MI_DMA_BURST_LEN_LUM_4 );
                    break;
                }

            case CAMERIC_MI_BURSTLENGTH_8:
                {
                    REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_BURST_LEN_LUM, MRV_MI_DMA_BURST_LEN_LUM_8 );
                    break;
                }

            case CAMERIC_MI_BURSTLENGTH_16:
                {
                    REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_BURST_LEN_LUM, MRV_MI_DMA_BURST_LEN_LUM_16 );
                    break;
                }

            default:
                {
                    TRACE( CAMERIC_MI_DRV_ERROR, "%s: unknown dma luminance burst length (%d)\n", __FUNCTION__, (uint32_t)ctx->pMiContext->y_burstlength);
                    return ( RET_INVALID_PARM );
                }
        }

        switch ( ctx->pMiContext->c_burstlength )
        {
            case CAMERIC_MI_BURSTLENGTH_4:
                {
                    REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_BURST_LEN_CHROM, MRV_MI_DMA_BURST_LEN_CHROM_4 );
                    break;
                }

            case CAMERIC_MI_BURSTLENGTH_8:
                {
                    REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_BURST_LEN_CHROM, MRV_MI_DMA_BURST_LEN_CHROM_8 );
                    break;
                }

            case CAMERIC_MI_BURSTLENGTH_16:
                {
                    REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_BURST_LEN_CHROM, MRV_MI_DMA_BURST_LEN_CHROM_16 );
                    break;
                }

            default:
                {
                    TRACE( CAMERIC_MI_DRV_ERROR, "%s: unknown dma chrominance burst length (%d)\n", __FUNCTION__, (int32_t)ctx->pMiContext->c_burstlength);
                    return ( RET_INVALID_PARM );
                }
        }

        TRACE( CAMERIC_MI_DRV_INFO, "%s: start dma-transfer (y:0x%08x w*h:%d, cb:0x%08x, cr:0x%08x, dma-ctrl:0x%08x)\n",
                    __FUNCTION__,
                    ( MRV_MI_DMA_Y_PIC_START_AD_MASK & baseY ),
                    ( MRV_MI_DMA_Y_PIC_SIZE_MASK & (widthY * heightY) ),
                    ( MRV_MI_DMA_CB_PIC_START_AD_MASK & baseCb ),
                    ( MRV_MI_DMA_CR_PIC_START_AD_MASK & baseCr ),
                    mi_dma_ctrl);

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_dma_y_pic_start_ad),  ( MRV_MI_DMA_Y_PIC_START_AD_MASK & baseY ) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_dma_y_pic_width),     ( MRV_MI_DMA_Y_PIC_WIDTH_MASK & widthY ) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_dma_y_llength),       ( MRV_MI_DMA_Y_LLENGTH_MASK & widthY ) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_dma_y_pic_size),      ( MRV_MI_DMA_Y_PIC_SIZE_MASK & (widthY * heightY) ) );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_dma_cb_pic_start_ad), ( MRV_MI_DMA_CB_PIC_START_AD_MASK & baseCb ) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_dma_cr_pic_start_ad), ( MRV_MI_DMA_CR_PIC_START_AD_MASK & baseCr ) );

        REG_SET_SLICE( mi_dma_ctrl, MRV_MI_DMA_CONTINUOUS_EN, ((BOOL_TRUE == cont) ? 1U : 0U) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_dma_ctrl), mi_dma_ctrl);

        /* enable mi dma-ready interrupt */
        mi_imsc = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_imsc) );
        mi_imsc |= MRV_MI_DMA_READY_MASK;
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_imsc), mi_imsc );

        TRACE( CAMERIC_MI_DRV_INFO, "%s: mi_imsc: 0x%08x\n", __FUNCTION__, mi_imsc );

        uint32_t vi_dpcl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_dpcl) );
        TRACE( CAMERIC_MI_DRV_INFO, "%s: vi_dpcl: 0x%08x\n", __FUNCTION__, vi_dpcl );

        /* start dma-transfer */
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_dma_start), MRV_MI_DMA_START_MASK );
    }

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_PENDING );
}


/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcMiRegisterRequestCb()
 *****************************************************************************/
RESULT CamerIcMiRegisterRequestCb
(
    CamerIcDrvHandle_t      handle,
    CamerIcRequestFunc_t    func,
    void                    *pUserContext
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pMiContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)    &&
         (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( ctx->pMiContext->RequestCb.func != NULL )
    {
        return ( RET_BUSY );
    }

    if ( func == NULL )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        ctx->pMiContext->RequestCb.func          = func;
        ctx->pMiContext->RequestCb.pUserContext  = pUserContext;
    }

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcMiDeRegisterRequestCb()
 *****************************************************************************/
RESULT CamerIcMiDeRegisterRequestCb
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pMiContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)    &&
         (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    ctx->pMiContext->RequestCb.func          = NULL;
    ctx->pMiContext->RequestCb.pUserContext  = NULL;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcMiRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcMiRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void                *pUserContext
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pMiContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)    &&
         (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( ctx->pMiContext->EventCb.func != NULL )
    {
        return ( RET_BUSY );
    }

    if ( func == NULL )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        ctx->pMiContext->EventCb.func          = func;
        ctx->pMiContext->EventCb.pUserContext  = pUserContext;
    }

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcMiDeRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcMiDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pMiContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)    &&
         (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    ctx->pMiContext->EventCb.func          = NULL;
    ctx->pMiContext->EventCb.pUserContext  = NULL;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcMiSetBurstLength()
 *****************************************************************************/
RESULT CamerIcMiSetBurstLength
(
    CamerIcDrvHandle_t              handle,
    const CamerIcMiBurstLength_t    y_burstlength,
    const CamerIcMiBurstLength_t    c_burstlength
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pMiContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow burstlength changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)    &&
         (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    ctx->pMiContext->y_burstlength = y_burstlength;
    ctx->pMiContext->c_burstlength = c_burstlength;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcMiSetResolution()
 *****************************************************************************/
RESULT CamerIcMiSetResolution
(
    CamerIcDrvHandle_t          handle,
    const CamerIcMiPath_t       path,
    const uint32_t              in_width,
    const uint32_t              in_height,
    const uint32_t              out_width,
    const uint32_t              out_height
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pMiContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check path */
    if ( (path != CAMERIC_MI_PATH_MAIN) && (path != CAMERIC_MI_PATH_SELF) )
    {
        return ( RET_INVALID_PARM );
    }

    /* don't allow datamode changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)    &&
         (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    TRACE( CAMERIC_MI_DRV_INFO, "%s: set resolution %d (%d, %d)->(%d, %d)\n", __FUNCTION__,
                    (uint32_t)path, (uint32_t)in_width, (uint32_t)in_height, (uint32_t)out_width, (uint32_t)out_height);

    ctx->pMiContext->PathCtx[path].in_width     = in_width;
    ctx->pMiContext->PathCtx[path].in_height    = in_height;

    ctx->pMiContext->PathCtx[path].out_width    = out_width;
    ctx->pMiContext->PathCtx[path].out_height   = out_height;

    ctx->pMiContext->PathCtx[path].hscale       = ( in_width  == out_width ) ? BOOL_FALSE : BOOL_TRUE;
    ctx->pMiContext->PathCtx[path].vscale       = ( in_height == out_height ) ? BOOL_FALSE : BOOL_TRUE;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcMiSetDataMode()
 *****************************************************************************/
RESULT CamerIcMiSetDataMode
(
    CamerIcDrvHandle_t          handle,
    const CamerIcMiPath_t       path,
    const CamerIcMiDataMode_t   mode
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pMiContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check path */
    if ( (path != CAMERIC_MI_PATH_MAIN) && (path != CAMERIC_MI_PATH_SELF) )
    {
        return ( RET_INVALID_PARM );
    }

    /* range check */
    if ( (mode <= CAMERIC_MI_DATAMODE_INVALID)
            || (mode >= CAMERIC_MI_DATAMODE_MAX) )
    {
        return ( RET_OUTOFRANGE );
    }

    /* don't allow datamode changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)    &&
         (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    TRACE( CAMERIC_MI_DRV_INFO, "%s: set datamode (%d, %d)\n", __FUNCTION__, (uint32_t)path, (uint32_t)mode);
    ctx->pMiContext->PathCtx[path].out_mode = mode;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcMiSetDataLayout()
 *****************************************************************************/
RESULT CamerIcMiSetDataLayout
(
    CamerIcDrvHandle_t          handle,
    const CamerIcMiPath_t       path,
    const CamerIcMiDataLayout_t layout
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pMiContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check path */
    if ( (path != CAMERIC_MI_PATH_MAIN) && (path != CAMERIC_MI_PATH_SELF) )
    {
        return ( RET_INVALID_PARM );
    }

    /* range check */
    if ( (layout <= CAMERIC_MI_DATASTORAGE_INVALID)
            || (layout >= CAMERIC_MI_DATASTORAGE_MAX) )
    {
        return ( RET_OUTOFRANGE );
    }

    /* don't allow datamode changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)    &&
         (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    TRACE( CAMERIC_MI_DRV_INFO, "%s: set datalayout (%d, %d)\n", __FUNCTION__, (uint32_t)path, (uint32_t)layout);
    ctx->pMiContext->PathCtx[path].datalayout = layout;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcMiSwapColorChannel()
 *****************************************************************************/
RESULT CamerIcMiSwapColorChannel
(
    CamerIcDrvHandle_t          handle,
    const CamerIcMiPath_t       path,
    const bool_t                swap
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pMiContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check path */
    if ( (path != CAMERIC_MI_PATH_MAIN) && (path != CAMERIC_MI_PATH_SELF) )
    {
        return ( RET_INVALID_PARM );
    }

    /* don't allow datamode changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)    &&
         (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) &&
         (ctx->DriverState != CAMERIC_DRIVER_STATE_RUNNING) )
    {
        return ( RET_WRONG_STATE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t mi_xtd_format_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_xtd_format_ctrl) );

        /* set color channel swapping for main-path */
        if ( CAMERIC_MI_PATH_MAIN == path )
        {
            REG_SET_SLICE( mi_xtd_format_ctrl, MRV_MI_NV21_MAIN, (BOOL_TRUE == swap) ? 1u : 0u );
        }
        else
        {
            REG_SET_SLICE( mi_xtd_format_ctrl, MRV_MI_NV21_SELF, (BOOL_TRUE == swap) ? 1u : 0u );
        }

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->mi_xtd_format_ctrl), mi_xtd_format_ctrl );

        TRACE( CAMERIC_MI_DRV_INFO, "%s: swap color channel (%d, %d)\n", __FUNCTION__, (uint32_t)path, (swap) ? 1 : 0 );
        ctx->pMiContext->PathCtx[path].swap_UV = swap;
    }

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcMiIsPictureOrientationAllowed()
 *****************************************************************************/
RESULT CamerIcMiIsPictureOrientationAllowed
(
    CamerIcDrvHandle_t              handle,
    const CamerIcMiPath_t           path,
    const CamerIcMiOrientation_t    orientation
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pMiContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* only self path supports rotation and flipping */
    if ( path != CAMERIC_MI_PATH_SELF )
    {
        return ( RET_NOTSUPP );
    }
   
    /* range check */
    if ( (orientation <= CAMERIC_MI_ORIENTATION_INVALID)
            || (orientation >= CAMERIC_MI_ORIENTATION_MAX) )
    {
        return ( RET_INVALID_PARM );
    }

    switch ( orientation )
    {
        case CAMERIC_MI_ORIENTATION_HORIZONTAL_FLIP:
        case CAMERIC_MI_ORIENTATION_VERTICAL_FLIP:
        case CAMERIC_MI_ORIENTATION_ROTATE90:
        case CAMERIC_MI_ORIENTATION_ROTATE270:
            {
                if ( (ctx->pMiContext->PathCtx[path].out_mode != CAMERIC_MI_DATAMODE_RGB888)                &&
                        (ctx->pMiContext->PathCtx[path].out_mode != CAMERIC_MI_DATAMODE_RGB666)             &&
                        (ctx->pMiContext->PathCtx[path].out_mode != CAMERIC_MI_DATAMODE_RGB565)             && 
                        !( (ctx->pMiContext->PathCtx[path].out_mode == CAMERIC_MI_DATAMODE_YUV422) &&
                            (ctx->pMiContext->PathCtx[path].datalayout == CAMERIC_MI_DATASTORAGE_PLANAR) )  &&
                        !( (ctx->pMiContext->PathCtx[path].out_mode == CAMERIC_MI_DATAMODE_YUV444) &&
                            (ctx->pMiContext->PathCtx[path].datalayout == CAMERIC_MI_DATASTORAGE_PLANAR) )
                        )
                {
                    return ( RET_NOTSUPP );
                }
            }
            break;

        case CAMERIC_MI_ORIENTATION_ORIGINAL:
        default:
            break;
    }
    
    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcMiSetSelfPathOrientation()
 *****************************************************************************/
RESULT CamerIcMiSetPictureOrientation
(
    CamerIcDrvHandle_t              handle,
    const CamerIcMiPath_t           path,
    const CamerIcMiOrientation_t    orientation
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pMiContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* only self path supports rotation and flipping */
    if ( path != CAMERIC_MI_PATH_SELF )
    {
        return ( RET_NOTSUPP );
    }

    /* range check */
    if ( (orientation <= CAMERIC_MI_ORIENTATION_INVALID)
            || (orientation >= CAMERIC_MI_ORIENTATION_MAX) )
    {
        return ( RET_INVALID_PARM );
    }

    /* don't allow datamode changing if running */
    if ( ctx->DriverState != CAMERIC_DRIVER_STATE_RUNNING )
    {
        return ( RET_WRONG_STATE );
    }

    if ( ctx->pMiContext->PathCtx[path].orientation != orientation )
    {
        bool_t rotate = BOOL_FALSE;

        switch ( orientation )
        {
            case CAMERIC_MI_ORIENTATION_HORIZONTAL_FLIP:
            case CAMERIC_MI_ORIENTATION_VERTICAL_FLIP:
            case CAMERIC_MI_ORIENTATION_ROTATE90:
            case CAMERIC_MI_ORIENTATION_ROTATE270:
                {
                    if ( (ctx->pMiContext->PathCtx[path].out_mode != CAMERIC_MI_DATAMODE_RGB888)            &&
                         (ctx->pMiContext->PathCtx[path].out_mode != CAMERIC_MI_DATAMODE_RGB666)            &&
                         (ctx->pMiContext->PathCtx[path].out_mode != CAMERIC_MI_DATAMODE_RGB565)            && 
                         !( (ctx->pMiContext->PathCtx[path].out_mode == CAMERIC_MI_DATAMODE_YUV422) &&
                           (ctx->pMiContext->PathCtx[path].datalayout == CAMERIC_MI_DATASTORAGE_PLANAR) )   &&
                         !( (ctx->pMiContext->PathCtx[path].out_mode == CAMERIC_MI_DATAMODE_YUV444) &&
                           (ctx->pMiContext->PathCtx[path].datalayout == CAMERIC_MI_DATASTORAGE_PLANAR) )
                       )
                    {
                        TRACE( CAMERIC_MI_DRV_ERROR, "%s: rotation not allowed in YUV mode\n", __FUNCTION__ );
                        return ( RET_OUTOFRANGE );
                    }
                }
                break;

            default:
                {
                    break;
                }
        }

        /* check if we have a situation where we have to prepare a rotation */
        rotate = (
                    /* case 1: enable rotation  */
                    (((orientation == CAMERIC_MI_ORIENTATION_ROTATE90)
                        || (orientation == CAMERIC_MI_ORIENTATION_ROTATE270))
                        && ((ctx->pMiContext->PathCtx[path].orientation == CAMERIC_MI_ORIENTATION_ORIGINAL)
                               || (ctx->pMiContext->PathCtx[path].orientation == CAMERIC_MI_ORIENTATION_HORIZONTAL_FLIP)
                               || (ctx->pMiContext->PathCtx[path].orientation == CAMERIC_MI_ORIENTATION_VERTICAL_FLIP)
                               || (ctx->pMiContext->PathCtx[path].orientation == CAMERIC_MI_ORIENTATION_ROTATE180)))

                    /* case 2: disable rotation */
                    || (((orientation == CAMERIC_MI_ORIENTATION_ORIGINAL)
                                || (orientation == CAMERIC_MI_ORIENTATION_HORIZONTAL_FLIP)
                                || (orientation == CAMERIC_MI_ORIENTATION_VERTICAL_FLIP)
                                || (orientation == CAMERIC_MI_ORIENTATION_ROTATE180))
                            && ((ctx->pMiContext->PathCtx[path].orientation == CAMERIC_MI_ORIENTATION_ROTATE90)
                                || (ctx->pMiContext->PathCtx[path].orientation == CAMERIC_MI_ORIENTATION_ROTATE270)))
                    );

        /* note: we allowing to enable rotation during streaming, but we already
         * have a buffer in hardware so we need to prepare this action for the
         * next frame but one */
        ctx->pMiContext->PathCtx[path].prepare_rotation = rotate;

        TRACE( CAMERIC_MI_DRV_INFO, "%s: set picture orientation (%d, %d)\n", __FUNCTION__, (uint32_t)path, (uint32_t)orientation);
        ctx->pMiContext->PathCtx[path].orientation = orientation;
    }

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcMiSetFramesToSkip()
 *****************************************************************************/
RESULT CamerIcMiSetFramesToSkip
(
    CamerIcDrvHandle_t              handle,
    uint32_t                        numFramesToSkip
)
{
    RESULT result = RET_SUCCESS;

    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pMiContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow setting frames to skip if running */
    if ( ctx->DriverState == CAMERIC_DRIVER_STATE_RUNNING )
    {
        return ( RET_WRONG_STATE );
    }

    /* set frames to skip; must be IRQ safe */
    result = osAtomicSet( &ctx->pMiContext->numFramesToSkip, numFramesToSkip );

    TRACE( CAMERIC_MI_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * CamerIcMiGetBufferId()
 *****************************************************************************/
ulong_t CamerIcMiGetBufferId
(
    CamerIcDrvHandle_t      handle,
    const CamerIcMiPath_t   path
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    /* check precondition */
    DCT_ASSERT( ((ctx != NULL) && (ctx->pMiContext != NULL)) );

    /* check path and picture buffer pointer */
    if ( ( CAMERIC_MI_PATH_MAIN == path ) || 
         ( CAMERIC_MI_PATH_SELF == path ) )
    {
        TRACE( CAMERIC_MI_DRV_INFO, "%s: (id=0x%08x)\n", __FUNCTION__, (ulong_t)(ctx->pMiContext->PathCtx[path].pShdBuffer) );
        return ( (ulong_t)(ctx->pMiContext->PathCtx[path].pShdBuffer) );
    }

    return ( 0xFFFFFFFFUL );
}

uint32_t CamerIcSetDataPathWhileStreaming
(
    CamerIcDrvHandle_t      handle,
    CamerIcWindow_t* pWin,
    uint32_t outWidth, 
    uint32_t outHeight
    
)
{


    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;
    volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    uint32_t vi_dpcl,vi_dpcl_ori,dual_crop_ctrl;
    uint32_t sizeY,sizeCb;
    CamerIcMiDataPathContext_t *pMiPathCtx;

        osMutexLock(&ctx->pMiContext->buffer_mutex);
        //disable YC splitter
        vi_dpcl_ori = vi_dpcl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_dpcl) );
        REG_SET_SLICE(vi_dpcl, MRV_VI_CHAN_MODE, MRV_VI_CHAN_MODE_OFF);
 //       HalWriteReg( ctx->HalHandle, ((uint32_t)&ptCamerIcRegMap->vi_dpcl), vi_dpcl );



        dual_crop_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_ctrl) );

        REG_SET_SLICE( dual_crop_ctrl, MRV_dual_crop_DUAL_CROP_CFG_UPD, 0u  );

        uint32_t dual_crop_h_offs = 0u;
        uint32_t dual_crop_v_offs = 0u;
        uint32_t dual_crop_h_size = 0u;
        uint32_t dual_crop_v_size = 0u;

        pMiPathCtx = &ctx->pMiContext->PathCtx[CAMERIC_MI_PATH_MAIN];

        //have to filter frames ?
        if((pMiPathCtx->out_width != outWidth) || (pMiPathCtx->out_height != outHeight)){
            
            osAtomicSet( &ctx->pMiContext->numFramesToSkip, 2 );
            CamerIcSetupPictureBuffer( ctx, CAMERIC_MI_PATH_MAIN, pMiPathCtx->pBuffer );
            CamerIcSetupPictureBuffer( ctx, CAMERIC_MI_PATH_MAIN, pMiPathCtx->pShdBuffer);
        }
        
        pMiPathCtx->out_width = outWidth;
        pMiPathCtx->out_height= outHeight;

        pMiPathCtx->in_width = pWin->width;
        pMiPathCtx->in_height = pWin->height;


//  set crop
        {
        
            REG_SET_SLICE( dual_crop_h_offs, MRV_dual_crop_DUAL_CROP_M_H_OFFS, pWin->hOffset );
            REG_SET_SLICE( dual_crop_v_offs, MRV_dual_crop_DUAL_CROP_M_V_OFFS, pWin->vOffset );
            REG_SET_SLICE( dual_crop_h_size, MRV_dual_crop_DUAL_CROP_M_H_SIZE, pWin->width   );
            REG_SET_SLICE( dual_crop_v_size, MRV_dual_crop_DUAL_CROP_M_V_SIZE, pWin->height  );

            REG_SET_SLICE( dual_crop_ctrl, MRV_dual_crop_DUAL_CROP_CFG_UPD_PERMANENT, 1u  );

            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_m_h_offs), dual_crop_h_offs);
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_m_v_offs), dual_crop_v_offs);
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_m_h_size), dual_crop_h_size);
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_m_v_size), dual_crop_v_size);

            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_ctrl), dual_crop_ctrl );
        }

//set scale

        CamerIcSetupScaling(ctx,CAMERIC_MI_PATH_MAIN,BOOL_TRUE);

   //   restore YC splitter

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_dpcl), vi_dpcl_ori );

        TRACE( CAMERIC_MI_DRV_ERROR, "%s: width = %d,height = %d,skip = %d\n", __FUNCTION__,outWidth,outHeight,ctx->pMiContext->numFramesToSkip);
        osMutexUnlock(&ctx->pMiContext->buffer_mutex);
        
    return 0;
}
