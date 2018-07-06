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
#ifndef __AEC_CTRL_H__
#define __AEC_CTRL_H__

/**
 * @file aec_ctrl.h
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
 * @defgroup AEC Auto Exposure Control
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <isi/isi_iss.h>
#include <isi/isi.h>

#include "aec.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum AecState_e
{
    AEC_STATE_INVALID       = 0,
    AEC_STATE_INITIALIZED   = 1,
    AEC_STATE_STOPPED       = 2,
    AEC_STATE_RUNNING       = 3,
    AEC_STATE_LOCKED        = 4,
    AEC_STATE_MAX
} AecState_t;


typedef struct AecContext_s
{
    AecState_t          state;
//    bool_t              running;

    float               StartExposure;      /**< start exposure value of the ae control system */
    float               Exposure;           /**< current exposure value of the ae control system */
    float               MinExposure;        /**< minmal exposure value (sensor specific) */
    float               MaxExposure;        /**< maximal exposure value (sensor specific) */

    float               MinGain;            /**< minmal exposure value (sensor specific) */
    float               MaxGain;            /**< maximal exposure value (sensor specific) */

    float               MinIntegrationTime; /**< minmal exposure value (sensor specific) */
    float               MaxIntegrationTime; /**< maximal exposure value (sensor specific) */

    /* scene evaluation module */
    AecSemMode_t        SemMode;            /**< scene evaluation mode */

    float               SetPoint;           /**< set point to hit by the ae control system */
    float               SemSetPoint;        /**< set point calculated by scene evaluation module */

    AecDampingMode_t    DampingMode;        /**< damping mode */
    float               DampOverStill;      /**< Damping coefficient for still image mode */
    float               DampUnderStill;     /**< Damping coefficient for still image mode */
    float               DampOverVideo;      /**< Damping coefficient for video mode */
    float               DampUnderVideo;     /**< Damping coefficient for video mode */
    
    float               MeanLuma;           /**< mean luminace calculated */
    float               MeanLumaObject;

    float               d;
    float               z;
    float               m0;

    float               EcmOldAlpha;        /**< for ECM lock range >*/
    float               EcmOldGain;         /**< for ECM lock range >*/
    float               EcmOldTint;         /**< for ECM lock range >*/

    union SemCtx_u
    {
        struct sem_s
        {
            float MeanLumaRegion0;
            float MeanLumaRegion1;
            float MeanLumaRegion2;
            float MeanLumaRegion3;
            float MeanLumaRegion4;
        } sem;

        struct asem_s
        {
            float               OtsuThreshold;  /**< threshold calculated with otsu */

            CamerIcMeanLuma_t   Binary0;        /**< thresholded luma matrix */
            CamerIcMeanLuma_t   Cc0;            /**< matrix of labeled 0-conponents */
            CamerIcMeanLuma_t   Ll0;            /**< matrix of labeled 1-conponents */
            uint32_t            NumCc0;         /**< number of 0-conponents */
            uint32_t            LblBiggestCc0;  /**< label of biggest 0-connected-component */
            float               MaxCenterX0;
            float               MaxCenterY0;

            CamerIcMeanLuma_t   Binary1;        /**< thresholded luma matrix */
            CamerIcMeanLuma_t   Cc1;            /**< matrix of labeled 1-conponents */
            CamerIcMeanLuma_t   Ll1;            /**< matrix of labeled 1-conponents */
            uint32_t            NumCc1;         /**< number of 1-conponents */
            uint32_t            LblBiggestCc1;  /**< label of biggest 1-connected-component */
            float               MaxCenterX1;
            float               MaxCenterY1;

            CamerIcMeanLuma_t   ObjectRegion;   /**< resulting matrix  (0 = Object, 1 = Background ) */
        } asem;

    } SemCtx;

    /* control loop module */
    #define CLM_HIST_NUM_BINS   ( 3 * CAMERIC_ISP_HIST_NUM_BINS )

    float               ClmTolerance;
    float               LumaDeviation;
    uint32_t            ClmHistogram[CLM_HIST_NUM_BINS];
    uint32_t            ClmHistogramSize;
    float               ClmHistogramX[CLM_HIST_NUM_BINS];
    uint32_t            SumHistogram;
    float               MeanHistogram;

	#define ECM_DOT_NO		   (6)
    float               EcmTimeDot[ECM_DOT_NO];
	float               EcmGainDot[ECM_DOT_NO];
	float				EcmDotEnable;

    /* debug output */
    CamerIcHistBins_t   Histogram;
    CamerIcMeanLuma_t   Luma;
    unsigned int        GridWeights[CAMERIC_ISP_EXP_GRID_ITEMS];

    /* ECM module (incl. AFPS) */
    AfpsResChangeCb_t   *pResChangeCbFunc;      /**< callback function to trigger resolution change */
    void                *pResChangeCbContext;   /**< reference to context to pass to callback */
    bool_t              AfpsEnabled;
    IsiAfpsInfo_t       AfpsInfo;
    uint32_t            AfpsCurrResolution;
    float               AfpsMaxGain;

    AecEcmFlickerPeriod_t EcmFlickerSelect;
    float               EcmTflicker;        /**< flicker period */
    float               EcmT0fac;
    float               EcmT0;              /**< start of flicker avoidance */
    float               EcmA0;              /**< linear: slope of gain */

    float               Gain;
    float               IntegrationTime;

    IsiSensorHandle_t   hSensor;            /**< sensor handle on which the ae system is working */
    IsiSensorHandle_t   hSubSensor;         /**< sensor handle on which the ae system is working */

    CamCalibDbHandle_t  hCamCalibDb;        /**< calibration database handle */
} AecContext_t;


#ifdef __cplusplus
}
#endif

/* @} AEC */


#endif /* __AEC_CTRL_H__*/
