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
 * @file wprevert.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>
#include <common/misc.h>

#include "af.h"
#include "af_ctrl.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( AF_INFO  , "AF: ", INFO     , 1 );
CREATE_TRACER( AF_WARN  , "AF: ", WARNING  , 1 );
CREATE_TRACER( AF_ERROR , "AF: ", ERROR    , 1 );
CREATE_TRACER( AF_NOTICE1 , "AF: ", TRACE_NOTICE1    , 1 );
CREATE_TRACER( AF_DEBUG , "AF: ", TRACE_DEBUG, 1 );

#define container_of(ptr, type, member) \
    ((type *)(((ulong_t)(uint8_t *)(ptr)) - (ulong_t)(&((type *)0)->member)))

/*****************************************************************************/
/*!
 * \name parameters of out-of-focus detection
 */
/*****************************************************************************/
//@{
/*! percentage the sharpness value is allowed to deviate without
    triggering new AF search */
#define SHARPNESS_DEVIATION_PERCENT 15
//@}


/*****************************************************************************/
/*!
 * \name parameters of adaptive range search
 */
/*****************************************************************************/
//@{
/*! approx. number of steps to use in one adaptive scan range. */
#define ADAPTIVE_STEPS_PER_RANGE 8
#define ONSHOT_WAIT_CNT          1
#define TRACKING_STABLE_TIMES    200   //ms   
#define SHARPNESS_MIN            500
//@}




/******************************************************************************
 * local type definitions
 *****************************************************************************/


/******************************************************************************
 * local variable declarations
 *****************************************************************************/
static int64_t t0,t1;

/******************************************************************************
 * local function prototypes
 *****************************************************************************/
static RESULT AfSearchInit
(
    AfHandle_t      handle,
    const uint32_t  LensePosMin,
    const uint32_t  LensePosMax
)
{
    RESULT result = RET_SUCCESS;

    AfContext_t *pAfCtx = (AfContext_t *)handle;

    TRACE( AF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAfCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }
    
	if (pAfCtx->isSOCAf) {
		TRACE( AF_INFO, "%s: this is soc af\n", __FUNCTION__);
		return result;
	}

    pAfCtx->AfSearchCtx.State       = AFM_FSSTATE_INIT;
    pAfCtx->AfSearchCtx.LensePosMin = LensePosMin;
    pAfCtx->AfSearchCtx.LensePosMax = LensePosMax;

    pAfCtx->AfSearchCtx.Step = (pAfCtx->AfSearchCtx.LensePosMin - pAfCtx->AfSearchCtx.LensePosMax);
    pAfCtx->AfSearchCtx.Step /= ADAPTIVE_STEPS_PER_RANGE;


    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

static RESULT AfSearchFine
(
    AfHandle_t      handle,
    int32_t        *pLensPos
)
{
    RESULT result = RET_SUCCESS;
    bool_t setMdiFocus;
    AfSeachPos_t *nPos,*pPos,*cPos,*mem,*curPos=NULL,*nxtPos=NULL,*prePos=NULL;
    int32_t fineSeachStart, fineSeachMid,fineSeachEnd, fineSeachNum,i;
    AfContext_t *pAfCtx = (AfContext_t *)handle;        

    if (pAfCtx->AfSearchCtx.Path.index < 1) {
        
        curPos = container_of(pAfCtx->AfSearchCtx.Path.maxSharpnessPos, AfSeachPos_t, nlist);
        if ((curPos->plist.p_next != NULL) && (curPos->nlist.p_next!=NULL)) {
            nxtPos = container_of(curPos->nlist.p_next, AfSeachPos_t, nlist);
            prePos = container_of(curPos->plist.p_next, AfSeachPos_t, plist);
            fineSeachStart = prePos->pos;
            fineSeachMid = curPos->pos;
            fineSeachEnd = nxtPos->pos;
            fineSeachNum = 5;
        } else if (curPos->plist.p_next != NULL) {
            prePos = container_of(curPos->plist.p_next, AfSeachPos_t, plist);
            fineSeachStart = prePos->pos;
            fineSeachEnd = curPos->pos;
            fineSeachMid = (fineSeachStart + fineSeachEnd)/2;
            fineSeachNum = 3;
        } else if (curPos->nlist.p_next!=NULL) {
            nxtPos = container_of(curPos->nlist.p_next, AfSeachPos_t, nlist);
            fineSeachStart = curPos->pos;
            fineSeachEnd = nxtPos->pos;
            fineSeachMid = (fineSeachStart + fineSeachEnd)/2;
            fineSeachNum = 3;
        }

        if (abs(fineSeachEnd - fineSeachStart) < 4) {    /* ddl@rock-chips.com: v0.0x29.0 */
            TRACE(AF_DEBUG, "AF Seachth-%d(%d->%d) is not necessary, directly to (%d,%f)",
                pAfCtx->AfSearchCtx.Path.index,
                fineSeachStart, fineSeachEnd, 
                pAfCtx->AfSearchCtx.MaxSharpnessPos,
                pAfCtx->AfSearchCtx.MaxSharpness
                );
            return RET_SUCCESS;
        }
        
        mem = container_of(ListGetHead(&pAfCtx->AfSearchCtx.Path.nlist), AfSeachPos_t, nlist);                            
        ListInit(&pAfCtx->AfSearchCtx.Path.nlist);
        ListInit(&pAfCtx->AfSearchCtx.Path.plist);
        
        nPos = (AfSeachPos_t*)osMalloc(sizeof(AfSeachPos_t)*fineSeachNum);    
        if (nPos == NULL) {
            TRACE(AF_WARN,"%s: nPos malloc failed, AF must be end advance!",__FUNCTION__);
            result = RET_SUCCESS;
            goto end;
        }

        setMdiFocus = BOOL_FALSE;
        pPos = nPos + fineSeachNum-1;
        TRACE(AF_DEBUG, "AF SeachPath-%d(%d->%d):\n",pAfCtx->AfSearchCtx.Path.index+1,fineSeachStart,fineSeachEnd);
            
        for (i=0; i<fineSeachNum; i++) {
            nPos->index = i;
            ListAddTail(&pAfCtx->AfSearchCtx.Path.nlist, &nPos->nlist);
            ListAddTail(&pAfCtx->AfSearchCtx.Path.plist, &pPos->plist);
            nPos->dSharpness = 0;
            nPos->sharpness = 0;
            if (i==0) {
                nPos->pos = fineSeachStart;
            } else if (i==(fineSeachNum-1)) {
                nPos->pos = fineSeachEnd;
            } else if (i==(fineSeachNum/2)) {
                nPos->pos = fineSeachMid;
            } else {
                nPos->pos = -1;
            }
            nPos++;
            pPos--;
        }

        {
            List *l;

            l = ListHead( &pAfCtx->AfSearchCtx.Path.nlist );
            while ( l )
            {
                cPos = container_of(l, AfSeachPos_t, nlist);                
                if (cPos->pos == -1) {
                    nPos = container_of(cPos->nlist.p_next, AfSeachPos_t, nlist); 
                    pPos = container_of(cPos->plist.p_next, AfSeachPos_t, plist);
                    cPos->pos = (nPos->pos + pPos->pos)/2;
                } 

                if (cPos->pos == curPos->pos) {
                    cPos->sharpness = curPos->sharpness;
                } else if (prePos && (cPos->pos == prePos->pos)) {
                    cPos->sharpness = prePos->sharpness;
                } else if (nxtPos && (cPos->pos == nxtPos->pos)) {
                    cPos->sharpness = nxtPos->sharpness;                
                }

                if (setMdiFocus == BOOL_FALSE) {
                    if (cPos->sharpness == 0) {
                        setMdiFocus = BOOL_TRUE;
                        *pLensPos = cPos->pos;
                        pAfCtx->AfSearchCtx.Path.curPos = &cPos->nlist;
                    }
                }
                TRACE(AF_DEBUG, "    nPos->index: %d  pos: %d  sharpness: %f\n",cPos->index,cPos->pos,cPos->sharpness);
                l = l->p_next;
            }            
        }

        pAfCtx->AfSearchCtx.Path.index++;
        osFree(mem);
        result = RET_PENDING;
        
    }
end:
    return result;
}

/*****************************************************************************/
/*!
 *  \FUNCTION    MrvSls_AfGetSingleVal \n
 *  \RETURNVALUE biggest measurement value of the three individuals \n
 *  \PARAMETERS  ptMrvAfMeas .. measurement tripel \n
 */
/*****************************************************************************/
float AfGetSingleSharpness
(
    AfHandle_t                          handle,
    const CamerIcAfmMeasuringResult_t   *pMeasResults
)
{
    AfContext_t *pAfCtx = (AfContext_t *)handle;

    float value = 0U;
    float l = 0.0f;

    TRACE( AF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAfCtx == NULL )
    {
        TRACE( AF_ERROR, "%d\n", __LINE__);
        return ( RET_WRONG_HANDLE );
    }

    if ( (CamerIcIspAfmMeasuringWindowIsEnabled( pAfCtx->hCamerIc, CAMERIC_ISP_AFM_WINDOW_A ) == BOOL_TRUE) && (pMeasResults->LuminanceA != 0) )
    {        
        l = (float)pMeasResults->LuminanceA / (float)pMeasResults->PixelCntA;
        value = (float)pMeasResults->SharpnessA;
    }
    else if ( (CamerIcIspAfmMeasuringWindowIsEnabled( pAfCtx->hCamerIc, CAMERIC_ISP_AFM_WINDOW_B ) == BOOL_TRUE) && (pMeasResults->LuminanceB != 0) )
    {
        TRACE( AF_ERROR, "%d\n", __LINE__);
        l = (float)pMeasResults->LuminanceB / (float)pMeasResults->PixelCntB;
        value = (float)pMeasResults->SharpnessB;
    }
    else if ( (CamerIcIspAfmMeasuringWindowIsEnabled( pAfCtx->hCamerIc, CAMERIC_ISP_AFM_WINDOW_C ) == BOOL_TRUE) && (pMeasResults->LuminanceC != 0) )
    {
        TRACE( AF_ERROR, "%d\n", __LINE__);
        l = (float)pMeasResults->LuminanceC / (float)pMeasResults->PixelCntC;
        value = (float)pMeasResults->SharpnessC;
    }

    value = ( l > FLT_EPSILON ) ? ((float)value / (l * l)) : (uint32_t)(FLT_MAX);
    
    TRACE( AF_DEBUG, "%s: %ld %f\n", __FUNCTION__, pMeasResults->LuminanceA, value );

    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( value );
}
/* ddl@rock-chips.com: v1.6.0 */
/******************************************************************************
 * AfGetMeasureWindow()
 *****************************************************************************/
int AfGetMeasureWindow
(
    AfHandle_t                          handle,
    const CamerIcAfmMeasuringResult_t   *pMeasResults,
    CamerIcWindow_t                     *pWdw
)
{
    AfContext_t *pAfCtx = (AfContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( AF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAfCtx == NULL )
    {
        TRACE( AF_ERROR, "%d\n", __LINE__);
        return ( RET_WRONG_HANDLE );
    }
    
    if ( pWdw == NULL )
    {
        TRACE( AF_ERROR, "%d\n", __LINE__);
        return ( RET_WRONG_HANDLE );
    }
    
    if ( (CamerIcIspAfmMeasuringWindowIsEnabled( pAfCtx->hCamerIc, CAMERIC_ISP_AFM_WINDOW_A ) == BOOL_TRUE))
    {        
        *pWdw = pMeasResults->WindowA;
    }
    else if ( (CamerIcIspAfmMeasuringWindowIsEnabled( pAfCtx->hCamerIc, CAMERIC_ISP_AFM_WINDOW_B ) == BOOL_TRUE) )
    {
        *pWdw = pMeasResults->WindowB;
    }
    else if ( (CamerIcIspAfmMeasuringWindowIsEnabled( pAfCtx->hCamerIc, CAMERIC_ISP_AFM_WINDOW_C ) == BOOL_TRUE)  )
    {
        *pWdw = pMeasResults->WindowC;
    }
    else
    {
        result = RET_FAILURE;
    }

    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__ );

    return result;
}

/******************************************************************************
 * AfSearchFullRange()
 *****************************************************************************/
static RESULT AfSearchFullRange
(
    AfHandle_t                          handle,
    const CamerIcAfmMeasuringResult_t   *pMeasResults,
    int32_t                             *pLensPos
)
{
    (void) pMeasResults;
    (void) pLensPos;

    RESULT result = RET_SUCCESS;

    AfContext_t *pAfCtx = (AfContext_t *)handle;

    TRACE( AF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAfCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }
    if ( (pMeasResults == NULL) || (pLensPos==NULL) )
    {
        return ( RET_INVALID_PARM );
    }

    switch ( pAfCtx->AfSearchCtx.State )
    {
        case AFM_FSSTATE_INIT:
            {
                TRACE( AF_DEBUG, "%s: (enter AFM_FSSTATE_INIT)\n", __FUNCTION__);
                
                if ( (*pLensPos == pAfCtx->AfSearchCtx.LensePosMin)
                        || (*pLensPos == pAfCtx->AfSearchCtx.LensePosMax) )
                {
                    float Sharpness = AfGetSingleSharpness( handle, pMeasResults );
                    pAfCtx->AfSearchCtx.MaxSharpness    = Sharpness;
                    pAfCtx->AfSearchCtx.MaxSharpnessPos = *pLensPos;

                    pAfCtx->AfSearchCtx.Step = ( *pLensPos == pAfCtx->AfSearchCtx.LensePosMax ) ? -1 : 1;

                    TRACE( AF_DEBUG, "%s: (AFM_FSSTATE_INIT: %d, %f)\n", __FUNCTION__, *pLensPos, Sharpness, pAfCtx->AfSearchCtx.MaxSharpness );
                    
                    *pLensPos += pAfCtx->AfSearchCtx.Step;
                    pAfCtx->AfSearchCtx.State = AFM_FSSTATE_SEARCHFOCUS;
                }
                else
                {
                    pAfCtx->AfSearchCtx.MaxSharpness = 0;
                }

                result = RET_PENDING;
            
                TRACE( AF_DEBUG, "%s: (exit AFM_FSSTATE_INIT)\n", __FUNCTION__);
                break;
            }

        case AFM_FSSTATE_SEARCHFOCUS:
            {
                TRACE( AF_DEBUG, "%s: (enter AFM_FSSTATE_SEARCHFOCUS)\n", __FUNCTION__ );

                float Sharpness = AfGetSingleSharpness( handle, pMeasResults );
                TRACE( AF_DEBUG, "%s: (AFM_FSSTATE_SEARCHFOCUS: %d, %f)\n", __FUNCTION__, *pLensPos, Sharpness, pAfCtx->AfSearchCtx.MaxSharpness );
                if ( Sharpness > pAfCtx->AfSearchCtx.MaxSharpness )
                {
                    pAfCtx->AfSearchCtx.MaxSharpness    = Sharpness;
                    pAfCtx->AfSearchCtx.MaxSharpnessPos = *pLensPos;
                }

                if ( (*pLensPos == pAfCtx->AfSearchCtx.LensePosMin)
                        || (*pLensPos == pAfCtx->AfSearchCtx.LensePosMax) )
                {
                     // finish 
                     pAfCtx->AfSearchCtx.State = AFM_FSSTATE_FOCUSFOUND;
                     *pLensPos = pAfCtx->AfSearchCtx.MaxSharpnessPos;
                     result = RET_SUCCESS;
                     TRACE( AF_DEBUG, "%s: (found: %d)\n", __FUNCTION__, *pLensPos );
                }
                else
                {
                    *pLensPos += pAfCtx->AfSearchCtx.Step;
                    result = RET_PENDING;
                }
                 
                TRACE( AF_DEBUG, "%s: (exit AFM_FSSTATE_SEARCHFOCUS)\n", __FUNCTION__);
                
                break;
            }

        default:
            {
                break;
            }
    }

    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AfSearchHillClimb()
 *****************************************************************************/
static RESULT AfSearchHillClimb
(
    AfHandle_t                          handle,
    const CamerIcAfmMeasuringResult_t   *pMeasResults,
    int32_t                             *pLensPos
)
{
    (void) pMeasResults;
    (void) pLensPos;

    RESULT result = RET_SUCCESS;

    AfContext_t *pAfCtx = (AfContext_t *)handle;

    TRACE( AF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAfCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (pMeasResults == NULL) || (pLensPos==NULL) )
    {
        return ( RET_INVALID_PARM );
    }

    switch ( pAfCtx->AfSearchCtx.State )
    {
        case AFM_FSSTATE_INIT:
            {
                TRACE( AF_DEBUG, "%s: (enter AFM_FSSTATE_INIT)\n", __FUNCTION__);
                
                if ( (*pLensPos == pAfCtx->AfSearchCtx.LensePosMin)
                        || (*pLensPos == pAfCtx->AfSearchCtx.LensePosMax) )
                {
                    float Sharpness = AfGetSingleSharpness( handle, pMeasResults );
                    pAfCtx->AfSearchCtx.MaxSharpness    = Sharpness;
                    pAfCtx->AfSearchCtx.MaxSharpnessPos = *pLensPos;

                    TRACE( AF_DEBUG, "%s: (AFM_FSSTATE_INIT: %d, %f)\n", __FUNCTION__, *pLensPos, Sharpness, pAfCtx->AfSearchCtx.MaxSharpness );
                    
                    *pLensPos += pAfCtx->AfSearchCtx.Step;
                    pAfCtx->AfSearchCtx.State = AFM_FSSTATE_SEARCHFOCUS;
                }
                else
                {
                    pAfCtx->AfSearchCtx.MaxSharpness = 0;
                }

                result = RET_PENDING;
            
                TRACE( AF_DEBUG, "%s: (exit AFM_FSSTATE_INIT)\n", __FUNCTION__);
                break;
            }

        case AFM_FSSTATE_SEARCHFOCUS:
            {
                TRACE( AF_DEBUG, "%s: (enter AFM_FSSTATE_SEARCHFOCUS)\n", __FUNCTION__ );

                float Sharpness = AfGetSingleSharpness( handle, pMeasResults );
                TRACE( AF_DEBUG, "%s: (AFM_FSSTATE_SEARCHFOCUS: %d, %f)\n", __FUNCTION__, *pLensPos, Sharpness, pAfCtx->AfSearchCtx.MaxSharpness );
                if ( Sharpness > pAfCtx->AfSearchCtx.MaxSharpness )
                {
                    pAfCtx->AfSearchCtx.MaxSharpness    = Sharpness;
                    pAfCtx->AfSearchCtx.MaxSharpnessPos = *pLensPos;
                }

                if ( (*pLensPos == pAfCtx->AfSearchCtx.LensePosMin)
                        || (*pLensPos == pAfCtx->AfSearchCtx.LensePosMax) )
                {
                    if ( ( pAfCtx->AfSearchCtx.MaxSharpnessPos > pAfCtx->AfSearchCtx.LensePosMin)
                            && ( pAfCtx->AfSearchCtx.MaxSharpnessPos < pAfCtx->AfSearchCtx.LensePosMax) )
                    {
                        if ( pAfCtx->AfSearchCtx.Step < -1 )
                        {
                            // neg direction => searching fwd
                            pAfCtx->AfSearchCtx.LensePosMin = pAfCtx->AfSearchCtx.MaxSharpnessPos +  pAfCtx->AfSearchCtx.Step;
                            pAfCtx->AfSearchCtx.LensePosMax = pAfCtx->AfSearchCtx.MaxSharpnessPos -  pAfCtx->AfSearchCtx.Step;
                            pAfCtx->AfSearchCtx.Step /= -2;
                            *pLensPos = pAfCtx->AfSearchCtx.LensePosMin + pAfCtx->AfSearchCtx.Step;
                            result = RET_PENDING;
                            TRACE( AF_DEBUG, "%s: (AFM_FSSTATE_SEARCHFOCUS: %d, min:%d, max:%d, s:%d)\n", 
                                    __FUNCTION__, *pLensPos, pAfCtx->AfSearchCtx.LensePosMin, pAfCtx->AfSearchCtx.LensePosMax, pAfCtx->AfSearchCtx.Step );
                        }
                        else if ( pAfCtx->AfSearchCtx.Step > 1 )
                        {
                            // pos direction => searching bwd
                            pAfCtx->AfSearchCtx.LensePosMin = pAfCtx->AfSearchCtx.MaxSharpnessPos -  pAfCtx->AfSearchCtx.Step;
                            pAfCtx->AfSearchCtx.LensePosMax = pAfCtx->AfSearchCtx.MaxSharpnessPos +  pAfCtx->AfSearchCtx.Step;
                            pAfCtx->AfSearchCtx.Step /= -2;
                            *pLensPos = pAfCtx->AfSearchCtx.LensePosMax + pAfCtx->AfSearchCtx.Step;
                            result = RET_PENDING;
                            TRACE( AF_DEBUG, "%s: (AFM_FSSTATE_SEARCHFOCUS: %d, min:%d, max:%d, s:%d)\n", 
                                    __FUNCTION__, *pLensPos, pAfCtx->AfSearchCtx.LensePosMin, pAfCtx->AfSearchCtx.LensePosMax, pAfCtx->AfSearchCtx.Step );
                        }
                        else
                        {
                            // finish 
                            pAfCtx->AfSearchCtx.State = AFM_FSSTATE_FOCUSFOUND;
                            *pLensPos = pAfCtx->AfSearchCtx.MaxSharpnessPos;
                            result = RET_SUCCESS;
                            TRACE( AF_DEBUG, "%s: (found: %d)\n", __FUNCTION__, *pLensPos );
                        }
                    }
                    else if ( pAfCtx->AfSearchCtx.MaxSharpnessPos ==  pAfCtx->AfSearchCtx.LensePosMin )
                    {
                        if ( pAfCtx->AfSearchCtx.Step < -1 )
                        {
                            // neg direction => searching fwd
                            pAfCtx->AfSearchCtx.LensePosMax = pAfCtx->AfSearchCtx.MaxSharpnessPos -  pAfCtx->AfSearchCtx.Step;
                            pAfCtx->AfSearchCtx.Step /= -2;
                            *pLensPos = pAfCtx->AfSearchCtx.LensePosMin + pAfCtx->AfSearchCtx.Step;
                            result = RET_PENDING;
                            TRACE( AF_DEBUG, "%s: (AFM_FSSTATE_SEARCHFOCUS: %d, min:%d, max:%d, s:%d)\n", 
                                    __FUNCTION__, *pLensPos, pAfCtx->AfSearchCtx.LensePosMin, pAfCtx->AfSearchCtx.LensePosMax, pAfCtx->AfSearchCtx.Step );
                        }
                        else if ( pAfCtx->AfSearchCtx.Step > 1 )
                        {
                            // pos direction => searching bwd
                            pAfCtx->AfSearchCtx.LensePosMax = pAfCtx->AfSearchCtx.MaxSharpnessPos + pAfCtx->AfSearchCtx.Step;
                            pAfCtx->AfSearchCtx.Step /= -2;
                            *pLensPos = pAfCtx->AfSearchCtx.LensePosMax + pAfCtx->AfSearchCtx.Step;
                            result = RET_PENDING;
                            TRACE( AF_DEBUG, "%s: (AFM_FSSTATE_SEARCHFOCUS: %d, min:%d, max:%d, s:%d)\n", 
                                    __FUNCTION__, *pLensPos, pAfCtx->AfSearchCtx.LensePosMin, pAfCtx->AfSearchCtx.LensePosMax, pAfCtx->AfSearchCtx.Step );
                        }
                        else
                        {
                            // finish 
                            pAfCtx->AfSearchCtx.State = AFM_FSSTATE_FOCUSFOUND;
                            *pLensPos = pAfCtx->AfSearchCtx.MaxSharpnessPos;
                            result = RET_SUCCESS;
                            TRACE( AF_DEBUG, "%s: (found: %d)\n", __FUNCTION__, *pLensPos );
                        }
                    }
                    else if ( pAfCtx->AfSearchCtx.MaxSharpnessPos ==  pAfCtx->AfSearchCtx.LensePosMax )
                    {
                        if ( pAfCtx->AfSearchCtx.Step < -1 )
                        {
                            // neg direction => searching fwd
                            pAfCtx->AfSearchCtx.LensePosMin = pAfCtx->AfSearchCtx.MaxSharpnessPos +  pAfCtx->AfSearchCtx.Step;
                            pAfCtx->AfSearchCtx.Step /= -2;
                            *pLensPos = pAfCtx->AfSearchCtx.LensePosMin + pAfCtx->AfSearchCtx.Step;
                            result = RET_PENDING;
                            TRACE( AF_DEBUG, "%s: (AFM_FSSTATE_SEARCHFOCUS: %d, min:%d, max:%d, s:%d)\n", 
                                    __FUNCTION__, *pLensPos, pAfCtx->AfSearchCtx.LensePosMin, pAfCtx->AfSearchCtx.LensePosMax, pAfCtx->AfSearchCtx.Step );
                        }
                        else if ( pAfCtx->AfSearchCtx.Step > 1 )
                        {
                            // pos direction => searching bwd
                            pAfCtx->AfSearchCtx.LensePosMin = pAfCtx->AfSearchCtx.MaxSharpnessPos -  pAfCtx->AfSearchCtx.Step;
                            pAfCtx->AfSearchCtx.Step /= -2;
                            *pLensPos = pAfCtx->AfSearchCtx.LensePosMax + pAfCtx->AfSearchCtx.Step;
                            result = RET_PENDING;
                            TRACE( AF_DEBUG, "%s: (AFM_FSSTATE_SEARCHFOCUS: %d, min:%d, max:%d, s:%d)\n", 
                                    __FUNCTION__, *pLensPos, pAfCtx->AfSearchCtx.LensePosMin, pAfCtx->AfSearchCtx.LensePosMax, pAfCtx->AfSearchCtx.Step );
                        }
                        else
                        {
                            // finish 
                            pAfCtx->AfSearchCtx.State = AFM_FSSTATE_FOCUSFOUND;
                            *pLensPos = pAfCtx->AfSearchCtx.MaxSharpnessPos;
                            result = RET_SUCCESS;
                            TRACE( AF_DEBUG, "%s: (found: %d)\n", __FUNCTION__, *pLensPos );
                        }
                    }
                }
                else
                {
                    *pLensPos += pAfCtx->AfSearchCtx.Step;
                    result = RET_PENDING;
                }

                TRACE( AF_DEBUG, "%s: (exit AFM_FSSTATE_SEARCHFOCUS)\n", __FUNCTION__);
                break;
            }

        default:
            {
                break;
            }
    }

    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__);


    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          AfSearchAdaptiveRange()
 *
 * @brief   This is similar to the full range search. Subsequent scans will
 *          be performed, each one with a narrowed range and a lowered step
 *          size to lower the search steps needed to find the point of best
 *          focus.
 *
 *          Additionally, this search method mow has support for continous
 *          AF searches (as needed for video applications)
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * @retval  RET_OUTOFMEM
 *
 *****************************************************************************/
static RESULT AfSearchAdaptiveRange
(
    AfHandle_t                          handle,
    const CamerIcAfmMeasuringResult_t   *pMeasResults,
    int32_t                             *pLensPos
)
{
    RESULT result = RET_SUCCESS;
    bool_t focused;
    AfContext_t *pAfCtx = (AfContext_t *)handle;

    TRACE( AF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAfCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (pMeasResults == NULL) || (pLensPos==NULL) )
    {
        return ( RET_INVALID_PARM );
    }

    switch ( pAfCtx->AfSearchCtx.State )
    {
        case AFM_FSSTATE_INIT:            
        case AFM_FSSTATE_SEARCHFOCUS:
            {               
                float dSharpness;
                AfSeachPos_t *curPos=NULL,*prePos=NULL,*nxtPos=NULL;
                float Sharpness = AfGetSingleSharpness( handle, pMeasResults );

new_afm_measure:
                if (pAfCtx->AfSearchCtx.Path.curPos == NULL) {
                    TRACE(AF_ERROR, "%s: pAfCtx->AfSearchCtx.Path.curPos is NULL\n",__FUNCTION__);
                    DCT_ASSERT(0);
                }  
                
                curPos = container_of(pAfCtx->AfSearchCtx.Path.curPos, AfSeachPos_t, nlist);
                if (curPos->sharpness == 0)
                    curPos->sharpness = Sharpness;
                    
                TRACE( AF_DEBUG, "%s: (AFM_FSSTATE_SEARCHFOCUS: %d, %f)\n", __FUNCTION__, curPos->pos, curPos->sharpness );
                
                if (curPos->plist.p_next != NULL) {
                    prePos = container_of(curPos->plist.p_next, AfSeachPos_t, plist);
                    curPos->dSharpness = (curPos->sharpness-prePos->sharpness)/(curPos->sharpness+prePos->sharpness);                    
                } else {
                    curPos->dSharpness = 0;
                }

                if ( curPos->sharpness > pAfCtx->AfSearchCtx.MaxSharpness ) {
                    pAfCtx->AfSearchCtx.MaxSharpness    = curPos->sharpness;
                    pAfCtx->AfSearchCtx.MaxSharpnessPos = *pLensPos;
                    pAfCtx->AfSearchCtx.Path.maxSharpnessPos = &curPos->nlist;
                    TRACE( AF_DEBUG, "%s: Now maxsharpness: %f, pos: %d\n",
                        __FUNCTION__, pAfCtx->AfSearchCtx.MaxSharpness, pAfCtx->AfSearchCtx.MaxSharpnessPos);
                }

                if ((curPos->sharpness < pAfCtx->AfSearchCtx.MinSharpness) || 
                    (pAfCtx->AfSearchCtx.MinSharpness == 0)) {
                    pAfCtx->AfSearchCtx.MinSharpness = curPos->sharpness;
                }

                focused = BOOL_FALSE;
                if (curPos->index>=2) {
                    TRACE( AF_DEBUG,"quick founding: %f %f \n",curPos->dSharpness,prePos->dSharpness);  
                    if (curPos->dSharpness<0) {
                        if (pAfCtx->AfSearchCtx.Path.index>0) {
                            focused = BOOL_TRUE;
                        } else {
                            if (curPos->dSharpness<-0.75) {
                                focused = BOOL_TRUE;
                            }

                            if ((prePos->dSharpness - curPos->dSharpness) > 1.1) {
                                focused = BOOL_TRUE;
                            }  

                            if ((curPos->dSharpness<0) && (prePos->dSharpness < 0)) {
                                if ((curPos->dSharpness+prePos->dSharpness) < -0.5) {
                                    focused = BOOL_TRUE;                            
                                }

                                if (curPos->index>=3) {
                                    AfSeachPos_t *pprePos;
                                    pprePos = container_of(prePos->plist.p_next, AfSeachPos_t, plist);
                                    if (pprePos->dSharpness<0) {
                                        if ((curPos->dSharpness+prePos->dSharpness+pprePos->dSharpness)<-0.5)
                                            focused = BOOL_TRUE;
                                    }
                                }
                            }
                        }
                    }
                }
                
                if (focused == BOOL_TRUE) {
                    result = AfSearchFine(pAfCtx,pLensPos);
                    if (result == RET_SUCCESS) 
                        goto focus_found;
                    else
                        focused = BOOL_FALSE;
                } else {
                    if (curPos->nlist.p_next != NULL) {
                        nxtPos = container_of(curPos->nlist.p_next, AfSeachPos_t, nlist);
                        pAfCtx->AfSearchCtx.Path.curPos = &nxtPos->nlist;

                        TRACE( AF_DEBUG, "%s: nxtPos(index: %d pos: %d sharpness: %f)\n", __FUNCTION__,
                            nxtPos->index,nxtPos->pos,nxtPos->sharpness);                        
                        if (nxtPos->sharpness != 0) {                            
                            goto new_afm_measure;
                        } else {
                            *pLensPos = nxtPos->pos;
                            result = RET_PENDING;
                        }
                    } else {                        
                        result = AfSearchFine(pAfCtx,pLensPos);
                        if (result == RET_SUCCESS) {
                            dSharpness = (pAfCtx->AfSearchCtx.MaxSharpness - pAfCtx->AfSearchCtx.MinSharpness)/
                                         (pAfCtx->AfSearchCtx.MinSharpness + pAfCtx->AfSearchCtx.MaxSharpness);

                            if (dSharpness < 0.35)  {                        
                                //ddl@rock-chips.com:  May be focus is failed ! 
                                focused = BOOL_TRUE;
                            } else {                    
                                focused = BOOL_TRUE;
                            }
                        }
                    }
                }

focus_found:
                if (focused == BOOL_TRUE) {
                    pAfCtx->AfSearchCtx.State = AFM_FSSTATE_FOCUSFOUND;
                    *pLensPos = pAfCtx->AfSearchCtx.MaxSharpnessPos;
                    result = RET_SUCCESS;
                    TRACE( AF_DEBUG, "%s: Found focus(pos: %d  sharpness: %f)!\n",
                        __FUNCTION__,pAfCtx->AfSearchCtx.MaxSharpnessPos,pAfCtx->AfSearchCtx.MaxSharpness);
                }
                 
                
                break;
            }

        default:
            {
                break;
            }
    }

    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

/******************************************************************************
 * AfIsiMdiFocusSet()
 *****************************************************************************/
bool_t AfIsiMdiFocusSet
(
    AfHandle_t          afHandle, 
    IsiSensorHandle_t   sensorHandle,
    const uint32_t      AbsStep
)
{
    AfContext_t *pAfCtx = (AfContext_t *)afHandle;
    int32_t diff_pos;
    RESULT result = RET_SUCCESS; 
    
    result = IsiMdiFocusSet(sensorHandle, AbsStep);
    if ( result == RET_SUCCESS ) {
        diff_pos = AbsStep - pAfCtx->LensePos;
        if (diff_pos<0)
            diff_pos = -diff_pos;

        pAfCtx->vcm_move_idx = 0;
        pAfCtx->vcm_move_frames = ((pAfCtx->vcm_movefull_t*(uint32_t)diff_pos)/pAfCtx->MaxFocus)/pAfCtx->afmFrmIntval + 1;
        
        TRACE(AF_DEBUG,"%s: set focus pos(%d %dms) success, frame interval: %dms, need %d frames for vcm move!\n",__FUNCTION__,
            AbsStep, ((pAfCtx->vcm_movefull_t*(uint32_t)diff_pos)/pAfCtx->MaxFocus),pAfCtx->afmFrmIntval,pAfCtx->vcm_move_frames);
    } else {
        TRACE(AF_ERROR, "%s: set focus pos(%d) error!\n",__FUNCTION__,AbsStep);
    }

    return result;
}


/******************************************************************************
 * AfEvtQueWr()
 *****************************************************************************/
bool_t AfSharpnessRawLogChk
(
    AfHandle_t       handle, 
    float            curSharpness,
    uint32_t         oneshot_or_contus
)
{
    AfContext_t *pAfCtx = (AfContext_t *)handle;
    float dSharpness, dSharpness1[2], dSharpness2[2];
    bool_t shot;

    dSharpness1[0] = 0.25;
    dSharpness1[1] = 0.35;

    dSharpness2[0] = 0.15;
    dSharpness2[1] = 0.25;

    DCT_ASSERT(oneshot_or_contus<2);
    
    dSharpness  = (pAfCtx->Sharpness - curSharpness) / (pAfCtx->Sharpness + curSharpness);
    dSharpness  = (dSharpness < 0.0f) ? -dSharpness : dSharpness;
    
    if (dSharpness > dSharpness1[oneshot_or_contus]) {
        if(pAfCtx->dSharpnessRawIdx>=2) { 
            dSharpness  = (pAfCtx->oldSharpness - curSharpness) / (pAfCtx->oldSharpness + curSharpness);
            dSharpness  = (dSharpness < 0.0f) ? -dSharpness : dSharpness;

            if ((dSharpness>0.3) || 
                (pAfCtx->dSharpnessRawLog[(pAfCtx->dSharpnessRawIdx-1)%10] > 0.3) ||
                (pAfCtx->dSharpnessRawLog[(pAfCtx->dSharpnessRawIdx-2)%10] > 0.3)) {
                shot = BOOL_FALSE;
            } else {
                shot = BOOL_TRUE;
            }            
        } else {
            shot = BOOL_TRUE;            
        }
    } else {

        if (dSharpness>dSharpness2[oneshot_or_contus]) {            
            if(pAfCtx->dSharpnessRawIdx>=4) { 
                dSharpness  = (pAfCtx->oldSharpness - curSharpness) / (pAfCtx->oldSharpness + curSharpness);
                dSharpness  = (dSharpness < 0.0f) ? -dSharpness : dSharpness;

                if ((dSharpness>0.05) || 
                    (pAfCtx->dSharpnessRawLog[(pAfCtx->dSharpnessRawIdx-1)%10] > 0.05) ||
                    (pAfCtx->dSharpnessRawLog[(pAfCtx->dSharpnessRawIdx-2)%10] > 0.05) ||
                    (pAfCtx->dSharpnessRawLog[(pAfCtx->dSharpnessRawIdx-3)%10] > 0.05) ||
                    (pAfCtx->dSharpnessRawLog[(pAfCtx->dSharpnessRawIdx-4)%10] > 0.05) ) {
                    shot = BOOL_FALSE;
                } else {
                    shot = BOOL_TRUE;
                }                
            } else {                
                shot = BOOL_FALSE;
            }
        } else {
            shot = BOOL_FALSE;
        }
    }
    /*
    TRACE(AF_DEBUG, "%s: %f/%f  dSharpness: %f  RawLog: [%f %f %f %f] shot = %d\n",
             __FUNCTION__,pAfCtx->Sharpness,curSharpness,
             dSharpness,
             (pAfCtx->dSharpnessRawIdx>=1)?pAfCtx->dSharpnessRawLog[(pAfCtx->dSharpnessRawIdx-1)%10]:0,
             (pAfCtx->dSharpnessRawIdx>=2)?pAfCtx->dSharpnessRawLog[(pAfCtx->dSharpnessRawIdx-2)%10]:0,
             (pAfCtx->dSharpnessRawIdx>=3)?pAfCtx->dSharpnessRawLog[(pAfCtx->dSharpnessRawIdx-3)%10]:0,
             (pAfCtx->dSharpnessRawIdx>=4)?pAfCtx->dSharpnessRawLog[(pAfCtx->dSharpnessRawIdx-4)%10]:0,
             shot
             ); 
    */
    return shot;
    
}


/******************************************************************************
 * AfEvtQueWr()
 *****************************************************************************/
void AfEvtQueWr
(
    List *list, 
    void *param    
)
{

    AfEvtQue_t *evtQue;
    AfEvt_t *evt = (AfEvt_t*)param;

    evtQue = container_of(list,AfEvtQue_t,list);

    osQueueWrite(&evtQue->queue, param);    
}


/******************************************************************************
 * AfEvtSignal()
 *****************************************************************************/
RESULT AfEvtSignal
(
    AfHandle_t       handle,    
    AfEvt_t          *evt
)
{
    AfContext_t *pAfCtx = (AfContext_t *)handle;

    /* initial checks */
    if ( pAfCtx == NULL )
    {
        TRACE( AF_ERROR, "%s: pAfCtx is NULL",__FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

    if ((pAfCtx->state == AF_STATE_INVALID) && (pAfCtx->state >= AF_STATE_MAX))
    {
        return ( RET_WRONG_STATE );
    }

    osMutexLock(&pAfCtx->EvtQuePool.lock);
    ListForEach(&pAfCtx->EvtQuePool.QueLst, AfEvtQueWr,(void*)evt);    
    osMutexUnlock(&pAfCtx->EvtQuePool.lock);

    return RET_SUCCESS;

}
/******************************************************************************
 * AfSearchReset()
 *****************************************************************************/
RESULT AfSearchReset
(
    AfHandle_t                  handle,
    float                       curSharpness
)
{
    AfContext_t *pAfCtx = (AfContext_t *)handle;
    AfSeachPos_t  *nPos,*pPos;
    int32_t i,maxPos,step,curPos,num,curPosIdx;
    List *list;
    bool_t setMdiFocus;
    RESULT result = RET_SUCCESS;

    TRACE( AF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAfCtx == NULL )
    {
        TRACE( AF_ERROR, "%s: pAfCtx is NULL",__FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

    if ( (AF_STATE_RUNNING == pAfCtx->state)
            || (AF_STATE_LOCKED == pAfCtx->state) )
    {
        TRACE( AF_ERROR, "%s: pAfCtx->state: %d is invalidate!",__FUNCTION__, pAfCtx->state );
        return ( RET_WRONG_STATE );
    }

    if ( pAfCtx->pAfSearchFunc == NULL )
    {
    	TRACE( AF_ERROR, "%s: pAfCtx->pAfSearchFunc is null\n", __FUNCTION__);
        return ( RET_WRONG_CONFIG );
    }

    if (!ListEmpty(&pAfCtx->AfSearchCtx.Path.nlist)) {
        list = ListGetHead(&pAfCtx->AfSearchCtx.Path.nlist);
        nPos = container_of(list, AfSeachPos_t, nlist);
        osFree(nPos);
        ListInit(&pAfCtx->AfSearchCtx.Path.nlist);
        ListInit(&pAfCtx->AfSearchCtx.Path.plist);
        nPos = NULL;
    }

    pAfCtx->AfSearchCtx.LensePosMin = pAfCtx->MinFocus;
    pAfCtx->AfSearchCtx.LensePosMax = pAfCtx->MaxFocus;

    pAfCtx->AfSearchCtx.Step = (pAfCtx->AfSearchCtx.LensePosMin - pAfCtx->AfSearchCtx.LensePosMax);
    pAfCtx->AfSearchCtx.Step /= ADAPTIVE_STEPS_PER_RANGE;
    
    maxPos = pAfCtx->AfSearchCtx.LensePosMax;
    result = IsiMdiFocusGet( pAfCtx->hSensor, (uint32_t *)(&curPos) );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    num = ((curPos%abs(pAfCtx->AfSearchCtx.Step))==0)?(ADAPTIVE_STEPS_PER_RANGE+1):(ADAPTIVE_STEPS_PER_RANGE+2);
    nPos = (AfSeachPos_t*)osMalloc(sizeof(AfSeachPos_t)*num);
    if (nPos == NULL) {
        TRACE( AF_ERROR, "%s: malloc AfSeachPos_t failed!\n",__FUNCTION__);
        return ( RET_FAILURE );
    }

    setMdiFocus = BOOL_FALSE;
    pAfCtx->AfSearchCtx.Path.curPos = NULL;
    pAfCtx->AfSearchCtx.Path.index  = 0;
    pAfCtx->AfSearchCtx.Path.maxSharpnessPos = NULL; 
    pAfCtx->AfSearchCtx.MinSharpness    = curSharpness;
    pAfCtx->AfSearchCtx.MaxSharpness    = curSharpness;
    pAfCtx->AfSearchCtx.MaxSharpnessPos = curPos;
    pPos = nPos + num - 1;
    curPosIdx = 0;
    TRACE(AF_DEBUG, "AF SeachPatch-%d(%d->%d):\n",pAfCtx->AfSearchCtx.Path.index,
        pAfCtx->AfSearchCtx.LensePosMin,pAfCtx->AfSearchCtx.LensePosMax);
    for (i=0; i<=ADAPTIVE_STEPS_PER_RANGE; i++) {
        nPos->index = i+curPosIdx;
        ListAddTail(&pAfCtx->AfSearchCtx.Path.nlist, &nPos->nlist);
        ListAddTail(&pAfCtx->AfSearchCtx.Path.plist, &pPos->plist);
        nPos->pos = maxPos + i*pAfCtx->AfSearchCtx.Step;        
        nPos->dSharpness = 0;
        if (nPos->pos == curPos) {
            nPos->sharpness = curSharpness;
            pAfCtx->AfSearchCtx.Path.maxSharpnessPos = &nPos->nlist;
        } else if ((curPos>nPos->pos) && ((curPos-nPos->pos)<abs(pAfCtx->AfSearchCtx.Step))) {
            curPosIdx = 1;
            nPos->pos = curPos;
            nPos->sharpness = curSharpness;
            pAfCtx->AfSearchCtx.Path.maxSharpnessPos = &nPos->nlist;

            TRACE(AF_DEBUG, "%s: nPos->index: %d  pos: %d  curPos: %d  curSharpness: %f\n",__FUNCTION__,
            nPos->index,nPos->pos,curPos,curSharpness);
            
            nPos++;
            pPos--;
            ListAddTail(&pAfCtx->AfSearchCtx.Path.nlist, &nPos->nlist);
            ListAddTail(&pAfCtx->AfSearchCtx.Path.plist, &pPos->plist);
            nPos->index = i+1;
            nPos->pos = maxPos + i*pAfCtx->AfSearchCtx.Step;
            nPos->dSharpness = 0;
            nPos->sharpness = 0;
        } else {
            nPos->sharpness = 0;        
            if (setMdiFocus == BOOL_FALSE) {
                setMdiFocus = BOOL_TRUE;
                AfIsiMdiFocusSet(handle, pAfCtx->hSensor, nPos->pos );
                IsiMdiFocusGet( pAfCtx->hSensor, (uint32_t *)(&pAfCtx->LensePos));                
                pAfCtx->AfSearchCtx.Path.curPos = &nPos->nlist;
            }
        }  

        TRACE(AF_DEBUG, "%s: nPos->index: %d  pos: %d  curPos: %d\n",__FUNCTION__,
            nPos->index,nPos->pos,curPos);
            
        nPos++;
        pPos--;
    } 
    
    pAfCtx->AfSearchCtx.State       = AFM_FSSTATE_SEARCHFOCUS;

    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__);
    return RET_SUCCESS;
    
}

/******************************************************************************
 * AfSearching()
 *****************************************************************************/
RESULT AfSearching
(
    AfHandle_t                  handle,
    CamerIcAfmMeasuringResult_t *pMeasResults
)
{
    AfContext_t *pAfCtx = (AfContext_t *)handle;
    AfEvt_t evt;
    int32_t NewLensePosition = pAfCtx->LensePos;
    RESULT result = RET_SUCCESS;

    TRACE( AF_INFO, "%s: (enter)\n", __FUNCTION__);
    switch (pAfCtx->state)
    {
        case AF_STATE_RUNNING:
        {
            result = pAfCtx->pAfSearchFunc( handle, pMeasResults, &NewLensePosition );
            if ( (result==RET_PENDING) && (NewLensePosition != pAfCtx->LensePos) )
            {
                result = AfIsiMdiFocusSet(handle, pAfCtx->hSensor, (uint32_t)NewLensePosition );
                if ( result != RET_SUCCESS )
                {
                    return ( RET_SUCCESS );
                }
                pAfCtx->LensePos = NewLensePosition;
            }
            else if ( (result==RET_SUCCESS) && (pAfCtx->AfSearchCtx.State == AFM_FSSTATE_FOCUSFOUND) )
            {
                if (pAfCtx->AfSearchCtx.MaxSharpness <= SHARPNESS_MIN) {
                    result = AfIsiMdiFocusSet(handle,  pAfCtx->hSensor, (uint32_t)pAfCtx->MaxFocus );
                    NewLensePosition = pAfCtx->MaxFocus;
                    TRACE(AF_DEBUG, " MaxSharpness(%f) < %d, Fix max focus!!!!!!!!",
                        pAfCtx->AfSearchCtx.MaxSharpness,SHARPNESS_MIN);
                } else {            
                    result = AfIsiMdiFocusSet(handle,  pAfCtx->hSensor, (uint32_t)NewLensePosition );
                }
                if ( result != RET_SUCCESS )
                {
                    return ( RET_SUCCESS );
                }
                pAfCtx->Sharpness   = pAfCtx->AfSearchCtx.MaxSharpness;
                pAfCtx->LensePos    = NewLensePosition;            
                pAfCtx->state       = AF_STATE_STOPPED;

                pAfCtx->oneShotWait = ONSHOT_WAIT_CNT;
            } 
            else 
            {
                pAfCtx->oneShotWait = 0;
            }
            
            break;
        }

        case AF_STATE_TRACKING:
        {
            if ( pAfCtx->AfSearchCtx.State != AFM_FSSTATE_FOCUSFOUND ) {
                result = pAfCtx->pAfSearchFunc( handle, pMeasResults, &NewLensePosition );
                if ( (result==RET_PENDING) && (NewLensePosition != pAfCtx->LensePos) ) {
                    result = AfIsiMdiFocusSet(handle,  pAfCtx->hSensor, (uint32_t)NewLensePosition );
                    if ( result != RET_SUCCESS )
                    {
                        return ( RET_SUCCESS );
                    }
                    pAfCtx->LensePos = NewLensePosition;

                    if (pAfCtx->trackNotify == BOOL_TRUE) {
                        if (pAfCtx->AfSearchCtx.Step != 1) 
                        {
                            evt.evnt_id = AFM_AUTOFOCUS_MOVE;
                            evt.info.mveEvt.start = BOOL_TRUE;
                            AfEvtSignal(handle, &evt);
                        }
                    }

                } else if ( (result==RET_SUCCESS) && (pAfCtx->AfSearchCtx.State == AFM_FSSTATE_FOCUSFOUND) ) {                   
                    if (pAfCtx->AfSearchCtx.MaxSharpness <= SHARPNESS_MIN) {
                        result = AfIsiMdiFocusSet(handle,  pAfCtx->hSensor, (uint32_t)pAfCtx->MaxFocus );
                        NewLensePosition = pAfCtx->MaxFocus;
                        TRACE(AF_DEBUG, " MaxSharpness(%f) < %d, Fix max focus!!!!!!!!",
                            pAfCtx->AfSearchCtx.MaxSharpness,SHARPNESS_MIN);
                    } else {            
                        result = AfIsiMdiFocusSet(handle,  pAfCtx->hSensor, (uint32_t)NewLensePosition );
                    }
                    if ( result != RET_SUCCESS )
                    {
                        return ( RET_SUCCESS );
                    }
                    
                    pAfCtx->dSharpness  = 0.0f;
                    pAfCtx->Sharpness   = pAfCtx->AfSearchCtx.MaxSharpness;
                    pAfCtx->LensePos    = NewLensePosition;

                    if (pAfCtx->trackNotify == BOOL_TRUE) {
                        evt.evnt_id = AFM_AUTOFOCUS_MOVE;
                        evt.info.mveEvt.start = BOOL_FALSE;
                        AfEvtSignal(handle, &evt);
                    }

                }
            }

            
            break;
        }

        default:
            break;
    }

    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__);
    return result;
}

/******************************************************************************
 * CamAfThreadHandler
 *****************************************************************************/
static int32_t SOC_CamAfThreadHandler
(
    void *p_arg
)
{
    AfEvt_t evt;
	//CamEngineAfEvt_t evnt;
    bool bExit = false;
    int evnt_id;
	int tmp_step;
    if ( p_arg )
    {
        AfContext_t *pAfCtx = (AfContext_t *)p_arg;
	    while (bExit == false)
	    {
	        /* wait for next command */
	        SOC_AfCmd_t SOC_af_command;
	        OSLAYER_STATUS osStatus = osQueueRead(&pAfCtx->SOC_af_commandQueue, &SOC_af_command);
	        if (OSLAYER_OK != osStatus)
	        {
	            TRACE( AF_ERROR, "%s (receiving command failed -> OSLAYER_RESULT=%d)\n", __FUNCTION__, osStatus );
	            continue; /* for now we simply try again */
	        }
	        evnt_id = SOC_af_command.cmdId;
	        switch (evnt_id)
	        {
	            case SOC_AF_CMD_FIRMWARE:
	            {
					TRACE( AF_DEBUG, "Receive SOC_AF_CMD_FIRMWARE");
					pAfCtx->state = AF_STATE_DNFIRMWARE;
					IsiMdiFocusSet( pAfCtx->hSensor, 1);
					pAfCtx->state = AF_STATE_STOPPED;
					break;
	            }

	            case SOC_AF_CMD_FOCUS:
	            {
					TRACE( AF_DEBUG, "Receive SOC_AF_CMD_FOCUS");
					pAfCtx->state = AF_STATE_RUNNING;
					IsiMdiFocusSet( pAfCtx->hSensor, 2);
					evt.evnt_id = AFM_AUTOFOCUS_FINISHED;
                    evt.info.fshEvt.focus = BOOL_TRUE;
                    AfEvtSignal(pAfCtx, &evt);
					pAfCtx->state = AF_STATE_STOPPED;
					break;
	            }
                case SOC_AF_CMD_STOP:
                {
					TRACE( AF_DEBUG, "Receive SOC_AF_CMD_STOP");
	                pAfCtx->state = AF_STATE_STOPPED;
	                break;
                }
	            case SOC_AF_CMD_EXIT:
	            {
					TRACE( AF_DEBUG, "Receive SOC_AF_CMD_EXIT");
	                bExit = true;
	                break;
	            }
	            default:
	            {
					TRACE( AF_ERROR, "%s soc af cmd : %d is invalidate!", __FUNCTION__,evnt_id);
	                break;
	            }
	        }
	    }
    }
    return 0;
}




/******************************************************************************
 * Implementation of AF API Functions
 *****************************************************************************/


/******************************************************************************
 * AfInit()
 *****************************************************************************/
RESULT AfInit
(
    AfInstanceConfig_t *pInstConfig
)
{
    RESULT result = RET_SUCCESS;

    AfContext_t *pAfCtx;

    TRACE( AF_INFO, "INFO (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pInstConfig == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    /* allocate auto focus control context */
    pAfCtx = ( AfContext_t *)osMalloc( sizeof(AfContext_t) );
    if ( pAfCtx == NULL )
    {
        TRACE( AF_ERROR,  "%s: Can't allocate AF context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }

    /* pre-initialize context */
    MEMSET( pAfCtx, 0, sizeof(*pAfCtx) );

    ListInit(&pAfCtx->EvtQuePool.QueLst);
    osMutexInit(&pAfCtx->EvtQuePool.lock);

    osQueueInit(&pAfCtx->afmCmdQue, 10, sizeof(AfmCmdItem_t));
    osQueueInit(&pAfCtx->afmChkAckQue, 1, sizeof(AfChkAckItem_t));
    osEventInit(&pAfCtx->event_afm,1,0);

    /* create command queue */
    if ( OSLAYER_OK != osQueueInit( &pAfCtx->SOC_af_commandQueue, 10, sizeof(SOC_AfCmd_t) ) )
    {
        TRACE( AF_ERROR, "%s (creating command queue of depth: %d failed)\n", __FUNCTION__, pAfCtx->SOC_af_maxCommands );
        result = RET_FAILURE;
        //goto cleanup_2;
    }

    /* create handler thread */
    if ( OSLAYER_OK != osThreadCreate( &pAfCtx->SOC_af_thread, SOC_CamAfThreadHandler, pAfCtx) )
    {
        TRACE( AF_ERROR, "%s (creating handler thread failed)\n", __FUNCTION__ );
        result = RET_FAILURE;
        //goto cleanup_3;
    }

    pAfCtx->afm_cnt = 0;
    pAfCtx->afm_inval = 5;
    
    pAfCtx->state = AF_STATE_INITIALIZED;    
    /* return handle */
    pInstConfig->hAf = (AfHandle_t)pAfCtx;

    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__);
    
    return ( result );
}



/******************************************************************************
 * AfRelease()
 *****************************************************************************/
RESULT AfRelease
(
    AfHandle_t handle
)
{
    AfContext_t *pAfCtx = (AfContext_t *)handle;
    SOC_AfCmd_t SOC_af_command;
    RESULT result = RET_SUCCESS;

    TRACE( AF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAfCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check state */
    if ( (AF_STATE_RUNNING == pAfCtx->state)
            || (AF_STATE_LOCKED == pAfCtx->state) )
    {
        return ( RET_BUSY );
    }

    if (!ListEmpty(&pAfCtx->AfSearchCtx.Path.nlist)) {
        AfSeachPos_t  *nPos;
                
        nPos = container_of(ListGetHead(&pAfCtx->AfSearchCtx.Path.nlist), AfSeachPos_t, nlist);
        osFree(nPos);
        ListInit(&pAfCtx->AfSearchCtx.Path.nlist);
        ListInit(&pAfCtx->AfSearchCtx.Path.plist);
        nPos = NULL;
    }
    
    osMutexDestroy(&pAfCtx->EvtQuePool.lock);
    osEventDestroy(&pAfCtx->event_afm);
    osQueueDestroy(&pAfCtx->afmChkAckQue);
    osQueueDestroy(&pAfCtx->afmCmdQue);

	SOC_af_command.cmdId = SOC_AF_CMD_EXIT;
	SOC_af_command.pCmdCtx = pAfCtx;
	OSLAYER_STATUS osStatus = osQueueWrite(&pAfCtx->SOC_af_commandQueue, &SOC_af_command);
    if ( osStatus != OSLAYER_OK )
    {
        TRACE( AF_ERROR, "%s: (sending command to queue failed -> OSLAYER_STATUS=%d)\n", __FUNCTION__, osStatus );
    }

	/* wait for handler thread to have stopped due to the shutdown command given above */
    if ( OSLAYER_OK != osThreadWait( &pAfCtx->SOC_af_thread ) )
    {
        TRACE( AF_ERROR, "%s (waiting for handler thread failed)\n", __FUNCTION__ );
        UPDATE_RESULT( result, RET_FAILURE );
    }

    /* destroy handler thread */
    if ( OSLAYER_OK != osThreadClose( &pAfCtx->SOC_af_thread ) )
    {
        TRACE( AF_ERROR, "%s (closing handler thread failed)\n", __FUNCTION__ );
        UPDATE_RESULT( result, RET_FAILURE );
    }

	osQueueDestroy(&pAfCtx->SOC_af_commandQueue);

    MEMSET( pAfCtx, 0, sizeof(AfContext_t) );
    osFree ( pAfCtx );

    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AfConfigure()
 *****************************************************************************/
RESULT AfConfigure
(
    AfHandle_t handle,
    AfConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    AfContext_t *pAfCtx = (AfContext_t*)handle;
    SOC_AfCmd_t SOC_af_command;
    TRACE( AF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAfCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ( pConfig == NULL           ) || 
         ( pConfig->hSensor == NULL  ) ||
         ( pConfig->hCamerIc == NULL ) )
    {
        return ( RET_INVALID_PARM );
    }

    if ( (pConfig->Afss <= AFM_FSS_INVALID)
            || (pConfig->Afss >= AFM_FSS_MAX) )
    {
        return ( RET_OUTOFRANGE );
    }

    if ( (AF_STATE_INITIALIZED != pAfCtx->state)
            && (AF_STATE_STOPPED != pAfCtx->state) )
    {
        return ( RET_WRONG_STATE );
    }

    pAfCtx->hCamerIc = pConfig->hCamerIc;

    /* update context */
    switch ( pConfig->Afss )
    {
        case AFM_FSS_FULLRANGE:
            {
                pAfCtx->pAfSearchFunc = AfSearchFullRange;
                break;
            }

        case AFM_FSS_HILLCLIMBING:
            {
                pAfCtx->pAfSearchFunc = AfSearchHillClimb;
                break;
            }

        case AFM_FSS_ADAPTIVE_RANGE:
            {
                pAfCtx->pAfSearchFunc = AfSearchAdaptiveRange;
                break;
            }

        default:
            {
                return ( RET_OUTOFRANGE );
            }
    }

    pAfCtx->Afss        = pConfig->Afss;
    pAfCtx->hSensor     = pConfig->hSensor;
    pAfCtx->hSubSensor  = pConfig->hSubSensor;

    result = IsiMdiInitMotoDrive( pAfCtx->hSensor );
	if (result == RET_SOC_AF) {
        pAfCtx->isSOCAf = true;
        TRACE( AF_INFO, "This is SOC AF\n", __FUNCTION__);
		SOC_af_command.cmdId = SOC_AF_CMD_FIRMWARE;
        SOC_af_command.pCmdCtx = pAfCtx;
        osQueueWrite(&pAfCtx->SOC_af_commandQueue, &SOC_af_command);
		return RET_SUCCESS;
	} else if ( result != RET_SUCCESS )
    {
        pAfCtx->isSOCAf = false;
        return ( result );
    }

    pAfCtx->MinFocus = 0U;
    result = IsiMdiSetupMotoDrive( pAfCtx->hSensor, &pAfCtx->MaxFocus );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }
    pAfCtx->MaxFocus &= 0xffff;

    result = AfSearchInit( (AfHandle_t)pAfCtx, pAfCtx->MinFocus, pAfCtx->MaxFocus );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    result = IsiMdiFocusGet( pAfCtx->hSensor, (uint32_t *)(&pAfCtx->LensePos) );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    TRACE( AF_DEBUG, "%s: pAfCtx->MaxFocus: %d\n", __FUNCTION__, pAfCtx->MaxFocus);

    TRACE( AF_DEBUG, "%s: focus-range: %d..%d current: %d\n",
                __FUNCTION__, pAfCtx->MinFocus, pAfCtx->MaxFocus, pAfCtx->LensePos );

    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AfReConfigure()
 *****************************************************************************/
RESULT AfReConfigure
(
    AfHandle_t handle
)
{
    RESULT result = RET_SUCCESS;

    AfContext_t *pAfCtx = (AfContext_t*)handle;

    TRACE( AF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAfCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (AF_STATE_LOCKED != pAfCtx->state)
            && (AF_STATE_RUNNING != pAfCtx->state)
            && (AF_STATE_STOPPED != pAfCtx->state) )
    {
        return ( RET_WRONG_STATE );
    }
    
    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AfSettled()
 *****************************************************************************/
RESULT AfSettled
(
    AfHandle_t  handle,
    bool_t      *pSettled
)
{
    AfContext_t *pAfCtx = (AfContext_t *)handle;

    TRACE( AF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAfCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pSettled == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    switch ( pAfCtx->state )
    {
        case AF_STATE_STOPPED:
        {
            *pSettled = (pAfCtx->AfSearchCtx.State == AFM_FSSTATE_FOCUSFOUND) ?  BOOL_TRUE : BOOL_FALSE;
            break;
        }

        case AF_STATE_TRACKING:
        {
            *pSettled = (pAfCtx->AfSearchCtx.State == AFM_FSSTATE_FOCUSFOUND) ?  BOOL_TRUE : BOOL_FALSE;
            break;
        }

        default:
        {
            *pSettled = BOOL_FALSE;
        }
    }

    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AfStart()
 *****************************************************************************/
RESULT AfStart
(
    AfHandle_t                handle,
    const AfSearchStrategy_t  fss
)
{
    AfContext_t *pAfCtx = (AfContext_t *)handle;
    RESULT result = RET_SUCCESS;
    AfmCmdItem_t afCmd;
    
    TRACE( AF_DEBUG, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAfCtx == NULL )
    {
        TRACE( AF_ERROR, "%s: pAfCtx is NULL!\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

    if ( (AF_STATE_RUNNING == pAfCtx->state)
            || (AF_STATE_LOCKED == pAfCtx->state) )
    {
        TRACE( AF_ERROR, "%s: pAfCtx->state(%d) is error!\n", __FUNCTION__,pAfCtx->state);
        return ( RET_WRONG_STATE );
    }
    

    afCmd.cmd = AF_TRACKING;
    afCmd.fss = fss;
    osQueueWrite(&pAfCtx->afmCmdQue,&afCmd);
    TRACE( AF_DEBUG, "%s: (exit)\n", __FUNCTION__);
    
    return ( result );
}



/******************************************************************************
 * AfOneShot()
 *****************************************************************************/
RESULT AfOneShot
(
    AfHandle_t                handle,
    const AfSearchStrategy_t  fss
)
{
    AfContext_t *pAfCtx = (AfContext_t *)handle;
    AfmCmdItem_t afCmd;
    RESULT result = RET_SUCCESS;
    SOC_AfCmd_t SOC_af_command;
    TRACE( AF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAfCtx == NULL )
    {
        TRACE( AF_ERROR, "%s: pAfCtx is NULL!\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

    if ( (AF_STATE_RUNNING == pAfCtx->state)
            || (AF_STATE_LOCKED == pAfCtx->state)
            || (AF_STATE_DNFIRMWARE== pAfCtx->state))
    {
        TRACE( AF_ERROR, "%s: pAfCtx->state(%d) is error!\n", __FUNCTION__,pAfCtx->state);
        return ( RET_WRONG_STATE );
    }
	if (pAfCtx->isSOCAf) {
	    SOC_af_command.cmdId = SOC_AF_CMD_FOCUS;
		SOC_af_command.pCmdCtx = pAfCtx;
		osQueueWrite(&pAfCtx->SOC_af_commandQueue, &SOC_af_command);
	} else {
	    afCmd.cmd = AF_ONESHOT;
	    afCmd.fss = fss;
	    osQueueWrite(&pAfCtx->afmCmdQue,&afCmd);
	}

    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AfStop()
 *****************************************************************************/
RESULT AfStop
(
    AfHandle_t handle
)
{
    AfContext_t *pAfCtx = (AfContext_t *)handle;
    AfmCmdItem_t afCmd;
	SOC_AfCmd_t command;
    TRACE( AF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAfCtx == NULL )
    {
        TRACE( AF_ERROR, "%s: pAfCtx is NULL!\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

    if ( AF_STATE_LOCKED == pAfCtx->state )
    {
        TRACE( AF_ERROR, "%s: pAfCtx->state(%d) is error!\n", __FUNCTION__,pAfCtx->state);
        return ( RET_WRONG_STATE );
    }

    if ( (AF_STATE_RUNNING == pAfCtx->state) 
		|| (AF_STATE_TRACKING== pAfCtx->state)
        || (AF_STATE_DNFIRMWARE== pAfCtx->state)) {
        if(!pAfCtx->isSOCAf) {
	        afCmd.cmd = AF_STOP;
	        osQueueWrite(&pAfCtx->afmCmdQue,&afCmd);
		}
    } else {
        TRACE( AF_DEBUG,"%s: pAfCtx->state: %d isn't been stoped!",__FUNCTION__, pAfCtx->state);
    }
    
    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}




/******************************************************************************
 * AfStatus()
 *****************************************************************************/
RESULT AfStatus
(
    AfHandle_t          handle,
    bool_t              *pRunning,
    AfSearchStrategy_t  *pFss,
    float               *sharpness
)
{
    AfContext_t *pAfCtx = (AfContext_t *)handle;

    TRACE( AF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAfCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (pRunning == NULL) || (pFss == NULL) )
    {
        return ( RET_INVALID_PARM );
    }

    *pRunning = ( ( pAfCtx->state == AF_STATE_RUNNING )
                    || ( pAfCtx->state == AF_STATE_LOCKED ) );
    *pFss = pAfCtx->Afss;
    if (sharpness != NULL)
        *sharpness = pAfCtx->Sharpness;
    
    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}

/******************************************************************************
 * AfMeasureCbRestart()
 *****************************************************************************/
RESULT AfMeasureCbRestart
(
    AfHandle_t                  handle
)
{
    AfContext_t *pAfCtx = (AfContext_t *)handle;

    /* initial checks */
    if ( pAfCtx == NULL )
    {
    	TRACE( AF_ERROR, "%s: pAfCtx is null\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

    pAfCtx->afm_cnt = 0;
    pAfCtx->afm_inval = 5;
    pAfCtx->afmFrmIntval = 0;
    t0=0;
    t1=0;

    return RET_SUCCESS;
}
/******************************************************************************
 * AfProcessFrame()
 *****************************************************************************/
RESULT AfProcessFrame
(
    AfHandle_t                  handle,
    CamerIcAfmMeasuringResult_t *pMeasResults
)
{
    AfContext_t *pAfCtx = (AfContext_t *)handle;
    AfmCmdItem_t afmCmd;
    uint32_t i;
    int32_t afcmd_read;
    float dOldSharpness,curSharpness,dSharpness, dSharpness2;
    RESULT result = RET_SUCCESS;
    CamerIcWindow_t measureWdw;
    
    TRACE( AF_INFO, "%s: (enter)\n", __FUNCTION__);
    
    /* initial checks */
    if ( pAfCtx == NULL )
    {
    	TRACE( AF_ERROR, "%s: pAfCtx is null\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

    if ( pMeasResults == NULL )
    {
    	TRACE( AF_ERROR, "%s: pMeasResults is null\n", __FUNCTION__);
        return ( RET_INVALID_PARM );
    }

    if ( pAfCtx->pAfSearchFunc == NULL )
    {
    	TRACE( AF_ERROR, "%s: pAfCtx->pAfSearchFunc is null\n", __FUNCTION__);
        return ( RET_WRONG_CONFIG );
    }

    if (t0==0) {
        osTimeStampUs(&t0);
    } else {
        if (t1==0) {
            osTimeStampUs(&t1);
        }
    }

    if ((t0>0) && (t1>0) && (pAfCtx->afmFrmIntval==0)) {
        pAfCtx->afmFrmIntval = ((t1-t0)/1000);
    }
    
    curSharpness  = AfGetSingleSharpness( handle, pMeasResults );
    AfGetMeasureWindow(handle, pMeasResults, &measureWdw);

    
    afcmd_read = osQueueTryRead(&pAfCtx->afmCmdQue, &afmCmd);

    if (afcmd_read == OSLAYER_OK) {        
        switch (afmCmd.cmd)
        {
            case AF_TRACKING:
            {
                TRACE(AF_DEBUG, "%s: Receive cmd: AF_TRACKING\n",__FUNCTION__);
                pAfCtx->afm_cnt = 0;
                pAfCtx->afm_inval = 5;
                pAfCtx->dSharpnessRawIdx = 0;
                pAfCtx->state = AF_STATE_TRACKING;
                break;
            }

            case AF_STOP:
            {
                TRACE(AF_DEBUG, "%s: Receive cmd: AF_STOP\n",__FUNCTION__);
                pAfCtx->state = AF_STATE_STOPPED;
                break;
            }

            case AF_SHOTCHK:
            {                
                AfChkAckItem_t chkAck;
                bool_t shot,rawlog_chk;

                chkAck.shot = BOOL_TRUE;
                TRACE(AF_DEBUG, "%s: Receive cmd: AF_SHOTCHK\n",__FUNCTION__);
                if (pAfCtx->state == AF_STATE_RUNNING) {
                    shot = BOOL_FALSE;  
                } else if ( (pAfCtx->state == AF_STATE_STOPPED)||( pAfCtx->state == AF_STATE_TRACKING )) {            
                    if (pAfCtx->state == AF_STATE_STOPPED) {
                        rawlog_chk = BOOL_TRUE;
                    } else if (pAfCtx->state == AF_STATE_TRACKING) {
                        if (pAfCtx->AfSearchCtx.State == AFM_FSSTATE_FOCUSFOUND) {
                            rawlog_chk = BOOL_TRUE;
                        } else {
                            
                            shot = BOOL_TRUE;
                        }
                    }
                    if (rawlog_chk == BOOL_TRUE) {
                        shot = AfSharpnessRawLogChk(handle,curSharpness,0);
                    } else {
                        TRACE(AF_DEBUG, "%s: pAfCtx->state: %d pAfCtx->AfSearchCtx.State: %d, shot: %d\n",__FUNCTION__,
                            pAfCtx->state,pAfCtx->AfSearchCtx.State,shot);
                    }                    
                }
                osQueueTryRead(&pAfCtx->afmChkAckQue, &chkAck);
                chkAck.shot = shot;
                osQueueWrite(&pAfCtx->afmChkAckQue, &chkAck);
                break;
            }
            default:
                break;
        }
    }

    if (pAfCtx->vcm_move_idx < pAfCtx->vcm_move_frames) {        
        result = RET_CANCELED;
        /*TRACE(AF_DEBUG, "%s: pAfCtx->vcm_move_idx(%d) < pAfCtx->vcm_move_frames(%d),Ignore!\n",
            __FUNCTION__,pAfCtx->vcm_move_idx,pAfCtx->vcm_move_frames);*/
        pAfCtx->vcm_move_idx++; 

        if (afcmd_read == OSLAYER_OK) {            
            if (afmCmd.cmd == AF_ONESHOT) {
                osQueueWrite(&pAfCtx->afmCmdQue,&afmCmd);
                TRACE(AF_DEBUG, "%s: AF_ONESHOT next frame exec!",__FUNCTION__);
            }
        }
        
        goto end;
    }    

    if (afcmd_read == OSLAYER_OK) {            
        if (afmCmd.cmd == AF_ONESHOT) {
            AfSearchReset(handle, curSharpness);
            pAfCtx->afm_cnt = pAfCtx->afm_inval;
            pAfCtx->state = AF_STATE_RUNNING;
            result = RET_SUCCESS;
            goto end;
        }
    }    

    if (pAfCtx->afm_cnt < pAfCtx->afm_inval) {
        pAfCtx->afm_cnt++;
        TRACE(AF_DEBUG, "%s: pAfCtx->afm_cnt(%d) < pAfCtx->afm_inval(%d),Ignore!\n",
            __FUNCTION__,pAfCtx->afm_cnt,pAfCtx->afm_inval);
        result = RET_CANCELED;
        goto end;
    }
    
    dSharpness  = (pAfCtx->oldSharpness - curSharpness) / (pAfCtx->oldSharpness + curSharpness); 
    dSharpness  = (dSharpness < 0.0f) ? -dSharpness : dSharpness;
    pAfCtx->oldSharpness = curSharpness;

    pAfCtx->dSharpnessRawIdx++;
    pAfCtx->dSharpnessRawLog[pAfCtx->dSharpnessRawIdx%10] = dSharpness;  
    
    if ((pAfCtx->AfSearchCtx.State == AFM_FSSTATE_SEARCHFOCUS) || 
        (pAfCtx->AfSearchCtx.State == AFM_FSSTATE_INIT)) {
        result = AfSearching(handle,pMeasResults);
        if (pAfCtx->AfSearchCtx.State == AFM_FSSTATE_FOCUSFOUND) {
            pAfCtx->dSharpnessRawIdx=0;
            memset(pAfCtx->dSharpnessRawLog,0x00,sizeof(pAfCtx->dSharpnessRawLog));
        }
        goto end;
    } else {
        pAfCtx->vcm_move_idx = pAfCtx->vcm_move_frames + 1;
    }
    
    switch (pAfCtx->state)
    {
        case AF_STATE_TRACKING:
        {    
            if ((pAfCtx->AfSearchCtx.State == AFM_FSSTATE_FOCUSFOUND) && (pAfCtx->dSharpnessRawIdx == 1)) {
                pAfCtx->Sharpness = curSharpness;
            }
            if ( (measureWdw.vOffset != pAfCtx->measureWdw.vOffset) ||
                (measureWdw.hOffset != pAfCtx->measureWdw.hOffset) ||
                (measureWdw.width != pAfCtx->measureWdw.width) ||
                (measureWdw.height != pAfCtx->measureWdw.height) ) {
                /* ddl@rock-chips.com: v1.6.0 */
                pAfCtx->WdwChanged = BOOL_TRUE;
                pAfCtx->StableFrames = 0;
                pAfCtx->measureWdw = measureWdw;
                TRACE(AF_DEBUG,">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>WindowChanged---1");
            } else {            
                /* ddl@rock-chips.com: v1.5.0 */ 
                if (((pAfCtx->AfSearchCtx.State == AFM_FSSTATE_FOCUSFOUND) && (pAfCtx->dSharpnessRawIdx <= 3)) == BOOL_FALSE) {
                    if (pAfCtx->MachineMoved == BOOL_FALSE) {
                        if (dSharpness > 0.05) {
                            TRACE(AF_DEBUG,">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>MachineMoved---1");
                            pAfCtx->MachineMoved = BOOL_TRUE;
                            pAfCtx->StableFrames = 0;
                        }
                    }
                }
            }

            if ((pAfCtx->MachineMoved == BOOL_TRUE) || (pAfCtx->WdwChanged == BOOL_TRUE)) {
                if (dSharpness > 0.02) {
                    pAfCtx->StableFrames = 0;
                } else {
                    pAfCtx->StableFrames++;
                    TRACE(AF_DEBUG,"StableFrames: %d",pAfCtx->StableFrames);
                }

                if ((pAfCtx->StableFrames*pAfCtx->afmFrmIntval) >= TRACKING_STABLE_TIMES) {
                    pAfCtx->StableFrames = 0;                    
                        
                    pAfCtx->trackNotify = (pAfCtx->WdwChanged == BOOL_TRUE)?BOOL_FALSE:BOOL_TRUE;
                    pAfCtx->MachineMoved = BOOL_FALSE;
                    pAfCtx->WdwChanged = BOOL_FALSE;
                    
                    
                    TRACE(AF_DEBUG,">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>StartAf---0");
                    AfSearchReset(handle, curSharpness);
                    pAfCtx->measureWdw = measureWdw;
                }
            }
            
            break;
        }

        case AF_STATE_STOPPED:
        {   
            AfEvt_t evt;
            
            if (pAfCtx->oneShotWait) {
                pAfCtx->oneShotWait--;
                if (pAfCtx->oneShotWait == 0) {
                    curSharpness  = AfGetSingleSharpness( handle, pMeasResults );
                    dSharpness  = (pAfCtx->Sharpness - curSharpness) / (pAfCtx->Sharpness + curSharpness);
                    dSharpness = (dSharpness<0.0f) ? -dSharpness : dSharpness;
                        
                    evt.evnt_id = AFM_AUTOFOCUS_FINISHED;
                    if (dSharpness >= 0.25)                   
                        evt.info.fshEvt.focus = BOOL_FALSE;
                    AfEvtSignal(handle, &evt);                     

                    pAfCtx->Sharpness = curSharpness;
                    
                    TRACE(AF_ERROR, "%s: OneShot finished, sharpness(%f/%f, %f)\n",
                            __FUNCTION__,curSharpness,pAfCtx->Sharpness,dSharpness);
                    
                    pAfCtx->measureWdw = measureWdw;
                }
            }
            result = RET_CANCELED;
            break;
        }

        default:
            break;
    }
end:    
    
    /* ddl@rock-chips.com: v0.0x28.0 : add debug trace */
    if (result == RET_PENDING) {
        TRACE(AF_ERROR, "WARNNING: %s result is RET_PENDING, pAfCtx->LensePos: %d, Search list is:\n",__FUNCTION__,pAfCtx->LensePos);
        
        List *l;
        AfSeachPos_t *pPos;
        
        l = ListHead( &pAfCtx->AfSearchCtx.Path.nlist );
        while ( l )
        {
            pPos = container_of(l, AfSeachPos_t, nlist);
            if (pAfCtx->AfSearchCtx.Path.curPos == l) {
                TRACE(AF_ERROR, "  ->index: %d  pos: %d  sharpness: %f\n",
                    pPos->index, pPos->pos, pPos->sharpness);
            } else {
                TRACE(AF_ERROR, "    index: %d  pos: %d  sharpness: %f\n",
                    pPos->index, pPos->pos, pPos->sharpness);
            }
            l = l->p_next;
        }
    }
    
    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__);
    return result;

}

/******************************************************************************
 * AfTryLock()
 *****************************************************************************/
RESULT AfTryLock
(
    AfHandle_t handle
)
{
    AfContext_t *pAfCtx = (AfContext_t *)handle;

    RESULT result = RET_FAILURE;
    bool_t settled = BOOL_FALSE;

    TRACE( AF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAfCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    switch ( pAfCtx->state )
    {
        case AF_STATE_RUNNING:
            {
                result = RET_PENDING;
                break;
            }
        case AF_STATE_STOPPED:
        case AF_STATE_TRACKING:
            {
                if ( pAfCtx->AfSearchCtx.State != AFM_FSSTATE_FOCUSFOUND ) 
                {
                    result = RET_PENDING;
                }
                break;
            }

        default:
        {
            return ( RET_WRONG_STATE );
        }
    }

    if ( result != RET_PENDING )
    {
        result = AfSettled( handle, &settled );
        if ( (RET_SUCCESS == result) && (BOOL_TRUE == settled) )
        {
            pAfCtx->state_before_lock = pAfCtx->state;
            pAfCtx->state = AF_STATE_LOCKED;
            result = RET_SUCCESS;
        }
        else
        {
            result = RET_PENDING;
        }
    }

    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AfUnLock()
 *****************************************************************************/
RESULT AfUnLock
(
    AfHandle_t handle
)
{
    AfContext_t *pAfCtx = (AfContext_t *)handle;

    RESULT result = RET_FAILURE;

    TRACE( AF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAfCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    switch ( pAfCtx->state )
    {
        case AF_STATE_LOCKED:
            {
                pAfCtx->state = pAfCtx->state_before_lock;
                result = RET_SUCCESS;

                break;
            }

        default:
            {
                result = RET_WRONG_STATE;
                break;
            }
    }
 
    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/******************************************************************************
 * AfShotCheck()
 *****************************************************************************/
RESULT AfShotCheck
(
    AfHandle_t                  handle,
    bool_t                      *shot
)
{
    AfContext_t *pAfCtx = (AfContext_t *)handle;
    AfmCmdItem_t afCmd;
    AfChkAckItem_t ChkAck;
    int32_t err;
    RESULT result = RET_SUCCESS;

    /* initial checks */
    if ( pAfCtx == NULL )
    {
    	TRACE( AF_ERROR, "%s: pAfCtx is null\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }
    if(!pAfCtx->isSOCAf){
	    afCmd.cmd = AF_SHOTCHK;
	    osQueueWrite(&pAfCtx->afmCmdQue,&afCmd);
	    err = osQueueTimedRead(&pAfCtx->afmChkAckQue, &ChkAck, 200);
	    if (err == OSLAYER_OK) {
	        *shot = ChkAck.shot;
	    } else {
	        TRACE(AF_ERROR , "%s: osQueueTimedRead time out!!!\n",__FUNCTION__);
	        *shot = BOOL_TRUE;
	    }
    } else {
        *shot = BOOL_TRUE;
	}
    return ( result );
}

/******************************************************************************
 * AfRegisterEvtQue
 *****************************************************************************/

RESULT AfRegisterEvtQue
(
    AfHandle_t                  handle,
    AfEvtQue_t                  *evtQue
)
{
    AfContext_t *pAfCtx = (AfContext_t *)handle;
    RESULT result = RET_SUCCESS;

    TRACE( AF_DEBUG, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAfCtx == NULL )
    {
        TRACE( AF_ERROR, "%s: pAfCtx is NULL",__FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

    if ((pAfCtx->state == AF_STATE_INVALID) && (pAfCtx->state >= AF_STATE_MAX))
    {
        return ( RET_WRONG_STATE );
    }

    osMutexLock(&pAfCtx->EvtQuePool.lock);

    ListAddTail(&pAfCtx->EvtQuePool.QueLst, &evtQue->list);
    
    osMutexUnlock(&pAfCtx->EvtQuePool.lock);

    TRACE( AF_DEBUG, "AfRegisterEvtQue success!");
    return result;

}


/******************************************************************************
 * AfReset()
 *****************************************************************************/
RESULT AfReset
(
    AfHandle_t                handle,
    const AfSearchStrategy_t  fss
)
{
    AfContext_t *pAfCtx = (AfContext_t *)handle;
    AfSeachPos_t  *nPos,*pPos,*curPos;
    int32_t i,maxPos,step;
    List *list;
    RESULT result = RET_SUCCESS;

    TRACE( AF_DEBUG, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAfCtx == NULL )
    {
        TRACE( AF_ERROR, "%s: pAfCtx is NULL",__FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

    if ( (AF_STATE_RUNNING == pAfCtx->state)
            || (AF_STATE_LOCKED == pAfCtx->state) )
    {
        TRACE( AF_ERROR, "%s: pAfCtx->state(%d) is error!\n", __FUNCTION__,pAfCtx->state);
        return ( RET_WRONG_STATE );
    }

    switch ( pAfCtx->Afss )
    {
        case AFM_FSS_FULLRANGE:
            {
                pAfCtx->pAfSearchFunc = AfSearchFullRange;
                break;
            }

        case AFM_FSS_ADAPTIVE_RANGE:
            {
                pAfCtx->pAfSearchFunc = AfSearchAdaptiveRange;
                break;
            }
 
        case AFM_FSS_HILLCLIMBING:
            {
                pAfCtx->pAfSearchFunc = AfSearchHillClimb;
                break;
            }

        default:
            {
                TRACE( AF_ERROR, "%s: invalid focus search function\n", __FUNCTION__);
                return ( RET_INVALID_PARM );
            }

    }

    pAfCtx->MinFocus = 0U;
    result = IsiMdiSetupMotoDrive( pAfCtx->hSensor, &pAfCtx->MaxFocus );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    pAfCtx->vcm_movefull_t = (pAfCtx->MaxFocus&0xffff0000)>>16;
    pAfCtx->MaxFocus &= 0xffff;
    t0 = t1 = 0;
    pAfCtx->afmFrmIntval = 0;

    result = AfSearchInit( (AfHandle_t)pAfCtx, pAfCtx->MinFocus, pAfCtx->MaxFocus );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    result = IsiMdiFocusGet( pAfCtx->hSensor, (uint32_t *)(&pAfCtx->LensePos) );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    if (!ListEmpty(&pAfCtx->AfSearchCtx.Path.nlist)) {
        list = ListGetHead(&pAfCtx->AfSearchCtx.Path.nlist);
        nPos = container_of(list, AfSeachPos_t, nlist);
        osFree(nPos);
        ListInit(&pAfCtx->AfSearchCtx.Path.nlist);
        ListInit(&pAfCtx->AfSearchCtx.Path.plist);
        nPos = NULL;
    }

    nPos = (AfSeachPos_t*)osMalloc(sizeof(AfSeachPos_t)*(ADAPTIVE_STEPS_PER_RANGE+1));
    if (nPos == NULL) {
        TRACE( AF_ERROR, "%s: malloc AfSeachPos_t failed!\n",__FUNCTION__);
        return ( RET_FAILURE );
    }
    
    pPos = nPos + ADAPTIVE_STEPS_PER_RANGE;
    for (i=0; i<=ADAPTIVE_STEPS_PER_RANGE; i++) {
        nPos->index = i;
        nPos->dSharpness = 0;
        nPos->sharpness = 0;
        nPos->pos = pAfCtx->MaxFocus + i*pAfCtx->AfSearchCtx.Step;
        ListAddTail(&pAfCtx->AfSearchCtx.Path.nlist, &nPos->nlist);
        ListAddTail(&pAfCtx->AfSearchCtx.Path.plist, &pPos->plist);
        
        TRACE(AF_DEBUG, "%s: nPos->index: %d  pos: %d \n",__FUNCTION__,
            nPos->index,nPos->pos,pAfCtx->MaxFocus);
        
        nPos++;
        pPos--;
    } 

    pAfCtx->AfSearchCtx.MinSharpness = 0;
    pAfCtx->AfSearchCtx.MaxSharpness = 0;
    pAfCtx->AfSearchCtx.MaxSharpnessPos = pAfCtx->MaxFocus;    
    pAfCtx->AfSearchCtx.Path.maxSharpnessPos = ListGetHead(&pAfCtx->AfSearchCtx.Path.nlist);
    pAfCtx->AfSearchCtx.Path.curPos = ListGetHead(&pAfCtx->AfSearchCtx.Path.nlist);
    pAfCtx->AfSearchCtx.Path.index = 0;
    curPos = container_of(pAfCtx->AfSearchCtx.Path.curPos, AfSeachPos_t, nlist);
    TRACE( AF_DEBUG, "%s: curpos: %d  maxpos: %d  vcm_movefull_t: %d ms \n",__FUNCTION__,
        curPos->pos, pAfCtx->MaxFocus,pAfCtx->vcm_movefull_t);
    
    pAfCtx->state = AF_STATE_STOPPED;

    TRACE( AF_DEBUG, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}