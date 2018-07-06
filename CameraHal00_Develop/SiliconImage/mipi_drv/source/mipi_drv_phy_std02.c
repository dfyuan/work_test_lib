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
 * @file mipi_drv_phy_std02.c
 *
 * @brief   Implementation of MIPI Phy driver.
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>
#include <common/misc.h>

#include <hal/hal_api.h>

#include "mipi_drv_phy.h"
#include "mipi_drv_phy_std02.h"

CREATE_TRACER( MIPI_PHY_DEBUG, "MIPI-PHY-STD02: ", INFO,    0 );
CREATE_TRACER( MIPI_PHY_INFO , "MIPI-PHY-STD02: ", INFO,    0 );
CREATE_TRACER( MIPI_PHY_WARN , "MIPI-PHY-STD02: ", WARNING, 1 );
CREATE_TRACER( MIPI_PHY_ERROR, "MIPI-PHY-STD02: ", ERROR,   1 );


/******************************************************************************
 * local macro definitions
 *****************************************************************************/

//RESULT HalReadI2CReg(  HalHandle_t HalHandle, uint8_t bus_num, uint16_t slave_addr, uint32_t reg_address, uint8_t reg_addr_size, uint32_t *preg_value, uint8_t reg_size )
//RESULT HalWriteI2CReg( HalHandle_t HalHandle, uint8_t bus_num, uint16_t slave_addr, uint32_t reg_address, uint8_t reg_addr_size, uint32_t  reg_value,  uint8_t reg_size )
#define PHY_READ_REG(  _pPhyContext, _regAddr, _pData )  HalReadI2CReg(  (_pPhyContext)->HalHandle, (_pPhyContext)->I2cBusNum, (_pPhyContext)->I2cSlaveAddr,  (_regAddr), (_pPhyContext)->I2cRegAddrSize, (_pData), 1 )
#define PHY_WRITE_REG( _pPhyContext, _regAddr, _Data )   HalWriteI2CReg( (_pPhyContext)->HalHandle, (_pPhyContext)->I2cBusNum, (_pPhyContext)->I2cSlaveAddr,  (_regAddr), (_pPhyContext)->I2cRegAddrSize, (_Data),  1 )


/******************************************************************************
 * local type definitions
 *****************************************************************************/
typedef struct MipiPhyContext_s
{
    HalHandle_t     HalHandle;          //!< Handle of HAL session to use.
    uint32_t        HalDevID;           //!< HalDeviceID of the PHY.
    uint8_t         I2cBusNum;          //!< The I2C bus the PHY is connected to.
    uint16_t        I2cSlaveAddr;       //!< The I2C slave addr of the PHY.
    uint8_t         I2cRegAddrSize;     //!< The size of the I2C register address.
    MipiConfig_t    MipiConfig;         //!< MIPI config currently in use.
    bool_t          Running;            //!< Running state of PHY.
} MipiPhyContext_t;


/******************************************************************************
 * local variable declarations
 *****************************************************************************/


/******************************************************************************
 * local function prototypes
 *****************************************************************************/

/******************************************************************************
 * MipiPhyControlSTD02()
 *****************************************************************************/
static RESULT MipiPhyControlSTD02
(
    MipiPhyContext_t    *pMipiPhyCtx,
    bool_t              bEnable
);

/////******************************************************************************
//// * MipiPhyXxx()
//// *****************************************************************************/
////static RESULT MipiPhyXxx
////(
////    MipiPhyContext_t    *pMipiPhyCtx
////);

/******************************************************************************
 * API functions; see header file for detailed comment.
 *****************************************************************************/

/******************************************************************************
 * MipiPhyInit()
 *****************************************************************************/
RESULT MipiPhyInit
(
    MipiPhyHandle_t     *pMipiPhyHandle,
    MipiDrvConfig_t     *pMipiDrvConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( MIPI_PHY_INFO, "%s (enter)\n", __FUNCTION__ );

    // check params
    if ( (pMipiPhyHandle == NULL) || (pMipiDrvConfig == NULL) || (pMipiDrvConfig->HalHandle == NULL) )
    {
        return RET_NULL_POINTER;
    }

    uint32_t HalDevID = 0;
    switch(pMipiDrvConfig->InstanceNum)
    {
        case 0:
            //HalDevID = (pMipiDrvConfig->ItfNum == 0) ? HAL_DEVID_CAM_1A_PHY : HAL_DEVID_CAM_1B_PHY;
            break;
        case 1:
            //HalDevID = HAL_DEVID_CAM_2_PHY;
            break;
        default:
            result = RET_INVALID_PARM;
            goto error_exit;
    }

    // allocate context
    MipiPhyContext_t *pMipiPhyCtx = malloc( sizeof(MipiPhyContext_t) );
    if (pMipiPhyCtx == NULL)
    {
        result = RET_OUTOFMEM;
        goto error_exit;
    }

    // pre initialize context
    memset( pMipiPhyCtx, 0, sizeof(*pMipiPhyCtx) );
    pMipiPhyCtx->MipiConfig.NumLanes = 0;
    pMipiPhyCtx->Running = false;
    pMipiPhyCtx->HalHandle = pMipiDrvConfig->HalHandle;
    pMipiPhyCtx->HalDevID = HalDevID;

    #if 0
    switch(pMipiPhyCtx->HalDevID)
    {
        case HAL_DEVID_CAMPHY_1A:
            pMipiPhyCtx->I2cBusNum      = HAL_I2C_BUS_CAMPHY_1A;
            pMipiPhyCtx->I2cSlaveAddr   = STMIPID02_SLAVE_ADDR;
            pMipiPhyCtx->I2cRegAddrSize = STMIPID02_REGADDR_SIZE;
            break;

        case HAL_DEVID_CAMPHY_1B:
            pMipiPhyCtx->I2cBusNum      = HAL_I2C_BUS_CAMPHY_1B;
            pMipiPhyCtx->I2cSlaveAddr   = STMIPID02_SLAVE_ADDR;
            pMipiPhyCtx->I2cRegAddrSize = STMIPID02_REGADDR_SIZE;
            break;

        case HAL_DEVID_CAMPHY_2:
            pMipiPhyCtx->I2cBusNum      = HAL_I2C_BUS_CAMPHY_2;
            pMipiPhyCtx->I2cSlaveAddr   = STMIPID02_SLAVE_ADDR;
            pMipiPhyCtx->I2cRegAddrSize = STMIPID02_REGADDR_SIZE;
            break;

        default:
            result = RET_NOTSUPP;
            goto cleanup_1;
    }
    #endif
    
    // add HAL reference
    result = HalAddRef( pMipiPhyCtx->HalHandle );

    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_PHY_ERROR, "%s: HalAddRef() failed\n", __FUNCTION__ );
        goto cleanup_1;
    }

#if 0 //zyc ,for test
    // configure MIPI PHY in HAL
    result = HalSetCamPhyConfig( pMipiPhyCtx->HalHandle, pMipiPhyCtx->HalDevID, true, true );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_PHY_ERROR, "%s: HalSetCamPhyConfig() failed\n", __FUNCTION__ );
        goto cleanup_2;
    }

    // turn PHY on & take it out of reset
    result = HalSetReset( pMipiPhyCtx->HalHandle, pMipiPhyCtx->HalDevID, true );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_PHY_ERROR, "%s: HalSetReset(false) failed\n", __FUNCTION__ );
        goto cleanup_3;
    }
    result = HalSetPower( pMipiPhyCtx->HalHandle, pMipiPhyCtx->HalDevID, true );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_PHY_ERROR, "%s: HalSetPower(true) failed\n", __FUNCTION__ );
        goto cleanup_2;
    }
    osSleep(10);
    result = HalSetReset( pMipiPhyCtx->HalHandle, pMipiPhyCtx->HalDevID, false );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_PHY_ERROR, "%s: HalSetReset(false) failed\n", __FUNCTION__ );
        goto cleanup_3;
    }
    osSleep(10);

    // try to read Chip-&RevID register(s)
    uint32_t RevID = 0xA5, RevIDExpect = 0x00;
    result = PHY_READ_REG( pMipiPhyCtx, CLK_LANE_REG1_OFFS, &RevID ); //TODO: identify RevID or similar register...
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_PHY_ERROR, "%s: PHY_READ_REG(CLK_LANE_REG1_OFFS) failed\n", __FUNCTION__ );
        goto cleanup_3;
    }
    if (RevID != RevIDExpect)
    {
        TRACE( MIPI_PHY_ERROR, "%s: wrong RevID (0x%x, expected 0x%x)\n", __FUNCTION__, RevID, RevIDExpect );
        goto cleanup_3;
    }
#endif
    // success, so return handle
    *pMipiPhyHandle = (MipiPhyHandle_t)pMipiPhyCtx;

    TRACE( MIPI_PHY_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;

cleanup_3: // turn PHY off
    HalSetReset( pMipiPhyCtx->HalHandle, pMipiPhyCtx->HalDevID, true );
    HalSetPower( pMipiPhyCtx->HalHandle, pMipiPhyCtx->HalDevID, false );

cleanup_2: // remove HAL reference
    HalDelRef( pMipiPhyCtx->HalHandle );

cleanup_1: // free PHY context
    free( pMipiPhyCtx );

error_exit: // just return with error
    TRACE( MIPI_PHY_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}

/******************************************************************************
 * MipiPhyDestroy()
 *****************************************************************************/
RESULT MipiPhyDestroy
(
    MipiPhyHandle_t     PhyHandle
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE( MIPI_PHY_INFO, "%s (enter)\n", __FUNCTION__ );

    // check params
    if ( PhyHandle == NULL )
    {
        return RET_NULL_POINTER;
    }

    // get context
    MipiPhyContext_t *pMipiPhyCtx = (MipiPhyContext_t*) PhyHandle;

    // check state
    if (pMipiPhyCtx->Running)
    {
        TRACE( MIPI_PHY_ERROR, "%s: wrong state\n", __FUNCTION__ );
        result = RET_WRONG_STATE;
        goto error_exit;
    }
#if 0
    // turn PHY off
    lres = HalSetReset( pMipiPhyCtx->HalHandle, pMipiPhyCtx->HalDevID, true );
    if (RET_SUCCESS != lres)
    {
        TRACE( MIPI_PHY_ERROR, "%s: HalSetReset(true) failed\n", __FUNCTION__ );
        UPDATE_RESULT( result, lres );
    }
    lres = HalSetPower( pMipiPhyCtx->HalHandle, pMipiPhyCtx->HalDevID, false );
    if (RET_SUCCESS != lres)
    {
        TRACE( MIPI_PHY_ERROR, "%s: HalSetPower(true) failed\n", __FUNCTION__ );
        UPDATE_RESULT( result, lres );
    }
#endif
    // remove HAL reference
    lres = HalDelRef( pMipiPhyCtx->HalHandle );

    if (RET_SUCCESS != lres)
    {
        TRACE( MIPI_PHY_ERROR, "%s: HalDelRef() failed\n", __FUNCTION__ );
        UPDATE_RESULT( result, lres );
    }

    // that's it
    if (RET_SUCCESS != result)
    {
        goto error_exit;
    }

    TRACE( MIPI_PHY_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;

error_exit:
    TRACE( MIPI_PHY_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}


/******************************************************************************
 * MipiPhyConfig()
 *****************************************************************************/
RESULT MipiPhyConfig
(
    MipiPhyHandle_t     PhyHandle,
    MipiConfig_t        *pMipiConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( MIPI_PHY_INFO, "%s (enter)\n", __FUNCTION__ );

    // check params
    if ( (PhyHandle == NULL) || (pMipiConfig == NULL) )
    {
        return RET_NULL_POINTER;
    }

#if 0 //for test ,zyc; use 2 lanes
    switch(pMipiConfig->NumLanes)
    {
        case 1: // -> only 1 lane supported in bypass mode
            break;
        default:
            result = RET_OUTOFRANGE;
            goto error_exit;
    }
#endif 
    // get context
    MipiPhyContext_t *pMipiPhyCtx = (MipiPhyContext_t*) PhyHandle;

    // check state
    if (pMipiPhyCtx->Running)
    {
        TRACE( MIPI_PHY_ERROR, "%s: wrong state\n", __FUNCTION__ );
        result = RET_WRONG_STATE;
        goto error_exit;
    }

    // configure CamerIc MIPI PHY
    // -> nothing to do right now

    // remember new config
    pMipiPhyCtx->MipiConfig = *pMipiConfig;

    // that's it
    TRACE( MIPI_PHY_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;

error_exit:
    TRACE( MIPI_PHY_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}


/******************************************************************************
 * MipiPhyStart()
 *****************************************************************************/
RESULT MipiPhyStart
(
    MipiPhyHandle_t     PhyHandle
)
{
    RESULT result = RET_SUCCESS;

    TRACE( MIPI_PHY_INFO, "%s (enter)\n", __FUNCTION__ );

    // check params
    if ( PhyHandle == NULL )
    {
        return RET_NULL_POINTER;
    }

    // get context
    MipiPhyContext_t *pMipiPhyCtx = (MipiPhyContext_t*) PhyHandle;

    // check state
    if ( pMipiPhyCtx->Running || (pMipiPhyCtx->MipiConfig.NumLanes == 0) )
    {
        TRACE( MIPI_PHY_ERROR, "%s: wrong state\n", __FUNCTION__ );
        result = RET_WRONG_STATE;
        goto error_exit;
    }

    // enable PHY
    result = MipiPhyControlSTD02( pMipiPhyCtx, true );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_PHY_ERROR, "%s: MipiPhyControlSTD02(enable) failed\n", __FUNCTION__ );
        goto error_exit;
    }

    // set new state
    pMipiPhyCtx->Running = true;

    // that's it
    TRACE( MIPI_PHY_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;

error_exit:
    TRACE( MIPI_PHY_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}


/******************************************************************************
 * MipiPhyStop()
 *****************************************************************************/
RESULT MipiPhyStop
(
    MipiPhyHandle_t     PhyHandle
)
{
    RESULT result = RET_SUCCESS;

    TRACE( MIPI_PHY_INFO, "%s (enter)\n", __FUNCTION__ );

    // check params
    if ( PhyHandle == NULL )
    {
        return RET_NULL_POINTER;
    }

    // get context
    MipiPhyContext_t *pMipiPhyCtx = (MipiPhyContext_t*) PhyHandle;

    // check state
    if (!pMipiPhyCtx->Running)
    {
        TRACE( MIPI_PHY_ERROR, "%s: wrong state\n", __FUNCTION__ );
        result = RET_WRONG_STATE;
        goto error_exit;
    }

    // disable PHY
    result = MipiPhyControlSTD02( pMipiPhyCtx, false );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_PHY_ERROR, "%s: MipiPhyControlSTD02(disable) failed\n", __FUNCTION__ );
        goto error_exit;
    }

    // set new state
    pMipiPhyCtx->Running = false;

    // that's it
    TRACE( MIPI_PHY_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;

error_exit:
    TRACE( MIPI_PHY_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}


/******************************************************************************
 * local functions
 *****************************************************************************/

/******************************************************************************
 * MipiPhyControlSTD02()
 *****************************************************************************/
static RESULT MipiPhyControlSTD02
(
    MipiPhyContext_t *pMipiPhyCtx,
    bool_t           bEnable
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE( MIPI_PHY_INFO, "%s: (enter)\n", __FUNCTION__ );

    // check params
    DCT_ASSERT( pMipiPhyCtx != NULL );

    if(bEnable){
        HalPhyCtrl(pMipiPhyCtx->HalHandle,
               pMipiPhyCtx->MipiConfig.DevId, 
               0,
               pMipiPhyCtx->MipiConfig.NumLanes,
               pMipiPhyCtx->MipiConfig.Freq);

       return HalPhyCtrl(pMipiPhyCtx->HalHandle,
               pMipiPhyCtx->MipiConfig.DevId, 
               1,
               pMipiPhyCtx->MipiConfig.NumLanes,
               pMipiPhyCtx->MipiConfig.Freq);
    }else{
       return HalPhyCtrl(pMipiPhyCtx->HalHandle,
               pMipiPhyCtx->MipiConfig.DevId, 
               0,
               pMipiPhyCtx->MipiConfig.NumLanes,
               pMipiPhyCtx->MipiConfig.Freq);

    }

#if 0
    // do the work
    if( bEnable )
    {
        uint32_t regval;
        //INLINE uint32_t HalGetMaskedValue( uint32_t reg_value, uint32_t reg_mask, uint32_t shift_mask )
        //INLINE uint32_t HalSetMaskedValue( uint32_t reg_value, uint32_t reg_mask, uint32_t shift_mask, uint32_t value )

        TRACE( MIPI_PHY_DEBUG, "%s: start MIPI D-phy (STMIPID02)\n", __FUNCTION__ );

        // system
        regval  = 0;                                // Data on lower bits of bus
        regval |= 0;                                // bypass
        regval |= MODE_REG1_DECOMPRESSION_MODE_OFF; // no decompression
        regval |= 0;                                // only 1 lane supported in bypass mode // (pMipiPhyCtx->MipiConfig.NumLanes == 2) ? MODE_REG1_LANE_CONTROL_2_LANES : 0; // 2-lanes : 1-lane
        regval |= 0;                                // CSI2
        lres = PHY_WRITE_REG( pMipiPhyCtx, MODE_REG1_OFFS, regval ); //0x00 in 1 lane mode
        UPDATE_RESULT( result, lres );

        regval  = 0;                                // outputs not tristated
        regval |= 0;                                // clock not gated
        regval |= 0;                                // don't invert polarity of clock signal
        regval |= 0;                                // don't invert polarity of HSync signal
        regval |= 0;                                // don't invert polarity of VSync signal
        lres = PHY_WRITE_REG( pMipiPhyCtx, MODE_REG2_OFFS, regval ); //0x00
        UPDATE_RESULT( result, lres );

        regval  = MODE_REG3_I2C_COMP_LEAKAGE_COMP_ENABLE;   // Enable compensation macro
        regval |= 0;                                        // 0.90Rev of DPHY for main cam
        regval |= 0;                                        // select main camera
        lres = PHY_WRITE_REG( pMipiPhyCtx, MODE_REG3_OFFS, regval ); //0x20
        UPDATE_RESULT( result, lres );

        // data lane1.1
        regval = DATA_LANE0_REG2_CNTRL_MIPI_SEL;                                                // Data Lane 1.1: MIPI CSI2
        lres = PHY_WRITE_REG( pMipiPhyCtx, DATA_LANE0_REG2_OFFS, regval ); //0x01
        UPDATE_RESULT( result, lres );

        regval  = DATA_LANE0_REG1_SWAP_PINS_NOT;                                                // Data lane 1.1: no swap
        regval |= DATA_LANE0_REG1_ENABLE;                                                       // Data lane 1.1: enable
        lres = PHY_WRITE_REG( pMipiPhyCtx, DATA_LANE0_REG1_OFFS, regval ); //0x03
        UPDATE_RESULT( result, lres );

        //// data lane1.2 // -> only 1 lane supported in bypass mode
        //if (pMipiPhyCtx->MipiConfig.NumLanes == 2)
        //{
        //    regval = DATA_LANE1_REG2_CNTRL_MIPI_SEL;                                            // Data Lane 1.2: MIPI CSI2
        //    lres = PHY_WRITE_REG( pMipiPhyCtx, DATA_LANE1_REG2_OFFS, regval ); //0x01
        //    UPDATE_RESULT( result, lres );
        //
        //    regval  = 0;                                                                        // Data lane 1.2: no swap; yes, setting is different from lane1.1 ;-(((
        //    regval |= DATA_LANE1_REG1_ENABLE;                                                   // Data lane 1.2: enable
        //    lres = PHY_WRITE_REG( pMipiPhyCtx, DATA_LANE1_REG1_OFFS, regval ); //0x01
        //    UPDATE_RESULT( result, lres );
        //}

        // clock lane1
        regval = CLK_LANE_REG3_CNTRL_MIPI_SEL;                                                  // Clock lane 1: MIPI CSI
        lres = PHY_WRITE_REG( pMipiPhyCtx, CLK_LANE_REG3_OFFS, regval ); //0x02
        UPDATE_RESULT( result, lres );

        regval = HalSetMaskedValue( 0, CLK_LANE_REG1_UIX4_MASK, CLK_LANE_REG1_UIX4_SHFT, 11 );  // Clock lane 1: 1x 0.25ns sample delay from clock edge //TODO: THIS IS PIXCLK, (COMPRESSED) SAMPLE WIDTH AND PCB LAYOUT DEPENDENT, WE USE A FIXED WORST CASE VALUE FOR NOW...
        regval |= CLK_LANE_REG1_ENABLE;                                                         // Clock lane 1: enable
        lres = PHY_WRITE_REG( pMipiPhyCtx, CLK_LANE_REG1_OFFS, regval ); //0x2d new // 0x41 old
        UPDATE_RESULT( result, lres );
    }
    else
    {
        uint32_t regval;
        TRACE( MIPI_PHY_DEBUG, "%s: stop MIPI D-phy (STMIPID02)\n", __FUNCTION__ );

        // clock lane1
        regval = 0; // disable
        lres = PHY_WRITE_REG( pMipiPhyCtx, CLK_LANE_REG1_OFFS, regval ); //0x00
        UPDATE_RESULT( result, lres );

        // data lane1.1
        regval = 0; // disable
        lres = PHY_WRITE_REG( pMipiPhyCtx, DATA_LANE0_REG1_OFFS, regval ); //0x00
        UPDATE_RESULT( result, lres );

        //// data lane1.2 // -> only 1 lane supported in bypass mode
        //regval = 0; // disable
        //lres = PHY_WRITE_REG( pMipiPhyCtx, DATA_LANE1_REG1_OFFS, regval ); //0x00
        //UPDATE_RESULT( result, lres );
    }

    // that's it
    if (RET_SUCCESS != result)
    {
        goto error_exit;
    }
#endif
    TRACE( MIPI_PHY_INFO, "%s: (exit)\n", __FUNCTION__ );

    return result;

error_exit:
    TRACE( MIPI_PHY_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}


/////******************************************************************************
//// * MipiPhyXxx()
//// *****************************************************************************/
////static RESULT MipiPhyXxx
////(
////    MipiPhyContext_t *pMipiPhyCtx
////)
////{
////    RESULT result = RET_SUCCESS;
////
////    TRACE( MIPI_PHY_INFO, "%s (enter)\n", __FUNCTION__ );
////
////    // check params
////    DCT_ASSERT( pMipiPhyCtx != NULL );
////
////    // do some work
////    //...
////
////
////    TRACE( MIPI_PHY_INFO, "%s (exit)\n", __FUNCTION__ );
////
////    return result;
////}
