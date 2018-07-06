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
 * @file mipi_drv_phy.h
 *
 * @brief   Definition of MIPI phy driver API.
 *
 *****************************************************************************/

#ifndef __MIPI_DRV_PHY_H__
#define __MIPI_DRV_PHY_H__

#include <ebase/types.h>
#include <common/return_codes.h>

#include "mipi_drv_common.h"


/*****************************************************************************/
/**
 * @brief Handle to MIPI Phy driver context.
 *
 *****************************************************************************/
typedef struct MipiPhyContext_s *MipiPhyHandle_t;


/*****************************************************************************/
/**
 * @brief   Initializes MIPI PHY according to given settings
 *
 * @param   pPhyHandle          Reference to MIPI PHY context handle. A valid handle
 *                              will be returned upon successfull initialization,
 *                              otherwise the handle is undefined.
 * @param   pMipiDrvConfig      Config to use for initialization..
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         MIPI PHY successfully initialized.
 * @retval  RET_FAILURE et al.  Error initializing MIPI PHY.
 *
 *****************************************************************************/
extern RESULT MipiPhyInit
(
    MipiPhyHandle_t     *pPhyHandle,
    MipiDrvConfig_t     *pMipiDrvConfig
);


/*****************************************************************************/
/**
 * @brief   Shuts down MIPI PHY.
 *
 * @param   PhyHandle           MIPI PHY context handle as returned by @ref MipiPhyInit.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         MIPI PHY successfully shutdown.
 * @retval  RET_FAILURE et al.  Error shutting down MIPI PHY.
 *
 *****************************************************************************/
extern RESULT MipiPhyDestroy
(
    MipiPhyHandle_t     PhyHandle
);


/*****************************************************************************/
/**
 * @brief   Configures the MIPI PHY.
 *
 * @param   PhyHandle           MIPI PHY handle as returned by @ref MipiPhyInit.
 * @param   pMipiConfig         Reference to new MIPI configuration.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         MIPI PHY successfully configured.
 * @retval  RET_FAILURE et al.  Error starting the MIPI PHY.
 *
 *****************************************************************************/
extern RESULT MipiPhyConfig
(
    MipiPhyHandle_t     PhyHandle,
    MipiConfig_t        *pMipiConfig
);


/*****************************************************************************/
/**
 * @brief   Starts the MIPI PHY.
 *
 * @param   PhyHandle           MIPI PHY handle as returned by @ref MipiPhyInit.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         MIPI PHY successfully started.
 * @retval  RET_FAILURE et al.  Error starting the MIPI PHY.
 *
 *****************************************************************************/
extern RESULT MipiPhyStart
(
    MipiPhyHandle_t     PhyHandle
);


/*****************************************************************************/
/**
 * @brief   Stopps the MIPI PHY.
 *
 * @param   PhyHandle           MIPI PHY handle as returned by @ref MipiPhyInit.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         MIPI PHY successfully stopped.
 * @retval  RET_FAILURE et al.  Error stopping the MIPI PHY.
 *
 *****************************************************************************/
extern RESULT MipiPhyStop
(
    MipiPhyHandle_t     PhyHandle
);

#endif /* __MIPI_DRV_PHY_H__ */
