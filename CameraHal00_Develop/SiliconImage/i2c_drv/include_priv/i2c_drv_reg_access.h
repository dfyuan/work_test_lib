#ifndef __REG__ACCESS_H__
#define __REG__ACCESS_H__

#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <hal/hal_api.h>

USE_TRACER(I2C_DRV_INFO);
USE_TRACER(I2C_DRV_ERROR);


/******************************************************************************
 * API function prototypes
 *****************************************************************************/

/*****************************************************************************/
/**
 *          i2c_ctrl_reg_read8()
 *
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
static inline uint8_t i2c_ctrl_reg_read8(const i2c_bus_t*i2c, uint8_t addr);

/*****************************************************************************/
/**
 *          i2c_ctrl_reg_write8()
 *
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
static inline int32_t i2c_ctrl_reg_write8(const i2c_bus_t*i2c, const uint8_t addr, uint8_t data);


/******************************************************************************
 * API implementation
 *****************************************************************************/


/******************************************************************************
 * API implementation ALTERA
 *****************************************************************************/
#if defined ( HAL_ALTERA )

/******************************************************************************
 * i2c_ctrl_reg_read8()
 *****************************************************************************/
static inline uint8_t i2c_ctrl_reg_read8(const i2c_bus_t*i2c, uint8_t addr)
{
    return ( (uint8_t)AlteraFPGABoard_ReadBAR(HAL_BASEREGION_I2C, (i2c->ulControllerBaseAddress + FPGA_REG_ADDRESS_MOD(addr))) );
}


/******************************************************************************
 * i2c_ctrl_reg_write8()
 *****************************************************************************/
static inline int32_t i2c_ctrl_reg_write8(const i2c_bus_t*i2c, const uint8_t addr, uint8_t data)
{
    uint32_t result;

    result = AlteraFPGABoard_WriteBAR(HAL_BASEREGION_I2C, i2c->ulControllerBaseAddress + FPGA_REG_ADDRESS_MOD(addr), (uint32_t)data);
    if ( result != FPGA_RES_OK )
    {
        TRACE( I2C_DRV_ERROR, "%s (writing 0x%02x to 0x%08x failed with %d) \n",
                __FUNCTION__, data, (i2c->ulControllerBaseAddress + FPGA_REG_ADDRESS_MOD(addr)), result);
        return ( 1 );
    }

    return ( 0 );
}

#endif /* HAL_ALTERA */


/******************************************************************************
 * API implementation MOCKUP
 *****************************************************************************/
#if defined ( HAL_MOCKUP )

/******************************************************************************
 * i2c_ctrl_reg_read8()
 *****************************************************************************/
static inline uint8_t i2c_ctrl_reg_read8(const i2c_bus_t*i2c, uint8_t addr)
{
    return ( 0xCB );
}


/******************************************************************************
 * i2c_ctrl_reg_write8()
 *****************************************************************************/
static inline int32_t i2c_ctrl_reg_write8(const i2c_bus_t*i2c, const uint8_t addr, uint8_t data)
{
    return ( 0 );
}

#endif /* HAL_MOCKUP */


#endif /* __REG__ACCESS_H__ */
