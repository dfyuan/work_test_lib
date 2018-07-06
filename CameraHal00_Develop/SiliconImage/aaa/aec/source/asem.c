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
 * @file sem.c
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

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_exp_drv_api.h>

#include "asem.h"


/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( ASEM_INFO  , "ASEM: ", INFO     , 0 );
CREATE_TRACER( ASEM_WARN  , "ASEM: ", WARNING  , 1 );
CREATE_TRACER( ASEM_ERROR , "ASEM: ", ERROR    , 1 );

CREATE_TRACER( ASEM_DEBUG , "", INFO     , 0 );

#define ABS_DIFF( a, b )     ( (a > b) ? (a-b) : (b-a) )

/******************************************************************************
 * local type definitions
 *****************************************************************************/
static const Cam5x5FloatMatrix_t distances =
{
    {
        2.828427f, 2.236068f, 2.000000f, 2.236068f, 2.828427f,
        2.236068f, 1.414214f, 1.000000f, 1.414214f, 2.236068f,
        2.000000f, 1.000000f, 0.000000f, 1.000000f, 2.000000f,
        2.236068f, 1.414214f, 1.000000f, 1.414214f, 2.236068f,
        2.828427f, 2.236068f, 2.000000f, 2.236068f, 2.828427f
    }
};



static const Cam5x5FloatMatrix_t corBlocksX =
{
    {
        0.5f, 1.5f, 2.5f, 3.5f, 4.5f,
        0.5f, 1.5f, 2.5f, 3.5f, 4.5f,
        0.5f, 1.5f, 2.5f, 3.5f, 4.5f,
        0.5f, 1.5f, 2.5f, 3.5f, 4.5f,
        0.5f, 1.5f, 2.5f, 3.5f, 4.5f
    }
};



static const Cam5x5FloatMatrix_t corBlocksY =
{
    {
        0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
        1.5f, 1.5f, 1.5f, 1.5f, 1.5f,
        2.5f, 2.5f, 2.5f, 2.5f, 2.5f,
        3.5f, 3.5f, 3.5f, 3.5f, 3.5f,
        4.5f, 4.5f, 4.5f, 4.5f, 4.5f,
    }
};



/******************************************************************************
 * local variable declarations
 *****************************************************************************/
static const float c1  =  10.0f;
static const float c2  = 100.0f;



/******************************************************************************
 * local function prototypes
 *****************************************************************************/

/******************************************************************************
 * SemCalcPx()
 *
 * function is used to compute the q values in the equation
 *****************************************************************************/
static uint32_t SemCalcPx
(
    const uint32_t start, 
    const uint32_t end,
    const uint8_t  *lhist
)
{
    uint32_t sum = 0UL;
    uint32_t i;

    for ( i = start; i <= end; i++ )
    {
        sum += (uint32_t)lhist[i];
    }

    return ( sum );
}


/******************************************************************************
 * SemCalcMx()
 *
 * function is used to compute the mean values in the equation (mu)
 *****************************************************************************/
static uint32_t SemCalcMx
(
    const uint32_t start, 
    const uint32_t end,
    const uint8_t  *lhist
)
{
    uint32_t sum = 0UL;
    uint32_t i;

    for ( i = start; i <= end; i++ )
    {
        sum += ( i * (uint32_t)lhist[i] );
    }

    return ( sum );
}



/******************************************************************************
 * SemCalcFindMax()
 *
 * finds the maximum element in a vector
 *****************************************************************************/
static uint32_t SemCalcFindMax
(
    const float     *vec,
    const uint32_t  n
)
{
    float max = 0.0f;

    int idx=0;
    uint32_t i;

    for ( i = 1; i < (n - 1); i++ )
    {
        if ( vec[i] > max )
        {
            max = vec[i];
            idx = i;
        }
    }

    return ( idx );
}



/******************************************************************************
 * SemCalcLumaHistogram()
 *****************************************************************************/
static RESULT SemCalcLumaHistogram
(
    const uint8_t *luma,        /**< measured luma matrix (5x5) */
    const uint8_t row,          /**< rows of the luma matrix */
    const uint8_t col,          /**< colums of the luma matrix */
    uint8_t       *lhist
)
{
    uint32_t i, j;

    if ( (luma == NULL) || (lhist == NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    for ( i = 0U; i < col; i++ )
    {
        for ( j = 0U; j < row; j++ )
        {
            uint32_t idx=i*row + j;
            ++lhist[ luma[idx] ];
        }
    }

    return ( RET_SUCCESS );
}



/******************************************************************************
 * SemCalcOtsuThreshold()
 *****************************************************************************/
static RESULT SemCalcOtsuThreshold
(
    const CamerIcMeanLuma_t luma,           /**< measured luma matrix (5x5) */
    const uint8_t           row,            /**< rows of the luma matrix */
    const uint8_t           col,            /**< colums of the luma matrix */
    float                   *pThreshold     /**< returned otsu-threshold */
)
{
    RESULT result = RET_SUCCESS ;

    uint8_t lhist[256];         /**< luma histogram */
    float   vec[256];
    uint32_t k = 0U;

    uint32_t p1;
    uint32_t p2;
    uint32_t p12;
    uint32_t diff;

    TRACE( ASEM_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (luma == NULL) || (pThreshold == NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    MEMSET( lhist, 0, sizeof(lhist) );
    MEMSET( vec, 0, sizeof(vec) );

    result = SemCalcLumaHistogram( luma, row, col, lhist );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    for ( k=1U; k < 255U; k++ )
    {
        p1 = SemCalcPx( 0, k, lhist );
        p2 = SemCalcPx( (k + 1), 255U, lhist );
        p12 = ( (p1 * p2) == 0U )  ? 1U : (p1 * p2);
        diff = ABS_DIFF( (SemCalcMx( 0, k, lhist ) * p2), (SemCalcMx( (k + 1), 255, lhist) * p1) );
        vec[k] = ( diff == 0U ) ? 0.0f : ( ((float)diff / (float)p12) * (float)diff );
    }

    *pThreshold = (float)SemCalcFindMax( vec, (sizeof(vec) / sizeof(vec[0])) );

    TRACE( ASEM_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * SemCalcThresholding()
 *****************************************************************************/
static RESULT SemCalcThresholding
(
    const CamerIcMeanLuma_t luma,       /**< measured luma matrix (5x5) */
    const uint8_t           row,        /**< rows of the luma matrix */
    const uint8_t           col,        /**< colums of the luma matrix */
    const float             threshold,  /**< otsu-threshold */
    CamerIcMeanLuma_t       bin         /**< binary luma matrix (5x5) */
)
{
    RESULT result = RET_SUCCESS;

    uint32_t i, j;

    if ( (luma == NULL) || (bin == NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    for ( i = 0U; i < col; i++ )
    {
        for ( j = 0U; j < row; j++ )
        {
            uint32_t idx=i*row + j;
            bin[idx] = ( luma[idx] > threshold ) ? 1U : 0U;
        }
    }

    return ( result );
}



/******************************************************************************
 * SemCalcNegateMatrix()
 *****************************************************************************/
static RESULT SemCalcNegateMatrix
(
    const CamerIcMeanLuma_t in,             /**< input matrix (5x5) */
    const uint8_t           row,            /**< rows of the luma matrix */
    const uint8_t           col,            /**< colums of the luma matrix */
    CamerIcMeanLuma_t       out
)
{
    uint32_t k = 0U;

    TRACE( ASEM_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (in == NULL) || (out == NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    for ( k=0U; k < (row*col); k++ )
    {
        out[k] = ( in[k] == 1U ) ? 0U : 1U;
    }

    TRACE( ASEM_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * SemGetBinValue()
 *****************************************************************************/
static uint8_t SemGetBinValue
(
    const CamerIcMeanLuma_t cc,        /**< binary matrix (5x5) */
    const uint8_t           col,        /**< rows of the binary matrix */
    const uint8_t           row,        /**< colums of the binary matrix */
    const int8_t            x,
    const int8_t            y
)
{
    if ( (x >= 0) && (y >= 0) && (x<col) && (y<row) )
    {
        return ( cc[col*y+x] );
    }
    else
    {
        return ( 0 );
    }
}



/******************************************************************************
 * SemCCLFirstPass()
 *****************************************************************************/
static RESULT SemCCLFirstPass
(
    CamerIcMeanLuma_t   cc,             /**< binary luma matrix (5x5) */
    const uint8_t       row,            /**< rows of the luma matrix */
    const uint8_t       col,            /**< colums of the luma matrix */
    int32_t             *pActIndex      /**< index */
)
{
    uint8_t a;  /* north west */
    uint8_t b;  /* north */
    uint8_t c;  /* north east */
    uint8_t d;  /* west */

    int8_t si;

    int8_t i;
    int8_t j;

    if ( (cc == NULL) || (pActIndex==NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    *pActIndex = 1;

    for ( i=0; i<row; i++ )
    {
        si = row * i;

        for ( j=0; j<col; j++ )
        {

            if ( cc[si+j] )
            {
                a=SemGetBinValue( cc, col, row, j-1, i-1 );
                b=SemGetBinValue( cc, col, row, j  , i-1 );
                c=SemGetBinValue( cc, col, row, j+1, i-1 );
                d=SemGetBinValue( cc, col, row, j-1, i   );

                if ( d )
                {
                    cc[si+j]=d;
                }
                else
                {
                    if ( b )
                    {
                        cc[si+j]=b;
                    }
                    else
                    {
                        if ( a )
                        {
                            cc[si+j]=a;
                        }
                        else
                        {
                            if ( c )
                            {
                                cc[si+j]=c;
                            }
                            else
                            {
                                cc[si+j] = *pActIndex;
                                (*pActIndex)++;
                            }
                        }
                    }
                }
            }
        }
    }

    return ( RET_SUCCESS );
}



/******************************************************************************
 * SemGetBinValue()
 *****************************************************************************/
static RESULT SemCCLCheckCollision
(
    const CamerIcMeanLuma_t cc,         /**< binary luma matrix (5x5) */
    const uint8_t           row,        /**< rows of the luma matrix */
    const uint8_t           col,        /**< colums of the luma matrix */
    const int32_t           ActIndex,
    uint8_t                 *matrix
)
{
    uint8_t a;
    uint8_t b;
    uint8_t c;

    int8_t si;

    int i, j;

    for ( i=1; i<row; i++ )
    {
        si=i*j;

        for ( j=1; j<col; j++ )
        {

            if ( cc[si+j] )
            {

                a = SemGetBinValue( cc, col, row, j-1, i-1);
                b = SemGetBinValue( cc, col, row, j,   i-1);
                c = SemGetBinValue( cc, col, row, j+1, i-1);

                if ( (a) && (a != cc[si+j]) )
                { 
                    matrix[((ActIndex)*(cc[si+j]))+(a+1)] = 1;
                    matrix[((ActIndex)*a)+(cc[si+j]+1)]   = 1;
                }

                if ( (b) && (b != cc[si+j]) )
                {
                    matrix[((ActIndex)*(cc[si+j]))+(b+1)] = 1;
                    matrix[((ActIndex)*b)+(cc[si+j]+1)]   = 1;
                }

                if ( (c) && (c != cc[si+j]) )
                {
                    matrix[((ActIndex)*(cc[si+j]))+(c+1)] = 1;
                    matrix[((ActIndex)*c)+(cc[si+j]+1)]   = 1;
                }
            }
        }
    }

    return ( RET_SUCCESS );
}



/******************************************************************************
 * SemCCLChangeMatrix()
 *****************************************************************************/
static RESULT SemCCLChangeMatrix
(
    const int32_t   ActIndex,
    uint8_t         *matrix
)
{
    /* pomocne promenna aby se tolik nenasobilo */ 

    int8_t ActIndexI;
    int8_t ActIndexJ;

    int8_t i;
    int8_t j;
    int8_t k;

    for ( i=1; i<ActIndex; i++ )
    {
        ActIndexI = ActIndex * i;
        matrix[ActIndexI + i+1] = 1;
    }

    for ( j=1; j<ActIndex; j++)
    {
        ActIndexJ = ActIndex * j;

        for ( i=1; i<ActIndex; i++ )
        {
            ActIndexI = ActIndex * i;

            if ( matrix[ActIndexI+j+1] )
            {
                for ( k=1; k<ActIndex; k++ )
                {
                    if ( (matrix[ActIndexI+k+1]) || (matrix[ActIndexJ+k+1]) )
                    {
                        matrix[ActIndexI+k+1]=1; 
                    }
                }
            }
        }
    }

    return ( RET_SUCCESS );
}



/******************************************************************************
 * SemCCLChangeMatrix()
 *****************************************************************************/
static void SemCCLClearColumn
(
    const int32_t   ActIndex,
    const int       col, 
    uint8_t         *matrix
)
{
    int8_t n;

    for ( n=1; n<ActIndex; n++)
    {
        matrix[ (ActIndex*n) + (col+1) ] = 0;
    }
}



/******************************************************************************
 * SemCCLMakeReindex()
 *****************************************************************************/
static RESULT SemCCLMakeReindex
(
    const int32_t   ActIndex,
    uint8_t         *matrix,
    uint8_t         *reindex,
    int32_t         *pActColor
)
{
    int8_t pom=0;

    int8_t ActIndexI;

    int8_t i;
    int8_t j;

    *pActColor = 1;

    for ( i=1; i<ActIndex; i++ )
    {
        ActIndexI = ActIndex * i;

        for ( j=1; j<ActIndex; j++ )
        {
            if ( matrix[ActIndexI + j+1]==1 )
            {
                reindex[j]=*pActColor;
                pom++;
                SemCCLClearColumn( ActIndex, j, matrix);
            }
        }

        if ( pom )
        {
            (*pActColor)++;
            pom=0;
        }
    }

    return ( RET_SUCCESS );
}



/******************************************************************************
 * SemCCLSecondPass()
 *****************************************************************************/
static RESULT SemCCLSecondPass
(
    CamerIcMeanLuma_t   data,       /**< binary luma matrix (5x5) */
    const uint8_t       row,        /**< rows of the luma matrix */
    const uint8_t       col,        /**< colums of the luma matrix */
    uint8_t             *reindex
)
{
    int8_t si;
    int8_t i;
    int8_t j;
	
    for ( i=0; i<col; i++ )
    {  
        si=col*i;

        for (j=0; j<row ; j++ )
        {
            if ( data[si+j] )
            {
                data[si+j]=reindex[data[si+j]];
            }
        }
    }

    return ( RET_SUCCESS );
}
                


/******************************************************************************
 * SemCalcCCL() - Check Connected Component
 *****************************************************************************/
static RESULT SemCalcCCL
(
    const CamerIcMeanLuma_t bin,            /**< binary matrix (5x5) */
    const uint8_t           col,            /**< colums of the luma matrix */
    const uint8_t           row,            /**< rows of the luma matrix */
    CamerIcMeanLuma_t       cc,             /**< connected compontent matrix (5x5) */
    uint32_t                *pNoComponents  /**< number of connected components */
)
{
    RESULT result = RET_SUCCESS;

    uint8_t matrix[100];
    uint8_t reindex[10];

    int32_t actIndex;
    int32_t actColor;

    if ( (bin == NULL) || (cc == NULL) || (pNoComponents == NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    MEMSET( matrix, 0, sizeof( matrix ) );
    MEMSET( reindex, 0, sizeof( reindex ) );

    MEMCPY( cc, bin, sizeof( CamerIcMeanLuma_t ) );

    result = SemCCLFirstPass( cc, row, col, &actIndex );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    result = SemCCLCheckCollision( cc, row, col, actIndex, matrix ); 
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    result = SemCCLChangeMatrix( actIndex, matrix );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    result = SemCCLMakeReindex( actIndex, matrix, reindex, &actColor );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    result = SemCCLSecondPass( cc, row, col, reindex );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    *pNoComponents = actColor - 1;

    return ( result );
}



/******************************************************************************
 * SemCalcSizeOfComponent() - calculate the size of a component
 *****************************************************************************/
static uint32_t SemCalcSizeOfComponent
(
    const CamerIcMeanLuma_t cc,             /**< connected compontent matrix (5x5) */
    const uint8_t           col,            /**< colums of the luma matrix */
    const uint8_t           row,            /**< rows of the luma matrix */
    uint32_t                label           /**< number of connected components */
)
{
    uint32_t result = 0U;
    int32_t i;

    for ( i=0; i<(col*row); i++ )
    {
        if ( cc[i] == label )
        {
            result ++;
        }
    }

    return ( result );
}



/******************************************************************************
 * SemCalcCenterOfComponent() - calculate the center of a component
 *****************************************************************************/
static float SemCalcCenterOfComponent
(
    const CamerIcMeanLuma_t     cc,             /**< connected compontent matrix (5x5) */
    const Cam5x5FloatMatrix_t   meshgrid,
    const uint8_t               col,            /**< colums of the luma matrix */
    const uint8_t               row,            /**< rows of the luma matrix */
    const uint32_t              label           /**< number of connected components */
)
{
    float sum = 0.0f;
    int32_t i;

    for ( i=0; i<(col*row); i++ )
    {
        if ( cc[i] == label )
        {
            sum += meshgrid.fCoeff[i];
        }
    }

    return ( sum );
}



/******************************************************************************
 * SemCalcMaxCoG() - calculate the maximum Center of Gravity
 *****************************************************************************/
static RESULT SemCalcMaxCoG
(
    const CamerIcMeanLuma_t cc,             /**< connected compontent matrix (5x5) */
    const uint8_t           col,            /**< colums of the luma matrix */
    const uint8_t           row,            /**< rows of the luma matrix */
    uint32_t                NoComponents,   /**< number of connected components */

    float                   *pMaxCenterX,
    float                   *pMaxCenterY,
    CamerIcMeanLuma_t       pLL             /**< connected compontent matrix (5x5) */
)
{
    RESULT result = RET_SUCCESS;

    uint32_t  i;
    uint32_t  j;

    uint32_t size;
    uint32_t max;

    float sumX;
    float sumY;

    float centerX;
    float centerY;

    float maxsumX;
    float maxsumY;

    float maxcenterX;
    float maxcenterY;

    CamerIcMeanLuma_t LL;
    CamerIcMeanLuma_t LLold;

    size = 0U;
    max = 0U;

    sumX = 0.0f;
    sumY = 0.0f;

    centerX = 0.0f;
    centerY = 0.0f;

    maxsumX = 0.0f;
    maxsumY = 0.0f;

    maxcenterX = 0.0f;
    maxcenterY = 0.0f;

    MEMSET( LL, 0, sizeof( CamerIcMeanLuma_t ) );
    MEMSET( LLold, 0, sizeof( CamerIcMeanLuma_t ) );

    for ( i=1U; i<(NoComponents+1); i++ )
    {
        size = SemCalcSizeOfComponent( cc, col, row, i );
        if ( size >= max )
        {
            /* generate a matrix which marks the biggest area */
            for ( j = 0; j<(row*col); j++ )
            {
                if ( cc[j] == i )
                {
                    LL[j] = 1;
                }
            }

            /* calculate the center of gravity of this region */

            /* store the sumX and sumY to easily combine if necessary */
            sumX = SemCalcCenterOfComponent( cc, corBlocksX, col, row, i );
            sumY = SemCalcCenterOfComponent( cc, corBlocksX, col, row, i );

            centerX = sumX/(float)size;
            centerY = sumY/(float)size;

            /* choose new connected region has more blocks or
             * if two objects have the same number of blocks 
             * choose the objects which has lower center of gravity */
            if ( (size > max) && ( (centerY) > maxcenterY) )
            {
                max         = size;
                maxsumX     = sumX;
                maxsumY     = sumY;
                maxcenterX  = centerX;
                maxcenterY  = centerY;
            }
            else
            {
                /* two blocks have the same number of blocks and the same y-axis
                 * of center of gravity, combine these two objects */
                for ( j = 0; j<(row*col); j++ )
                {
                    if ( (LL[j] == 1) || (LLold[j] == 1) )
                    {
                        LL[j] = 1;
                    }
                }

                max *= 2;
                maxsumX += sumX;
                maxsumY += sumY;

                maxcenterX = maxsumX / (float)max;
                maxcenterY = maxsumY / (float)max;
            }

            MEMCPY( LLold, LL, sizeof( CamerIcMeanLuma_t ) );
        }
    }

    *pMaxCenterX = maxcenterX;
    *pMaxCenterY = maxcenterY;

    MEMCPY( pLL, LL, sizeof( CamerIcMeanLuma_t ) );

    TRACE( ASEM_DEBUG, "CoG (%f, %f) \n", maxcenterX, maxcenterY );

    return ( result );
}



/******************************************************************************
 * SemCalcLuminaceDifference()
 *****************************************************************************/
static RESULT SemCalcLuminaceMeanLuminaceAndDifference
(
    AecContext_t        *pAecCtx,
    CamerIcMeanLuma_t   luma,
    CamerIcMeanLuma_t   ObjectRegion
)
{
    uint32_t i;

    uint32_t cntObj;    /* block counter object region */
    uint32_t cntBg;     /* block counter background region */

    uint32_t sumObj;    /* sum object region */
    uint32_t sumBg;     /* sum background region */

    TRACE( ASEM_INFO, "%s: (enter)\n", __FUNCTION__ );

    sumObj = 0U;
    cntObj = 0U;

    sumBg = 0U;
    cntBg = 0U;

    for ( i=0U; i<CAMERIC_ISP_EXP_GRID_ITEMS; i++ )
    {
        if ( !ObjectRegion[i] )
        {
            sumBg += (float)((uint32_t)luma[i]);
            ++cntBg;
        }
        else
        {
            sumObj += (float)((uint32_t)luma[i]);
            ++cntObj;
        }
    }

    if ( cntBg > 0U )
    {
        sumBg /= (float)cntBg;
    }

    if ( cntObj > 0U )
    {
        sumObj /= (float)cntObj;
    }

    pAecCtx->MeanLumaObject = sumObj;

    pAecCtx->d = (sumBg > sumObj) ? (sumBg - sumObj) : (sumObj - sumBg);

    TRACE( ASEM_DEBUG, "mean = %f, mean_object = %f\n", pAecCtx->MeanLuma, pAecCtx->MeanLumaObject );

    TRACE( ASEM_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * SemCalcDegreeOfBackLight()
 *****************************************************************************/
static RESULT SemCalcDegreeOfBackLight
(
    AecContext_t *pAecCtx
)
{
    TRACE( ASEM_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( pAecCtx->d < c1 )
    {
        pAecCtx->z = 0.0f;
    }
    else
    {
        if ( pAecCtx->d > c2 )
        {
            pAecCtx->z = 1.0f;
        }
        else
        {
            pAecCtx->z = ( pAecCtx->d - c1 ) / ( c2 - c1 );
        }
    }

    TRACE( ASEM_DEBUG, "z = %f\n", pAecCtx->z );

    TRACE( ASEM_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * SemCalcTargetLuminace()
 *****************************************************************************/
static RESULT SemCalcTargetLuminace
(
    AecContext_t        *pAecCtx,
    CamerIcMeanLuma_t   luma
)
{
    uint32_t i;

    float value;
    float mean = 0.0f;

    TRACE( ASEM_INFO, "%s: (enter)\n", __FUNCTION__ );

    for ( i=0U; i<CAMERIC_ISP_EXP_GRID_ITEMS; i++ )
    {
        value = ( ((float)luma[i]) * pAecCtx->SetPoint ) / pAecCtx->MeanLumaObject;
        mean += ( value > 255.0f ) ? 255.0f : value;
    }

    mean /= (float)CAMERIC_ISP_EXP_GRID_ITEMS;

    pAecCtx->m0 = ( ( 1.0f - pAecCtx->z ) * pAecCtx->SetPoint ) + ( pAecCtx->z * mean );

    TRACE( ASEM_DEBUG, "m0 = %f\n", pAecCtx->m0 );

    TRACE( ASEM_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * See header file for detailed comment.
 *****************************************************************************/
RESULT AdaptSemExecute
(
    AecContext_t        *pAecCtx,
    CamerIcMeanLuma_t   luma
)
{
    RESULT result;

    TRACE( ASEM_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( luma == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    memset( pAecCtx->SemCtx.asem.Binary0, 0, sizeof( CamerIcMeanLuma_t ) );
    memset( pAecCtx->SemCtx.asem.Binary1, 0, sizeof( CamerIcMeanLuma_t ) );
    memset( pAecCtx->SemCtx.asem.Cc0    , 0, sizeof( CamerIcMeanLuma_t ) );
    memset( pAecCtx->SemCtx.asem.Cc1    , 0, sizeof( CamerIcMeanLuma_t ) );

    /* 1.) calc otsu threshold */
    result = SemCalcOtsuThreshold( luma, 5, 5, &pAecCtx->SemCtx.asem.OtsuThreshold );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    /* 2.) calc binary matrix with otsu threshold */
    result = SemCalcThresholding( luma, 5, 5, pAecCtx->SemCtx.asem.OtsuThreshold, pAecCtx->SemCtx.asem.Binary1 );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    /* 3.) negate the binary matrix */
    result = SemCalcNegateMatrix( pAecCtx->SemCtx.asem.Binary1, 5, 5, pAecCtx->SemCtx.asem.Binary0 );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    /* 4.) calculate connected 1-compontens */
    result = SemCalcCCL( pAecCtx->SemCtx.asem.Binary1, 5, 5, pAecCtx->SemCtx.asem.Cc1, &pAecCtx->SemCtx.asem.NumCc1 );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    /* 4.1.) calculate max center of gravity */
    result = SemCalcMaxCoG( pAecCtx->SemCtx.asem.Cc1, 5, 5, pAecCtx->SemCtx.asem.NumCc1,
            &pAecCtx->SemCtx.asem.MaxCenterX1, &pAecCtx->SemCtx.asem.MaxCenterY1, pAecCtx->SemCtx.asem.Ll1 );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    /* 5.) calculate connected 0-compontens */
    result = SemCalcCCL( pAecCtx->SemCtx.asem.Binary0, 5, 5, pAecCtx->SemCtx.asem.Cc0, &pAecCtx->SemCtx.asem.NumCc0 );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    /* 5.1.) calculate max center of gravity */
    result = SemCalcMaxCoG( pAecCtx->SemCtx.asem.Cc0, 5, 5, pAecCtx->SemCtx.asem.NumCc0,
            &pAecCtx->SemCtx.asem.MaxCenterX0, &pAecCtx->SemCtx.asem.MaxCenterY0, pAecCtx->SemCtx.asem.Ll0 );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    /* 6.) check distances */
    if ( pAecCtx->SemCtx.asem.MaxCenterY0 > pAecCtx->SemCtx.asem.MaxCenterY1 )
    {
        MEMCPY( pAecCtx->SemCtx.asem.ObjectRegion, pAecCtx->SemCtx.asem.Ll0, sizeof( CamerIcMeanLuma_t ) );
    }
    else if (  pAecCtx->SemCtx.asem.MaxCenterY0 < pAecCtx->SemCtx.asem.MaxCenterY1 )
    {
        MEMCPY( pAecCtx->SemCtx.asem.ObjectRegion, pAecCtx->SemCtx.asem.Ll1, sizeof( CamerIcMeanLuma_t ) );
    }
    else
    {
        /* check distances */
    }

    /* 7.) calculate mean luminace for current frame */
    result = SemCalcLuminaceMeanLuminaceAndDifference( pAecCtx, luma, pAecCtx->SemCtx.asem.ObjectRegion );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    /* 8.) calculate degree of back/front-light */
    result = SemCalcDegreeOfBackLight( pAecCtx );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    /* 9.) calculate m0 (object luminace to hit) */
    result = SemCalcTargetLuminace( pAecCtx, luma );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    pAecCtx->SemSetPoint = pAecCtx->m0;

    TRACE( ASEM_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}

