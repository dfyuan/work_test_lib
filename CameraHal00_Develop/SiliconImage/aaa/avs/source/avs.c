/******************************************************************************
 *
 * Copyright 2012, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
/**
 * @file avs.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <math.h>

#include <common/return_codes.h>
#include <common/misc.h>

#include <cameric_drv/cameric_isp_is_drv_api.h>



#include "avs.h"
#include "avs_ctrl.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( AVS_INFO  , "AVS: ", INFO     , 0 );
CREATE_TRACER( AVS_WARN  , "AVS: ", WARNING  , 1 );
CREATE_TRACER( AVS_ERROR , "AVS: ", ERROR    , 1 );

CREATE_TRACER( AVS_DEBUG , ""    , INFO     , 0 );


/******************************************************************************
 * local type definitions
 *****************************************************************************/


/******************************************************************************
 * local variable declarations
 *****************************************************************************/


/******************************************************************************
 * local functions
 *****************************************************************************/

/*****************************************************************************/
/**
 *          AvsCheckDampingFunctionLut()
 *
 * @brief   Check the correctness of a damping function lookup table.
 *
 * @param   lutSize     Number of elements in the LUT
 * @param   pDampLut    Pointer to LUT
 *
 * @return  Returns the result of the check operation.
 * @retval  RET_SUCCESS
 * @retval  RET_INVALID_PARM
 * @retval  RET_WRONG_CONFIG
 *****************************************************************************/
static RESULT AvsCheckDampingFunctionLut
(
    int8_t             lutSize,
    AvsDampingPoint_t *pDampLut
)
{
    int8_t i;
    float lastOffset = 0;

    TRACE( AVS_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pDampLut == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    if ( lutSize <= 0 )
    {
        return ( RET_WRONG_CONFIG );
    }

    for (i = 0; i < lutSize; i++)
    {
        if ( pDampLut[i].offset < lastOffset )
        {
            /* offsets shall be monotonically increasing */
            return ( RET_WRONG_CONFIG );
        }
        if ( pDampLut[i].offset > 1.0 )
        {
            return ( RET_WRONG_CONFIG );
        }
        if ( pDampLut[i].value < 0 )
        {
            return ( RET_WRONG_CONFIG );
        }
        lastOffset = pDampLut[i].offset;
    }

    TRACE( AVS_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/*****************************************************************************/
/**
 *          AvsCheckDampingFunctionParams()
 *
 * @brief   Check the correctness of parameters for the default damping function
 *
 * @param   pDampParams Pointer to parameters to check
 *
 * @return  Returns the result of the check operation.
 * @retval  RET_SUCCESS
 * @retval  RET_INVALID_PARM
 * @retval  RET_WRONG_CONFIG
 *****************************************************************************/
static RESULT AvsCheckDampingFunctionParams
(
    AvsDampFuncParams_t *pDampParams
)
{
    TRACE( AVS_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pDampParams == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    if ( pDampParams->numItpPoints == 0 )
    {
        return ( RET_WRONG_CONFIG );
    }

    if ( (pDampParams->theta < 0) || (pDampParams->theta > 1) )
    {
        return ( RET_WRONG_CONFIG );
    }

    if ( (pDampParams->baseGain < 0) || (pDampParams->baseGain > 1) )
    {
        return ( RET_WRONG_CONFIG );
    }

    if ( pDampParams->fallOff > pDampParams->baseGain )
    {
        return ( RET_WRONG_CONFIG );
    }

    if ( pDampParams->acceleration < 0 )
    {
        return ( RET_WRONG_CONFIG );
    }

    TRACE( AVS_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/*****************************************************************************/
/**
 *          AvsCalcDampingFunctionLut()
 *
 * @brief   Calculate a damping function lookup table
 *
 * @param   lutConfSize   Size of LUT pointed to by pLutConf.
 * @param   pLutConf      Configured LUT, can be NULL.
 * @param   pParams       Configured parameters for default damping function in
 *                        case pLutConf is NULL.
 * @param   pLutSizeStore Place to store the calculated LUT size.
 * @param   ppLutStore    Place to store the calculated LUT pointer.
 * @param   pParamsStore  Place to store the passed parameters for default
 *                        damping function.
 *
 * @return  Returns the result of the operation.
 * @retval  RET_SUCCESS
 * @retval  RET_OUTOFMEM
 *****************************************************************************/
static RESULT AvsCalcDampingFunctionLut
(
    uint16_t             lutConfSize,
    AvsDampingPoint_t   *pLutConf,
    AvsDampFuncParams_t *pParams,
    uint16_t            *pLutSizeStore,
    AvsDampingPoint_t  **ppLutStore,
    AvsDampFuncParams_t *pParamsStore
)
{
    AvsDampingPoint_t *pLutCreated;
    uint32_t           lutSizeInBytes;

    TRACE( AVS_INFO, "%s: (enter)\n", __FUNCTION__);

    DCT_ASSERT( pLutSizeStore != NULL );
    DCT_ASSERT( ppLutStore != NULL );
    DCT_ASSERT( pParamsStore != NULL );

    if ( pLutConf != NULL )
    {
        lutSizeInBytes = lutConfSize * sizeof(AvsDampingPoint_t);
        /* simply create copy of configured LUT */
        pLutCreated = ( AvsDampingPoint_t* ) malloc( lutSizeInBytes );
        if ( pLutCreated == NULL )
        {
            return ( RET_OUTOFMEM );
        }

        MEMCPY( pLutCreated, pLutConf, lutSizeInBytes );

        *pLutSizeStore = lutConfSize;
        *ppLutStore    = pLutCreated;
    }
    else
    {
        int32_t i;

        /* create LUT by using parametrized default damping function */
        if ( pParams != NULL )
        {
            *pParamsStore = *pParams;
            pParams = pParamsStore;
        }
        else
        {
            pParams = pParamsStore;
            MEMSET( pParams, 0, sizeof(AvsDampFuncParams_t) );
            pParams->numItpPoints = NUM_ITP_POINTS_DEFAULT;
            pParams->theta        = THETA_DEFAULT;
            pParams->baseGain     = BASE_GAIN_DEFAULT;
            pParams->fallOff      = FALL_OFF_DEFAULT;
            pParams->acceleration = ACCELERATION_DEFAULT;
        }

        lutSizeInBytes = pParams->numItpPoints * sizeof(AvsDampingPoint_t);
        pLutCreated = ( AvsDampingPoint_t* ) malloc( lutSizeInBytes );
        if ( pLutCreated == NULL )
        {
            return ( RET_OUTOFMEM );
        }
        MEMSET( pLutCreated, 0, lutSizeInBytes );

        /* see documentation of AvsDampFuncParams_t */
        pLutCreated[0].offset = pParams->theta;
        pLutCreated[0].value  = pParams->baseGain;

        if ( pParams->numItpPoints > 1 )
        {
            uint16_t numMinus1 = pParams->numItpPoints - 1;
            float stepSize = ( 1.0 - pParams->theta ) / numMinus1;
            for (i = 1; i < numMinus1; i++)
            {
                float v;
                float sineVal;
                pLutCreated[i].offset = pLutCreated[i-1].offset + stepSize;
                v = pLutCreated[i].offset - pParams->theta;
                /* Note: 1.5708 is ~ PI/2, in precision which fits for float */
                sineVal = sinf( (v*1.5708) / (1-pParams->theta) );
                pLutCreated[i].value = pParams->baseGain -
                    ( pParams->fallOff * powf(sineVal, pParams->acceleration) );
            }
            pLutCreated[numMinus1].offset = 1.0;
            pLutCreated[numMinus1].value  =
                pParams->baseGain - pParams->fallOff;
        }

        *pLutSizeStore = pParams->numItpPoints;
        *ppLutStore    = pLutCreated;
    }

    TRACE( AVS_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/*****************************************************************************/
/**
 *          AvsSetCroppingWindowInHw()
 *
 * @brief   Calculate the cropping window according to the current offset
 *          value and call driver functions to apply it to HW performing
 *          the cropping.
 *
 * @param   pAvsCtx       Pointer to Avs context
 * @param   pOffsetVec    Pointer to offset vector to use for cropping window
 *                        calculation
 *
 *****************************************************************************/
static void AvsSetCroppingWindowInHw
(
    AvsContext_t      *pAvsCtx,
    CamEngineVector_t *pOffsetVec
)
{
    RESULT resCrop = RET_SUCCESS;

    TRACE( AVS_INFO, "%s: (enter)\n", __FUNCTION__);

    /* calculate offsets from left and top; note that the offsVec.x and
     * offsVec.y values are already clipped before so that the additions
     * do not produce out-of-range results */
    pAvsCtx->cropWin.hOffset =
        pAvsCtx->config.dstWindow.hOffset + pOffsetVec->x;
    pAvsCtx->cropWin.vOffset =
        pAvsCtx->config.dstWindow.vOffset + pOffsetVec->y;

    TRACE( AVS_INFO, "crop offset hor: %d + %d = %d \n",
        pAvsCtx->config.dstWindow.hOffset, pOffsetVec->x,
        pAvsCtx->cropWin.hOffset);
    TRACE( AVS_INFO, "crop offset ver: %d + %d = %d \n",
        pAvsCtx->config.dstWindow.vOffset, pOffsetVec->y,
        pAvsCtx->cropWin.vOffset);

    /* Call IS driver to set cropping window. */
    pAvsCtx->cropWinIs.hOffset = pAvsCtx->cropWin.hOffset;
    pAvsCtx->cropWinIs.vOffset = pAvsCtx->cropWin.vOffset;

    TRACE( AVS_DEBUG, "%d;%d;", pAvsCtx->cropWinIs.hOffset,
        pAvsCtx->cropWinIs.vOffset);

    resCrop = CamerIcIspIsSetOutputWindow( pAvsCtx->hSubCamerIc,
        &pAvsCtx->cropWinIs, BOOL_TRUE );

    TRACE( AVS_INFO, "resCrop = %d\n", resCrop);

    TRACE( AVS_INFO, "%s: (exit)\n", __FUNCTION__);
}



/*****************************************************************************/
/**
 *          AvsApplyConfiguration()
 *
 * @brief   This function configures the Auto Video Stabilization Module.
 *
 * @param   handle      Handle to AVSMresCrop
 * @param   pConfig     Pointer to configuration structure to be applied
 * @param   reconfigure Whether the function is called by AvsReConfigure
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_INVALID_PARM
 * @retval  RET_WRONG_STATE
 * @retval  RET_OUTOFRANGE
 *
 *****************************************************************************/
static RESULT AvsApplyConfiguration
(
    AvsHandle_t  handle,
    AvsConfig_t *pConfig,
    bool_t       reconfigure
)
{
    RESULT result = RET_SUCCESS;

    AvsContext_t *pAvsCtx = (AvsContext_t*)handle;

    TRACE( AVS_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAvsCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check state */
    if ( (AVS_STATE_INITIALIZED != pAvsCtx->state) &&
         ( AVS_STATE_STOPPED != pAvsCtx->state) &&
         (!reconfigure || (AVS_STATE_RUNNING != pAvsCtx->state)) )
    {
        return ( RET_WRONG_STATE );
    }


    if ( (pConfig == NULL) || (pConfig->hCamerIc == NULL) || (pConfig->hSubCamerIc == NULL) )
    {
        return ( RET_INVALID_PARM );
    }
    
    pAvsCtx->hCamerIc    = pConfig->hCamerIc;
    pAvsCtx->hSubCamerIc = pConfig->hSubCamerIc;

    /* check source and destination window */
    if ( (pConfig->srcWindow.width == 0) ||
         (pConfig->srcWindow.height == 0) ||
         (pConfig->dstWindow.width == 0) ||
         (pConfig->dstWindow.height == 0) ||
         (pConfig->dstWindow.width + pConfig->dstWindow.hOffset >
          pConfig->srcWindow.width) ||
         (pConfig->dstWindow.height + pConfig->dstWindow.vOffset >
          pConfig->srcWindow.height) )
    {
        TRACE( AVS_ERROR, "%s: src(%d,%d,%d,%d) dst(%d,%d,%d,%d)\n",
                __FUNCTION__,
                pConfig->srcWindow.hOffset, pConfig->srcWindow.vOffset, 
                pConfig->srcWindow.width, pConfig->srcWindow.height,
                pConfig->dstWindow.hOffset, pConfig->dstWindow.vOffset, 
                pConfig->dstWindow.width, pConfig->dstWindow.height );
        return ( RET_OUTOFRANGE );
    }

    /* calculate maximum offsets from center position */
    pAvsCtx->maxAbsOffsetHor =
        (pConfig->srcWindow.width - pConfig->dstWindow.width) >> 1;
    pAvsCtx->maxAbsOffsetVer =
        (pConfig->srcWindow.height - pConfig->dstWindow.height) >> 1;

    /* our current implementation only works if the destination window
     * is centered at the beginning, since otherwise we would need two
     * maximum offset values for each direction */
    if ( pConfig->dstWindow.hOffset != pAvsCtx->maxAbsOffsetHor )
    {
        return ( RET_WRONG_CONFIG );
    }
    if ( pConfig->dstWindow.vOffset != pAvsCtx->maxAbsOffsetVer )
    {
        return ( RET_WRONG_CONFIG );
    }

    /* check offset data array size */
    if ( pConfig->offsetDataArraySize <= 0 )
    {
        return ( RET_WRONG_CONFIG );
    }

    /* check horizontal damping function LUT */
    if ( pConfig->pDampLutHor != NULL )
    {
        result = AvsCheckDampingFunctionLut( pConfig->dampLutHorSize,
        pConfig->pDampLutHor );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
    }
    else if ( pConfig->pDampParamsHor != NULL ) {
        /* check horizontal default damping function parameters */
        result = AvsCheckDampingFunctionParams( pConfig->pDampParamsHor );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
    }

    /* check vertical damping function LUT */
    if ( pConfig->pDampLutVer != NULL )
    {
        result = AvsCheckDampingFunctionLut( pConfig->dampLutVerSize,
            pConfig->pDampLutVer );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
    }
    else if ( pConfig->pDampParamsHor != NULL ) {
        /* check horizontal default damping function parameters */
        result = AvsCheckDampingFunctionParams( pConfig->pDampParamsVer );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
    }

    /* save configuration */
    pAvsCtx->config  = *pConfig;
    /* copy destination window to cropping window */
    pAvsCtx->cropWin = pConfig->dstWindow;
    /* we need to copy the elements of our two cropping window structures one by
     * one since though they have the same exact elements, the types are not the
     * same */
    pAvsCtx->cropWinIs.hOffset = pAvsCtx->cropWin.hOffset;
    pAvsCtx->cropWinIs.vOffset = pAvsCtx->cropWin.vOffset;
    pAvsCtx->cropWinIs.width = pAvsCtx->cropWin.width;
    pAvsCtx->cropWinIs.height = pAvsCtx->cropWin.height;

    /* allocate offset data array */
    {
        uint32_t arraySizeInBytes =
            pConfig->offsetDataArraySize * sizeof(AvsOffsetData_t);

        if ( pAvsCtx->pOffsetData != NULL )
        {
            free( pAvsCtx->pOffsetData );
        }
        pAvsCtx->pOffsetData = (AvsOffsetData_t*) malloc( arraySizeInBytes );
        if ( pAvsCtx->pOffsetData == NULL )
        {
            return ( RET_OUTOFMEM );
        }
        MEMSET( pAvsCtx->pOffsetData, 0, arraySizeInBytes );
    }

    /* calculate horizontal damping function LUT */
    result = AvsCalcDampingFunctionLut( pConfig->dampLutHorSize,
        pConfig->pDampLutHor, pConfig->pDampParamsHor, &pAvsCtx->dampLutHorSize,
        &pAvsCtx->pDampLutHor, &pAvsCtx->dampHorParams );
    if ( result != RET_SUCCESS )
    {
        DCT_ASSERT( pAvsCtx->pOffsetData != NULL );
        free( pAvsCtx->pOffsetData );
        return ( result );
    }
    /* Record pointers to LUT and parameters in config in order to provide the
     * correct values to a caller of AvsGetConfig. */
    pAvsCtx->config.pDampLutHor = pAvsCtx->pDampLutHor;
    if ( pConfig->pDampLutHor == NULL )
    {
        pAvsCtx->config.pDampParamsHor = &pAvsCtx->dampHorParams;
    }
    else
    {
        pAvsCtx->config.pDampParamsHor = NULL;
    }

    /* calculate vertical damping function LUT */
    result = AvsCalcDampingFunctionLut( pConfig->dampLutVerSize,
        pConfig->pDampLutVer, pConfig->pDampParamsVer, &pAvsCtx->dampLutVerSize,
        &pAvsCtx->pDampLutVer, &pAvsCtx->dampVerParams );
    if ( result != RET_SUCCESS )
    {
        DCT_ASSERT( pAvsCtx->pOffsetData != NULL );
        free( pAvsCtx->pOffsetData );
        DCT_ASSERT( pAvsCtx->pDampLutHor != NULL );
        free( pAvsCtx->pDampLutHor );
        return ( result );
    }
    /* Record pointers to LUT and parameters in config in order to provide the
     * correct values to a caller of AvsGetConfig. */
    pAvsCtx->config.pDampLutVer = pAvsCtx->pDampLutVer;
    if ( pConfig->pDampLutVer == NULL )
    {
        pAvsCtx->config.pDampParamsVer = &pAvsCtx->dampVerParams;
    }
    else
    {
        pAvsCtx->config.pDampParamsVer = NULL;
    }

    /* set current offsets to zero*/
    pAvsCtx->offsVec.x = pAvsCtx->offsVec.y = 0;

    /* set initial cropping window in image stabilization hardware */
    AvsSetCroppingWindowInHw( pAvsCtx, &pAvsCtx->offsVec );

    if ( AVS_STATE_INITIALIZED == pAvsCtx->state )
    {
        pAvsCtx->state = AVS_STATE_STOPPED;
    }

    TRACE( AVS_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          AvsDampingFunction()
 *
 * @brief   Get the value of the damping function for a specific offset.
 *
 * @param   lutSize     Number of elements in the Lut
 * @param   pDampLut    Pointer to Lut
 * @param   offset      Argument of the function for which its value shall be
 *                      determined
 * @param   maxOffset   Maximum possible offset
 *
 * @return  Returns the value of the function for the given offset.
 *****************************************************************************/
static float AvsDampingFunction
(
    int8_t             lutSize,
    AvsDampingPoint_t *pDampLut,
    int16_t            offset,
    uint16_t           maxOffset
)
{
    float absOffset;
    float offsetLower, offsetHigher;
    float valueLower, valueHigher;
    float retVal;
    int8_t i;

    TRACE( AVS_INFO, "%s: (enter)\n", __FUNCTION__);

    TRACE( AVS_INFO, "offset = %d\n", offset);
    TRACE( AVS_INFO, "maxOffset = %d\n", maxOffset);

    DCT_ASSERT( maxOffset > 0 );

    absOffset = abs( offset ) / (float) maxOffset;

    TRACE( AVS_INFO, "absOffset = %f\n", absOffset);

    DCT_ASSERT( pDampLut != NULL );
    /* Note: If pDampLut[0].offset should be > 0, we assume that the value
     * for offset 0 also is the value for pDampLut[0].offset. */
    offsetLower  = 0;
    valueLower   = pDampLut[0].value;
    offsetHigher = pDampLut[lutSize-1].offset;
    valueHigher  = pDampLut[lutSize-1].value;

    for ( i = 0; i < lutSize; i++ ) {
        if ( absOffset < pDampLut[i].offset )
        {
            offsetHigher = pDampLut[i].offset;
            valueHigher  = pDampLut[i].value;
            break;
        }
        offsetLower = pDampLut[i].offset;
        valueLower  = pDampLut[i].value;
    }

    TRACE( AVS_INFO, "offsetLower  = %.f\n", offset);
    TRACE( AVS_INFO, "valueLower   = %.f\n", offset);
    TRACE( AVS_INFO, "offsetHigher = %.f\n", offset);
    TRACE( AVS_INFO, "valueHigher  = %.f\n", offset);

    if ( absOffset >= offsetHigher )
    {
        retVal= valueHigher;
    }
    else
    {
        float offsetRelative = absOffset - offsetLower;
        float segmentLength  = offsetHigher - offsetLower;
        float valueDiff = valueHigher - valueLower;
        float offsetNormed;
        DCT_ASSERT( offsetRelative >= 0 );
        DCT_ASSERT( segmentLength > 0 );
        /* offsetNormed is a number between 0.0 and 1.0 */
        offsetNormed = offsetRelative / segmentLength;

        TRACE( AVS_INFO, "offsetRelative  = %.f\n", offset);
        TRACE( AVS_INFO, "segmentLength   = %.f\n", segmentLength);
        TRACE( AVS_INFO, "valueDiff       = %.f\n", valueDiff);
        TRACE( AVS_INFO, "offsetNormed    = %.f\n", offsetNormed);

        retVal = valueLower + offsetNormed*valueDiff;
    }

    TRACE( AVS_INFO, "return value %.f\n", retVal);

    TRACE( AVS_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( retVal );
}



/*****************************************************************************/
/**
 *          AvsCalcNewOffset()
 *
 * @brief   Calculate the new offset for one direction.
 *
 * @param   pAvsCtx       Pointer to Avs context
 * @param   lutSize       Number of elements in the damping function Lut
 * @param   pDampLut      Pointer to damping function Lut
 * @param   pCurrOffset   Pointer to current offset value
 * @param   displMeasured Displacement measured by VSM
 * @param   maxAbsOffset  Maximum allowed absolute value of new offset
 *
 *****************************************************************************/
static void AvsCalcNewOffset
(
    AvsContext_t      *pAvsCtx,
    int8_t             lutSize,
    AvsDampingPoint_t *pDampLut,
    int16_t           *pCurrOffset,
    int16_t            displMeasured,
    uint16_t           maxAbsOffset
)
{
    float dampFactor;
    uint16_t absOffset;
    int16_t  offset = *pCurrOffset;

    TRACE( AVS_INFO, "%s: (enter)\n", __FUNCTION__);

    TRACE( AVS_DEBUG, "%+d;", offset);
    TRACE( AVS_DEBUG, "%+d;", displMeasured);

    /* get damping factor */
    dampFactor = AvsDampingFunction( lutSize, pDampLut, offset, maxAbsOffset );

    TRACE( AVS_DEBUG, "%.4f;", dampFactor);

    /* calculate new offset */
    offset = (int16_t) ( offset*dampFactor - displMeasured );

    TRACE( AVS_DEBUG, "%+d;", offset);

    /* clip offset if needed */
    absOffset = abs( offset );
    if ( absOffset > maxAbsOffset )
    {
        if (offset >= 0)
        {
            *pCurrOffset = +maxAbsOffset;
        }
        else
        {
            *pCurrOffset = -maxAbsOffset;
        }
    }
    else {
        *pCurrOffset = offset;
    }

    TRACE( AVS_DEBUG, "%+d;", *pCurrOffset);

    TRACE( AVS_INFO, "%s: (exit)\n", __FUNCTION__);
}



/*****************************************************************************/
/**
 *          AvsGetFreeOffsetDataElement()
 *
 * @brief   Get an unused element of the offset data array.
 *
 * @param   pAvsCtx       Pointer to Avs context
 *
 * @return  The desired element, or NULL if none was available.
 *
 *****************************************************************************/
static AvsOffsetData_t* AvsGetFreeOffsetDataElement
(
    AvsContext_t  *pAvsCtx,
    ulong_t       frameId
)
{
    uint16_t i;
    AvsOffsetData_t *pOffsetData;

    TRACE( AVS_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pAvsCtx->pOffsetData == NULL )
    {
        return NULL;
    }

    for ( i = 0; i < pAvsCtx->config.offsetDataArraySize; i++ )
    {
        pOffsetData = &pAvsCtx->pOffsetData[i];
        if ( (pOffsetData->frameId == frameId) && pOffsetData->used )
        {
            break;
        }
        pOffsetData = NULL;
    }

    if ( NULL == pOffsetData )
    {
        for ( i = 0; i < pAvsCtx->config.offsetDataArraySize; i++ )
        {
            pOffsetData = &pAvsCtx->pOffsetData[i];
            if ( !pOffsetData->used )
            {
                break;
            }
            pOffsetData = NULL;
        }
    }

    TRACE( AVS_INFO, "%s: (exit)\n", __FUNCTION__);
    
    return pOffsetData;
}



/*****************************************************************************/
/**
 *          AvsGetOffsetDataByFrameId()
 *
 * @brief   Search for an offset data structure containing the given frameId.
 *
 * @param   pAvsCtx   Pointer to Avs context
 * @param   frameId   Value of frameId to look for in offset data elements
 *
 * @return  The desired element, or NULL if it could not be found.
 *
 *****************************************************************************/
static AvsOffsetData_t* AvsGetOffsetDataByFrameId
(
    AvsContext_t  *pAvsCtx,
    ulong_t        frameId
)
{
    uint16_t i;
    AvsOffsetData_t *pOffsetData;

    TRACE( AVS_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pAvsCtx->pOffsetData == NULL )
    {
        return NULL;
    }

    for ( i = 0; i < pAvsCtx->config.offsetDataArraySize; i++ )
    {
        pOffsetData = &pAvsCtx->pOffsetData[i];
        if ( pOffsetData->used && ( pOffsetData->frameId == frameId ) )
        {
            break;
        }
        pOffsetData = NULL;
    }
    
    TRACE( AVS_INFO, "%s: (exit)\n", __FUNCTION__);

    return pOffsetData;
}



/******************************************************************************
 * Implementation of AVS API Functions
 *****************************************************************************/


/******************************************************************************
 * AvsInit()
 *****************************************************************************/
RESULT AvsInit
(
    AvsInstanceConfig_t *pInstConfig
)
{
    RESULT result = RET_SUCCESS;

    AvsContext_t *pAvsCtx;

    TRACE( AVS_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pInstConfig == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    /* allocate auto video stabilization control context */
    pAvsCtx = ( AvsContext_t *)malloc( sizeof(AvsContext_t) );
    if ( pAvsCtx == NULL )
    {
        TRACE( AVS_ERROR,  "%s: Can't allocate AVS context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }

    /* pre-initialize context */
    MEMSET( pAvsCtx, 0, sizeof(*pAvsCtx) );
    pAvsCtx->state = AVS_STATE_INITIALIZED;

    /* return handle */
    pInstConfig->hAvs = (AvsHandle_t)pAvsCtx;

    TRACE( AVS_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AvsRelease()
 *****************************************************************************/
RESULT AvsRelease
(
    AvsHandle_t handle
)
{
    AvsContext_t *pAvsCtx = (AvsContext_t *)handle;

    TRACE( AVS_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAvsCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check state */
    if ( AVS_STATE_RUNNING == pAvsCtx->state )
    {
        return ( RET_BUSY );
    }

    if ( pAvsCtx->pOffsetData != NULL )
    {
        free ( pAvsCtx->pOffsetData );
    }

    if ( pAvsCtx->pDampLutHor != NULL )
    {
        free ( pAvsCtx->pDampLutHor );
    }

    if ( pAvsCtx->pDampLutVer != NULL )
    {
        free ( pAvsCtx->pDampLutVer );
    }

    MEMSET( pAvsCtx, 0, sizeof(AvsContext_t) );
    free ( pAvsCtx );

    TRACE( AVS_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AvsConfigure()
 *****************************************************************************/
RESULT AvsConfigure
(
    AvsHandle_t handle,
    AvsConfig_t *pConfig
)
{
    RESULT result;

    TRACE( AVS_INFO, "%s: (enter)\n", __FUNCTION__);

    result = AvsApplyConfiguration(handle, pConfig, BOOL_FALSE);

    TRACE( AVS_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AvsReConfigure()
 *****************************************************************************/
RESULT AvsReConfigure
(
    AvsHandle_t handle,
    AvsConfig_t *pConfig
)
{
    RESULT result;

    TRACE( AVS_INFO, "%s: (enter)\n", __FUNCTION__);

    result = AvsApplyConfiguration(handle, pConfig, BOOL_TRUE);

    TRACE( AVS_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AvsStart()
 *****************************************************************************/
RESULT AvsStart
(
    AvsHandle_t  handle
)
{
    AvsContext_t *pAvsCtx = (AvsContext_t *)handle;
    RESULT result = RET_SUCCESS;

    TRACE( AVS_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAvsCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (AVS_STATE_STOPPED != pAvsCtx->state) &&
         (AVS_STATE_RUNNING != pAvsCtx->state) )
    {
        return ( RET_WRONG_STATE );
    }

    pAvsCtx->state = AVS_STATE_RUNNING;

    TRACE( AVS_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AvsStop()
 *****************************************************************************/
RESULT AvsStop
(
    AvsHandle_t handle
)
{
    AvsContext_t *pAvsCtx = (AvsContext_t *)handle;
    uint16_t i;

    TRACE( AVS_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAvsCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (AVS_STATE_INITIALIZED != pAvsCtx->state) &&
         (AVS_STATE_RUNNING != pAvsCtx->state) &&
         (AVS_STATE_STOPPED != pAvsCtx->state) )
    {
        return ( RET_WRONG_STATE );
    }

    /* discard any stored offset data */
    for ( i = 0; i < pAvsCtx->config.offsetDataArraySize; i++ )
    {
        pAvsCtx->pOffsetData[i].used = BOOL_FALSE;
    }

    /* reset offset vector to 0,0 and set cropping window accordingly */
    pAvsCtx->offsVec.x = pAvsCtx->offsVec.y = 0;
    AvsSetCroppingWindowInHw( pAvsCtx, &pAvsCtx->offsVec );

    pAvsCtx->state = AVS_STATE_STOPPED;

    TRACE( AVS_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AvsGetConfig()
 *****************************************************************************/
RESULT AvsGetConfig
(
    AvsHandle_t   handle,
    AvsConfig_t  *pConfig
)
{
    AvsContext_t *pAvsCtx = (AvsContext_t *)handle;

    TRACE( AVS_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAvsCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pConfig == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    *pConfig = pAvsCtx->config;

    TRACE( AVS_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AvsGetStatus()
 *****************************************************************************/
RESULT AvsGetStatus
(
    AvsHandle_t         handle,
    bool_t             *pRunning,
    CamEngineVector_t  *pCurrDisplVec,
    CamEngineVector_t  *pCurrOffsetVec
)
{
    AvsContext_t *pAvsCtx = (AvsContext_t *)handle;

    TRACE( AVS_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAvsCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pRunning )
    {
        *pRunning       = ( pAvsCtx->state == AVS_STATE_RUNNING );
    }

    if ( pCurrDisplVec )
    {
        *pCurrDisplVec  = pAvsCtx->displVec;
    }

    if ( pCurrOffsetVec )
    {
        *pCurrOffsetVec = pAvsCtx->offsVec;
    }

    TRACE( AVS_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AvsProcessFrame()
 *****************************************************************************/
RESULT AvsProcessFrame
(
    AvsHandle_t        handle,
    ulong_t            frameId,
    CamEngineVector_t *pDisplVec
)
{
    AvsContext_t *pAvsCtx = (AvsContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( AVS_INFO, "%s: (enter)\n", __FUNCTION__);

    TRACE( AVS_DEBUG, "\nproc;%d;", frameId);

    /* initial checks */
    if ( pAvsCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pDisplVec == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    if ( pAvsCtx->state == AVS_STATE_RUNNING )
    {
        AvsOffsetData_t* pOffsetData;

        /* get an offset data array element to store the new offset vector
         * into */
        pOffsetData = AvsGetFreeOffsetDataElement( pAvsCtx, frameId );
        if ( pOffsetData == NULL )
        {
            result = RET_FAILURE;
        }
        else  {
            /* store measured displacement vector */
            pAvsCtx->displVec = *pDisplVec;

            /* calculate new horizontal offset */
            TRACE( AVS_INFO, "Calc new hor offset\n");
            AvsCalcNewOffset( pAvsCtx, pAvsCtx->dampLutHorSize,
                pAvsCtx->pDampLutHor, &pAvsCtx->offsVec.x, pDisplVec->x,
                pAvsCtx->maxAbsOffsetHor );

            /* calculate new vertical offset */
            TRACE( AVS_INFO, "Calc new ver offset\n");
            AvsCalcNewOffset( pAvsCtx, pAvsCtx->dampLutVerSize,
                pAvsCtx->pDampLutVer, &pAvsCtx->offsVec.y, pDisplVec->y,
                pAvsCtx->maxAbsOffsetVer );

            /* store the resulting calculation in our array */
            pOffsetData->frameId = frameId;
            pOffsetData->offsVec = pAvsCtx->offsVec;
            pOffsetData->used    = BOOL_TRUE;
        }
    }
    else
    {
        result = RET_CANCELED;
    }

    TRACE( AVS_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AvsSetCroppingWindow()
 *****************************************************************************/
RESULT AvsSetCroppingWindow
(
    AvsHandle_t        handle,
    ulong_t            frameId
)
{
    AvsContext_t *pAvsCtx = (AvsContext_t *)handle;
    AvsOffsetData_t* pOffsetData;

    TRACE( AVS_INFO, "%s: (enter)\n", __FUNCTION__);

    TRACE( AVS_DEBUG, "\nset;%d;", frameId);

    /* initial checks */
    if ( pAvsCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* look for offset data array element containing frame id */
    pOffsetData = AvsGetOffsetDataByFrameId( pAvsCtx, frameId );
    if ( pOffsetData == NULL )
    {
        return ( RET_FAILURE );
    }

    /* set new cropping window */
    AvsSetCroppingWindowInHw( pAvsCtx, &pOffsetData->offsVec );

    /* mark offset data element as unused */
    pOffsetData->used = BOOL_FALSE;

    TRACE( AVS_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


