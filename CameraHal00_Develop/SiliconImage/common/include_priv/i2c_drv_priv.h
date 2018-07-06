#ifndef __I2C_DRV_PRIV_H__
#define __I2C_DRV_PRIV_H__

#define I2C_10_BIT_ADDRESS 0x8000  //!< needs to be ORed into slave_addr parameter of
                                   //!< i2c_read/i2c_write to enable 10-bit slave address
#define I2C_DONT_SET_SLAVE_ADDRESS 0xFFFF


/* 8 bit register definitions */
#define I2C_TX                  0x00
#define I2C_RX                  0x00
#define I2C_CMD_REG             0x04
#define I2C_STATUS              0x04
#define I2C_M_ADDR_L            0x08
#define I2C_M_ADDR_H            0x0C
#define I2C_S_ADDR_L            0x10
#define I2C_S_ADDR_H            0x14
#define I2C_CFG_L               0x18
#define I2C_CFG_H               0x1C

/* bit definitions for CMD register */
#define I2C_PIPE_MODE_MASK      0x80
#define I2C_PIPE_MODE_SHIFT     7
#define I2C_DLEN_MASK           0x40
#define I2C_DLEN_SHIFT          6
#define I2C_TX_DATA_VAL_MASK    0x20
#define I2C_TX_DATA_VAL_SHIFT   5
#define I2C_CMD_MASK            0x1F
#define I2C_CMD_SHIFT           0

/* bit definitions for STATUS register */
#define I2C_BUS_STAT_MASK       0x80
#define I2C_BUS_STAT_SHIFT      7
#define I2C_RX_DATA_EN_MASK     0x40
#define I2C_RX_DATA_EN_SHIFT    6
#define I2C_S_CMD_LOCK_MASK     0x20
#define I2C_S_CMD_LOCK_SHIFT    5
#define I2C_M_CMD_LOCK_MASK     0x10
#define I2C_M_CMD_LOCK_SHIFT    4
#define I2C_IRQ_CODE_MASK       0x0F
#define I2C_IRQ_CODE_SHIFT      0

/* bit definitions for master ADDR register */
// LOW
#define I2C_M_ADDR_MASK         0xFE
#define I2C_M_ADDR_SHIFT        1
#define I2C_M_ADDR_MODE_MASK    0x01
#define I2C_M_ADDR_MODE_SHIFT   0
// HIGH
#define I2C_M_ADDR10_MASK       0x07
#define I2C_M_ADDR10_SHIFT      0

/* bit definitions for slave ADDR register */
// LOW
#define I2C_S_ADDR_MASK         0xFE
#define I2C_S_ADDR_SHIFT        1
#define I2C_S_ADDR_MODE_MASK    0x01
#define I2C_S_ADDR_MODE_SHIFT   0
// HIGHT
#define I2C_S_GCA_IRQ_EN_MASK   0x08
#define I2C_S_GCA_IRQ_EN_SHIFT  7
#define I2C_S_ADDR10_MASK       0x07
#define I2C_S_ADDR10_SHIFT      0

/* bit definitions for CFG register*/
#define I2C_SCL_REF_MASK        0xFF
#define I2C_SCL_REF_SHIFT       1
#define I2C_IRQ_DIS_MASK        0x80
#define I2C_IRQ_DIS_SHIFT       7
#define I2C_TIMING_MODE_MASK    0x40
#define I2C_TIMING_MODE_SHIFT   6
#define I2C_VSCD_MASK           0x20
#define I2C_VSCD_SHIFT          5
#define I2C_SPIKE_FILTER_MASK   0x1E
#define I2C_SPIKE_FILTER_SHIFT  1
#define I2C_CLK_REF9_MASK       0x01
#define I2C_CLK_REF9_SHIFT      0

/* Commands */
#define M_READ                  0x04 // start reading bytes from slave
#define M_WRITE                 0x05 // start writing bytes to slave
#define M_STOP                  0x06 // stop transaction

/* Interrupt Codes */
#define NO_IRQ                  0x0 // no interrupt active
#define M_ARBIT_LOST            0x1 // master arbitration lost, the transfer has to be repeated
#define M_NO_ACK                0x2 // master no acknowledge by slave
#define M_TX_DATA               0x3 // master tx data required in register TX_DATA
#define M_RX_DATA               0x4 // master rx data available in register RX_DATA
#define M_CMD_ACK               0x5 // master command acknowledge interrupt

#endif /* __I2C_DRV_PRIV_H__ */
