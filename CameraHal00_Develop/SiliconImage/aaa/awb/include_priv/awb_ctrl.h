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
#ifndef __AWB_CTRL_H__
#define __AWB_CTRL_H__

/**
 * @file awb.h
 *
 * @brief
 *
 *****************************************************************************/
/**
 * @page module_name_page Module Name
 * Describe here what this module does.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref module_name
 *
 * @defgroup AWB Auto white Balance
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>
#include <common/cam_types.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_drv_api.h>
#include <cameric_drv/cameric_isp_awb_drv_api.h>
#include <cameric_drv/cameric_isp_hist_drv_api.h>

#include <cam_calibdb/cam_calibdb_api.h>

#include "expprior_common.h"
#include "wbgain_common.h"
#include "illuest_common.h"

#ifdef __cplusplus
extern "C"
{
#endif


/**< limits for out of range tests. */
#define DIVMIN     0.00001f  /**< lowest denominator for divisions  */
#define LOGMIN     0.0001f   /**< lowest value for logarithmic calculations */


#define AWB_MAX_ILLUMINATION_PROFILES   32L


/*****************************************************************************/
/**
 * @brief This enum type specifies the different possible states of the AWB.
 *
 */
/*****************************************************************************/
typedef enum AwbState_e
{
    AWB_STATE_INVALID           = 0,        /**< initialization value */
    AWB_STATE_INITIALIZED       = 1,
    AWB_STATE_STOPPED           = 2,
    AWB_STATE_RUNNING           = 3,
    AWB_STATE_LOCKED            = 4,
    AWB_STATE_MAX
} AwbState_t;



/*****************************************************************************/
/**
 * @brief   Context of the AWB module.
 */
/*****************************************************************************/
typedef struct AwbContext_s
{
    AwbState_t                      state;
    AwbMode_t                       Mode;
    uint32_t                        Flags;
    AwbConfig_t                     Config;

    /* handle to access the hardware modules */
    IsiSensorHandle_t               hSensor;                /**< sensor handle */

    CamerIcDrvHandle_t              hCamerIc;
    CamerIcDrvHandle_t              hSubCamerIc;

    bool_t                          damp;

    CamResolutionName_t             ResName;                /**< identifier for accessing resolution depended calibration data */
    int32_t                         ResIdx;                 /**< resolution index */
    CamCalibDbHandle_t              hCamCalibDb;            /**< calibration database handle */

    /* meassured mode */
    CamerIcIspAwbMeasuringMode_t    MeasMode;               /**< specifies the means measuring mode (YCbCr or RGB) */
    CamerIcAwbMeasuringConfig_t     MeasConfig;             /**< measuring config */

    /* configuration data */
    uint32_t                        NumWhitePixelsMax;      /**< maximal number of white pixel */
    uint32_t                        NumWhitePixelsMin;      /**< minimal number of white pixel */

    float                           RegionSizeStart;
    float                           RgProjMaxSky;
    float                           RgProjIndoorMin;
    float                           RgProjOutdoorMin;
    float                           RgProjMax;

	float 							fRgProjALimit;    //oyyf
	float							fRgProjAWeight;		//oyyf
	float 							fRgProjYellowLimit;		//oyyf
	float							fRgProjIllToCwf;		//oyyf
	float							fRgProjIllToCwfWeight;	//oyyf
	

    float                           RegionSizeInc;
    float                           RegionSizeDec;

    /* pointer to calibration data */
    Cam1x1FloatMatrix_t             *pKFactor;              /**< auto white balance constant */
    Cam3x2FloatMatrix_t             *pPcaMatrix;
    Cam3x1FloatMatrix_t             *pSvdMeanValue;
    CamCenterLine_t                 *pCenterLine;           /**< center-line in Rg/Rb-layer */

    CamAwbClipParm_t                *pGainClipCurve;
    CamAwbGlobalFadeParm_t          *pGlobalFadeParam;
    CamAwbFade2Parm_t               *pFadeParam;

    uint32_t                        IlluIdx;                /**< index of start illumination profile */
    int32_t                         NoIlluProfiles;         /**< number of illumination profiles 0..31 */
    CamIlluProfile_t                *pIlluProfiles[AWB_MAX_ILLUMINATION_PROFILES];  /**< array of illumination profile pointer */

    CamCcProfile_t                  *pCcProfiles[AWB_MAX_ILLUMINATION_PROFILES][CAM_NO_CC_PROFILES];
    CamLscProfile_t                 *pLscProfiles[CAM_NO_RESOLUTIONS][AWB_MAX_ILLUMINATION_PROFILES][CAM_NO_LSC_PROFILES];

    /* AWB itermediary values */
    uint32_t                        NoWhitePixel;
    uint32_t                        dNoWhitePixel;
    AwbNumWhitePixelEval_t          WhitePixelEvaluation;
    float                           fStableDeviation;       /**< min deviation in percent to enter stable state */
    float                           fRestartDeviation;      /**< max tolerated deviation in precent for staying in stable state */
    uint32_t                        StableThreshold;
    uint32_t                        RestartThreshold;

    /* input values (stored) (current means last frame) */
    CamerIcAwbMeasuringResult_t     MeasuredMeans;          /**< current measured values from hardware */
    Cam3x3FloatMatrix_t             CtMatrix;               /**< current cross talk matrix from hardware */
    AwbXTalkOffset_t                CtOffset;               /**< current cross talk offset from hardware */

    AwbGains_t                      Gains;                  /**< current gains from hardware */
    float                           SensorGain;             /**< current sensor gain from hardware */
    float                           IntegrationTime;        /**< current intergration time from hardware */

    /* output values */

    /* Exposure Prior Probaility Module */
    ExpPriorCalcEvaluation_t        DoorType;
    float                           AwbIIRDampCoef;
    float                           ExpPriorIn;             /**< exposure prior indoor probability for current frame */
    float                           ExpPriorOut;            /**< exposure prior outdoor probability for current frame */
    AwbExpPriorContext_t            ExpPriorCtx;            /**< exposure context */

    /* White-Point Revert Module */
    AwbComponent_t                  RevertedMeansRgb;

    /* White-Balance Gain Module */
    float                           RgProj;
    WbGainsOverG_t                  WbGainsOverG;
    WbGainsOverG_t                  WbClippedGainsOverG;
    bool_t                          WbGainsOutOfRange;
    bool_t                          RgProjClippedToOutDoorMin;
    AwbGains_t                      WbGains;
    AwbGains_t                      WbDampedGains;
    AwbGains_t                      WbUnDampedGains;
	float							Wb_s;
	float							Wb_s_max1;
	float							Wb_s_max2;
	float							WbBg;
	float							WbRg;
    /* Illumination Estimation */
    int32_t                         D50IlluProfileIdx;
    int32_t                         CwfIlluProfileIdx;        /* ddl@rock-chips.com */
    int32_t                         DominateIlluProfileIdx;
    IlluEstRegion_t                 Region;
    AwbComponent_t                  NormalizedMeansRgb;
    float                           LikeHood[AWB_MAX_ILLUMINATION_PROFILES];
    float                           Weight[AWB_MAX_ILLUMINATION_PROFILES];
    float                           WeightTrans[AWB_MAX_ILLUMINATION_PROFILES];

    /* aCC */
    float                           fSaturation[AWB_MAX_ILLUMINATION_PROFILES]; /**< saturation values of all illu's */
    Cam3x3FloatMatrix_t             CcMatrix[AWB_MAX_ILLUMINATION_PROFILES];    /**< color correction matrices of all illu's */
    Cam3x3FloatMatrix_t             UndampedCcMatrix;                           /**< undamped color correction matrix */
    Cam3x3FloatMatrix_t             DampedCcMatrix;                             /**< damped color correction matrix */

    Cam1x3FloatMatrix_t             CcOffset[AWB_MAX_ILLUMINATION_PROFILES];
    Cam1x3FloatMatrix_t             UndampedCcOffset;           /**< undamped color correction offset */
    Cam1x3FloatMatrix_t             DampedCcOffset;             /**< damped color correction offset */

    /* aLSC */
    float                           fVignetting;                /**< current vignetting */
    CamerIcIspLscSectorConfig_t     SectorConfig;               /**< lsc grid */
    CamLscMatrix_t                  UndampedLscMatrixTable;     /**< undamped lsc matrix */
    CamLscMatrix_t                  DampedLscMatrixTable;       /**< damped lsc matrix */

    /* Whie-Balance Region Adaptation */
    float                           RegionSize;
    CamerIcAwbMeasuringConfig_t     MeasWdw;                   /**< measuring window */

    /* Histogram used for offset scaling*/
    CamerIcHistBins_t               Histogram;
    float                           ScalingThreshold0;
    float                           ScalingThreshold1;
    float                           ScalingFactor;
    float                           DampingFactorScaling;
	float 							MeanHistogram; //oyyf
	/*TemperatureRgLine  Ct=a*Rg+b*/
	float TemRgLine_k;
	float TemRgLine_b;
	bool  TemRgLine_CalFlag;
} AwbContext_t;



#ifdef __cplusplus
}
#endif

/* @} AWB */


#endif /* __AWB_CTRL_H__*/
