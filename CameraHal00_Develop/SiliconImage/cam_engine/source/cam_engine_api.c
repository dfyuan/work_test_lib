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
 * @file cam_engine_api.c
 *
 * @brief
 *   Implementation of the CamEngine API.
 *
 *****************************************************************************/
#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <ebase/dct_assert.h>
#include <common/misc.h>

#include "cam_engine_api.h"
#include "cam_engine.h"
#include "cam_engine_subctrls.h"
#include "cam_engine_modules.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAM_ENGINE_API_INFO , "CAM_ENGINE_API: ", INFO,    0 );
CREATE_TRACER( CAM_ENGINE_API_WARN , "CAM_ENGINE_API: ", WARNING, 1 );
CREATE_TRACER( CAM_ENGINE_API_ERROR, "CAM_ENGINE_API: ", ERROR,   1 );


/******************************************************************************
 * local type definitions
 *****************************************************************************/


/******************************************************************************
 * local variable declarations
 *****************************************************************************/
static uint32_t g_idx = 0;

/******************************************************************************
 * local function prototypes
 *****************************************************************************/


/******************************************************************************
 * local function
 *****************************************************************************/
static void CamEngineSwapChains
(
    CamEngineContext_t *pCamEngineCtx
)
{
    uint32_t chainIdx = pCamEngineCtx->chainIdx0;
    pCamEngineCtx->chainIdx0 = pCamEngineCtx->chainIdx1;
    pCamEngineCtx->chainIdx1 = chainIdx;
}


/******************************************************************************
 * See header file for detailed comment.
 *****************************************************************************/


/******************************************************************************
 * CamEngineInit
 *****************************************************************************/
RESULT CamEngineInit
(
    CamEngineInstanceConfig_t *pConfig
)
{
    CamEngineContext_t *pCamEngineCtx;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // initial checks 
    if ( pConfig == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    if ( pConfig->hHal == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pConfig->maxPendingCommands == 0 )
    {
        return ( RET_OUTOFRANGE );
    }

    // allocate control context
    pCamEngineCtx = malloc( sizeof(CamEngineContext_t) );
    if ( pCamEngineCtx == NULL )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s (allocating control context failed)\n", __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( pCamEngineCtx, 0, sizeof(CamEngineContext_t) );

    // pre initialize control context
    pCamEngineCtx->state                = CAM_ENGINE_STATE_INVALID;
    pCamEngineCtx->maxCommands          = pConfig->maxPendingCommands;
    pCamEngineCtx->isSystem3D           = pConfig->isSystem3D;
    pCamEngineCtx->cbCompletion         = pConfig->cbCompletion;
    pCamEngineCtx->cbAfpsResChange      = pConfig->cbAfpsResChange;
    pCamEngineCtx->pUserCbCtx           = pConfig->pUserCbCtx;
    pCamEngineCtx->hHal                 = pConfig->hHal;

    pCamEngineCtx->orient               = CAMERIC_MI_ORIENTATION_ORIGINAL;
    pCamEngineCtx->mode                 = CAM_ENGINE_MODE_SENSOR_2D;

    pCamEngineCtx->enableJPE            = BOOL_FALSE;
    pCamEngineCtx->chainIdx0            = CHAIN_MASTER;
    pCamEngineCtx->chainIdx1            = CHAIN_SLAVE;

    pCamEngineCtx->flickerPeriod        = CAM_ENGINE_FLICKER_OFF;
    pCamEngineCtx->enableAfps           = BOOL_FALSE;
    pCamEngineCtx->index           		= g_idx++;

    // create control process
    result = CamEngineCreate( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s (creating control process failed)\n", __FUNCTION__ );
        free( pCamEngineCtx );
    }
    else
    {
        // control context is initialized, we're ready and in idle state
        pCamEngineCtx->state = CAM_ENGINE_STATE_INITIALIZED;

        // success, so let's return control context
        pConfig->hCamEngine = (CamEngineHandle_t)pCamEngineCtx;
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineShutDown
 *****************************************************************************/
RESULT CamEngineShutDown
(
    CamEngineHandle_t hCamEngine
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // initial checks
    if( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    // check cam-engine thread state
    if ( ( CAM_ENGINE_STATE_INITIALIZED != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_RUNNING     != pCamEngineCtx->state) )
    {
        return ( RET_WRONG_STATE );
    }

    result = CamEngineDestroy( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
         TRACE( CAM_ENGINE_API_ERROR, "%s (destroying control process failed -> RESULT=%d)\n", __FUNCTION__, result);
    }

    // release context memory
    free( pCamEngineCtx );
	g_idx = 0;
    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineStart
 *****************************************************************************/
RESULT CamEngineStart
(
    CamEngineHandle_t   hCamEngine,
    CamEngineConfig_t   *pConfig
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // initial checks
    if( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pConfig )
    {
        return ( RET_NULL_POINTER );
    }

    // check cam-engine thread state
    if ( CAM_ENGINE_STATE_INITIALIZED != pCamEngineCtx->state )
    {
        return ( RET_WRONG_STATE );
    }

    // set scenario mode
    if (  ( pConfig->mode <= CAM_ENGINE_MODE_INVALID ) ||
          ( pConfig->mode >= CAM_ENGINE_MODE_MAX ) )
    {
        return ( RET_INVALID_PARM );
    }
    pCamEngineCtx->mode = pConfig->mode;
    
    // set calibration database handle
    pCamEngineCtx->hCamCalibDb = pConfig->hCamCalibDb;

    // check and set configuration type specific values
    switch ( pConfig->mode )
    {
        case CAM_ENGINE_MODE_SENSOR_2D:
        case CAM_ENGINE_MODE_SENSOR_3D:
        {
            if ( NULL == pConfig->data.sensor.hSensor )
            {
                return ( RET_WRONG_HANDLE );
            }

            if ( ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode ) &&
                 ( NULL == pConfig->data.sensor.hSubSensor ) )
            {
                return ( RET_WRONG_HANDLE );
            }

            if ( ( pConfig->data.sensor.ifSelect <= CAMERIC_ITF_SELECT_INVALID ) ||
                 ( pConfig->data.sensor.ifSelect >= CAMERIC_ITF_SELECT_MAX ) )
            {
                return ( RET_INVALID_PARM );
            }
           
            if ( BOOL_TRUE == pCamEngineCtx->chainsSwapped )
            {
                CamEngineSwapChains( pCamEngineCtx );
                pCamEngineCtx->chainsSwapped = BOOL_FALSE;
            }

            // set sensor handles
            pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor  = pConfig->data.sensor.hSensor;
            if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
            {
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hSensor = pConfig->data.sensor.hSubSensor;
            }
            else
            {
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hSensor = NULL;
            }

            // configure frame size
            pCamEngineCtx->acqWindow = pConfig->data.sensor.acqWindow;
            pCamEngineCtx->outWindow = pConfig->data.sensor.outWindow;

            pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].isWindow = pConfig->data.sensor.isWindow;
            pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].isWindow = pConfig->data.sensor.isWindow;

            // flicker period
            pCamEngineCtx->flickerPeriod = pConfig->data.sensor.flickerPeriod;
            pCamEngineCtx->enableAfps    = pConfig->data.sensor.enableAfps;
            break;
        }

        case CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB:
        {
            if ( NULL == pConfig->data.sensor.hSensor )
            {
                return ( RET_WRONG_HANDLE );
            }

            if ( ( pConfig->data.sensor.ifSelect <= CAMERIC_ITF_SELECT_INVALID ) ||
                 ( pConfig->data.sensor.ifSelect >= CAMERIC_ITF_SELECT_MAX ) )
            {
                return ( RET_INVALID_PARM );
            }

            if ( BOOL_FALSE == pCamEngineCtx->chainsSwapped )
            {
                CamEngineSwapChains( pCamEngineCtx );
                pCamEngineCtx->chainsSwapped = BOOL_TRUE;
            }

            // set sensor handles
            pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor  = pConfig->data.sensor.hSensor;
            pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hSensor   = NULL;

            // configure frame size
            pCamEngineCtx->acqWindow = pConfig->data.sensor.acqWindow;
            pCamEngineCtx->outWindow = pConfig->data.sensor.outWindow;

            pCamEngineCtx->chain[CHAIN_MASTER].isWindow = pConfig->data.sensor.isCroppingWindow;
            pCamEngineCtx->chain[CHAIN_SLAVE].isWindow  = pConfig->data.sensor.isWindow;

            // flicker period
            pCamEngineCtx->flickerPeriod = pConfig->data.sensor.flickerPeriod;
            pCamEngineCtx->enableAfps    = pConfig->data.sensor.enableAfps;
            break;
        }

        case CAM_ENGINE_MODE_IMAGE_PROCESSING:
        {
            if ( pConfig->data.image.pBuffer == NULL )
            {
                return ( RET_NULL_POINTER );
            }
           
            if ( BOOL_TRUE == pCamEngineCtx->chainsSwapped )
            {
                CamEngineSwapChains( pCamEngineCtx );
                pCamEngineCtx->chainsSwapped = BOOL_FALSE;
            }
 
            // set sensor handles
            pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor  = NULL;
            pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hSensor = NULL;

            // configurae frame size
            pCamEngineCtx->acqWindow = (CamEngineWindow_t){0, 0, pConfig->data.image.width, pConfig->data.image.height};
            pCamEngineCtx->outWindow = (CamEngineWindow_t){0, 0, pConfig->data.image.width, pConfig->data.image.height};
            pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].isWindow  = (CamEngineWindow_t){0, 0, pConfig->data.image.width, pConfig->data.image.height};
            pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].isWindow  = (CamEngineWindow_t){0, 0, pConfig->data.image.width, pConfig->data.image.height};

            // flicker period
            pCamEngineCtx->flickerPeriod = CAM_ENGINE_FLICKER_OFF;
            pCamEngineCtx->enableAfps    = BOOL_FALSE;

            pCamEngineCtx->vGain  = pConfig->data.image.vGain;
            pCamEngineCtx->vItime = pConfig->data.image.vItime;

            break;
        }

        default:
        {
            TRACE( CAM_ENGINE_API_ERROR, "%s (invalid configuration type)\n", __FUNCTION__);
            return ( RET_INVALID_PARM );
        }
    }

    // setup calibration database access
    if ( NULL != pCamEngineCtx->hCamCalibDb )
    {
        result = CamEnginePrepareCalibDbAccess( pCamEngineCtx );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR, "%s (preparing calibration database failed -> RESULT=%d)\n", __FUNCTION__, result);
            return ( result );
        }
    }

    // initialize camerIC
    result = CamEngineInitCamerIc( pCamEngineCtx, pConfig );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s (initializing camerIC failed -> RESULT=%d)\n", __FUNCTION__, result);
        return ( result );
    }

    // initialize pixel interface (MIPI, SMIA etc)
    // must be called after CamEngineInitCamerIc as long as MIPI & SMIA modules are part of CamerIc
    result = CamEngineInitPixelIf( pCamEngineCtx, pConfig ); 
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s (initializing PixelInterface failed -> RESULT=%d)\n", __FUNCTION__, result);
        (void)CamEngineReleaseCamerIc( pCamEngineCtx );
        return ( result );
    }

    // setup input acquisition
    if ( ( CAM_ENGINE_MODE_SENSOR_2D         == pConfig->mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pConfig->mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_3D         == pConfig->mode ) )
    {
        // master pipe
        result = CamEngineSetupAcqForSensor( pCamEngineCtx, pConfig, pCamEngineCtx->chainIdx0 );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_API_ERROR, "%s (configuring camerIC failed -> RESULT=%d)\n", __FUNCTION__, result);
            (void)CamEngineReleasePixelIf( pCamEngineCtx ); // must be called before CamEngineReleaseCamerIc as long as MIPI & SMIA modules are part of CamerIc
            (void)CamEngineReleaseCamerIc( pCamEngineCtx );
            return ( result );
        }

        // slave pipe
        if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
        {
            result = CamEngineSetupAcqForSensor( pCamEngineCtx, pConfig, pCamEngineCtx->chainIdx1 );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_API_ERROR, "%s (configuring camerIC failed -> RESULT=%d)\n", __FUNCTION__, result);
                (void)CamEngineReleasePixelIf( pCamEngineCtx ); // must be called before CamEngineReleaseCamerIc as long as MIPI & SMIA modules are part of CamerIc
                (void)CamEngineReleaseCamerIc( pCamEngineCtx );
                return ( result );
            }
        }
        else if ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pConfig->mode )
        {
            result = CamEngineSetupAcqForImgStab( pCamEngineCtx, pConfig, pCamEngineCtx->chainIdx1 );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_API_ERROR, "%s (configuring camerIC failed -> RESULT=%d)\n", __FUNCTION__, result);
                (void)CamEngineReleasePixelIf( pCamEngineCtx ); // must be called before CamEngineReleaseCamerIc as long as MIPI & SMIA modules are part of CamerIc
                (void)CamEngineReleaseCamerIc( pCamEngineCtx );
                return ( result );
            }
        }
    }
    else if ( CAM_ENGINE_MODE_IMAGE_PROCESSING == pConfig->mode )
    {
        result = CamEnginePreloadImage( pCamEngineCtx, pConfig );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_API_ERROR, "%s (loading image failed -> RESULT=%d)\n", __FUNCTION__, result);
            (void)CamEngineReleasePixelIf( pCamEngineCtx ); // must be called before CamEngineReleaseCamerIc as long as MIPI & SMIA modules are part of CamerIc
            (void)CamEngineReleaseCamerIc( pCamEngineCtx );
            return ( result );
        }

        result = CamEngineSetupAcqForDma( pCamEngineCtx, pConfig, pCamEngineCtx->chainIdx0 );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_API_ERROR, "%s (configuring camerIC failed -> RESULT=%d)\n", __FUNCTION__, result);
            (void)CamEngineReleasePixelIf( pCamEngineCtx ); // must be called before CamEngineReleaseCamerIc as long as MIPI & SMIA modules are part of CamerIc
            (void)CamEngineReleaseCamerIc( pCamEngineCtx );
            return ( result );
        }
    }
	
    // get sensor color : 0:W/B 1:Color
	{
		char sensor_color = 0;
		result = IsiGetColorIss(pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor,&sensor_color);
		if(!result) {
			pCamEngineCtx->color_mode = sensor_color;
		} else {
			TRACE( CAM_ENGINE_API_ERROR, "%s (IsiGetColorIss failed -> RESULT=%d)\n", __FUNCTION__, result);
		}
	}

    // setup driver modules
    if ( ( CAM_ENGINE_MODE_SENSOR_2D         == pConfig->mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pConfig->mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_3D         == pConfig->mode ) )
    {
        result = (pConfig->data.sensor.enableTestpattern)
                    ? CamEngineInitDrvForTestpattern( pCamEngineCtx, pConfig )
                    : CamEngineInitDrvForSensor( pCamEngineCtx, pConfig );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_API_ERROR, "%s (setup of driver modules failed -> RESULT=%d)\n", __FUNCTION__, result);
            (void)CamEngineReleasePixelIf( pCamEngineCtx ); // must be called before CamEngineReleaseCamerIc as long as MIPI & SMIA modules are part of CamerIc
            (void)CamEngineReleaseCamerIc( pCamEngineCtx );
            return ( result );
        }
    }
    else if  ( CAM_ENGINE_MODE_IMAGE_PROCESSING == pConfig->mode )
    {
        result = CamEngineInitDrvForDma( pCamEngineCtx, pConfig );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_API_ERROR, "%s (setup of driver modules failed -> RESULT=%d)\n", __FUNCTION__, result);
            (void)CamEngineReleasePixelIf( pCamEngineCtx ); // must be called before CamEngineReleaseCamerIc as long as MIPI & SMIA modules are part of CamerIc
            (void)CamEngineReleaseCamerIc( pCamEngineCtx );
            return ( result );
        }
    }

    // setup memory interface & data path
    result = CamEngineSetupMiDataPath( pCamEngineCtx, &pConfig->pathConfigMaster[CAMERIC_MI_PATH_MAIN], &pConfig->pathConfigMaster[CAMERIC_MI_PATH_SELF], pCamEngineCtx->chainIdx0 );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s (configuring camerIC MI failed -> RESULT=%d)\n", __FUNCTION__, result);
        (void)CamEngineReleaseDrv( pCamEngineCtx );
        (void)CamEngineReleasePixelIf( pCamEngineCtx ); // must be called before CamEngineReleaseCamerIc as long as MIPI & SMIA modules are part of CamerIc
        (void)CamEngineReleaseCamerIc( pCamEngineCtx );
        return ( result );
    }

    if ( ( CAM_ENGINE_MODE_SENSOR_3D         == pCamEngineCtx->mode ) || 
         ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pCamEngineCtx->mode ) )
    {
        result = CamEngineSetupMiDataPath( pCamEngineCtx, &pConfig->pathConfigSlave[CAMERIC_MI_PATH_MAIN], &pConfig->pathConfigSlave[CAMERIC_MI_PATH_SELF], pCamEngineCtx->chainIdx1 );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_API_ERROR, "%s (configuring camerIC MI failed -> RESULT=%d)\n", __FUNCTION__, result);
            (void)CamEngineReleaseDrv( pCamEngineCtx );
            (void)CamEngineReleasePixelIf( pCamEngineCtx ); // must be called before CamEngineReleaseCamerIc as long as MIPI & SMIA modules are part of CamerIc
            (void)CamEngineReleaseCamerIc( pCamEngineCtx );
            return ( result );
        }
    }

    // build start command
    CamEngineCmd_t command;
    MEMSET( &command, 0, sizeof( CamEngineCmd_t ) );
    command.cmdId = CAM_ENGINE_CMD_START;
    
    // ...send command to cam-engine thread via command-queue
    result = CamEngineSendCommand( pCamEngineCtx, &command );
    if ( result != RET_SUCCESS )
    {
        (void)CamEngineReleaseDrv( pCamEngineCtx );
        (void)CamEngineReleasePixelIf( pCamEngineCtx ); // must be called before CamEngineReleaseCamerIc as long as MIPI & SMIA modules are part of CamerIc
        (void)CamEngineReleaseCamerIc( pCamEngineCtx );
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_PENDING );
}


/******************************************************************************
 * CamEngineStop
 *****************************************************************************/
RESULT CamEngineStop
(
    CamEngineHandle_t hCamEngine
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // initial checks
    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    // check cam-engine thread state
    if ( CAM_ENGINE_STATE_RUNNING != CamEngineGetState( pCamEngineCtx ) )
    {
        return ( RET_WRONG_STATE );
    }

    // build stop comand
    CamEngineCmd_t command;
    MEMSET( &command, 0, sizeof( CamEngineCmd_t ) );
    command.cmdId = CAM_ENGINE_CMD_STOP;
    
    // ...send command to cam-engine thread via command-queue
    result = CamEngineSendCommand( pCamEngineCtx, &command );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_PENDING );
}


/******************************************************************************
 * CamEngineStartStreaming
 *****************************************************************************/
RESULT CamEngineStartStreaming
(
    CamEngineHandle_t   hCamEngine,
    uint32_t            frames
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // initial checks
    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    // check cam-engine thread state
    if ( CAM_ENGINE_STATE_RUNNING != CamEngineGetState( pCamEngineCtx ) )
    {
        return ( RET_WRONG_STATE );
    }

    // start cam-engine
    CamEngineCmd_t command;
    MEMSET( &command, 0, sizeof( CamEngineCmd_t ) );
    command.cmdId = CAM_ENGINE_CMD_START_STREAMING;
    command.pCmdCtx = ( uint32_t *)malloc( sizeof(uint32_t) );
    if ( command.pCmdCtx == NULL )
    {
        return ( RET_OUTOFMEM );
    }
    *( uint32_t *)command.pCmdCtx = frames;

    // ...send command to cam-engine thread via command-queue
    result = CamEngineSendCommand( pCamEngineCtx, &command );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_PENDING );
}


/******************************************************************************
 * CamEngineStopStreaming
 *****************************************************************************/
RESULT CamEngineStopStreaming
(
    CamEngineHandle_t   hCamEngine
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // initial checks
    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    // check cam-engine thread state
    if ( CAM_ENGINE_STATE_STREAMING != CamEngineGetState( pCamEngineCtx ) )
    {
        return ( RET_WRONG_STATE );
    }

    // stop cam-engine
    CamEngineCmd_t camCommand;
    MEMSET( &camCommand, 0, sizeof( CamEngineCmd_t ) );
    camCommand.cmdId = CAM_ENGINE_CMD_STOP_STREAMING;
    
    // ...send command to cam-engine thread via command-queue
    result = CamEngineSendCommand( pCamEngineCtx, &camCommand );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_PENDING );
}


/******************************************************************************
 * CamEngineRegisterBufferCb
 *****************************************************************************/
RESULT CamEngineRegisterBufferCb
(
    CamEngineHandle_t   hCamEngine,
    CamEngineBufferCb_t fpCallback,
    void*               pBufferCbCtx
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // initial checks
    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if( NULL == fpCallback )
    {
        return RET_NULL_POINTER;
    }

    // check cam-engine thread state
    if ( CAM_ENGINE_STATE_INVALID == CamEngineGetState( pCamEngineCtx ) )
    {
        return ( RET_WRONG_STATE );
    }

    result = CamEngineSubCtrlsRegisterBufferCb( pCamEngineCtx, fpCallback, pBufferCbCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s (registering buffer callback failed)\n", __FUNCTION__ );
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineDeRegisterBufferCb
 *****************************************************************************/
RESULT CamEngineDeRegisterBufferCb
(
    CamEngineHandle_t   hCamEngine
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // initial checks
    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    // check cam-engine thread state
    if ( CAM_ENGINE_STATE_INVALID == CamEngineGetState( pCamEngineCtx ) )
    {
        return ( RET_WRONG_STATE );
    }

    result = CamEngineSubCtrlsDeRegisterBufferCb( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s (unregistering buffer callback failed)\n", __FUNCTION__ );
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineGetBufferCb
 *****************************************************************************/
RESULT CamEngineGetBufferCb
(
    CamEngineHandle_t    hCamEngine,
    CamEngineBufferCb_t* fpCallback,
    void**               ppBufferCbCtx
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // initial checks
    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    // check cam-engine thread state
    if ( CAM_ENGINE_STATE_INVALID == CamEngineGetState( pCamEngineCtx ) )
    {
        return ( RET_WRONG_STATE );
    }

    *fpCallback    = pCamEngineCtx->cbBuffer;
    *ppBufferCbCtx = pCamEngineCtx->pBufferCbCtx;

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineGetState
 *****************************************************************************/
CamEngineState_t CamEngineGetState
(
    CamEngineHandle_t   hCamEngine
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // initial checks
    if( NULL == pCamEngineCtx )
    {
        return ( CAM_ENGINE_STATE_INVALID );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( pCamEngineCtx->state );
}


/******************************************************************************
 * CamEngineGetMode
 *****************************************************************************/
CamEngineModeType_t CamEngineGetMode
(
    CamEngineHandle_t   hCamEngine
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // initial checks 
    if( NULL == pCamEngineCtx )
    {
        return ( CAM_ENGINE_STATE_INVALID );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( pCamEngineCtx->mode );
}


/******************************************************************************
 * CamEngineSetPathConfig
 *****************************************************************************/
RESULT CamEngineSetPathConfig
(
    CamEngineHandle_t               hCamEngine,
    const CamEngineChainIdx_t		chainIdx,
    const CamEnginePathConfig_t     *pConfigMain,
    const CamEnginePathConfig_t     *pConfigSelf
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // initial checks 
    if( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ( chainIdx <= CHAIN_INVALID ) || ( chainIdx >= CHAIN_MAX ) )
    {
        return ( RET_INVALID_PARM );
    }

    /* check CamEngine thread state */
    if ( ( CAM_ENGINE_STATE_INITIALIZED != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_RUNNING     != pCamEngineCtx->state) )
    {
        return ( RET_WRONG_STATE );
    }

    /* setup memory interface & data path */
    result = CamEngineSetupMiDataPath( pCamEngineCtx, pConfigMain, pConfigSelf, chainIdx );
    if ( result != RET_SUCCESS )
    {
         TRACE( CAM_ENGINE_API_ERROR, "%s (configuring path failed)\n", __FUNCTION__);
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineSetAcqResolution
 *****************************************************************************/
RESULT CamEngineSetAcqResolution
(
    CamEngineHandle_t   hCamEngine,
    CamEngineWindow_t   acqWindow,          // isp input acq.
    CamEngineWindow_t   outWindow,          // isp output formatter
    CamEngineWindow_t   isWindow,           // isp image stabilization
    CamEngineWindow_t   isCroppingWindow,   // isp iamge stabilization for 2nd pipt in case of vsm
    uint32_t            numFramesToSkip
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // initial checks 
    if( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check CamEngine thread state */
    if ( CAM_ENGINE_STATE_RUNNING != pCamEngineCtx->state)
    {
        return ( RET_WRONG_STATE );
    }

    /* size */
    pCamEngineCtx->acqWindow = acqWindow;
    pCamEngineCtx->outWindow = outWindow;

    pCamEngineCtx->chain[CHAIN_MASTER].isWindow = isCroppingWindow;
    pCamEngineCtx->chain[CHAIN_SLAVE].isWindow  = isWindow;

    /* setup isp input acquisition resolution */
    result = CamEngineSetupAcqResolution( pCamEngineCtx, pCamEngineCtx->chainIdx0 );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s (configuring acquisition resolution failed)\n", __FUNCTION__);
        return ( result );
    }
    /* setup memory interface + scaler */
    result = CamEngineSetupMiResolution( pCamEngineCtx, pCamEngineCtx->chainIdx0 );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s: (configuring resolution on CamerIc MI module failed)\n", __FUNCTION__ );
        return ( result );
    }

    if ( ( CAM_ENGINE_MODE_SENSOR_3D         == pCamEngineCtx->mode ) || 
         ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pCamEngineCtx->mode ) )
    {
        /* setup isp input acquisition resolution */
        result = CamEngineSetupAcqResolution( pCamEngineCtx, pCamEngineCtx->chainIdx1 );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR, "%s (configuring acquisition resolution failed)\n", __FUNCTION__);
            return ( result );
        }
        /* setup memory interface + scaler */
        result = CamEngineSetupMiResolution( pCamEngineCtx, pCamEngineCtx->chainIdx1 );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR, "%s: (configuring resolution on CamerIc MI module failed)\n", __FUNCTION__ );
            return ( result );
        }
    }

    /* setup calibration database access */
    if ( NULL != pCamEngineCtx->hCamCalibDb )
    {
        result = CamEnginePrepareCalibDbAccess( pCamEngineCtx );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_API_ERROR, "%s (preparing calibration database failed)\n", __FUNCTION__);
            return ( result );
        }
    }

    /* reconfigure modules */
    uint32_t numFramesToSkipModules = 0;
    
#if 1 // soc camera test
    if(!pCamEngineCtx->isSOCsensor){
        result = CamEngineModulesReConfigure( pCamEngineCtx, &numFramesToSkipModules );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_API_ERROR, "%s (reconfiguration of modules failed)\n", __FUNCTION__ );
            return ( result );
        }
    }

    #endif
    numFramesToSkip = MAX( numFramesToSkip, numFramesToSkipModules );

    /* re-init driver modules */
    result = CamEngineReInitDrv( pCamEngineCtx, numFramesToSkip );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s (re-initialization of driver modules failed)\n", __FUNCTION__ );
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineSetEcm
 *****************************************************************************/
RESULT CamEngineSetEcm
(
    CamEngineHandle_t           hCamEngine,
    CamEngineFlickerPeriod_t    flickerPeriod,
    bool_t                      enableAfps
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // initial checks 
    if( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check CamEngine thread state */
    if ( CAM_ENGINE_STATE_RUNNING != pCamEngineCtx->state)
    {
        return ( RET_WRONG_STATE );
    }

    /* check parm */
    switch( flickerPeriod )
    {
        case CAM_ENGINE_FLICKER_OFF  :
        case CAM_ENGINE_FLICKER_100HZ:
        case CAM_ENGINE_FLICKER_120HZ:
            break;

        default:
            return ( RET_INVALID_PARM );
    }

    /* update context */
    pCamEngineCtx->flickerPeriod = flickerPeriod;
    pCamEngineCtx->enableAfps    = enableAfps;

    /* reconfigure modules */
    uint32_t numFramesToSkipModules = 0;
    result = CamEngineModulesReConfigure( pCamEngineCtx, &numFramesToSkipModules );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s (reconfiguration of modules failed)\n", __FUNCTION__ );
        return ( result );
    }
    numFramesToSkipModules = MAX( 1, numFramesToSkipModules );

    /* re-init driver modules */
    result = CamEngineReInitDrv( pCamEngineCtx, numFramesToSkipModules );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s (re-initialization of driver modules failed)\n", __FUNCTION__ );
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineSetCalibDb
 *****************************************************************************/
RESULT CamEngineSetCalibDb
(
    CamEngineHandle_t   hCamEngine,
    CamCalibDbHandle_t  hCamCalibDb
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // initial checks 
    if( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check CamEngine thread state */
    if ( CAM_ENGINE_STATE_RUNNING    != pCamEngineCtx->state)
    {
        return ( RET_WRONG_STATE );
    }


    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamEngineSearchAndLock
 *****************************************************************************/
RESULT CamEngineSearchAndLock
(
    CamEngineHandle_t   hCamEngine,
    CamEngineLockType_t locks
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // initial checks 
    if( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check cam-engine thread state */
    if ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state )
    {
        return ( RET_WRONG_STATE );
    }

    if ( CAM_ENGINE_MODE_IMAGE_PROCESSING == pCamEngineCtx->mode )
    {
        return ( RET_NOTSUPP );
    }

    /* command */
    CamEngineCmd_t command;
    MEMSET( &command, 0, sizeof( CamEngineCmd_t ) );
    command.cmdId = CAM_ENGINE_CMD_ACQUIRE_LOCK;
    command.pCmdCtx = ( CamEngineLockType_t *)malloc( sizeof(CamEngineLockType_t) );
    if ( command.pCmdCtx == NULL )
    {
        return ( RET_OUTOFMEM );
    }
    *( CamEngineLockType_t *)command.pCmdCtx = locks;
    /* ...send command to cam-engine thread via command-queue */
    result = CamEngineSendCommand( pCamEngineCtx, &command );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_PENDING );
}


/******************************************************************************
 * CamEngineUnlock
 *****************************************************************************/
RESULT CamEngineUnlock
(
    CamEngineHandle_t   hCamEngine,
    CamEngineLockType_t locks
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // initial checks 
    if( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check cam-engine thread state */
    if ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state )
    {
        return ( RET_WRONG_STATE );
    }

    if ( CAM_ENGINE_MODE_IMAGE_PROCESSING == pCamEngineCtx->mode )
    {
        return ( RET_NOTSUPP );
    }

    /* command */
    CamEngineCmd_t command;
    MEMSET( &command, 0, sizeof( CamEngineCmd_t ) );
    command.cmdId = CAM_ENGINE_CMD_RELEASE_LOCK;
    command.pCmdCtx = ( CamEngineLockType_t *)malloc( sizeof(CamEngineLockType_t) );
    if ( command.pCmdCtx == NULL )
    {
        return ( RET_OUTOFMEM );
    }
    *( CamEngineLockType_t *)command.pCmdCtx = locks;
    /* ...send command to cam-engine thread via command-queue */
    result = CamEngineSendCommand( pCamEngineCtx, &command );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_PENDING );
}

void CamEngineSetIsSocSensor
(
    CamEngineHandle_t   hCamEngine,
    bool isSoc
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;


    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // initial checks 
    if( NULL == pCamEngineCtx )
    {
        return ;
    }
    pCamEngineCtx->isSOCsensor = isSoc;
}


/******************************************************************************
 * CamEngineStartPixelIf
 *****************************************************************************/
RESULT CamEngineStartPixelIfApi
(
    CamEngineHandle_t  hCamEngine,
    CamEngineConfig_t   *pConfig
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;


    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // initial checks 
    if( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }
    return CamEngineStartPixelIf(pCamEngineCtx,pConfig);
}


/******************************************************************************
 * CamEngineConfigureFlash
 *****************************************************************************/
RESULT CamEngineConfigureFlash
(
    CamEngineHandle_t  hCamEngine,
    CamerIcIspFlashCfg_t *cfgFsh
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    // initial checks 
    if( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }
    
    return CamerIcIspFlashConfigure(pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc,cfgFsh);

}

/******************************************************************************
 * CamEngineStartFlash
 *****************************************************************************/
RESULT CamEngineStartFlash
(
    CamEngineHandle_t  hCamEngine,
    bool_t operate_now
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    // initial checks 
    if( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    return CamerIcIspFlashStart(pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc,operate_now);
}

/******************************************************************************
 * CamEngineStopFlash
 *****************************************************************************/
RESULT CamEngineStopFlash
(
    CamEngineHandle_t  hCamEngine,
    bool_t operate_now
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    // initial checks 
    if( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    return CamerIcIspFlashStop(pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc,operate_now);

}

RESULT CamEngineSetAecPoint
(
    CamEngineHandle_t  hCamEngine,
    float point
)
{
	CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;
	   if( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }
	   
	return CamEngineModulesSetAecPoint(pCamEngineCtx, point);
}

void CamEngineSetBufferSize
(
    CamEngineHandle_t   hCamEngine,
    uint32_t bufNum,
    uint32_t bufSize
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;


    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // initial checks 
    if( NULL == pCamEngineCtx )
    {
        return ;
    }
    pCamEngineCtx->bufNum = bufNum;
    pCamEngineCtx->bufSize = bufSize;
}

