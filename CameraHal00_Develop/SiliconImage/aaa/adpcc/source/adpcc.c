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
 * @file adpf.c
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
#include <common/utl_fixfloat.h>

#include <cam_calibdb/cam_calibdb_api.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_dpcc_drv_api.h>

#include "adpcc.h"
#include "adpcc_ctrl.h"



/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( ADPCC_INFO  , "ADPCC: ", INFO     , 0 );
CREATE_TRACER( ADPCC_WARN  , "ADPCC: ", WARNING  , 1 );
CREATE_TRACER( ADPCC_ERROR , "ADPCC: ", ERROR    , 1 );

CREATE_TRACER( ADPCC_DEBUG , "ADPCC: ", INFO     , 0 );



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
 * @brief   This local function prepares a resolution access identifier for
 *          calibration database access.
 *
 * @param   pAdpccCtx       adpcc context
 * @param   hCamCalibDb
 * @param   width
 * @param   height
 * @param   framerate

 * @return                  Return the result of the function call.
 * @retval  RET_SUCCESS     function succeed
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
static RESULT AdpccPrepareCalibDbAccess
(
    AdpccContext_t              *pAdpccCtx,
    const CamCalibDbHandle_t    hCamCalibDb,
    const uint16_t              width,
    const uint16_t              height,
    const uint16_t              framerate
)
{
    RESULT result = RET_SUCCESS;

    TRACE( ADPCC_INFO, "%s: (enter)\n", __FUNCTION__);

    result = CamCalibDbGetResolutionNameByWidthHeight( hCamCalibDb, width, height, &pAdpccCtx->ResName );
    if ( RET_SUCCESS != result )
    {
        TRACE( ADPCC_ERROR, "%s: resolution (%dx%d@%d) not found in database\n", __FUNCTION__, width, height, framerate );
        return ( result );
    }
    TRACE( ADPCC_INFO, "%s: resolution = %s\n", __FUNCTION__, pAdpccCtx->ResName );

    pAdpccCtx->hCamCalibDb = hCamCalibDb;

    TRACE( ADPCC_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * AdpccApplyConfiguration()
 *****************************************************************************/
static RESULT AdpccApplyConfiguration
(
    AdpccContext_t   *pAdpccCtx,
    AdpccConfig_t    *pConfig
)
{
    RESULT result = RET_SUCCESS;

    CamerIcDpccStaticConfig_t DpccConfig;

    TRACE( ADPCC_INFO, "%s: (enter)\n", __FUNCTION__);

    // clear
    MEMSET( &DpccConfig, 0, sizeof( DpccConfig ) );
    
    pAdpccCtx->hCamerIc     = pConfig->hCamerIc;
    pAdpccCtx->hSubCamerIc  = pConfig->hSubCamerIc;

    // configuration with data from calibration database
    if ( pConfig->type == ADPCC_USE_CALIB_DATABASE )
    {
        CamDpccProfile_t *pDpccProfile = NULL;

        // initialize calibration database access
        result = AdpccPrepareCalibDbAccess( pAdpccCtx,
                                            pConfig->data.db.hCamCalibDb,
                                            pConfig->data.db.width,
                                            pConfig->data.db.height,
                                            pConfig->data.db.framerate );
        if ( result != RET_SUCCESS )
        {
            TRACE( ADPCC_ERROR, "%s: Can't prepare database access\n",  __FUNCTION__ );
            return ( result );
        }

        // get dpf-profile from calibration database
        result = CamCalibDbGetDpccProfileByResolution( pAdpccCtx->hCamCalibDb, pAdpccCtx->ResName, &pDpccProfile );
        if ( result != RET_SUCCESS )
        {
            TRACE( ADPCC_ERROR, "%s: Getting DPCC profile for resolution %s from calibration database failed (%d)\n",
                                    __FUNCTION__, pAdpccCtx->ResName, result );
            return ( result );
        }
        DCT_ASSERT( NULL != pDpccProfile );

        DpccConfig.isp_dpcc_mode            = pDpccProfile->isp_dpcc_mode;
        DpccConfig.isp_dpcc_output_mode     = pDpccProfile->isp_dpcc_output_mode;
        DpccConfig.isp_dpcc_set_use         = pDpccProfile->isp_dpcc_set_use;

        DpccConfig.isp_dpcc_methods_set_1   = pDpccProfile->isp_dpcc_methods_set_1;
        DpccConfig.isp_dpcc_methods_set_2   = pDpccProfile->isp_dpcc_methods_set_2;
        DpccConfig.isp_dpcc_methods_set_3   = pDpccProfile->isp_dpcc_methods_set_3;

        DpccConfig.isp_dpcc_line_thresh_1   = pDpccProfile->isp_dpcc_line_thresh_1;
        DpccConfig.isp_dpcc_line_mad_fac_1  = pDpccProfile->isp_dpcc_line_mad_fac_1;
        DpccConfig.isp_dpcc_pg_fac_1        = pDpccProfile->isp_dpcc_pg_fac_1;
        DpccConfig.isp_dpcc_rnd_thresh_1    = pDpccProfile->isp_dpcc_rnd_thresh_1;
        DpccConfig.isp_dpcc_rg_fac_1        = pDpccProfile->isp_dpcc_rg_fac_1;

        DpccConfig.isp_dpcc_line_thresh_2   = pDpccProfile->isp_dpcc_line_thresh_2;
        DpccConfig.isp_dpcc_line_mad_fac_2  = pDpccProfile->isp_dpcc_line_mad_fac_2;
        DpccConfig.isp_dpcc_pg_fac_2        = pDpccProfile->isp_dpcc_pg_fac_2;
        DpccConfig.isp_dpcc_rnd_thresh_2    = pDpccProfile->isp_dpcc_rnd_thresh_2;
        DpccConfig.isp_dpcc_rg_fac_2        = pDpccProfile->isp_dpcc_rg_fac_2;

        DpccConfig.isp_dpcc_line_thresh_3   = pDpccProfile->isp_dpcc_line_thresh_3;
        DpccConfig.isp_dpcc_line_mad_fac_3  = pDpccProfile->isp_dpcc_line_mad_fac_3;
        DpccConfig.isp_dpcc_pg_fac_3        = pDpccProfile->isp_dpcc_pg_fac_3;
        DpccConfig.isp_dpcc_rnd_thresh_3    = pDpccProfile->isp_dpcc_rnd_thresh_3;
        DpccConfig.isp_dpcc_rg_fac_3        = pDpccProfile->isp_dpcc_rg_fac_3;

        DpccConfig.isp_dpcc_ro_limits       = pDpccProfile->isp_dpcc_ro_limits;
        DpccConfig.isp_dpcc_rnd_offs        = pDpccProfile->isp_dpcc_rnd_offs;
    }
    // configuration with default configuration data
    else if ( pConfig->type == ADPCC_USE_DEFAULT_CONFIG )
    {
        DpccConfig.isp_dpcc_mode            = pConfig->data.def.isp_dpcc_mode;
        DpccConfig.isp_dpcc_output_mode     = pConfig->data.def.isp_dpcc_output_mode;
        DpccConfig.isp_dpcc_set_use         = pConfig->data.def.isp_dpcc_set_use;

        DpccConfig.isp_dpcc_methods_set_1   = pConfig->data.def.isp_dpcc_methods_set_1;
        DpccConfig.isp_dpcc_methods_set_2   = pConfig->data.def.isp_dpcc_methods_set_2;
        DpccConfig.isp_dpcc_methods_set_3   = pConfig->data.def.isp_dpcc_methods_set_3;

        DpccConfig.isp_dpcc_line_thresh_1   = pConfig->data.def.isp_dpcc_line_thresh_1;
        DpccConfig.isp_dpcc_line_mad_fac_1  = pConfig->data.def.isp_dpcc_line_mad_fac_1;
        DpccConfig.isp_dpcc_pg_fac_1        = pConfig->data.def.isp_dpcc_pg_fac_1;
        DpccConfig.isp_dpcc_rnd_thresh_1    = pConfig->data.def.isp_dpcc_rnd_thresh_1;
        DpccConfig.isp_dpcc_rg_fac_1        = pConfig->data.def.isp_dpcc_rg_fac_1;

        DpccConfig.isp_dpcc_line_thresh_2   = pConfig->data.def.isp_dpcc_line_thresh_2;
        DpccConfig.isp_dpcc_line_mad_fac_2  = pConfig->data.def.isp_dpcc_line_mad_fac_2;
        DpccConfig.isp_dpcc_pg_fac_2        = pConfig->data.def.isp_dpcc_pg_fac_2;
        DpccConfig.isp_dpcc_rnd_thresh_2    = pConfig->data.def.isp_dpcc_rnd_thresh_2;
        DpccConfig.isp_dpcc_rg_fac_2        = pConfig->data.def.isp_dpcc_rg_fac_2;

        DpccConfig.isp_dpcc_line_thresh_3   = pConfig->data.def.isp_dpcc_line_thresh_3;
        DpccConfig.isp_dpcc_line_mad_fac_3  = pConfig->data.def.isp_dpcc_line_mad_fac_3;
        DpccConfig.isp_dpcc_pg_fac_3        = pConfig->data.def.isp_dpcc_pg_fac_3;
        DpccConfig.isp_dpcc_rnd_thresh_3    = pConfig->data.def.isp_dpcc_rnd_thresh_3;
        DpccConfig.isp_dpcc_rg_fac_3        = pConfig->data.def.isp_dpcc_rg_fac_3;

        DpccConfig.isp_dpcc_ro_limits       = pConfig->data.def.isp_dpcc_ro_limits;
        DpccConfig.isp_dpcc_rnd_offs        = pConfig->data.def.isp_dpcc_rnd_offs;
    }
    else
    {
        TRACE( ADPCC_ERROR,  "%s: unsupported ADPCC configuration\n",  __FUNCTION__ );
        return ( RET_OUTOFRANGE );
    }

    // apply config to cameric dpcc driver
    result = CamerIcIspDpccSetStaticConfig( pAdpccCtx->hCamerIc, &DpccConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( ADPCC_ERROR, "%s: CamerIc Driver DPCC Configuration failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }
    if ( NULL != pAdpccCtx->hSubCamerIc )
    {
        result = CamerIcIspDpccSetStaticConfig( pAdpccCtx->hSubCamerIc, &DpccConfig );
        if ( result != RET_SUCCESS )
        {
            TRACE( ADPCC_ERROR,  "%s: 2nd CamerIc Driver DPCC Configuration failed (%d)\n",  __FUNCTION__, result );
            return ( result );
        }
    }

    // save configuration
    pAdpccCtx->Config = *pConfig;

    TRACE( ADPCC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * Implementation of ADPCC API Functions
 *****************************************************************************/

/******************************************************************************
 * AdpccInit()
 *****************************************************************************/
RESULT AdpccInit
(
    AdpccInstanceConfig_t *pInstConfig
)
{
    RESULT result = RET_SUCCESS;

    AdpccContext_t *pAdpccCtx;

    TRACE( ADPCC_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pInstConfig == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    /* allocate auto exposure control context */
    pAdpccCtx = ( AdpccContext_t *)malloc( sizeof(AdpccContext_t) );
    if ( pAdpccCtx == NULL )
    {
        TRACE( ADPCC_ERROR,  "%s: Can't allocate ADCC context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }

    /* pre-initialize context */
    MEMSET( pAdpccCtx, 0, sizeof(*pAdpccCtx) );
    pAdpccCtx->state        = ADPCC_STATE_INITIALIZED;

    /* return handle */
    pInstConfig->hAdpcc = (AdpccHandle_t)pAdpccCtx;

    TRACE( ADPCC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AdpccRelease()
 *****************************************************************************/
RESULT AdpccRelease
(
    AdpccHandle_t handle
)
{
    AdpccContext_t *pAdpccCtx = (AdpccContext_t *)handle;

    TRACE( ADPCC_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pAdpccCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check state */
    if ( (ADPCC_STATE_RUNNING == pAdpccCtx->state)
            || (ADPCC_STATE_LOCKED == pAdpccCtx->state) )
    {
        return ( RET_BUSY );
    }

    MEMSET( pAdpccCtx, 0, sizeof(AdpccContext_t) );
    free ( pAdpccCtx );

    TRACE( ADPCC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AdpccConfigure()
 *****************************************************************************/
RESULT AdpccConfigure
(
    AdpccHandle_t handle,
    AdpccConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    AdpccContext_t *pAdpccCtx = (AdpccContext_t*)handle;

    TRACE( ADPCC_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAdpccCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pConfig )
    {
        return ( RET_INVALID_PARM );
    }

    if ( (ADPCC_STATE_INITIALIZED != pAdpccCtx->state)
            && (ADPCC_STATE_STOPPED != pAdpccCtx->state) )
    {
        return ( RET_WRONG_STATE );
    }

    // apply config
    result = AdpccApplyConfiguration( pAdpccCtx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( ADPCC_ERROR,  "%s: Can't configure CamerIc DPCC (%d)\n",  __FUNCTION__, result );
        return ( result );
    }

    TRACE( ADPCC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AdpccReConfigure()
 *****************************************************************************/
RESULT AdpccReConfigure
(
    AdpccHandle_t handle,
    AdpccConfig_t *pConfig
)
{
    AdpccContext_t *pAdpccCtx = (AdpccContext_t*)handle;

    RESULT result = RET_SUCCESS;

    TRACE( ADPCC_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAdpccCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pConfig )
    {
        return ( RET_INVALID_PARM );
    }

    if ( (ADPCC_STATE_LOCKED == pAdpccCtx->state)
            || (ADPCC_STATE_RUNNING == pAdpccCtx->state) )
    {
        // check if we switched the resolution
        if ( ( ADPCC_USE_CALIB_DATABASE == pConfig->type )
                && ((pConfig->data.db.width != pAdpccCtx->Config.data.db.width)
                        || (pConfig->data.db.height != pAdpccCtx->Config.data.db.height)
                        || (pConfig->data.db.hCamCalibDb != pAdpccCtx->Config.data.db.hCamCalibDb)) )
        {
            // disable cameric driver instance(s)
            result = CamerIcIspDpccDisable( pAdpccCtx->hCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( ADPCC_ERROR,  "%s: Can't disable CamerIc DPCC (%d)\n",  __FUNCTION__, result );
                return ( result );
            }
            if ( NULL != pAdpccCtx->hSubCamerIc )
            {
                result = CamerIcIspDpccDisable( pAdpccCtx->hSubCamerIc );
                if ( result != RET_SUCCESS )
                {
                    TRACE( ADPCC_ERROR,  "%s: Can't disable 2nd CamerIc DPCC (%d)\n",  __FUNCTION__, result );
                    return ( result );
                }
            }

            // apply config
            result = AdpccApplyConfiguration( pAdpccCtx, pConfig );
            if ( result != RET_SUCCESS )
            {
                TRACE( ADPCC_ERROR,  "%s: Can't reconfigure CamerIc DPCC (%d)\n",  __FUNCTION__, result );
                return ( result );
            }

            // enable cameric driver instance(s)
            result = CamerIcIspDpccEnable( pAdpccCtx->hCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( ADPCC_ERROR,  "%s: Can't enable CamerIc DPCC (%d)\n",  __FUNCTION__, result );
                return ( result );
            }
            if ( NULL != pAdpccCtx->hSubCamerIc )
            {
                result = CamerIcIspDpccEnable( pAdpccCtx->hSubCamerIc );
                if ( result != RET_SUCCESS )
                {
                    TRACE( ADPCC_ERROR,  "%s: Can't enable 2nd CamerIc DPCC (%d)\n",  __FUNCTION__, result );
                    return ( result );
                }
            }

            // save configuration
            pAdpccCtx->Config = *pConfig;
        }

        result = RET_SUCCESS;
    }
    else if ( ADPCC_STATE_STOPPED == pAdpccCtx->state )
    {
        result = RET_SUCCESS;
    }
    else
    {
        result = RET_WRONG_STATE;
    }

    TRACE( ADPCC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AdpccStart()
 *****************************************************************************/
RESULT AdpccStart
(
    AdpccHandle_t handle
)
{
    AdpccContext_t *pAdpccCtx = (AdpccContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( ADPCC_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAdpccCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (ADPCC_STATE_RUNNING == pAdpccCtx->state)
            || (ADPCC_STATE_LOCKED == pAdpccCtx->state) )
    {
        return ( RET_WRONG_STATE );
    }

    /* enable cameric driver instance(s) */
    result = CamerIcIspDpccEnable( pAdpccCtx->hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( ADPCC_ERROR,  "%s: Can't enable CamerIc DPCC (%d)\n",  __FUNCTION__, result );
        return ( result );
    }
    if ( NULL != pAdpccCtx->hSubCamerIc )
    {
        result = CamerIcIspDpccEnable( pAdpccCtx->hSubCamerIc );
        if ( result != RET_SUCCESS )
        {
            TRACE( ADPCC_ERROR,  "%s: Can't enable 2nd CamerIc DPCC (%d)\n",  __FUNCTION__, result );
            return ( result );
        }
    }

    pAdpccCtx->state = ADPCC_STATE_RUNNING;

    TRACE( ADPCC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AdpccStop()
 *****************************************************************************/
RESULT AdpccStop
(
    AdpccHandle_t handle
)
{
    AdpccContext_t *pAdpccCtx = (AdpccContext_t *)handle;

    RESULT result;

    TRACE( ADPCC_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pAdpccCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* before stopping, unlock the ADPCC if locked */
    if ( ADPCC_STATE_LOCKED == pAdpccCtx->state )
    {
        result = RET_BUSY;
    }
    else if ( ADPCC_STATE_RUNNING == pAdpccCtx->state )
    {
        /* disable cameric driver instance(s) */
        result = CamerIcIspDpccDisable( pAdpccCtx->hCamerIc );
        if ( result != RET_SUCCESS )
        {
            TRACE( ADPCC_ERROR,  "%s: Can't disable CamerIc DPCC (%d)\n",  __FUNCTION__, result );
            return ( result );
        }
        if ( NULL != pAdpccCtx->hSubCamerIc )
        {
            result = CamerIcIspDpccDisable( pAdpccCtx->hSubCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( ADPCC_ERROR,  "%s: Can't disable 2nd CamerIc DPCC (%d)\n",  __FUNCTION__, result );
                return ( result );
            }
        }

        pAdpccCtx->state = ADPCC_STATE_STOPPED;
    }
    else if ( (ADPCC_STATE_STOPPED == pAdpccCtx->state)     ||
              (ADPCC_STATE_INITIALIZED == pAdpccCtx->state) )

    {
        result = RET_SUCCESS;
    }
    else
    {
        result = RET_WRONG_STATE;
    }

    TRACE( ADPCC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AdpccStatus()
 *****************************************************************************/
RESULT AdpccStatus
(
    AdpccHandle_t   handle,
    bool_t          *pRunning
)
{
    AdpccContext_t *pAdpccCtx = (AdpccContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( ADPCC_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAdpccCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pRunning == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    *pRunning = ( (pAdpccCtx->state == ADPCC_STATE_RUNNING) ||
                  (pAdpccCtx->state == ADPCC_STATE_LOCKED)  ) ? BOOL_TRUE : BOOL_FALSE;

    TRACE( ADPCC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AdpccProcessFrame()
 *****************************************************************************/
RESULT AdpccProcessFrame
(
    AdpccHandle_t   handle,
    const float     gain
)
{
    AdpccContext_t *pAdpccCtx = (AdpccContext_t *)handle;

    RESULT result = RET_SUCCESS;

    float dgain = 0.0f; /* gain difference */

    TRACE( ADPCC_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAdpccCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ADPCC_STATE_RUNNING == pAdpccCtx->state )
    {
        dgain = ( gain > pAdpccCtx->gain ) ? (gain - pAdpccCtx->gain) : ( pAdpccCtx->gain -gain);
        if ( dgain > 0.15f )
        {
#if 0
            if ( gain <= 2.0f )
            {
                CamerIcIspDpccSetStaticDemoConfigGain12( pAdpccCtx->hCamerIc );

            }
            else if ( gain <= 4.0f )
            {
                CamerIcIspDpccSetStaticDemoConfigGain24( pAdpccCtx->hCamerIc );
            }
            else
            {
                CamerIcIspDpccSetStaticDemoConfigGain48( pAdpccCtx->hCamerIc );
            }
#endif

            pAdpccCtx->gain = gain;
        }
        else
        {
            result = RET_CANCELED;
        }
    }
    else
    {
        result = RET_CANCELED ;
    }

    TRACE( ADPCC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

