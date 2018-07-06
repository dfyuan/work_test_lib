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
 * @file aec.c
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
#include <common/misc.h>

#include "sem.h"
#include "asem.h"
#include "clm.h"
#include "ecm.h"

#include "aec.h"
#include "aec_ctrl.h"



/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( AEC_INFO  , "AEC: ", INFO     , 0 );
CREATE_TRACER( AEC_WARN  , "AEC: ", WARNING  , 1 );
CREATE_TRACER( AEC_ERROR , "AEC: ", ERROR    , 1 );

CREATE_TRACER( AEC_DEBUG , "AEC: ", INFO     , 0 );



/******************************************************************************
 * local type definitions
 *****************************************************************************/


/******************************************************************************
 * local variable declarations
 *****************************************************************************/


/******************************************************************************
 * local functions
 *****************************************************************************/

/******************************************************************************
 * AecMeanLuma()
 *****************************************************************************/
static float AecMeanLuma
(
    AecContext_t        *pAecCtx,
    const CamerIcMeanLuma_t luma
)
{
    float sum0 = 0.0f,sum = 0.0f;
    int32_t i;
    unsigned int sum1=0,sum2=0,sum3=0;

    for ( i=0U; i<CAMERIC_ISP_EXP_GRID_ITEMS; i++ )
    {
        sum += (float)(((uint32_t)luma[i]*pAecCtx->GridWeights[i]));
        sum1 += pAecCtx->GridWeights[i];
    }

    return ( sum / sum1 );
}



/******************************************************************************
 * AecUpdateConfig()
 *****************************************************************************/
static RESULT AecUpdateConfig
(
    AecContext_t        *pAecCtx,
    AecConfig_t         *pConfig,
    bool_t              isReConfig
)
{
    RESULT result = RET_SUCCESS;
	int32_t i = 0;
	int32_t j = 0;
    bool_t getLimits = BOOL_TRUE; // default to use current resolution's limit
    uint32_t EcmResolution = 0; // default to use current resolution
    float curMaxIntegrationTime,curMinIntegrationTime,curMaxGain,curMinGain;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    DCT_ASSERT(pAecCtx != NULL);
    DCT_ASSERT(pConfig != NULL);

    // update context
    //TODO: update ECM with new resolution dependent settings from CamCalibDB
    pAecCtx->SemMode            = pConfig->SemMode;

    if ( ( pAecCtx->EcmFlickerSelect != pConfig->EcmFlickerSelect ) || 
         ( pAecCtx->AfpsEnabled      != pConfig->AfpsEnabled ) )
    {
        // we need to restart AEC to ensure flickerfreeness
        isReConfig = BOOL_FALSE;
    }

    pAecCtx->AfpsEnabled      = pConfig->AfpsEnabled;
    pAecCtx->EcmFlickerSelect = pConfig->EcmFlickerSelect;    // update context
    switch( pConfig->EcmFlickerSelect )
    {
        default:
            TRACE( AEC_ERROR, "%s: Invalid flicker period; defaulting to AEC_EXPOSURE_CONVERSION_TFLICKER_OFF\n", __FUNCTION__ );
            // fall through!
        case AEC_EXPOSURE_CONVERSION_FLICKER_OFF: 
            pAecCtx->EcmTflicker = ECM_TFLICKER_OFF;
            break;

        case AEC_EXPOSURE_CONVERSION_FLICKER_100HZ:
            pAecCtx->EcmTflicker = ECM_TFLICKER_100HZ;
            break;

        case AEC_EXPOSURE_CONVERSION_FLICKER_120HZ:
            pAecCtx->EcmTflicker = ECM_TFLICKER_120HZ;
            break;
    }

    if (isReConfig == BOOL_FALSE)
    {
        pAecCtx->EcmT0fac           = 0; //pConfig->EcmT0fac;
        pAecCtx->EcmT0              = 0; //pConfig->EcmT0fac * pAecCtx->EcmTflicker;
        pAecCtx->EcmA0              = 0; //pConfig->EcmA0;
        pAecCtx->AfpsCurrResolution = 0;

        pAecCtx->hSensor            = pConfig->hSensor;
        pAecCtx->hSubSensor         = pConfig->hSubSensor;
    
        pAecCtx->hCamCalibDb        = pConfig->hCamCalibDb;

        if ( pAecCtx->hCamCalibDb )
        {
            CamCalibAecGlobal_t *pAecGlobal;
            CamEcmProfile_t     *ppEcmProfile;

            result = CamCalibDbGetAecGlobal( pAecCtx->hCamCalibDb, &pAecGlobal );
            if ( result != RET_SUCCESS )
            {
                TRACE( AEC_ERROR, "%s: Access to CalibDB failed. (%d)\n", __FUNCTION__, result );
                return ( result );
            }
            
            pAecCtx->SetPoint       = pAecGlobal->SetPoint;
            pAecCtx->ClmTolerance   = pAecGlobal->ClmTolerance;
            pAecCtx->DampOverStill  = pAecGlobal->DampOverStill;
            pAecCtx->DampUnderStill = pAecGlobal->DampUnderStill;
            pAecCtx->DampOverVideo  = pAecGlobal->DampOverVideo;
            pAecCtx->DampUnderVideo = pAecGlobal->DampUnderVideo;
            pAecCtx->AfpsMaxGain    = pAecGlobal->AfpsMaxGain;
 			for(i=0; i<CAMERIC_ISP_EXP_GRID_ITEMS; i++){
 				if(pAecGlobal->GridWeights.uCoeff[i])
 					break;
 			}
 			if(i<CAMERIC_ISP_EXP_GRID_ITEMS){
				//MEMCPY(pAecCtx->GridWeights,pAecGlobal->GridWeights.uCoeff,sizeof(pAecGlobal->GridWeights.uCoeff));
				for(j=0; j<CAMERIC_ISP_EXP_GRID_ITEMS; j++){
					pAecCtx->GridWeights[j] = pAecGlobal->GridWeights.uCoeff[j];
				}
 			}else{
				pAecCtx->GridWeights[0] = 0x00;    //exp_weight_00to40
				pAecCtx->GridWeights[1] = 0x00;
				pAecCtx->GridWeights[2] = 0x01;
				pAecCtx->GridWeights[3] = 0x00;
				pAecCtx->GridWeights[4] = 0x00;
				
				pAecCtx->GridWeights[5] = 0x00;    //exp_weight_01to41
				pAecCtx->GridWeights[6] = 0x02;
				pAecCtx->GridWeights[7] = 0x02;
				pAecCtx->GridWeights[8] = 0x02;
				pAecCtx->GridWeights[9] = 0x00;
				
				pAecCtx->GridWeights[10] = 0x00;	//exp_weight_02to42
				pAecCtx->GridWeights[11] = 0x04;
				pAecCtx->GridWeights[12] = 0x08;
				pAecCtx->GridWeights[13] = 0x04;
				pAecCtx->GridWeights[14] = 0x00;
				
				pAecCtx->GridWeights[15] = 0x00;	//exp_weight_03to43
				pAecCtx->GridWeights[16] = 0x02;
				pAecCtx->GridWeights[17] = 0x04;
				pAecCtx->GridWeights[18] = 0x02;
				pAecCtx->GridWeights[19] = 0x00;
				
				pAecCtx->GridWeights[20] = 0x00;	//exp_weight_04to44
				pAecCtx->GridWeights[21] = 0x00;
				pAecCtx->GridWeights[22] = 0x00;
				pAecCtx->GridWeights[23] = 0x00;
				pAecCtx->GridWeights[24] = 0x00;
 			}
			if(pAecGlobal->EcmDotEnable >= 1.0){
				pAecCtx->EcmDotEnable = 1.0;
				float EcmTimeDot[ECM_DOT_NO] = {0.0, 0.03, 0.03, 0.03, 0.03, 0.03};
				float EcmGainDot[ECM_DOT_NO] = {1, 1, 6, 6, 8, 8};
				
				for (i=0; i<ECM_DOT_NO; i++){
					if (pAecGlobal->EcmTimeDot.fCoeff[i] <= 0.0f) {
						pAecCtx->EcmTimeDot[i] = EcmTimeDot[i];
					} else {
						pAecCtx->EcmTimeDot[i] = pAecGlobal->EcmTimeDot.fCoeff[i];
					}
				
					if (pAecGlobal->EcmGainDot.fCoeff[i] <= 0.0f) {
						pAecCtx->EcmGainDot[i] = EcmGainDot[i];
					} else {
						pAecCtx->EcmGainDot[i] = pAecGlobal->EcmGainDot.fCoeff[i];
					}
				
					pAecCtx->MaxIntegrationTime = pAecCtx->EcmTimeDot[i];
					pAecCtx->MaxGain			= pAecCtx->EcmGainDot[i];
				}
			    pAecCtx->MinIntegrationTime = pAecCtx->EcmTimeDot[0];
				pAecCtx->MinGain			= pAecCtx->EcmGainDot[0];
				TRACE( AEC_INFO, "EcmTimeDot: %f-%f-%f-%f-%f\n", pAecCtx->EcmTimeDot[0],
				pAecCtx->EcmTimeDot[1], pAecCtx->EcmTimeDot[2], pAecCtx->EcmTimeDot[3], pAecCtx->EcmTimeDot[4]);
				TRACE( AEC_INFO, "EcmGainDot: %f-%f-%f-%f-%f\n", pAecCtx->EcmGainDot[0],
				pAecCtx->EcmGainDot[1], pAecCtx->EcmGainDot[2], pAecCtx->EcmGainDot[3], pAecCtx->EcmGainDot[4]);
				getLimits = BOOL_FALSE;
			}
        }
        else
        {
            pAecCtx->SetPoint       = pConfig->SetPoint;
            pAecCtx->ClmTolerance   = pConfig->ClmTolerance;
            pAecCtx->DampOverStill  = pConfig->DampOverStill;
            pAecCtx->DampUnderStill = pConfig->DampUnderStill;
            pAecCtx->DampOverVideo  = pConfig->DampOverVideo;
            pAecCtx->DampUnderVideo = pConfig->DampUnderVideo;


            pAecCtx->AfpsMaxGain    = pConfig->AfpsMaxGain;
        }
        
        pAecCtx->DampingMode        = pConfig->DampingMode;
        pAecCtx->SemSetPoint        = pConfig->SetPoint;
    }
    else
    {
        pAecCtx->SetPoint       = pConfig->SetPoint;
        pAecCtx->ClmTolerance   = pConfig->ClmTolerance;
        pAecCtx->DampOverStill  = pConfig->DampOverStill;
        pAecCtx->DampUnderStill = pConfig->DampUnderStill;
        pAecCtx->DampOverVideo  = pConfig->DampOverVideo;
        pAecCtx->DampUnderVideo = pConfig->DampUnderVideo;

        pAecCtx->AfpsMaxGain    = pConfig->AfpsMaxGain;

        pAecCtx->SemSetPoint    = (pAecCtx->SemMode != pConfig->SemMode) ? pConfig->SetPoint : pAecCtx->SemSetPoint; // re-init only on mode change
    }

    // get AFPS params & exposure limits from sensor?
    if (pAecCtx->AfpsEnabled)
    {
        result = IsiGetAfpsInfoIss( pAecCtx->hSensor, 0, &pAecCtx->AfpsInfo );
        if (RET_SUCCESS != result)
        {
            if (RET_NOTSUPP != result)
            {
                TRACE( AEC_ERROR, "%s: IsiGetAfpsInfoIss() failed!\n",  __FUNCTION__ );
                return ( result );
            }
            else
            {
                TRACE( AEC_INFO, "%s: IsiGetAfpsInfoIss() not supported \n", __FUNCTION__);
            }
        }
        else
        {
            pAecCtx->AfpsCurrResolution = pAecCtx->AfpsInfo.CurrResolution;

            EcmResolution = pAecCtx->AfpsInfo.AecSlowestResolution;

            if (pAecCtx->AfpsInfo.Stage[0].Resolution != 0)
            {
                TRACE( AEC_DEBUG, "%s: using AfpsInfo for min/max exposure values\n", __FUNCTION__);
                pAecCtx->MinGain            = pAecCtx->AfpsInfo.AecMinGain;
                pAecCtx->MaxGain            = ( pAecCtx->MinGain <= pAecCtx->AfpsMaxGain ) ?
                       MIN( pAecCtx->AfpsMaxGain, pAecCtx->AfpsInfo.AecMaxGain ) : pAecCtx->AfpsInfo.AecMaxGain;
                pAecCtx->MinIntegrationTime = pAecCtx->AfpsInfo.AecMinIntTime;
                pAecCtx->MaxIntegrationTime = pAecCtx->AfpsInfo.AecMaxIntTime;
                getLimits = BOOL_FALSE; // we have limits in place
            }
            else
            {
                TRACE( AEC_INFO, "%s: Afps not supported for this resolution\n", __FUNCTION__);
            }
        }
    }

    result = IsiGetGainLimitsIss( pAecCtx->hSensor, &curMinGain, &curMaxGain );
    if ( result != RET_SUCCESS )
    {
        TRACE( AEC_ERROR, "%s: IsiGetGainLimitsIss() failed!\n",  __FUNCTION__ );
        return ( result );
    }

    result = IsiGetIntegrationTimeLimitsIss( pAecCtx->hSensor, &curMinIntegrationTime, &curMaxIntegrationTime );
    if ( result != RET_SUCCESS )
    {
        TRACE( AEC_ERROR, "%s: IsiGetIntegrationTimeLimitsIss() failed!\n",  __FUNCTION__ );
        return ( result );
    }

    // still no limits available?
    if (getLimits)
    {
        TRACE( AEC_DEBUG, "%s: using current resolution's min/max exposure values\n", __FUNCTION__);
        pAecCtx->MinGain            = curMinGain;
        pAecCtx->MaxGain            = curMaxGain;
        pAecCtx->MinIntegrationTime = curMinIntegrationTime;
        pAecCtx->MaxIntegrationTime = curMaxIntegrationTime;
    }

    // get current resolution?
    //if (EcmResolution == 0)  /*ddl@rock-chips.com*/ 
    {
        uint16_t width = 0U, height = 0U, fps = 0U;
        CamEcmProfileName_t EcmProfileName;

        // get current configured sensor-resolution
        result = IsiGetResolutionIss( pAecCtx->hSensor, &EcmResolution );
        if (RET_SUCCESS != result)
        {
            TRACE( AEC_ERROR, "%s: IsiGetResolutionIss() failed!\n",  __FUNCTION__ );
            return ( result );
        }
        
        // get width, height and fps of current configured sensor-resolution
        result = IsiGetResolutionParam( EcmResolution, &width, &height, &fps );
        if (RET_SUCCESS != result)
        {
            TRACE( AEC_ERROR, "%s: IsiGetResolutionParam() failed! EcmResolution: 0x%x \n",  __FUNCTION__,
                EcmResolution);
            return ( result );
        }

        // convert the resolution parameters into ecm-profile-name
        result = CamCalibDbGetEcmProfileNameByWidthHeightFrameRate( pAecCtx->hCamCalibDb, width, height, fps, &EcmProfileName );
        if ( RET_SUCCESS != result )
        {
            TRACE( AEC_ERROR, "%s: CamCalibDbGetEcmProfileNameByWidthHeightFrameRate() failed!\n",  __FUNCTION__ );
            return ( result );
        }

        // get the ecm-profile by profile-name
    }

    // determine EcmT0/A0 depending on current resolution
    switch (EcmResolution)
    {
        //                                                                               //  T0/A0      T0/A0      T0/A0
        //                                                                               //  fast       normal     slow
        // OV14825
        case ISI_RES_4416_3312P7:
        case ISI_RES_4416_3312P15:
        case ISI_RES_4416_3312P30:
            pAecCtx->EcmT0fac = 3.0f; pAecCtx->EcmA0 = 0.80f; break; // 1.0/0.90   3.0/0.80   5.0/0.70
            
        case ISI_RES_TV1080P5 : pAecCtx->EcmT0fac = 2.0f; pAecCtx->EcmA0 = 0.90f; break; // 1.0/1.00   2.0/0.90   4.0/0.90
        case ISI_RES_TV1080P10: pAecCtx->EcmT0fac = 1.0f; pAecCtx->EcmA0 = 1.00f; break; // 1.0/2.00   1.0/1.00   2.0/1.00
//        case ISI_RES_TV1080P15: pAecCtx->EcmT0fac = 1.0f; pAecCtx->EcmA0 = 1.00f; break; // 1.0/2.00   1.0/1.00   2.0/1.00
        // OV2715 slow
        case ISI_RES_TV1080P6 : pAecCtx->EcmT0fac = 1.0f; pAecCtx->EcmA0 = 0.6f; break; //  nn/nn      nn/nn     1.0/0.78
        case ISI_RES_TV1080P12: pAecCtx->EcmT0fac = 1.0f; pAecCtx->EcmA0 = 0.8f; break; //  nn/nn      nn/nn     1.0/1.08
//        case ISI_RES_TV1080P24: pAecCtx->EcmT0fac = 1.0f; pAecCtx->EcmA0 = 1.0f; break; //  nn/nn      nn/nn     1.0/1.80
        // OV2715 slow with min. 15fps
        case ISI_RES_TV1080P15: pAecCtx->EcmT0fac = 1.0f; pAecCtx->EcmA0 = 0.6f; break; //  nn/nn      nn/nn     1.0/0.6
        case ISI_RES_TV1080P20: pAecCtx->EcmT0fac = 1.0f; pAecCtx->EcmA0 = 0.8f; break; //  nn/nn      nn/nn     1.0/1.8
        case ISI_RES_TV1080P24: pAecCtx->EcmT0fac = 1.0f; pAecCtx->EcmA0 = 1.0f; break; //  nn/nn      nn/nn     1.0/1.0
        case ISI_RES_TV1080P30: pAecCtx->EcmT0fac = 1.0f; pAecCtx->EcmA0 = 1.0f; break; //  nn/nn      nn/nn     1.0/1.0

        case ISI_RES_VGAP5:
        case ISI_RES_VGAP10:
        case ISI_RES_VGAP15:
        case ISI_RES_VGAP20:
        case ISI_RES_VGAP30:
        case ISI_RES_VGAP60:
        case ISI_RES_VGAP120:
	case ISI_RES_3120_3120P30:
            pAecCtx->EcmT0fac = 1.0f; pAecCtx->EcmA0 = 1.0f; break; //  nn/nn      nn/nn     1.0/1.0

        case ISI_RES_SVGAP5:
        case ISI_RES_SVGAP10:
        case ISI_RES_SVGAP15:
        case ISI_RES_SVGAP20:
        case ISI_RES_SVGAP30:
        case ISI_RES_SVGAP60:
        case ISI_RES_SVGAP120:
            pAecCtx->EcmT0fac = 1.0f; pAecCtx->EcmA0 = 1.0f; break; //  nn/nn      nn/nn     1.0/1.0

		case ISI_RES_1280_960P10:
		case ISI_RES_1280_960P15:
		case ISI_RES_1280_960P20:
		case ISI_RES_1280_960P25:
		case ISI_RES_1280_960P30:
			pAecCtx->EcmT0fac = 1.0f; pAecCtx->EcmA0 = 1.0f; break; //  nn/nn      nn/nn     1.0/1.0
		
        case ISI_RES_1296_972P7:
        case ISI_RES_1296_972P10:			
        case ISI_RES_1296_972P15:
        case ISI_RES_1296_972P20:
        case ISI_RES_1296_972P25:			
        case ISI_RES_1296_972P30:
            pAecCtx->EcmT0fac = 1.0f; pAecCtx->EcmA0 = 1.0f; break; //  nn/nn      nn/nn     1.0/1.0

        case ISI_RES_2592_1944P5:
        case ISI_RES_2592_1944P7:
        case ISI_RES_2592_1944P10:
        case ISI_RES_2592_1944P15:
        case ISI_RES_2592_1944P20:
        case ISI_RES_2592_1944P25:
        case ISI_RES_2592_1944P30:        
        case ISI_RES_3264_2448P7: 
        case ISI_RES_3264_2448P10:
        case ISI_RES_3264_2448P15:
        case ISI_RES_3264_2448P20:
        case ISI_RES_3264_2448P25:
        case ISI_RES_3264_2448P30:
        {
            pAecCtx->EcmT0fac = floorf(curMaxIntegrationTime/pAecCtx->EcmTflicker)*ISI_FPS_GET(EcmResolution)/60; 
            pAecCtx->EcmA0 = 1.0f;
            break;
        }

        case ISI_RES_1632_1224P7: 
        case ISI_RES_1632_1224P10:
        case ISI_RES_1632_1224P15:
        case ISI_RES_1632_1224P20:
        case ISI_RES_1632_1224P25:
        case ISI_RES_1632_1224P30:
            pAecCtx->EcmT0fac = floorf(curMaxIntegrationTime/pAecCtx->EcmTflicker)*ISI_FPS_GET(EcmResolution)/60; 
            pAecCtx->EcmA0 = 1.0f; 
            break; 
        case ISI_RES_1600_1200P30:
        case ISI_RES_1600_1200P20:
        case ISI_RES_1600_1200P15:
        case ISI_RES_1600_1200P10:
        case ISI_RES_1600_1200P7:
            pAecCtx->EcmT0fac = floorf(curMaxIntegrationTime/pAecCtx->EcmTflicker)*ISI_FPS_GET(EcmResolution)/60; 
            pAecCtx->EcmA0 = 1.0f; 
            break; //  nn/nn      nn/nn     1.0/1.0
		case ISI_RES_4224_3136P4:
        case ISI_RES_4224_3136P7:
        case ISI_RES_4224_3136P10:
        case ISI_RES_4224_3136P15: 
        case ISI_RES_4224_3136P20:
        case ISI_RES_4224_3136P25:
        case ISI_RES_4224_3136P30: 
        {
            pAecCtx->EcmT0fac = floorf(curMaxIntegrationTime/pAecCtx->EcmTflicker)*ISI_FPS_GET(EcmResolution)/60; 
            pAecCtx->EcmA0 = 1.0f;
            break;
        }
        case ISI_RES_2688_1520P7:
        case ISI_RES_2688_1520P10:
        case ISI_RES_2688_1520P15:
        case ISI_RES_2688_1520P20:
        case ISI_RES_2688_1520P25:
		case ISI_RES_2688_1520P30:

        case ISI_RES_672_376P60:
        case ISI_RES_672_376P30:
        case ISI_RES_672_376P25:
		case ISI_RES_672_376P20:
		case ISI_RES_672_376P15:
		case ISI_RES_672_376P10:
        {
            pAecCtx->EcmT0fac = floorf(curMaxIntegrationTime/pAecCtx->EcmTflicker)*ISI_FPS_GET(EcmResolution)/60; 
            pAecCtx->EcmA0 = 1.0f;
            break;
        }

        case ISI_RES_2112_1568P7:
        case ISI_RES_2112_1568P10:
        case ISI_RES_2112_1568P15: 
        case ISI_RES_2112_1568P20:
        case ISI_RES_2112_1568P25:
        case ISI_RES_2112_1568P30: 
        case ISI_RES_2112_1568P40:
        case ISI_RES_2112_1568P50:
        case ISI_RES_2112_1568P60: 
        case ISI_RES_4224_3120P15:
        case ISI_RES_3120_3120P15:
        case ISI_RES_3120_3120P25:
        case ISI_RES_2112_1560P30:
        {
            pAecCtx->EcmT0fac = floorf(curMaxIntegrationTime/pAecCtx->EcmTflicker)*ISI_FPS_GET(EcmResolution)/60; 
            pAecCtx->EcmA0 = 1.0f;
            break;
        }
        case ISI_RES_2208_1656P7:
        case ISI_RES_2208_1656P15:
        case ISI_RES_2208_1656P30:
            pAecCtx->EcmT0fac = 1.0f; pAecCtx->EcmA0 = 1.0f; break; //  nn/nn      nn/nn     1.0/1.0

        case ISI_RES_4208_3120P4:
        case ISI_RES_4208_3120P7:
        case ISI_RES_4208_3120P10:
        case ISI_RES_4208_3120P15: 
        case ISI_RES_4208_3120P20: 
        case ISI_RES_4208_3120P25: 
        case ISI_RES_4208_3120P30: 
        {
            pAecCtx->EcmT0fac = floorf(curMaxIntegrationTime/pAecCtx->EcmTflicker)*ISI_FPS_GET(EcmResolution)/60; 
            pAecCtx->EcmA0 = 1.0f;
            break;
        }

            
        case ISI_RES_2104_1560P7:                 
        case ISI_RES_2104_1560P10:                   
        case ISI_RES_2104_1560P15:                   
        case ISI_RES_2104_1560P20:                   
        case ISI_RES_2104_1560P25:                   
        case ISI_RES_2104_1560P30:                  
        case ISI_RES_2104_1560P40:                  
        case ISI_RES_2104_1560P50:                   
        case ISI_RES_2104_1560P60:                   
        {
            pAecCtx->EcmT0fac = floorf(curMaxIntegrationTime/pAecCtx->EcmTflicker)*ISI_FPS_GET(EcmResolution)/60; 
            pAecCtx->EcmA0 = 1.0f;
            break;
        }

		case ISI_RES_1640_1232P10:
		case ISI_RES_1640_1232P15:
		case ISI_RES_1640_1232P20:
		case ISI_RES_1640_1232P25:
		case ISI_RES_1640_1232P30:
		{
            pAecCtx->EcmT0fac = floorf(curMaxIntegrationTime/pAecCtx->EcmTflicker)*ISI_FPS_GET(EcmResolution)/60;
            pAecCtx->EcmA0 = 1.0f;
            break;
		}

		case ISI_RES_3280_2464P7:
		case ISI_RES_3280_2464P15:
		case ISI_RES_3280_2464P20:
		case ISI_RES_3280_2464P25:
		case ISI_RES_3280_2464P30:
		{
            pAecCtx->EcmT0fac = floorf(curMaxIntegrationTime/pAecCtx->EcmTflicker)*ISI_FPS_GET(EcmResolution)/60;
            pAecCtx->EcmA0 = 1.0f;
            break;
		}

        // oops...
        default:
        {
            char *szResName = "";
            (void)IsiGetResolutionName(EcmResolution, &szResName);
            TRACE( AEC_ERROR, "%s: Invalid EcmResolution %s(0x%08x)!\n",  __FUNCTION__, szResName, EcmResolution );
            return ( RET_OUTOFRANGE );
        }
    }

    pAecCtx->EcmT0 = pAecCtx->EcmT0fac * pAecCtx->EcmTflicker;

	if(pAecCtx->EcmT0 > pAecCtx->MaxIntegrationTime){
		pAecCtx->EcmT0 = pAecCtx->MaxIntegrationTime;
	}

    // calc exposure limits
    pAecCtx->MinExposure = pAecCtx->MinGain * pAecCtx->MinIntegrationTime;
    pAecCtx->MaxExposure = pAecCtx->MaxGain * pAecCtx->MaxIntegrationTime;

    // calc initial exposure if appropriate
    if (isReConfig == BOOL_FALSE)
    {
        // calc initial exposure...
        if (getLimits)
        {
            // ...in normal mode
            pAecCtx->StartExposure = ( pAecCtx->MinExposure + pAecCtx->MaxExposure ) / 2.0f;
        }
        else
        {
            // ...in AFPS mode
            TRACE( AEC_DEBUG, "%s: using current Afps resolution data for Start-Exposure calculation\n", __FUNCTION__ );
            pAecCtx->StartExposure = ( (pAecCtx->AfpsInfo.AecMinGain * pAecCtx->AfpsInfo.CurrMinIntTime) + (pAecCtx->AfpsInfo.AecMaxGain * pAecCtx->AfpsInfo.CurrMaxIntTime) ) / 2.0f;
        }

        // ...and finally set it
        pAecCtx->Exposure = pAecCtx->StartExposure;
        pAecCtx->EcmOldAlpha = 0.0f; //forces ECM to operate
    }

    TRACE( AEC_DEBUG, "%s: %s-Exposure: %f Min-Exposure: %f Max-Exposure: %f\n", __FUNCTION__, (isReConfig == BOOL_FALSE) ? "Start" : "Old", pAecCtx->Exposure, pAecCtx->MinExposure, pAecCtx->MaxExposure );
    TRACE( AEC_DEBUG, "%s: EcmT0fac/Tflicker: %f / %f\n", __FUNCTION__, pAecCtx->EcmT0fac, pAecCtx->EcmTflicker );
    TRACE( AEC_DEBUG, "%s: EcmT0/A0: %f / %f\n", __FUNCTION__, pAecCtx->EcmT0, pAecCtx->EcmA0 );

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AecSensorControl()
 *****************************************************************************/
static RESULT AecSensorControl
(
    AecContext_t        *pAecCtx,
    float               NewExposure,
    float               SplitGain,
    float               SplitIntegrationTime,
    uint32_t            NewResolution,
    uint32_t            *pNumFramesToSkip // may be NULL
)
{
    RESULT result = RET_SUCCESS;

    uint8_t NumberOfFramesToSkip = 0;
    float SetGain                = 0.0f;
    float SetIntegrationTime     = 0.0f;
    float SetExposure            = 0.0f;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    DCT_ASSERT(pAecCtx != NULL);

    // feed exposure to sensor driver
    result = IsiExposureControlIss( pAecCtx->hSensor, SplitGain, SplitIntegrationTime, &NumberOfFramesToSkip, &SetGain, &SetIntegrationTime );
    if (pNumFramesToSkip != NULL)
    {
        *pNumFramesToSkip = NumberOfFramesToSkip;
    }
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }
    SetExposure = SetGain * SetIntegrationTime;
    TRACE( AEC_DEBUG, "%s: left(%p) Exp(%f, %f) New: %f Set: %f \n", __FUNCTION__, pAecCtx->hSensor, pAecCtx->MinExposure, pAecCtx->MaxExposure, NewExposure, SetExposure );

    // feed exposure to sub sensor driver
    if ( pAecCtx->hSubSensor != NULL )
    {
        uint8_t NumberOfFramesToSkip2 = 0;
        float SetGain2                = 0.0f;
        float SetIntegrationTime2     = 0.0f;
        float SetExposure2            = 0.0f;
        result = IsiExposureControlIss( pAecCtx->hSubSensor, SplitGain, SplitIntegrationTime, &NumberOfFramesToSkip2, &SetGain2, &SetIntegrationTime2 );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
        SetExposure2 = SetGain2 * SetIntegrationTime2;
        TRACE( AEC_DEBUG, "%s: right(%p) Exp(%f, %f) New: %f Set: %f \n", __FUNCTION__, pAecCtx->hSubSensor, pAecCtx->MinExposure, pAecCtx->MaxExposure, NewExposure, SetExposure2 );
    }

    // store the exposure...
    if ( NewResolution == 0 )
    {
        // ...set by sensor driver
        pAecCtx->Exposure        = SetExposure;
        pAecCtx->Gain            = SetGain;
        pAecCtx->IntegrationTime = SetIntegrationTime;
    }
    else
    {
        // ...calculated by ECM; will be set to values set by sensor on resolution change later on
        pAecCtx->Exposure        = NewExposure;
        pAecCtx->Gain            = SplitGain;
        pAecCtx->IntegrationTime = SplitIntegrationTime;
    }

    // finally trigger external resolution change request
    if ( NewResolution != 0 )
    {
        if ( pAecCtx->pResChangeCbFunc != NULL )
        {
            TRACE( AEC_DEBUG, "ResChange requested -> calling pResChangeCbFunc()\n" );
            pAecCtx->pResChangeCbFunc( pAecCtx->pResChangeCbContext, NewResolution );
        }
    }

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of AEC API Functions
 *****************************************************************************/

RESULT AecSetMeanLumaGridWeights
(
    AecHandle_t handle,
    const unsigned char  *pWeights
)
{
    RESULT result = RET_SUCCESS;

    AecContext_t *pAecCtx = (AecContext_t *)handle;
    unsigned char i;
    
    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( pAecCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if (NULL == pWeights)
    {
        return ( RET_NULL_POINTER );
    }

    if (AEC_STATE_RUNNING == pAecCtx->state) 
    {
        TRACE( AEC_ERROR, "%s: pAecCtx->state(0x%x) wrong state\n", __FUNCTION__,pAecCtx->state );
        return ( RET_WRONG_STATE );
    }

    for (i=0; i<CAMERIC_ISP_EXP_GRID_ITEMS; i++)
        pAecCtx->GridWeights[i] = pWeights[i];
    
    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);
    return result;
}


/******************************************************************************
 * AecInit()
 *****************************************************************************/
RESULT AecInit
(
    AecInstanceConfig_t *pInstConfig
)
{
    AecContext_t *pAecCtx;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( NULL == pInstConfig )
    {
        return ( RET_INVALID_PARM );
    }

    // allocate auto exposure control context
    pAecCtx = ( AecContext_t *)malloc( sizeof(AecContext_t) );
    if ( NULL == pAecCtx )
    {
        TRACE( AEC_ERROR, "%s: Can't allocate AEC context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }

    // pre-initialize context
    MEMSET( pAecCtx, 0, sizeof(*pAecCtx) );
    pAecCtx->pResChangeCbFunc    = pInstConfig->pResChangeCbFunc;
    pAecCtx->pResChangeCbContext = pInstConfig->pResChangeCbContext;
    pAecCtx->state = AEC_STATE_INITIALIZED;

    // return handle
    pInstConfig->hAec = (AecHandle_t)pAecCtx;

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AecRelease()
 *****************************************************************************/
RESULT AecRelease
(
    AecHandle_t handle
)
{
    AecContext_t *pAecCtx = (AecContext_t *)handle;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( NULL == pAecCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    // check state
    if ( (AEC_STATE_RUNNING == pAecCtx->state)
            || (AEC_STATE_LOCKED == pAecCtx->state) )
    {
        return ( RET_BUSY );
    }

    // free context
    MEMSET( pAecCtx, 0, sizeof(AecContext_t) );
    free ( pAecCtx );

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AecConfigure()
 *****************************************************************************/
RESULT AecConfigure
(
    AecHandle_t handle,
    AecConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    AecContext_t *pAecCtx = (AecContext_t *)handle;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( NULL == pAecCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if (NULL == pConfig)
    {
        return ( RET_NULL_POINTER );
    }

    if (NULL == pConfig->hSensor)
    {
        return ( RET_INVALID_PARM );
    }

    if ( (AEC_STATE_INITIALIZED != pAecCtx->state)
            && (AEC_STATE_STOPPED != pAecCtx->state) )
    {
        return ( RET_WRONG_STATE );
    }

    // update settings
    result = AecUpdateConfig( pAecCtx, pConfig, BOOL_FALSE );
    if ( result != RET_SUCCESS )
    {
        TRACE( AEC_ERROR, "%s: AecUpdateConfig() failed!\n", __FUNCTION__ );
        return ( result );
    }

    // feed current exposure to sensor driver
    {
        float NewExposure          = pAecCtx->Exposure;
        float SplitGain            = 0;
        float SplitIntegrationTime = 0;

        // 3.) split exposure into gain & integration time values
        result = EcmExecute( pAecCtx, NewExposure, &SplitGain, &SplitIntegrationTime );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }

        // 5.) feed new exposure & resolution to sensor
        result = AecSensorControl( pAecCtx, NewExposure, SplitGain, SplitIntegrationTime, 0, NULL );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
    }

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/******************************************************************************
 * AecReConfigure()
 *****************************************************************************/
RESULT AecReConfigure
(
    AecHandle_t handle,
    AecConfig_t *pConfig,
    uint32_t    *pNumFramesToSkip
)
{
    RESULT result = RET_SUCCESS;

    AecContext_t *pAecCtx = (AecContext_t *)handle;

    float CurGain;
    float CurIntegrationTime;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( pAecCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if (NULL == pConfig)
    {
        return ( RET_NULL_POINTER );
    }

    if ( (AEC_STATE_LOCKED != pAecCtx->state)
            && (AEC_STATE_RUNNING != pAecCtx->state)
            && (AEC_STATE_STOPPED != pAecCtx->state) )
    {
        TRACE( AEC_ERROR, "%s: wrong state\n", __FUNCTION__ );
        return ( RET_WRONG_STATE );
    }

    // update context
    result = AecUpdateConfig( pAecCtx, pConfig, BOOL_TRUE );
    if ( result != RET_SUCCESS )
    {
        TRACE( AEC_ERROR, "%s: AecUpdateConfig() failed!\n", __FUNCTION__ );
        return ( result );
    }

    // feed current exposure to sensor driver
    if ( AEC_STATE_STOPPED != pAecCtx->state )
    {
        float NewExposure          = pAecCtx->Exposure;
        float SplitGain            = 0;
        float SplitIntegrationTime = 0;
		float TolerancePercent = 0;
		float AePointLel1,AePointLel2,AePointLel3,AePointLel4,AePointLel5;
	    CamCalibAecGlobal_t *pAecGlobal;
		
        result = CamCalibDbGetAecGlobal( pAecCtx->hCamCalibDb, &pAecGlobal );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
		
		result = IsiGetGainLimitsIss(pAecCtx->hSensor,&pAecCtx->MinGain,&pAecCtx->MaxGain);
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }

		TolerancePercent = pAecGlobal->ClmTolerance/100.0;
	
		#if 0
		AePointLel3 = pAecGlobal->SetPoint;
		AePointLel4 = (AePointLel3/(1-TolerancePercent))+pAecGlobal->ClmTolerance*1.5;
		AePointLel5 = (AePointLel4/(1-TolerancePercent))+pAecGlobal->ClmTolerance*1.5;		
		AePointLel2 = (AePointLel3/(1+TolerancePercent))-pAecGlobal->ClmTolerance*1.5;
		if(AePointLel2 < 0)
			AePointLel2 = 0;
		AePointLel1 = (AePointLel2/(1+TolerancePercent))-pAecGlobal->ClmTolerance*1.5;
		if(AePointLel1 < 0)
			AePointLel1 = 0; 			

		if(abs(pConfig->SetPoint-AePointLel1)<0.001)	
		{
			pAecCtx->MaxGain = pAecCtx->MaxGain*0.6;
			pAecCtx->MaxExposure = pAecCtx->MaxGain * pAecCtx->MaxIntegrationTime;
		}
		else if(abs(pConfig->SetPoint-AePointLel2)<0.001)
		{
			pAecCtx->MaxGain = pAecCtx->MaxGain*0.7;
			pAecCtx->MaxExposure = pAecCtx->MaxGain * pAecCtx->MaxIntegrationTime;
		}
		else if(abs(pConfig->SetPoint-AePointLel3)<0.001)
		{
			pAecCtx->MaxGain = pAecCtx->MaxGain*0.8;
			pAecCtx->MaxExposure = pAecCtx->MaxGain * pAecCtx->MaxIntegrationTime;
		}
		else if(abs(pConfig->SetPoint-AePointLel4)<0.001)
		{	 
			pAecCtx->MaxGain = pAecCtx->MaxGain*0.9;
			pAecCtx->MaxExposure = pAecCtx->MaxGain * pAecCtx->MaxIntegrationTime;
			
		}else if(abs(pConfig->SetPoint-AePointLel5)<0.001){
			pAecCtx->MaxGain = pAecCtx->MaxGain;
			pAecCtx->MaxExposure = pAecCtx->MaxGain * pAecCtx->MaxIntegrationTime;
		}
		#endif
			pAecCtx->MaxGain = pAecCtx->MaxGain;
			pAecCtx->MaxExposure = pAecCtx->MaxGain * pAecCtx->MaxIntegrationTime;

        // 3.) split exposure into gain & integration time values
        //result = EcmExecute( pAecCtx, NewExposure, &SplitGain, &SplitIntegrationTime );
        /* ddl@rock-chips.com */
        result = EcmExecuteDirect( pAecCtx, NewExposure, &SplitGain, &SplitIntegrationTime );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }

        // 5.) feed new exposure & resolution to sensor
        result = AecSensorControl( pAecCtx, NewExposure, SplitGain, SplitIntegrationTime, 0, pNumFramesToSkip );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
    }

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AecSemExecute()
 *****************************************************************************/
RESULT AecSemExecute
(
    AecHandle_t         handle,
    CamerIcMeanLuma_t   luma
)
{
    AecContext_t *pAecCtx = (AecContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( pAecCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( luma == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    // copy luminance matrix
    MEMCPY( pAecCtx->Luma, luma, sizeof( CamerIcMeanLuma_t ) );

    if ( pAecCtx->state == AEC_STATE_RUNNING )
    {
        if (  (pAecCtx->SemMode > AEC_SCENE_EVALUATION_INVALID)
                && (pAecCtx->SemMode < AEC_SCENE_EVALUATION_MAX) )
        {
            pAecCtx->MeanLuma = AecMeanLuma( pAecCtx, luma );
            if ( pAecCtx->MeanLuma == 0.0f )
            {
                return ( RET_OUTOFRANGE );
            }
        }
        else
        {
            return ( RET_NOTSUPP );
        }

        if ( pAecCtx->SemMode == AEC_SCENE_EVALUATION_ADAPTIVE )
        {
            result = AdaptSemExecute( pAecCtx, luma );
            RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
        }
        else if ( pAecCtx->SemMode == AEC_SCENE_EVALUATION_FIX )
        {
            result = SemExecute( pAecCtx, luma );
            RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
        }
        else
        {
            // AEC_SCENE_EVALUATION_DISABLED
            result = RET_SUCCESS;
        }

        TRACE( AEC_DEBUG, "SP: %f/%f, ML: %f\n", pAecCtx->SetPoint, pAecCtx->SemSetPoint, pAecCtx->MeanLuma );
    }

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AecClmExecute()
 *****************************************************************************/
RESULT AecClmExecute
(
    AecHandle_t         handle,
    CamerIcHistBins_t   bins
)
{
    AecContext_t *pAecCtx = (AecContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( pAecCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( bins == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    // copy histogram
    MEMCPY( pAecCtx->Histogram, bins, sizeof( CamerIcHistBins_t ) );

    if ( pAecCtx->state == AEC_STATE_RUNNING )
    {
        float NewExposure           = 0.0f;
        float SplitGain             = 0.0f;
        float SplitIntegrationTime  = 0.0f;
        uint32_t NewResolution      = 0;


        // 1.) call loop module
        if ( (pAecCtx->SemMode == AEC_SCENE_EVALUATION_ADAPTIVE)
                || ( pAecCtx->SemMode == AEC_SCENE_EVALUATION_FIX ) )
        {
            // take setpoint from scene evaluation
            result = ClmExecute( pAecCtx, pAecCtx->SemSetPoint, bins, &NewExposure );
            RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
        }
        else if ( pAecCtx->SemMode == AEC_SCENE_EVALUATION_DISABLED )
        {
            // take setpoint from static configured setpoint
            result = ClmExecute( pAecCtx, pAecCtx->SetPoint, bins, &NewExposure );
            RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
        }
        else
        {
            return ( RET_NOTSUPP );
        }

        // 2.) damping
        if ( pAecCtx->MeanLuma > pAecCtx->SemSetPoint )
        {
            float DampOver = 0.0f;
            
            switch (pAecCtx->DampingMode)
            {
                case AEC_DAMPING_MODE_STILL_IMAGE:
                    {
                        DampOver = pAecCtx->DampOverStill;
                        break;
                    }
                case AEC_DAMPING_MODE_VIDEO:
                    {
                        DampOver = pAecCtx->DampOverVideo;
                        break;
                    }
                default:
                    {
                        return ( RET_NOTSUPP );
                    }
            }
            NewExposure = NewExposure * (1.0f - DampOver) + (pAecCtx->Exposure * DampOver);
        }
        else
        {
            float DampUnder = 0.0f;
            
            switch (pAecCtx->DampingMode)
            {
                case AEC_DAMPING_MODE_STILL_IMAGE:
                    {
                        DampUnder = pAecCtx->DampUnderStill;
                        break;
                    }
                case AEC_DAMPING_MODE_VIDEO:
                    {
                        DampUnder = pAecCtx->DampUnderVideo;
                        break;
                    }
                default:
                    {
                        return ( RET_NOTSUPP );
                    }
            }
            NewExposure = NewExposure * (1.0f - DampUnder) + (pAecCtx->Exposure * DampUnder);
        }

        // 3.) split exposure into gain & integration time values
        result = EcmExecute( pAecCtx, NewExposure, &SplitGain, &SplitIntegrationTime );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
        TRACE( AEC_INFO, "%s: NewExposure(%f) SplitGain(%f) SplitIntegrationTime(%f) NewResolution(%d) \n", __FUNCTION__,
		 	NewExposure,SplitGain, SplitIntegrationTime, NewResolution);
        // 4.) check for AFPS resolution change request
        result = AfpsExecute( pAecCtx, SplitIntegrationTime, &NewResolution );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
		TRACE( AEC_INFO, "%s: after APFS NewExposure(%f) SplitGain(%f) SplitIntegrationTime(%f) NewResolution(%d) \n", __FUNCTION__,
		 	NewExposure,SplitGain, SplitIntegrationTime, NewResolution);
		if(NewExposure != pAecCtx->Exposure ||  NewResolution != 0){
	        // 5.) feed new exposure & resolution to sensor
	        result = AecSensorControl( pAecCtx, NewExposure, SplitGain, SplitIntegrationTime, NewResolution, NULL );
	        if ( result != RET_SUCCESS )
	        {
	            return ( result );
	        }
		}
    }

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AecSettled()
 *****************************************************************************/
RESULT AecSettled
(
    AecHandle_t handle,
    bool_t      *pSettled
)
{
    AecContext_t *pAecCtx = (AecContext_t *)handle;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( pAecCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pSettled == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    *pSettled = ( ( (pAecCtx->LumaDeviation > 0.0f) && (pAecCtx->LumaDeviation < 0.36) )
            || ( ( pAecCtx->MaxExposure - pAecCtx->Exposure ) <= ( pAecCtx->EcmTflicker * pAecCtx->MaxGain) ) ) ? BOOL_TRUE : BOOL_FALSE;

    TRACE( AEC_DEBUG, "%s: dL: %f -> %ssettled\n", __FUNCTION__, pAecCtx->LumaDeviation, *pSettled == BOOL_TRUE ? "" : "not " );

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AecGetCurrentGain()
 *****************************************************************************/
RESULT AecGetCurrentGain
(
    AecHandle_t handle,
    float       *pGain
)
{
    AecContext_t *pAecCtx = (AecContext_t *)handle;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( pAecCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pGain == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    if ( ( pAecCtx->state == AEC_STATE_RUNNING )
                    || ( pAecCtx->state == AEC_STATE_LOCKED ) )
    {
        *pGain = pAecCtx->Gain;
    }
    else
    {
        RESULT result = IsiGetGainIss( pAecCtx->hSensor, pGain );
        if ( result != RET_SUCCESS )
        {
            TRACE( AEC_ERROR, "%s: IsiGetGainIss() failed!\n",  __FUNCTION__ );
            return ( result );
        }
    }

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AecGetCurrentIntegrationTime()
 *****************************************************************************/
RESULT AecGetCurrentIntegrationTime
(
    AecHandle_t handle,
    float       *pIntegrationTime
)
{
    AecContext_t *pAecCtx = (AecContext_t *)handle;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( pAecCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIntegrationTime == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    if ( ( pAecCtx->state == AEC_STATE_RUNNING )
                    || ( pAecCtx->state == AEC_STATE_LOCKED ) )
    {
        *pIntegrationTime = pAecCtx->IntegrationTime;
    }
    else
    {
        RESULT result = IsiGetIntegrationTimeIss( pAecCtx->hSensor, pIntegrationTime );
        if ( result != RET_SUCCESS )
        {
            TRACE( AEC_ERROR, "%s: IsiGetIntegrationTimeIss() failed!\n",  __FUNCTION__ );
            return ( result );
        }
    }

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AecGetCurrentHistogram()
 *****************************************************************************/
RESULT AecGetCurrentHistogram
(
    AecHandle_t         handle,
    CamerIcHistBins_t   *pHistogram
)
{
    AecContext_t *pAecCtx = (AecContext_t *)handle;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( pAecCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pHistogram == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    MEMCPY( *pHistogram, pAecCtx->Histogram, sizeof( CamerIcHistBins_t ) );

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AecGetCurrentLuminance()
 *****************************************************************************/
RESULT AecGetCurrentLuminance
(
    AecHandle_t         handle,
    CamerIcMeanLuma_t   *pLuma
)
{
    AecContext_t *pAecCtx = (AecContext_t *)handle;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( pAecCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pLuma == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    MEMCPY( *pLuma, pAecCtx->Luma, sizeof( CamerIcMeanLuma_t ) );

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}

/******************************************************************************
 * AecGetClmTolerance()
 *****************************************************************************/
RESULT AecGetClmTolerance
(
    AecHandle_t         handle,
    float   *clmtolerance
)
{
    AecContext_t *pAecCtx = (AecContext_t *)handle;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( pAecCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( clmtolerance == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    *clmtolerance = pAecCtx->ClmTolerance;

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}

RESULT AecSetClmTolerance
(
	AecHandle_t 		handle,
	float	clmtolerance
)
{
	AecContext_t *pAecCtx = (AecContext_t *)handle;

	TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

	// initial checks
	if ( pAecCtx == NULL )
	{
		return ( RET_WRONG_HANDLE );
	}

	pAecCtx->ClmTolerance = clmtolerance;

	TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

	return ( RET_SUCCESS );
}

/******************************************************************************
 * AecGetCurrentObjectRegion()
 *****************************************************************************/
RESULT AecGetCurrentObjectRegion
(
    AecHandle_t         handle,
    CamerIcMeanLuma_t   *pObjectRegion
)
{
    AecContext_t *pAecCtx = (AecContext_t *)handle;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( pAecCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pObjectRegion == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    if ( ( pAecCtx->state == AEC_STATE_RUNNING )
        && ( pAecCtx->SemMode == AEC_SCENE_EVALUATION_ADAPTIVE ) )
    {
        MEMCPY( *pObjectRegion, pAecCtx->SemCtx.asem.ObjectRegion, sizeof( CamerIcMeanLuma_t ) );
    }
    else
    {
        MEMSET( *pObjectRegion, 1, sizeof( CamerIcMeanLuma_t ) );
    }

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AecGetCurrentConfig()
 *****************************************************************************/
RESULT AecGetCurrentConfig
(
    AecHandle_t handle,
    AecConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    AecContext_t *pAecCtx = (AecContext_t *)handle;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( pAecCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if (NULL == pConfig)
    {
        return ( RET_NULL_POINTER );
    }

    // return config
    pConfig->hCamCalibDb        = pAecCtx->hCamCalibDb;
    
    pConfig->SemMode            = pAecCtx->SemMode;
    pConfig->SetPoint           = pAecCtx->SetPoint;
    pConfig->ClmTolerance       = pAecCtx->ClmTolerance;
    pConfig->DampingMode        = pAecCtx->DampingMode;
    pConfig->DampUnderStill     = pAecCtx->DampUnderStill;
    pConfig->DampOverStill      = pAecCtx->DampOverStill;
    pConfig->DampUnderVideo     = pAecCtx->DampUnderVideo;
    pConfig->DampOverVideo      = pAecCtx->DampOverVideo;

    pConfig->AfpsEnabled        = pAecCtx->AfpsEnabled;
    pConfig->AfpsMaxGain        = pAecCtx->AfpsMaxGain;

    pConfig->EcmFlickerSelect   = pAecCtx->EcmFlickerSelect;
    //pConfig->EcmT0              = pAecCtx->EcmT0;
    //pConfig->EcmA0              = pAecCtx->EcmA0;

    pConfig->hSensor            = pAecCtx->hSensor;
    pConfig->hSubSensor         = pAecCtx->hSubSensor;

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/******************************************************************************
 * AecStatus()
 *****************************************************************************/
RESULT AecStatus
(
    AecHandle_t         handle,
    bool_t              *pRunning,
    AecSemMode_t        *pMode,
    float               *pSetPoint,
    float               *pClmTolerance,
    AecDampingMode_t    *pDampingMode,
    float               *pDampOverStill,
    float               *pDampUnderStill,
    float               *pDampOverVideo,
    float               *pDampUnderVideo
)
{
    AecContext_t *pAecCtx = (AecContext_t *)handle;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( pAecCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pRunning == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    *pRunning           = ( ( pAecCtx->state == AEC_STATE_RUNNING )
                                || ( pAecCtx->state == AEC_STATE_LOCKED ) );
    *pMode              = pAecCtx->SemMode;
    *pSetPoint          = pAecCtx->SetPoint;
    *pClmTolerance      = pAecCtx->ClmTolerance;
    *pDampingMode       = pAecCtx->DampingMode;
    *pDampOverStill     = pAecCtx->DampUnderStill;
    *pDampUnderStill    = pAecCtx->DampOverStill;
    *pDampOverVideo     = pAecCtx->DampUnderVideo;
    *pDampUnderVideo    = pAecCtx->DampOverVideo;

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AecStart()
 *****************************************************************************/
RESULT AecStart
(
    AecHandle_t handle
)
{
    AecContext_t *pAecCtx = (AecContext_t *)handle;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( pAecCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (AEC_STATE_RUNNING == pAecCtx->state)
            || (AEC_STATE_LOCKED == pAecCtx->state) )
    {
        return ( RET_WRONG_STATE );
    }

    pAecCtx->state = AEC_STATE_RUNNING;

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AecStop()
 *****************************************************************************/
RESULT AecStop
(
    AecHandle_t handle
)
{
    AecContext_t *pAecCtx = (AecContext_t *)handle;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( pAecCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    // before stopping, unlock the AEC if locked
    if ( AEC_STATE_LOCKED == pAecCtx->state )
    {
        return ( RET_WRONG_STATE );
    }

    pAecCtx->state = AEC_STATE_STOPPED;

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AecReset()
 *****************************************************************************/
RESULT AecReset
(
    AecHandle_t handle
)
{
    AecContext_t *pAecCtx = (AecContext_t *)handle;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( pAecCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if (AEC_STATE_LOCKED == pAecCtx->state)
    {
        return ( RET_WRONG_STATE );
    }

    if ( pAecCtx->hCamCalibDb )
    {
        RESULT result = RET_SUCCESS;

        CamCalibAecGlobal_t *pAecGlobal;
        CamEcmProfile_t     *ppEcmProfile;

        result = CamCalibDbGetAecGlobal( pAecCtx->hCamCalibDb, &pAecGlobal );
        if ( result != RET_SUCCESS )
        {
            TRACE( AEC_ERROR, "%s: Access to CalibDB failed. (%d)\n", __FUNCTION__, result );
            return ( result );
        }
        
        pAecCtx->SetPoint       = pAecGlobal->SetPoint;
        pAecCtx->ClmTolerance   = pAecGlobal->ClmTolerance;
        pAecCtx->DampOverStill  = pAecGlobal->DampOverStill;
        pAecCtx->DampUnderStill = pAecGlobal->DampUnderStill;
        pAecCtx->DampOverVideo  = pAecGlobal->DampOverVideo;
        pAecCtx->DampUnderVideo = pAecGlobal->DampUnderVideo;
    }

    pAecCtx->StartExposure = ( pAecCtx->MinExposure + pAecCtx->MaxExposure ) / 2.0f;
    pAecCtx->Exposure = pAecCtx->StartExposure;
    pAecCtx->EcmOldAlpha = 0.0f; //forces ECM to operate

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AecTryLock()
 *****************************************************************************/
RESULT AecTryLock
(
    AecHandle_t handle
)
{
    AecContext_t *pAecCtx = (AecContext_t *)handle;

    RESULT result = RET_FAILURE;
    bool_t settled = BOOL_FALSE;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( pAecCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (AEC_STATE_RUNNING != pAecCtx->state)
            && (AEC_STATE_LOCKED != pAecCtx->state)
            && (AEC_STATE_STOPPED != pAecCtx->state) )
    {
        return ( RET_WRONG_STATE );
    }

    // check working mode
    if ( AEC_STATE_STOPPED != pAecCtx->state )
    {
        result = AecSettled( handle, &settled );
        if ( (RET_SUCCESS == result) && (BOOL_TRUE == settled) )
        {
            pAecCtx->state = AEC_STATE_LOCKED;
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

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AecUnLock()
 *****************************************************************************/
RESULT AecUnLock
(
    AecHandle_t handle
)
{
    AecContext_t *pAecCtx = (AecContext_t *)handle;

    TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial checks
    if ( pAecCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (AEC_STATE_LOCKED == pAecCtx->state)
            || (AEC_STATE_RUNNING == pAecCtx->state) )
    {
        pAecCtx->state = AEC_STATE_RUNNING;
    }
    else if ( AEC_STATE_STOPPED == pAecCtx->state )
    {
        // do nothing
    }
    else
    {
        return ( RET_WRONG_STATE );
    }

    TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}

RESULT AecGetGridWeights
(
	AecHandle_t handle,
	CamerIcHistWeights_t pWeights
)
{
    RESULT result = RET_SUCCESS;
    uint8_t i;
    AecContext_t *pAecCtx = (AecContext_t *)handle;
    #if 0
	CamCalibAecGlobal_t *pAecGlobal;
	TRACE( AEC_INFO, "%s: (enter)\n", __FUNCTION__);
	result = CamCalibDbGetAecGlobal( pAecCtx->hCamCalibDb, &pAecGlobal );
	if ( result != RET_SUCCESS )
	{
		return ( result );
	}
	MEMCPY(*pWeights, pAecGlobal->GridWeights.uCoeff, sizeof(pAecGlobal->GridWeights.uCoeff));
	#endif

	for(i=0; i<CAMERIC_ISP_EXP_GRID_ITEMS; i++)
		pWeights[i] = pAecCtx->GridWeights[i];
	TRACE( AEC_INFO, "%s: (exit)\n", __FUNCTION__);
	return ( result );
}

