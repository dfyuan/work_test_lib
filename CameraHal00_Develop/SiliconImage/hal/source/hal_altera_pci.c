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
 * @file hal_altera_pci.c
 *
 * Description:
 *   This file implements the IRQ interface realized by the Altera
 *   PCI-Express board access function. You should use it for your
 *   PC implementation in combination with the Altera FPGA board.\n
 *
 *****************************************************************************/

#include <ebase/trace.h>
#include <common/align.h>

#include "hal_api.h"  // this includes 'hal_altera_pci.h' as well
#include "hal_altera_pci_mem.h"

#if defined ( HAL_ALTERA )


#include <i2c_drv/i2c_drv.h>


//CREATE_TRACER(HAL_DEBUG, "HAL-ALTERA: ", INFO, 1);
//CREATE_TRACER(HAL_INFO , "HAL-ALTERA: ", INFO, 1);
CREATE_TRACER(HAL_ERROR, "HAL-ALTERA: ", ERROR, 1);


/******************************************************************************
 * local macro definitions
 *****************************************************************************/
#define NUM_I2C 3

#define SYSCTRL_REVID_OFFS  0x00
#define SYSCTRL_SELECT_OFFS 0x10
#define SYSCTRL_RESET_OFFS  0x20

#define SYSCTRL_SELECT_CAM_1A_NEGEDGE   0x00000100
#define SYSCTRL_SELECT_CAM_1A_RESET     0x00000200
#define SYSCTRL_SELECT_CAM_1A_POWERDN   0x00000400

#define SYSCTRL_SELECT_CAM_1B_NEGEDGE   0x00002000
#define SYSCTRL_SELECT_CAM_1B_RESET     0x00004000
#define SYSCTRL_SELECT_CAM_1B_POWERDN   0x00008000

#define SYSCTRL_SELECT_CAM_2_NEGEDGE    0x00002000
#define SYSCTRL_SELECT_CAM_2_RESET      0x00004000
#define SYSCTRL_SELECT_CAM_2_POWERDN    0x00008000

#define SYSCTRL_SELECT_CAMPHY_1A_RESET   0x00001000
#define SYSCTRL_SELECT_CAMPHY_1A_POWERDN 0x00000000  //NOTE: not connected so far

#define SYSCTRL_SELECT_CAMPHY_1B_RESET   0x00010000
#define SYSCTRL_SELECT_CAMPHY_1B_POWERDN 0x00000000  //NOTE: not connected so far

#define SYSCTRL_SELECT_CAMPHY_2_RESET    0x00010000
#define SYSCTRL_SELECT_CAMPHY_2_POWERDN  0x00000000  //NOTE: not connected so far

#define EXT_MEM_ALIGN    0x1000 // 4k
#define EXT_MEM_BASE     EXT_MEM_ALIGN // musn't be 0!; will be aligned up to EXT_MEM_ALIGNment
#define EXT_MEM_SIZE (0x10000000 - EXT_MEM_BASE) // ~256MB

#define DMA_STRIDE_BYTES 8 ////FPGA_DMA_SIZE_ALIGNMENT // currently 32
#define DMA_MEM_ALIGN    512

#define HAL_USE_RAW_DMA // comment out to use dma functions with byte reordering & stuff


/******************************************************************************
 * local type definitions
 *****************************************************************************/
typedef struct HalCamConfig_s
{
    bool_t configured;      //!< Mark whether this config was set.
    bool_t power_lowact;    //!< Power on is low-active.
    bool_t reset_lowact;    //!< Reset is low-active.
    //bool_t negedge;         //!< Capture data on negedge.
} HalCamConfig_t;

typedef struct HalCamPhyConfig_s
{
    bool_t configured;      //!< Mark whether this config was set.
    bool_t power_lowact;    //!< Power on is low-active.
    bool_t reset_lowact;    //!< Reset is low-active.
} HalCamPhyConfig_t;

typedef struct HalMemMap_s
{
    ulong_t    mem_address;    //!< Hardware memory address.
    uint32_t        byte_size;      //!< Size of mapped buffer.
    HalMapMemType_t mapping_type;   //!< How the buffer is mapped.
    void            *p_bufbase;     //!< Base of allocated buffer.
} HalMemMap_t;

/******************************************************************************
 * HalContext_t
 *****************************************************************************/
typedef struct HalContext_s
{
    osMutex             modMutex;               //!< common short term mutex; e.g. for read-modify-write accesses
    uint32_t            refCount;               //!< internal ref count

    osMutex             iicMutex[NUM_I2C];      //!< transaction locks for I2C controller 1..NUM_I2C
    i2c_bus_t           iicConfig[NUM_I2C];     //!< configurations for I2C controller 1..NUM_I2C

    HalCamConfig_t      cam1AConfig;            //!< configuration for CAM1; set at runtime
    HalCamPhyConfig_t   camPhy1AConfig;         //!< configuration for CAMPHY1; set at runtime
    
    HalCamConfig_t      cam1BConfig;            //!< configuration for CAM1; set at runtime
    HalCamPhyConfig_t   camPhy1BConfig;         //!< configuration for CAMPHY1; set at runtime
    
    HalCamConfig_t      cam2Config;             //!< configuration for CAM2; set at runtime
    HalCamPhyConfig_t   camPhy2Config;          //!< configuration for CAMPHY2; set at runtime
} HalContext_t;


/******************************************************************************
 * local variable declarations
 *****************************************************************************/
static HalContext_t gHalContext  = { .refCount = 0 };
static bool_t       gInitialized = false;

/******************************************************************************
 * PLL stuff
 * Fout,x = Fin * M / N / Cx
 * Cx = Fin * M / N / Fout,x
 * with: M = 1..511; N = 1..511; Cx = 1..511
 * Limits for Stratix IV -3(-2) devices:
 * Fin, Fout       =>   5 ..  717 (800) MHz
 * Fpfd = Fin / N  =>   5 ..  325       MHz
 * Fvco = Fpfd * M => 600 .. 1300(1600) MHz
 *****************************************************************************/
static const AlteraFPGAPLLConfig_t pll_cfg_vdupclk_25_2  = { 54000000, 105,  5, {  45, 14 } }; // Fin, M, N, C0 = pclk = 25.2MHz,  C1 = vdu = 81MHz
static const AlteraFPGAPLLConfig_t pll_cfg_vdupclk_74_25 = { 54000000,  33,  2, {  12, 11 } }; // Fin, M, N, C0 = pclk = 74.25MHz, C1 = vdu = 81MHz
static const AlteraFPGAPLLConfig_t pll_cfg_vdupclk_148_5 = { 54000000,  33,  2, {   6, 11 } }; // Fin, M, N, C0 = pclk = 148.5MHz, C1 = vdu = 81MHz
static const AlteraFPGAPLLConfig_t pll_cfg_cam1_06_0     = { 50000000, 105,  5, { 175     } }; // Fin, M, N, C0 = cam1 =  6.0MHz
static const AlteraFPGAPLLConfig_t pll_cfg_cam1_10_0     = { 50000000, 100,  5, { 100     } }; // Fin, M, N, C0 = cam1 = 10.0MHz
static const AlteraFPGAPLLConfig_t pll_cfg_cam1_20_0     = { 50000000, 100,  5, {  50     } }; // Fin, M, N, C0 = cam1 = 20.0MHz
static const AlteraFPGAPLLConfig_t pll_cfg_cam1_24_0     = { 50000000,  96,  2, { 100     } }; // Fin, M, N, C0 = cam1 = 24.0MHz

static const AlteraFPGAPLL_t *pll_vdupclk = &AlteraFPGAPLL_1;
static const AlteraFPGAPLL_t *pll_cam1    = &AlteraFPGAPLL_2;

/******************************************************************************
 * local function prototypes
 *****************************************************************************/
static int32_t halIsrHandler( void *pArg );

static RESULT HalLockContext( HalContext_t *pHalCtx );
static RESULT HalUnlockContext( HalContext_t *pHalCtx );


/******************************************************************************
 * API functions; see header file for detailed comment.
 *****************************************************************************/


/******************************************************************************
 * HalOpen()
 *****************************************************************************/
HalHandle_t HalOpen( void )
{
    HalContext_t *pHalCtx = NULL;
    RESULT result = RET_SUCCESS;
    RESULT lres;
    OSLAYER_STATUS osStatus = OSLAYER_OK;
    i2c_bus_t *pConfig;
    I2cReturnType i2c_result;
    int32_t currI2c = -1;

    //TODO: need a global mutex for manipulating gInitialized
    // just one instance allowed
    if (gInitialized)
    {
        return NULL;
    }
    gInitialized = true;

    // 'create' context
    pHalCtx = &gHalContext;
    memset( pHalCtx, 0, sizeof(HalContext_t) );
    pHalCtx->refCount = 1;

    // open board
    if ( FPGA_RES_OK != AlteraFPGABoard_Open() )
    {
        TRACE( HAL_ERROR, "Can't open Altera FPGA board\n" );
        goto cleanup_1;
    }

    // initialize context
    if ( OSLAYER_OK != osMutexInit( &pHalCtx->modMutex ) )
    {
        TRACE( HAL_ERROR, "Can't initialize modMutex\n" );
        goto cleanup_2;
    }

    for ( currI2c = 0; currI2c < NUM_I2C; currI2c++ )
    {
        if ( OSLAYER_OK != osMutexInit( &pHalCtx->iicMutex[currI2c] ) )
        {
            TRACE( HAL_ERROR, "Can't initialize iicMutex #%d\n", currI2c );
            goto cleanup_3;
        }
    }

    pHalCtx->cam1AConfig.configured = false;
    pHalCtx->camPhy1AConfig.configured = false;
    pHalCtx->cam1BConfig.configured = false;
    pHalCtx->camPhy1BConfig.configured = false;
    pHalCtx->cam2Config.configured = false;
    pHalCtx->camPhy2Config.configured = false;

    // power on all internal devices; external ones aren't configured right now
    lres = HalSetPower( (HalHandle_t)pHalCtx, HAL_DEVID_INTERNAL, true );
    UPDATE_RESULT( result, lres );
    osSleep(1);

    // activate all internal resets; external ones aren't configured right now
    lres = HalSetReset( (HalHandle_t)pHalCtx, HAL_DEVID_INTERNAL, true );
    UPDATE_RESULT( result, lres );
    osSleep(1);

    // deactivate all internal resets; external ones aren't configured right now
    lres = HalSetReset( (HalHandle_t)pHalCtx, HAL_DEVID_INTERNAL, false );
    UPDATE_RESULT( result, lres );
    osSleep(1);

    if (RET_SUCCESS != result)
    {
        TRACE( HAL_ERROR, "Reseting internal devices (0x%08x) failed\n", HAL_DEVID_INTERNAL );
        goto cleanup_4;
    }

    // initialize all I2C modules
    for ( currI2c = 0; currI2c < NUM_I2C; currI2c++ )
    {
        static const uint32_t gI2cBaseAddr[NUM_I2C] = { HAL_BASEADDR_I2C_0, HAL_BASEADDR_I2C_1, HAL_BASEADDR_I2C_2 };

        pConfig = &pHalCtx->iicConfig[currI2c];

        pConfig->ulControllerBaseAddress       = gI2cBaseAddr[currI2c];
        pConfig->aucMasterAddrMode             = 0;     // 7 bit Master Address Mode
        pConfig->ulMasterAddr                  = 0x75;  // Master Device Address
        pConfig->ulSclRef                      = 0x1F0; // Clock Divider Value for 75MHz
        pConfig->aucVirtualClockDividerEnable  = 1;     // Virtual System Clock Divider on
        pConfig->aucTimingMode                 = 0;     // Timing Mode standard
        pConfig->aucSpikeFilter                = 1;     // Suppressed Spike Width 1
        pConfig->aucIrqDisable                 = 1;     // Interrupt disabled

        i2c_result =  i2c_init( pConfig );
        if ( I2C_RET_SUCCESS != i2c_result )
        {
            TRACE( HAL_ERROR, "Can't configure I2C bus #%d (i2c_result=%d)\n", currI2c, i2c_result );
            goto cleanup_5;
        }
    }

    // create ext mem allocator
    lres = ExtMemCreate( EXT_MEM_BASE, EXT_MEM_SIZE, EXT_MEM_ALIGN );
    if (RET_SUCCESS != lres)
    {
        TRACE( HAL_ERROR, "Creating ext mem allocator failed, BaseAddr=0x%x, Size=0x%x, Align=0x%x (result=%d)\n", lres, EXT_MEM_BASE, EXT_MEM_SIZE, EXT_MEM_ALIGN );
        UPDATE_RESULT( result, lres );
        goto cleanup_5;
    }


    // success
    return (HalHandle_t)pHalCtx;

    // failure cleanup
cleanup_5: // shutdown some i2c controllers
    while ( currI2c > 0 )
    {
        //TODO: i2c_shutdown( &pHalCtx->iicConfig[--currI2c] );
    }

cleanup_4: // cleanup all mutexes
    currI2c = NUM_I2C;

cleanup_3: // cleanup some mutexes
    while ( currI2c > 0 )
    {
        osMutexDestroy( &pHalCtx->iicMutex[--currI2c] );
    }

    osMutexDestroy( &pHalCtx->modMutex );

cleanup_2: // close board
    AlteraFPGABoard_Close();

cleanup_1: // 'destroy' context
    memset( pHalCtx, 0, sizeof(HalContext_t) );
    pHalCtx->refCount = 0;

cleanup_0: // uninitialized again
    gInitialized = false; //TODO: need a global mutex for manipulating gInitialized

    return NULL;
}


/******************************************************************************
 * HalClose()
 *****************************************************************************/
RESULT HalClose( HalHandle_t HalHandle )
{
    RESULT result = RET_SUCCESS;
    OSLAYER_STATUS osStatus;
    I2cReturnType i2c_result;
    int32_t currI2c = -1;

    if (HalHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    if (HalHandle != (HalHandle_t)&gHalContext)
    {
        return RET_WRONG_HANDLE;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        return RET_WRONG_STATE;
    }

    //TODO: need a mutex for manipulating pHalCtx->refCount
    if (pHalCtx->refCount == 1)
    {
        RESULT lres;
        uint32_t fpga_result;

        // destroy ext mem allocator
        lres = ExtMemDestroy();
        if (RET_SUCCESS != lres)
        {
            TRACE( HAL_ERROR, "Destroying ext mem allocator failed (result=%d)\n", lres );
            UPDATE_RESULT( result, lres );
        }

        for ( currI2c = 0; currI2c < NUM_I2C; currI2c++ )
        {
            //TODO: i2c_result = i2c_shutdown( &pHalCtx->iicConfig[--currI2c] );
            //      UPDATE_RESULT( result, (I2C_RET_SUCCESS == i2c_result) ? RET_SUCCESS : FAILURE );
        }

        // get common device mask
        uint32_t dev_mask = HAL_DEVID_INTERNAL
                            | (pHalCtx->cam1AConfig.configured ? HAL_DEVID_CAM_1A : 0)
                            | (pHalCtx->camPhy1AConfig.configured ? HAL_DEVID_CAMPHY_1A : 0)
                            | (pHalCtx->cam1BConfig.configured ? HAL_DEVID_CAM_1B : 0)
                            | (pHalCtx->camPhy1BConfig.configured ? HAL_DEVID_CAMPHY_1B : 0)
                            | (pHalCtx->cam2Config.configured ? HAL_DEVID_CAM_2 : 0)
                            | (pHalCtx->camPhy2Config.configured ? HAL_DEVID_CAMPHY_2 : 0);

        // activate all resets
        lres = HalSetReset( HalHandle, dev_mask, true );
        if (RET_SUCCESS != lres)
        {
            TRACE( HAL_ERROR, "Reseting devices (0x%08x) failed\n", dev_mask );
            UPDATE_RESULT( result, lres );
        }
        osSleep(1);

        // power off all devices
        lres = HalSetPower( HalHandle, dev_mask, true );
        if (RET_SUCCESS != lres)
        {
            TRACE( HAL_ERROR, "PowerDown of devices (0x%08x) failed\n", dev_mask );
            UPDATE_RESULT( result, lres );
        }
        osSleep(1);

        // cleanup context
        for ( currI2c = 0; currI2c < NUM_I2C; currI2c++ )
        {
            osStatus = osMutexDestroy( &pHalCtx->iicMutex[currI2c] );
            UPDATE_RESULT( result, (OSLAYER_OK == osStatus) ? RET_SUCCESS : RET_FAILURE );
        }

        osStatus = osMutexDestroy( &pHalCtx->modMutex );
        UPDATE_RESULT( result, (OSLAYER_OK == osStatus) ? RET_SUCCESS : RET_FAILURE );

        // close board
        fpga_result = AlteraFPGABoard_Close();
        UPDATE_RESULT( result, (FPGA_RES_OK == fpga_result) ? RET_SUCCESS : RET_FAILURE );

        // 'destroy' context
        memset( pHalCtx, 0, sizeof(HalContext_t) );
        pHalCtx->refCount = 0;

        // not initialized again
        gInitialized = false; //TODO: need a global mutex for manipulating gInitialized
    }
    else
    {
        //TODO: need a mutex for manipulating pHalCtx->refCount
        pHalCtx->refCount--;
    }

    return result;
}


/******************************************************************************
 * HalAddRef()
 *****************************************************************************/
RESULT HalAddRef( HalHandle_t HalHandle )
{
    if (HalHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        return RET_WRONG_STATE;
    }

    //TODO: need a mutex for manipulating pHalCtx->refCount
    pHalCtx->refCount++;

    return RET_SUCCESS;
}


/******************************************************************************
 * HalDelRef()
 *****************************************************************************/
RESULT HalDelRef( HalHandle_t HalHandle )
{
    // just forward to HalClose; it takes care of everything
    return HalClose( HalHandle );
}


/******************************************************************************
 * HalNumOfCams()
 *****************************************************************************/
RESULT HalNumOfCams( HalHandle_t HalHandle, uint32_t *no )
{
    RESULT result = RET_SUCCESS;

    if ( no )
    {
        *no = HAL_NO_CAMS;
    }
    else
    {
        result = RET_NULL_POINTER;
    }
   
    return ( result );
}


/******************************************************************************
 * HalSetCamConfig()
 *****************************************************************************/
RESULT HalSetCamConfig( HalHandle_t HalHandle, uint32_t dev_mask, bool_t power_lowact, bool_t reset_lowact, bool_t pclk_negedge )
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    if (HalHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    // check for valid devices
    if (dev_mask & ~(HAL_DEVID_CAM_1A | HAL_DEVID_CAM_1B | HAL_DEVID_CAM_2))
    {
        return RET_INVALID_PARM;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        return RET_WRONG_STATE;
    }

    // update config(s)
    //NOTE: select clock edges for all CAM devices at once as well
    //TODO: need a mutex around the following block:
    {
        uint32_t extdev_mask = 0;
        uint32_t negedge_extdev = 0;
        uint32_t curValue;
        uint32_t newValue;

        // build external devices & high-active external devices masks
        if (dev_mask & HAL_DEVID_CAM_1A) // duplicate & modify this 'if'-block for further external devices
        {
            // set config
            pHalCtx->cam1AConfig.configured   = true;
            pHalCtx->cam1AConfig.power_lowact = power_lowact;
            pHalCtx->cam1AConfig.reset_lowact = reset_lowact;

            // sample edge
            extdev_mask    |= SYSCTRL_SELECT_CAM_1A_NEGEDGE;
            negedge_extdev |= pclk_negedge ? SYSCTRL_SELECT_CAM_1A_NEGEDGE : 0;
        }

        if (dev_mask & HAL_DEVID_CAM_1B) // duplicate & modify this 'if'-block for further external devices
        {
            // set config
            pHalCtx->cam1BConfig.configured   = true;
            pHalCtx->cam1BConfig.power_lowact = power_lowact;
            pHalCtx->cam1BConfig.reset_lowact = reset_lowact;

            // sample edge
            extdev_mask    |= SYSCTRL_SELECT_CAM_1B_NEGEDGE;
            negedge_extdev |= pclk_negedge ? SYSCTRL_SELECT_CAM_1B_NEGEDGE : 0;
        }

        if (dev_mask & HAL_DEVID_CAM_2) // duplicate & modify this 'if'-block for further external devices
        {
            // set config
            pHalCtx->cam2Config.configured   = true;
            pHalCtx->cam2Config.power_lowact = power_lowact;
            pHalCtx->cam2Config.reset_lowact = reset_lowact;

            // sample edge
            extdev_mask    |= SYSCTRL_SELECT_CAM_2_NEGEDGE;
            negedge_extdev |= pclk_negedge ? SYSCTRL_SELECT_CAM_2_NEGEDGE : 0;
        }

        // get current select reg value
        curValue = AlteraFPGABoard_ReadBAR( HAL_BASEREGION_SYSCTRL, HAL_BASEADDR_SYSCTRL + SYSCTRL_SELECT_OFFS );
        if (FPGA_READ_ERROR == curValue) // well, we're checking for 0xDEADBEEF here, which in fact is a valid register value...
        {
            UPDATE_RESULT( result, RET_FAILURE );
        }
        else
        {
            // calc new value
            newValue = (curValue & ~extdev_mask) | (negedge_extdev & extdev_mask);

            // set new select reg value
            uint32_t alteraResult = AlteraFPGABoard_WriteBAR( HAL_BASEREGION_SYSCTRL, HAL_BASEADDR_SYSCTRL + SYSCTRL_SELECT_OFFS, newValue );
            if ( alteraResult != FPGA_RES_OK )
            {
                UPDATE_RESULT( result, RET_FAILURE );
            }
        }
    }

    // set default state (power down, reset active)
    lres = HalSetPower( HalHandle, dev_mask, false );
    UPDATE_RESULT( result, lres );
    lres = HalSetReset( HalHandle, dev_mask, true );
    UPDATE_RESULT( result, lres );

    return result;
}

/******************************************************************************
 * HalSetCamPhyConfig()
 *****************************************************************************/
RESULT HalSetCamPhyConfig( HalHandle_t HalHandle, uint32_t dev_mask, bool_t power_lowact, bool_t reset_lowact )
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    if (HalHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    // check for valid devices
    if (dev_mask & ~(HAL_DEVID_CAMPHY_1A | HAL_DEVID_CAMPHY_1B | HAL_DEVID_CAMPHY_2))
    {
        return RET_INVALID_PARM;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        return RET_WRONG_STATE;
    }

    // update config(s)
    //TODO: need a mutex around the following block:
    {
        // build external devices & high-active external devices masks
        if (dev_mask & HAL_DEVID_CAMPHY_1A) // duplicate & modify this 'if'-block for further external devices
        {
            // set config
            pHalCtx->camPhy1AConfig.configured   = true;
            pHalCtx->camPhy1AConfig.power_lowact = power_lowact;
            pHalCtx->camPhy1AConfig.reset_lowact = reset_lowact;
        }
      
        if (dev_mask & HAL_DEVID_CAMPHY_1B) // duplicate & modify this 'if'-block for further external devices
        {
            // set config
            pHalCtx->camPhy1BConfig.configured   = true;
            pHalCtx->camPhy1BConfig.power_lowact = power_lowact;
            pHalCtx->camPhy1BConfig.reset_lowact = reset_lowact;
        }

        if (dev_mask & HAL_DEVID_CAMPHY_2)
        {
            // set config
            pHalCtx->camPhy2Config.configured   = true;
            pHalCtx->camPhy2Config.power_lowact = power_lowact;
            pHalCtx->camPhy2Config.reset_lowact = reset_lowact;
        }
    }

    // set default state (power down, reset active)
    lres = HalSetPower( HalHandle, dev_mask, false );
    UPDATE_RESULT( result, lres );
    lres = HalSetReset( HalHandle, dev_mask, true );
    UPDATE_RESULT( result, lres );

    return result;
}

/******************************************************************************
 * HalSetReset()
 *****************************************************************************/
RESULT HalSetReset( HalHandle_t HalHandle, uint32_t dev_mask, bool_t activate )
{
    RESULT result = RET_SUCCESS;

    if (HalHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    // check for valid devices
    if (dev_mask & ~HAL_DEVID_ALL)
    {
        return RET_INVALID_PARM;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        return RET_WRONG_STATE;
    }

    // first process all internal devices
    //TODO: need a mutex around the following block:
    {
        uint32_t intdev_mask = 0;
        uint32_t curValue;
        uint32_t newValue;

        // build internal devices mask
        intdev_mask = dev_mask & HAL_DEVID_INTERNAL;

        // get current reg value
        curValue = AlteraFPGABoard_ReadBAR( HAL_BASEREGION_SYSCTRL, HAL_BASEADDR_SYSCTRL + SYSCTRL_RESET_OFFS );
        if (FPGA_READ_ERROR == curValue) // well, we're checking for 0xDEADBEEF here, which in fact is a valid register value...
        {
            UPDATE_RESULT( result, RET_FAILURE );
        }
        else
        {
            // calc new value
            if (activate)
            {
                // assert reset    (low-active ones) | (high-active ones)
                newValue = (curValue & ~intdev_mask) | (HAL_DEVID_HIGHACTIVE_RESET_DEVICES & intdev_mask);
            }
            else
            {
                // deassert reset (low-active ones) & ~(high-active ones)
                newValue = (curValue | intdev_mask) & ~(HAL_DEVID_HIGHACTIVE_RESET_DEVICES & intdev_mask);
            }

            // set new reg value
            uint32_t alteraResult = AlteraFPGABoard_WriteBAR( HAL_BASEREGION_SYSCTRL, HAL_BASEADDR_SYSCTRL + SYSCTRL_RESET_OFFS, newValue );
            if ( alteraResult != FPGA_RES_OK )
            {
                UPDATE_RESULT( result, RET_FAILURE );
            }
        }
    }

    // then process all external devices
    //TODO: need a mutex around the following block:
    {
        uint32_t extdev_mask = 0;
        uint32_t highact_extdev = 0;
        uint32_t curValue;
        uint32_t newValue;

        // build external devices & high-active external devices masks
        if (dev_mask & HAL_DEVID_CAM_1A) // duplicate & modify this 'if'-block for further external devices
        {
            if (!pHalCtx->cam1AConfig.configured)
            {
                UPDATE_RESULT( result, RET_INVALID_PARM );
            }
            else
            {
                extdev_mask    |= SYSCTRL_SELECT_CAM_1A_RESET;
                highact_extdev |= pHalCtx->cam1AConfig.reset_lowact ? 0 : SYSCTRL_SELECT_CAM_1A_RESET;
            }
        }
        if (dev_mask & HAL_DEVID_CAM_1B)
        {
            if (!pHalCtx->cam1BConfig.configured)
            {
                UPDATE_RESULT( result, RET_INVALID_PARM );
            }
            else
            {
                extdev_mask    |= SYSCTRL_SELECT_CAM_1B_RESET;
                highact_extdev |= pHalCtx->cam1BConfig.reset_lowact ? 0 : SYSCTRL_SELECT_CAM_1B_RESET;
            }
        }
        if (dev_mask & HAL_DEVID_CAM_2) 
        {
            if (!pHalCtx->cam2Config.configured)
            {
                UPDATE_RESULT( result, RET_INVALID_PARM );
            }
            else
            {
                extdev_mask    |= SYSCTRL_SELECT_CAM_2_RESET;
                highact_extdev |= pHalCtx->cam2Config.reset_lowact ? 0 : SYSCTRL_SELECT_CAM_2_RESET;
            }
        }

        if (dev_mask & HAL_DEVID_CAMPHY_1A)
        {
            if (!pHalCtx->camPhy1AConfig.configured)
            {
                UPDATE_RESULT( result, RET_INVALID_PARM );
            }
            else
            {
                extdev_mask    |= SYSCTRL_SELECT_CAMPHY_1A_RESET;
                highact_extdev |= pHalCtx->camPhy1AConfig.reset_lowact ? 0 : SYSCTRL_SELECT_CAMPHY_1A_RESET;
            }
        }
        if (dev_mask & HAL_DEVID_CAMPHY_1B)
        {
            if (!pHalCtx->camPhy1BConfig.configured)
            {
                UPDATE_RESULT( result, RET_INVALID_PARM );
            }
            else
            {
                extdev_mask    |= SYSCTRL_SELECT_CAMPHY_1B_RESET;
                highact_extdev |= pHalCtx->camPhy1BConfig.reset_lowact ? 0 : SYSCTRL_SELECT_CAMPHY_1B_RESET;
            }
        }
        if (dev_mask & HAL_DEVID_CAMPHY_2)
        {
            if (!pHalCtx->camPhy2Config.configured)
            {
                UPDATE_RESULT( result, RET_INVALID_PARM );
            }
            else
            {
                extdev_mask    |= SYSCTRL_SELECT_CAMPHY_2_RESET;
                highact_extdev |= pHalCtx->camPhy2Config.reset_lowact ? 0 : SYSCTRL_SELECT_CAMPHY_2_RESET;
            }
        }

        // get current select reg value
        curValue = AlteraFPGABoard_ReadBAR( HAL_BASEREGION_SYSCTRL, HAL_BASEADDR_SYSCTRL + SYSCTRL_SELECT_OFFS );
        if (FPGA_READ_ERROR == curValue) // well, we're checking for 0xDEADBEEF here, which in fact is a valid register value...
        {
            UPDATE_RESULT( result, RET_FAILURE );
        }
        else
        {
            // calc new value
            if (activate)
            {
                // assert reset    (low-active ones) | (high-active ones)
                newValue = (curValue & ~extdev_mask) | (highact_extdev & extdev_mask);
            }
            else
            {
                // deassert reset (low-active ones) & ~(high-active ones)
                newValue = (curValue | extdev_mask) & ~(highact_extdev & extdev_mask);
            }

            // set new select reg value
            uint32_t alteraResult = AlteraFPGABoard_WriteBAR( HAL_BASEREGION_SYSCTRL, HAL_BASEADDR_SYSCTRL + SYSCTRL_SELECT_OFFS, newValue );
            if ( alteraResult != FPGA_RES_OK )
            {
                UPDATE_RESULT( result, RET_FAILURE );
            }
        }
    }

    return result;
}


/******************************************************************************
 * HalSetPower()
 *****************************************************************************/
RESULT HalSetPower( HalHandle_t HalHandle, uint32_t dev_mask, bool_t activate )
{
    RESULT result = RET_SUCCESS;

    if (HalHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    // check for valid devices
    if (dev_mask & ~HAL_DEVID_ALL)
    {
        return RET_INVALID_PARM;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        return RET_WRONG_STATE;
    }

    // process external devices only
    //TODO: need a mutex around the following block:
    {
        uint32_t extdev_mask = 0;
        uint32_t highact_extdev = 0;
        uint32_t curValue;
        uint32_t newValue;

        // build external devices & high-active external devices masks
        if (dev_mask & HAL_DEVID_CAM_1A) // duplicate & modify this 'if'-block for further external devices
        {
            if (!pHalCtx->cam1AConfig.configured)
            {
                UPDATE_RESULT( result, RET_FAILURE );
            }
            else
            {
                extdev_mask    |= SYSCTRL_SELECT_CAM_1A_POWERDN;
                highact_extdev |= pHalCtx->cam1AConfig.power_lowact ? 0 : SYSCTRL_SELECT_CAM_1A_POWERDN;
            }
        }
        if (dev_mask & HAL_DEVID_CAM_1B)
        {
            if (!pHalCtx->cam1BConfig.configured)
            {
                UPDATE_RESULT( result, RET_FAILURE );
            }
            else
            {
                extdev_mask    |= SYSCTRL_SELECT_CAM_1B_POWERDN;
                highact_extdev |= pHalCtx->cam1BConfig.power_lowact ? 0 : SYSCTRL_SELECT_CAM_1B_POWERDN;
            }
        }
        if (dev_mask & HAL_DEVID_CAM_2) 
        {
            if (!pHalCtx->cam2Config.configured)
            {
                UPDATE_RESULT( result, RET_FAILURE );
            }
            else
            {
                extdev_mask    |= SYSCTRL_SELECT_CAM_2_POWERDN;
                highact_extdev |= pHalCtx->cam2Config.power_lowact ? 0 : SYSCTRL_SELECT_CAM_2_POWERDN;
            }
        }

        if (dev_mask & HAL_DEVID_CAMPHY_1A)
        {
            if (!pHalCtx->camPhy1AConfig.configured)
            {
                UPDATE_RESULT( result, RET_FAILURE );
            }
            else
            {
                extdev_mask    |= SYSCTRL_SELECT_CAMPHY_1A_POWERDN;
                highact_extdev |= pHalCtx->camPhy1AConfig.power_lowact ? 0 : SYSCTRL_SELECT_CAMPHY_1A_POWERDN;
            }
        }
        if (dev_mask & HAL_DEVID_CAMPHY_1B)
        {
            if (!pHalCtx->camPhy1BConfig.configured)
            {
                UPDATE_RESULT( result, RET_FAILURE );
            }
            else
            {
                extdev_mask    |= SYSCTRL_SELECT_CAMPHY_1B_POWERDN;
                highact_extdev |= pHalCtx->camPhy1BConfig.power_lowact ? 0 : SYSCTRL_SELECT_CAMPHY_1B_POWERDN;
            }
        }
        if (dev_mask & HAL_DEVID_CAMPHY_2)
        {
            if (!pHalCtx->camPhy2Config.configured)
            {
                UPDATE_RESULT( result, RET_FAILURE );
            }
            else
            {
                extdev_mask    |= SYSCTRL_SELECT_CAMPHY_2_POWERDN;
                highact_extdev |= pHalCtx->camPhy2Config.power_lowact ? 0 : SYSCTRL_SELECT_CAMPHY_2_POWERDN;
            }
        }

        // get current select reg value
        curValue = AlteraFPGABoard_ReadBAR( HAL_BASEREGION_SYSCTRL, HAL_BASEADDR_SYSCTRL + SYSCTRL_SELECT_OFFS );
        if (FPGA_READ_ERROR == curValue) // well, we're checking for 0xDEADBEEF here, which in fact is a valid register value...
        {
            UPDATE_RESULT( result, RET_FAILURE );
        }
        else
        {
            // calc new value
            if (activate)
            {
                // assert reset    (low-active ones) | (high-active ones)
                newValue = (curValue & ~extdev_mask) | (highact_extdev & extdev_mask);
            }
            else
            {
                // deassert reset (low-active ones) & ~(high-active ones)
                newValue = (curValue | extdev_mask) & ~(highact_extdev & extdev_mask);
            }

            // set new select reg value
            uint32_t alteraResult = AlteraFPGABoard_WriteBAR( HAL_BASEREGION_SYSCTRL, HAL_BASEADDR_SYSCTRL + SYSCTRL_SELECT_OFFS, newValue );
            if ( alteraResult != FPGA_RES_OK )
            {
                UPDATE_RESULT( result, RET_FAILURE );
            }
        }
    }

    return result;
}


/******************************************************************************
 * HalSetClock()
 *****************************************************************************/
RESULT HalSetClock( HalHandle_t HalHandle, uint32_t dev_mask, uint32_t frequency )
{
    RESULT result = RET_SUCCESS;

    if (HalHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    // check for valid devices
    if (dev_mask & ~(HAL_DEVID_CAM_1A | HAL_DEVID_CAM_1B | HAL_DEVID_CAM_2 | HAL_DEVID_PCLK))
    {
        return RET_INVALID_PARM;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        return RET_WRONG_STATE;
    }

    // set frequencies for all devices individually
    if (dev_mask & (HAL_DEVID_CAM_1A | HAL_DEVID_CAM_1B | HAL_DEVID_CAM_2))
    {
        const AlteraFPGAPLLConfig_t *pPLLConf = NULL;

        // check for frequency being one of the (currently) hard coded values
        //TODO: calc settings dynamically
        switch (frequency)
        {
            case   6000000ul:
                pPLLConf = &pll_cfg_cam1_06_0;
                break;
            case  10000000ul:
                pPLLConf = &pll_cfg_cam1_10_0;
                break;
            case  20000000ul:
                pPLLConf = &pll_cfg_cam1_20_0;
                break;
            case  24000000ul:
                pPLLConf = &pll_cfg_cam1_24_0;
                break;
            default:
                UPDATE_RESULT( result, RET_OUTOFRANGE );
        }

        // new config to set?
        if (pPLLConf != NULL)
        {
            uint32_t alteraResult = AlteraFPGABoard_SetPLLConfig( pll_cam1, pPLLConf );
            if ( alteraResult != FPGA_RES_OK )
            {
                UPDATE_RESULT( result, RET_FAILURE );
            }
        }
    }

    if (dev_mask & HAL_DEVID_PCLK)
    {
        const AlteraFPGAPLLConfig_t *pPLLConf = NULL;

        // check for frequency being one of the (currently) hard coded values
        //TODO: calc settings dynamically
        switch (frequency)
        {
            case  25200000ul:
                pPLLConf = &pll_cfg_vdupclk_25_2;
                break;
            case  74250000ul:
                pPLLConf = &pll_cfg_vdupclk_74_25;
                break;
            case 148500000ul:
                pPLLConf = &pll_cfg_vdupclk_148_5;
                break;
            default:
                UPDATE_RESULT( result, RET_OUTOFRANGE );
        }

        // new config to set?
        if (pPLLConf != NULL)
        {
            uint32_t alteraResult = AlteraFPGABoard_SetPLLConfig( pll_vdupclk, pPLLConf );
            if ( alteraResult != FPGA_RES_OK )
            {
                UPDATE_RESULT( result, RET_FAILURE );
            }
        }
    }

    return result;
}


/******************************************************************************
 * HalAllocMemory()
 *****************************************************************************/
uint32_t HalAllocMemory( HalHandle_t HalHandle, uint32_t byte_size )
{
    if (HalHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        return RET_WRONG_STATE;
    }

    return ExtMemAlloc( byte_size );
}


/******************************************************************************
 * HalFreeMemory()
 *****************************************************************************/
RESULT HalFreeMemory( HalHandle_t HalHandle, ulong_t mem_address )
{
    if (HalHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        return RET_WRONG_STATE;
    }

    ExtMemFree( mem_address );

    return RET_SUCCESS;
}


/******************************************************************************
 * HalReadMemory()
 *****************************************************************************/
RESULT HalReadMemory( HalHandle_t HalHandle, ulong_t mem_address, uint8_t* p_read_buffer, uint32_t byte_size )
{
    if (HalHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        return RET_WRONG_STATE;
    }

    //DCT_ASSERT((mem_address & (DMA_STRIDE_BYTES-1)) == 0);
    DCT_ASSERT((byte_size & (DMA_STRIDE_BYTES-1)) == 0);

#ifdef HAL_USE_RAW_DMA
    return ( AlteraFPGABoard_RawDMARead( p_read_buffer, mem_address, byte_size ) == 0) ? RET_SUCCESS : RET_FAILURE;
#else
    return ( AlteraFPGABoard_DMARead( p_read_buffer, mem_address, byte_size ) == 0) ? RET_SUCCESS : RET_FAILURE;
#endif
}


/******************************************************************************
 * HalWriteMemory()
 *****************************************************************************/
RESULT HalWriteMemory( HalHandle_t HalHandle, ulong_t mem_address, uint8_t* p_write_buffer, uint32_t byte_size )
{
    if (HalHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        return RET_WRONG_STATE;
    }

    //DCT_ASSERT((mem_address & (DMA_STRIDE_BYTES-1)) == 0);
    DCT_ASSERT((byte_size & (DMA_STRIDE_BYTES-1)) == 0);

#ifdef HAL_USE_RAW_DMA
    return ( AlteraFPGABoard_RawDMAWrite( p_write_buffer, mem_address, byte_size ) == 0) ? RET_SUCCESS : RET_FAILURE;
#else
    return ( AlteraFPGABoard_DMAWrite( p_write_buffer, mem_address, byte_size ) == 0) ? RET_SUCCESS : RET_FAILURE;
#endif
}


/******************************************************************************
 * HalMapMemory()
 *****************************************************************************/
RESULT HalMapMemory( HalHandle_t HalHandle, ulong_t mem_address, uint32_t byte_size, HalMapMemType_t mapping_type, void **pp_mapped_buf )
{
    RESULT result = RET_SUCCESS;
    void *p_bufbase;
    HalMemMap_t *pMapData;
    void *p_mapped_buffer;

    if ( (HalHandle == NULL) || (pp_mapped_buf == NULL) )
    {
        return RET_NULL_POINTER;
    }

    if ( (byte_size == 0)
      || (mem_address < EXT_MEM_BASE)
      || (mem_address >= (EXT_MEM_BASE + EXT_MEM_SIZE))
      || ((mem_address+byte_size) >= (EXT_MEM_BASE + EXT_MEM_SIZE)) )
    {
        return RET_OUTOFRANGE;
    }

    switch (mapping_type)
    {
        case HAL_MAPMEM_READWRITE:
        case HAL_MAPMEM_READONLY:
        case HAL_MAPMEM_WRITEONLY:
            break;
        default:
            return RET_INVALID_PARM;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        return RET_WRONG_STATE;
    }

    // allocate container buffer for mapping descriptor & local buffer
    p_bufbase = malloc( ALIGN_UP( byte_size + sizeof(HalMemMap_t) + DMA_MEM_ALIGN, DMA_STRIDE_BYTES ) );
    if (p_bufbase == NULL)
    {
        return RET_OUTOFMEM;
    }

    // calc aligned local buffer address from buffer base; leaving space for mapping descriptor in front of it
    p_mapped_buffer = (void*) ALIGN_UP( ((ulong_t)p_bufbase) + sizeof(HalMemMap_t), DMA_MEM_ALIGN );

    // put mapping descriptor directly below local buffer
    pMapData = (HalMemMap_t*)( ((HalMemMap_t*)p_mapped_buffer) - 1 );

    // store buffer data in descriptor
    pMapData->mem_address  = mem_address;
    pMapData->byte_size    = ALIGN_UP( byte_size, DMA_STRIDE_BYTES );
    pMapData->mapping_type = mapping_type;
    pMapData->p_bufbase    = p_bufbase;

    // 'map' buffer by reading it into local buffer?
    if (mapping_type != HAL_MAPMEM_WRITEONLY)
    {
        result = HalReadMemory( HalHandle, pMapData->mem_address, p_mapped_buffer, pMapData->byte_size );
        if (result != RET_SUCCESS)
        {
            // release container buffer
            free( p_bufbase );
            return result;
        }
    }

    // return mapped buffer
    *pp_mapped_buf = p_mapped_buffer;

    return result;
}


/******************************************************************************
 * HalUnMapMemory()
 *****************************************************************************/
RESULT HalUnMapMemory( HalHandle_t HalHandle, void* p_mapped_buf )
{
    RESULT result = RET_SUCCESS;
    HalMemMap_t *pMapData;

    if ( (HalHandle == NULL) || (p_mapped_buf == NULL) )
    {
        return RET_NULL_POINTER;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        return RET_WRONG_STATE;
    }

    // get mapping descriptor from mapped address
    pMapData = ((HalMemMap_t*)p_mapped_buf) - 1;

    // 'unmap' buffer by writing it back into local buffer?
    if (pMapData->mapping_type != HAL_MAPMEM_READONLY)
    {
        result = HalWriteMemory( HalHandle, pMapData->mem_address, p_mapped_buf, pMapData->byte_size );
    }

    // release container buffer of mapping descriptor & local buffer
    free( pMapData->p_bufbase );

    return result;
}


/******************************************************************************
 * HalReadI2CMem()
 *****************************************************************************/
RESULT HalReadI2CMem
(
    HalHandle_t HalHandle,
    uint8_t     bus_num,
    uint16_t    slave_addr,
    ulong_t     reg_address,
    uint8_t     reg_addr_size,
    uint8_t     *p_read_buffer,
    uint32_t    byte_size
)
{
    RESULT result = RET_SUCCESS;
    OSLAYER_STATUS osStatus;
    I2cReturnType i2c_result;

    if ( (HalHandle == NULL) || (p_read_buffer == NULL) )
    {
        return RET_NULL_POINTER;
    }

    if ( (bus_num >= NUM_I2C) || (reg_addr_size > 4) )
    {
        return RET_INVALID_PARM;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        return RET_WRONG_STATE;
    }

    // lock controller
    osStatus = osMutexLock( &pHalCtx->iicMutex[bus_num] );
    if (OSLAYER_OK != osStatus)
    {
        TRACE( HAL_ERROR, "Can't lock I2C bus #%d\n", bus_num );
        return RET_FAILURE;
    }

    // access controller
    {
        i2c_result = i2c_read_ex( &pHalCtx->iicConfig[bus_num], slave_addr, reg_address, reg_addr_size, p_read_buffer, byte_size );
        if (I2C_RET_SUCCESS != i2c_result)
        {
            TRACE( HAL_ERROR, "Access on I2C bus #%d failed (i2c_result=%d)\n", bus_num, i2c_result );
            UPDATE_RESULT( result, RET_FAILURE );
        }
    }

    // unlock controller again
    osStatus = osMutexUnlock( &pHalCtx->iicMutex[bus_num] );
    if (OSLAYER_OK != osStatus)
    {
        TRACE( HAL_ERROR, "Can't unlock I2C bus #%d\n", bus_num );
        UPDATE_RESULT( result, RET_FAILURE );
    }

    return result;
}


/******************************************************************************
 * HalWriteI2CMem()
 *****************************************************************************/
RESULT HalWriteI2CMem
(
    HalHandle_t HalHandle,
    uint8_t     bus_num,
    uint16_t    slave_addr,
    ulong_t     reg_address,
    uint8_t     reg_addr_size,
    uint8_t     *p_write_buffer,
    uint32_t    byte_size
)
{
    RESULT result = RET_SUCCESS;
    OSLAYER_STATUS osStatus;
    I2cReturnType i2c_result;

    if ( (HalHandle == NULL) || (p_write_buffer == NULL) )
    {
        return RET_NULL_POINTER;
    }

    if ( (bus_num >= NUM_I2C) || (reg_addr_size > 4) )
    {
        return RET_INVALID_PARM;
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        return RET_WRONG_STATE;
    }

    // lock controller
    osStatus = osMutexLock( &pHalCtx->iicMutex[bus_num] );
    if (OSLAYER_OK != osStatus)
    {
        TRACE( HAL_ERROR, "Can't lock I2C bus #%d\n", bus_num );
        return RET_FAILURE;
    }

    // access controller
    {
        i2c_result = i2c_write_ex( &pHalCtx->iicConfig[bus_num], slave_addr, reg_address, reg_addr_size, p_write_buffer, byte_size );
        if (I2C_RET_SUCCESS != i2c_result)
        {
            TRACE( HAL_ERROR, "Access on I2C bus #%d failed (i2c_result=%d)\n", bus_num, i2c_result );
            UPDATE_RESULT( result, RET_FAILURE );
        }
    }

    // unlock controller again
    osStatus = osMutexUnlock( &pHalCtx->iicMutex[bus_num] );
    if (OSLAYER_OK != osStatus)
    {
        TRACE( HAL_ERROR, "Can't unlock I2C bus #%d\n", bus_num );
        UPDATE_RESULT( result, RET_FAILURE );
    }

    return result;
}


/******************************************************************************
 * HalConnectIrq()
 *****************************************************************************/
RESULT HalConnectIrq
(
    HalHandle_t HalHandle,
    HalIrqCtx_t *pIrqCtx,
    uint32_t    int_src,
    osIsrFunc   IsrFunction,
    osDpcFunc   DpcFunction,
    void*       pContext
)
{
    uint32_t fpgaResult;

    if ( (!HalHandle) || (!pIrqCtx) )
    {
        return ( RET_NULL_POINTER );
    }

    // get context from handle & check
    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
    if (pHalCtx->refCount == 0)
    {
        return RET_WRONG_STATE;
    }

    if( HalAddRef( HalHandle ) != RET_SUCCESS )
    {
        return ( RET_FAILURE );
    }
    pIrqCtx->HalHandle = HalHandle;


    osInterrupt *pOsIrq = &pIrqCtx->OsIrq;

    pOsIrq->IsrFunc   = IsrFunction;
    pOsIrq->DpcFunc   = DpcFunction;
    pOsIrq->irq_num   = int_src;
    pOsIrq->p_context = pContext;

    if( osMutexInit( &pOsIrq->isr_access_lock ) != OSLAYER_OK )
    {
        HalDelRef( pIrqCtx->HalHandle );

        return ( RET_FAILURE );
    }

	if( osEventInit( &pOsIrq->isr_event, 1, 0 ) != OSLAYER_OK )
	{
	    osMutexDestroy( &pOsIrq->isr_access_lock );
        HalDelRef( pIrqCtx->HalHandle );

        return ( RET_FAILURE );
	}

	if( osEventInit( &pOsIrq->isr_exit_event, 1, 0 ) != OSLAYER_OK )
	{
	    osMutexDestroy( &pOsIrq->isr_access_lock );
		osEventDestroy( &pOsIrq->isr_event );
        HalDelRef( pIrqCtx->HalHandle );

        return ( RET_FAILURE );
	}

    fpgaResult = AlteraFPGABoard_SetupIRQ( &pIrqCtx->AlteraIrqHandle, pIrqCtx->misRegAddress, pIrqCtx->icrRegAddress, 0 );
    if ( fpgaResult != FPGA_RES_OK )
    {
	    osMutexDestroy( &pOsIrq->isr_access_lock );
		osEventDestroy( &pOsIrq->isr_event );
		osEventDestroy( &pOsIrq->isr_exit_event );
        HalDelRef( pIrqCtx->HalHandle );

        return ( RET_FAILURE );
    }

    if( osThreadCreate(&pOsIrq->isr_thread, halIsrHandler, pIrqCtx) != OSLAYER_OK )
	{
	    AlteraFPGABoard_StopIRQ( &pIrqCtx->AlteraIrqHandle );
	    osMutexDestroy( &pOsIrq->isr_access_lock );
		osEventDestroy( &pOsIrq->isr_event );
		osEventDestroy( &pOsIrq->isr_exit_event );
        HalDelRef( pIrqCtx->HalHandle );

        return ( RET_FAILURE );
	}

	(void)osThreadSetPriority( &pOsIrq->isr_thread, OSLAYER_THREAD_PRIO_HIGHEST );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * HalDisconnectIrq()
 *****************************************************************************/
RESULT HalDisconnectIrq
(
    HalIrqCtx_t *pIrqCtx
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    if (pIrqCtx == NULL)
    {
        return RET_NULL_POINTER;
    }

    //TODO: add proper error checking below

    osInterrupt *pOsIrq = &pIrqCtx->OsIrq;

    uint32_t fpgaResult;

    (void)osEventSignal( &pOsIrq->isr_exit_event );
    (void)osEventSignal( &pOsIrq->isr_event );

    fpgaResult = AlteraFPGABoard_CancelIRQ ( &pIrqCtx->AlteraIrqHandle );
    fpgaResult = AlteraFPGABoard_StopIRQ ( &pIrqCtx->AlteraIrqHandle );
    if ( fpgaResult == FPGA_RES_OK )
    {
        (void)osThreadClose( &pOsIrq->isr_thread );
    }
    UPDATE_RESULT( result, (fpgaResult == FPGA_RES_OK) ? RET_SUCCESS : RET_FAILURE );

    (void)osEventDestroy( &pOsIrq->isr_exit_event );
    (void)osEventDestroy( &pOsIrq->isr_event );

    /* destroy mutex */
    (void)osMutexDestroy( &pOsIrq->isr_access_lock );

    pOsIrq->IsrFunc   = NULL;
    pOsIrq->DpcFunc   = NULL;
    pOsIrq->irq_num   = 0;
    pOsIrq->p_context = NULL;

    lres = HalDelRef( pIrqCtx->HalHandle );
    UPDATE_RESULT( result, lres );

    return result;
}

/******************************************************************************
 * Local functions
 *****************************************************************************/


/******************************************************************************
 * halIsrHandler
 *****************************************************************************/
static int32_t halIsrHandler( void *pArg )
{
    HalIrqCtx_t *pIrqCtx = (HalIrqCtx_t *)pArg;
    uint32_t result;

    while( osEventTimedWait( &(pIrqCtx->OsIrq.isr_exit_event), 0) == OSLAYER_TIMEOUT )
    {

        /* check again if the ISR thread should have been quit by now */
        if ( osEventTimedWait( &(pIrqCtx->OsIrq.isr_exit_event), 0) != OSLAYER_TIMEOUT )
        {
            break;
        }

        {
            int32_t ret_altera;

            result = AlteraFPGABoard_WaitForIRQ( &pIrqCtx->AlteraIrqHandle, &pIrqCtx->misValue );
            if( result != FPGA_RES_OK )
            {
                /* no interrupt has been signaled,
                 * maybe we shall terminate simply look at the exit event again */
                continue;
            }
        }

        if ( ( pIrqCtx->OsIrq.DpcFunc )
          && ( (pIrqCtx->OsIrq.DpcFunc)( (void *)pIrqCtx ) != 0 ) )
        {
            return ( -1 );
        }
	}

    return ( 0 );
}


/******************************************************************************
 * HalLockContext
 *****************************************************************************/
static RESULT HalLockContext( HalContext_t *pHalCtx )
{
    // enforce validity of context
    DCT_ASSERT( pHalCtx && pHalCtx->refCount );

    // lock context
    if( osMutexLock( &pHalCtx->modMutex ) != OSLAYER_OK )
    {
        TRACE( HAL_ERROR, "Locking context failed.\n" );
        return ( RET_FAILURE );
    }

    return RET_SUCCESS;
}

/******************************************************************************
 * HalUnlockContext
 *****************************************************************************/
static RESULT HalUnlockContext( HalContext_t *pHalCtx )
{
    // enforce validity of context
    DCT_ASSERT( pHalCtx && pHalCtx->refCount );

    // unlock context
    if( osMutexUnlock( &pHalCtx->modMutex ) != OSLAYER_OK )
    {
        TRACE( HAL_ERROR, "Unlocking context failed.\n" );
        return ( RET_FAILURE );
    }

    return RET_SUCCESS;
}


#endif // defined ( HAL_ALTERA )
