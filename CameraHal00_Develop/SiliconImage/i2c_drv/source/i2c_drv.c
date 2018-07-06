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
 * @file i2c_drv.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include "i2c_drv.h"

#include "i2c_drv_priv.h"
#include "i2c_drv_reg_access.h"


CREATE_TRACER(I2C_DRV_INFO, "I2C-DRV: ", INFO   , 0);
CREATE_TRACER(I2C_DRV_WARN, "I2C-DRV: ", WARNING, 0);
CREATE_TRACER(I2C_DRV_ERROR,"I2C-DRV: ", ERROR  , 0);

USE_TRACER(I2C_DRV_INFO);
USE_TRACER(I2C_DRV_WARN);
USE_TRACER(I2C_DRV_ERROR);

/* Interrupt names (for error message output) */
static const char* irq_name[] = { "timeout", "M_ARBIT_LOST", "M_NO_ACK", "M_TX_DATA", "M_RX_DATA", "M_CMD_ACK" };

/* Interrupt polling macro */
#define I2C_POLL_FOR_IRQ(irq) \
        do { \
            uint32_t mask, retries = 0x40000;  \
            for (mask = 0;  !mask && retries;  --retries) \
            { \
                mask = (i2c_ctrl_reg_read8(i2c, I2C_STATUS) & I2C_IRQ_CODE_MASK);  \
            } \
            if (mask != irq) \
            { \
                TRACE(I2C_DRV_ERROR, "Error in %s() - unexpected interrupt %d (%s) while waiting for %s\n", \
                        __FUNCTION__, mask, (mask <= M_CMD_ACK) ? irq_name[mask] : "unknown", irq_name[irq]); \
                return I2C_RET_WRITE_ERROR; \
            } \
        } while (0)


/*****************************************************************************/
/**
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
static I2cReturnType i2c_ctrl_set_slave_addr
(
    const i2c_bus_t* i2c,
    const uint16_t slave_addr
)
{
    TRACE( I2C_DRV_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( slave_addr == I2C_DONT_SET_SLAVE_ADDRESS )
    {
        return ( I2C_RET_SUCCESS );
    }

    if ( (slave_addr & I2C_10_BIT_ADDRESS) || (slave_addr > 0x7F) )
    {
        if ( i2c_ctrl_reg_write8(i2c, I2C_M_ADDR_L, ((slave_addr & 0x007F) << 1) | 1) != 0 )
        {
            return ( I2C_RET_WRITE_ERROR );
        }
        if ( i2c_ctrl_reg_write8(i2c, I2C_M_ADDR_H,  (slave_addr & 0x0380) >> 7) != 0 )
        {
            return ( I2C_RET_WRITE_ERROR );
        }
    }
    else
    {
        if ( i2c_ctrl_reg_write8(i2c, I2C_M_ADDR_L, (slave_addr & 0x007F) << 1) != 0 )
        {
            return ( I2C_RET_WRITE_ERROR );
        }
        if ( i2c_ctrl_reg_write8(i2c, I2C_M_ADDR_H, 0) != 0 )
        {
            return ( I2C_RET_WRITE_ERROR );
        }
    }

    TRACE( I2C_DRV_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( I2C_RET_SUCCESS );
}



/*****************************************************************************/
/**
 *          i2c_read()
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
static I2cReturnType i2c_read
(
    const i2c_bus_t *i2c,
    const uint16_t  slave_addr,
    uint8_t         *data,
    int32_t         size
)
{
    I2cReturnType result;

    TRACE( I2C_DRV_INFO, "%s (enter)\n", __FUNCTION__ );

    result = i2c_ctrl_set_slave_addr(i2c, slave_addr);
    if ( result != I2C_RET_SUCCESS )
    {
        return ( result );
    }

    if ( i2c_ctrl_reg_write8(i2c, I2C_CMD_REG, M_READ) != 0 )
    {
        return ( I2C_RET_FAILURE );
    }

    do
    {
        I2C_POLL_FOR_IRQ( M_RX_DATA );

        --size;

        if ( size > 0 )
        {
            *data++ = (uint8_t)i2c_ctrl_reg_read8(i2c, I2C_RX);
        }
        else
        {
            if ( i2c_ctrl_reg_write8(i2c, I2C_CMD_REG, M_STOP) != 0 )
            {
                return ( I2C_RET_WRITE_ERROR );
            }
            *data++ = (uint8_t)i2c_ctrl_reg_read8(i2c, I2C_RX);
            I2C_POLL_FOR_IRQ( M_CMD_ACK );
        }
    }
    while ( size );

    TRACE( I2C_DRV_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( I2C_RET_SUCCESS );
}



/*****************************************************************************/
/**
 *          i2c_write()
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
static I2cReturnType i2c_write
(
    const i2c_bus_t *i2c,
    const uint16_t  slave_addr,
    const uint8_t   *data,
    int32_t         size
)
{
    I2cReturnType result;

    TRACE( I2C_DRV_INFO, "%s (enter)\n", __FUNCTION__ );

    result = i2c_ctrl_set_slave_addr(i2c, slave_addr);
    if ( result != I2C_RET_SUCCESS )
    {
        return ( result );
    }

    if ( i2c_ctrl_reg_write8(i2c, I2C_CMD_REG, M_WRITE) != 0 )
    {
        return ( I2C_RET_WRITE_ERROR );
    }

    while ( size )
    {
        if ( i2c_ctrl_reg_write8(i2c, I2C_TX, *data) != 0 )
        {
            return ( I2C_RET_WRITE_ERROR );
        }
        I2C_POLL_FOR_IRQ( M_TX_DATA );

        data++;
        size--;
    }

    if ( i2c_ctrl_reg_write8(i2c, I2C_CMD_REG, M_STOP) != 0 )
    {
        return ( I2C_RET_WRITE_ERROR );
    }

    I2C_POLL_FOR_IRQ( M_CMD_ACK );

    TRACE( I2C_DRV_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( I2C_RET_SUCCESS );
}



/******************************************************************************
 * i2c_init
 *****************************************************************************/
I2cReturnType i2c_init
(
    const i2c_bus_t* i2c
)
{
    uint32_t value;

    TRACE( I2C_DRV_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( !i2c )
    {
        return ( RET_NULL_POINTER );
    }

    if ( (i2c->ulSclRef & 0x03FF) < 4 )
    {
        /* bit 0 of the Clock Devider Value must be 0 */
        TRACE( I2C_DRV_ERROR, "%s (invalid SclRef) \n", __FUNCTION__ );
        return ( I2C_RET_INVALID_PARM );
    }

    if ( ((i2c->aucMasterAddrMode & 1) == 0) && ((i2c->ulMasterAddr & 0x380) != 0) )
    {
        /* master address out of range for 7-bit mod */
        TRACE( I2C_DRV_ERROR, "%s (invalid MasterAddrMode) \n", __FUNCTION__ );
        return ( I2C_RET_INVALID_PARM );
    }

    value =   ((i2c->ulSclRef                     >> 9)                      & I2C_CLK_REF9_MASK)
            | ((i2c->aucVirtualClockDividerEnable << I2C_VSCD_SHIFT)         & I2C_VSCD_MASK)
            | ((i2c->aucTimingMode                << I2C_TIMING_MODE_SHIFT)  & I2C_TIMING_MODE_MASK)
            | ((i2c->aucSpikeFilter               << I2C_SPIKE_FILTER_SHIFT) & I2C_SPIKE_FILTER_MASK)
            | ((i2c->aucIrqDisable                << I2C_IRQ_DIS_SHIFT)      & I2C_IRQ_DIS_MASK) ;

    if ( i2c_ctrl_reg_write8(i2c, I2C_CFG_H, value) != 0 )
    {
        TRACE( I2C_DRV_ERROR, "%s (set I2C_CFG_H to 0x%x failed) \n", __FUNCTION__, value);
        return ( I2C_RET_WRITE_ERROR );
    }

    value = ( (i2c->ulSclRef >> I2C_SCL_REF_SHIFT) & I2C_SCL_REF_MASK );
    if ( i2c_ctrl_reg_write8(i2c, I2C_CFG_L, value) != 0 )
    {
        TRACE( I2C_DRV_ERROR, "%s (I2C_CFG_L failed) \n", __FUNCTION__ );
        return ( I2C_RET_WRITE_ERROR );
    }

    TRACE( I2C_DRV_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( I2C_RET_SUCCESS );
}



/******************************************************************************
 * i2c_write_reg8()
 *****************************************************************************/
I2cReturnType i2c_write_reg8
(
    const i2c_bus_t *i2c,
    const uint16_t  slave_addr,
    uint8_t         reg_addr,
    uint8_t         value
)
{
    uint8_t data[2] = { reg_addr, value };

    return ( i2c_write(i2c, slave_addr, data, 2) );
}



/******************************************************************************
 * i2c_write_reg16()
 *****************************************************************************/
I2cReturnType i2c_write_reg16
(
    const i2c_bus_t *i2c,
    const uint16_t  slave_addr,
    uint16_t        reg_addr,
    uint8_t         value
)
{
    uint8_t data[3] = { (uint8_t)((reg_addr>>8) & 0xff) , (uint8_t)(reg_addr & 0xff) , value };

    return ( i2c_write(i2c, slave_addr, data, 3) );
}



/******************************************************************************
 * i2c_write_ex()
 *****************************************************************************/
I2cReturnType i2c_write_ex
(
    const i2c_bus_t *i2c,
    const uint16_t  slave_addr,
    uint32_t        reg_addr,
    uint8_t         reg_addr_size,
    uint8_t         *p_data,
    uint8_t         num_data
)
{
    return i2c_write_ex2(i2c, slave_addr, reg_addr, reg_addr_size, p_data, num_data, NULL);
}

extern I2cReturnType i2c_write_ex2
(
    const i2c_bus_t *i2c,
    const uint16_t  slave_addr,
    uint32_t        reg_addr,
    uint8_t         reg_addr_size,
    uint8_t         *p_data,
    uint8_t         num_data,
    int16_t         *pdata_written
)
{
    uint8_t reg_addr_bus[4];
    uint8_t byte;

    I2cReturnType result;

    // initial condition
    if ( (i2c == NULL) || (reg_addr_size > 4) || ((p_data == NULL) && (num_data != 0)) )
    {
        if (pdata_written) *pdata_written = (int16_t)0x8000; // max negative value, not returned under "normal" error conditions
        return ( I2C_RET_INVALID_PARM );
    }

    // reorder reg_addr to bus order (MSByte first)
    switch (reg_addr_size)
    {
        // intended fall through from 4 via 3, 2, 1 into 0
        case 4:
            reg_addr_bus[3] = (uint8_t)(reg_addr & 0xff);
            reg_addr >>= 8;
        case 3:
            reg_addr_bus[2] = (uint8_t)(reg_addr & 0xff);
            reg_addr >>= 8;
        case 2:
            reg_addr_bus[1] = (uint8_t)(reg_addr & 0xff);
            reg_addr >>= 8;
        case 1:
            reg_addr_bus[0] = (uint8_t)(reg_addr & 0xff);
            reg_addr >>= 8;
        case 0:
            break;
        default:
            return ( I2C_RET_INVALID_PARM );
    }

    // write slave address
    if (pdata_written) *pdata_written = -1; // slave addr phase
    result = i2c_ctrl_set_slave_addr(i2c, slave_addr);
    if ( result != I2C_RET_SUCCESS )
    {
        return ( result );
    }

    // set write command
    if ( i2c_ctrl_reg_write8( i2c, I2C_CMD_REG, M_WRITE ) != 0 )
    {
        return ( I2C_RET_FAILURE );
    }

    // write register address-bytes
    for ( byte = 0; byte < reg_addr_size; byte++ )
    {
        if (pdata_written) *pdata_written -= 1; // next reg addr phase
        if ( i2c_ctrl_reg_write8( i2c, I2C_TX, reg_addr_bus[byte] ) != 0 )
        {
            return ( I2C_RET_FAILURE );
        }
        I2C_POLL_FOR_IRQ( M_TX_DATA );
    }

    // write data bytes
    if (pdata_written) *pdata_written = 0; // first data phase
    while ( num_data )
    {
        if ( i2c_ctrl_reg_write8(i2c, I2C_TX, *p_data) != 0 )
        {
            return ( I2C_RET_WRITE_ERROR );
        }
        I2C_POLL_FOR_IRQ( M_TX_DATA );

        if (pdata_written) *pdata_written += 1; // next data phase
        p_data++;
        num_data--;
    }

    // send stop condition
    if ( i2c_ctrl_reg_write8(i2c, I2C_CMD_REG, M_STOP) != 0 )
    {
        return ( I2C_RET_WRITE_ERROR );
    }
    I2C_POLL_FOR_IRQ( M_CMD_ACK );

    return ( I2C_RET_SUCCESS );
}



/******************************************************************************
 * i2c_read_reg8()
 *****************************************************************************/
I2cReturnType i2c_read_reg8
(
    const i2c_bus_t *i2c,
    const uint16_t  slave_addr,
    uint8_t         reg_addr,
    uint8_t         *data
)
{
    I2cReturnType result;

    int32_t size;

    /* initial condition */
    if ( data == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    /* write slave address */
    result = i2c_ctrl_set_slave_addr(i2c, slave_addr);
    if ( result != I2C_RET_SUCCESS )
    {
        return ( result );
    }

    /* set write command */
    if ( i2c_ctrl_reg_write8( i2c, I2C_CMD_REG, M_WRITE ) != 0 )
    {
        return ( I2C_RET_FAILURE );
    }

    /* write register address */
    if ( i2c_ctrl_reg_write8( i2c, I2C_TX, reg_addr ) != 0 )
    {
        return ( I2C_RET_FAILURE );
    }
    I2C_POLL_FOR_IRQ( M_TX_DATA );

    result = i2c_read( i2c, slave_addr, data, 1 );

    return ( result );
}



/******************************************************************************
 * i2c_read_reg16()
 *****************************************************************************/
I2cReturnType i2c_read_reg16
(
    const i2c_bus_t *i2c,
    const uint16_t  slave_addr,
    uint16_t        reg_addr,
    uint8_t         *data
)
{
    I2cReturnType result;

    /* initial condition */
    if ( data == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    /* write slave address */
    result = i2c_ctrl_set_slave_addr(i2c, slave_addr);
    if ( result != I2C_RET_SUCCESS )
    {
        return ( result );
    }

    /* set write command */
    if ( i2c_ctrl_reg_write8( i2c, I2C_CMD_REG, M_WRITE ) != 0 )
    {
        return ( I2C_RET_FAILURE );
    }

    /* write register high-address-byte */
    if ( i2c_ctrl_reg_write8( i2c, I2C_TX, (uint8_t)((reg_addr>>8) & 0xff) ) != 0 )
    {
        return ( I2C_RET_FAILURE );
    }
    I2C_POLL_FOR_IRQ( M_TX_DATA );

    /* write register low-address-byte */
    if ( i2c_ctrl_reg_write8( i2c, I2C_TX, (uint8_t)(reg_addr & 0xff) ) != 0 )
    {
        return ( I2C_RET_FAILURE );
    }
    I2C_POLL_FOR_IRQ( M_TX_DATA );

    result = i2c_read( i2c, slave_addr, data, 1 );

    return ( result );
}



/******************************************************************************
 * i2c_read_ex()
 *****************************************************************************/
I2cReturnType i2c_read_ex
(
    const i2c_bus_t *i2c,
    const uint16_t  slave_addr,
    uint32_t        reg_addr,
    uint8_t         reg_addr_size,
    uint8_t         *p_data,
    uint8_t         num_data
)
{
    return i2c_read_ex2(i2c, slave_addr, reg_addr, reg_addr_size, p_data, num_data, NULL);
}

I2cReturnType i2c_read_ex2
(
    const i2c_bus_t *i2c,
    const uint16_t  slave_addr,
    uint32_t        reg_addr,
    uint8_t         reg_addr_size,
    uint8_t         *p_data,
    uint8_t         num_data,
    int16_t         *pdata_read
)
{
    uint8_t reg_addr_bus[4];
    uint8_t byte;

    I2cReturnType result;

    // initial condition
    if ( (i2c == NULL) || (reg_addr_size > 4) || ((p_data == NULL) && (num_data != 0)) )
    {
        if (pdata_read) *pdata_read = (int16_t)0x8000; // max negative value, not returned under "normal" error conditions
        return ( I2C_RET_INVALID_PARM );
    }

    // reorder reg_addr to bus order (MSByte first)
    switch (reg_addr_size)
    {
        // intended fall through from 4 via 3, 2, 1 into 0
        case 4:
            reg_addr_bus[3] = (uint8_t)(reg_addr & 0xff);
            reg_addr >>= 8;
        case 3:
            reg_addr_bus[2] = (uint8_t)(reg_addr & 0xff);
            reg_addr >>= 8;
        case 2:
            reg_addr_bus[1] = (uint8_t)(reg_addr & 0xff);
            reg_addr >>= 8;
        case 1:
            reg_addr_bus[0] = (uint8_t)(reg_addr & 0xff);
            reg_addr >>= 8;
        case 0:
            break;
        default:
            return ( I2C_RET_INVALID_PARM );
    }

    // sub adressing?
    if (reg_addr_size)
    {
        // write slave address
        if (pdata_read) *pdata_read = -1; // slave addr phase
        result = i2c_ctrl_set_slave_addr(i2c, slave_addr);
        if ( result != I2C_RET_SUCCESS )
        {
            return ( result );
        }

        // set write command
        if ( i2c_ctrl_reg_write8( i2c, I2C_CMD_REG, M_WRITE ) != 0 )
        {
            return ( I2C_RET_FAILURE );
        }

        // write register address-bytes
        for (byte = 0; byte < reg_addr_size; byte++)
        {
            if (pdata_read) *pdata_read -= 1; // next reg addr phase
            if ( i2c_ctrl_reg_write8( i2c, I2C_TX, reg_addr_bus[byte] ) != 0 )
            {
                return ( I2C_RET_FAILURE );
            }
            I2C_POLL_FOR_IRQ( M_TX_DATA );
        }
    }

    // write slave address again
    if (pdata_read) *pdata_read -= 1; // next slave addr phase
    result = i2c_ctrl_set_slave_addr(i2c, slave_addr);
    if ( result != I2C_RET_SUCCESS )
    {
        return ( result );
    }

    // set read command
    if ( i2c_ctrl_reg_write8(i2c, I2C_CMD_REG, M_READ) != 0 )
    {
        return ( I2C_RET_FAILURE );
    }

    // read data bytes
    if (pdata_read) *pdata_read = 0; // first data phase
    do
    {
        I2C_POLL_FOR_IRQ( M_RX_DATA );

        if (pdata_read) *pdata_read += 1; // next data phase
        --num_data;

        if ( num_data > 0 )
        {
            *p_data++ = (uint8_t)i2c_ctrl_reg_read8(i2c, I2C_RX);
        }
        else
        {
            if ( i2c_ctrl_reg_write8(i2c, I2C_CMD_REG, M_STOP) != 0 )
            {
                return ( I2C_RET_WRITE_ERROR );
            }
            *p_data++ = (uint8_t)i2c_ctrl_reg_read8(i2c, I2C_RX);
            I2C_POLL_FOR_IRQ( M_CMD_ACK );
        }
    }
    while ( num_data );

    return ( result );
}
