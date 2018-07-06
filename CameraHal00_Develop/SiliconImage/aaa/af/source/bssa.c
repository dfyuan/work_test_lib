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

#include "bssa.h"



/******************************************************************************
 * local macro definitions
 *****************************************************************************/
USE_TRACER( AF_INFO  );
USE_TRACER( AF_WARN  );
USE_TRACER( AF_ERROR );

USE_TRACER( AF_DEBUG );



/*****************************************************************************/
/*!
 * \name parameters of adaptive range search
 */
/*****************************************************************************/
//@{
/*! approx. number of steps to use in one adaptive scan range. */
#define ADAPTIVE_STEPS_PER_RANGE 8
//@}



/*****************************************************************************/
/*!
 *  \FUNCTION    MrvSls_AfReinitAdaptive \n
 *  \RETURNVALUE none \n
 *  \PARAMETERS  ptBSSA .. pointer to basic step search algorithm context
 *                 to be re-initialized \n
 *               iStart, iStop .. start and stop lens position of
 *                 the new range search \n
 *  \DESCRIPTION This routines clips the given start/stop values to the maximum
 *  possible range, initializes the basic step search algorithm and
 *  chooses a step site that would give approx. ADAPTIVE_STEPS_PER_RANGE
 *  steps to cover the range \n
 */
/*****************************************************************************/
void AfReinitBssa
(
    BssaCtx_t       *pBssaCtx,
    const int32_t   Start,
    const int32_t   Stop
)
{
    TRACE( AF_INFO, "%s: (enter)\n", __FUNCTION__);

    pBssaCtx->LensStart = Start;
    pBssaCtx->LensStop  = Stop;

    pBssaCtx->Step      = (Start > Stop) ? (Start - Stop) : (Stop -Start);
    pBssaCtx->Step      /= ADAPTIVE_STEPS_PER_RANGE;
    pBssaCtx->Step      = ( !pBssaCtx->Step ) ?  1L : pBssaCtx->Step;

    pBssaCtx->State     = STATE_BSSA_START;

    pBssaCtx->SharpnessMax  = 0UL;
    pBssaCtx->SharpnessMin  = 0xFFFFFFFFUL;
 
    TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__);
}




/*****************************************************************************/
/*!
 *  \FUNCTION    MrvSls_AfSearchBSSA \n
 *  \RETURNVALUE State of search: \n
 *                 RET_BUSY: Busy searching, \n
 *                 RET_SUCCESS: Search interval is completely processed.
 *                   Results can be found in the context structure.  \n
 *                   The state is set to STATE_BSSA_DONE. The lens
 *                   position does not have been changed.  \n
 *  \PARAMETERS  ptMrvAfMeas .. current AF measurement values \n
 *               pulLensPos .. pointer to a variable that contains the 
 *                 lens position at which the measurements were taken,
 *                 MRV_AF_LENSPOS_UNKNOWN for unknown. The routine can
 *                 change this value if another lens position should 
 *                 be applied for the next measurement.   \n
 *               pCtx .. context for basic full search algoritm \n
 *  \DESCRIPTION Basic step search algoritm (BSSA). This routine performs a stepped 
 *  search on a given interval of lens positions with a configuable
 *  step size. It must be called subsequently for every processed frame. \n
 *  \n
 *  To start the search, the following context members must be initialized: \n
 *    ulLensStart - lens position to start from  \n
 *    ulLensStop - lens position to stop the search \n
 *    iStep - absolute step size, always positive. The direction of the
 *            search is given with the relation between ulLensStart and
 *            ulLensStop \n
 *    iState - must be set to STATE_BSSA_START to indicate the start
 *             of the search. It is changed later on to reflect internal
 *             states of the algorithm. \n
 *  \n
 *  If finished, the context members ulLensPosBest, ulSharpnessMax
 *  and ulSharpnessMin contain the results of the search. \n
 */
/*****************************************************************************/
RESULT AfSearchBssa
(
    BssaCtx_t       *pBssaCtx,
    const uint32_t  Sharpness,
    int32_t         *pLensPos
)
{
    TRACE( AF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* first time AF search */
    if ( pBssaCtx->State == STATE_BSSA_START )
    { 
        /* we are just at the beginning of a search sequence */
        if ( *pLensPos != pBssaCtx->LensStart )
        {
            /* move to start of interval first and come back later */
            *pLensPos = pBssaCtx->LensStart;
        }
        else
        {
            /* initialize and start search */
            pBssaCtx->SharpnessMax     = Sharpness;
            pBssaCtx->SharpnessMin     = Sharpness;
            pBssaCtx->LensPosBest      = *pLensPos;
            pBssaCtx->LensPosWorst     = *pLensPos;
            pBssaCtx->State            = STATE_BSSA_SEARCH;
        }
   }

   /* normal AF search, we are already busy seraching for longer */
   else if ( pBssaCtx->State == STATE_BSSA_SEARCH )
   {
       if ( Sharpness > pBssaCtx->SharpnessMax )
       {  
           /* found a new maximum */
           pBssaCtx->SharpnessMax = Sharpness;
           pBssaCtx->LensPosBest  = *pLensPos;
       }

       if ( Sharpness < pBssaCtx->SharpnessMin )
       {  
           /* found a new minimum */
           pBssaCtx->SharpnessMin = Sharpness;
           pBssaCtx->LensPosWorst = *pLensPos;
       }
   }

   /*****************************************************************************/
   // calculation of the next focus point to evaluate and 
   // and detection of search-end-situation
   /*****************************************************************************/
   if ( pBssaCtx->State == STATE_BSSA_SEARCH )
   {
       if ( *pLensPos == pBssaCtx->LensStop )
       {
           /* ok, we're done */
           pBssaCtx->State = STATE_BSSA_DONE;
           return ( RET_SUCCESS );
       }
       else
       {
           /* move to the next focus point */
           if ( pBssaCtx->LensStart < pBssaCtx->LensStop )
           {
               /* ascending lens positions */
               (*pLensPos) += pBssaCtx->Step;
               if ( *pLensPos> pBssaCtx->LensStop )
               {
                   *pLensPos= pBssaCtx->LensStop;
               }
           }
           else
           {
               /* descending lens positions */
               if ( (pBssaCtx->LensStop + pBssaCtx->Step) > *pLensPos )
               {
                   *pLensPos = pBssaCtx->LensStop;
               }
               else
               {
                   (*pLensPos) -= pBssaCtx->Step;
               }
           }
       }
   }

   TRACE( AF_INFO, "%s: (exit)\n", __FUNCTION__);

   return ( RET_BUSY );
}



/*****************************************************************************/
/*!
 *  \FUNCTION    MrvSls_AfIsBestAtRangeBorder \n
 *  \RETURNVALUE Three possible return values: \n
 *                 0:  The point of best focus is not at the border of
 *                     the range to be searched, or it is there but it
 *                     is also the limitation of the lens system, so 
 *                     there is no need to redo searching. \n
 *                 ptBSSA->iStep: The point of best focus is at the 
 *                     border of the range that has been searched. The
 *                     true maximum is expected at lens positions greater
 *                     than the one detected as being the best in the
 *                     last search. \n
 *                 -(ptBSSA->iStep): The point of best focus is at the 
 *                     border of the range that has been searched. The
 *                     true maximum is expected at lens positions smaller
 *                     than the one detected as being the best in the
 *                     last search. \n
 *  \PARAMETERS  ptBSSA .. pointer to basic step search algorithm context
 *                 to be used \n
 *  \DESCRIPTION A little helper to get information of wether the point of best focus
 *  is located at the border of the search range and may therefore not the 
 *  absolute maximum of the mesasurement. \n
 */
/*****************************************************************************/
int32_t AfIsBestAtRangeBorder
(
    BssaCtx_t   *pBssaCtx
)
{
   if ( (pBssaCtx->LensPosBest == pBssaCtx->LensStart)
           || (pBssaCtx->LensPosBest == pBssaCtx->LensStop)
           || (pBssaCtx->Step == 1) )
   {
       if ( pBssaCtx->LensPosBest > pBssaCtx->LensPosWorst )
       {  
           // we expect the absolute maximum at lens positons greater than ptBSSA->ulLensPosBest
           return ( pBssaCtx->Step );
       }
       else
       {
           // we expect the absolute maximum at lens positons smaller than ptBSSA->ulLensPosBest
           return ( -(pBssaCtx->Step) );
       }
   }

   return ( 0 );
}


