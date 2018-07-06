
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
 * @file awb.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>
#include <common/utl_fixfloat.h>

#include <cam_calibdb/cam_calibdb_api.h>

#include "awb.h"
#include "awb_ctrl.h"

#include "expprior.h"
#include "wprevert.h"
#include "wbgain.h"
#include "illuest.h"
#include "wpregionadapt.h"
#include "acc.h"
#include "alsc.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( AWB_INFO  , "AWB: ", INFO     , 0 );
CREATE_TRACER( AWB_NOTICE0  , "AWB: ", TRACE_NOTICE0  , 1 );
CREATE_TRACER( AWB_NOTICE1  , "AWB: ", TRACE_NOTICE1  , 1 );
CREATE_TRACER( AWB_WARN  , "AWB: ", WARNING  , 1 );
CREATE_TRACER( AWB_ERROR , "AWB: ", ERROR    , 1 );

CREATE_TRACER( AWB_DEBUG , ""     , INFO     , 0 );



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
 * @brief   This local function prepares the AWB working range (min- and max
 *          values of white pixel regarding 5MP resolution)
 *
 * @param   pAwbCtx         awb context
 * @param   width           picture width
 * @param   height          picture height
 *
 *****************************************************************************/
static inline void AwbPrepareWorkingRange
(
    AwbContext_t    *pAwbCtx,
    const uint16_t  width,
    const uint16_t  height
)
{
    float div =  ( 2592.0f  * 1944.0f )  / ( width * height );
    pAwbCtx->NumWhitePixelsMin          = (uint32_t)( 600000.0f / div );
    pAwbCtx->NumWhitePixelsMax          = (uint32_t)( 800000.0f / div );
}



/*****************************************************************************/
/**
 * @brief   This local function prepares a resolution access identifier for
 *          calibration database access.
 *
 * @param   pAwbCtx         awb context
 * @param   hCamCalibDb
 * @param   width
 * @param   height
 * @param   framerate

 * @return                  Return the result of the function call.
 * @retval  RET_SUCCESS     function succeed
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
static RESULT AwbPrepareCalibDbAccess
(
    AwbContext_t                *pAwbCtx,
    const CamCalibDbHandle_t    hCamCalibDb,
    const uint16_t              width,
    const uint16_t              height,
    const float                 framerate
)
{
    RESULT result = RET_SUCCESS;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    result = CamCalibDbGetResolutionNameByWidthHeight( hCamCalibDb, width, height, &pAwbCtx->ResName );
    if ( RET_SUCCESS != result )
    {
        TRACE( AWB_ERROR, "%s: resolution (%dx%d@%d) not found in database\n", __FUNCTION__, width, height, framerate );
        return ( result );
    }

    result = CamCalibDbGetResolutionIdxByName( hCamCalibDb, pAwbCtx->ResName, &pAwbCtx->ResIdx );
    DCT_ASSERT( pAwbCtx->ResIdx < CAM_NO_RESOLUTIONS );

    TRACE( AWB_INFO, "%s: resolution(%d) = %s\n", __FUNCTION__, pAwbCtx->ResIdx, pAwbCtx->ResName );

    pAwbCtx->hCamCalibDb = hCamCalibDb;

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}



/*****************************************************************************/
/**
 * @brief   This local function sort the Color Correction Profiles by
 *          saturation in descending order.
 *
 * @param   pCcProfiles     pointer array of CC-Profiles
 * @param   cnt             number of CC-Profiles in pointer array

 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
static RESULT AwbOrderCcProfilesBySaturation
(
    CamCcProfile_t  *pCcProfiles[],
    const int32_t   cnt
)
{
    int32_t i, j;

    for ( i=0; i<(cnt-1); ++i )
    {
        for ( j=0; j<(cnt-i-1); ++j)
        {
            if ( pCcProfiles[j]->saturation < pCcProfiles[j+1]->saturation )
            {
                CamCcProfile_t  *temp   = pCcProfiles[j];
                pCcProfiles[j]          = pCcProfiles[j+1];
                pCcProfiles[j+1]        = temp;
            }
        }
    }

    return ( RET_SUCCESS );
}



/*****************************************************************************/
/**
 * @brief   This local function sort the Lense Shade Correction Profiles by
 *          vignetting level in descending order.
 *
 * @param   pLscProfiles    pointer array of LSC-Profiles
 * @param   cnt             number of LSC-Profiles in pointer array

 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
static RESULT AwbOrderLscProfilesByVignetting
(
    CamLscProfile_t *pLscProfiles[],
    const int32_t   cnt
)
{
    int32_t i, j;

    for ( i=0; i<(cnt-1); ++i )
    {
        for ( j=0; j<(cnt-i-1); ++j)
        {
            if ( pLscProfiles[j]->vignetting < pLscProfiles[j+1]->vignetting )
            {
                CamLscProfile_t *temp   = pLscProfiles[j];
                pLscProfiles[j]         = pLscProfiles[j+1];
                pLscProfiles[j+1]       = temp;
            }
        }
    }

    return ( RET_SUCCESS );
}



/*****************************************************************************/
/**
 * @brief   This local function recalculates the gradient values for x- and
 *          y-direction of the sector grid items by the following formula:
 *
 *          Y_grad = 2^Lsc_Yo / Y_size
 *
 * @param   pLscProfiles    pointer array of LSC-Profiles
 * @param   cnt             number of LSC-Profiles in pointer array

 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
static RESULT AwbLscGradientCheck
(
    CamLscProfile_t *pLscProfile
)
{
    uint32_t i;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( NULL == pLscProfile )
    {
        return ( RET_INVALID_PARM );
    }

    if ( 0 < pLscProfile->LscYo )
    {
        for (i = 0; i<( sizeof(pLscProfile->LscYGradTbl)/ sizeof(pLscProfile->LscYGradTbl[0]) ); ++i )
        {
            if ( 0 < pLscProfile->LscYSizeTbl[i] )
            {
                    pLscProfile->LscYGradTbl[i] = (uint16_t)( (1UL << pLscProfile->LscYo) / pLscProfile->LscYSizeTbl[i] );
            }
            else
            {
                return ( RET_DIVISION_BY_ZERO );
            }
        }
    }
    else
    {
        memset( pLscProfile->LscYGradTbl, 0, sizeof( pLscProfile->LscYGradTbl ) );
    }

    if ( 0 < pLscProfile->LscXo )
    {
        for (i = 0; i<( sizeof(pLscProfile->LscXGradTbl)/ sizeof(pLscProfile->LscXGradTbl[0]) ); ++i )
        {
            if ( 0 < pLscProfile->LscXSizeTbl[i] )
            {
                    pLscProfile->LscXGradTbl[i] = (uint16_t)( (1UL << pLscProfile->LscXo) / pLscProfile->LscXSizeTbl[i] );
            }
            else
            {
                return ( RET_DIVISION_BY_ZERO );
            }
        }
    }
    else
    {
        memset( pLscProfile->LscXGradTbl, 0, sizeof( pLscProfile->LscXGradTbl ) );
    }

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/*****************************************************************************/
/**
 * @brief   This function prepares the calibration data.
 *
 * @param   pAwbCtx         awb context
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
static RESULT AwbPrepareCalibrationData
(
    AwbContext_t *pAwbCtx
)
{
    RESULT result = RET_SUCCESS;

    int32_t i;
    int32_t cnt;

    int32_t no = 0;

    CamCalibAwbGlobal_t *pAwbGlobal     = NULL;
    CamIlluProfile_t    *pIllumination  = NULL;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    // reset values
    pAwbCtx->NoIlluProfiles     = 0L;
    pAwbCtx->D50IlluProfileIdx  = -1;
    pAwbCtx->CwfIlluProfileIdx  = -1;

    // get awb-global calibration data from database
    result = CamCalibDbGetAwbGlobalByResolution( pAwbCtx->hCamCalibDb, pAwbCtx->ResName, &pAwbGlobal );
    if ( RET_SUCCESS != result )
    {
        TRACE( AWB_ERROR, "%s: database does not conatin AWB data for resolution %s\n", __FUNCTION__, pAwbCtx->ResName );
        return ( result );
    }
    DCT_ASSERT( pAwbGlobal != NULL );

    // store the pointers and values into instance context
    pAwbCtx->RgProjMaxSky       = pAwbGlobal->fRgProjMaxSky;
    pAwbCtx->RgProjIndoorMin    = pAwbGlobal->fRgProjIndoorMin;
    pAwbCtx->RgProjOutdoorMin   = pAwbGlobal->fRgProjOutdoorMin;
    pAwbCtx->RgProjMax          = pAwbGlobal->fRgProjMax;

	pAwbCtx->fRgProjALimit          	= pAwbGlobal->fRgProjALimit;      		//oyyf
	pAwbCtx->fRgProjAWeight         	= pAwbGlobal->fRgProjAWeight;			//oyyf
	pAwbCtx->fRgProjYellowLimit         = pAwbGlobal->fRgProjYellowLimit;		//oyyf
	pAwbCtx->fRgProjIllToCwf          	= pAwbGlobal->fRgProjIllToCwf;			//oyyf
	pAwbCtx->fRgProjIllToCwfWeight      = pAwbGlobal->fRgProjIllToCwfWeight;	//oyyf

	float 							fRgProjALimit;    //oyyf
	float							fRgProjAWeight;		//oyyf
	float 							fRgProjYellowLimit;		//oyyf
	float							fRgProjIllToCwf;		//oyyf
	float							fRgProjIllToCwfWeight;	//oyyf

	TRACE( AWB_DEBUG, "%s %d: awb limit fRgProjALimit(%f) fRgProjAWeight(%f) fRgProjYellowLimit(%f) fRgProjIllToCwf(%f)  fRgProjIllToCwfWeight(%f) \n",
	__FUNCTION__, __LINE__, pAwbCtx->fRgProjALimit, pAwbCtx->fRgProjAWeight, 
	pAwbCtx->fRgProjYellowLimit, pAwbCtx->fRgProjIllToCwf, pAwbCtx->fRgProjIllToCwfWeight);

    pAwbCtx->pKFactor           = &pAwbGlobal->KFactor;
    pAwbCtx->pPcaMatrix         = &pAwbGlobal->PCAMatrix;
    pAwbCtx->pSvdMeanValue      = &pAwbGlobal->SVDMeanValue;
    pAwbCtx->pCenterLine        = &pAwbGlobal->CenterLine;

    pAwbCtx->pGainClipCurve     = &pAwbGlobal->AwbClipParam;
    pAwbCtx->pGlobalFadeParam   = &pAwbGlobal->AwbGlobalFadeParm;
    pAwbCtx->pFadeParam         = &pAwbGlobal->AwbFade2Parm;

    pAwbCtx->RegionSizeStart    = pAwbGlobal->fRegionSize;
    pAwbCtx->RegionSize         = pAwbGlobal->fRegionSize;
    pAwbCtx->RegionSizeInc      = pAwbGlobal->fRegionSizeInc;
    pAwbCtx->RegionSizeDec      = pAwbGlobal->fRegionSizeDec;

    // get number of availabe illumination profiles from database
    result = CamCalibDbGetNoOfIlluminations( pAwbCtx->hCamCalibDb, &no );
    if ( (RET_SUCCESS != result) || ( !no ) )
    {
        TRACE( AWB_ERROR, "%s: database does not conatin illumination data for resolution %s\n",
                __FUNCTION__, pAwbCtx->ResName );
        return ( result );
    }

    // restrict the number of used profiles to max
    if ( AWB_MAX_ILLUMINATION_PROFILES < no )
    {
        TRACE( AWB_WARN, "%s: number of available illumination (%d) needs to be restricted to %d\n",
                __FUNCTION__, no, AWB_MAX_ILLUMINATION_PROFILES );
        no = AWB_MAX_ILLUMINATION_PROFILES;
    }

    // reset illumination profile counter
    cnt = 0;

    // run over all illumination profiles
    for ( i = 0; i < no; i++ )
    {
        // get an illumination profile from database
        result = CamCalibDbGetIlluminationByIdx( pAwbCtx->hCamCalibDb, i, &pIllumination );
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

        // store profile pointer in pointer array
        pAwbCtx->pIlluProfiles[cnt] = pIllumination;
        if ( !strcmp( pIllumination->name, pAwbGlobal->outdoor_clipping_profile ) )
        {
            TRACE( AWB_DEBUG, "illumination (out clip) %d %s\n", i, pIllumination->name );
            pAwbCtx->D50IlluProfileIdx = cnt;
        }
        else
        {
            TRACE( AWB_DEBUG, "illumination            %d %s\n", i, pIllumination->name );
        }
        // ddl@rock-chips.com:  store cwf profile index    
        if ( strcmp( pIllumination->name, "F2_CWF" ) == 0) {
            pAwbCtx->CwfIlluProfileIdx = cnt;
        }

        // get all cc-profiles for this illumination from database
        int32_t j = 0;
        for ( j = 0; j<pIllumination->cc_no; ++j )
        {
            CamCcProfile_t *pCcProfile = NULL;

            // get a cc-profile from database
            result = CamCalibDbGetCcProfileByName( pAwbCtx->hCamCalibDb, pIllumination->cc_profiles[j], &pCcProfile );
            RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
            DCT_ASSERT( pCcProfile != NULL );

            // store cc-profile in pointer array
            pAwbCtx->pCcProfiles[i][j] = pCcProfile;
        }

        // sort cc-profiles by saturation in descending order
        result = AwbOrderCcProfilesBySaturation(  pAwbCtx->pCcProfiles[i], pIllumination->cc_no );
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

        // get all lsc-profiles for this illumination from database
        // (note: lsc is resolution depended, run over all available resolution)
        int32_t k = 0;
        for ( k=0; k<pIllumination->lsc_res_no; ++k )
        {
            for ( j=0; j<pIllumination->lsc_no[k]; ++j )
            {
                CamLscProfile_t *pLscProfile = NULL;

				TRACE( AWB_NOTICE1, "%s%d  LSC name           %s\n", __FUNCTION__,__LINE__,pIllumination->lsc_profiles[k][j] );
                // get a lsc-profile from database
                result = CamCalibDbGetLscProfileByName( pAwbCtx->hCamCalibDb, pIllumination->lsc_profiles[k][j], &pLscProfile );
                RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
                DCT_ASSERT( pLscProfile != NULL );

                // recalculate gradient values (grad values are no longer presented
                // by XML/Matlab calibration tools, so we need to calc. these values)
                result = AwbLscGradientCheck( pLscProfile );
                RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

                // store lsc-profile in pointer array
                pAwbCtx->pLscProfiles[k][i][j] = pLscProfile;
            }

            // order lsc-profiles by vignetting
            result = AwbOrderLscProfilesByVignetting( pAwbCtx->pLscProfiles[k][i], pIllumination->lsc_no[k] );
            RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
        }

        // increment illumination profile counter
        cnt ++;
    }

    TRACE( AWB_DEBUG, "added %d illuminations\n", cnt );

    // check if we found an outdoor-clipping profile
    if ( -1 == pAwbCtx->D50IlluProfileIdx )
    {
        TRACE( AWB_ERROR, "Outdoor clipping illumination is missing or undefined\n" );
        return ( RET_OUTOFRANGE );
    }

    pAwbCtx->NoIlluProfiles = cnt;

    // initialize the IIR filter
    AwbExpPriorConfig_t ExpPriorCfg;

    MEMSET( &ExpPriorCfg, 0, sizeof( ExpPriorCfg ) );
    ExpPriorCfg.IIRDampCoefAdd          = pAwbGlobal->IIR.fIIRDampCoefAdd;
    ExpPriorCfg.IIRDampCoefSub          = pAwbGlobal->IIR.fIIRDampCoefSub;
    ExpPriorCfg.IIRDampFilterThreshold  = pAwbGlobal->IIR.fIIRDampFilterThreshold;
    ExpPriorCfg.IIRDampingCoefMin       = pAwbGlobal->IIR.fIIRDampingCoefMin;
    ExpPriorCfg.IIRDampingCoefMax       = pAwbGlobal->IIR.fIIRDampingCoefMax;
    ExpPriorCfg.IIRDampingCoefInit      = pAwbGlobal->IIR.fIIRDampingCoefInit;
    ExpPriorCfg.IIRFilterSize           = pAwbGlobal->IIR.IIRFilterSize;
    ExpPriorCfg.IIRFilterInitValue      = pAwbGlobal->IIR.fIIRFilterInitValue;
    result = AwbExpPriorInit( (AwbHandle_t)pAwbCtx, &ExpPriorCfg );

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/*****************************************************************************/
/**
 * @brief   prepares the sensor calibration data
 *
 * @param   pAwbCtx         awb context
 * @param   idx             index of illumination profile in list
 * @param   pIlluProfile    double pointer for returning a pointer to
 *                          illumination
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
static RESULT AwbGetProfile
(
    AwbContext_t        *pAwbCtx,
    const uint32_t      idx,
    CamIlluProfile_t    **pIlluProfile
)
{
    int32_t i;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( NULL == pIlluProfile )
    {
        return ( RET_INVALID_PARM );
    }

    *pIlluProfile = NULL;

    if ( AWB_MAX_ILLUMINATION_PROFILES <= idx )
    {
        return ( RET_OUTOFRANGE );
    }

    *pIlluProfile = pAwbCtx->pIlluProfiles[idx];

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/*****************************************************************************/
/**
 * @brief   This function prepares the input values for one calculation run.
 *
 * @param   pAwbCtx         awb context
 * @param   pMeasResult     measured mean values ( of last frame )
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
static RESULT AwbPrepareInputValues
(
    AwbContext_t                        *pAwbCtx,
    const CamerIcAwbMeasuringResult_t   *pMeasResult
)
{
    RESULT result = RET_SUCCESS;

    CamerIcGains_t          Gains;
    CamerIc3x3Matrix_t      XTalk;
    CamerIcXTalkOffset_t    XTalkOffset;

    float dummy;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* 1.) store the measured means to context */
    pAwbCtx->MeasuredMeans = *pMeasResult;

    /* 2.) fetch current CamerIc gains and convert to float */
    result = CamerIcIspAwbGetGains( pAwbCtx->hCamerIc, &Gains );
    RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
    result = CamerIcGains2AwbGains( &Gains, &pAwbCtx->Gains );
    RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

    TRACE( AWB_DEBUG, "Gains: %f, %f, %f, %f\n",
            pAwbCtx->Gains.fRed, pAwbCtx->Gains.fGreenR, pAwbCtx->Gains.fGreenB, pAwbCtx->Gains.fBlue );

    /* 3.) fetch current CamerIc crosstalk matrix */
    result = CamerIcIspGetCrossTalkCoefficients( pAwbCtx->hCamerIc, &XTalk );
    RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
    result = CamerIcXtalk2AwbXtalk( &XTalk, &pAwbCtx->CtMatrix );
    RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

    /* 4.) fetch current CamerIc crosstalk matrix */
    result = CamerIcIspGetCrossTalkOffset( pAwbCtx->hCamerIc, &XTalkOffset );
    RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
    result = CamerIcXTalkOffset2AwbXTalkOffset( &XTalkOffset, &pAwbCtx->CtOffset );
    RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 * @brief   Write calculated values to hardware
 *
 * @param   pAwbCtx         awb context
 * @param   pMeasResult     measured mean values ( of last frame )
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
static RESULT AwbSetValues
(
    AwbContext_t *pAwbCtx
)
{
    RESULT result = RET_SUCCESS;

    CamerIcGains_t          Gains;
    CamerIc3x3Matrix_t      XTalk;
    CamerIcXTalkOffset_t    XTalkOffset;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* 1.) convert to fix point and set current CamerIc gains */
    result = AwbGains2CamerIcGains( &pAwbCtx->WbGains, &Gains );
    RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
    result = CamerIcIspAwbSetGains( pAwbCtx->hCamerIc, &Gains );
    RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

    /* 2.) convert to fix point and set current CamerIc crosstalk matrix */
    result = AwbXtalk2CamerIcXtalk(  &pAwbCtx->DampedCcMatrix, &XTalk );
    RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
    result = CamerIcIspSetCrossTalkCoefficients( pAwbCtx->hCamerIc, &XTalk );
    RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

    /* 2.1) set current CamerIc crosstalk offsets */
    if ( pAwbCtx->Flags & AWB_WORKING_FLAG_USE_CC_OFFSET )
    {
        result = AwbXTalkOffset2CamerIcXTalkOffset( &pAwbCtx->DampedCcOffset, &XTalkOffset );
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
    }
    else
    {
        memset( &XTalkOffset, 0, sizeof(  XTalkOffset ) );
    }
    result = CamerIcIspSetCrossTalkOffset( pAwbCtx->hCamerIc, &XTalkOffset );
    RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

    TRACE( AWB_DEBUG, "programed CC-Offsets ( %d, %d, %d )\n", XTalkOffset.Red, XTalkOffset.Green, XTalkOffset.Blue );

    /* 3.) set current CamerIc measuring config */
    result = CamerIcIspAwbSetMeasuringMode( pAwbCtx->hCamerIc, pAwbCtx->MeasMode, &pAwbCtx->MeasWdw );
    RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

    /* 4.) set current CamerIc measuring config */
    result = CamerIcIspLscSetLenseShadeCorrectionMatrix( pAwbCtx->hCamerIc,
                    pAwbCtx->DampedLscMatrixTable.LscMatrix[ISI_COLOR_COMPONENT_RED].uCoeff,
                    pAwbCtx->DampedLscMatrixTable.LscMatrix[ISI_COLOR_COMPONENT_GREENR].uCoeff,
                    pAwbCtx->DampedLscMatrixTable.LscMatrix[ISI_COLOR_COMPONENT_GREENB].uCoeff,
                    pAwbCtx->DampedLscMatrixTable.LscMatrix[ISI_COLOR_COMPONENT_BLUE].uCoeff );
    RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

    if ( pAwbCtx->hSubCamerIc != NULL )
    {
        /* 1.) set current CamerIc gains */
        result = CamerIcIspAwbSetGains( pAwbCtx->hSubCamerIc, &Gains );
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

        /* 2.) set current CamerIc crosstalk matrix */
        result = CamerIcIspSetCrossTalkCoefficients( pAwbCtx->hSubCamerIc, &XTalk );
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

        /* 2.1) set current CamerIc crosstalk offsets */
        result = CamerIcIspSetCrossTalkOffset( pAwbCtx->hSubCamerIc, &XTalkOffset );
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

        /* 3.) set current CamerIc measuring config */
        //result = CamerIcIspAwbSetMeasuringMode( pAwbCtx->hSubCamerIc, pAwbCtx->MeasMode, &pAwbCtx->MeasWdw );
        //RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

        /* 4.) set current CamerIc measuring config */
        result = CamerIcIspLscSetLenseShadeCorrectionMatrix( pAwbCtx->hSubCamerIc,
                        pAwbCtx->DampedLscMatrixTable.LscMatrix[ISI_COLOR_COMPONENT_RED].uCoeff,
                        pAwbCtx->DampedLscMatrixTable.LscMatrix[ISI_COLOR_COMPONENT_GREENR].uCoeff,
                        pAwbCtx->DampedLscMatrixTable.LscMatrix[ISI_COLOR_COMPONENT_GREENB].uCoeff,
                        pAwbCtx->DampedLscMatrixTable.LscMatrix[ISI_COLOR_COMPONENT_BLUE].uCoeff );
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
    }

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 * @brief   This local function evaluates the number of white pixels.
 *
 * @param   pAwbCtx         awb context
 * @param   pMeasResult     measured mean values ( of last frame )
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
static AwbNumWhitePixelEval_t AwbWpNumCheck
(
    AwbContext_t    *pAwbCtx,
    const uint32_t  NumWhitePixel
)
{
    AwbNumWhitePixelEval_t NumWhitePixelEval = AWB_NUM_WHITE_PIXEL_INVALID;

    if ( (NumWhitePixel < pAwbCtx->NumWhitePixelsMax)
            && (NumWhitePixel >= pAwbCtx->NumWhitePixelsMin) )
    {
        NumWhitePixelEval = AWB_NUM_WHITE_PIXEL_TARGET_RANGE;
    }
    else if ( NumWhitePixel < pAwbCtx->NumWhitePixelsMin )
    {
        NumWhitePixelEval = AWB_NUM_WHITE_PIXEL_LTMIN;
    }
    else
    {
        // NumWhitePixel > pAwbCtx->NumWhitePixelsMax
        NumWhitePixelEval = AWB_NUM_WHITE_PIXEL_GTMAX;
    }

    return ( NumWhitePixelEval );
}



/******************************************************************************
 * Implementation of AWB API Functions
 *****************************************************************************/

/******************************************************************************
 * AwbGains2CamerIcGains()
 *****************************************************************************/
RESULT AwbGains2CamerIcGains
(
    AwbGains_t      *pAwbGains,
    CamerIcGains_t  *pCamerIcGains
)
{
    RESULT result = RET_SUCCESS;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (pAwbGains != NULL) && (pCamerIcGains != NULL) )
    {
        pCamerIcGains->Red      = UtlFloatToFix_U0208( pAwbGains->fRed );
        pCamerIcGains->GreenR   = UtlFloatToFix_U0208( pAwbGains->fGreenR );
        pCamerIcGains->GreenB   = UtlFloatToFix_U0208( pAwbGains->fGreenB );
        pCamerIcGains->Blue     = UtlFloatToFix_U0208( pAwbGains->fBlue );
    }
    else
    {
        result = RET_NULL_POINTER;
    }

    TRACE( AWB_INFO, "%s: (exit %d)\n", __FUNCTION__, result );

    return ( result );
}



/******************************************************************************
 * CamerIcGains2AwbGains()
 *****************************************************************************/
RESULT CamerIcGains2AwbGains
(
    CamerIcGains_t  *pCamerIcGains,
    AwbGains_t      *pAwbGains
)
{
    RESULT result = RET_SUCCESS;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (pAwbGains != NULL) && (pCamerIcGains != NULL) )
    {
        pAwbGains->fRed      = UtlFixToFloat_U0208( pCamerIcGains->Red );
        pAwbGains->fGreenR   = UtlFixToFloat_U0208( pCamerIcGains->GreenR );
        pAwbGains->fGreenB   = UtlFixToFloat_U0208( pCamerIcGains->GreenB );
        pAwbGains->fBlue     = UtlFixToFloat_U0208( pCamerIcGains->Blue );
    }
    else
    {
        result = RET_NULL_POINTER;
    }

    TRACE( AWB_INFO, "%s: (exit %d)\n", __FUNCTION__, result );

    return ( result );
}



/******************************************************************************
 * AwbXtalk2CamerIcXtalk()
 *****************************************************************************/
RESULT AwbXtalk2CamerIcXtalk
(
    Cam3x3FloatMatrix_t *pAwbXTalkMatrix,
    CamerIc3x3Matrix_t  *pXTalkMatrix
)
{
    RESULT result = RET_SUCCESS;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (pAwbXTalkMatrix != NULL) && (pXTalkMatrix != NULL) )
    {
        int32_t i;
        for ( i = 0UL; i<9; i++ )
        {
            pXTalkMatrix->Coeff[i] = UtlFloatToFix_S0407( pAwbXTalkMatrix->fCoeff[i] );
        }
    }
    else
    {
        result = RET_NULL_POINTER;
    }

    TRACE( AWB_INFO, "%s: (exit %d)\n", __FUNCTION__, result );

    return ( result );
}


/******************************************************************************
 * CamerIcXtalk2AwbXtalk()
 *****************************************************************************/
RESULT CamerIcXtalk2AwbXtalk
(
    CamerIc3x3Matrix_t  *pXTalkMatrix,
    Cam3x3FloatMatrix_t *pAwbXTalkMatrix
)
{
    RESULT result = RET_SUCCESS;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (NULL != pXTalkMatrix) && (NULL != pAwbXTalkMatrix) )
    {
        int32_t i;
        for ( i = 0UL; i<9; i++ )
        {
            pAwbXTalkMatrix->fCoeff[i] = UtlFixToFloat_S0407( pXTalkMatrix->Coeff[i] );
        }
    }
    else
    {
        result = RET_NULL_POINTER;
    }

    TRACE( AWB_INFO, "%s: (exit %d)\n", __FUNCTION__, result );

    return ( result );
}



/******************************************************************************
 * AwbXTalkOffset2CamerIcXTalkOffset()
 *****************************************************************************/
RESULT AwbXTalkOffset2CamerIcXTalkOffset
(
    Cam1x3FloatMatrix_t     *pAwbXTalkOffset,
    CamerIcXTalkOffset_t    *pCamerIcXTalkOffset
)
{
    RESULT result = RET_SUCCESS;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (pAwbXTalkOffset != NULL) && (pCamerIcXTalkOffset != NULL) )
    {
        if ( (pAwbXTalkOffset->fCoeff[CAM_3CH_COLOR_COMPONENT_RED] > 2047.0f)
                || (pAwbXTalkOffset->fCoeff[CAM_3CH_COLOR_COMPONENT_RED] < -2048.0f)
                || (pAwbXTalkOffset->fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN] > 2047.0f)
                || (pAwbXTalkOffset->fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN] < -2048.0f)
                || (pAwbXTalkOffset->fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE] > 2047.0f)
                || (pAwbXTalkOffset->fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE] < -2048.0f) )
        {
            result = RET_OUTOFRANGE;
        }
        else
        {
            pCamerIcXTalkOffset->Red      = UtlFloatToFix_S1200( pAwbXTalkOffset->fCoeff[CAM_3CH_COLOR_COMPONENT_RED] );
            pCamerIcXTalkOffset->Green    = UtlFloatToFix_S1200( pAwbXTalkOffset->fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN] );
            pCamerIcXTalkOffset->Blue     = UtlFloatToFix_S1200( pAwbXTalkOffset->fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE] );
        }
    }
    else
    {
        result = RET_NULL_POINTER;
    }

    TRACE( AWB_INFO, "%s: (exit %d)\n", __FUNCTION__, result );

    return ( result );
}



/******************************************************************************
 * CamerIcXTalkOffset2AwbXTalkOffset()
 *****************************************************************************/
RESULT CamerIcXTalkOffset2AwbXTalkOffset
(
    CamerIcXTalkOffset_t    *pCamerIcXTalkOffset,
    AwbXTalkOffset_t        *pAwbXTalkOffset
)
{
    RESULT result = RET_SUCCESS;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (pCamerIcXTalkOffset != NULL) && (pAwbXTalkOffset != NULL) )
    {
        pAwbXTalkOffset->fRed   = UtlFixToFloat_S1200( pCamerIcXTalkOffset->Red );
        pAwbXTalkOffset->fGreen = UtlFixToFloat_S1200( pCamerIcXTalkOffset->Green );
        pAwbXTalkOffset->fBlue  = UtlFixToFloat_S1200( pCamerIcXTalkOffset->Blue );
    }
    else
    {
        result = RET_NULL_POINTER;
    }

    TRACE( AWB_INFO, "%s: (exit %d)\n", __FUNCTION__, result );

    return ( result );
}



/******************************************************************************
 * AwbInit()
 *****************************************************************************/
RESULT AwbInit
(
    AwbInstanceConfig_t *pInstConfig
)
{
    RESULT result = RET_SUCCESS;

    AwbContext_t *pAwbCtx;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pInstConfig ) 
    {
        return ( RET_INVALID_PARM );
    }

    /* allocate auto white balance control context */
    pAwbCtx = (AwbContext_t *)malloc( sizeof(AwbContext_t) );
    if ( pAwbCtx == NULL )
    {
        TRACE( AWB_ERROR,  "%s: Can't allocate AWB context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }

    /* pre-initialize context */
    MEMSET( pAwbCtx, 0, sizeof(*pAwbCtx) );
    pAwbCtx->state       = AWB_STATE_INITIALIZED;

    pAwbCtx->ScalingThreshold0      = 0.3;
    pAwbCtx->ScalingThreshold1      = 0.7;
    pAwbCtx->ScalingFactor          = 1.0f;
    pAwbCtx->DampingFactorScaling   = 0.25f;

	pAwbCtx->TemRgLine_CalFlag      = BOOL_FALSE;
    /* return handle */
    pInstConfig->hAwb = (AwbHandle_t)pAwbCtx;

    TRACE( AWB_INFO, "%s: (exit %d)\n", __FUNCTION__, result );

    return ( result );
}



/******************************************************************************
 * AwbRelease()
 *****************************************************************************/
RESULT AwbRelease
(
    AwbHandle_t handle
)
{
    RESULT result = RET_SUCCESS;

    AwbContext_t *pAwbCtx = (AwbContext_t *)handle;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pAwbCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check state */
    if ( (AWB_STATE_RUNNING == pAwbCtx->state)
            || (AWB_STATE_LOCKED == pAwbCtx->state) )
    {
        return ( RET_BUSY );
    }

    result = AwbExpPriorRelease( pAwbCtx );
    if ( RET_SUCCESS != result )
    {
        return ( result );
    }

    MEMSET( pAwbCtx, 0, sizeof(AwbContext_t) );
    free ( pAwbCtx );

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AwbConfigure()
 *****************************************************************************/
RESULT AwbConfigure
(
    AwbHandle_t handle,
    AwbConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    AwbContext_t *pAwbCtx = (AwbContext_t*)handle;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pAwbCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ( NULL == pConfig ) ||
         ( NULL == pConfig->hCamCalibDb ) )
    {
        return ( RET_INVALID_PARM );
    }

    if ( ( AWB_STATE_INITIALIZED != pAwbCtx->state ) &&
         ( AWB_STATE_STOPPED != pAwbCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( ( pConfig->MeasMode <= CAMERIC_ISP_AWB_MEASURING_MODE_INVALID ) &&
         ( pConfig->MeasMode >= CAMERIC_ISP_AWB_MEASURING_MODE_MAX ) )
    {
        return ( RET_OUTOFRANGE );
    }

    // update context
    pAwbCtx->hCamerIc           = pConfig->hCamerIc;
    pAwbCtx->hSubCamerIc        = pConfig->hSubCamerIc;
    pAwbCtx->Mode               = pConfig->Mode;
    pAwbCtx->MeasMode           = pConfig->MeasMode;
    pAwbCtx->MeasConfig         = pConfig->MeasConfig;
    pAwbCtx->MeasWdw            = pConfig->MeasConfig;
    pAwbCtx->Flags              = pConfig->Flags;
    pAwbCtx->fStableDeviation   = pConfig->fStableDeviation;
    pAwbCtx->fRestartDeviation  = pConfig->fRestartDeviation;

    // initialize working range min<->max of white pixel
    AwbPrepareWorkingRange( pAwbCtx, pConfig->width, pConfig->height );

    // initialize calibration database access
    result = AwbPrepareCalibDbAccess( pAwbCtx, pConfig->hCamCalibDb, pConfig->width, pConfig->height, pConfig->framerate );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    // initialize calibration data
    result = AwbPrepareCalibrationData( pAwbCtx );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    pAwbCtx->Config = *pConfig;

	//fit ct rg line
	result = AwbFitTemperatureRgLine(handle);
	TRACE( AWB_INFO, "%s: ( %d)\n", __FUNCTION__, result );
	TRACE( AWB_INFO, "%s: fit line CT=(%f)*Rg+(%f), \n", __FUNCTION__,pAwbCtx->TemRgLine_k,pAwbCtx->TemRgLine_b);
	if ( result != RET_SUCCESS )	{
	
	 return ( result );
	}
    TRACE( AWB_INFO, "%s: (exit %d)\n", __FUNCTION__, result );

    return ( result );
}



/******************************************************************************
 * AwbReConfigure()
 *****************************************************************************/
RESULT AwbReConfigure
(
    AwbHandle_t handle,
    AwbConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    AwbContext_t *pAwbCtx = (AwbContext_t*)handle;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pAwbCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (AWB_STATE_LOCKED != pAwbCtx->state)
            && (AWB_STATE_RUNNING != pAwbCtx->state)
            && (AWB_STATE_STOPPED != pAwbCtx->state) )
    {
        return ( RET_WRONG_STATE );
    }

    pAwbCtx->hCamerIc           = pConfig->hCamerIc;
    pAwbCtx->hSubCamerIc        = pConfig->hSubCamerIc;

    // check if we switched the resolution
    if ( (pConfig->width != pAwbCtx->Config.width) 
            || (pConfig->height != pAwbCtx->Config.height)
            || (pConfig->framerate != pAwbCtx->Config.framerate) )
    {
        pAwbCtx->Config.width       = pConfig->width;
        pAwbCtx->Config.height      = pConfig->height;
        pAwbCtx->Config.framerate   = pConfig->framerate;
        pAwbCtx->Config.Flags       = pConfig->Flags;

        // re-initialize calibration database access
        result = AwbPrepareCalibDbAccess( pAwbCtx, pConfig->hCamCalibDb, pConfig->width, pConfig->height, pConfig->framerate );
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

        // get LSC-profile and reset sector and gradient tables
        CamLscProfile_t *pLscProfile = pAwbCtx->pLscProfiles[pAwbCtx->ResIdx][pAwbCtx->DominateIlluProfileIdx][0];
        DCT_ASSERT( pLscProfile != NULL );

        memcpy( &pAwbCtx->UndampedLscMatrixTable, &pLscProfile->LscMatrix[0], sizeof(CamLscMatrix_t) );
        memcpy( &pAwbCtx->DampedLscMatrixTable, &pLscProfile->LscMatrix[0], sizeof(CamLscMatrix_t) );

        memcpy( &pAwbCtx->SectorConfig.LscXGradTbl[0], &pLscProfile->LscXGradTbl[0], sizeof(pLscProfile->LscXGradTbl) );
        memcpy( &pAwbCtx->SectorConfig.LscYGradTbl[0], &pLscProfile->LscYGradTbl[0], sizeof(pLscProfile->LscYGradTbl) );
        memcpy( &pAwbCtx->SectorConfig.LscXSizeTbl[0], &pLscProfile->LscXSizeTbl[0], sizeof(pLscProfile->LscXSizeTbl) );
        memcpy( &pAwbCtx->SectorConfig.LscYSizeTbl[0], &pLscProfile->LscYSizeTbl[0], sizeof(pLscProfile->LscYSizeTbl) );

        CamerIcIspLscSetLenseShadeSectorConfig(  pAwbCtx->hCamerIc,  &pAwbCtx->SectorConfig );
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

        if ( pAwbCtx->hSubCamerIc != NULL )
        {
            /* 1.) set current CamerIc gains */
            result = CamerIcIspLscSetLenseShadeSectorConfig( pAwbCtx->hSubCamerIc,  &pAwbCtx->SectorConfig );
            RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
        }

        // run alsc processing once
        result = AwbAlscProcessFrame( pAwbCtx );
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

        // write Data to hardware
        result = AwbSetValues( pAwbCtx );
    }

    TRACE( AWB_INFO, "%s: (exit %d)\n", __FUNCTION__, result );

    return ( result );
}



/******************************************************************************
 * AwbStart()
 *****************************************************************************/
RESULT AwbStart
(
    AwbHandle_t         handle,
    const AwbMode_t     mode,
    const uint32_t      idx
)
{
	
    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);
	RESULT result = RET_SUCCESS;
    AwbContext_t *pAwbCtx = (AwbContext_t *)handle;

    CamIlluProfile_t    *pIlluProfile    = NULL;
    float Rg, Bg,ct,fRg, fBg,fGreenR,fGreenB,fRed,fBlue,min_dis,dis;
	int i,illNO;
    //oyyf
    uint32_t D65_idx,estmation_idx;
    /*if(pAwbCtx->D50IlluProfileIdx != -1)
    {
	D65_idx = pAwbCtx->D50IlluProfileIdx;
    }else{
	D65_idx = idx;
    }//oyyf
*/
    /* initial checks */
    if ( pAwbCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ( AWB_STATE_RUNNING == pAwbCtx->state ) ||
         ( AWB_STATE_LOCKED == pAwbCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( ( mode <= AWB_MODE_INVALID ) &&
         ( mode >= AWB_MODE_MAX ) )
    {
        return ( RET_OUTOFRANGE );
    }
   if(AWB_MODE_MANUAL_CT ==mode)
   	{
		/*1). illumination estimation*/
		/*1.1)calculate rg bg by ct*/
		ct=(float)idx;
		if(ct<2000){		
		TRACE( AWB_ERROR, "%s: The setting color temperature is (%f)K. And the CT is too low.\n", __FUNCTION__,ct);
		}else if(ct>7500){		
		TRACE( AWB_ERROR, "%s: The setting color temperature is (%f)K. And the CT is too high.\n", __FUNCTION__,ct);
		}
		result = AwbCalcWBgainbyCT(	handle,ct,&Rg,&Bg);	
		RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
		TRACE( AWB_INFO, "%s: AWB_MODE_MANUAL_CT,ct_set(%f),Rg_C(%f),Bg_C(%f)\n", __FUNCTION__,ct,Rg,Bg);
		/*1.2). illumination estimation*/
    	illNO=pAwbCtx->NoIlluProfiles;
		min_dis=1e+10;
		for(i=0;i<illNO;i++){		
		  fRed       = pAwbCtx->pIlluProfiles[i]->ComponentGain.fCoeff[CAM_4CH_COLOR_COMPONENT_RED];
          fGreenR    = pAwbCtx->pIlluProfiles[i]->ComponentGain.fCoeff[CAM_4CH_COLOR_COMPONENT_GREENR];		  
 		  fGreenB    = pAwbCtx->pIlluProfiles[i]->ComponentGain.fCoeff[CAM_4CH_COLOR_COMPONENT_GREENB];
    	  fBlue      = pAwbCtx->pIlluProfiles[i]->ComponentGain.fCoeff[CAM_4CH_COLOR_COMPONENT_BLUE];
		  fRg= fRed/fGreenR;
		  fBg= fBlue/fGreenB;
		  dis=(fRg-Rg)*(fRg-Rg)+(fBg-Bg)*(fBg-Bg);
		  if((i==0)||(dis<min_dis)){
		  		min_dis=dis;
				estmation_idx=i;
		  	}		  
		  TRACE( AWB_INFO, "%s: %s: fRg(%f), fBg(%f), dis(%f)\n", __FUNCTION__,pAwbCtx->pIlluProfiles[i]->name,fRg,fBg,dis);
		}		
		TRACE( AWB_INFO, "%s: illu_est(%s)\n", __FUNCTION__,pAwbCtx->pIlluProfiles[estmation_idx]->name);

		/* 2.) get the selected illumination profile, if this not available returns RET_OUTOFRANGE */
		//result = AwbGetProfile( pAwbCtx, idx, &pIlluProfile );
		result = AwbGetProfile( pAwbCtx, estmation_idx, &pIlluProfile ); 
		RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );	
		pAwbCtx->IlluIdx = estmation_idx;	

		 /* 3.) set white balance gains */
		 pAwbCtx->WbGains.fRed		 = Rg;
		 pAwbCtx->WbGains.fGreenR	 = 1;
		 pAwbCtx->WbGains.fGreenB	 = 1;
	 	 pAwbCtx->WbGains.fBlue 	 = Bg;

		 }else {
	  		 /* 1.) set  D65_idx*/
			 D65_idx = idx;
			 TRACE( AWB_NOTICE1, "%s: (enter) cie_index(%d)\n", __FUNCTION__, idx);

			 /* 2.) get the selected illumination profile, if this not available returns RET_OUTOFRANGE */
			 //result = AwbGetProfile( pAwbCtx, idx, &pIlluProfile );
			 result = AwbGetProfile( pAwbCtx, D65_idx, &pIlluProfile ); //oyyf
			 RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );	 
			  //pAwbCtx->IlluIdx = idx;
			 pAwbCtx->IlluIdx = D65_idx;//oyyf	 
	 
			  /* 3.) set white balance gains */
			  pAwbCtx->WbGains.fRed 	  = pIlluProfile->ComponentGain.fCoeff[CAM_4CH_COLOR_COMPONENT_RED];
			  pAwbCtx->WbGains.fGreenR	  = pIlluProfile->ComponentGain.fCoeff[CAM_4CH_COLOR_COMPONENT_GREENR];
			  pAwbCtx->WbGains.fGreenB	  = pIlluProfile->ComponentGain.fCoeff[CAM_4CH_COLOR_COMPONENT_GREENB];
			  pAwbCtx->WbGains.fBlue	  = pIlluProfile->ComponentGain.fCoeff[CAM_4CH_COLOR_COMPONENT_BLUE];
		 	}	 
	
	/* 4.) set CC-Matrix */
	memcpy( &pAwbCtx->DampedCcMatrix, &pIlluProfile->CrossTalkCoeff, sizeof(Cam3x3FloatMatrix_t) );
	memcpy( &pAwbCtx->DampedCcOffset, &pIlluProfile->CrossTalkOffset, sizeof(Cam1x3FloatMatrix_t) );
	/* 5.) set LSC-Matrix (start with max vignetting) */
	CamLscProfile_t *pLscProfile = pAwbCtx->pLscProfiles[pAwbCtx->ResIdx][pAwbCtx->IlluIdx][0];
	 DCT_ASSERT( pLscProfile != NULL );
	
	memcpy( &pAwbCtx->UndampedLscMatrixTable, &pLscProfile->LscMatrix[0], sizeof(CamLscMatrix_t) );
	memcpy( &pAwbCtx->DampedLscMatrixTable, &pLscProfile->LscMatrix[0], sizeof(CamLscMatrix_t) );
	
	memcpy( &pAwbCtx->SectorConfig.LscXGradTbl[0], &pLscProfile->LscXGradTbl[0], sizeof(pLscProfile->LscXGradTbl) );
	memcpy( &pAwbCtx->SectorConfig.LscYGradTbl[0], &pLscProfile->LscYGradTbl[0], sizeof(pLscProfile->LscYGradTbl) );
	memcpy( &pAwbCtx->SectorConfig.LscXSizeTbl[0], &pLscProfile->LscXSizeTbl[0], sizeof(pLscProfile->LscXSizeTbl) );
	memcpy( &pAwbCtx->SectorConfig.LscYSizeTbl[0], &pLscProfile->LscYSizeTbl[0], sizeof(pLscProfile->LscYSizeTbl) );
	
	/* 6).  initially set LSC sector specification to hw */
	CamerIcIspLscSetLenseShadeSectorConfig(  pAwbCtx->hCamerIc,  &pAwbCtx->SectorConfig );
	RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
	
	if ( pAwbCtx->hSubCamerIc != NULL )
	{
		 /* 1.) set current CamerIc gains */
		 result = CamerIcIspLscSetLenseShadeSectorConfig( pAwbCtx->hSubCamerIc,  &pAwbCtx->SectorConfig );
		RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
	}
	
	/* 7.) write Data to hardware */
	 result = AwbSetValues( pAwbCtx );
	RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
	
	/* 8.) set working mode */
	pAwbCtx->Mode	= mode;
	pAwbCtx->state	= AWB_STATE_RUNNING;

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AwbStop()
 *****************************************************************************/
RESULT AwbStop
(
    AwbHandle_t handle
)
{
    AwbContext_t *pAwbCtx = (AwbContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pAwbCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* before stopping, unlock the AWB if locked */
    if ( AWB_STATE_LOCKED == pAwbCtx->state )
    {
        return ( RET_BUSY );
    }

    pAwbCtx->state  = AWB_STATE_STOPPED;

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AwbReset()
 *****************************************************************************/
RESULT AwbReset
(
    AwbHandle_t handle
)
{
    AwbContext_t *pAwbCtx = (AwbContext_t *)handle;

    CamIlluProfile_t *pIlluProfile = NULL;

    RESULT result = RET_SUCCESS;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pAwbCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if (AWB_STATE_LOCKED == pAwbCtx->state)
    {
        return ( RET_WRONG_STATE );
    }

    /* initialize working range min<->max of white pixel */
    AwbPrepareWorkingRange( pAwbCtx, pAwbCtx->Config.width, pAwbCtx->Config.height );

    // initialize calibration database access
    result = AwbPrepareCalibDbAccess( pAwbCtx, pAwbCtx->Config.hCamCalibDb,
                                        pAwbCtx->Config.width, pAwbCtx->Config.height, pAwbCtx->Config.framerate );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    // initialize calibration data
    result = AwbPrepareCalibrationData( pAwbCtx );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    pAwbCtx->Mode = pAwbCtx->Config.Mode;

    /* 1.) get the selected illumination profile, if this not available returns RET_OUTOFRANGE */
    result = AwbGetProfile( pAwbCtx, pAwbCtx->IlluIdx, &pIlluProfile );
    RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

    /* 2.) set CC-Matrix */
    memcpy( &pAwbCtx->DampedCcMatrix, &pIlluProfile->CrossTalkCoeff, sizeof(Cam3x3FloatMatrix_t) );
    memcpy( &pAwbCtx->DampedCcOffset, &pIlluProfile->CrossTalkOffset, sizeof(Cam1x3FloatMatrix_t) );

    /* 3.) set white balance gains */
    pAwbCtx->WbGains.fRed       = pIlluProfile->ComponentGain.fCoeff[CAM_4CH_COLOR_COMPONENT_RED];
    pAwbCtx->WbGains.fGreenR    = pIlluProfile->ComponentGain.fCoeff[CAM_4CH_COLOR_COMPONENT_GREENR];
    pAwbCtx->WbGains.fGreenB    = pIlluProfile->ComponentGain.fCoeff[CAM_4CH_COLOR_COMPONENT_GREENB];
    pAwbCtx->WbGains.fBlue      = pIlluProfile->ComponentGain.fCoeff[CAM_4CH_COLOR_COMPONENT_BLUE];

    /* 4.) set LSC-Matrix (start with max vignetting) */
    CamLscProfile_t *pLscProfile = pAwbCtx->pLscProfiles[pAwbCtx->ResIdx][pAwbCtx->IlluIdx][0];
    DCT_ASSERT( pLscProfile != NULL );

    memcpy( &pAwbCtx->UndampedLscMatrixTable, &pLscProfile->LscMatrix[0], sizeof(CamLscMatrix_t) );
    memcpy( &pAwbCtx->DampedLscMatrixTable, &pLscProfile->LscMatrix[0], sizeof(CamLscMatrix_t) );

    memcpy( &pAwbCtx->SectorConfig.LscXGradTbl[0], &pLscProfile->LscXGradTbl[0], sizeof(pLscProfile->LscXGradTbl) );
    memcpy( &pAwbCtx->SectorConfig.LscYGradTbl[0], &pLscProfile->LscYGradTbl[0], sizeof(pLscProfile->LscYGradTbl) );
    memcpy( &pAwbCtx->SectorConfig.LscXSizeTbl[0], &pLscProfile->LscXSizeTbl[0], sizeof(pLscProfile->LscXSizeTbl) );
    memcpy( &pAwbCtx->SectorConfig.LscYSizeTbl[0], &pLscProfile->LscYSizeTbl[0], sizeof(pLscProfile->LscYSizeTbl) );

    /* 4.1 initially set LSC sector specification to hw */
    CamerIcIspLscSetLenseShadeSectorConfig(  pAwbCtx->hCamerIc,  &pAwbCtx->SectorConfig );
    RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

    if ( pAwbCtx->hSubCamerIc != NULL )
    {
        /* 1.) set current CamerIc gains */
        result = CamerIcIspLscSetLenseShadeSectorConfig( pAwbCtx->hSubCamerIc,  &pAwbCtx->SectorConfig );
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
    }

    /* 5.) write Data to hardware */
    result = AwbSetValues( pAwbCtx );
    RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AwbSetFlags()
 *****************************************************************************/
RESULT AwbSetFlags
(
    AwbHandle_t     handle,
    const uint32_t  Flags
)
{
    AwbContext_t *pAwbCtx = (AwbContext_t *)handle;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pAwbCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    pAwbCtx->Flags = Flags;

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AwbSetHistogram()
 *****************************************************************************/
RESULT AwbSetHistogram
(
    AwbHandle_t         handle,
    CamerIcHistBins_t   bins
)
{
    AwbContext_t *pAwbCtx = (AwbContext_t *)handle;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pAwbCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMCPY( pAwbCtx->Histogram, bins, sizeof( CamerIcHistBins_t ) );

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AwbGetFlags()
 *****************************************************************************/
RESULT AwbGetFlags
(
    AwbHandle_t handle,
    uint32_t    *pFlags
)
{
    AwbContext_t *pAwbCtx = (AwbContext_t *)handle;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pAwbCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pFlags == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    *pFlags = pAwbCtx->Flags;

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AwbStatus()
 *****************************************************************************/
RESULT AwbStatus
(
    AwbHandle_t     handle,
    bool_t          *pRunning,       /**< BOOL_TRUE: running, BOOL_FALSE: stopped */
    AwbMode_t       *pMode,
    uint32_t        *pIlluIdx,
    AwbRgProj_t     *pRgProj
)
{
    AwbContext_t *pAwbCtx = (AwbContext_t *)handle;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pAwbCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (pRunning == NULL) || (pMode == NULL) || (pIlluIdx == NULL) )
    {
        return ( RET_INVALID_PARM );
    }

    *pRunning = ( (pAwbCtx->state == AWB_STATE_RUNNING)
                    || (pAwbCtx->state == AWB_STATE_LOCKED) ) ? BOOL_TRUE : BOOL_FALSE;
    *pMode      = pAwbCtx->Mode;
    *pIlluIdx   = pAwbCtx->IlluIdx;

    if ( pRgProj != NULL )
    {
        pRgProj->fRgProjIndoorMin  = pAwbCtx->RgProjIndoorMin;
        pRgProj->fRgProjOutdoorMin = pAwbCtx->RgProjOutdoorMin;
        pRgProj->fRgProjMax        = pAwbCtx->RgProjMax;
        pRgProj->fRgProjMaxSky     = pAwbCtx->RgProjMaxSky;

		pRgProj->fRgProjALimit     			= pAwbCtx->fRgProjALimit; //oyyf
		pRgProj->fRgProjAWeight     		= pAwbCtx->fRgProjAWeight; //oyyf
		pRgProj->fRgProjYellowLimit     	= pAwbCtx->fRgProjYellowLimit; //oyyf
		pRgProj->fRgProjIllToCwf     		= pAwbCtx->fRgProjIllToCwf; //oyyf
		pRgProj->fRgProjIllToCwfWeight     	= pAwbCtx->fRgProjIllToCwfWeight; //oyyf
    }

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * AwbSettled()
 *****************************************************************************/
RESULT AwbSettled
(
    AwbHandle_t handle,
    bool_t      *pSettled,
    uint32_t    *pDNoWhitePixel
)
{
    AwbContext_t *pAwbCtx = (AwbContext_t *)handle;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pAwbCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pSettled == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    if ( (pAwbCtx->Mode == AWB_MODE_AUTO) && (pAwbCtx->state == AWB_STATE_RUNNING) )
    {
        TRACE( AWB_INFO, "%s: %d < %d\n", __FUNCTION__, pAwbCtx->dNoWhitePixel, pAwbCtx->StableThreshold );
        *pSettled = ( pAwbCtx->dNoWhitePixel <= pAwbCtx->StableThreshold ) ? BOOL_TRUE : BOOL_FALSE;
		*pDNoWhitePixel = pAwbCtx->dNoWhitePixel;
    }
    else
    {
        *pSettled = BOOL_FALSE;
		*pDNoWhitePixel = 0;
    }

    TRACE( AWB_INFO, "%s: (exit) stable(%d)\n", __FUNCTION__,*pSettled);

    return ( RET_SUCCESS );
}


//oyyf add for 0 offset in lowlight luma 
RESULT AwbCCOffsetCal(
	AwbContext_t                        *pAwbCtx
)
{

	RESULT result = RET_SUCCESS;
    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__ );
	
	uint32_t SumHistogram = 0U;
    for ( uint32_t i = 0U;  i < CAMERIC_ISP_HIST_NUM_BINS; i++ )
    {
        SumHistogram += pAwbCtx->Histogram[i];
    }

    /* avoid division by zero */
    if ( SumHistogram == 0U )
    {
        TRACE( AWB_WARN, "%s: SumHistogram == 0, avoid division by zero, correcting to 1\n", __FUNCTION__ );
        SumHistogram = 1U;
    }
	
	pAwbCtx->MeanHistogram = 0.0f;
	for (uint32_t i = 0U;  i < CAMERIC_ISP_HIST_NUM_BINS; i++ )
	{
	   pAwbCtx->MeanHistogram += ( (float)(16U * pAwbCtx->Histogram[i]) ) *	(i + 0.5f );
	}
	pAwbCtx->MeanHistogram = pAwbCtx->MeanHistogram / SumHistogram;


	float fExp = pAwbCtx->SensorGain * pAwbCtx->IntegrationTime;

	
	TRACE( AWB_DEBUG, "%s: oyyf fExp(%f) MeanHistogram(%f) \n", __FUNCTION__, fExp, pAwbCtx->MeanHistogram);
	if(pAwbCtx->MeanHistogram < 20  &&  fExp > 0.01)
	{

		TRACE( AWB_ERROR, "%s: oyyf ( offset zero)\n", __FUNCTION__ );
		pAwbCtx->DampedCcOffset.fCoeff[CAM_3CH_COLOR_COMPONENT_RED] = 0.0;
		pAwbCtx->DampedCcOffset.fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN] = 0.0;
		pAwbCtx->DampedCcOffset.fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE] = 0.0;

		CamerIcXTalkOffset_t    XTalkOffset;
		result = AwbXTalkOffset2CamerIcXTalkOffset( &pAwbCtx->DampedCcOffset, &XTalkOffset );
		RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
		
		result = CamerIcIspSetCrossTalkOffset( pAwbCtx->hCamerIc, &XTalkOffset );
   		RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
		
	}
	
	
	TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__ );
	
	return ( RET_SUCCESS );

}



/******************************************************************************
 * AwbProcessFrame()
 *****************************************************************************/
RESULT AwbProcessFrame
(
    AwbHandle_t                         handle,
    const CamerIcAwbMeasuringResult_t   *pMeasResult,
    const float                         fGain,
    const float                         fIntegrationTime
)
{
    AwbContext_t *pAwbCtx = (AwbContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pAwbCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pMeasResult == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    if ( (pAwbCtx->Mode == AWB_MODE_AUTO) && (pAwbCtx->state == AWB_STATE_RUNNING) )
    {
        uint32_t d = (((int32_t)pAwbCtx->NoWhitePixel - (int32_t)pMeasResult->NoWhitePixel) < 0 )
                        ? ( pMeasResult->NoWhitePixel - pAwbCtx->NoWhitePixel )
                        : ( pAwbCtx->NoWhitePixel - pMeasResult->NoWhitePixel );

        pAwbCtx->SensorGain         = fGain;
        pAwbCtx->IntegrationTime    = fIntegrationTime;

        result = AwbPrepareInputValues( pAwbCtx, pMeasResult );
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

        pAwbCtx->WhitePixelEvaluation = AwbWpNumCheck( pAwbCtx, pMeasResult->NoWhitePixel );
        TRACE( AWB_DEBUG, "white pixel evaluation: %d\n", pAwbCtx->WhitePixelEvaluation );

        result = AwbExpPriorProcessFrame( pAwbCtx );
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

        TRACE( AWB_DEBUG, "p_in=%f p_out=%f doortype=%d C_damp=%f\n",
                pAwbCtx->ExpPriorIn, pAwbCtx->ExpPriorOut, pAwbCtx->DoorType, pAwbCtx->AwbIIRDampCoef );

        result = AwbWpRevertProcessFrame( pAwbCtx );
        TRACE( AWB_DEBUG, "Result = %d\n", result );
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

        TRACE( AWB_DEBUG, "Reverted Means (R:%f, G:%f, B:%f )\n",
                pAwbCtx->RevertedMeansRgb.fRed, pAwbCtx->RevertedMeansRgb.fGreen, pAwbCtx->RevertedMeansRgb.fBlue );

        result = AwbIlluEstProcessFrame( pAwbCtx );
		//AwbCCOffsetCal(pAwbCtx);//oyyf add
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

        result = AwbWbGainProcessFrame( pAwbCtx );
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

        TRACE( AWB_DEBUG, "Gain (R:%f, Gr:%f, Gb:%f,  B:%f )\n",
                pAwbCtx->WbGains.fRed, pAwbCtx->WbGains.fGreenR, pAwbCtx->WbGains.fGreenB, pAwbCtx->WbGains.fBlue );

        result = AwbAccProcessFrame( pAwbCtx );		
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

        TRACE( AWB_DEBUG, "CC-Matrix ( %f, %f, %f, %f, %f, %f, %f, %f, %f )\n",
                pAwbCtx->DampedCcMatrix.fCoeff[0], pAwbCtx->DampedCcMatrix.fCoeff[1], pAwbCtx->DampedCcMatrix.fCoeff[2],
                pAwbCtx->DampedCcMatrix.fCoeff[3], pAwbCtx->DampedCcMatrix.fCoeff[4], pAwbCtx->DampedCcMatrix.fCoeff[5],
                pAwbCtx->DampedCcMatrix.fCoeff[6], pAwbCtx->DampedCcMatrix.fCoeff[7], pAwbCtx->DampedCcMatrix.fCoeff[8] );

        TRACE( AWB_DEBUG, "CC-Offsets ( %f, %f, %f )\n",
                pAwbCtx->UndampedCcOffset.fCoeff[CAM_3CH_COLOR_COMPONENT_RED],
                pAwbCtx->UndampedCcOffset.fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN],
                pAwbCtx->UndampedCcOffset.fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE] );

        TRACE( AWB_DEBUG, "CC-Offsets ( %f, %f, %f )\n",
                pAwbCtx->DampedCcOffset.fCoeff[CAM_3CH_COLOR_COMPONENT_RED],
                pAwbCtx->DampedCcOffset.fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN],
                pAwbCtx->DampedCcOffset.fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE] );

        result = AwbAlscProcessFrame( pAwbCtx );
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

        TRACE( AWB_DEBUG, "LSC-Matrices \n RED   : %u, %u, %u, ... \n GREENR: %u, %u, %u, ... \n GREENB: %u, %u, %u, ... \n BLUE  : %u, %u, %u, ... \n",
                pAwbCtx->UndampedLscMatrixTable.LscMatrix[ISI_COLOR_COMPONENT_RED].uCoeff[0],
                pAwbCtx->UndampedLscMatrixTable.LscMatrix[ISI_COLOR_COMPONENT_RED].uCoeff[1],
                pAwbCtx->UndampedLscMatrixTable.LscMatrix[ISI_COLOR_COMPONENT_RED].uCoeff[2],
                pAwbCtx->UndampedLscMatrixTable.LscMatrix[ISI_COLOR_COMPONENT_GREENR].uCoeff[0],
                pAwbCtx->UndampedLscMatrixTable.LscMatrix[ISI_COLOR_COMPONENT_GREENR].uCoeff[1],
                pAwbCtx->UndampedLscMatrixTable.LscMatrix[ISI_COLOR_COMPONENT_GREENR].uCoeff[2],
                pAwbCtx->UndampedLscMatrixTable.LscMatrix[ISI_COLOR_COMPONENT_GREENB].uCoeff[0],
                pAwbCtx->UndampedLscMatrixTable.LscMatrix[ISI_COLOR_COMPONENT_GREENB].uCoeff[1],
                pAwbCtx->UndampedLscMatrixTable.LscMatrix[ISI_COLOR_COMPONENT_GREENB].uCoeff[2],
                pAwbCtx->UndampedLscMatrixTable.LscMatrix[ISI_COLOR_COMPONENT_BLUE].uCoeff[0],
                pAwbCtx->UndampedLscMatrixTable.LscMatrix[ISI_COLOR_COMPONENT_BLUE].uCoeff[1],
                pAwbCtx->UndampedLscMatrixTable.LscMatrix[ISI_COLOR_COMPONENT_BLUE].uCoeff[2] );

        result = AwbWpRegionAdaptProcessFrame( pAwbCtx );
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

        float BlsGain = ( 4095.0f ) / ( 4095.0f - 66.0f );
        pAwbCtx->WbGains.fRed    *= BlsGain;
        pAwbCtx->WbGains.fGreenR *= BlsGain;
        pAwbCtx->WbGains.fGreenB *= BlsGain;
        pAwbCtx->WbGains.fBlue   *= BlsGain;

        TRACE( AWB_DEBUG, "Gain (R:%f, Gr:%f, Gb:%f,  B:%f )\n",
                pAwbCtx->WbGains.fRed, pAwbCtx->WbGains.fGreenR, pAwbCtx->WbGains.fGreenB, pAwbCtx->WbGains.fBlue );

        result = AwbSetValues( pAwbCtx );
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

        pAwbCtx->NoWhitePixel = pMeasResult->NoWhitePixel;

        TRACE( AWB_DEBUG, "RES: %d %d %d %d %f %u %u %u %u %u\n",
                pMeasResult->NoWhitePixel,
                pMeasResult->MeanY__G,
                pMeasResult->MeanCb__B,
                pMeasResult->MeanCr__R,
                pAwbCtx->RegionSize,
                pAwbCtx->MeasWdw.RefCr_MaxR,
                pAwbCtx->MeasWdw.RefCb_MaxB,
                pAwbCtx->MeasWdw.MinC,
                pAwbCtx->MeasWdw.MaxCSum,
                pAwbCtx->MeasWdw.MaxY );

        pAwbCtx->NoWhitePixel       = pMeasResult->NoWhitePixel;
        pAwbCtx->dNoWhitePixel      = d;
        pAwbCtx->StableThreshold    = (uint32_t)((float)pAwbCtx->NoWhitePixel * pAwbCtx->fStableDeviation);
        pAwbCtx->RestartThreshold   = (uint32_t)((float)pAwbCtx->NoWhitePixel * pAwbCtx->fRestartDeviation);
    }
    else
    {
        result = RET_CANCELED;
    }

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AwbTryLock()
 *****************************************************************************/
RESULT AwbTryLock
(
    AwbHandle_t handle
)
{
    AwbContext_t *pAwbCtx = (AwbContext_t *)handle;

    RESULT result = RET_FAILURE;
    bool_t settled = BOOL_FALSE;
	uint32_t dNoWhitePixel = 0;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pAwbCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    // check state
    if ( (AWB_STATE_RUNNING != pAwbCtx->state)
            && (AWB_STATE_LOCKED != pAwbCtx->state) )
    {
        return ( RET_WRONG_STATE );
    }

    // check working mode
    if ( pAwbCtx->Mode == AWB_MODE_AUTO )
    {
        result = AwbSettled( handle, &settled, &dNoWhitePixel );
        if ( (RET_SUCCESS == result) && (BOOL_TRUE == settled) )
        {
            pAwbCtx->state = AWB_STATE_LOCKED;
            result = RET_SUCCESS;
        }
        else
        {
            result = RET_PENDING;
        }
    }
    else
    {
        result = RET_SUCCESS;
    }

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AwbUnLock()
 *****************************************************************************/
RESULT AwbUnLock
(
    AwbHandle_t handle
)
{
    AwbContext_t *pAwbCtx = (AwbContext_t *)handle;

    RESULT result = RET_FAILURE;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pAwbCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (AWB_STATE_LOCKED == pAwbCtx->state)
            || (AWB_STATE_RUNNING == pAwbCtx->state) )
    {
        pAwbCtx->state = AWB_STATE_RUNNING;
        result = RET_SUCCESS;
    }
    else if ( AWB_STATE_STOPPED == pAwbCtx->state )
    {
        result = RET_SUCCESS;
    }
    else
    {
        result = RET_WRONG_STATE;
    }

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

RESULT AwbGetGainParam
(
	AwbHandle_t handle,
	float *f_RgProj, 
	float *f_s, 
	float *f_s_Max1, 
	float *f_s_Max2, 
	float *f_Bg1, 
	float *f_Rg1, 
	float *f_Bg2, 
	float *f_Rg2
)
{
	AwbContext_t *pAwbCtx = (AwbContext_t *)handle;

	*f_RgProj = pAwbCtx->RgProj;
	*f_s = pAwbCtx->Wb_s;
	*f_s_Max1 = pAwbCtx->Wb_s_max1;
	*f_s_Max2 = pAwbCtx->Wb_s_max2;
	*f_Bg1 = pAwbCtx->WbBg;
	*f_Rg1 = pAwbCtx->WbRg;	
	*f_Bg2 = pAwbCtx->WbClippedGainsOverG.GainBOverG;
	*f_Rg2 = pAwbCtx->WbClippedGainsOverG.GainROverG;

	return ( RET_SUCCESS );
}

RESULT AwbGetIlluEstInfo
(
	AwbHandle_t handle,
	float *ExpPriorIn,
	float *ExpPriorOut,
	char (*name)[20],
	float likehood[],
	float wight[],
	int *curIdx,
	int *region,
	int *count
)
{
	int i;
	AwbContext_t *pAwbCtx = (AwbContext_t *)handle;
	
	*ExpPriorIn = pAwbCtx->ExpPriorIn;
	*ExpPriorOut = pAwbCtx->ExpPriorOut;
	*region = pAwbCtx->Region;
	*curIdx = pAwbCtx->DominateIlluProfileIdx;
	*count = pAwbCtx->NoIlluProfiles;
	for(i=0; i<pAwbCtx->NoIlluProfiles; i++)
	{
		memset(name[i],0x00,20);
		strncpy(name[i], pAwbCtx->pIlluProfiles[i]->name,19);
		(likehood)[i] = pAwbCtx->LikeHood[i];
		(wight)[i] = pAwbCtx->Weight[i];
	}
	return ( RET_SUCCESS );

}

/*************************************************************************
 最小二乘法拟合直线，y = a*x + b; n组数据; r-相关系数[-1,1],fabs(r)->1,说明x,y之间线性关系好，fabs(r)->0，x,y之间无线性关系，拟合无意义
 a = (n*C - B*D) / (n*A - B*B)
 b = (A*D - B*C) / (n*A - B*B)
 r = E / F
 其中：
 A = sum(Xi * Xi)
 B = sum(Xi)
 C = sum(Xi * Yi)
 D = sum(Yi)
 E = sum((Xi - Xmean)*(Yi - Ymean))
 F = sqrt(sum((Xi - Xmean)*(Xi - Xmean))) * sqrt(sum((Yi - Ymean)*(Yi - Ymean)))

**************************************************************************/
void LineFitLeastSquares(float *data_x, float *data_y, int data_n,float *a,float *b)
{
   TRACE( AWB_INFO, "%s: (Enter)\n", __FUNCTION__);
	float A = 0.0;
	float B = 0.0;
	float C = 0.0;
	float D = 0.0;
	float E = 0.0;
	float F = 0.0;

	for (int i=0; i<data_n; i++)
	{
		A += data_x[i] * data_x[i];
		B += data_x[i];
		C += data_x[i] * data_y[i];
		D += data_y[i];
	}

	// 计算斜率a和截距b
	float temp = 0;
	if( temp = (data_n*A - B*B) )// 判断分母不为0
	{
		*a = (data_n*C - B*D) / temp;
		*b = (A*D - B*C) / temp;
	}
	else
	{
		*a = 1;
		*b = 0;
	}

	// 计算相关系数r
	/*float Xmean, Ymean;
	Xmean = B / data_n;
	Ymean = D / data_n;

	float tempSumXX = 0.0, tempSumYY = 0.0;
	for (int i=0; i<data_n; i++)
	{
		tempSumXX += (data_x[i] - Xmean) * (data_x[i] - Xmean);
		tempSumYY += (data_y[i] - Ymean) * (data_y[i] - Ymean);
		E += (data_x[i] - Xmean) * (data_y[i] - Ymean);
	}
	F = sqrt(tempSumXX) * sqrt(tempSumYY);

	float r;
	r = E / F;*/
	//TRACE( AWB_INFO, "%s: fit line y=(%f)x+(%f), \n", __FUNCTION__,*a,*b,);
	TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

}
RESULT AwbFitTemperatureRgLine
(
	AwbHandle_t handle	
)
{
	TRACE( AWB_INFO, "%s: (Enter)\n", __FUNCTION__);
	AwbContext_t *pAwbCtx = (AwbContext_t *)handle;
	RESULT result;
    int illNO,i;
	int j;
	//get (rg,bg) of illu
	float fRed,fGreenR,tmp;
	float *Rg;//R/G	
	float *TempSet;
	float k,b;
	illNO=pAwbCtx->NoIlluProfiles;
	Rg=(float *)malloc(sizeof(float)*illNO);
	
	TempSet=(float *)malloc(sizeof(float)*illNO);
	int illNO_use=0;	
	TRACE( AWB_INFO, "%s: illNO(%D)\n", __FUNCTION__,illNO);
	for(i=0;i<illNO;i++){		
		TRACE( AWB_INFO, "%s: profiles_%d,%s\n", __FUNCTION__,i,pAwbCtx->pIlluProfiles[i]->name);
		if (strcmp(pAwbCtx->pIlluProfiles[i]->name,"A") == 0){
		  	TempSet[illNO_use]=2856;
		}else if(strcmp(pAwbCtx->pIlluProfiles[i]->name,"D65") == 0){
			TempSet[illNO_use]=6500;			
		}else if((strcmp(pAwbCtx->pIlluProfiles[i]->name,"F11_TL84") == 0)||(strcmp(pAwbCtx->pIlluProfiles[i]->name,"TL84") == 0)){
			TempSet[illNO_use]=4100;
		}//else if(!strcmp(pAwbCtx->pIlluProfiles[i]->name,"F2_CWF")){
			//TempSet[i]=4150;
			//}
		else{
			continue;
		}		
		fRed       = pAwbCtx->pIlluProfiles[i]->ComponentGain.fCoeff[CAM_4CH_COLOR_COMPONENT_RED];
        fGreenR    = pAwbCtx->pIlluProfiles[i]->ComponentGain.fCoeff[CAM_4CH_COLOR_COMPONENT_GREENR]; 		
		tmp=fRed/fGreenR;
		Rg[illNO_use]=tmp;
		TRACE( AWB_INFO, "%s: TempSet[%d]: %f \n", __FUNCTION__,illNO_use,TempSet[illNO_use]);
		TRACE( AWB_INFO, "%s: use_%d,%s ,fRed(%f),fGreenR(%f)\n", __FUNCTION__,i,pAwbCtx->pIlluProfiles[i]->name,fRed,fGreenR);
		TRACE( AWB_INFO, "%s: Rg[%d]: %f \n", __FUNCTION__,illNO_use,Rg[illNO_use]);
		illNO_use=illNO_use+1;		  
		}
	if (illNO_use<2){		
		TRACE( AWB_INFO, "%s: illNO_use(%d)\n", __FUNCTION__,illNO_use);
		return (RET_FAILURE);
    	}	
    LineFitLeastSquares(Rg,TempSet,illNO_use,&k,&b);
	pAwbCtx->TemRgLine_k=k;
	pAwbCtx->TemRgLine_b=b;
	pAwbCtx->TemRgLine_CalFlag=BOOL_TRUE;
	free(Rg);
	free(TempSet);
	TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);
	return ( RET_SUCCESS );	

}
RESULT AwbGetTemperature
(
	AwbHandle_t handle,	
	float *ct
)
{
	TRACE( AWB_INFO, "%s: (Enter)\n", __FUNCTION__);
	AwbContext_t *pAwbCtx = (AwbContext_t *)handle;
	float Rg;
	if (pAwbCtx->TemRgLine_CalFlag!=BOOL_TRUE){
		return (RET_FAILURE);
		}
	//get gain;	
	Rg=pAwbCtx->WbGains.fRed/pAwbCtx->WbGains.fGreenR;
    *ct=pAwbCtx->TemRgLine_k*Rg+pAwbCtx->TemRgLine_b;	
   	TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);
	return ( RET_SUCCESS );	
}
RESULT AwbCalcWBgainbyCT
(
	AwbHandle_t handle,	
	float ct,
	float * Rg,
	float *Bg
)
{
	TRACE( AWB_INFO, "%s: (Enter)\n", __FUNCTION__);
	AwbContext_t *pAwbCtx = (AwbContext_t *)handle;    
	if (pAwbCtx->TemRgLine_CalFlag!=BOOL_TRUE){
		return (RET_FAILURE);
		}
	if(ABS_DIFF(pAwbCtx->TemRgLine_k,0)<= 1e-7||ABS_DIFF(pAwbCtx->pCenterLine->f_N0_Bg,0)<= 1e-7){
		return(RET_DIVISION_BY_ZERO);
		}    	
	*Rg=(ct-pAwbCtx->TemRgLine_b)/pAwbCtx->TemRgLine_k;
	*Bg=(pAwbCtx->pCenterLine->f_d-pAwbCtx->pCenterLine->f_N0_Rg * (*Rg))/pAwbCtx->pCenterLine->f_N0_Bg;	
   	TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);
	return ( RET_SUCCESS );	

}

