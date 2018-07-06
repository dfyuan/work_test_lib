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
 * @file cameric_isp_dpf.c
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
 * Is the hardware DPF module available ?
 *****************************************************************************/
#if defined(MRV_DPF_VERSION) 

/******************************************************************************
 * DPF module is available.
 *****************************************************************************/


/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_ISP_DPF_DRV_INFO  , "CAMERIC-ISP-DPF-DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_ISP_DPF_DRV_WARN  , "CAMERIC-ISP-DPF-DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_ISP_DPF_DRV_ERROR , "CAMERIC-ISP-DPF-DRV: ", ERROR   , 1 );


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
 * CamerIcIspDpfSetMode()
 *****************************************************************************/
static RESULT CamerIcIspDpfSetMode
(
    CamerIcDrvContext_t                 *ctx,

    const CamerIcDpfGainUsage_t         GainUsage,
    const CamerIcDpfRedBlueFilterSize_t RBFilterSize,
    const bool_t                        ProcessRedPixel,
    const bool_t                        ProcessGreenRPixel,
    const bool_t                        ProcessGreenBPixel,
    const bool_t                        ProcessBluePixel
)
{
    volatile MrvAllRegister_t *ptMrvReg;
    uint32_t isp_dpf_mode;

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspDpfContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_dpf_mode = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_mode) );
        isp_dpf_mode &= (MRV_DPF_DPF_ENABLE_MASK | MRV_DPF_NLL_SEGMENTATION_MASK);

        switch ( GainUsage )
        {
            case CAMERIC_DPF_GAIN_USAGE_DISABLED:
                {
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_USE_NF_GAIN    , 0 );
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_LSC_GAIN_COMP  , 0 );
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_AWB_GAIN_COMP  , 0 );
                    break;
                }

            case CAMERIC_DPF_GAIN_USAGE_NF_GAINS:
                {
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_USE_NF_GAIN    , 1 );
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_LSC_GAIN_COMP  , 0 );
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_AWB_GAIN_COMP  , 1 );
                    break;
                }

            case CAMERIC_DPF_GAIN_USAGE_LSC_GAINS:
                {
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_USE_NF_GAIN    , 0 );
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_LSC_GAIN_COMP  , 1 );
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_AWB_GAIN_COMP  , 0 );
                    break;
                }

            case CAMERIC_DPF_GAIN_USAGE_NF_LSC_GAINS:
                {
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_USE_NF_GAIN    , 1 );
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_LSC_GAIN_COMP  , 1 );
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_AWB_GAIN_COMP  , 1 );
                    break;
                }

            case CAMERIC_DPF_GAIN_USAGE_AWB_GAINS:
                {
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_USE_NF_GAIN    , 0 );
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_LSC_GAIN_COMP  , 0 );
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_AWB_GAIN_COMP  , 1 );
                    break;
                }

            case CAMERIC_DPF_GAIN_USAGE_AWB_LSC_GAINS:
                {
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_USE_NF_GAIN    , 0 );
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_LSC_GAIN_COMP  , 1 );
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_AWB_GAIN_COMP  , 1 );
                    break;
                }

            default:
                {
                    TRACE( CAMERIC_ISP_DPF_DRV_ERROR,  "%s: unsupported gain usage\n",  __FUNCTION__ );
                    return ( RET_OUTOFRANGE );
                }
        }

        switch ( RBFilterSize )
        {
            case CAMERIC_DPF_RB_FILTERSIZE_13x9:
                {
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_RB_FILTER_SIZE , 0U );
                    break;
                }

            case CAMERIC_DPF_RB_FILTERSIZE_9x9:
                {
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_RB_FILTER_SIZE , 1U );
                    break;
                }

            default:
                {
                    TRACE( CAMERIC_ISP_DPF_DRV_ERROR,  "%s: unsupported filter kernel size for red/blue pixel\n",  __FUNCTION__ );
                    return ( RET_OUTOFRANGE );
                }
        }

        REG_SET_SLICE( isp_dpf_mode, MRV_DPF_R_FILTER_OFF   , ( ProcessRedPixel     == BOOL_TRUE ) ? 0 : 1 );
        REG_SET_SLICE( isp_dpf_mode, MRV_DPF_GR_FILTER_OFF  , ( ProcessGreenRPixel  == BOOL_TRUE ) ? 0 : 1 );
        REG_SET_SLICE( isp_dpf_mode, MRV_DPF_GB_FILTER_OFF  , ( ProcessGreenBPixel  == BOOL_TRUE ) ? 0 : 1 );
        REG_SET_SLICE( isp_dpf_mode, MRV_DPF_B_FILTER_OFF   , ( ProcessBluePixel    == BOOL_TRUE ) ? 0 : 1 );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_mode), isp_dpf_mode);

        ctx->pIspDpfContext->GainUsage          = GainUsage;
        ctx->pIspDpfContext->RBFilterSize       = RBFilterSize;

        ctx->pIspDpfContext->ProcessRedPixel    = ProcessRedPixel;
        ctx->pIspDpfContext->ProcessGreenRPixel = ProcessGreenRPixel;
        ctx->pIspDpfContext->ProcessGreenBPixel = ProcessGreenBPixel;
        ctx->pIspDpfContext->ProcessBluePixel   = ProcessBluePixel;
    }

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver Internal API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspDpfInit()
 *****************************************************************************/
RESULT CamerIcIspDpfInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pIspDpfContext = ( CamerIcIspDpfContext_t *)malloc( sizeof(CamerIcIspDpfContext_t) );
    if (  ctx->pIspDpfContext == NULL )
    {
        TRACE( CAMERIC_ISP_DPF_DRV_ERROR,  "%s: Can't allocate CamerIC ISP context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pIspDpfContext, 0, sizeof( CamerIcIspDpfContext_t ) );

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspRelease()
 *****************************************************************************/
RESULT CamerIcIspDpfRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspDpfContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pIspDpfContext, 0, sizeof( CamerIcIspDpfContext_t ) );
    free ( ctx->pIspDpfContext );
    ctx->pIspDpfContext = NULL;

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspDpfEnable()
 *****************************************************************************/
RESULT CamerIcIspDpfEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspDpfContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_dpf_mode = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_mode) );
        isp_dpf_mode |= MRV_DPF_DPF_ENABLE_MASK;
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_mode), isp_dpf_mode);
    }

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspDpfDisable()
 *****************************************************************************/
RESULT CamerIcIspDpfDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspDpfContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_dpf_mode = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_mode) );
        isp_dpf_mode &= (~ MRV_DPF_DPF_ENABLE_MASK);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_mode), isp_dpf_mode);
    }

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspDpfConfig()
 *****************************************************************************/
RESULT CamerIcIspDpfConfig
(
    CamerIcDrvHandle_t          handle,
    const CamerIcDpfConfig_t    *pConfig
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL)
            || (ctx->pIspDpfContext == NULL)
            || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pConfig == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    if ( ctx->pIspDpfContext->enabled == BOOL_TRUE )
    {
        return ( RET_WRONG_STATE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
      
        uint32_t value = 0UL;

        RESULT result;

        if ( (pConfig->SpatialG.WeightCoeff[0] > (MRV_DPF_S_WEIGHT_G1_MASK >> MRV_DPF_S_WEIGHT_G1_SHIFT)) 
                || (pConfig->SpatialG.WeightCoeff[1] > (MRV_DPF_S_WEIGHT_G2_MASK >> MRV_DPF_S_WEIGHT_G2_SHIFT))
                || (pConfig->SpatialG.WeightCoeff[2] > (MRV_DPF_S_WEIGHT_G3_MASK >> MRV_DPF_S_WEIGHT_G3_SHIFT))
                || (pConfig->SpatialG.WeightCoeff[3] > (MRV_DPF_S_WEIGHT_G4_MASK >> MRV_DPF_S_WEIGHT_G4_SHIFT))
                || (pConfig->SpatialG.WeightCoeff[4] > (MRV_DPF_S_WEIGHT_G5_MASK >> MRV_DPF_S_WEIGHT_G5_SHIFT))
                || (pConfig->SpatialG.WeightCoeff[5] > (MRV_DPF_S_WEIGHT_G6_MASK >> MRV_DPF_S_WEIGHT_G6_SHIFT)) )
        {
            return ( RET_OUTOFRANGE );
        }

        if ( (pConfig->SpatialRB.WeightCoeff[0] > (MRV_DPF_S_WEIGHT_RB1_MASK >> MRV_DPF_S_WEIGHT_RB1_SHIFT))
                || (pConfig->SpatialRB.WeightCoeff[1] > (MRV_DPF_S_WEIGHT_RB2_MASK >> MRV_DPF_S_WEIGHT_RB2_SHIFT))
                || (pConfig->SpatialRB.WeightCoeff[2] > (MRV_DPF_S_WEIGHT_RB3_MASK >> MRV_DPF_S_WEIGHT_RB3_SHIFT))
                || (pConfig->SpatialRB.WeightCoeff[3] > (MRV_DPF_S_WEIGHT_RB4_MASK >> MRV_DPF_S_WEIGHT_RB4_SHIFT))
                || (pConfig->SpatialRB.WeightCoeff[4] > (MRV_DPF_S_WEIGHT_RB5_MASK >> MRV_DPF_S_WEIGHT_RB5_SHIFT))
                || (pConfig->SpatialRB.WeightCoeff[5] > (MRV_DPF_S_WEIGHT_RB6_MASK >> MRV_DPF_S_WEIGHT_RB6_SHIFT)) )
        {
            return ( RET_OUTOFRANGE );
        }

        result =  CamerIcIspDpfSetMode( ctx,
                                            pConfig->GainUsage,
                                            pConfig->RBFilterSize,
                                            pConfig->ProcessRedPixel,
                                            pConfig->ProcessGreenRPixel,
                                            pConfig->ProcessGreenBPixel,
                                            pConfig->ProcessBluePixel );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }

        /* green spatial weight register */
        value = 0UL;
        REG_SET_SLICE(value, MRV_DPF_S_WEIGHT_G1, pConfig->SpatialG.WeightCoeff[0]);
        REG_SET_SLICE(value, MRV_DPF_S_WEIGHT_G2, pConfig->SpatialG.WeightCoeff[1]);
        REG_SET_SLICE(value, MRV_DPF_S_WEIGHT_G3, pConfig->SpatialG.WeightCoeff[2]);
        REG_SET_SLICE(value, MRV_DPF_S_WEIGHT_G4, pConfig->SpatialG.WeightCoeff[3]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_s_weight_g_1_4), value);

        value = 0UL;
        REG_SET_SLICE(value, MRV_DPF_S_WEIGHT_G5, pConfig->SpatialG.WeightCoeff[4]);
        REG_SET_SLICE(value, MRV_DPF_S_WEIGHT_G6, pConfig->SpatialG.WeightCoeff[5]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_s_weight_g_5_6), value);

        /* red/blue spatial weight register */
        value = 0UL;
        REG_SET_SLICE(value, MRV_DPF_S_WEIGHT_RB1, pConfig->SpatialRB.WeightCoeff[0]);
        REG_SET_SLICE(value, MRV_DPF_S_WEIGHT_RB2, pConfig->SpatialRB.WeightCoeff[1]);
        REG_SET_SLICE(value, MRV_DPF_S_WEIGHT_RB3, pConfig->SpatialRB.WeightCoeff[2]);
        REG_SET_SLICE(value, MRV_DPF_S_WEIGHT_RB4, pConfig->SpatialRB.WeightCoeff[3]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_s_weight_rb_1_4), value);

        value = 0UL;
        REG_SET_SLICE(value, MRV_DPF_S_WEIGHT_RB5, pConfig->SpatialRB.WeightCoeff[4]);
        REG_SET_SLICE(value, MRV_DPF_S_WEIGHT_RB6, pConfig->SpatialRB.WeightCoeff[5]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_s_weight_rb_5_6), value);
        
        MEMCPY( &ctx->pIspDpfContext->SpatialG, &pConfig->SpatialG, sizeof( CamerIcDpfSpatial_t ));
        MEMCPY( &ctx->pIspDpfContext->SpatialRB, &pConfig->SpatialRB, sizeof( CamerIcDpfSpatial_t ));
    }

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}
 


/******************************************************************************
 * CamerIcIspDpfSetNoiseLevelGain()
 *****************************************************************************/
RESULT CamerIcIspDpfSetNoiseFunctionGain
(
    CamerIcDrvHandle_t      handle,
    const CamerIcGains_t    *pNfGains
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspDpfContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pNfGains == NULL  )
    {
        return ( RET_INVALID_PARM );
    }
    
    if ( (pNfGains->Red & ~MRV_DPF_DPF_NF_GAIN_R_MASK)
            || (pNfGains->Blue & ~MRV_DPF_DPF_NF_GAIN_B_MASK)
            || (pNfGains->GreenR & ~MRV_DPF_DPF_NF_GAIN_GR_MASK)
            || (pNfGains->GreenB & ~MRV_DPF_DPF_NF_GAIN_GB_MASK) )
    {
        return ( RET_OUTOFRANGE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        /* noise level gain register */
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_nf_gain_r),  pNfGains->Red );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_nf_gain_gr), pNfGains->GreenR );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_nf_gain_gb), pNfGains->GreenB );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_nf_gain_b),  pNfGains->Blue );
    }
  
    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspDpfSetStrength()
 *****************************************************************************/
RESULT CamerIcIspDpfSetStrength
(
    CamerIcDrvHandle_t              handle,
    const CamerIcDpfInvStrength_t   *pDpfStrength
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspDpfContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pDpfStrength == NULL )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
         volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

         /* inverse strength register */
         HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_strength_r), ( MRV_DPF_INV_WEIGHT_R_MASK & pDpfStrength->WeightR ) );
         HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_strength_g), ( MRV_DPF_INV_WEIGHT_G_MASK & pDpfStrength->WeightG ) );
         HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_strength_b), ( MRV_DPF_INV_WEIGHT_B_MASK & pDpfStrength->WeightB ) );
    }

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspDpfSetNoiseLevelLookUp()
 *****************************************************************************/
RESULT CamerIcIspDpfSetNoiseLevelLookUp
(
    CamerIcDrvHandle_t                  handle,
    const CamerIcDpfNoiseLevelLookUp_t  *pDpfNll
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    uint32_t i = 0UL;

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspDpfContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pDpfNll == NULL )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t isp_dpf_mode;

        /* check and write NLF coefficients */
        for ( i=0U; i<NLF_LOOKUP_TABLE_BLOCK_ARR_SIZE; i++ )
        {
            if ( pDpfNll->NllCoeff[i] > MRV_DPF_NLL_COEFF_N_MASK )
            {
                return ( RET_OUTOFRANGE );
            }
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->nlf_lookup_table_block_arr[i] ), pDpfNll->NllCoeff[i] );
        }
        
        /* write segmentation type of NLF */
        isp_dpf_mode = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_mode) );
        switch ( pDpfNll->xScale )
        {
            case CAMERIC_NLL_SCALE_LINEAR:
                {
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_NLL_SEGMENTATION, 0 );
                    break;
                }

            case CAMERIC_NLL_SCALE_LOGARITHMIC:
                {
                    REG_SET_SLICE( isp_dpf_mode, MRV_DPF_NLL_SEGMENTATION, 1 );
                    break;
                }

            default:
                {
                    TRACE( CAMERIC_ISP_DPF_DRV_ERROR, "%s: selected x-scale not supported (%d)\n", __FUNCTION__, pDpfNll->xScale );
                    return ( RET_OUTOFRANGE );
                }
        }
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_mode), isp_dpf_mode);
    }

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspDpfSetStaticDemoConfig12()
 * for gains between 1..2
 *****************************************************************************/
RESULT CamerIcIspDpfSetStaticDemoConfigGain12
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->pIspDpfContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_s_weight_g_1_4),  0x00000203 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_s_weight_g_5_6),  0x00000000 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_s_weight_rb_1_4), 0x02030406 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_s_weight_rb_5_6), 0x00000101 );

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspDpfSetStaticDemoConfig24()
 * for gains between 2..4
 *****************************************************************************/
RESULT CamerIcIspDpfSetStaticDemoConfigGain24
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->pIspDpfContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_s_weight_g_1_4),  0x00010204 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_s_weight_g_5_6),  0x00000000 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_s_weight_rb_1_4), 0x090A0C0E );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_s_weight_rb_5_6), 0x00000305 );

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspDpfSetStaticDemoConfig48()
 * for gains between 4..8
 *****************************************************************************/
RESULT CamerIcIspDpfSetStaticDemoConfigGain48
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->pIspDpfContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_s_weight_g_1_4),  0x00010206 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_s_weight_g_5_6),  0x00000000 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_s_weight_rb_1_4), 0x090A0C0E );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpf_s_weight_rb_5_6), 0x00000305 );

    TRACE( CAMERIC_ISP_DPF_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}

#else  /* #if defined(MRV_DPF_VERSION) */

/******************************************************************************
 * DPF module is not available.
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 *
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspDpfEnable()
 *****************************************************************************/
RESULT CamerIcIspDpfEnable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspDpfDisable()
 *****************************************************************************/
RESULT CamerIcIspDpfDisable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspDpfConfig()
 *****************************************************************************/
RESULT CamerIcIspDpfConfig
(
    CamerIcDrvHandle_t          handle,
    const CamerIcDpfConfig_t    *pConfig
)
{
    (void)handle;
    (void)pConfig;
    return ( RET_NOTSUPP );
}
 


/******************************************************************************
 * CamerIcIspDpfSetNoiseLevelGain()
 *****************************************************************************/
RESULT CamerIcIspDpfSetNoiseFunctionGain
(
    CamerIcDrvHandle_t      handle,
    const CamerIcGains_t    *pNfGains
)
{
    (void)handle;
    (void)pNfGains;
    return ( RET_NOTSUPP );
}



/******************************************************************************
 * CamerIcIspDpfSetStrength()
 *****************************************************************************/
RESULT CamerIcIspDpfSetStrength
(
    CamerIcDrvHandle_t              handle,
    const CamerIcDpfInvStrength_t   *pDpfStrength
)
{
    (void)handle;
    (void)pDpfStrength;
    return ( RET_NOTSUPP );
}



/******************************************************************************
 * CamerIcIspDpfSetNoiseLevelLookUp()
 *****************************************************************************/
RESULT CamerIcIspDpfSetNoiseLevelLookUp
(
    CamerIcDrvHandle_t                  handle,
    const CamerIcDpfNoiseLevelLookUp_t  *pDpfNll
)
{
    (void)handle;
    (void)pDpfNll;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspDpfSetStaticDemoConfig12()
 * for gains in range 1..2
 *****************************************************************************/
RESULT CamerIcIspDpfSetStaticDemoConfigGain12
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspDpfSetStaticDemoConfig24()
 * for gains in range 2..4
 *****************************************************************************/
RESULT CamerIcIspDpfSetStaticDemoConfigGain24
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspDpfSetStaticDemoConfig48()
 * for gains in range 4..8
 *****************************************************************************/
RESULT CamerIcIspDpfSetStaticDemoConfigGain48
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


#endif /* #if defined(MRV_DPF_VERSION) */

