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
 * @file mipi_drv_phy_std02.h
 *
 * @brief   Definition of MIPI PHY internals.
 *
 *****************************************************************************/

#ifndef __MIPI_DRV_PHY_STD02_H__
#define __MIPI_DRV_PHY_STD02_H__


/***********************************************/
// I2C slave stuff
#define STMIPID02_SLAVE_ADDR     0x14
#define STMIPID02_REGADDR_SIZE   2

/***********************************************/
// Clock lane 1 registers
#define CLK_LANE_REG1_OFFS                      0x0002
#define CLK_LANE_REG1_UIX4_SHFT                 2
#define CLK_LANE_REG1_UIX4_MASK                 0xfc
#define CLK_LANE_REG1_SWAP_PINS                 0x02
#define CLK_LANE_REG1_ENABLE                    0x01

#define CLK_LANE_REG3_OFFS                      0x0004
#define CLK_LANE_REG3_HS_RX_TERM_CCP            0x10
#define CLK_LANE_REG3_HS_RX_ENABLE_CCP          0x08
#define CLK_LANE_REG3_HS_RX_WAKEUP_CCP          0x04
#define CLK_LANE_REG3_CNTRL_MIPI_SEL            0x02

#define CLK_LANE_WR_REG1_OFFS                   0x0001 // ReadOnly
#define CLK_LANE_WR_REG1_ULP_ACTIVE             0x02
#define CLK_LANE_WR_REG1_STOPPED                0x01

/***********************************************/
// Data lane 1.1 (#0) registers
#define DATA_LANE0_REG1_OFFS                    0x0005
#define DATA_LANE0_REG1_SWAP_PINS_NOT           0x02 // Don't miss the 'NOT' here!
#define DATA_LANE0_REG1_ENABLE                  0x01

#define DATA_LANE0_REG2_OFFS                    0x0006
#define DATA_LANE0_REG2_HS_RX_TERM_CCP          0x08
#define DATA_LANE0_REG2_HS_RX_ENABLE_CCP        0x04
#define DATA_LANE0_REG2_HS_RX_WAKEUP_CCP        0x02
#define DATA_LANE0_REG2_CNTRL_MIPI_SEL          0x01

#define DATA_LANE0_REG3_OFFS                    0x0007 // ReadOnly
#define DATA_LANE0_REG3_ULP_ACTIVE              0x02
#define DATA_LANE0_REG3_STOPPED                 0x01

#define DATA_LANE0_REG4_OFFS                    0x000C // ReadOnly
#define DATA_LANE0_REG4_ERR_CONTROL             0x20
#define DATA_LANE0_REG4_ERR_SYNC_ESC            0x10
#define DATA_LANE0_REG4_ERR_ESC                 0x08
#define DATA_LANE0_REG4_ERR_EOT_SYNC_HS         0x04
#define DATA_LANE0_REG4_ERR_SOT_SYNC_HS         0x02
#define DATA_LANE0_REG4_ERR_SOT_HS              0x01

/***********************************************/
// Data lane 1.2 (#1) registers
#define DATA_LANE1_REG1_OFFS                    0x0009
#define DATA_LANE1_REG1_SWAP_PINS               0x02
#define DATA_LANE1_REG1_ENABLE                  0x01

#define DATA_LANE1_REG2_OFFS                    0x000A
#define DATA_LANE1_REG2_HS_RX_TERM_CCP          0x08
#define DATA_LANE1_REG2_HS_RX_ENABLE_CCP        0x04
#define DATA_LANE1_REG2_HS_RX_WAKEUP_CCP        0x02
#define DATA_LANE1_REG2_CNTRL_MIPI_SEL          0x01

#define DATA_LANE1_REG3_OFFS                    0x000B // ReadOnly
#define DATA_LANE1_REG3_ULP_ACTIVE              0x02
#define DATA_LANE1_REG3_STOPPED                 0x01

#define DATA_LANE1_REG4_OFFS                    0x0008 // ReadOnly
#define DATA_LANE1_REG4_ERR_CONTENTION_LP1      0x80
#define DATA_LANE1_REG4_ERR_CONTENTION_LP0      0x40
#define DATA_LANE1_REG4_ERR_CONTROL             0x20
#define DATA_LANE1_REG4_ERR_SYNC_ESC            0x10
#define DATA_LANE1_REG4_ERR_ESC                 0x08
#define DATA_LANE1_REG4_ERR_EOT_SYNC_HS         0x04
#define DATA_LANE1_REG4_ERR_SOT_SYNC_HS         0x02
#define DATA_LANE1_REG4_ERR_SOT_HS              0x01


/***********************************************/
// Clock lane 2 registers
#define CLK_LANE_REG1_C2_OFFS                   0x0031
#define CLK_LANE_REG1_C2_UIX4_SHFT              2
#define CLK_LANE_REG1_C2_UIX4_MASK              0xfc
#define CLK_LANE_REG1_C2_SWAP_PINS_NOT          0x02 // Don't miss the 'NOT' here!
#define CLK_LANE_REG1_C2_ENABLE                 0x01

#define CLK_LANE_REG3_C2_OFFS                   0x0033
#define CLK_LANE_REG3_C2_HS_RX_TERM_CCP         0x10
#define CLK_LANE_REG3_C2_HS_RX_ENABLE_CCP       0x08
#define CLK_LANE_REG3_C2_HS_RX_WAKEUP_CCP       0x04
#define CLK_LANE_REG3_C2_CNTRL_MIPI_SEL         0x02

#define CLK_LANE_WR_REG1_C2_OFFS                0x0039 // ReadOnly; 0x3A according to Spec...
#define CLK_LANE_WR_REG1_C2_ULP_ACTIVE          0x02
#define CLK_LANE_WR_REG1_C2_STOPPED             0x01

/***********************************************/
// Data lane 2 (#3) registers
#define DATA_LANE3_REG1_OFFS                    0x0034
#define DATA_LANE3_REG1_SWAP_PINS               0x02
#define DATA_LANE3_REG1_ENABLE                  0x01

#define DATA_LANE3_REG2_OFFS                    0x0035
#define DATA_LANE3_REG2_HS_RX_TERM_CCP          0x08
#define DATA_LANE3_REG2_HS_RX_ENABLE_CCP        0x04
#define DATA_LANE3_REG2_HS_RX_WAKEUP_CCP        0x02
#define DATA_LANE3_REG2_CNTRL_MIPI_SEL          0x01

#define DATA_LANE3_REG3_OFFS                    0x003A // ReadOnly
#define DATA_LANE3_REG3_ULP_ACTIVE              0x02
#define DATA_LANE3_REG3_STOPPED                 0x01

#define DATA_LANE3_REG4_OFFS                    0x003B // ReadOnly
#define DATA_LANE3_REG4_ERR_CONTROL             0x20
#define DATA_LANE3_REG4_ERR_SYNC_ESC            0x10
#define DATA_LANE3_REG4_ERR_ESC                 0x08
#define DATA_LANE3_REG4_ERR_EOT_SYNC_HS         0x04
#define DATA_LANE3_REG4_ERR_SOT_SYNC_HS         0x02
#define DATA_LANE3_REG4_ERR_SOT_HS              0x01


/***********************************************/
// CCP RX & error flag registers
#define CCP_RX_REG1_OFFS                        0x000D
#define CCP_RX_REG1_DELAY_SHFT                  3
#define CCP_RX_REG1_DELAY_MASK                  0xf8
#define CCP_RX_REG1_DS_MODE                     0x01

#define CCP_RX_REG2_OFFS                        0x000E
#define CCP_RX_REG2_CLR_GLUE_SYNC_ERROR         0x40
#define CCP_RX_REG2_PIX_WIDTH_CCP_RX_SHIFT      2
#define CCP_RX_REG2_PIX_WIDTH_CCP_RX_MASK       0x3C
#define CCP_RX_REG2_CLR_CCP_SHIFT_SYNC          0x02
#define CCP_RX_REG2_CLR_CCP_CRC_ERROR           0x01

#define CCP_RX_REG3_OFFS                        0x000F // ReadOnly
#define CCP_RX_REG3_GLUE_LOGIC_SYNC_ERROR       0x80
#define CCP_RX_REG3_CCP_CHANNEL_SHIFT           3
#define CCP_RX_REG3_CCP_CHANNEL_MASK            0x78
#define CCP_RX_REG3_CCP_SHIFT_SYNC              0x04
#define CCP_RX_REG3_CCP_FALSE_SYNC              0x02
#define CCP_RX_REG3_CCP_CRC_ERROR               0x01

#define CCP_RX_REG1_C2_OFFS                     0x0038
#define CCP_RX_REG1_C2_DS_MODE                  0x01


/***********************************************/
// Mode control registers
#define MODE_REG1_OFFS                          0x0014
#define MODE_REG1_JUSTIFICATION_CONTROL_LEFT    0x80
#define MODE_REG1_BYPASS_MODE_DISABLE           0x40
#define MODE_REG1_DECOMPRESSION_MODE_MASK       0x38
#define MODE_REG1_DECOMPRESSION_MODE_7_12       0x38
#define MODE_REG1_DECOMPRESSION_MODE_6_12       0x30
#define MODE_REG1_DECOMPRESSION_MODE_10_12      0x28
#define MODE_REG1_DECOMPRESSION_MODE_8_12       0x20
#define MODE_REG1_DECOMPRESSION_MODE_8_10       0x18
#define MODE_REG1_DECOMPRESSION_MODE_7_10       0x10
#define MODE_REG1_DECOMPRESSION_MODE_6_10       0x08
#define MODE_REG1_DECOMPRESSION_MODE_OFF        0x00
#define MODE_REG1_LANE_CONTROL_SWAP_LANES       0x04
#define MODE_REG1_LANE_CONTROL_2_LANES          0x02
#define MODE_REG1_CCP_STREAM_SELECT             0x01

#define MODE_REG2_OFFS                          0x0015
#define MODE_REG2_TRISTATE_OUTPUT_NOT           0x80
#define MODE_REG2_CLEAR_ERROR_SIGNAL_NOT        0x40
#define MODE_REG2_ERROR_SIGNAL_POLARITY_INVERT  0x20
#define MODE_REG2_CLOCK_GATING_ENABLE           0x10
#define MODE_REG2_OUTPUT_POLARITY_CLK           0x80
#define MODE_REG2_OUTPUT_POLARITY_VSYNC         0x40
#define MODE_REG2_OUTPUT_POLARITY_HSYNC         0x20
#define MODE_REG2_INTERRUPT_POLARITY_INVERTED   0x10

#define MODE_REG3_OFFS                          0x0036
#define MODE_REG3_I2C_COMP_LEAKAGE_COMP_ENABLE  0x20
#define MODE_REG3_SPEC_0_81_SEL_C2              0x08
#define MODE_REG3_SPEC_0_81_SEL                 0x04
#define MODE_REG3_CAM_SEL_CAM2                  0x01


/***********************************************/
// clock control register
#define CLOCK_CONTROL_REG1_OFFS                 0x0016
#define CLOCK_CONTROL_REG1_CLR_CSI2_IRQ         0x20
#define CLOCK_CONTROL_REG1_CLR_CSI2_ERR         0x10


/***********************************************/
// system error register
#define ERROR_REG_OFFS                          0x0010
#define ERROR_REG_CHECKSUM_FAILED               0x02
#define ERROR_REG_ECC_FAILED                    0x01


/***********************************************/
// data pipe info register
#define DATA_ID_W_REG_OFFS                      0x0011
#define DATA_ID_R_REG_OFFS                      0x0017
#define DATA_ID_R_EMB_REG_OFFS                  0x0018
#define DATA_ID_REGS_VC_SHIFT                   6
#define DATA_ID_REGS_VC_MASK                    0xC0
#define DATA_ID_REGS_DATA_TYPE_MASK             0x3F

#define DATA_SEL_CTRL_REG_OFFS                  0x0019
#define DATA_SEL_CTRL_REG_PIX_WIDTH_REG_SEL     0x08
#define DATA_SEL_CTRL_REG_DATA_TYPE_REG_SEL     0x04
#define DATA_SEL_CTRL_REG_VC_MASK               0x03

#define FRAME_NO_LSB_REG_OFFS                   0x0012
#define FRAME_NO_MSB_REG_OFFS                   0x0013

#define ACTIVE_LINE_NO_LSB_REG_OFFS             0x001B
#define ACTIVE_LINE_NO_MSB_REG_OFFS             0x001A

#define SOF_LINE_NO_LSB_REG_OFFS                0x001D
#define SOF_LINE_NO_MSB_REG_OFFS                0x001C

#define PIX_WIDTH_CTRL_REG_OFFS                 0x001E
#define PIX_WIDTH_CTRL_REG_DECOMP_ENABLE        0x10
#define PIX_WIDTH_CTRL_REG_MASK                 0x0F

#define PIX_WIDTH_CTRL_EMB_REG_OFFS             0x001F
#define PIX_WIDTH_CTRL_EMB_REG_DECOMP_ENABLE    0x10
#define PIX_WIDTH_CTRL_REG_MASK                 0x0F

#define DATA_FIELD_LSB_REG_OFFS                 0x0021
#define DATA_FIELD_MSB_REG_OFFS                 0x0020


/* sorted register list for convenience only
CLK_LANE_WR_REG1_OFFS                   0x0001 // ReadOnly
CLK_LANE_REG1_OFFS                      0x0002
CLK_LANE_REG3_OFFS                      0x0004
DATA_LANE0_REG1_OFFS                    0x0005
DATA_LANE0_REG2_OFFS                    0x0006
DATA_LANE0_REG3_OFFS                    0x0007 // ReadOnly
DATA_LANE1_REG4_OFFS                    0x0008 // ReadOnly
DATA_LANE1_REG1_OFFS                    0x0009
DATA_LANE1_REG2_OFFS                    0x000A
DATA_LANE1_REG3_OFFS                    0x000B // ReadOnly
DATA_LANE0_REG4_OFFS                    0x000C // ReadOnly
CCP_RX_REG1_OFFS                        0x000D
CCP_RX_REG2_OFFS                        0x000E
CCP_RX_REG3_OFFS                        0x000F // ReadOnly
ERROR_REG_OFFS                          0x0010
DATA_ID_W_REG_OFFS                      0x0011
FRAME_NO_LSB_REG_OFFS                   0x0012
FRAME_NO_MSB_REG_OFFS                   0x0013
MODE_REG1_OFFS                          0x0014
MODE_REG2_OFFS                          0x0015
CLOCK_CONTROL_REG1_OFFS                 0x0016
DATA_ID_R_REG_OFFS                      0x0017
DATA_ID_R_EMB_REG_OFFS                  0x0018
DATA_SEL_CTRL_REG_OFFS                  0x0019
ACTIVE_LINE_NO_MSB_REG_OFFS             0x001A
ACTIVE_LINE_NO_LSB_REG_OFFS             0x001B
SOF_LINE_NO_MSB_REG_OFFS                0x001C
SOF_LINE_NO_LSB_REG_OFFS                0x001D
PIX_WIDTH_CTRL_REG_OFFS                 0x001E
PIX_WIDTH_CTRL_EMB_REG_OFFS             0x001F
DATA_FIELD_MSB_REG_OFFS                 0x0020
DATA_FIELD_LSB_REG_OFFS                 0x0021
CLK_LANE_REG1_C2_OFFS                   0x0031
CLK_LANE_REG3_C2_OFFS                   0x0033
DATA_LANE3_REG1_OFFS                    0x0034
DATA_LANE3_REG2_OFFS                    0x0035
MODE_REG3_OFFS                          0x0036
CCP_RX_REG1_C2_OFFS                     0x0038
CLK_LANE_WR_REG1_C2_OFFS                0x0039 // ReadOnly
DATA_LANE3_REG3_OFFS                    0x003A // ReadOnly
DATA_LANE3_REG4_OFFS                    0x003B // ReadOnly
*/

#endif /* __MIPI_DRV_PHY_STD02_H__ */
