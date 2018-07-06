/******************************************************************************
 *
 * Copyright 2011, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
/**
 * @cond    cameric_ie
 *
 * @file    cameric_ie.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>

#include "mrv_all_bits.h"

#include "cameric_drv_cb.h"
#include "cameric_drv.h"


/******************************************************************************
 * Is the hardware Image Effects module available ?
 *****************************************************************************/
#if defined(MRV_IMAGE_EFFECTS_VERSION)

/******************************************************************************
 * Image Effects Module is available.
 *****************************************************************************/


/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_IE_DRV_INFO  , "CAMERIC-IE-DRV: ", INFO     , 0 );
CREATE_TRACER( CAMERIC_IE_DRV_WARN  , "CAMERIC-IE-DRV: ", WARNING  , 1 );
CREATE_TRACER( CAMERIC_IE_DRV_ERROR , "CAMERIC-IE-DRV: ", ERROR    , 1 );



/******************************************************************************
 * local type definitions
 *****************************************************************************/


/******************************************************************************
 * local variable declarations
 *****************************************************************************/


/******************************************************************************
 * local function prototypes
 *****************************************************************************/


/******************************************************************************/
/**
 * @brief   This function translates a chrominance component value from usual
 *          representation (range 16..240, 128=neutral grey) to the one used
 *          by the IMG_EFF_TINT register.
 *
 * @param   Cx  Chrominance component value in usual representation
 *
 * @return  Chrominance component value in register representation
 *
 *****************************************************************************/
static uint32_t CamerIcIeCx2RegVal
(
    uint8_t Cx
)
{
    int32_t  temp;
    uint32_t value;
   
    // apply scaling
    temp = 128L - (int32_t)Cx;
    temp = ( (temp * 64L) / 110L );

    // convert from two's complement to sign/value
    if ( temp < 0 )
    {
        value = 0x80UL;
        temp *= ( -1L );
    }
    else
    {
        value = 0x00UL;
    }

    // saturate at 7 bits
    if ( temp > 0x7F )
    {
        temp = 0x7F;
    }

    // combine sign and value to build the register value
    value |= (uint32_t)temp;
   
    return ( value );
}


/******************************************************************************/
/**
 * @brief   Translates usual (decimal) matrix coefficient into the 4 bit
 *          register representation (used in the ie_mat_X and ie_sket_X 
 *          registers). For unsupported decimal numbers, a supported 
 *          replacement is automatically selected. 
 *
 * @param   Matrix coefficient value in usual (decimal) representation
 *
 * @return  Matrix coefficient value in register representation.
 *
 *****************************************************************************/
static uint32_t CamerIcDec2CoeffValue
(
    int8_t decimal
)
{
    TRACE( CAMERIC_IE_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    uint32_t value = 0UL;

    if ( decimal <= -6L )
    {
        // equivlent to -8
        value = (uint32_t)(MRV_IMGEFF_COEF_MIN_EIGHT | (MRV_IMGEFF_COEF_ON << MRV_IMGEFF_COEF_SHIFT));
    }
    else if ( decimal <= -3L )
    {
        // equivlent to -4
        value = (uint32_t)(MRV_IMGEFF_COEF_MIN_FOUR | (MRV_IMGEFF_COEF_ON << MRV_IMGEFF_COEF_SHIFT));
    }
    else if ( decimal == -2L )
    {
        // equivlent to -2
        value = (uint32_t)(MRV_IMGEFF_COEF_MIN_TWO | (MRV_IMGEFF_COEF_ON << MRV_IMGEFF_COEF_SHIFT));
    }
    else if ( decimal == -1L )
    {
        // equivlent to -1
        value = (uint32_t)(MRV_IMGEFF_COEF_MIN_ONE | (MRV_IMGEFF_COEF_ON << MRV_IMGEFF_COEF_SHIFT));
    }
    else if ( decimal == 0L )
    {
        // equivlent to 0 (entry not used)
        value = (uint32_t)(MRV_IMGEFF_COEF_OFF << MRV_IMGEFF_COEF_SHIFT);
    }
    else if ( decimal == 1L )
    {
        // equivlent to 1
        value = (uint32_t)(MRV_IMGEFF_COEF_ONE | (MRV_IMGEFF_COEF_ON << MRV_IMGEFF_COEF_SHIFT));
    }
    else if ( decimal == 2L )
    {
        // equivlent to 2
        value = (uint32_t)(MRV_IMGEFF_COEF_TWO | (MRV_IMGEFF_COEF_ON << MRV_IMGEFF_COEF_SHIFT));
    }
    else if ( decimal < 6L )
    {
        // equivlent to 4
        value = (uint32_t)(MRV_IMGEFF_COEF_FOUR | (MRV_IMGEFF_COEF_ON << MRV_IMGEFF_COEF_SHIFT));
    }
    else
    {
        // equivlent to 8
        value = (uint32_t)(MRV_IMGEFF_COEF_EIGHT | (MRV_IMGEFF_COEF_ON << MRV_IMGEFF_COEF_SHIFT));
    }

    TRACE( CAMERIC_IE_DRV_INFO, "%s: %d, 0x%08x (exit)\n", __FUNCTION__, decimal, value );

    return ( value );
}


/******************************************************************************/
/**
 * @brief   This functions resets the CamerIC IE module. 
 *
 * @param   ctx                 Driver contecxt for getting HalHandle to access
 *                              hardware
 * @param   stay_in_reset       if TRUE than the IE hardware module stays in 
 *                              reset state
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
static RESULT CamerIcIeReset
(
    CamerIcDrvContext_t *ctx,
    const bool_t        stay_in_reset
)
{
    TRACE( CAMERIC_IE_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t vi_ircl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_ircl) );
        REG_SET_SLICE( vi_ircl, MRV_VI_IE_SOFT_RST, 1 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_ircl), vi_ircl );

        if ( BOOL_FALSE == stay_in_reset )
        {
            REG_SET_SLICE( vi_ircl, MRV_VI_IE_SOFT_RST, 0 );
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_ircl), vi_ircl );
        }
    }

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIeSepia()
 *****************************************************************************/
static RESULT CamerIcIeSepia
(
    CamerIcDrvContext_t *ctx
)
{
    TRACE( CAMERIC_IE_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->HalHandle == NULL) || (ctx->pIeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t img_eff_tint = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_tint) );

        //REG_SET_SLICE( img_eff_tint, MRV_IMGEFF_INCR_CR, CamerIcIeCx2RegVal(ctx->pIeContext->config.ModeConfig.Sepia.TintCr) );
        //REG_SET_SLICE( img_eff_tint, MRV_IMGEFF_INCR_CB, CamerIcIeCx2RegVal(ctx->pIeContext->config.ModeConfig.Sepia.TintCb) );
        REG_SET_SLICE( img_eff_tint, MRV_IMGEFF_INCR_CR, (uint32_t)(ctx->pIeContext->config.ModeConfig.Sepia.TintCr) );
        REG_SET_SLICE( img_eff_tint, MRV_IMGEFF_INCR_CB, (uint32_t)(ctx->pIeContext->config.ModeConfig.Sepia.TintCb) );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_tint), img_eff_tint );
    }

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIeColorSelection()
 *****************************************************************************/
static RESULT CamerIcIeColorSelection
(
    CamerIcDrvContext_t *ctx
)
{
    TRACE( CAMERIC_IE_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->HalHandle == NULL) || (ctx->pIeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t img_eff_color_sel = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_color_sel) );

        switch ( ctx->pIeContext->config.ModeConfig.ColorSelection.col_selection )
        {
            case CAMERIC_IE_COLOR_SELECTION_RGB:
                {
                    REG_SET_SLICE( img_eff_color_sel, MRV_IMGEFF_COLOR_SELECTION, MRV_IMGEFF_COLOR_SELECTION_RGB );
                    break;
                }

            case CAMERIC_IE_COLOR_SELECTION_B: 
                {
                    REG_SET_SLICE( img_eff_color_sel, MRV_IMGEFF_COLOR_SELECTION, MRV_IMGEFF_COLOR_SELECTION_B );
                    break;
                }

            case CAMERIC_IE_COLOR_SELECTION_G:
                {
                    REG_SET_SLICE( img_eff_color_sel, MRV_IMGEFF_COLOR_SELECTION, MRV_IMGEFF_COLOR_SELECTION_G );
                    break;
                }

            case CAMERIC_IE_COLOR_SELECTION_GB:
                {
                    REG_SET_SLICE( img_eff_color_sel, MRV_IMGEFF_COLOR_SELECTION, MRV_IMGEFF_COLOR_SELECTION_BG );
                    break;
                }

            case CAMERIC_IE_COLOR_SELECTION_R:
                {
                    REG_SET_SLICE( img_eff_color_sel, MRV_IMGEFF_COLOR_SELECTION, MRV_IMGEFF_COLOR_SELECTION_R );
                    break;
                }

            case CAMERIC_IE_COLOR_SELECTION_RB:
                {
                    REG_SET_SLICE( img_eff_color_sel, MRV_IMGEFF_COLOR_SELECTION, MRV_IMGEFF_COLOR_SELECTION_RB );
                    break;
                }

            case CAMERIC_IE_COLOR_SELECTION_RG:
                {
                    REG_SET_SLICE( img_eff_color_sel, MRV_IMGEFF_COLOR_SELECTION, MRV_IMGEFF_COLOR_SELECTION_RG );
                    break;
                }

            default:
                {
                    TRACE( CAMERIC_IE_DRV_ERROR, "%s: unsopported color selection(%d)\n", 
                            __FUNCTION__, ctx->pIeContext->config.ModeConfig.ColorSelection.col_selection );
                    return ( RET_NOTSUPP );
                }
        }

        REG_SET_SLICE( img_eff_color_sel, MRV_IMGEFF_COLOR_THRESHOLD, ctx->pIeContext->config.ModeConfig.ColorSelection.col_threshold );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_color_sel), img_eff_color_sel );
    }

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIeEmboss()
 *****************************************************************************/
static RESULT CamerIcIeEmboss
(
    CamerIcDrvContext_t *ctx
)
{
    TRACE( CAMERIC_IE_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->HalHandle == NULL) || (ctx->pIeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t img_eff_mat_1 = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_mat_1) );
        uint32_t img_eff_mat_2 = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_mat_2) );
        uint32_t img_eff_mat_3 = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_mat_3) );

        REG_SET_SLICE( img_eff_mat_1, MRV_IMGEFF_EMB_COEF_11_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Emboss.coeff[0U] ) );
        REG_SET_SLICE( img_eff_mat_1, MRV_IMGEFF_EMB_COEF_12_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Emboss.coeff[1U] ) );
        REG_SET_SLICE( img_eff_mat_1, MRV_IMGEFF_EMB_COEF_13_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Emboss.coeff[2U] ) );
        REG_SET_SLICE( img_eff_mat_1, MRV_IMGEFF_EMB_COEF_21_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Emboss.coeff[3U] ) );

        REG_SET_SLICE( img_eff_mat_2, MRV_IMGEFF_EMB_COEF_22_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Emboss.coeff[4U] ) );
        REG_SET_SLICE( img_eff_mat_2, MRV_IMGEFF_EMB_COEF_23_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Emboss.coeff[5U] ) );
        REG_SET_SLICE( img_eff_mat_2, MRV_IMGEFF_EMB_COEF_31_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Emboss.coeff[6U] ) );
        REG_SET_SLICE( img_eff_mat_2, MRV_IMGEFF_EMB_COEF_32_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Emboss.coeff[7U] ) );

        REG_SET_SLICE( img_eff_mat_3, MRV_IMGEFF_EMB_COEF_33_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Emboss.coeff[8U] ) );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_mat_1), img_eff_mat_1 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_mat_2), img_eff_mat_2 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_mat_3), img_eff_mat_3 );
    }

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIeSketch()
 *****************************************************************************/
static RESULT CamerIcIeSketch
(
    CamerIcDrvContext_t *ctx
)
{
    TRACE( CAMERIC_IE_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->HalHandle == NULL) || (ctx->pIeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t img_eff_mat_3 = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_mat_3) );
        uint32_t img_eff_mat_4 = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_mat_4) );
        uint32_t img_eff_mat_5 = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_mat_5) );

        REG_SET_SLICE( img_eff_mat_3, MRV_IMGEFF_SKET_COEF_11_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Sketch.coeff[0U] ) );
        REG_SET_SLICE( img_eff_mat_3, MRV_IMGEFF_SKET_COEF_12_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Sketch.coeff[1U] ) );
        REG_SET_SLICE( img_eff_mat_3, MRV_IMGEFF_SKET_COEF_13_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Sketch.coeff[2U] ) );

        REG_SET_SLICE( img_eff_mat_4, MRV_IMGEFF_SKET_COEF_21_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Sketch.coeff[3U] ) );
        REG_SET_SLICE( img_eff_mat_4, MRV_IMGEFF_SKET_COEF_22_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Sketch.coeff[4U] ) );
        REG_SET_SLICE( img_eff_mat_4, MRV_IMGEFF_SKET_COEF_23_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Sketch.coeff[5U] ) );
        REG_SET_SLICE( img_eff_mat_4, MRV_IMGEFF_SKET_COEF_31_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Sketch.coeff[6U] ) );

        REG_SET_SLICE( img_eff_mat_5, MRV_IMGEFF_SKET_COEF_32_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Sketch.coeff[7U] ) );
        REG_SET_SLICE( img_eff_mat_5, MRV_IMGEFF_SKET_COEF_33_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Sketch.coeff[8U] ) );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_mat_3), img_eff_mat_3 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_mat_4), img_eff_mat_4 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_mat_5), img_eff_mat_5 );
    }

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIeSharpen()
 *****************************************************************************/
static RESULT CamerIcIeSharpen
(
    CamerIcDrvContext_t *ctx
)
{
    TRACE( CAMERIC_IE_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->HalHandle == NULL) || (ctx->pIeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t img_eff_mat_3 = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_mat_3) );
        uint32_t img_eff_mat_4 = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_mat_4) );
        uint32_t img_eff_mat_5 = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_mat_5) );
        uint32_t img_eff_sharpen = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_sharpen) );

        REG_SET_SLICE( img_eff_mat_3, MRV_IMGEFF_SKET_COEF_11_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Sharpen.coeff[0U] ) );
        REG_SET_SLICE( img_eff_mat_3, MRV_IMGEFF_SKET_COEF_12_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Sharpen.coeff[1U] ) );
        REG_SET_SLICE( img_eff_mat_3, MRV_IMGEFF_SKET_COEF_13_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Sharpen.coeff[2U] ) );

        REG_SET_SLICE( img_eff_mat_4, MRV_IMGEFF_SKET_COEF_21_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Sharpen.coeff[3U] ) );
        REG_SET_SLICE( img_eff_mat_4, MRV_IMGEFF_SKET_COEF_22_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Sharpen.coeff[4U] ) );
        REG_SET_SLICE( img_eff_mat_4, MRV_IMGEFF_SKET_COEF_23_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Sharpen.coeff[5U] ) );
        REG_SET_SLICE( img_eff_mat_4, MRV_IMGEFF_SKET_COEF_31_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Sharpen.coeff[6U] ) );

        REG_SET_SLICE( img_eff_mat_5, MRV_IMGEFF_SKET_COEF_32_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Sharpen.coeff[7U] ) );
        REG_SET_SLICE( img_eff_mat_5, MRV_IMGEFF_SKET_COEF_33_4,
                CamerIcDec2CoeffValue( ctx->pIeContext->config.ModeConfig.Sharpen.coeff[8U] ) );
        
        REG_SET_SLICE( img_eff_sharpen, MRV_IMGEFF_SHARP_FACTOR,
                ctx->pIeContext->config.ModeConfig.Sharpen.factor );
        
        REG_SET_SLICE( img_eff_sharpen, MRV_IMGEFF_CORING_THR,
                ctx->pIeContext->config.ModeConfig.Sharpen.threshold );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_mat_3), img_eff_mat_3 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_mat_4), img_eff_mat_4 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_mat_5), img_eff_mat_5 );
        
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_sharpen), img_eff_sharpen );
    }

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver Internal API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIeEnableClock()
 *****************************************************************************/
RESULT CamerIcIeEnableClock
(
    CamerIcDrvHandle_t  handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->HalHandle == NULL) || (ctx->pIeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t vi_iccl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_iccl) );

        REG_SET_SLICE( vi_iccl, MRV_VI_IE_CLK_ENABLE, 1 );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_iccl), vi_iccl );
        
        result = CamerIcIeReset( ctx, BOOL_FALSE );
    }

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * CamerIcIeDisableClock()
 *****************************************************************************/
RESULT CamerIcIeDisableClock
(
    CamerIcDrvHandle_t  handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t vi_iccl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_iccl) );

        REG_SET_SLICE( vi_iccl, MRV_VI_IE_CLK_ENABLE, 0 );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_iccl), vi_iccl );

        result = CamerIcIeReset( ctx, BOOL_TRUE );
    }

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * CamerIcIeInit()
 *****************************************************************************/
RESULT CamerIcIeInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pIeContext = ( CamerIcIeContext_t *)malloc( sizeof(CamerIcIeContext_t) );
    if (  ctx->pIeContext == NULL )
    {
        TRACE( CAMERIC_IE_DRV_ERROR,  "%s: Can't allocate CamerIC IE context.\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pIeContext, 0, sizeof(CamerIcIeContext_t) );

    result = CamerIcIeEnableClock( ctx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAMERIC_IE_DRV_ERROR,  "%s: Failed to enable clock for CamerIC IE module.\n",  __FUNCTION__ );
        return ( result );
    }

    ctx->pIeContext->enabled = BOOL_FALSE;

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * CamerIcIeRelease()
 *****************************************************************************/
RESULT CamerIcIeRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIeContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pIeContext, 0, sizeof(CamerIcIeContext_t) );
    free ( ctx->pIeContext );
    ctx->pIeContext = NULL;

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIeEnable()
 *****************************************************************************/
RESULT CamerIcIeEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIeContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t img_eff_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_ctrl) );

        result = CamerIcIeReset( handle, BOOL_FALSE );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }

        switch ( ctx->pIeContext->config.mode )
        {
            case CAMERIC_IE_MODE_GRAYSCALE:
                {
                    REG_SET_SLICE( img_eff_ctrl, MRV_IMGEFF_EFFECT_MODE, MRV_IMGEFF_EFFECT_MODE_GRAY );
                    break;
                }

            case CAMERIC_IE_MODE_NEGATIVE:
                {
                    REG_SET_SLICE( img_eff_ctrl, MRV_IMGEFF_EFFECT_MODE, MRV_IMGEFF_EFFECT_MODE_NEGATIVE );
                    break;
                }

            case CAMERIC_IE_MODE_SEPIA:
                {
                    result = CamerIcIeSepia( handle );
                    if ( result != RET_SUCCESS )
                    {
                        return ( result );
                    }
                    REG_SET_SLICE( img_eff_ctrl, MRV_IMGEFF_EFFECT_MODE, MRV_IMGEFF_EFFECT_MODE_SEPIA );
                    break;
                }

            case CAMERIC_IE_MODE_COLOR:
                {
                    result = CamerIcIeColorSelection( handle );
                    if ( result != RET_SUCCESS )
                    {
                        return ( result );
                    }
                    REG_SET_SLICE( img_eff_ctrl, MRV_IMGEFF_EFFECT_MODE, MRV_IMGEFF_EFFECT_MODE_COLOR_SEL );
                    break;
                }

            case CAMERIC_IE_MODE_EMBOSS:
                {
                    result = CamerIcIeEmboss( handle );
                    if ( result != RET_SUCCESS )
                    {
                        return ( result );
                    }
                    REG_SET_SLICE( img_eff_ctrl, MRV_IMGEFF_EFFECT_MODE, MRV_IMGEFF_EFFECT_MODE_EMBOSS );
                    break;
                }

            case CAMERIC_IE_MODE_SKETCH:
                {
                    result = CamerIcIeSketch( handle );
                    if ( result != RET_SUCCESS )
                    {
                        return ( result );
                    }
                    REG_SET_SLICE( img_eff_ctrl, MRV_IMGEFF_EFFECT_MODE, MRV_IMGEFF_EFFECT_MODE_SKETCH );
                    break;
                }

            case CAMERIC_IE_MODE_SHARPEN:
                {
                    result = CamerIcIeSharpen( handle );
                    if ( result != RET_SUCCESS )
                    {
                        return ( result );
                    }

                    REG_SET_SLICE( img_eff_ctrl, MRV_IMGEFF_EFFECT_MODE, MRV_IMGEFF_EFFECT_MODE_SHARPEN );
                    break;
                }

            default:
                {
                    TRACE( CAMERIC_IE_DRV_ERROR, "%s: unsopported mode(%d)\n", __FUNCTION__, ctx->pIeContext->config.mode );
                    return ( RET_NOTSUPP );
                }
        }

        switch ( ctx->pIeContext->config.range )
        {
            case CAMERIC_IE_RANGE_BT601:
                {
                    REG_SET_SLICE( img_eff_ctrl, MRV_IMGEFF_FULL_RANGE, MRV_IMGEFF_FULL_RANGE_BT601 );
                    break;
                }

            case CAMERIC_IE_RANGE_FULL_RANGE:
                {
                    REG_SET_SLICE( img_eff_ctrl, MRV_IMGEFF_FULL_RANGE, MRV_IMGEFF_FULL_RANGE_FULL );
                    break;
                }

            default:
                {
                    TRACE( CAMERIC_IE_DRV_ERROR, "%s: unsopported range(%d)\n", __FUNCTION__, ctx->pIeContext->config.range );
                    return ( RET_NOTSUPP );
                }
        }
        
        REG_SET_SLICE( img_eff_ctrl, MRV_IMGEFF_CFG_UPD, MRV_IMGEFF_CFG_UPD_UPDATE );
        REG_SET_SLICE( img_eff_ctrl, MRV_IMGEFF_BYPASS_MODE, MRV_IMGEFF_BYPASS_MODE_PROCESS );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_ctrl), img_eff_ctrl );
        
        ctx->pIeContext->enabled = BOOL_TRUE;
    }

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIeDisable()
 *****************************************************************************/
RESULT CamerIcIeDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIeContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t img_eff_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_ctrl) );

        REG_SET_SLICE( img_eff_ctrl, MRV_IMGEFF_CFG_UPD, MRV_IMGEFF_CFG_UPD_UPDATE );
        REG_SET_SLICE( img_eff_ctrl, MRV_IMGEFF_BYPASS_MODE, MRV_IMGEFF_BYPASS_MODE_BYPASS );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->img_eff_ctrl), img_eff_ctrl );

        ctx->pIeContext->enabled = BOOL_FALSE;
    }


    TRACE( CAMERIC_IE_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIeIsEnabled()
 *****************************************************************************/
RESULT CamerIcIeIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pIsEnabled )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsEnabled = ctx->pIeContext->enabled;

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIeConfigure()
 *****************************************************************************/
RESULT CamerIcIeConfigure
(
    CamerIcDrvHandle_t      handle,
    CamerIcIeConfig_t       *pConfig
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pConfig == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    
    if ( ctx->pIeContext->enabled == BOOL_TRUE )
    {
        return ( RET_BUSY );
    }

    if ( (pConfig->mode <= CAMERIC_IE_MODE_INVALID) || (pConfig->mode >= CAMERIC_IE_MODE_MAX) )
    {
        TRACE( CAMERIC_IE_DRV_ERROR, "%s: unsopported effect mode(%d)\n", __FUNCTION__, pConfig->mode );
        return ( RET_NOTSUPP );
    }

    if ( (pConfig->range <= CAMERIC_IE_RANGE_INVALID) || (pConfig->range >= CAMERIC_IE_RANG_MAX) )
    {
        TRACE( CAMERIC_IE_DRV_ERROR, "%s: unsopported pixel range(%d)\n", __FUNCTION__, pConfig->range );
        return ( RET_NOTSUPP );
    }

    switch ( pConfig->mode )
    {
        case CAMERIC_IE_MODE_GRAYSCALE:
            {
                break;
            }

        case CAMERIC_IE_MODE_NEGATIVE:
            {
                break;
            }

        case CAMERIC_IE_MODE_SEPIA:
            {
                break;
            }

        case CAMERIC_IE_MODE_COLOR:
            {
                if ( (pConfig->ModeConfig.ColorSelection.col_selection <= CAMERIC_IE_COLOR_SELECTION_INVALID)
                        || (pConfig->ModeConfig.ColorSelection.col_selection >= CAMERIC_IE_COLOR_SELECTION_MAX) )
                {
                    TRACE( CAMERIC_IE_DRV_ERROR, "%s: unsopported color selection mode(%d)\n",
                                    __FUNCTION__, pConfig->ModeConfig.ColorSelection.col_selection );
                    return ( RET_NOTSUPP );
                }
                break;
            }

        case CAMERIC_IE_MODE_EMBOSS:
            {
                break;
            }

        case CAMERIC_IE_MODE_SKETCH:
            {
                break;
            }

        case CAMERIC_IE_MODE_SHARPEN:
            {
                break;
            }

        default:
            {
                /* already checked */
                TRACE( CAMERIC_IE_DRV_ERROR, "%s: unsopported mode(%d)\n", __FUNCTION__, ctx->pIeContext->config.mode );
                return ( RET_NOTSUPP );
            }
    }

    MEMCPY( &ctx->pIeContext->config, pConfig, sizeof( CamerIcIeConfig_t ) );

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIeSetTintCb()
 *****************************************************************************/
RESULT CamerIcIeSetTintCb
(
    CamerIcDrvHandle_t  handle,
    const uint8_t       tint
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIeContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->pIeContext->config.mode != CAMERIC_IE_MODE_SEPIA )
    {
        return ( RET_BUSY );
    }

    ctx->pIeContext->config.ModeConfig.Sepia.TintCb = tint;

    result = CamerIcIeSepia( handle );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIeSetTintCr()
 *****************************************************************************/
RESULT CamerIcIeSetTintCr
(
    CamerIcDrvHandle_t  handle,
    const uint8_t       tint
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIeContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->pIeContext->config.mode != CAMERIC_IE_MODE_SEPIA )
    {
        return ( RET_BUSY );
    }

    ctx->pIeContext->config.ModeConfig.Sepia.TintCr = tint;

    result = CamerIcIeSepia( handle );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIeSetColorSelection()
 *****************************************************************************/
RESULT CamerIcIeSetColorSelection
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIeColorSelection_t color,
    const uint8_t                   threshold
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIeContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->pIeContext->config.mode != CAMERIC_IE_MODE_COLOR )
    {
        return ( RET_BUSY );
    }

    ctx->pIeContext->config.ModeConfig.ColorSelection.col_selection = color;
    ctx->pIeContext->config.ModeConfig.ColorSelection.col_threshold = threshold;

    result = CamerIcIeColorSelection( handle );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIeSetSharpen()
 *****************************************************************************/
RESULT CamerIcIeSetSharpen
(
    CamerIcDrvHandle_t              handle,
    const uint8_t                   factor,
    const uint8_t                   threshold
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIeContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->pIeContext->config.mode != CAMERIC_IE_MODE_SHARPEN )
    {
        return ( RET_BUSY );
    }

    ctx->pIeContext->config.ModeConfig.Sharpen.factor = factor;
    ctx->pIeContext->config.ModeConfig.Sharpen.threshold = threshold;

    result = CamerIcIeSharpen( handle );
    if ( result != RET_SUCCESS )
    {
       return ( result );
    }

    TRACE( CAMERIC_IE_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


#else  /* #if defined(MRV_IMAGE_EFFECTS_VERSION) */

/******************************************************************************
 * Image Effects Module is not available.
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 * 
 *****************************************************************************/

/******************************************************************************
 * CamerIcIeEnable()
 *****************************************************************************/
RESULT CamerIcIeEnable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIeDisable()
 *****************************************************************************/
RESULT CamerIcIeDisable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIeIsEnabled()
 *****************************************************************************/
RESULT CamerIcIeIsEnabled
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
 * CamerIcIeConfigure()
 *****************************************************************************/
RESULT CamerIcIeConfigure
(
    CamerIcDrvHandle_t      handle,
    CamerIcIeConfig_t       *pConfig
)
{
    (void)handle;
    (void)pConfig;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIeSetTintCb()
 *****************************************************************************/
RESULT CamerIcIeSetTintCb
(
    CamerIcDrvHandle_t  handle,
    const uint8_t       tint
)
{
    (void)handle;
    (void)tint;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIeSetTintCr()
 *****************************************************************************/
RESULT CamerIcIeSetTintCr
(
    CamerIcDrvHandle_t  handle,
    const uint8_t       tint
)
{
    (void)handle;
    (void)tint;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIeSetColorSelection()
 *****************************************************************************/
RESULT CamerIcIeSetColorSelection
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIeColorSelection_t color,
    const uint8_t                   threshold
)
{
    (void)handle;
    (void)color;
    (void)threshold;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIeSetSharpen()
 *****************************************************************************/
RESULT CamerIcIeSetSharpen
(
    CamerIcDrvHandle_t              handle,
    const uint8_t                   factor,
    const uint8_t                   threshold
)
{
    (void)handle;
    (void)factor;
    (void)threshold;
    return ( RET_NOTSUPP );
}

#endif /* #if defined(MRV_IMAGE_EFFECTS_VERSION) */

