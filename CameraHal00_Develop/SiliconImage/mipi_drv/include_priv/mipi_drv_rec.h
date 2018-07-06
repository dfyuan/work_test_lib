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
 * @file mipi_drv_rec.h
 *
 * @brief   Definition of MIPI REC driver API.
 *
 *****************************************************************************/

#ifndef __MIPI_DRV_REC_H__
#define __MIPI_DRV_REC_H__

#include <ebase/types.h>
#include <common/return_codes.h>

#include "mipi_drv_common.h"


/*****************************************************************************/
/**
 * @brief Handle to MIPI REC driver context.
 *
 *****************************************************************************/
typedef struct MipiRecContext_s *MipiRecHandle_t;


/*****************************************************************************/
/**
 * @brief   Initializes MIPI REC according to given settings
 *
 * @param   pRecHandle          Reference to MIPI REC context handle. A valid handle
 *                              will be returned upon successfull initialization,
 *                              otherwise the handle is undefined.
 * @param   pMipiDrvConfig      Config to use for initialization..
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         MIPI REC successfully initialized.
 * @retval  RET_FAILURE et al.  Error initializing MIPI REC.
 *
 *****************************************************************************/
extern RESULT MipiRecInit
(
    MipiRecHandle_t     *pRecHandle,
    MipiDrvConfig_t     *pMipiDrvConfig
);


/*****************************************************************************/
/**
 * @brief   Shuts down MIPI receiver.
 *
 * @param   RecHandle           MIPI REC handle as returned by @ref MipiRecInit.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         MIPI REC successfully shutdown.
 * @retval  RET_FAILURE et al.  Error shutting down MIPI REC.
 *
 *****************************************************************************/
extern RESULT MipiRecDestroy
(
    MipiRecHandle_t     RecHandle
);


/*****************************************************************************/
/**
 * @brief   Check the given MIPI config structure for being a valid, supported combination.
 *
 * @param   pMipiConfig     Reference to MIPI config structure.
 *
 * @return                  Return the result of the function call.
 * @retval  RET_SUCCESS     combination is valid
 * @retval  RET_NOTSUPP     combination is invalid
 * @retval  RET_OUTOFRANGE  combination is not supported
 *
 *****************************************************************************/
extern RESULT MipiRecIsConfigValid
(
    MipiConfig_t        *pMipiConfig
);


/*****************************************************************************/
/**
 * @brief   Configures the MIPI receiver.
 *
 * @param   RecHandle           MIPI REC handle as returned by @ref MipiRecInit.
 * @param   pMipiConfig         Reference to new MIPI configuration.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         MIPI REC successfully configured.
 * @retval  RET_FAILURE et al.  Error starting the MIPI REC.
 *
 *****************************************************************************/
extern RESULT MipiRecConfig
(
    MipiRecHandle_t     RecHandle,
    MipiConfig_t        *pMipiConfig
);


/*****************************************************************************/
/**
 * @brief   Starts the MIPI receiver.
 *
 * @param   RecHandle           MIPI REC handle as returned by @ref MipiRecInit.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         MIPI REC successfully started.
 * @retval  RET_FAILURE et al.  Error starting the MIPI REC.
 *
 *****************************************************************************/
extern RESULT MipiRecStart
(
    MipiRecHandle_t     RecHandle
);


/*****************************************************************************/
/**
 * @brief   Stopps the MIPI receiver.
 *
 * @param   RecHandle           MIPI REC handle as returned by @ref MipiRecInit.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         MIPI REC successfully stopped.
 * @retval  RET_FAILURE et al.  Error stopping the MIPI REC.
 *
 *****************************************************************************/
extern RESULT MipiRecStop
(
    MipiRecHandle_t     RecHandle
);


#endif /* __MIPI_DRV_REC_H__ */
