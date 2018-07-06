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
 * @file cam_calibdb.h
 *
 * @brief
 *   Internal interface of the CamCalibDb.
 *
 *****************************************************************************/
/**
 *
 * @defgroup cam_calibdb CamCalibDb internal interface
 * @{
 *
 */
#ifndef __CAM_CALIBDB_H__
#define __CAM_CALIBDB_H__

#include <ebase/types.h>
#include <oslayer/oslayer.h>
#include <common/return_codes.h>
#include <common/cam_types.h>
#include <common/list.h>

#include "cam_calibdb_api.h"

#ifdef __cplusplus
extern "C"
{
#endif


/**
 * @brief   Internal context of the Cam-Calibration Database
 *
 * @note
 *
 */
typedef struct CamCalibDbContext_s
{
    creation_date_t             cdate;          /**< creation date */
    creator_name_t              cname;          /**< name of creator */
    creator_version_t           cversion;       /**< version of creation tool (matlab generator) */
    sensor_name_t               sname;          /**< sesor name */
    sensor_sample_id_t          sid;            /**< sensor sample id */
	OTPInfo_t					OTPInfo;		/**< sensor otp info */

    List                        resolution;     /**< list of supported resolutions */
    List                        awb_global;     /**< list of supported awb_globals */
    CamCalibAecGlobal_t         *pAecGlobal;    /**< AEC global settings */
    List                        ecm_profile;    /**< list of supported ECM profiles */
    List                        illumination;   /**< list of supported illuminations */
    List                        lsc_profile;    /**< list of supported LSC profiles */
    List                        cc_profile;     /**< list of supported CC profiles */
    List                        bls_profile;    /**< list of supported BLS profiles */
    List                        cac_profile;    /**< list of supported CAC profiles */
    List                        dpf_profile;    /**< list of supported DPF profiles */
    List                        dpcc_profile;   /**< list of supported DPCC profiles */
	CamCalibGammaOut_t 			*pGammaOut;		/**< Gamma out settings */
    CamCalibSystemData_t        system;
} CamCalibDbContext_t;


#ifdef __cplusplus
}
#endif


/* @} cam_calibdb */

#endif /* __CAM_CALIBDB_H__ */

