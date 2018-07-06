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
 * @file cam_engine_cb.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <ebase/dct_assert.h>
#include <common/misc.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_drv_api.h>
#include <cameric_drv/cameric_isp_hist_drv_api.h>
#include <cameric_drv/cameric_isp_exp_drv_api.h>
#include <cameric_drv/cameric_isp_awb_drv_api.h>
#include <cameric_drv/cameric_isp_afm_drv_api.h>
#include <cameric_drv/cameric_isp_vsm_drv_api.h>

#include "cam_engine.h"
#include "cam_engine_cb.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/

CREATE_TRACER( CAM_ENGINE_CB_INFO , "CAM-ENGINE-CB: ", INFO   , 0 );
CREATE_TRACER( CAM_ENGINE_CB_WARN , "CAM-ENGINE-CB: ", WARNING, 1 );
CREATE_TRACER( CAM_ENGINE_CB_ERROR, "CAM-ENGINE-CB: ", ERROR  , 1 );

CREATE_TRACER( CAM_ENGINE_CB_DEBUG, "CAM-ENGINE-CB: ", INFO   , 0 );

/******************************************************************************
 * local type definitions
 *****************************************************************************/


/******************************************************************************
 * local variable declarations
 *****************************************************************************/
// match initialization values with values in CamEngineCamerIcDrvMeasureCbRestart() below
static uint32_t MeanCnt    = 0UL;
static uint32_t ExpCnt     = 0UL;
static uint32_t AwbCnt     = 0UL;
static uint32_t AfCnt      = 0UL;
static uint32_t AfCntMax   = 80UL;
static uint32_t AvsCnt     = 0UL;
static uint32_t AvsCntWait = 0UL;
//static uint32_t AvsCntWait = 25UL;


/******************************************************************************
 * local function prototypes
 *****************************************************************************/


 /******************************************************************************
 * See header file for detailed comment.
 *****************************************************************************/

/******************************************************************************
 * CamEngineCamerIcDrvMeasureCbRestart()
 *****************************************************************************/
RESULT CamEngineCamerIcDrvMeasureCbRestart
(
    CamEngineContext_t  *pCamEngineCtx,
    uint32_t            numFramesToSkip
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_CB_INFO, "%s (enter)\n", __FUNCTION__ );

    if (pCamEngineCtx == NULL)
    {
        return RET_WRONG_HANDLE;
    }

    UNUSED_PARAM(numFramesToSkip);

    // match with static initialization above
    MeanCnt    = 0UL;
    ExpCnt     = 0UL;
    AwbCnt     = 0UL;
    AfCnt      = 0UL;
    //AfCntMax   = 120UL;
    AfCntMax   = 10UL;
    AvsCnt     = 0UL;
    AvsCntWait = 25UL;
    //AvsCntWait = 0UL;


    AfMeasureCbRestart(pCamEngineCtx->hAf);

    TRACE( CAM_ENGINE_CB_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}



/******************************************************************************
 * CamEngineCamerIcDrvLockCb()
 *****************************************************************************/
void CamEngineCamerIcDrvLockCb
(
    const CamerIcEventId_t  evtId,
    void                    *pParam,
    void                    *pUserContext
)
{
    TRACE( CAM_ENGINE_CB_INFO, "%s (enter evt=%04x)\n", __FUNCTION__, (uint32_t)evtId );

    if ( pUserContext != NULL )
    {
        CamEngineContext_t *pCamEngineCtx = ( CamEngineContext_t * )pUserContext;
        RESULT result = RET_FAILURE;

        switch ( evtId )
        {
            case CAMERIC_ISP_EVENT_FRAME_OUT:
                {
                    TRACE( CAM_ENGINE_CB_DEBUG, "CAMERIC_ISP_EVENT_FRAME_OUT (%d)\n", evtId );

                    bool_t locked = BOOL_TRUE;

                    if ( (BOOL_TRUE == locked) && (pCamEngineCtx->LockMask & CAM_ENGINE_LOCK_AEC) )
                    {
                        TRACE( CAM_ENGINE_CB_DEBUG, "try to lock AEC\n", evtId );
                        result = AecTryLock( pCamEngineCtx->hAec );
                        DCT_ASSERT( (result == RET_SUCCESS) || (result == RET_PENDING) );
                        if ( RET_SUCCESS == result )
                        {
                            pCamEngineCtx->LockMask &= ~CAM_ENGINE_LOCK_AEC;
                            pCamEngineCtx->LockedMask |= CAM_ENGINE_LOCK_AEC;
                        }
                        else
                        {
                            // RET_PENDING
                            locked = BOOL_FALSE;
                        }
                    }

                    if ( (BOOL_TRUE == locked) && (pCamEngineCtx->LockMask & CAM_ENGINE_LOCK_AWB) )
                    {
                        TRACE( CAM_ENGINE_CB_DEBUG, "try to lock AWB\n", evtId );
                        result = AwbTryLock( pCamEngineCtx->hAwb );
                        DCT_ASSERT( (result == RET_SUCCESS) || (result == RET_PENDING) );
                        if ( RET_SUCCESS == result )
                        {
                            pCamEngineCtx->LockMask &= ~CAM_ENGINE_LOCK_AWB;
                            pCamEngineCtx->LockedMask |= CAM_ENGINE_LOCK_AWB;
                        }
                        else
                        {
                            // RET_PENDING
                            locked = BOOL_FALSE;
                        }
                    }

                    if ( pCamEngineCtx->LockMask & CAM_ENGINE_LOCK_AF )
                    {
                        TRACE( CAM_ENGINE_CB_DEBUG, "try to lock AF\n", evtId );
                        result = AfTryLock( pCamEngineCtx->hAf );
                        DCT_ASSERT( (result == RET_SUCCESS) || (result == RET_PENDING) );
                        if ( RET_SUCCESS == result )
                        {
                            pCamEngineCtx->LockMask &= ~CAM_ENGINE_LOCK_AF;
                            pCamEngineCtx->LockedMask |= CAM_ENGINE_LOCK_AF;
                        }
                        else
                        {
                            locked = BOOL_FALSE;
                        }
                    }

                    if ( BOOL_TRUE == locked )
                    {
                        CamEngineCmd_t command;
                        MEMSET( &command, 0, sizeof( CamEngineCmd_t ) );
                        command.cmdId = CAM_ENGINE_CMD_AAA_LOCKED;

                        result = CamEngineSendCommand( pCamEngineCtx, &command );
                        DCT_ASSERT( result == RET_SUCCESS );
                    }

                    break;
                }

            default:
                {
                    TRACE( CAM_ENGINE_CB_ERROR, "unknown Event (%d)\n", evtId );
                    break;
                }
        }
    }

    TRACE( CAM_ENGINE_CB_INFO, "%s (exit)\n", __FUNCTION__ );
}



/******************************************************************************
 * CamEngineCamerIcDrvJpeCb()
 *****************************************************************************/
void CamEngineCamerIcDrvJpeCb
(
    const CamerIcEventId_t  evtId,
    void                    *pParam,
    void                    *pUserContext
)
{
    TRACE( CAM_ENGINE_CB_INFO, "%s (enter evt=%04x)\n", __FUNCTION__, (uint32_t)evtId );

    if ( pUserContext != NULL )
    {
        CamEngineContext_t *pCamEngineCtx = ( CamEngineContext_t * )pUserContext;

        switch ( evtId )
        {
            case CAMERIC_JPE_EVENT_HEADER_GENERATED:
                {
                    (void)osEventSignal( &pCamEngineCtx->JpeEventGenHeader );
                    break;
                }

            case CAMERIC_JPE_EVENT_DATA_ENCODED:
                {
                    break;
                }

            default:
                {
                    TRACE( CAM_ENGINE_CB_ERROR, "unknown Event \n" );
                    break;
                }
        }
    }

    TRACE( CAM_ENGINE_CB_INFO, "%s (exit)\n", __FUNCTION__ );
}



/******************************************************************************
 * CamEngineCamerIcDrvMeasureCb()
 *****************************************************************************/
void CamEngineCamerIcDrvMeasureCb
(
    const CamerIcEventId_t  evtId,
    void                    *pParam,
    void                    *pUserContext
)
{
    RESULT result = RET_FAILURE;

    TRACE( CAM_ENGINE_CB_INFO, "%s (enter evt=%04x)\n", __FUNCTION__, (uint32_t)evtId );
    //for test ,zyc
  //  return;
    if ( (pUserContext != NULL) && ( pParam != NULL) )
    {
        CamEngineContext_t *pCamEngineCtx = ( CamEngineContext_t * )pUserContext;

        switch ( evtId )
        {

            case CAMERIC_ISP_EVENT_HISTOGRAM:
                {
                    TRACE( CAM_ENGINE_CB_DEBUG, "CAMERIC_ISP_EVENT_HISTOGRAM (#%d)\n", ExpCnt );

                    if ( ExpCnt == 1UL )
                    {
                        float fGain = 0.0f;
                        float fTi = 0.0f;

                        ExpCnt = 0;

                        result = AecClmExecute( pCamEngineCtx->hAec, pParam );
                        if ( (result != RET_SUCCESS) && (result != RET_CANCELED) )
                        {
                            TRACE( CAM_ENGINE_CB_ERROR, "%s AecClmExecute: %d", __FUNCTION__, result );
                        }
                        DCT_ASSERT( ((result == RET_SUCCESS) || (result == RET_CANCELED)) );

                        if ( pCamEngineCtx->mode != CAM_ENGINE_MODE_IMAGE_PROCESSING )
                        {
                            /* get current gain */
                            result = AecGetCurrentGain( pCamEngineCtx->hAec, &fGain );
                            DCT_ASSERT( result == RET_SUCCESS );

                            result = AecGetCurrentIntegrationTime( pCamEngineCtx->hAec, &fTi );
                            DCT_ASSERT( result == RET_SUCCESS );
                        }
                        else
                        {
                            fGain  = pCamEngineCtx->vGain;
                            fTi = pCamEngineCtx->vItime;
                        }

                        TRACE( CAM_ENGINE_CB_DEBUG, "%s gain: %f, Ti: %f\n", __FUNCTION__, fGain, fTi );

                        /* calc. denoising pre-filter with current gain */
                        result = AdpfProcessFrame( pCamEngineCtx->hAdpf, fGain );
                        if ( (result != RET_SUCCESS) && (result != RET_CANCELED) )
                        {
                            TRACE( CAM_ENGINE_CB_ERROR, "%s AdpfProcessFrame: %d", __FUNCTION__, result );
                        }
                        DCT_ASSERT( ((result == RET_SUCCESS) || (result == RET_CANCELED)) );

                        /* calc. defect pixel cluster filter with current gain */
                        result = AdpccProcessFrame( pCamEngineCtx->hAdpcc, fGain );
                        if ( (result != RET_SUCCESS) && (result != RET_CANCELED) )
                        {
                            TRACE( CAM_ENGINE_CB_ERROR, "%s AdpccProcessFrame: %d", __FUNCTION__, result );
                        }
                        DCT_ASSERT( ((result == RET_SUCCESS) || (result == RET_CANCELED)) );

                        result = AwbSetHistogram( pCamEngineCtx->hAwb, pParam );
                        if ( result != RET_SUCCESS )
                        {
                            TRACE( CAM_ENGINE_CB_ERROR, "%s AwbSetHistogram: %d", __FUNCTION__, result );
                        }
                        DCT_ASSERT( (result == RET_SUCCESS) );
                    }
                    else
                    {
                        ++ExpCnt;
                        TRACE( CAM_ENGINE_CB_DEBUG, "%s nc\n", __FUNCTION__ );
                    };

                    break;
                }

            case CAMERIC_ISP_EVENT_MEANLUMA:
                {
                    TRACE( CAM_ENGINE_CB_DEBUG, "CAMERIC_ISP_EVENT_MEANLUMA (#%d)\n", MeanCnt );

                    if ( MeanCnt == 2UL )
                    {
                        MeanCnt = 0;
                        result = AecSemExecute( pCamEngineCtx->hAec, pParam );
                        if ( (result != RET_SUCCESS) && (result != RET_CANCELED) )
                        {
                            TRACE( CAM_ENGINE_CB_ERROR, "%s AecSemExecute: %d", __FUNCTION__, result );
                        }
                        //zyc for test
                        //zhy  v1.0x10.1:
                        //DCT_ASSERT( ((result == RET_SUCCESS) || (result == RET_CANCELED)) );
                    }
                    else
                    {
                        ++MeanCnt;
                    }

                    break;
                }

            case CAMERIC_ISP_EVENT_AWB:
                {
                    bool_t settled;

                    TRACE( CAM_ENGINE_CB_DEBUG, "CAMERIC_ISP_EVENT_AWB (#%d)\n", AwbCnt );

                    result = AecSettled( pCamEngineCtx->hAec, &settled );
                    if ( (RET_SUCCESS == result) && (BOOL_TRUE == settled) )
                    {
                        if ( (AwbCnt == 2UL) )
                        {
                            float fIntergrationTime = 0.0f;
                            float fGain = 0.0f;

                            AwbCnt = 0;

                            if ( pCamEngineCtx->mode != CAM_ENGINE_MODE_IMAGE_PROCESSING )
                            {
                                /* get current gain */
                                result = AecGetCurrentGain( pCamEngineCtx->hAec, &fGain );
                                DCT_ASSERT( result == RET_SUCCESS );

                                result = AecGetCurrentIntegrationTime( pCamEngineCtx->hAec, &fIntergrationTime );
                                DCT_ASSERT( result == RET_SUCCESS );
                            }
                            else
                            {
                                fGain  = pCamEngineCtx->vGain;
                                fIntergrationTime = pCamEngineCtx->vItime;
                            }

                            result = AwbProcessFrame( pCamEngineCtx->hAwb, pParam, fGain, fIntergrationTime );
                            if ( (result != RET_SUCCESS) && (result != RET_CANCELED) )
                            {
                                TRACE( CAM_ENGINE_CB_ERROR, "%s AwbProcessFrame: %d", __FUNCTION__, result );
                            }
                            DCT_ASSERT( ((result == RET_SUCCESS) || (result == RET_CANCELED)) );
                        }
                        else
                        {
                            ++AwbCnt;
                        }
                    }

                    break;
                }

            case CAMERIC_ISP_EVENT_AFM:
                {
                    TRACE( CAM_ENGINE_CB_DEBUG, "CAMERIC_ISP_EVENT_AFM (#%d)\n", AfCnt );

                    #if 0
                    if ( (AfCnt > AfCntMax) &&  )
                    {
                        AfCnt = 0;
                        AfCntMax = 0;
                        result = AfProcessFrame( pCamEngineCtx->hAf, pParam );
                        if ( (result != RET_SUCCESS) && (result != RET_CANCELED) )
                        {
                            TRACE( CAM_ENGINE_CB_ERROR, "%s AfProcessFrame: %d", __FUNCTION__, result );
                        }
                        DCT_ASSERT( ((result == RET_SUCCESS) || (result == RET_CANCELED)) );
                    }
                    else
                    {
                        ++AfCnt;
                    }
                    #else
                    bool_t settled;
                    if (pCamEngineCtx->hAf != NULL) {                    
                        //result = AecSettled( pCamEngineCtx->hAec, &settled );
                        //if ( (RET_SUCCESS == result) && (BOOL_TRUE == settled) ) {
                            result = AfProcessFrame( pCamEngineCtx->hAf, pParam );
                            if ( (result != RET_SUCCESS) && (result != RET_CANCELED) )
                            {
                                TRACE( CAM_ENGINE_CB_ERROR, "%s AfProcessFrame: %d", __FUNCTION__, result );
                            }
                            DCT_ASSERT( ((result == RET_SUCCESS) || (result == RET_CANCELED)) );
                        //}
                    }
                    #endif
                    break;
                }

            case CAMERIC_ISP_EVENT_VSM:
                {
                    CamerIcIspVsmEventData_t *pEventData =
                        (CamerIcIspVsmEventData_t *) pParam;
                    CamerIcIspVsmDisplVec_t *pDisplVec = &pEventData->DisplVec;
                    CamEngineVector_t displVec;

                    MEMSET( &displVec, 0, sizeof(displVec) );

                    displVec.x = pDisplVec->delta_h;
                    displVec.y = pDisplVec->delta_v;

                    if ( (AvsCnt >= AvsCntWait) && (NULL != pCamEngineCtx->hAvs) )
                    {
                        bool_t running = BOOL_FALSE;
                        AvsCnt = 0;
                        AvsCntWait = 0;
                        TRACE( CAM_ENGINE_CB_DEBUG, "CAMERIC_ISP_EVENT_VSM (#%d)\n", AvsCnt );

                        result = AvsGetStatus( pCamEngineCtx->hAvs, &running, NULL, NULL );
                        if ( result != RET_SUCCESS )
                        {
                            TRACE( CAM_ENGINE_CB_ERROR, "%s AvsGetStatus: %d", __FUNCTION__, result );
                        }
                        DCT_ASSERT( result == RET_SUCCESS );

                        if ( running )
                        {
                            result = AvsProcessFrame( pCamEngineCtx->hAvs, pEventData->frameId, &displVec );
                            if ( (result != RET_SUCCESS) && (result != RET_CANCELED) )
                            {
                                TRACE( CAM_ENGINE_CB_ERROR, "%s AvsProcessFrame: %d", __FUNCTION__, result );
                            }
                            DCT_ASSERT( ((result == RET_SUCCESS) || (result == RET_CANCELED)) );
                        }
                    }
                    else
                    {
                        ++AvsCnt;
                    }
                    break;
                }

             default:
                {
                    TRACE( CAM_ENGINE_CB_ERROR, "unknown Event \n" );
                    break;
                }
        }
    }

    TRACE( CAM_ENGINE_CB_INFO, "%s (exit res=%d)\n", __FUNCTION__, result );
}



/******************************************************************************
 * simple cam-engine
 *
 * The following function are implementing a simple (MOCK) Camera-Engine.
 *
 *****************************************************************************/
void CamEngineCamerIcDrvCommandCb
(
    const uint32_t      cmdId,
    const RESULT        result,
    void                *pParam,
    void                *pUserContext
)
{
    UNUSED_PARAM( result );
    UNUSED_PARAM( pParam );

    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)pUserContext;

    TRACE( CAM_ENGINE_CB_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx != NULL )
    {
        switch ( cmdId )
        {
            case CAMERIC_ISP_COMMAND_CAPTURE_FRAMES:
            case CAMERIC_ISP_COMMAND_STOP_INPUT:
                {
                    CamEngineCmd_t command;

                    /* build start comand */
                    MEMSET( &command, 0, sizeof( CamEngineCmd_t ) );
                    command.cmdId = CAM_ENGINE_CMD_HW_STREAMING_FINISHED;

                    /* signal that capturing is completed */
                    CamEngineSendCommand( pCamEngineCtx, &command );
                    break;
                }

            default:
                {
                    TRACE( CAM_ENGINE_CB_WARN, "%s: unknown command #%d\n", __FUNCTION__, (uint32_t)cmdId );
                    break;
                }
        }
    }

    TRACE( CAM_ENGINE_CB_INFO, "%s (exit)\n", __FUNCTION__ );
}
