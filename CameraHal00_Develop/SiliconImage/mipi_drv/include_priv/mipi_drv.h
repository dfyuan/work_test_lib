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
 * @file mipi_drv.h
 *
 * @brief   Definition of internal MIPI driver stuff.
 *
 *****************************************************************************/

#ifndef __MIPI_DRV_H__
#define __MIPI_DRV_H__

#include <ebase/types.h>

#include <mipi_drv/mipi_drv_common.h>
#include "mipi_drv_rec.h"
#include "mipi_drv_phy.h"


/******************************************************************************/
/**
 * @brief Driver command IDs.
 *
 ******************************************************************************/
typedef enum MipiDrvCmdID_e
{
    MIPI_DRV_CMD_STOP   = 0,
    MIPI_DRV_CMD_START  = 1,
    MIPI_DRV_CMD_CONFIG = 2
} MipiDrvCmdID_t;


/******************************************************************************/
/**
 * @brief Driver commands.
 *
 ******************************************************************************/
typedef struct MipiDrvCmd_s
{
    MipiDrvCmdID_t  ID;                     //!< ID of the command.
    union MipiDrvCmdData_u
    {
        struct MipiDrvCmdConfig_s
        {
            MipiConfig_t    *pMipiConfig;       //!< Reference to new MIPI configuration to use.
        } config;                           //!< Params for the @ref MIPI_DRV_CMD_CONFIG command.
    } params;                               //!< Any params for the command specified in @ref ID.
} MipiDrvCmd_t;


/******************************************************************************/
/**
 * @brief Driver states.
 *
 ******************************************************************************/
typedef enum MipiDrvState_e
{
    MIPI_DRV_STATE_NOT_CONFIGURED   = 0,
    MIPI_DRV_STATE_STOPPED          = 1,
    MIPI_DRV_STATE_RUNNING          = 2
} MipiDrvState_t;


/******************************************************************************/
/**
 * @brief MIPI driver context.
 *
 ******************************************************************************/
typedef struct MipiDrvContext_s
{
    void                    *pBase;             //!< Base addr of MIPI module.

    MipiDrvState_t          State;              //!< Current state of driver.

    MipiDrvConfig_t         Config;             //!< Config of driver (given at init).
    MipiConfig_t            MipiConfig;         //!< Current MIPI config of driver (set/updated during runtime).

    MipiRecHandle_t         RecHandle;          //!< Handle of Receiver instance.
    MipiPhyHandle_t         PhyHandle;          //!< Handle of PHY instance.
} MipiDrvContext_t;


/*****************************************************************************/
/**
 * @brief   This function creates the MIPI driver and associated MIPI HW drivers.
 *
 * @param   pMipiDrvCtx         Reference of MIPI driver context.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 *
 *****************************************************************************/
extern RESULT MipiDrvCreate
(
    MipiDrvContext_t    *pMipiDrvCtx
);


/*****************************************************************************/
/**
 * @brief   This function destroys the MIPI driver and associated MIPI HW drivers.
 *
 * @param   pMipiDrvCtx         Reference of MIPI driver context.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 *
 *****************************************************************************/
extern RESULT MipiDrvDestroy
(
    MipiDrvContext_t    *pMipiDrvCtx
);


/*****************************************************************************/
/**
 * @brief   This function sends a run control command to the MIPI driver.
 *
 * @param   pMipiDrvCtx         Reference of MIPI driver context.
 * @param   pCmd                Reference to structure specifying the command to execute and it's params.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 *
 *****************************************************************************/
extern RESULT MipiDrvCmd
(
    MipiDrvContext_t    *pMipiDrvCtx,
    MipiDrvCmd_t        *pCmd
);


/*****************************************************************************/
/**
 * @brief   Check the given MIPI config structure for being a valid, supported combination.
 *
 * @param   pMipiConfig         Reference to MIPI config structure.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         combination is valid
 * @retval  RET_INVALID_PARM    combination is invalid
 * @retval  RET_NOTSUPP         combination is not supported
 *
 *****************************************************************************/
extern RESULT MipiDrvIsConfigValid
(
    MipiConfig_t        *pMipiConfig
);


#endif /* __MIPI_DRV_H__ */
