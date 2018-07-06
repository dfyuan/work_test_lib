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
 * @file cameric_isp_afm.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>

#include "mrv_all_bits.h"

#include "cameric_drv_cb.h"
#include "cameric_drv.h"


/******************************************************************************
 * Is the hardware AF measuring module available ?
 *****************************************************************************/
#if defined(MRV_AUTOFOCUS_VERSION)

/******************************************************************************
 * Auto Focus Module is available.
 *****************************************************************************/

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_ISP_AFM_DRV_INFO  , "CAMERIC-ISP-AFM-DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_ISP_AFM_DRV_WARN  , "CAMERIC-ISP-AFM-DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_ISP_AFM_DRV_ERROR , "CAMERIC-ISP-AFM-DRV: ", ERROR   , 1 );


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
 * @brief   This function calculates the tenengrad shift parameter.
 *
 * @note    CALCULATION OF THE TENENGRAD SHIFT PARAMETER
 *          Experiemts have shown that the tenengrad algorithm (which is used
 *          by the af measurement hardware) is most sensitive to a black/white
 *          chessboard pattern with 2x2 pixel field size. With this input pattern,
 *          Tenengrad produces the sum of 4.227.989.504 for a window size of 
 *          128*128 = 16384 pixel. This is slightly below the maximum value 
 *          to store in a register with 32 bits width. Thus, we will increase
 *          the shift as long as the pixel count of the biggest measurement 
 *          window is above 16384.
 *
 *****************************************************************************/
static inline uint32_t CamerIcIspAfmCalcTenengradShift
(
    const uint32_t MaxPixelCnt
)
{
    uint32_t tgrad  = MaxPixelCnt;
    uint32_t tshift = 0U;

    while ( tgrad > (128*128) )
    {
        ++tshift;
        tgrad >>= 1;
    }

    return ( tshift );
}


/*****************************************************************************/
/**
 * @brief   This function calculates the luminance shift parameter.
 *
 * @note    CALCULATION OF THE LUMINANCE SHIFT PARAMETER
 *          The measurement routine will simply add all of the (8 bit) luminance
 *          samples to produce a 24 bit luminance sum. In worstcase-szenario,
 *          all summands can be (2^8)-1 = 255, so the sum will overflow at 
 *          (2^24) / 255 = 65793 pixels.
 *          Thus, we will increase the shift as long as the pixel count of the
 *          biggest measurement window is above 65793.
 *
 *****************************************************************************/
static inline uint32_t CamerIcIspAfmCalcLuminanceShift
(
    const uint32_t MaxPixelCnt
)
{
    uint32_t lgrad  = MaxPixelCnt;
    uint32_t lshift = 0U;

    while ( lgrad > 65793)
    {
        ++lshift;
        lgrad >>= 1;
    }

    return ( lshift );
}


/*****************************************************************************/
/**
 * @brief   This function calculates left-top and right-bottom register 
 *          values for a given Auto focus measurement window
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Register values successfully computed.
 * @retval  RET_OUTOFRANGE      Window size is out of range.
 *
 *****************************************************************************/
static RESULT CamerIcIspAfmWnd2Regs
(
    const CamerIcWindow_t   *pWindow,   /**< window to convert into register values */
    uint32_t                *pLT,       /**< pointer to variable to receive value for left-top register */
    uint32_t                *pRB        /**< pointer to variable to receive value for right-bottom register */
)
{
    if ( (pWindow == NULL) || (pLT == NULL) || (pRB == NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    if ( (pWindow->width != 0U) && (pWindow->height !=0 ) )
    {
        uint32_t left   = (uint32_t)pWindow->hOffset;
        uint32_t top    = (uint32_t)pWindow->vOffset;
        uint32_t right  = (uint32_t)(left + pWindow->width  - 1U);
        uint32_t bottom = (uint32_t)(top  + pWindow->height - 1U);

        /* according to the spec, the upper left corner has some restrictions: */
        if ( (left < MRV_AFM_A_H_L_MIN) || (left > MRV_AFM_A_H_L_MAX)
                || (top < MRV_AFM_A_V_T_MIN) || (top > MRV_AFM_A_V_T_MAX)
                || (right < MRV_AFM_A_H_R_MIN) || (right > MRV_AFM_A_H_R_MAX)
                || (bottom < MRV_AFM_A_V_B_MIN) || (bottom > MRV_AFM_A_V_B_MAX) )
        {
            return ( RET_OUTOFRANGE );
        }

        /* reset values */
        *pLT = 0U;
        *pRB = 0U;

        /* combine the values and return */
        REG_SET_SLICE( *pLT, MRV_AFM_A_H_L, left );
        REG_SET_SLICE( *pLT, MRV_AFM_A_V_T, top );
        REG_SET_SLICE( *pRB, MRV_AFM_A_H_R, right );
        REG_SET_SLICE( *pRB, MRV_AFM_A_V_B, bottom );
    }
    else
    {
        *pLT = 0U;
        *pRB = 0U;
    }

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver Internal API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspAfmInit()
 *****************************************************************************/
RESULT CamerIcIspAfmInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pIspAfmContext = ( CamerIcIspAfmContext_t *)malloc( sizeof(CamerIcIspAfmContext_t) );
    if (  ctx->pIspAfmContext == NULL )
    {
        TRACE( CAMERIC_ISP_AFM_DRV_ERROR,  "%s: Can't allocate CamerIC ISP AFM context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pIspAfmContext, 0, sizeof(CamerIcIspAfmContext_t) );

    ctx->pIspAfmContext->enabled    = BOOL_FALSE;

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAfmRelease()
 *****************************************************************************/
RESULT CamerIcIspAfmRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspAfmContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pIspAfmContext, 0, sizeof(CamerIcIspAfmContext_t) );
    free ( ctx->pIspAfmContext );
    ctx->pIspAfmContext = NULL;

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAfmSignal()
 *****************************************************************************/
void CamerIcIspAfmSignal
(
    const CamerIcIspAfmSignal_t signal,
    CamerIcDrvHandle_t          handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspAfmContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return;
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        volatile uint32_t value = 0UL;

        switch ( signal )
        {
            case CAMERIC_ISP_AFM_SIGNAL_MEASURMENT:
                {
                    CamerIcAfmMeasuringResult_t *pMeasResult = &ctx->pIspAfmContext->MeasResult;

                    MEMSET( pMeasResult, 0, sizeof(CamerIcAfmMeasuringResult_t) );

                    if ( ctx->pIspAfmContext->EnabledWdwA )
                    {
                        pMeasResult->SharpnessA = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_afm_sum_a) );
                        pMeasResult->LuminanceA = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_afm_lum_a) );
                        pMeasResult->LuminanceA <<= ctx->pIspAfmContext->lum_shift;
                        pMeasResult->PixelCntA  = ctx->pIspAfmContext->PixelCntA; 

                        pMeasResult->WindowA = ctx->pIspAfmContext->WindowA;
                    }
        
                    if ( ctx->pIspAfmContext->EnabledWdwB )
                    {
                        pMeasResult->SharpnessB = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_afm_sum_b) );
                        pMeasResult->LuminanceB = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_afm_lum_b) );
                        pMeasResult->LuminanceB <<= ctx->pIspAfmContext->lum_shift;
                        pMeasResult->PixelCntB  = ctx->pIspAfmContext->PixelCntB; 

                        pMeasResult->WindowB = ctx->pIspAfmContext->WindowB;
                    }
                    
                    if ( ctx->pIspAfmContext->EnabledWdwC )
                    {
                        pMeasResult->SharpnessC = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_afm_sum_c) );
                        pMeasResult->LuminanceC = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_afm_lum_c) );
                        pMeasResult->LuminanceC <<= ctx->pIspAfmContext->lum_shift;
                        pMeasResult->PixelCntC  = ctx->pIspAfmContext->PixelCntC; 

                        pMeasResult->WindowC = ctx->pIspAfmContext->WindowC;
                    }
                    
                    if ( ctx->pIspAfmContext->EventCb.func != NULL )
                    {
                        ctx->pIspAfmContext->EventCb.func( CAMERIC_ISP_EVENT_AFM,
                                (void *)pMeasResult, ctx->pIspAfmContext->EventCb.pUserContext );
                    }

                    break;
                }

            case CAMERIC_ISP_AFM_SIGNAL_SUM_OVERFLOW:
                {
                    TRACE( CAMERIC_ISP_AFM_DRV_WARN , "%s: LUMA-OVERFLOW\n", __FUNCTION__ );
                    break;
                }

            case CAMERIC_ISP_AFM_SIGNAL_LUMA_OVERFLOW:
                {
                    TRACE( CAMERIC_ISP_AFM_DRV_WARN , "%s: SUM-OVERFLOW\n", __FUNCTION__ );
                    break;
                }

            default:
                {
                    TRACE( CAMERIC_ISP_AFM_DRV_ERROR , "%s: unknow signal\n", __FUNCTION__ );
                    break;
                }
        }
    }               

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );
}



/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspAfmRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspAfmRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void 			    *pUserContext
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspAfmContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->pIspAfmContext->EventCb.func != NULL )
    {
        return ( RET_BUSY );
    }

    if ( func == NULL )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        ctx->pIspAfmContext->EventCb.func          = func;
        ctx->pIspAfmContext->EventCb.pUserContext  = pUserContext;
    }

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAfmDeRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspAfmDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspAfmContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)  
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    ctx->pIspAfmContext->EventCb.func          = NULL;
    ctx->pIspAfmContext->EventCb.pUserContext  = NULL;

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAfmEnable()
 *****************************************************************************/
RESULT CamerIcIspAfmEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspAfmContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_imsc;
        uint32_t isp_afm_ctrl;

        uint32_t isp_afm_var_shift = 0U;

        uint32_t MaxPixelCnt = 0U;
        uint32_t PixelCnt    = 0U;

        uint32_t lum_shift   = 0U;
        uint32_t afm_shift   = 0U;

        if ( ctx->pIspAfmContext->EnabledWdwA )
        {
            PixelCnt    =  ctx->pIspAfmContext->PixelCntA;
            MaxPixelCnt = PixelCnt;
        }
        
        if ( ctx->pIspAfmContext->EnabledWdwB )
        {
            PixelCnt =  ctx->pIspAfmContext->PixelCntB;
            MaxPixelCnt = ( PixelCnt > MaxPixelCnt ) ? PixelCnt : MaxPixelCnt;
        }

        if ( ctx->pIspAfmContext->EnabledWdwC )
        {
            PixelCnt =  ctx->pIspAfmContext->PixelCntC;
            MaxPixelCnt = ( PixelCnt > MaxPixelCnt ) ? PixelCnt : MaxPixelCnt;
        }

        lum_shift = CamerIcIspAfmCalcLuminanceShift( MaxPixelCnt );
        afm_shift = CamerIcIspAfmCalcTenengradShift( MaxPixelCnt );

        REG_SET_SLICE( isp_afm_var_shift, MRV_AFM_LUM_VAR_SHIFT, lum_shift );
        REG_SET_SLICE( isp_afm_var_shift, MRV_AFM_AFM_VAR_SHIFT, afm_shift );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_afm_var_shift), isp_afm_var_shift );

        ctx->pIspAfmContext->lum_shift   = lum_shift;
        ctx->pIspAfmContext->afm_shift   = afm_shift;
        ctx->pIspAfmContext->MaxPixelCnt = MaxPixelCnt;

        /* enable measuring module */
        isp_afm_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_afm_ctrl) );
        REG_SET_SLICE( isp_afm_ctrl, MRV_AFM_AFM_EN, 1U ); 
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_afm_ctrl), isp_afm_ctrl );

        /* enable interrupt */
        isp_imsc = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc) );
        isp_imsc |= ( MRV_ISP_IMSC_AFM_FIN_MASK | MRV_ISP_IMSC_AFM_LUM_OF_MASK | MRV_ISP_IMSC_AFM_SUM_OF_MASK );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc), isp_imsc );

        ctx->pIspAfmContext->enabled = BOOL_TRUE;
    }

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAfmDisable()
 *****************************************************************************/
RESULT CamerIcIspAfmDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspAfmContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_imsc;
        uint32_t isp_afm_ctrl;

        /* disable measuring module */
        isp_afm_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_afm_ctrl) );
        REG_SET_SLICE( isp_afm_ctrl, MRV_AFM_AFM_EN, 0U ); 
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_afm_ctrl), isp_afm_ctrl );

        /* disable interrupt */
        isp_imsc = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc) );
        isp_imsc &= ~( MRV_ISP_IMSC_AFM_FIN_MASK | MRV_ISP_IMSC_AFM_LUM_OF_MASK | MRV_ISP_IMSC_AFM_SUM_OF_MASK );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc), isp_imsc );

        ctx->pIspAfmContext->enabled = BOOL_FALSE;
    }


    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAfmIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspAfmIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspAfmContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pIsEnabled )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsEnabled = ctx->pIspAfmContext->enabled;

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAfmSetThreshold()
 *****************************************************************************/
RESULT CamerIcIspAfmSetThreshold
(
    CamerIcDrvHandle_t  handle,
    const uint32_t      threshold
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspAfmContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ( threshold == 0U ) || ( (threshold & MRV_AFM_AFM_THRES_MASK) == 0U ) )
    {
        return ( RET_OUTOFRANGE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_afm_thres), threshold );
        ctx->pIspAfmContext->Threshold = threshold;
    }

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}
/******************************************************************************
 * CamerIcIspAfmGetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspAfmGetMeasuringWindow
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspAfmWindowId_t   WdwId,
    CamerIcWindow_t                 *pWindow
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;
    

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspAfmContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    switch ( WdwId )
    {
        case CAMERIC_ISP_AFM_WINDOW_A:
            {
                *pWindow     = ctx->pIspAfmContext->WindowA;
                break;
            }

        case CAMERIC_ISP_AFM_WINDOW_B:
            {
                *pWindow     = ctx->pIspAfmContext->WindowB;
                break;
            }

        case CAMERIC_ISP_AFM_WINDOW_C:
            {
                *pWindow     = ctx->pIspAfmContext->WindowC;
                break;
            }

        default:
            {
                return ( RET_INVALID_PARM );
            }
    }    

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}

/******************************************************************************
 * CamerIcIspAfmSetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspAfmSetMeasuringWindow
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspAfmWindowId_t   WdwId,
    const uint16_t                  x,
    const uint16_t                  y,
    const uint16_t                  width,
    const uint16_t                  height
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    CamerIcWindow_t *pWindow;
    uint32_t        *pPixelCnt;

    uint32_t LT;
    uint32_t RB;

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );
    
    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (enter %d,%d,%d,%d)\n", __FUNCTION__, x, y, width, height );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspAfmContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    switch ( WdwId )
    {
        case CAMERIC_ISP_AFM_WINDOW_A:
            {
                pWindow     = &(ctx->pIspAfmContext->WindowA);
                pPixelCnt   = &(ctx->pIspAfmContext->PixelCntA);
                break;
            }

        case CAMERIC_ISP_AFM_WINDOW_B:
            {
                pWindow     = &(ctx->pIspAfmContext->WindowB);
                pPixelCnt   = &(ctx->pIspAfmContext->PixelCntB);
                break;
            }

        case CAMERIC_ISP_AFM_WINDOW_C:
            {
                pWindow     = &(ctx->pIspAfmContext->WindowC);
                pPixelCnt   = &(ctx->pIspAfmContext->PixelCntC);
                break;
            }

        default:
            {
                return ( RET_INVALID_PARM );
            }
    }

    pWindow->hOffset = x;
    pWindow->vOffset = y;
    pWindow->width   = width;
    pWindow->height  = height;
    *pPixelCnt       = ( ( height * width ) >> 1 ); /* only green pixel are measured */

    /* calc. measurement window boundaries */
    result = CamerIcIspAfmWnd2Regs( pWindow, &LT, &RB );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;


        switch ( WdwId )
        {
            case CAMERIC_ISP_AFM_WINDOW_A:
                {
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_afm_lt_a), LT );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_afm_rb_a), RB );
                    break;
                }

            case CAMERIC_ISP_AFM_WINDOW_B:
                {
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_afm_lt_b), LT );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_afm_rb_b), RB );
                    break;
                }

            case CAMERIC_ISP_AFM_WINDOW_C:
                {
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_afm_lt_c), LT );
                    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_afm_rb_c), RB );
                    break;
                }

            default:
                {
                    return ( RET_INVALID_PARM );
                }
        }
    }

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAfmSetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspAfmEnableMeasuringWindow
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspAfmWindowId_t   WdwId
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspAfmContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    switch ( WdwId )
    {
        case CAMERIC_ISP_AFM_WINDOW_A:
            {
                ctx->pIspAfmContext->EnabledWdwA = BOOL_TRUE;
                break;
            }

        case CAMERIC_ISP_AFM_WINDOW_B:
            {
                ctx->pIspAfmContext->EnabledWdwB = BOOL_TRUE;
                break;
            }

        case CAMERIC_ISP_AFM_WINDOW_C:
            {
                ctx->pIspAfmContext->EnabledWdwC = BOOL_TRUE;
                break;
            }

        default:
            {
                return ( RET_INVALID_PARM );
            }
    }

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspAfmMeasuringWindowIsEnabled()
 *****************************************************************************/
bool_t CamerIcIspAfmMeasuringWindowIsEnabled
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspAfmWindowId_t   WdwId
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    bool_t result = BOOL_FALSE;

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspAfmContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( BOOL_FALSE );
    }

    switch ( WdwId )
    {
        case CAMERIC_ISP_AFM_WINDOW_A:
            {
                result = ctx->pIspAfmContext->EnabledWdwA;
                break;
            }

        case CAMERIC_ISP_AFM_WINDOW_B:
            {
                result = ctx->pIspAfmContext->EnabledWdwB;
                break;
            }

        case CAMERIC_ISP_AFM_WINDOW_C:
            {
                result = ctx->pIspAfmContext->EnabledWdwC;
                break;
            }

        default:
            {
                result = BOOL_FALSE;
                break;
            }
    }

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamerIcIspAfmDisableMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspAfmDisableMeasuringWindow
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspAfmWindowId_t   WdwId
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspAfmContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    switch ( WdwId )
    {
        case CAMERIC_ISP_AFM_WINDOW_A:
            {
                ctx->pIspAfmContext->EnabledWdwA = BOOL_FALSE;
                break;
            }

        case CAMERIC_ISP_AFM_WINDOW_B:
            {
                ctx->pIspAfmContext->EnabledWdwB = BOOL_FALSE;
                break;
            }

        case CAMERIC_ISP_AFM_WINDOW_C:
            {
                ctx->pIspAfmContext->EnabledWdwC = BOOL_FALSE;
                break;
            }

        default:
            {
                return ( RET_INVALID_PARM );
            }
    }

    TRACE( CAMERIC_ISP_AFM_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}

#else  /* #if defined(MRV_AUTOFOCUS_VERSION) */

/******************************************************************************
 * Auto Focus Module is not available.
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 * 
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspAfmRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspAfmRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void 			    *pUserContext
)
{
    (void)handle;
    (void)func;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspAfmDeRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspAfmDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspAfmEnable()
 *****************************************************************************/
RESULT CamerIcIspAfmEnable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspAfmDisable()
 *****************************************************************************/
RESULT CamerIcIspAfmDisable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspAfmIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspAfmIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
)
{
    (void)handle;
    (void)pIsEnabled;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspAfmSetThreshold()
 *****************************************************************************/
RESULT CamerIcIspAfmSetThreshold
(
    CamerIcDrvHandle_t  handle,
    const uint32_t      threshold
)
{
    (void)handle;
    (void)threshold;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspAfmSetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspAfmSetMeasuringWindow
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspAfmWindowId_t   WdwId,
    const uint16_t                  x,
    const uint16_t                  y,
    const uint16_t                  width,
    const uint16_t                  height
)
{
    (void)handle;
    (void)WdwId;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspAfmSetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspAfmEnableMeasuringWindow
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspAfmWindowId_t   WdwId
)
{
    (void)handle;
    (void)WdwId;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspAfmMeasuringWindowIsEnabled()
 *****************************************************************************/
bool_t CamerIcIspAfmMeasuringWindowIsEnabled
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspAfmWindowId_t   WdwId
)
{
    (void)handle;
    (void)WdwId;
    return ( BOOL_FALSE );
}


/******************************************************************************
 * CamerIcIspAfmDisableMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspAfmDisableMeasuringWindow
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspAfmWindowId_t   WdwId
)
{
    (void)handle;
    (void)WdwId;
    return ( RET_NOTSUPP );
}

#endif /* #if defined(MRV_AUTOFOCUS_VERSION) */

