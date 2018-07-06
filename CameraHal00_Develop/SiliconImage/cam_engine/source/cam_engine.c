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
 * @file cam_engine.c
 *
 * @brief
 *   Implementation of the CamEngine.
 *
 *****************************************************************************/
#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <ebase/dct_assert.h>

#include <bufferpool/media_buffer.h>
#include <bufferpool/media_buffer_pool.h>
#include <bufferpool/media_buffer_queue_ex.h>

#include <common/utl_fixfloat.h>

#include "cam_engine.h"
#include "cam_engine_cb.h"
#include "cam_engine_subctrls.h"
#include "cam_engine_modules.h"
#include "cam_engine_drv.h"

#include <sys/time.h>


/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAM_ENGINE_INFO , "CAM-ENGINE: ", INFO   , 0 );
CREATE_TRACER( CAM_ENGINE_WARN , "CAM-ENGINE: ", WARNING, 0 );
CREATE_TRACER( CAM_ENGINE_ERROR, "CAM-ENGINE: ", ERROR  , 1 );
CREATE_TRACER( CAM_ENGINE_DEBUG, ""            , INFO   , 0 );

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
 * CamEngineSetState
 *****************************************************************************/
static inline void CamEngineSetState
(
    CamEngineContext_t   *pCamEngineCtx,
    CamEngineState_t     newState
);


/******************************************************************************
 * CamEngineCompleteCommand
 *****************************************************************************/
static void CamEngineCompleteCommand
(
    CamEngineContext_t   *pCamEngineCtx,
    CamEngineCmdId_t     cmdId,
    RESULT               result
);


/******************************************************************************
 * CamEngineThreadHandler
 *****************************************************************************/
static int32_t CamEngineThreadHandler
(
    void *p_arg
);


/******************************************************************************
 * See header file for detailed comment.
 *****************************************************************************/
static osMutex g_pCamEngineCtx_lock[2];
static CamEngineContext_t * g_pCamEngineCtx[2]={0,0};

/******************************************************************************
 * CamEngineCreate
 *****************************************************************************/
RESULT CamEngineCreate
(
    CamEngineContext_t *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    if ( CAM_ENGINE_STATE_INVALID != CamEngineGetState( pCamEngineCtx ) )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (wrong state %d)\n", __FUNCTION__, CamEngineGetState( pCamEngineCtx ));
        return ( RET_WRONG_STATE );
    }

    /* init camerIc driver */
    result = CamEngineInitCamerIcDrv( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (initialization of CamerIC driver failed)\n", __FUNCTION__ );
        goto cleanup_0;
    }

    /* init modules */
    result = CamEngineModulesInit( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (initialization of modules failed)\n", __FUNCTION__ );
        goto cleanup_1;
    }

    /* create command queue */
    if ( OSLAYER_OK != osQueueInit( &pCamEngineCtx->commandQueue, pCamEngineCtx->maxCommands, sizeof(CamEngineCmd_t) ) )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (creating command queue of depth: %d failed)\n", __FUNCTION__, pCamEngineCtx->maxCommands );
        result = RET_FAILURE;
        goto cleanup_2;
    }

    /* create handler thread */
    if ( OSLAYER_OK != osThreadCreate( &pCamEngineCtx->thread, CamEngineThreadHandler, pCamEngineCtx ) )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (creating handler thread failed)\n", __FUNCTION__ );
        result = RET_FAILURE;
        goto cleanup_3;
    }

    osMutexInit(&pCamEngineCtx->camEngineCtx_lock);

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    /* success */
    return ( RET_SUCCESS );

    /* failure cleanup */
cleanup_3: /* delete cmd queue */
    (void)osQueueDestroy( &pCamEngineCtx->commandQueue );

cleanup_2: /* release auto algo's */
    (void)CamEngineModulesRelease( pCamEngineCtx );

cleanup_1: /* release camerIC-Driver */
    (void)CamEngineReleaseCamerIcDrv( pCamEngineCtx );

cleanup_0: /* just return with error */

    return result;
}


/******************************************************************************
 * CamEngineDestroy
 *****************************************************************************/
RESULT CamEngineDestroy
(
    CamEngineContext_t *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;
    OSLAYER_STATUS osStatus;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    /* build shutdown command */
    CamEngineCmd_t command;
    MEMSET( &command, 0, sizeof(CamEngineCmd_t) );
    command.cmdId = CAM_ENGINE_CMD_SHUTDOWN;

    /* send handler thread a shutdown command */
    lres = CamEngineSendCommand( pCamEngineCtx, &command );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, lres );
    }

    /* wait for handler thread to have stopped due to the shutdown command given above */
    if ( OSLAYER_OK != osThreadWait( &pCamEngineCtx->thread ) )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (waiting for handler thread failed)\n", __FUNCTION__ );
        UPDATE_RESULT( result, RET_FAILURE );
    }

    /* destroy handler thread */
    if ( OSLAYER_OK != osThreadClose( &pCamEngineCtx->thread ) )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (closing handler thread failed)\n", __FUNCTION__ );
        UPDATE_RESULT( result, RET_FAILURE );
    }

    /* cancel any commands waiting in command queue */
    do
    {
        /* get next command from queue */
        osStatus = osQueueTryRead( &pCamEngineCtx->commandQueue, &command );

        switch (osStatus)
        {
            case OSLAYER_OK:        /* got a command, so cancel it */
                CamEngineCompleteCommand( pCamEngineCtx, command.cmdId, RET_CANCELED );
                break;
            case OSLAYER_TIMEOUT:   /* queue is empty */
                break;
            default:                /* something is broken... */
                UPDATE_RESULT( result, RET_FAILURE);
                break;
        }
    } while (osStatus == OSLAYER_OK);

    /* destroy command queue */
    if ( OSLAYER_OK != osQueueDestroy( &pCamEngineCtx->commandQueue ) )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (destroying command queue failed)\n", __FUNCTION__ );
        UPDATE_RESULT( result, RET_FAILURE);
    }

    /* release modules */
    lres = CamEngineModulesRelease( pCamEngineCtx );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (releasing modules failed -> RESULT=%d)\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, lres );
    }

    /* release camerIC-Driver */
    lres = CamEngineReleaseCamerIcDrv( pCamEngineCtx );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (releasing CamerIc-Driver failed -> RESULT=%d)\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, lres );
    }

    osMutexDestroy(&pCamEngineCtx->camEngineCtx_lock);

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineInitCamerIc
 *****************************************************************************/
RESULT CamEngineInitCamerIc
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineConfig_t   *pConfig
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    if ( CAM_ENGINE_STATE_INITIALIZED != CamEngineGetState( pCamEngineCtx ) )
    {
        return ( RET_WRONG_STATE );
    }

    if( NULL == pConfig )
    {
        return ( RET_NULL_POINTER );
    }

    if ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc )
    {
        return ( RET_WRONG_HANDLE );
    }
    
    // do we have a second pipe 
    if ( ( ( CAM_ENGINE_MODE_SENSOR_3D         == pCamEngineCtx->mode )   ||
           ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pCamEngineCtx->mode ) ) &&
         ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc ) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* init sub controls */
    result = CamEngineSubCtrlsSetup( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (initialization of sub controls failed)\n", __FUNCTION__ );
        return ( result );
    }

#if 1 // soc camera test
    //zyh@rock-chips.com: v0.0x20.0
    //if(!pCamEngineCtx->isSOCsensor){
        /* configure modules */
        result = CamEngineModulesConfigure( pCamEngineCtx );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s (configuration of modules failed)\n", __FUNCTION__ );
            (void)CamEngineSubCtrlsRelease( pCamEngineCtx );
            return ( result );
        }
    //}
#endif
    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * CamEngineReleaseCamerIc
 *****************************************************************************/
RESULT CamEngineReleaseCamerIc
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    /* release sub controls */
    lres = CamEngineSubCtrlsRelease( pCamEngineCtx );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (releasing of sub controls failed -> RESULT=%d)\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, lres );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * CamEngineInitPixelIf
 *****************************************************************************/
RESULT CamEngineInitPixelIf
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineConfig_t   *pConfig
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    if ( CAM_ENGINE_STATE_INITIALIZED != CamEngineGetState( pCamEngineCtx ) )
    {
        return ( RET_WRONG_STATE );
    }

    if( NULL == pConfig )
    {
        return ( RET_NULL_POINTER );
    }

    if ( NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMipi )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* do we have a second sensor ? */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        if ( NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMipi )
        {
            return ( RET_WRONG_HANDLE );
        }
    }

#ifdef MIPI_USE_CAMERIC
    if ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc )
    {
        return ( RET_WRONG_HANDLE );
    }
    
    /* do we have a second sensor ? */
    if ( ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pCamEngineCtx->mode ) || 
         ( CAM_ENGINE_MODE_SENSOR_3D == pConfig->mode ) )
    {
        if ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc )
        {
            return ( RET_WRONG_HANDLE );
        }
    }
#endif //MIPI_USE_CAMERIC

    CamerIcInterfaceSelect_t ifSelect = CAMERIC_ITF_SELECT_INVALID;
    if ( ( CAM_ENGINE_MODE_SENSOR_2D == pConfig->mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pConfig->mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_3D == pConfig->mode ) )
    {
        ifSelect = pConfig->data.sensor.ifSelect;
    }
    else
    {
        ifSelect = CAMERIC_ITF_SELECT_PARALLEL;
    }

    /* select cameric sensor interface */
    result = CamerIcDriverSetIfSelect( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, ifSelect );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s Can't select input interface (%d)\n", __FUNCTION__, result );
        return ( result );
    }
    
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcDriverSetIfSelect( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, ifSelect );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s Can't select input interface (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    /* init MIPI, SMIA etc driver(s) */
    switch (ifSelect)
    {
        case CAMERIC_ITF_SELECT_PARALLEL:
            break; // nothing to do here

        case CAMERIC_ITF_SELECT_MIPI:
        case CAMERIC_ITF_SELECT_MIPI_2:
        {
            MipiDrvConfig_t MipiDrvConfig;
            MEMSET( &MipiDrvConfig, 0, sizeof( MipiDrvConfig ) );
            MipiDrvConfig.ItfNum           = ( ifSelect == CAMERIC_ITF_SELECT_MIPI ) ? 0 : 1;
            MipiDrvConfig.InstanceNum      = pCamEngineCtx->chainIdx0;
            MipiDrvConfig.HalHandle        = pCamEngineCtx->hHal;
            MipiDrvConfig.MipiDrvHandle    = NULL;
#ifdef MIPI_USE_CAMERIC
            MipiDrvConfig.CamerIcDrvHandle = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
#endif //MIPI_USE_CAMERIC

            result = MipiDrvInit( &MipiDrvConfig ); // must be called after CamerIcDriverInit/CamEngineInitCamerIcDrv as long as MIPI & SMIA modules are part of CamerIc
            if ( (result != RET_SUCCESS) || (MipiDrvConfig.MipiDrvHandle == NULL) )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: Can't initialize MIPI Driver #%d (%d)\n", __FUNCTION__, MipiDrvConfig.InstanceNum, result );
                (void)CamEngineReleasePixelIf( pCamEngineCtx );
                return ( result );
            }
            pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMipi = MipiDrvConfig.MipiDrvHandle;

            /* do we have a second sensor ? */
            if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
            {
                MipiDrvConfig_t MipiDrvConfig;
                MEMSET( &MipiDrvConfig, 0, sizeof( MipiDrvConfig ) );
                MipiDrvConfig.InstanceNum      = pCamEngineCtx->chainIdx1;
                MipiDrvConfig.HalHandle        = pCamEngineCtx->hHal;
                MipiDrvConfig.MipiDrvHandle    = NULL;
#ifdef MIPI_USE_CAMERIC
                MipiDrvConfig.CamerIcDrvHandle = pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc;
#endif //MIPI_USE_CAMERIC

                result = MipiDrvInit( &MipiDrvConfig ); // must be called after CamerIcDriverInit/CamEngineInitCamerIcDrv as long as MIPI & SMIA modules are part of CamerIc
                if ( (result != RET_SUCCESS) || (MipiDrvConfig.MipiDrvHandle == NULL) )
                {
                    TRACE( CAM_ENGINE_ERROR, "%s: Can't initialize MIPI Driver #%d (%d)\n", __FUNCTION__, MipiDrvConfig.InstanceNum, result );
                    (void)CamEngineReleasePixelIf( pCamEngineCtx );
                    return ( result );
                }
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMipi = MipiDrvConfig.MipiDrvHandle;
            }

            break;
        }

        case CAMERIC_ITF_SELECT_SMIA:
            TRACE( CAM_ENGINE_ERROR, "%s: SMIA interface not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );

        default:
            TRACE( CAM_ENGINE_ERROR, "%s: Invalid pixel interface\n", __FUNCTION__ );
            return ( RET_INVALID_PARM );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * CamEngineReleasePixelIf
 *****************************************************************************/
RESULT CamEngineReleasePixelIf
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    /* release MIPI, SMIA etc driver(s) */
    if ( NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMipi )
    {
        lres = MipiDrvRelease( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMipi );
        if ( lres != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s (releasing MIPI driver failed -> RESULT=%d)\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }
        pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMipi = NULL;
    }

    if ( NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMipi )
    {
        lres = MipiDrvRelease( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMipi );
        if ( lres != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s (releasing MIPI driver failed -> RESULT=%d)\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }
        pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMipi = NULL;
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * CamEngineStartPixelIf
 *****************************************************************************/
RESULT CamEngineStartPixelIf
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineConfig_t   *pConfig
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    if( NULL == pConfig )
    {
        return ( RET_NULL_POINTER );
    }

    CamerIcInterfaceSelect_t ifSelect = CAMERIC_ITF_SELECT_INVALID;
    if ( ( CAM_ENGINE_MODE_SENSOR_2D == pConfig->mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pConfig->mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_3D == pConfig->mode ) )
    {
        ifSelect = pConfig->data.sensor.ifSelect;
    }
    else
    {
        ifSelect = CAMERIC_ITF_SELECT_PARALLEL;
    }

    /* start MIPI, SMIA etc driver(s) */
    switch (ifSelect)
    {
        case CAMERIC_ITF_SELECT_PARALLEL:
            break; // nothing to do here

        case CAMERIC_ITF_SELECT_MIPI:
        case CAMERIC_ITF_SELECT_MIPI_2:
        {
            lres = CamEngineStartMipiDrv( pCamEngineCtx, pConfig->data.sensor.mipiMode, pConfig->mipiLaneNum,pConfig->mipiLaneFreq,pConfig->phyAttachedDevId);
            if ( lres != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s (starting MIPI driver failed -> RESULT=%d)\n", __FUNCTION__, lres );
                UPDATE_RESULT( result, lres );
            }

            break;
        }

        case CAMERIC_ITF_SELECT_SMIA:
            TRACE( CAM_ENGINE_ERROR, "%s: SMIA interface not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );

        default:
            TRACE( CAM_ENGINE_ERROR, "%s: Invalid pixel interface\n", __FUNCTION__ );
            return ( RET_INVALID_PARM );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * CamEngineStopPixelIf
 *****************************************************************************/
RESULT CamEngineStopPixelIf
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    /* stop MIPI, SMIA etc driver(s) */
    lres = CamEngineStopMipiDrv( pCamEngineCtx );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (stopping MIPI driver failed -> RESULT=%d)\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, lres );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}



/******************************************************************************
 * CamEngineInitDrvForSensor
 *****************************************************************************/
RESULT CamEngineInitDrvForSensor
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineConfig_t   *pConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );
    DCT_ASSERT( ( CAM_ENGINE_MODE_SENSOR_2D == pConfig->mode ) ||
                ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pConfig->mode ) ||
                ( CAM_ENGINE_MODE_SENSOR_3D == pConfig->mode ) );

    if ( CAM_ENGINE_STATE_INITIALIZED != CamEngineGetState( pCamEngineCtx ) )
    {
        return ( RET_WRONG_STATE );
    }

    if( NULL == pConfig )
    {
        return ( RET_NULL_POINTER );
    }

	if(!pCamEngineCtx->isSOCsensor){
	/* ISP drv setup */
		if(pCamEngineCtx->color_mode == COLOR_SENSOR){
			result = CamEngineSetupCamerIcDrv( pCamEngineCtx, CAMERIC_ISP_DEMOSAIC_NORMAL_OPERATION, 4, BOOL_TRUE, BOOL_TRUE, BOOL_TRUE );
		}else {
			result = CamEngineSetupCamerIcDrv( pCamEngineCtx, CAMERIC_ISP_DEMOSAIC_BYPASS, 4, BOOL_TRUE, BOOL_TRUE, BOOL_TRUE );
		}
	}else{
	//	soc camera test
	   result = CamEngineSetupCamerIcDrv( pCamEngineCtx, CAMERIC_ISP_DEMOSAIC_BYPASS, 0, BOOL_FALSE, BOOL_FALSE, BOOL_FALSE );
	}

    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure isp %d)\n", __FUNCTION__, result );
        return ( result );
    }
    /* zyc@rock-chips.com: v0.0x21.0 */
    if(!pCamEngineCtx->isSOCsensor){
    
        /* ISP drv module setup */
        result = CamEngineSetupHistogramDrv( pCamEngineCtx, BOOL_TRUE, CAMERIC_ISP_HIST_MODE_RGB_COMBINED );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: (can't configure hist %d)\n", __FUNCTION__, result );
            return ( result );
        }
    
        result = CamEngineSetupExpDrv( pCamEngineCtx, BOOL_TRUE, CAMERIC_ISP_EXP_MEASURING_MODE_1 );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: (can't configure exp %d)\n", __FUNCTION__, result );
            (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
            return ( result );
        }
        result = CamEngineSetupAwbDrv( pCamEngineCtx, BOOL_TRUE );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: (can't configure awb %d)\n", __FUNCTION__, result );
            (void)CamEngineReleaseExpDrv( pCamEngineCtx );
            (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
            return ( result );
        }

        result = CamEngineSetupAfmDrv( pCamEngineCtx, BOOL_TRUE );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: (can't configure af %d)\n", __FUNCTION__, result );
            (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
            (void)CamEngineReleaseExpDrv( pCamEngineCtx );
            (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
            return ( result );
        }

        result = CamEngineSetupVsmDrv( pCamEngineCtx, BOOL_TRUE );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: (can't configure vsm %d)\n", __FUNCTION__, result );
            (void)CamEngineReleaseAfmDrv( pCamEngineCtx );
            (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
            (void)CamEngineReleaseExpDrv( pCamEngineCtx );
            (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
            return ( result );
        }

        result = CamEngineSetupBlsDrv( pCamEngineCtx, BOOL_TRUE );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: (can't configure bls %d)\n", __FUNCTION__, result );
            (void)CamEngineReleaseVsmDrv( pCamEngineCtx );
            (void)CamEngineReleaseAfmDrv( pCamEngineCtx );
            (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
            (void)CamEngineReleaseExpDrv( pCamEngineCtx );
            (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
            return ( result );
        }

        result = CamEngineSetupCacDrv( pCamEngineCtx, BOOL_FALSE);
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: (can't configure cac %d)\n", __FUNCTION__, result );
            (void)CamEngineReleaseVsmDrv( pCamEngineCtx );
            (void)CamEngineReleaseAfmDrv( pCamEngineCtx );
            (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
            (void)CamEngineReleaseExpDrv( pCamEngineCtx );
            (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
            return ( result );
        }

        result = CamEngineSetupLscDrv( pCamEngineCtx, BOOL_TRUE );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: (can't configure lsc %d)\n", __FUNCTION__, result );
            (void)CamEngineReleaseVsmDrv( pCamEngineCtx );
            (void)CamEngineReleaseAfmDrv( pCamEngineCtx );
            (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
            (void)CamEngineReleaseExpDrv( pCamEngineCtx );
            (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
            return ( result );
        }

        result = CamEngineSetupFltDrv( pCamEngineCtx, BOOL_TRUE );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: (can't configure flt %d)\n", __FUNCTION__, result );
            (void)CamEngineReleaseVsmDrv( pCamEngineCtx );
            (void)CamEngineReleaseAfmDrv( pCamEngineCtx );
            (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
            (void)CamEngineReleaseExpDrv( pCamEngineCtx );
            (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
            return ( result );
        }
    }

#if 0 //don't start here
    /* pixel interface */
    result = CamEngineStartPixelIf( pCamEngineCtx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't start pixel interface %d)\n", __FUNCTION__, result );
        (void)CamEngineReleaseVsmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAfmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
        (void)CamEngineReleaseExpDrv( pCamEngineCtx );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }
#endif
#if defined(MRV_JPE_VERSION) //zyc ,for test
    /* jpeg-encoder */
    result = CamEngineSetupJpeDrv( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (can't configure jpeg-encoder %d)\n", __FUNCTION__ );
        (void)CamEngineStopPixelIf( pCamEngineCtx );
        (void)CamEngineReleaseVsmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAfmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
        (void)CamEngineReleaseExpDrv( pCamEngineCtx );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }
#endif
    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * CamEngineInitDrvForTestpattern
 *****************************************************************************/
RESULT CamEngineInitDrvForTestpattern
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineConfig_t   *pConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );
    DCT_ASSERT( ( CAM_ENGINE_MODE_SENSOR_2D == pConfig->mode ) ||
                ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pConfig->mode ) ||
                ( CAM_ENGINE_MODE_SENSOR_3D == pConfig->mode ) );

    if ( CAM_ENGINE_STATE_INITIALIZED != CamEngineGetState( pCamEngineCtx ) )
    {
        return ( RET_WRONG_STATE );
    }

    if( NULL == pConfig )
    {
        return ( RET_NULL_POINTER );
    }

    CamerIc3x3Matrix_t ccMatrix = {
        {
            UtlFloatToFix_S0407(1.0f), UtlFloatToFix_S0407(0.0f), UtlFloatToFix_S0407(0.0f),
            UtlFloatToFix_S0407(0.0f), UtlFloatToFix_S0407(1.0f), UtlFloatToFix_S0407(0.0f),
            UtlFloatToFix_S0407(0.0f), UtlFloatToFix_S0407(0.0f), UtlFloatToFix_S0407(1.0f)
        }
    };

    CamerIcXTalkOffset_t ccOffset = { 0, 0, 0 };

    CamerIcGains_t wbGains = { UtlFloatToFix_U0208( 1.0f ),
                               UtlFloatToFix_U0208( 1.0f ),
                               UtlFloatToFix_U0208( 1.0f ),
                               UtlFloatToFix_U0208( 1.0f ) };

    result = CamerIcIspSetCrossTalkCoefficients( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &ccMatrix );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't configure color conversion (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcIspSetCrossTalkOffset( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &ccOffset );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't configure color conversion offsets (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcIspAwbSetGains( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &wbGains );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't configure gains (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspSetCrossTalkCoefficients( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, &ccMatrix );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't configure color conversion (%d)\n", __FUNCTION__, result );
            return ( result );
        }

        result = CamerIcIspSetCrossTalkOffset( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, &ccOffset );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't configure color conversion offsets (%d)\n", __FUNCTION__, result );
            return ( result );
        }

        result = CamerIcIspAwbSetGains( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, &wbGains );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't configure gains (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    /* ISP drv setup */
	if(pCamEngineCtx->color_mode == COLOR_SENSOR){
		result = CamEngineSetupCamerIcDrv( pCamEngineCtx, CAMERIC_ISP_DEMOSAIC_NORMAL_OPERATION, 4, BOOL_TRUE, BOOL_TRUE, BOOL_TRUE );
	}else {
		result = CamEngineSetupCamerIcDrv( pCamEngineCtx, CAMERIC_ISP_DEMOSAIC_BYPASS, 4, BOOL_TRUE, BOOL_TRUE, BOOL_TRUE );
	}

    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure isp %d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* ISP drv module setup */
    result = CamEngineSetupHistogramDrv( pCamEngineCtx, BOOL_FALSE, CAMERIC_ISP_HIST_MODE_RGB_COMBINED );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure hist %d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamEngineSetupExpDrv( pCamEngineCtx, BOOL_FALSE, CAMERIC_ISP_EXP_MEASURING_MODE_1 );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure exp %d)\n", __FUNCTION__, result );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }

    result = CamEngineSetupAwbDrv( pCamEngineCtx, BOOL_FALSE );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure awb %d)\n", __FUNCTION__, result );
        (void)CamEngineReleaseExpDrv( pCamEngineCtx );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }

    result = CamEngineSetupAfmDrv( pCamEngineCtx, BOOL_FALSE );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure af %d)\n", __FUNCTION__, result );
        (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
        (void)CamEngineReleaseExpDrv( pCamEngineCtx );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }

    result = CamEngineSetupVsmDrv( pCamEngineCtx, BOOL_FALSE );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure vsm %d)\n", __FUNCTION__, result );
        (void)CamEngineReleaseAfmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
        (void)CamEngineReleaseExpDrv( pCamEngineCtx );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }

    result = CamEngineSetupBlsDrv( pCamEngineCtx, BOOL_TRUE );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure bls %d)\n", __FUNCTION__, result );
        (void)CamEngineReleaseVsmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAfmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
        (void)CamEngineReleaseExpDrv( pCamEngineCtx );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }

    result = CamEngineSetupCacDrv( pCamEngineCtx, BOOL_FALSE );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure cac %d)\n", __FUNCTION__, result );
        (void)CamEngineReleaseVsmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAfmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
        (void)CamEngineReleaseExpDrv( pCamEngineCtx );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }

    result = CamEngineSetupLscDrv( pCamEngineCtx, BOOL_FALSE );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure lsc %d)\n", __FUNCTION__, result );
        (void)CamEngineReleaseVsmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAfmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
        (void)CamEngineReleaseExpDrv( pCamEngineCtx );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }

    result = CamEngineSetupFltDrv( pCamEngineCtx, BOOL_FALSE );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure flt %d)\n", __FUNCTION__, result );
        (void)CamEngineReleaseVsmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAfmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
        (void)CamEngineReleaseExpDrv( pCamEngineCtx );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }

    /* pixel interface */
    result = CamEngineStartPixelIf( pCamEngineCtx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't start pixel interface %d)\n", __FUNCTION__, result );
        (void)CamEngineReleaseVsmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAfmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
        (void)CamEngineReleaseExpDrv( pCamEngineCtx );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }
#if defined(MRV_JPE_VERSION) //zyc ,for test

    /* jpeg-encoder */
    result = CamEngineSetupJpeDrv( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (can't configure jpeg-encoder %d)\n", __FUNCTION__ );
        (void)CamEngineStopPixelIf( pCamEngineCtx );
        (void)CamEngineReleaseVsmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAfmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
        (void)CamEngineReleaseExpDrv( pCamEngineCtx );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }
#endif
    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * CamEngineInitDrvForDma
 *****************************************************************************/
RESULT CamEngineInitDrvForDma
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineConfig_t   *pConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );
    DCT_ASSERT( CAM_ENGINE_MODE_IMAGE_PROCESSING  == pConfig->mode );

    if ( CAM_ENGINE_STATE_INITIALIZED != CamEngineGetState( pCamEngineCtx ) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( NULL == pConfig )
    {
        return ( RET_NULL_POINTER );
    }

    if ( ( NULL == pConfig->data.image.pWbGains  ) ||
         ( NULL == pConfig->data.image.pCcMatrix ) ||
         ( NULL == pConfig->data.image.pCcOffset ) ||
         ( NULL == pConfig->data.image.pBlvl     ) )
    {
        return ( RET_NULL_POINTER );
    }

    CamerIc3x3Matrix_t ccMatrix = { { UtlFloatToFix_S0407(pConfig->data.image.pCcMatrix->Coeff[0]),
                                      UtlFloatToFix_S0407(pConfig->data.image.pCcMatrix->Coeff[1]),
                                      UtlFloatToFix_S0407(pConfig->data.image.pCcMatrix->Coeff[2]),
                                      UtlFloatToFix_S0407(pConfig->data.image.pCcMatrix->Coeff[3]),
                                      UtlFloatToFix_S0407(pConfig->data.image.pCcMatrix->Coeff[4]),
                                      UtlFloatToFix_S0407(pConfig->data.image.pCcMatrix->Coeff[5]),
                                      UtlFloatToFix_S0407(pConfig->data.image.pCcMatrix->Coeff[6]),
                                      UtlFloatToFix_S0407(pConfig->data.image.pCcMatrix->Coeff[7]),
                                      UtlFloatToFix_S0407(pConfig->data.image.pCcMatrix->Coeff[8]) } };

    CamerIcXTalkOffset_t ccOffset = { pConfig->data.image.pCcOffset->Red,
                                      pConfig->data.image.pCcOffset->Green,
                                      pConfig->data.image.pCcOffset->Blue };

    CamerIcGains_t wbGains = { UtlFloatToFix_U0208( pConfig->data.image.pWbGains->Red ),
                               UtlFloatToFix_U0208( pConfig->data.image.pWbGains->GreenR ),
                               UtlFloatToFix_U0208( pConfig->data.image.pWbGains->GreenB ),
                               UtlFloatToFix_U0208( pConfig->data.image.pWbGains->Blue ) };

    uint16_t fixedA = 0U;
    uint16_t fixedB = 0U;
    uint16_t fixedC = 0U;
    uint16_t fixedD = 0U;

    result = CamerIcIspSetCrossTalkCoefficients( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &ccMatrix );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't configure color conversion (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcIspSetCrossTalkOffset( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &ccOffset );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't configure color conversion offsets (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcIspAwbSetGains( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &wbGains );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't configure gains (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    switch ( pConfig->data.image.layout ) 
    {
        case PIC_BUF_LAYOUT_BAYER_RGRGGBGB:
            {
                fixedA = pConfig->data.image.pBlvl->Red;
                fixedB = pConfig->data.image.pBlvl->GreenR;
                fixedC = pConfig->data.image.pBlvl->GreenB;
                fixedD = pConfig->data.image.pBlvl->Blue;
                break;
            }

        case PIC_BUF_LAYOUT_BAYER_GRGRBGBG:
            {
                fixedA = pConfig->data.image.pBlvl->GreenR;
                fixedB = pConfig->data.image.pBlvl->Red;
                fixedC = pConfig->data.image.pBlvl->Blue;
                fixedD = pConfig->data.image.pBlvl->GreenB;
                break;
            }

        case PIC_BUF_LAYOUT_BAYER_GBGBRGRG:
            {
                fixedA = pConfig->data.image.pBlvl->GreenB;
                fixedB = pConfig->data.image.pBlvl->Blue;
                fixedC = pConfig->data.image.pBlvl->Red;
                fixedD = pConfig->data.image.pBlvl->GreenR;

                break;
            }

        case PIC_BUF_LAYOUT_BAYER_BGBGGRGR:
            {
                fixedA = pConfig->data.image.pBlvl->Blue;
                fixedB = pConfig->data.image.pBlvl->GreenB;
                fixedC = pConfig->data.image.pBlvl->GreenR;
                fixedD = pConfig->data.image.pBlvl->Red;
                break;
            }

        default:
            break;
    }

    result = CamerIcIspBlsSetBlackLevel( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, fixedA, fixedB, fixedC, fixedD );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver BLS set Black Level Substraction failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    // ISP drv setup
    if(pCamEngineCtx->color_mode == COLOR_SENSOR){
		result = CamEngineSetupCamerIcDrv( pCamEngineCtx, CAMERIC_ISP_DEMOSAIC_NORMAL_OPERATION, 4, BOOL_TRUE, BOOL_TRUE, BOOL_TRUE );
	}else {
		result = CamEngineSetupCamerIcDrv( pCamEngineCtx, CAMERIC_ISP_DEMOSAIC_BYPASS, 4, BOOL_TRUE, BOOL_TRUE, BOOL_TRUE );
	}
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure isp %d)\n", __FUNCTION__, result );
        return ( result );
    }

    // ISP drv module setup
    result = CamEngineSetupHistogramDrv( pCamEngineCtx, BOOL_TRUE, CAMERIC_ISP_HIST_MODE_RGB_COMBINED );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure hist %d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamEngineSetupExpDrv( pCamEngineCtx, BOOL_FALSE, CAMERIC_ISP_EXP_MEASURING_MODE_1 );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure exp %d)\n", __FUNCTION__, result );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }

    result = CamEngineSetupAwbDrv( pCamEngineCtx, BOOL_TRUE );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure awb %d)\n", __FUNCTION__, result );
        (void)CamEngineReleaseExpDrv( pCamEngineCtx );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }

    result = CamEngineSetupAfmDrv( pCamEngineCtx, BOOL_FALSE );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure af %d)\n", __FUNCTION__, result );
        (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
        (void)CamEngineReleaseExpDrv( pCamEngineCtx );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }

    result = CamEngineSetupVsmDrv( pCamEngineCtx, BOOL_FALSE );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure vsm %d)\n", __FUNCTION__, result );
        (void)CamEngineReleaseAfmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
        (void)CamEngineReleaseExpDrv( pCamEngineCtx );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }

    // try to enable bls-module if calibration database available
    result = CamEngineSetupBlsDrv( pCamEngineCtx, 
                                    (pConfig->hCamCalibDb || pConfig->data.image.pBlvl) ? BOOL_TRUE : BOOL_FALSE );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure bls %d)\n", __FUNCTION__, result );
        (void)CamEngineReleaseVsmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAfmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
        (void)CamEngineReleaseExpDrv( pCamEngineCtx );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }

    // try to enable cac-module if calibration database available
    result = CamEngineSetupCacDrv( pCamEngineCtx,
                                        (pConfig->hCamCalibDb) ? BOOL_TRUE : BOOL_FALSE );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure cac %d)\n", __FUNCTION__, result );
        (void)CamEngineReleaseVsmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAfmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
        (void)CamEngineReleaseExpDrv( pCamEngineCtx );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }

    result = CamEngineSetupLscDrv( pCamEngineCtx,
                                        (pConfig->hCamCalibDb) ? BOOL_TRUE : BOOL_FALSE );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure lsc %d)\n", __FUNCTION__, result );
        (void)CamEngineReleaseVsmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAfmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
        (void)CamEngineReleaseExpDrv( pCamEngineCtx );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }

    result = CamEngineSetupFltDrv( pCamEngineCtx, BOOL_FALSE );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't configure flt %d)\n", __FUNCTION__, result );
        (void)CamEngineReleaseVsmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAfmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
        (void)CamEngineReleaseExpDrv( pCamEngineCtx );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }

    /* jpeg-encoder */
#if defined(MRV_JPE_VERSION) //zyc ,for test
    result = CamEngineSetupJpeDrv( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (can't configure jpeg-encoder %d)\n", __FUNCTION__ );
        (void)CamEngineReleaseVsmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAfmDrv( pCamEngineCtx );
        (void)CamEngineReleaseAwbDrv( pCamEngineCtx );
        (void)CamEngineReleaseExpDrv( pCamEngineCtx );
        (void)CamEngineReleaseHistogramDrv( pCamEngineCtx );
        return ( result );
    }
#endif
    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * CamEngineReInitDrv
 *****************************************************************************/
RESULT CamEngineReInitDrv
(
    CamEngineContext_t  *pCamEngineCtx,
    uint32_t            numFramesToSkip
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    if ( CAM_ENGINE_STATE_RUNNING != CamEngineGetState( pCamEngineCtx ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* ISP drv module setup */
    bool_t isEnabled = BOOL_FALSE;
    result = CamerIcIspBlsIsEnabled( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &isEnabled );
    if ( RET_SUCCESS == result )
    {
        result = CamEngineSetupBlsDrv( pCamEngineCtx, isEnabled );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: (can't configure bls %d)\n", __FUNCTION__, result );
            return ( result );
        }
    }
    else
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't get status of bls (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    isEnabled = BOOL_FALSE;
    result = CamerIcIspCacIsEnabled( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &isEnabled );
    if ( RET_SUCCESS == result )
    {
        result = CamEngineSetupCacDrv( pCamEngineCtx, isEnabled );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: (can't configure cac %d)\n", __FUNCTION__, result );
            return ( result );
        }
    }
    else
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't get status of cac (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcMiSetFramesToSkip( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, numFramesToSkip );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't set frames to skip (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamEngineCamerIcDrvMeasureCbRestart( pCamEngineCtx, numFramesToSkip );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't reset measurement states (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * CamEngineReleaseDrv
 *****************************************************************************/
RESULT CamEngineReleaseDrv
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    lres = CamEngineStopPixelIf( pCamEngineCtx );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't stop pixel interface %d)\n", __FUNCTION__, result );
        UPDATE_RESULT( result, lres );
    }

    lres = CamEngineReleaseAfmDrv( pCamEngineCtx );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't release awb %d)\n", __FUNCTION__, result );
        UPDATE_RESULT( result, lres );
    }

    lres = CamEngineReleaseAwbDrv( pCamEngineCtx );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't release awb %d)\n", __FUNCTION__, result );
        UPDATE_RESULT( result, lres );
    }

    lres = CamEngineReleaseVsmDrv( pCamEngineCtx );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't release vsm %d)\n", __FUNCTION__, result );
        UPDATE_RESULT( result, lres );
    }

    lres = CamEngineReleaseExpDrv( pCamEngineCtx );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't release exp %d)\n", __FUNCTION__, result );
        UPDATE_RESULT( result, lres );
    }

    lres = CamEngineReleaseHistogramDrv( pCamEngineCtx );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (can't release hist %d)\n", __FUNCTION__, result );
        UPDATE_RESULT( result, lres );
    }

#if defined(MRV_JPE_VERSION) //zyc ,for test
    /* jpeg-encoder */
    lres = CamEngineReleaseJpeDrv( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (can't release jpeg-encoder %d)\n", __FUNCTION__ );
        UPDATE_RESULT( result, lres );
    }
#endif
    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}



/******************************************************************************
 * CamEnginePrepareCalibDbAccess
 *****************************************************************************/
RESULT CamEnginePrepareCalibDbAccess
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    uint16_t width      = 0;
    uint16_t height     = 0;
    uint16_t framerate  = 0;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );
    DCT_ASSERT( pCamEngineCtx->hCamCalibDb != NULL );

    width   = pCamEngineCtx->acqWindow.width;
    height  = pCamEngineCtx->acqWindow.height;

    DCT_ASSERT( width != 0 );
    DCT_ASSERT( height != 0 );

    result = CamCalibDbGetResolutionNameByWidthHeight(
                    pCamEngineCtx->hCamCalibDb, width, height, &pCamEngineCtx->ResName );

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * CamEngineSetupAcqForSensor
 *****************************************************************************/
RESULT CamEngineSetupAcqForSensor
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineConfig_t   *pConfig,
    CamEngineChainIdx_t idx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pConfig != NULL );
    DCT_ASSERT( pCamEngineCtx != NULL );
    DCT_ASSERT( idx < CHAIN_MAX );
    DCT_ASSERT( pCamEngineCtx->chain[idx].hCamerIc != NULL );
    DCT_ASSERT( pCamEngineCtx->chain[idx].hSensor != NULL );
    DCT_ASSERT( ( CAM_ENGINE_MODE_SENSOR_2D == pConfig->mode ) ||
                ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pConfig->mode ) ||
                ( CAM_ENGINE_MODE_SENSOR_3D == pConfig->mode ) );

    if ( CAM_ENGINE_STATE_INITIALIZED != CamEngineGetState( pCamEngineCtx ) )
    {
        return ( RET_WRONG_STATE );
    }

    if( NULL == pConfig )
    {
        return ( RET_NULL_POINTER );
    }

    /* setup input acquisition */

    /* set mode, raw or depending on sensor */
    pCamEngineCtx->chain[idx].inMode = pConfig->data.sensor.mode;

    /* set acquisition properties depending on sensor */
    result = CamerIcIspSetAcqProperties( pCamEngineCtx->chain[idx].hCamerIc,
                                            pConfig->data.sensor.sampleEdge,
                                            pConfig->data.sensor.hSyncPol,
                                            pConfig->data.sensor.vSyncPol,
                                            pConfig->data.sensor.bayerPattern,
                                            pConfig->data.sensor.subSampling,
                                            pConfig->data.sensor.seqCCIR,
                                            pConfig->data.sensor.fieldSelection,
                                            pConfig->data.sensor.inputSelection,
                                            CAMERIC_ISP_LATENCY_FIFO_INPUT_FORMATTER );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Configuring input-acquisition failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* set acquisition resolution */
    result = CamEngineSetupAcqResolution( pCamEngineCtx, idx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Configuring acquisition resolution failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * CamEngineSetupAcqForDma
 *****************************************************************************/
RESULT CamEngineSetupAcqForDma
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineConfig_t   *pConfig,
    CamEngineChainIdx_t idx
)
{
    CamerIcIspBayerPattern_t bayerPattern = CAMERIC_ISP_BAYER_PATTERN_RGRGGBGB;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );
    DCT_ASSERT( idx < CHAIN_MAX );
    DCT_ASSERT( pCamEngineCtx->chain[idx].hCamerIc != NULL );
    DCT_ASSERT( CAM_ENGINE_MODE_IMAGE_PROCESSING  == pConfig->mode );

    if ( CAM_ENGINE_STATE_INITIALIZED != CamEngineGetState( pCamEngineCtx ) )
    {
        return ( RET_WRONG_STATE );
    }

    if( NULL == pConfig )
    {
        return ( RET_NULL_POINTER );
    }

    switch ( pConfig->data.image.layout )
    {
        case PIC_BUF_LAYOUT_BAYER_RGRGGBGB:
        {
            bayerPattern = CAMERIC_ISP_BAYER_PATTERN_RGRGGBGB;
            break;
        }

        case PIC_BUF_LAYOUT_BAYER_GRGRBGBG:
        {
            bayerPattern = CAMERIC_ISP_BAYER_PATTERN_GRGRBGBG;
            break;
        }

        case PIC_BUF_LAYOUT_BAYER_GBGBRGRG:
        {
            bayerPattern = CAMERIC_ISP_BAYER_PATTERN_GBGBRGRG;
            break;
        }

        case PIC_BUF_LAYOUT_BAYER_BGBGGRGR:
        {
            bayerPattern = CAMERIC_ISP_BAYER_PATTERN_BGBGGRGR;
            break;
        }

        default:
        {
            return ( RET_INVALID_PARM );
        }
    }
    
    /* set mode, raw or Bayer RGB */
    pCamEngineCtx->chain[idx].inMode = CAMERIC_ISP_MODE_BAYER_RGB;

    /* setup input acquisition */

    /* set acquisition properties */
    result = CamerIcIspSetAcqProperties( pCamEngineCtx->chain[idx].hCamerIc,
                                            CAMERIC_ISP_SAMPLE_EDGE_RISING,
                                            CAMERIC_ISP_POLARITY_HIGH,
                                            CAMERIC_ISP_POLARITY_LOW,
                                            bayerPattern,
                                            CAMERIC_ISP_CONV422_NONCOSITED,
                                            CAMERIC_ISP_CCIR_SEQUENCE_YCbYCr,
                                            CAMERIC_ISP_FIELD_SELECTION_BOTH,
                                            CAMERIC_ISP_INPUT_10BIT_EX,
                                            CAMERIC_ISP_LATENCY_FIFO_DMA_READ_RAW );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Configuring input-acquisition failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* set acquisition resolution */
    result = CamEngineSetupAcqResolution( pCamEngineCtx, idx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Configuring acquisition resolution failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * CamEngineSetupAcqForImgStab
 *****************************************************************************/
RESULT CamEngineSetupAcqForImgStab
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineConfig_t   *pConfig,
    CamEngineChainIdx_t idx
)
{
    CamerIcIspBayerPattern_t bayerPattern = CAMERIC_ISP_BAYER_PATTERN_RGRGGBGB;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );
    DCT_ASSERT( idx < CHAIN_MAX );
    DCT_ASSERT( pCamEngineCtx->chain[idx].hCamerIc != NULL );
    DCT_ASSERT( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pConfig->mode );

    if ( CAM_ENGINE_STATE_INITIALIZED != CamEngineGetState( pCamEngineCtx ) )
    {
        return ( RET_WRONG_STATE );
    }

    if( NULL == pConfig )
    {
        return ( RET_NULL_POINTER );
    }
    
    /* set mode, raw or Bayer RGB */
    pCamEngineCtx->chain[idx].inMode = CAMERIC_ISP_MODE_601;

    /* setup input acquisition */

    /* set acquisition properties */
    result = CamerIcIspSetAcqProperties( pCamEngineCtx->chain[idx].hCamerIc,
                                            CAMERIC_ISP_SAMPLE_EDGE_RISING,
                                            CAMERIC_ISP_POLARITY_HIGH,
                                            CAMERIC_ISP_POLARITY_LOW,
                                            CAMERIC_ISP_BAYER_PATTERN_RGRGGBGB,
                                            CAMERIC_ISP_CONV422_NONCOSITED,
                                            CAMERIC_ISP_CCIR_SEQUENCE_YCbYCr,
                                            CAMERIC_ISP_FIELD_SELECTION_BOTH,
                                            CAMERIC_ISP_INPUT_10BIT_EX,
                                            CAMERIC_ISP_LATENCY_FIFO_DMA_READ_YUV );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Configuring input-acquisition failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* set acquisition resolution */
    result = CamEngineSetupAcqResolution( pCamEngineCtx, idx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Configuring acquisition resolution failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * CamEngineSetupAcqResolution
 *****************************************************************************/
RESULT CamEngineSetupAcqResolution
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineChainIdx_t idx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );
    DCT_ASSERT( idx < CHAIN_MAX );
    DCT_ASSERT( pCamEngineCtx->chain[idx].hCamerIc != NULL );

    if ( ( CAM_ENGINE_STATE_INITIALIZED != pCamEngineCtx->state )
      && ( CAM_ENGINE_STATE_RUNNING    != pCamEngineCtx->state) )
    {
        return ( RET_WRONG_STATE );
    }

    /* set acquisition resolution to match sensor resolution */
    result = CamerIcIspSetAcqResolution( pCamEngineCtx->chain[idx].hCamerIc,
            pCamEngineCtx->acqWindow.hOffset,
            pCamEngineCtx->acqWindow.vOffset,
            pCamEngineCtx->acqWindow.width,
            pCamEngineCtx->acqWindow.height );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Resolution not supported by input-acquisition (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* can be smaller than acquisition window, without black pixel areas */
    result = CamerIcIspSetOutputFormatterResolution( pCamEngineCtx->chain[idx].hCamerIc,
            pCamEngineCtx->outWindow.hOffset,
            pCamEngineCtx->outWindow.vOffset,
            pCamEngineCtx->outWindow.width,
            pCamEngineCtx->outWindow.height );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Resolution not supported by output-formatter (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* can be smaller than acquisition window, without black pixel areas */
    result = CamerIcIspCnrSetLineWidth( pCamEngineCtx->chain[idx].hCamerIc,
            pCamEngineCtx->outWindow.width );
    if ( (result != RET_SUCCESS) && (result != RET_NOTSUPP) )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Resolution not supported by color noise reduction (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* image stabilisation resolution */
    result = CamerIcIspSetImageStabilizationResolution( pCamEngineCtx->chain[idx].hCamerIc,
            pCamEngineCtx->chain[idx].isWindow.hOffset,
            pCamEngineCtx->chain[idx].isWindow.vOffset,
            pCamEngineCtx->chain[idx].isWindow.width,
            pCamEngineCtx->chain[idx].isWindow.height );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Resolution not supported by image stabilization (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * CamEngineSetupMiDataPath
 *****************************************************************************/
RESULT CamEngineSetupMiDataPath
(
    CamEngineContext_t              *pCamEngineCtx,
    const CamEnginePathConfig_t     *pConfigMain,
    const CamEnginePathConfig_t     *pConfigSelf,
    CamEngineChainIdx_t             idx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );
    DCT_ASSERT( idx < CHAIN_MAX );
    DCT_ASSERT( pCamEngineCtx->chain[idx].hCamerIc != NULL );

    if ( ( CAM_ENGINE_STATE_INITIALIZED != pCamEngineCtx->state )
      && ( CAM_ENGINE_STATE_RUNNING    != pCamEngineCtx->state) )
    {
        return ( RET_WRONG_STATE );
    }

    if( ( NULL == pConfigMain ) || ( NULL == pConfigSelf ) )
    {
        return ( RET_NULL_POINTER );
    }

    /* check configuration */
    bool_t ok = BOOL_FALSE;
    if ( (pConfigMain->mode == CAMERIC_MI_DATAMODE_DISABLED)
            && ( (pConfigSelf->mode == CAMERIC_MI_DATAMODE_DISABLED)
                    || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_YUV444)
                    || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_YUV422)
                    || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_YUV420)
                    || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_YUV400)
                    || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_RGB888)
                    || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_RGB666)
                    || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_RGB565)
                    || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_DPCC) ) )
    {
        /* valid setting */
        ok = BOOL_TRUE;
    }
    else if ( ((pConfigMain->mode == CAMERIC_MI_DATAMODE_YUV422)
                        || (pConfigMain->mode == CAMERIC_MI_DATAMODE_YUV420))
                && ( (pConfigSelf->mode == CAMERIC_MI_DATAMODE_DISABLED)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_YUV444)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_YUV422)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_YUV420)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_YUV400)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_RGB888)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_RGB666)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_RGB565)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_DPCC) ) )
    {
        /* valid setting */
        ok = BOOL_TRUE;
    }
    else if ( (pConfigMain->mode == CAMERIC_MI_DATAMODE_JPEG)
                && ( (pConfigSelf->mode == CAMERIC_MI_DATAMODE_DISABLED)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_YUV444)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_YUV422)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_YUV420)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_YUV400)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_RGB888)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_RGB666)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_RGB565)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_DPCC) ) )
    {
        /* valid setting */
        ok = BOOL_TRUE;
    }
    else if ( (pConfigMain->mode == CAMERIC_MI_DATAMODE_DPCC)
                && ( (pConfigSelf->mode == CAMERIC_MI_DATAMODE_DISABLED)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_YUV444)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_YUV422)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_YUV420)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_YUV400)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_RGB888)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_RGB666)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_RGB565) ) )
    {
        /* valid setting */
        ok = BOOL_TRUE;
    }
    else if ( ((pConfigMain->mode == CAMERIC_MI_DATAMODE_RAW8)
                        || (pConfigMain->mode == CAMERIC_MI_DATAMODE_RAW12))
                && ( (pConfigSelf->mode == CAMERIC_MI_DATAMODE_DISABLED)
                        || (pConfigSelf->mode == CAMERIC_MI_DATAMODE_DPCC) ) )
    {
        /* valid setting */
        ok = BOOL_TRUE;
    }
    if ( BOOL_TRUE != ok )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Unsupported path mode (%d)\n", __FUNCTION__, result );
        return ( RET_INVALID_PARM );
    }

    /* set mode, raw or Bayer RGB */
    if ( ( pConfigMain->mode == CAMERIC_MI_DATAMODE_RAW8  ) ||
         ( pConfigMain->mode == CAMERIC_MI_DATAMODE_RAW12 )  )
    {
        result = CamerIcIspSetMode( pCamEngineCtx->chain[idx].hCamerIc, CAMERIC_ISP_MODE_RAW );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: ISP-Mode not supported (%d)\n", __FUNCTION__, result);
            return ( result );
        }
    }
    else
    {
        result = CamerIcIspSetMode( pCamEngineCtx->chain[idx].hCamerIc, pCamEngineCtx->chain[idx].inMode );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: ISP-Mode not supported (%d)\n", __FUNCTION__, result);
            return ( result );
        }
    }

    /* configure CamerIc MI Module */
    //set burst to 16,reduce the probability of data loss caused by ddr bandwidth, by zyc
    /* ddl@rock-chips.com: v1.2.0 */
    if ( (pConfigMain->mode == CAMERIC_MI_DATAMODE_YUV422)
        || (pConfigMain->mode == CAMERIC_MI_DATAMODE_YUV420)
        || (pConfigMain->mode == CAMERIC_MI_DATAMODE_YUV422) ) {

        if ((pConfigMain->width*pConfigMain->height&0x7f)==0x40) {    
            result = CamerIcMiSetBurstLength( pCamEngineCtx->chain[idx].hCamerIc, CAMERIC_MI_BURSTLENGTH_16, CAMERIC_MI_BURSTLENGTH_8 );
        } else {
            result = CamerIcMiSetBurstLength( pCamEngineCtx->chain[idx].hCamerIc, CAMERIC_MI_BURSTLENGTH_16, CAMERIC_MI_BURSTLENGTH_16 );
        }
    } else {
        result = CamerIcMiSetBurstLength( pCamEngineCtx->chain[idx].hCamerIc, CAMERIC_MI_BURSTLENGTH_16, CAMERIC_MI_BURSTLENGTH_16 );
    }

    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't configure burstlength on CamerIc MI module (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* resolution and path cropping window */
    pCamEngineCtx->dcEnable[CAMERIC_MI_PATH_MAIN]   = pConfigMain->dcEnable;
    pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_MAIN]   = pConfigMain->dcWin;
    pCamEngineCtx->outWidth[CAMERIC_MI_PATH_MAIN]   = pConfigMain->width;
    pCamEngineCtx->outHeight[CAMERIC_MI_PATH_MAIN]  = pConfigMain->height;

    pCamEngineCtx->dcEnable[CAMERIC_MI_PATH_SELF]   = pConfigSelf->dcEnable;
    pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_SELF]   = pConfigSelf->dcWin;
    pCamEngineCtx->outWidth[CAMERIC_MI_PATH_SELF]   = pConfigSelf->width;
    pCamEngineCtx->outHeight[CAMERIC_MI_PATH_SELF]  = pConfigSelf->height;

    result = CamEngineSetupMiResolution( pCamEngineCtx, idx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't configure resolution on CamerIc MI module (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* configure main path */
    result = CamerIcMiSetDataMode( pCamEngineCtx->chain[idx].hCamerIc, CAMERIC_MI_PATH_MAIN, pConfigMain->mode );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't configure datamode on CamerIc MI module (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcMiSetDataLayout( pCamEngineCtx->chain[idx].hCamerIc, CAMERIC_MI_PATH_MAIN, pConfigMain->layout );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't configure datalayout on CamerIc MI module (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* configure self path */
    result = CamerIcMiSetDataMode( pCamEngineCtx->chain[idx].hCamerIc, CAMERIC_MI_PATH_SELF, pConfigSelf->mode );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't configure datamode on CamerIc MI module (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcMiSetDataLayout( pCamEngineCtx->chain[idx].hCamerIc, CAMERIC_MI_PATH_SELF, pConfigSelf->layout );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't configure datalayout on CamerIc MI module (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* configure CamerIc data path */
    CamerIcYcSplitterChannelMode_t ycSplitter = CAMERIC_YCSPLIT_CHMODE_OFF;
    if ( CAMERIC_MI_DATAMODE_DISABLED != pConfigMain->mode )
    {
        ycSplitter = CAMERIC_YCSPLIT_CHMODE_MP;
        if ( CAMERIC_MI_DATAMODE_DISABLED != pConfigSelf->mode )
        {
            ycSplitter = CAMERIC_YCSPLIT_CHMODE_MP_SP;
        }
    }
    else if ( CAMERIC_MI_DATAMODE_DISABLED != pConfigSelf->mode )
    {
        ycSplitter = CAMERIC_YCSPLIT_CHMODE_SP;
    }

    CamerIcDmaReadPath_t dmaRead = CAMERIC_DMA_READ_INVALID;
    result = CamerIcDriverGetViDmaSwitch( pCamEngineCtx->chain[idx].hCamerIc, &dmaRead );
    if ( ( result != RET_SUCCESS ) || ( dmaRead != CAMERIC_DMA_READ_SUPERIMPOSE ) )
    {
        dmaRead = CAMERIC_DMA_READ_ISP;
    }

    CamerIcMainPathMux_t mpMux = CAMERIC_MP_MUX_INVALID;
    if ( pConfigMain->mode == CAMERIC_MI_DATAMODE_JPEG )
    {
        mpMux = CAMERIC_MP_MUX_JPEG;
    }
    else
    {
        mpMux = CAMERIC_MP_MUX_MI;
    }

    CamerIcInterfaceSelect_t ifSelect = CAMERIC_ITF_SELECT_INVALID;
    result = CamerIcDriverGetIfSelect( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &ifSelect );
    if ( result != RET_SUCCESS )
    {
        ifSelect = CAMERIC_ITF_SELECT_PARALLEL;
    }

    result = CamerIcDriverSetDataPath( pCamEngineCtx->chain[idx].hCamerIc,
                                    mpMux,
                                    CAMERIC_SP_MUX_CAMERA,
                                    ycSplitter,
                                    CAMERIC_IE_MUX_CAMERA,
                                    dmaRead,
                                    ifSelect );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Invalid Data Path (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * CamEngineSetupMiResolution
 *****************************************************************************/
RESULT CamEngineSetupMiResolution
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineChainIdx_t idx
)
{
    RESULT result = RET_SUCCESS;

    uint32_t inWidth, inHeight, outWidth, outHeight;
    
    CamerIcDualCropingMode_t mode;
    CamerIcWindow_t dcWindow;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );
    DCT_ASSERT( idx < CHAIN_MAX );
    DCT_ASSERT( pCamEngineCtx->chain[idx].hCamerIc != NULL );

    if ( ( CAM_ENGINE_STATE_INITIALIZED != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_RUNNING     != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( ( BOOL_TRUE == pCamEngineCtx->dcEnable[CAMERIC_MI_PATH_MAIN] )  &&
         ( ( pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_MAIN].hOffset ) ||
           ( pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_MAIN].vOffset ) ||
           ( pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_MAIN].width   ) ||
           ( pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_MAIN].height  ) )   )
    {
        mode = CAMERIC_DCROP_YUV;
        dcWindow.hOffset = pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_MAIN].hOffset;
        dcWindow.vOffset = pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_MAIN].vOffset;
        dcWindow.width   = pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_MAIN].width;
        dcWindow.height  = pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_MAIN].height;
        
        /* scaler input size */
        inWidth  = pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_MAIN].width;
        inHeight = pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_MAIN].height;
    }
    else
    {
        mode = CAMERIC_DCROP_BYPASS;
        dcWindow.hOffset = 0u; 
        dcWindow.vOffset = 0u;
        dcWindow.width   = 0u;
        dcWindow.height  = 0u;
    
        /* scaler input size */
        inWidth  = pCamEngineCtx->chain[idx].isWindow.width;
        inHeight = pCamEngineCtx->chain[idx].isWindow.height;
    }

    /* scaler output size */
    outWidth  = ( 0 != pCamEngineCtx->outWidth[CAMERIC_MI_PATH_MAIN] )
                        ? pCamEngineCtx->outWidth[CAMERIC_MI_PATH_MAIN]
                        : inWidth;

    outHeight = ( 0 != pCamEngineCtx->outHeight[CAMERIC_MI_PATH_MAIN] )
                        ? pCamEngineCtx->outHeight[CAMERIC_MI_PATH_MAIN]
                        : inHeight;

    /* cropping window main path */

    /* cropping main path */
    result = CamerIcDualCropingSetOutputWindow(
                pCamEngineCtx->chain[idx].hCamerIc, CAMERIC_MI_PATH_MAIN, mode, &dcWindow );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't configure dual cropping uint (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* main path */
    result = CamerIcMiSetResolution( pCamEngineCtx->chain[idx].hCamerIc,
                    CAMERIC_MI_PATH_MAIN, inWidth, inHeight, outWidth, outHeight );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't configure resolution on CamerIc MI module (%d)\n", __FUNCTION__, result );
        return ( result );
    }
    
    /* cropping window self path */
    if ( ( BOOL_TRUE == pCamEngineCtx->dcEnable[CAMERIC_MI_PATH_SELF] )  &&
         ( ( pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_SELF].hOffset ) ||
           ( pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_SELF].vOffset ) ||
           ( pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_SELF].width   ) ||
           ( pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_SELF].height  ) )   )
    {
        mode = CAMERIC_DCROP_YUV;
        dcWindow.hOffset = pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_SELF].hOffset;
        dcWindow.vOffset = pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_SELF].vOffset;
        dcWindow.width   = pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_SELF].width;
        dcWindow.height  = pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_SELF].height;
        
        /* scaler input size */
        inWidth  = pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_SELF].width;
        inHeight = pCamEngineCtx->dcWindow[CAMERIC_MI_PATH_SELF].height;
    }
    else
    {
        mode = CAMERIC_DCROP_BYPASS;
        dcWindow.hOffset = 0u; 
        dcWindow.vOffset = 0u;
        dcWindow.width   = 0u;
        dcWindow.height  = 0u;
        
        /* scaler input size */
        inWidth  = pCamEngineCtx->chain[idx].isWindow.width;
        inHeight = pCamEngineCtx->chain[idx].isWindow.height;
    }

    /* cropping self path */
    result = CamerIcDualCropingSetOutputWindow(
                pCamEngineCtx->chain[idx].hCamerIc, CAMERIC_MI_PATH_SELF, mode, &dcWindow );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't configure dual cropping uint (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* scaler output size */
    outWidth  = ( 0 != pCamEngineCtx->outWidth[CAMERIC_MI_PATH_SELF] )
                        ? pCamEngineCtx->outWidth[CAMERIC_MI_PATH_SELF]
                        : inWidth;

    outHeight = ( 0 != pCamEngineCtx->outHeight[CAMERIC_MI_PATH_SELF] )
                        ? pCamEngineCtx->outHeight[CAMERIC_MI_PATH_SELF]
                        : inHeight;

    /* self path */
    result = CamerIcMiSetResolution( pCamEngineCtx->chain[idx].hCamerIc,
                    CAMERIC_MI_PATH_SELF, inWidth, inHeight, outWidth, outHeight );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't configure resolution on CamerIc MI module (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * CamEnginePreloadImage
 *****************************************************************************/
RESULT CamEnginePreloadImage
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineConfig_t   *pConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );
    DCT_ASSERT( CAM_ENGINE_MODE_IMAGE_PROCESSING == pConfig->mode );

    if ( CAM_ENGINE_STATE_INITIALIZED != CamEngineGetState( pCamEngineCtx ) )
    {
        return ( RET_WRONG_STATE );
    }

    if( NULL == pConfig )
    {
        return ( RET_NULL_POINTER );
    }

    PicBufType_t    type     = pConfig->data.image.type;
    PicBufLayout_t  layout   = pConfig->data.image.layout;
    const uint8_t   *pBuffer = pConfig->data.image.pBuffer;
    uint16_t        width    = pConfig->data.image.width;
    uint16_t        height   = pConfig->data.image.height;

    /* check parameter */
    if ( (pBuffer == NULL) || (width == 0U) || (height==0U) )
    {
        return ( RET_INVALID_PARM );
    }

    if ( (layout != PIC_BUF_LAYOUT_BAYER_RGRGGBGB)
            && (layout != PIC_BUF_LAYOUT_BAYER_GRGRBGBG)
            && (layout != PIC_BUF_LAYOUT_BAYER_GBGBRGRG)
            && (layout != PIC_BUF_LAYOUT_BAYER_BGBGGRGR) )
    {
        return ( RET_INVALID_PARM );
    }

    if ( (type != PIC_BUF_TYPE_RAW8) && ( type != PIC_BUF_TYPE_RAW16) )
    {
        return ( RET_NOTSUPP );
    }

    /* free existing buffer */
    if ( pCamEngineCtx->pDmaMediaBuffer != NULL )
    {
        MediaBufPoolFreeBuffer( &pCamEngineCtx->BufferPoolInput, pCamEngineCtx->pDmaMediaBuffer );
        pCamEngineCtx->pDmaMediaBuffer = NULL;
        pCamEngineCtx->pDmaBuffer = NULL;
    }

    /* get a picture buffer */
    MediaBuffer_t *pMediaBuffer = MediaBufPoolGetBuffer( &pCamEngineCtx->BufferPoolInput );
    if ( pMediaBuffer == NULL )
    {
        return ( RET_OUTOFMEM );
    }

    /* set picture buffer meta-data */
    PicBufMetaData_t *pPicBuffer = (PicBufMetaData_t *)pMediaBuffer->pMetaData;

    pPicBuffer->Type                    = type;
    pPicBuffer->Layout                  = layout;

    pPicBuffer->Data.raw.pBuffer        = (uint8_t *)pMediaBuffer->pBaseAddress;
    pPicBuffer->Data.raw.PicWidthBytes  = ( type == PIC_BUF_TYPE_RAW8 ) ? width : (width << 1);
    pPicBuffer->Data.raw.PicWidthPixel  = width;
    pPicBuffer->Data.raw.PicHeightPixel = height;

    /* dma-transfer buffer to onboard fpga memory */

    /* 1.) get a free write-only memory chunk from HAL */
    void *pMapBuffer = NULL;
    result = HalMapMemory( pCamEngineCtx->hHal,
                (unsigned long)( pPicBuffer->Data.raw.pBuffer ),
                (pPicBuffer->Data.raw.PicWidthBytes * pPicBuffer->Data.raw.PicHeightPixel),
                HAL_MAPMEM_WRITEONLY, &pMapBuffer ) ;
    if ( result != RET_SUCCESS )
    {
        MediaBufPoolFreeBuffer( &pCamEngineCtx->BufferPoolInput, pMediaBuffer );
        return ( result );
    }

    /* 2.) copy data to mapped memory chunk */
    MEMCPY( pMapBuffer, pBuffer,  (pPicBuffer->Data.raw.PicWidthBytes * pPicBuffer->Data.raw.PicHeightPixel) );

    /* 3.) initiate DMA transfer */
    result = HalUnMapMemory( pCamEngineCtx->hHal, pMapBuffer );
    if ( result != RET_SUCCESS )
    {
        MediaBufPoolFreeBuffer( &pCamEngineCtx->BufferPoolInput, pMediaBuffer );
        return ( result );
    }

    /* @TODO: actually we only have one dma-buffer, later we will
     * put this into a media-buffer-queue to store a sequence of
     * pictures to dma-load into cameric. */
    pCamEngineCtx->pDmaBuffer = pPicBuffer;
    pCamEngineCtx->pDmaMediaBuffer = pMediaBuffer;

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineSendCommand
 *****************************************************************************/
RESULT CamEngineSendCommand
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineCmd_t      *pCommand
)
{
    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    if( pCommand == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    /* are we shutting down? */
    if ( CamEngineGetState( pCamEngineCtx ) == CAM_ENGINE_STATE_INVALID )
    {
        return ( RET_CANCELED );
    }

    /* send command */
    OSLAYER_STATUS osStatus = osQueueWrite( &pCamEngineCtx->commandQueue, pCommand );
    if ( osStatus != OSLAYER_OK )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: (sending command to queue failed -> OSLAYER_STATUS=%d)\n", __FUNCTION__, CamEngineGetState( pCamEngineCtx ), osStatus );
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( (osStatus == OSLAYER_OK) ? RET_SUCCESS : RET_FAILURE );
}


/******************************************************************************
 * local functions
 *****************************************************************************/


/******************************************************************************
 * CamEngineSetState
 *****************************************************************************/
static inline void CamEngineSetState
(
    CamEngineContext_t   *pCamEngineCtx,
    CamEngineState_t     newState
)
{
    DCT_ASSERT( pCamEngineCtx != NULL );
    pCamEngineCtx->state = newState;
}


/******************************************************************************
 * local functions
 *****************************************************************************/


/******************************************************************************
 * CamEngineCompleteCommand
 *****************************************************************************/
static void CamEngineCompleteCommand
(
    CamEngineContext_t   *pCamEngineCtx,
    CamEngineCmdId_t     cmdId,
    RESULT               result
)
{
    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );
    DCT_ASSERT( pCamEngineCtx->cbCompletion != NULL );

    /* do callback */
    pCamEngineCtx->cbCompletion( cmdId, result, pCamEngineCtx->pUserCbCtx );

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );
}

static void StreamOffOverTimeHnd1(int sig)
{
    osMutexLock(&g_pCamEngineCtx_lock[0]);
    if(g_pCamEngineCtx[0]){
        TRACE( CAM_ENGINE_ERROR, "%s:strem off time out\n\n\n\n\n\n",__func__);
#if 1
        /* build shutdown command */
        CamEngineCmd_t command;
        MEMSET( &command, 0, sizeof(CamEngineCmd_t) );
        command.cmdId = CAM_ENGINE_CMD_HW_STREAMING_FINISHED;

        CamEngineSendCommand( g_pCamEngineCtx[0], &command );
#else
        CamEngineCmd_t command;
        CamEngineContext_t *pCamEngineCtx = g_pCamEngineCtx;
        RESULT lres;
        RESULT result = RET_WRONG_STATE;

        // stop the driver
        lres = CamEngineStopCamerIcDrv( pCamEngineCtx );
        DCT_ASSERT( lres == RET_SUCCESS );
        if ( lres != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: (can't stop the CamerIC driver %d)\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }

        // stop the sub controls
        lres = CamEngineSubCtrlsStop( pCamEngineCtx );
        DCT_ASSERT( lres == RET_SUCCESS );
        if ( lres != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: (can't stop the sub controls %d)\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }

        command.cmdId = CAM_ENGINE_CMD_STOP_STREAMING;

        /* complete command? */
        if ( result != RET_PENDING )
        {
            if ( NULL != pCamEngineCtx->cbCompletion )
            {
                CamEngineCompleteCommand( pCamEngineCtx, command.cmdId, result );
            }
        }
#endif
        TRACE( CAM_ENGINE_ERROR, "%s:strem off time out\n\n\n\n\n\n",__func__);
    }else{
        TRACE( CAM_ENGINE_ERROR, "%s:strem off normally \n\n\n\n\n\n",__func__);
    }
    osMutexUnlock(&g_pCamEngineCtx_lock[0]);
}

static void StreamOffOverTimeHnd2(int sig)
{
	osMutexLock(&g_pCamEngineCtx_lock[1]);
    if(g_pCamEngineCtx[1]){
        TRACE( CAM_ENGINE_ERROR, "%s:strem off time out\n\n\n\n\n\n",__func__);
#if 1
        /* build shutdown command */
        CamEngineCmd_t command;
        MEMSET( &command, 0, sizeof(CamEngineCmd_t) );
        command.cmdId = CAM_ENGINE_CMD_HW_STREAMING_FINISHED;

        CamEngineSendCommand( g_pCamEngineCtx[1], &command );
#else
        CamEngineCmd_t command;
        CamEngineContext_t *pCamEngineCtx = g_pCamEngineCtx;
        RESULT lres;
        RESULT result = RET_WRONG_STATE;

        // stop the driver
        lres = CamEngineStopCamerIcDrv( pCamEngineCtx );
        DCT_ASSERT( lres == RET_SUCCESS );
        if ( lres != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: (can't stop the CamerIC driver %d)\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }

        // stop the sub controls
        lres = CamEngineSubCtrlsStop( pCamEngineCtx );
        DCT_ASSERT( lres == RET_SUCCESS );
        if ( lres != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: (can't stop the sub controls %d)\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }

        command.cmdId = CAM_ENGINE_CMD_STOP_STREAMING;

        /* complete command? */
        if ( result != RET_PENDING )
        {
            if ( NULL != pCamEngineCtx->cbCompletion )
            {
                CamEngineCompleteCommand( pCamEngineCtx, command.cmdId, result );
            }
        }
#endif
        TRACE( CAM_ENGINE_ERROR, "%s:strem off time out\n\n\n\n\n\n",__func__);
    }else{
        TRACE( CAM_ENGINE_ERROR, "%s:strem off normally \n\n\n\n\n\n",__func__);
    }
    osMutexUnlock(&g_pCamEngineCtx_lock[1]);
}

/******************************************************************************
 * CamEngineThreadHandler
 *****************************************************************************/
static int32_t CamEngineThreadHandler
(
    void *p_arg
)
{
    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( p_arg )
    {
        CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)p_arg;

        bool_t bExit = BOOL_FALSE;

        do
        {
            /* set default result */
            RESULT result = RET_WRONG_STATE;

            /* wait for next command */
            CamEngineCmd_t command;
            OSLAYER_STATUS osStatus = osQueueRead(&pCamEngineCtx->commandQueue, &command);
            if (OSLAYER_OK != osStatus)
            {
                TRACE( CAM_ENGINE_ERROR, "%s (receiving command failed -> OSLAYER_RESULT=%d)\n", __FUNCTION__, osStatus );
                continue; /* for now we simply try again */
            }

            TRACE( CAM_ENGINE_DEBUG, "%s (received command %d)\n", __FUNCTION__, command.cmdId );
            switch ( CamEngineGetState( pCamEngineCtx ) )
            {
                case CAM_ENGINE_STATE_INITIALIZED:
                {
                    TRACE( CAM_ENGINE_INFO, "%s (enter init state)\n", __FUNCTION__ );

                    switch ( command.cmdId )
                    {
                        case CAM_ENGINE_CMD_SHUTDOWN:
                        {
                            TRACE( CAM_ENGINE_DEBUG, "%s (CAM_ENGINE_CMD_SHUTDOWN)\n", __FUNCTION__ );

                            /* stop further commands from being send to command queue */
                            CamEngineSetState( pCamEngineCtx, CAM_ENGINE_STATE_INVALID );

                            bExit = BOOL_TRUE;
                            result = RET_PENDING; /* avoid completion below */
                            break;
                        }

                        case CAM_ENGINE_CMD_START:
                        {
                            TRACE( CAM_ENGINE_DEBUG, "%s (CAM_ENGINE_CMD_START)\n", __FUNCTION__ );

                            CamEngineSetState( pCamEngineCtx, CAM_ENGINE_STATE_RUNNING );

                            result = RET_SUCCESS;
                            break;
                        }

                        default:
                        {
                            TRACE( CAM_ENGINE_ERROR, "%s (invalid command %d)\n", __FUNCTION__, (int32_t)command.cmdId );
                            break;
                        }
                    }

                    TRACE( CAM_ENGINE_INFO, "%s (exit init state)\n", __FUNCTION__ );

                    break;
                }

                case CAM_ENGINE_STATE_RUNNING:
                {
                    TRACE( CAM_ENGINE_INFO, "%s (enter run state)\n", __FUNCTION__ );

                    switch ( command.cmdId )
                    {

                        case CAM_ENGINE_CMD_SHUTDOWN:
                        case CAM_ENGINE_CMD_STOP:
                        {
                            TRACE( CAM_ENGINE_DEBUG, "%s (CAM_ENGINE_CMD_STOP)\n", __FUNCTION__ );

                            result = RET_SUCCESS;
                            RESULT lres;

                            lres = CamEngineReleaseDrv( pCamEngineCtx );
                            if ( lres != RET_SUCCESS )
                            {
                                TRACE( CAM_ENGINE_ERROR, "%s (releasing of camerIC failed %d)\n", __FUNCTION__, lres );
                                UPDATE_RESULT( result, lres );
                            }

                            lres = CamEngineReleasePixelIf( pCamEngineCtx ); // must be called before CamEngineReleaseCamerIc as long as MIPI & SMIA modules are part of CamerIc
                            if ( lres != RET_SUCCESS )
                            {
                                TRACE( CAM_ENGINE_ERROR, "%s (shutdown of MIPI failed %d)\n", __FUNCTION__, lres );
                                UPDATE_RESULT( result, lres );
                            }

                            lres = CamEngineReleaseCamerIc( pCamEngineCtx );
                            if ( lres != RET_SUCCESS )
                            {
                                TRACE( CAM_ENGINE_ERROR, "%s (shutdown of camerIC failed %d)\n", __FUNCTION__, lres );
                                UPDATE_RESULT( result, lres );
                            }

                            pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor  = NULL;
                            pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hSensor = NULL;
                            pCamEngineCtx->hCamCalibDb = NULL;
                            pCamEngineCtx->acqWindow = (CamEngineWindow_t){0, 0, 0, 0};
                            pCamEngineCtx->outWindow = (CamEngineWindow_t){0, 0, 0, 0};
                            pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].isWindow  = (CamEngineWindow_t){0, 0, 0, 0};
                            pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].isWindow  = (CamEngineWindow_t){0, 0, 0, 0};
                            
                            pCamEngineCtx->mode      = CAM_ENGINE_MODE_INVALID; 
                            //pCamEngineCtx->enableDMA = BOOL_FALSE;

                            CamEngineSetState( pCamEngineCtx, CAM_ENGINE_STATE_INITIALIZED );

                            if ( CAM_ENGINE_CMD_SHUTDOWN == command.cmdId )
                            {
                                /* stop further commands from being send to command queue */
                                CamEngineSetState( pCamEngineCtx, CAM_ENGINE_STATE_INVALID );

                                bExit = BOOL_TRUE;
                                result = RET_PENDING; /* avoid completion below */
                            }

                            break;
                        }

                        case CAM_ENGINE_CMD_START_STREAMING:
                        {
                            TRACE( CAM_ENGINE_DEBUG, "%s (CAM_ENGINE_CMD_START_STREAMING)\n", __FUNCTION__ );

                            result = RET_SUCCESS;
                            RESULT lres;

                            // number of frames to recieve before finishing
                            uint32_t nFrames = 0U;
                            if ( command.pCmdCtx != NULL )
                            {
                                nFrames = *((uint32_t *)command.pCmdCtx);
                                free ( command.pCmdCtx );
                            }

                            if(!pCamEngineCtx->isSOCsensor){
                                // enable interrupts for ISP drv modules
                                lres = CamEngineReStartEventCbCamerIcDrv( pCamEngineCtx );
                                DCT_ASSERT( lres == RET_SUCCESS );
                                if ( lres != RET_SUCCESS )
                                {
                                    TRACE( CAM_ENGINE_ERROR, "%s: (can't enable interrupts for ISP drv modules %d)\n", __FUNCTION__, lres );
                                    break;
                                }
                            }

                            // start the sub controls
                            lres = CamEngineSubCtrlsStart( pCamEngineCtx );
                            DCT_ASSERT( lres == RET_SUCCESS );
                            if ( lres != RET_SUCCESS )
                            {
                                TRACE( CAM_ENGINE_ERROR, "%s: (can't start the sub controls %d)\n", __FUNCTION__, lres );
                                break;
                            }

                            // start image loading by media input module
                            if ( (CAM_ENGINE_MODE_IMAGE_PROCESSING == pCamEngineCtx->mode) && 
                                 ( NULL != pCamEngineCtx->pDmaMediaBuffer ) )
                            {
                                lres = MimCtrlLoadPicture( pCamEngineCtx->hMimCtrl, pCamEngineCtx->pDmaBuffer );
                                DCT_ASSERT( lres == RET_PENDING );
                                if ( lres != RET_PENDING )
                                {
                                    TRACE( CAM_ENGINE_ERROR, "%s: (can't load picture %d)\n", __FUNCTION__, lres );
                                    (void)CamEngineSubCtrlsStop( pCamEngineCtx );
                                    UPDATE_RESULT( result, lres );
                                    break;
                                }
                            }

                            // start CamerIc driver
                            //set swap byte ,zyc
                            if(!pCamEngineCtx->isSOCsensor){
                                CamerIcDriverSetIsSwapByte(pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc,BOOL_FALSE );
                            }else{
                                CamerIcDriverSetIsSwapByte(pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc,BOOL_TRUE);
                            }

                            lres = CamEngineStartCamerIcDrv( pCamEngineCtx );
                            DCT_ASSERT( lres == RET_SUCCESS );
                            if ( lres != RET_SUCCESS )
                            {
                                TRACE( CAM_ENGINE_ERROR, "%s: (can't start the CamerIC driver %d)\n", __FUNCTION__, result );
                                (void)CamEngineSubCtrlsStop( pCamEngineCtx );
                                UPDATE_RESULT( result, lres );
                                break;
                            }

                            if ( pCamEngineCtx->enableJPE == BOOL_TRUE )
                            {
                                lres = CamerIcJpeStartHeaderGeneration( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc, CAMERIC_JPE_HEADER_JFIF );
                                DCT_ASSERT( lres == RET_PENDING );
                                if ( OSLAYER_OK != osEventWait( &pCamEngineCtx->JpeEventGenHeader ) )
                                {
                                    (void)CamEngineStopCamerIcDrv( pCamEngineCtx );
                                    (void)CamEngineSubCtrlsStop( pCamEngineCtx );
                                    result = RET_FAILURE;
                                    break;
                                }

                                lres = CamerIcJpeStartEncoding( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc, CAMERIC_JPE_MODE_LARGE_CONTINUOUS );
                                DCT_ASSERT( lres == RET_PENDING );
                            }

                            // start CamerIc input aqu.
                            pCamEngineCtx->camCaptureCompletionCb.pUserContext = (void *)pCamEngineCtx;
                            pCamEngineCtx->camCaptureCompletionCb.pParam       = NULL;
                            pCamEngineCtx->camCaptureCompletionCb.func         = CamEngineCamerIcDrvCommandCb;

                            lres = CamerIcDriverCaptureFrames( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, nFrames, &(pCamEngineCtx->camCaptureCompletionCb) );
                            DCT_ASSERT( lres == RET_PENDING );
                            if ( lres != RET_PENDING )
                            {
                                TRACE( CAM_ENGINE_ERROR, "%s: (can't start capturing %d)\n", __FUNCTION__, lres );
                                (void)CamerIcDriverStopInput( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &(pCamEngineCtx->camCaptureCompletionCb) );
                                (void)CamEngineStopCamerIcDrv( pCamEngineCtx );
                                (void)CamEngineSubCtrlsStop( pCamEngineCtx );
                                UPDATE_RESULT( result, lres );
                                break;
                            }
                            
                            // start 2nd pipe in 3D mode
                            if ( ( CAM_ENGINE_MODE_SENSOR_3D         == pCamEngineCtx->mode ) || 
                                 ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pCamEngineCtx->mode ) )
                            {
                                lres = CamerIcDriverCaptureFrames( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, nFrames, &(pCamEngineCtx->camCaptureCompletionCb) );
                                DCT_ASSERT( lres == RET_PENDING );
                                if ( lres != RET_PENDING )
                                {
                                    TRACE( CAM_ENGINE_ERROR, "%s: (can't start capturing %d)\n", __FUNCTION__, lres );
                                    (void)CamerIcDriverStopInput( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &(pCamEngineCtx->camCaptureCompletionCb) );
                                    (void)CamEngineStopCamerIcDrv( pCamEngineCtx );
                                    (void)CamEngineSubCtrlsStop( pCamEngineCtx );
                                    UPDATE_RESULT( result, lres );
                                    break;
                                }
                            }

                            // change state 
                            CamEngineSetState( pCamEngineCtx, CAM_ENGINE_STATE_STREAMING );

                            break;
                        }

                        case CAM_ENGINE_CMD_HW_STREAMING_FINISHED:
                        {
                            TRACE( CAM_ENGINE_INFO, "%s (CAM_ENGINE_CMD_HW_STREAMING_FINISHED)\n", __FUNCTION__ );
                            struct itimerval value,oval;
                            result = RET_SUCCESS;
                            RESULT lres;

                            //cancel timer
                        //    signal(SIGALRM, NULL);
                            osMutexLock(&pCamEngineCtx->camEngineCtx_lock);
                            if(g_pCamEngineCtx[0] || g_pCamEngineCtx[1]){
                            	if(g_pCamEngineCtx[0] == pCamEngineCtx)
                                	g_pCamEngineCtx[0] = NULL;
                            	if(g_pCamEngineCtx[1] == pCamEngineCtx)
                                	g_pCamEngineCtx[1] = NULL;
                                osMutexUnlock(&pCamEngineCtx->camEngineCtx_lock);
                                
                                value.it_value.tv_sec = 0;
                                value.it_value.tv_usec = 0;
                                value.it_interval.tv_sec = 0;
                                value.it_interval.tv_usec = 0;
                                setitimer(ITIMER_REAL,&value,&oval);
                                
                                // stop the driver
                                lres = CamEngineStopCamerIcDrv( pCamEngineCtx );
                                DCT_ASSERT( lres == RET_SUCCESS );
                                if ( lres != RET_SUCCESS )
                                {
                                    TRACE( CAM_ENGINE_ERROR, "%s: (can't stop the CamerIC driver %d)\n", __FUNCTION__, lres );
                                    UPDATE_RESULT( result, lres );
                                }

                                // stop the sub controls
                                lres = CamEngineSubCtrlsStop( pCamEngineCtx );
                                DCT_ASSERT( lres == RET_SUCCESS );
                                if ( lres != RET_SUCCESS )
                                {
                                    TRACE( CAM_ENGINE_ERROR, "%s: (can't stop the sub controls %d)\n", __FUNCTION__, lres );
                                    UPDATE_RESULT( result, lres );
                                }

                                command.cmdId = CAM_ENGINE_CMD_STOP_STREAMING;
                            }else{
                                //needn't process this msg
                                result = RET_PENDING;
                                osMutexUnlock(&pCamEngineCtx->camEngineCtx_lock);
                            }

                            break;
                        }

                        case CAM_ENGINE_CMD_HW_DMA_FINISHED:
                        {
                            TRACE( CAM_ENGINE_DEBUG, "%s (CAM_ENGINE_CMD_HW_DMA_FINISHED)\n", __FUNCTION__ );

                            result = RET_SUCCESS;

                            break;
                        }

                        default:
                        {
                            TRACE( CAM_ENGINE_ERROR, "%s (invalid command %d)\n", __FUNCTION__, (int32_t)command.cmdId );
                            break;
                        }
                    }

                    TRACE( CAM_ENGINE_INFO, "%s (exit run state)\n", __FUNCTION__ );

                    break;
                }

                case CAM_ENGINE_STATE_STREAMING:
                {
                    TRACE( CAM_ENGINE_INFO, "%s (enter streaming state)\n", __FUNCTION__ );

                    switch ( command.cmdId )
                    {
                        case CAM_ENGINE_CMD_AAA_LOCKED:
                        {
                            TRACE( CAM_ENGINE_DEBUG, "%s (CAM_ENGINE_CMD_AAA_LOCKED)\n", __FUNCTION__ );

                            result = RET_SUCCESS;
                            command.cmdId = CAM_ENGINE_CMD_ACQUIRE_LOCK;

                            result = CamerIcIspDeRegisterEventCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
                            DCT_ASSERT( result == RET_SUCCESS );

                            break;
                        }

                        case CAM_ENGINE_CMD_ACQUIRE_LOCK:
                        {
                            TRACE( CAM_ENGINE_DEBUG, "%s (CAM_ENGINE_CMD_ACQUIRE_LOCK)\n", __FUNCTION__ );

                            result = RET_SUCCESS;

                            CamEngineLockType_t locks = CAM_ENGINE_LOCK_NO;
                            if ( command.pCmdCtx != NULL )
                            {
                                locks = *((CamEngineLockType_t *)command.pCmdCtx);
                                free ( command.pCmdCtx );
                            }

                            if ( locks & CAM_ENGINE_LOCK_AEC )
                            {
                                RESULT lres = AecTryLock( pCamEngineCtx->hAec );
                                if ( lres == RET_SUCCESS )
                                {
                                    pCamEngineCtx->LockedMask |= CAM_ENGINE_LOCK_AEC;
                                }
                                else if ( lres == RET_PENDING )
                                {
                                    // AEC currently not settled so check for locking at frame end again
                                    result = RET_PENDING;
                                    pCamEngineCtx->LockMask |= CAM_ENGINE_LOCK_AEC;
                                }
                            }

                            if ( locks & CAM_ENGINE_LOCK_AWB )
                            {
                                if ( pCamEngineCtx->LockedMask & CAM_ENGINE_LOCK_AEC )
                                {
                                    RESULT lres = AwbTryLock( pCamEngineCtx->hAwb );
                                    if ( lres == RET_SUCCESS )
                                    {
                                        pCamEngineCtx->LockedMask |= CAM_ENGINE_LOCK_AWB;
                                    }
                                    else if ( lres == RET_PENDING )
                                    {
                                        result = RET_PENDING;
                                        pCamEngineCtx->LockMask |= CAM_ENGINE_LOCK_AWB;
                                    }
                                }
                                else
                                {
                                    result = RET_PENDING;
                                    pCamEngineCtx->LockMask |= CAM_ENGINE_LOCK_AWB;
                                }
                            }

                            if ( ( BOOL_TRUE == pCamEngineCtx->availableAf )
                                    && ( locks & CAM_ENGINE_LOCK_AF ) )
                            {
                                bool_t running = BOOL_FALSE;
                                AfSearchStrategy_t fss = AFM_FSS_INVALID;
                                if ( RET_SUCCESS == AfStatus( pCamEngineCtx->hAf, &running, &fss, NULL ) )
                                {
                                    if (running == BOOL_FALSE)
                                    {
                                        if ( RET_SUCCESS == AfOneShot( pCamEngineCtx->hAf, fss ) )
                                        {
                                            result = RET_PENDING;
                                            pCamEngineCtx->LockMask |= CAM_ENGINE_LOCK_AF;
                                        }
                                    }
                                    else
                                    {
                                        RESULT lres = AfTryLock( pCamEngineCtx->hAf );
                                        if ( lres == RET_SUCCESS )
                                        {
                                            pCamEngineCtx->LockedMask |= CAM_ENGINE_LOCK_AF;
                                        }
                                        else if ( lres == RET_PENDING )
                                        {
                                            result = RET_PENDING;
                                            pCamEngineCtx->LockMask |= CAM_ENGINE_LOCK_AF;
                                        }
                                    }
                                }
                            }

                            if ( result == RET_PENDING )
                            {
                                result = CamerIcIspRegisterEventCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc,
                                        CamEngineCamerIcDrvLockCb, pCamEngineCtx );
                                DCT_ASSERT( result == RET_SUCCESS );
                                result = RET_PENDING;
                            }

                            break;
                        }

                        case CAM_ENGINE_CMD_RELEASE_LOCK:
                        {
                            TRACE( CAM_ENGINE_DEBUG, "%s (CAM_ENGINE_CMD_RELEASE_LOCK)\n", __FUNCTION__ );

                            result = RET_SUCCESS;

                            uint32_t locks = CAM_ENGINE_LOCK_NO;
                            if ( command.pCmdCtx != NULL )
                            {
                                locks = *((uint32_t *)command.pCmdCtx);
                                free ( command.pCmdCtx );
                            }

                            if ( locks & CAM_ENGINE_LOCK_AEC )
                            {
                                result = AecUnLock( pCamEngineCtx->hAec );
                                DCT_ASSERT( result == RET_SUCCESS );
                                pCamEngineCtx->LockMask &= ~CAM_ENGINE_LOCK_AEC;
                                pCamEngineCtx->LockedMask &= ~CAM_ENGINE_LOCK_AEC;
                            }

                            if ( locks & CAM_ENGINE_LOCK_AWB )
                            {
                                result = AwbUnLock( pCamEngineCtx->hAwb );
                                DCT_ASSERT( result == RET_SUCCESS );
                                pCamEngineCtx->LockMask &= ~CAM_ENGINE_LOCK_AWB;
                                pCamEngineCtx->LockedMask &= ~CAM_ENGINE_LOCK_AWB;
                            }

                            if ( ( BOOL_TRUE == pCamEngineCtx->availableAf )
                                    && (locks & CAM_ENGINE_LOCK_AF) )
                            {
                                result = AfUnLock( pCamEngineCtx->hAf );
                                DCT_ASSERT( result == RET_SUCCESS );
                                pCamEngineCtx->LockMask &= ~CAM_ENGINE_LOCK_AF;
                                pCamEngineCtx->LockedMask &= ~CAM_ENGINE_LOCK_AF;
                            }

                            if ( (!pCamEngineCtx->LockMask) && (!pCamEngineCtx->LockedMask) )
                            {
                                result = CamerIcIspDeRegisterEventCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
                                DCT_ASSERT( result == RET_SUCCESS );
                            }

                            break;
                        }

                        case CAM_ENGINE_CMD_STOP_STREAMING:
                        {
                            TRACE( CAM_ENGINE_DEBUG, "%s (CAM_ENGINE_CMD_STOP_STREAMING)\n", __FUNCTION__ );

                            result = RET_PENDING;
                            RESULT lres;
                            struct itimerval value,oval;

                            /* stop input */
                            pCamEngineCtx->camStopInputCompletionCb.pUserContext = (void *)pCamEngineCtx;
                            pCamEngineCtx->camStopInputCompletionCb.pParam       = NULL;
                            pCamEngineCtx->camStopInputCompletionCb.func         = CamEngineCamerIcDrvCommandCb;

                            lres = CamerIcDriverStopInput( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &(pCamEngineCtx->camStopInputCompletionCb) );
                            DCT_ASSERT( lres == RET_PENDING );
                            if ( lres != RET_PENDING )
                            {
                                TRACE( CAM_ENGINE_ERROR, "%s: (can't stop the input %d)\n", __FUNCTION__, lres );
                                UPDATE_RESULT( result, lres );
                            }


                            //start timer
                            if(pCamEngineCtx->index == 0){
                            	g_pCamEngineCtx[0] = pCamEngineCtx;
                            	g_pCamEngineCtx_lock[0] = pCamEngineCtx->camEngineCtx_lock;
								value.it_value.tv_sec = 0;
								value.it_value.tv_usec = 500*1000;
								value.it_interval.tv_sec = 0;
								value.it_interval.tv_usec = 0;
								signal(SIGALRM,StreamOffOverTimeHnd1);
								setitimer(ITIMER_REAL,&value,&oval);

                            }else{
                            	g_pCamEngineCtx[1] = pCamEngineCtx;
                            	g_pCamEngineCtx_lock[1] = pCamEngineCtx->camEngineCtx_lock;
								value.it_value.tv_sec = 0;
								value.it_value.tv_usec = 500*1000;
								value.it_interval.tv_sec = 0;
								value.it_interval.tv_usec = 0;
								signal(SIGALRM,StreamOffOverTimeHnd2);
								setitimer(ITIMER_REAL,&value,&oval);
                            }

                            
                            CamEngineSetState( pCamEngineCtx, CAM_ENGINE_STATE_RUNNING );

                            break;
                        }

                        case CAM_ENGINE_CMD_HW_DMA_FINISHED:
                        {
                            TRACE( CAM_ENGINE_DEBUG, "%s (CAM_ENGINE_CMD_HW_DMA_FINISHED)\n", __FUNCTION__ );

                            result = RET_SUCCESS;
                            RESULT lres;

                            if ( (CAM_ENGINE_MODE_IMAGE_PROCESSING == pCamEngineCtx->mode) && ( NULL != pCamEngineCtx->pDmaMediaBuffer ) )
                            {
                                lres = MimCtrlLoadPicture( pCamEngineCtx->hMimCtrl, pCamEngineCtx->pDmaBuffer );
                                DCT_ASSERT( lres == RET_PENDING );
                                if ( lres != RET_PENDING )
                                {
                                    TRACE( CAM_ENGINE_ERROR, "%s: (can't load picture %d)\n", __FUNCTION__, lres );
                                    UPDATE_RESULT( result, lres );
                                }
                            }
                            else if ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pCamEngineCtx->mode && ( NULL == pCamEngineCtx->pDmaMediaBuffer ) )
                            {
                                MediaBuffer_t *pBuffer = NULL;

                                osStatus = osQueueTryRead( &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MainPathQueue, &pBuffer );
                                if ( (OSLAYER_OK == osStatus) && (NULL!=pBuffer) )
                                {
                                    ulong_t frameId = (ulong_t)&(pBuffer);

                                    TRACE( CAM_ENGINE_INFO, "%s: (received buffer (id=0x%08x)\n", __FUNCTION__, frameId );

                                    lres = AvsSetCroppingWindow( pCamEngineCtx->hAvs, frameId );
                                    //DCT_ASSERT( lres == RET_SUCCESS );
                                    if ( lres != RET_SUCCESS )
                                    {
                                        TRACE( CAM_ENGINE_WARN, "%s: (can't set cropping window %d (id=0x%08x)\n", __FUNCTION__, lres, frameId );
                                        UPDATE_RESULT( result, lres );
                                    }
                                    else
                                    {
                                        TRACE( CAM_ENGINE_INFO, "%s: (set cropping window (id=0x%08x)\n", __FUNCTION__, frameId );
                                    }
    
                                    pCamEngineCtx->pDmaMediaBuffer = pBuffer;
                                    pCamEngineCtx->pDmaBuffer      = (PicBufMetaData_t *)pBuffer->pMetaData;

                                    lres = MimCtrlLoadPicture( pCamEngineCtx->hMimCtrl, pCamEngineCtx->pDmaBuffer );
                                    DCT_ASSERT( lres == RET_PENDING );
                                    if ( lres != RET_PENDING )
                                    {
                                        TRACE( CAM_ENGINE_ERROR, "%s: (can't load picture %d)\n", __FUNCTION__, lres );
                                        UPDATE_RESULT( result, lres );
                                    }
                                 }
                                 else
                                 {
                                     if ( pBuffer )
                                     {
                                         MediaBufUnlockBuffer( pBuffer );
                                     }
                                 }
                            }

                            break;
                        }

                        default:
                        {
                            TRACE( CAM_ENGINE_ERROR, "%s (invalid command %d)\n", __FUNCTION__, (int32_t)command.cmdId );
                            break;
                        }
                    }

                    TRACE( CAM_ENGINE_INFO, "%s (exit streaming state)\n", __FUNCTION__ );

                    break;
                }

                default:
                {
                    TRACE( CAM_ENGINE_ERROR, "%s (illegal state %d)\n", __FUNCTION__, (int32_t)CamEngineGetState( pCamEngineCtx ) );
                    break;
                }
            }

            /* complete command? */
            if ( result != RET_PENDING )
            {
                if ( NULL != pCamEngineCtx->cbCompletion )
                {
                    CamEngineCompleteCommand( pCamEngineCtx, command.cmdId, result );
                }
            }
        }
        while ( bExit == BOOL_FALSE );  /* !bExit */
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( 0 );
}


uint32_t CamerEngineSetDataPathWhileStreaming
(
    CamEngineHandle_t  pCamEngineCtx,
    CamerIcWindow_t* pWin,
    uint32_t outWidth, 
    uint32_t outHeight
    
)
{

     CamerIcSetDataPathWhileStreaming(((CamEngineContext_t*)pCamEngineCtx)->chain[pCamEngineCtx->chainIdx0].hCamerIc,
                            pWin,outWidth,outHeight);

    return 0;
}

