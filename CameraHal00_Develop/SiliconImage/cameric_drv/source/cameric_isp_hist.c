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
 * @file cameric_isp_hist.c
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
 * Is the hardware Histogram measuring module available ?
 *****************************************************************************/
#if defined(MRV_HISTOGRAM_VERSION)

/******************************************************************************
 * Histogram Measuring Module is available.
 *****************************************************************************/

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_ISP_HIST_DRV_INFO  , "CAMERIC-ISP-HIST-DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_ISP_HIST_DRV_WARN  , "CAMERIC-ISP-HIST-DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_ISP_HIST_DRV_ERROR , "CAMERIC-ISP-HIST-DRV: ", ERROR   , 1 );


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


/******************************************************************************
 * UpdateStepSize()
 *****************************************************************************/
static RESULT UpdateStepSize
(
    const CamerIcIspHistMode_t  mode,
    const CamerIcHistWeights_t  weights,
    const uint16_t              width,
    const uint16_t              height,
    uint8_t                     *StepSize
)
{
    uint32_t i;
    uint32_t square;

    uint32_t MaxNumOfPixel = 0U;

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    for ( i=0; i<CAMERIC_ISP_HIST_GRID_ITEMS; i++ )
    {
        MaxNumOfPixel += weights[i];
    }
    MaxNumOfPixel = MaxNumOfPixel * ( (uint32_t)height * (uint32_t)width );

    switch ( mode )
    {
        case CAMERIC_ISP_HIST_MODE_RGB_COMBINED:
            {
                square = (uint8_t)( (3 * MaxNumOfPixel) / MRV_HIST_BIN_N_MASK );
                break;
            }

        case CAMERIC_ISP_HIST_MODE_R:
        case CAMERIC_ISP_HIST_MODE_G:
        case CAMERIC_ISP_HIST_MODE_B:
        case CAMERIC_ISP_HIST_MODE_Y:
            {
                square = (uint8_t)( MaxNumOfPixel / MRV_HIST_BIN_N_MASK );
                break;
            }

        default:
            {
                TRACE( CAMERIC_ISP_HIST_DRV_ERROR, "%s: Invalid histogram mode (%d) selected\n", __FUNCTION__, mode );
                return ( RET_OUTOFRANGE );
            }
    }

    /* avoid fsqrt */
    for (i = 3; i<127; i++ )
    {
        if ( square <= ( i * i ) )
        {
            *StepSize = (uint8_t)i;
            break;
        }
    }

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspHistInitGridWeigths()
 *
 * set all weights to 1 to overwrite the RESET-Value
 *
 *****************************************************************************/
static RESULT CamerIcIspHistInitGridWeigths
(
    CamerIcDrvHandle_t  handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    uint16_t value;

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspHistContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t i;
        #if 0
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_00to30), 0x01010100U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_40to21), 0x04020100U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_31to12), 0x04010102U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_22to03), 0x01010408U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_13to43), 0x01020402U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_04to34), 0x01010100U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_44), 0x00U );
        ctx->pIspHistContext->Weights[0] = 0x00;    //exp_weight_00to40
        ctx->pIspHistContext->Weights[1] = 0x01;
        ctx->pIspHistContext->Weights[2] = 0x01;
        ctx->pIspHistContext->Weights[3] = 0x01;
        ctx->pIspHistContext->Weights[4] = 0x00;

        ctx->pIspHistContext->Weights[5] = 0x01;    //exp_weight_01to41
        ctx->pIspHistContext->Weights[6] = 0x02;
        ctx->pIspHistContext->Weights[7] = 0x04;
        ctx->pIspHistContext->Weights[8] = 0x02;
        ctx->pIspHistContext->Weights[9] = 0x01;

        ctx->pIspHistContext->Weights[10] = 0x01;    //exp_weight_02to42
        ctx->pIspHistContext->Weights[11] = 0x04;
        ctx->pIspHistContext->Weights[12] = 0x08;
        ctx->pIspHistContext->Weights[13] = 0x04;
        ctx->pIspHistContext->Weights[14] = 0x01;

        ctx->pIspHistContext->Weights[15] = 0x01;    //exp_weight_03to43
        ctx->pIspHistContext->Weights[16] = 0x02;
        ctx->pIspHistContext->Weights[17] = 0x04;
        ctx->pIspHistContext->Weights[18] = 0x02;
        ctx->pIspHistContext->Weights[19] = 0x01;

        ctx->pIspHistContext->Weights[20] = 0x00;    //exp_weight_04to44
        ctx->pIspHistContext->Weights[21] = 0x01;
        ctx->pIspHistContext->Weights[22] = 0x01;
        ctx->pIspHistContext->Weights[23] = 0x01;
        ctx->pIspHistContext->Weights[24] = 0x00;
        #elif 1
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_00to30), 0x00010000U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_40to21), 0x02020000U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_31to12), 0x04000002U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_22to03), 0x00000408U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_13to43), 0x00020202U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_04to34), 0x00000000U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_44), 0x00U );
        ctx->pIspHistContext->Weights[0] = 0x00;    //exp_weight_00to40
        ctx->pIspHistContext->Weights[1] = 0x00;
        ctx->pIspHistContext->Weights[2] = 0x01;
        ctx->pIspHistContext->Weights[3] = 0x00;
        ctx->pIspHistContext->Weights[4] = 0x00;

        ctx->pIspHistContext->Weights[5] = 0x00;    //exp_weight_01to41
        ctx->pIspHistContext->Weights[6] = 0x02;
        ctx->pIspHistContext->Weights[7] = 0x02;
        ctx->pIspHistContext->Weights[8] = 0x02;
        ctx->pIspHistContext->Weights[9] = 0x00;

        ctx->pIspHistContext->Weights[10] = 0x00;    //exp_weight_02to42
        ctx->pIspHistContext->Weights[11] = 0x04;
        ctx->pIspHistContext->Weights[12] = 0x08;
        ctx->pIspHistContext->Weights[13] = 0x04;
        ctx->pIspHistContext->Weights[14] = 0x00;

        ctx->pIspHistContext->Weights[15] = 0x00;    //exp_weight_03to43
        ctx->pIspHistContext->Weights[16] = 0x02;
        ctx->pIspHistContext->Weights[17] = 0x04;
        ctx->pIspHistContext->Weights[18] = 0x02;
        ctx->pIspHistContext->Weights[19] = 0x00;

        ctx->pIspHistContext->Weights[20] = 0x00;    //exp_weight_04to44
        ctx->pIspHistContext->Weights[21] = 0x00;
        ctx->pIspHistContext->Weights[22] = 0x00;
        ctx->pIspHistContext->Weights[23] = 0x00;
        ctx->pIspHistContext->Weights[24] = 0x00;
        #else
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_00to30), 0x01010101U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_40to21), 0x01010101U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_31to12), 0x01010101U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_22to03), 0x01010101U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_13to43), 0x01010101U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_04to34), 0x01010101U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_44), 0x01U );
        for ( i=0; i<CAMERIC_ISP_HIST_GRID_ITEMS; i++ )
        {
            ctx->pIspHistContext->Weights[i] = 0x01U;
        }
        #endif
        
    }

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver internal API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspHistInit()
 *****************************************************************************/
RESULT CamerIcIspHistInit
(
    CamerIcDrvHandle_t handle
)
{
    RESULT result;

    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pIspHistContext= ( CamerIcIspHistContext_t *)malloc( sizeof(CamerIcIspHistContext_t) );
    if (  ctx->pIspHistContext == NULL )
    {
        TRACE( CAMERIC_ISP_HIST_DRV_ERROR,  "%s: Can't allocate CamerIC ISP HIST context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pIspHistContext, 0, sizeof( CamerIcIspHistContext_t ) );

    ctx->pIspHistContext->enabled = BOOL_FALSE;

    result = CamerIcIspHistInitGridWeigths( handle );

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamerIcIspHistRelease()
 *****************************************************************************/
RESULT CamerIcIspHistRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspHistContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pIspHistContext, 0, sizeof( CamerIcIspHistContext_t ) );
    free ( ctx->pIspHistContext );
    ctx->pIspHistContext= NULL;

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspHistSignal()
 *****************************************************************************/
void CamerIcIspHistSignal
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspHistContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return;
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t i;

        for ( i=0; i<CAMERIC_ISP_HIST_NUM_BINS; i++ )
        {
            ctx->pIspHistContext->Bins[i] = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->histogram_measurement_result_arr[i]) );
        }

        if ( ctx->pIspHistContext->EventCb.func != NULL )
        {
            ctx->pIspHistContext->EventCb.func( CAMERIC_ISP_EVENT_HISTOGRAM,
                                                    (void *)(ctx->pIspHistContext->Bins),
                                                    ctx->pIspHistContext->EventCb.pUserContext );
        }
    }

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );
}



/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspHistRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspHistRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void 			    *pUserContext
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspHistContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( ctx->pIspHistContext->EventCb.func != NULL )
    {
        return ( RET_BUSY );
    }

    if ( func == NULL )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        ctx->pIspHistContext->EventCb.func          = func;
        ctx->pIspHistContext->EventCb.pUserContext  = pUserContext;
    }

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspHistDeRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspHistDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspHistContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    ctx->pIspHistContext->EventCb.func          = NULL;
    ctx->pIspHistContext->EventCb.pUserContext  = NULL;

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspHistEnable()
 *****************************************************************************/
RESULT CamerIcIspHistEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_hist_prop;

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspHistContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_hist_prop;
        uint32_t isp_imsc;

        isp_hist_prop = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_prop) );
        switch ( ctx->pIspHistContext->mode )
        {

            case CAMERIC_ISP_HIST_MODE_RGB_COMBINED:
                {
                    REG_SET_SLICE( isp_hist_prop, MRV_HIST_MODE, MRV_HIST_MODE_RGB );
                    break;
                }

            case CAMERIC_ISP_HIST_MODE_R:
                {
                    REG_SET_SLICE( isp_hist_prop, MRV_HIST_MODE, MRV_HIST_MODE_R );
                    break;
                }

            case CAMERIC_ISP_HIST_MODE_G:
                {
                    REG_SET_SLICE( isp_hist_prop, MRV_HIST_MODE, MRV_HIST_MODE_G );
                    break;
                }

            case CAMERIC_ISP_HIST_MODE_B:
                {
                    REG_SET_SLICE( isp_hist_prop, MRV_HIST_MODE, MRV_HIST_MODE_B );
                    break;
                }

            case CAMERIC_ISP_HIST_MODE_Y:
                {
                    REG_SET_SLICE( isp_hist_prop, MRV_HIST_MODE, MRV_HIST_MODE_LUM );
                    break;
                }

            default:
                {
                    ctx->pIspHistContext->enabled = BOOL_FALSE;
                    TRACE( CAMERIC_ISP_HIST_DRV_ERROR,
                            "%s: Invalid histogram mode (%d) selected\n", __FUNCTION__, ctx->pIspHistContext->mode );
                    return ( RET_OUTOFRANGE );
                }
        }

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_prop), isp_hist_prop);

        /* enable interrupt */
        isp_imsc = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc) );
        isp_imsc |= MRV_ISP_IMSC_HIST_MEASURE_RDY_MASK;
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc), isp_imsc );

        TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: isp_imsc=0x%08x\n", __FUNCTION__, isp_imsc);

        ctx->pIspHistContext->enabled = BOOL_TRUE;
    }

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspHistDisable()
 *****************************************************************************/
RESULT CamerIcIspHistDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspHistContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_hist_prop;
        uint32_t isp_imsc;

        isp_hist_prop = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_prop) );
        REG_SET_SLICE(isp_hist_prop,   MRV_HIST_MODE, MRV_HIST_MODE_NONE);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_prop), isp_hist_prop);

        /* disable interrupt */
        isp_imsc = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc) );
        isp_imsc &= ~MRV_ISP_IMSC_HIST_MEASURE_RDY_MASK;
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_imsc), isp_imsc );

        ctx->pIspHistContext->enabled = BOOL_FALSE;
    }

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspHistIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspHistIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspHistContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pIsEnabled )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsEnabled = ctx->pIspHistContext->enabled;

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspHistSetMeasuringMode()
 *****************************************************************************/
RESULT CamerIcIspHistSetMeasuringMode
(
    CamerIcDrvHandle_t  		handle,
    const CamerIcIspHistMode_t	mode
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspHistContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if (  (mode > CAMERIC_ISP_HIST_MODE_INVALID) && (mode < CAMERIC_ISP_HIST_MODE_MAX) )
    {
        ctx->pIspHistContext->mode = mode;
    }
    else
    {
        return ( RET_OUTOFRANGE );
    }

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspHistSetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspHistSetMeasuringWindow
(
    CamerIcDrvHandle_t  handle,
    const uint16_t      x,
    const uint16_t      y,
    const uint16_t      width,
    const uint16_t      height
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    uint16_t value;

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspHistContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ((x!=0) && !(MRV_HIST_H_OFFSET_MASK & x))
            || ((y!=0) && !(MRV_HIST_V_OFFSET_MASK & y))
            || !(MRV_HIST_V_SIZE_MASK & (height / 5))
            || !(MRV_HIST_H_SIZE_MASK & (width / 5)) )
    {
        return ( RET_OUTOFRANGE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t isp_hist_prop;

        uint8_t StepSize = 0;
        uint16_t GridWidth;
        uint16_t GridHeight;

        RESULT result;

        GridWidth  = (uint16_t)(width / 5);
        GridHeight = (uint16_t)(height / 5);

        /* update stepsize */
        result = UpdateStepSize( ctx->pIspHistContext->mode, ctx->pIspHistContext->Weights, GridWidth, GridHeight, &StepSize );
        if ( RET_SUCCESS != result )
        {
            return ( result );
        }

        /* write updated stepsize to hardware */
        isp_hist_prop = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_prop) );
        REG_SET_SLICE(isp_hist_prop, MRV_HIST_STEPSIZE, StepSize);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_prop), isp_hist_prop);

        /* write measuting grid to hardware */
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_h_offs), (MRV_HIST_H_OFFSET_MASK & x) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_v_offs), (MRV_HIST_V_OFFSET_MASK & y) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_h_size), (MRV_HIST_H_SIZE_MASK & GridWidth) );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_v_size), (MRV_HIST_V_SIZE_MASK & GridHeight) );

        ctx->pIspHistContext->Grid.hOffset = 0;
        ctx->pIspHistContext->Grid.vOffset = 0;
        ctx->pIspHistContext->Grid.width   = GridWidth;
        ctx->pIspHistContext->Grid.height  = GridHeight;

        ctx->pIspHistContext->Window.hOffset = x;
        ctx->pIspHistContext->Window.vOffset = y;
        ctx->pIspHistContext->Window.width   = width;
        ctx->pIspHistContext->Window.height  = height;

        ctx->pIspHistContext->StepSize     = StepSize;
    }

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspHistSetGridWeights()
 *****************************************************************************/
RESULT CamerIcIspHistSetGridWeights
(
    CamerIcDrvHandle_t          handle,
    const CamerIcHistWeights_t  weights
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;

    RESULT result;

    uint32_t i, value;

    uint8_t StepSize = 0;

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspHistContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    for ( i=0; i<CAMERIC_ISP_HIST_GRID_ITEMS; i++ )
    {
        if ( weights[i] > MRV_HIST_WEIGHT_MAX )
        {
            return ( RET_OUTOFRANGE );
        }
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

    /* update stepsize */
    result = UpdateStepSize( ctx->pIspHistContext->mode, weights,
                                ctx->pIspHistContext->Grid.width, ctx->pIspHistContext->Grid.height, &StepSize );
    if ( RET_SUCCESS != result )
    {
        return ( result );
    }

    /* write updated stepsize to hardware */
    value = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_prop) );
    REG_SET_SLICE( value, MRV_HIST_STEPSIZE, StepSize );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_prop), value);

    /* write weights to hardware */
    value = 0U;
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_00, weights[0] );
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_10, weights[1] );
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_20, weights[2] );
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_30, weights[3] );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_00to30), value );

    value = 0U;
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_40, weights[4] );
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_01, weights[5] );
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_11, weights[6] );
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_21, weights[7] );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_40to21), value);

    value = 0U;
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_31, weights[8] );
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_41, weights[9] );
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_02, weights[10] );
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_12, weights[11] );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_31to12), value );

    value = 0U;
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_22, weights[12] );
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_32, weights[13] );
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_42, weights[14] );
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_03, weights[15] );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_22to03), value );

    value = 0U;
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_13, weights[16] );
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_23, weights[17] );
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_33, weights[18] );
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_43, weights[19] );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_13to43), value );

    value = 0U;
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_04, weights[20] );
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_14, weights[21] );
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_24, weights[22] );
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_34, weights[23] );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_04to34), value );

    value = 0U;
    REG_SET_SLICE( value, MRV_HIST_WEIGHT_44, weights[24] );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_hist_weight_44), value );

    /* store value into context */
    for ( i=0; i<CAMERIC_ISP_HIST_GRID_ITEMS; i++ )
    {
        ctx->pIspHistContext->Weights[i] = weights[i];
    }

    ctx->pIspHistContext->StepSize = StepSize;

    TRACE( CAMERIC_ISP_HIST_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


#else  /* if defined(MRV_HISTOGRAM_VERSION) */

/******************************************************************************
 * Histogram Measuring Module is not available.
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 * 
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspHistRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspHistRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void 			    *pUserContext
)
{
    (void)handle;
    (void)pUserContext;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspHistDeRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcIspHistDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspHistEnable()
 *****************************************************************************/
RESULT CamerIcIspHistEnable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspHistDisable()
 *****************************************************************************/
RESULT CamerIcIspHistDisable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspHistIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspHistIsEnabled
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
 * CamerIcIspHistSetMeasuringMode()
 *****************************************************************************/
RESULT CamerIcIspHistSetMeasuringMode
(
    CamerIcDrvHandle_t  		handle,
    const CamerIcIspHistMode_t	mode
)
{
    (void)handle;
    (void)mode;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspHistSetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspHistSetMeasuringWindow
(
    CamerIcDrvHandle_t  handle,
    const uint16_t      x,
    const uint16_t      y,
    const uint16_t      width,
    const uint16_t      height
)
{
    (void)handle;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspHistSetGridWeights()
 *****************************************************************************/
RESULT CamerIcIspHistSetGridWeights
(
    CamerIcDrvHandle_t          handle,
    const CamerIcHistWeights_t  weights
)
{
    (void)handle;
    (void)weights;
    return ( RET_NOTSUPP );
}


#endif /* if defined(MRV_HISTOGRAM_VERSION) */

