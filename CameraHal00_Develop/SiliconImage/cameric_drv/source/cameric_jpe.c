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
 * @file cameric_jpe.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/list.h>
#include <common/return_codes.h>

#include "mrv_all_bits.h"

#include "cameric_drv_cb.h"
#include "cameric_drv.h"


/******************************************************************************
 * Is the hardware JPE module available ?
 *****************************************************************************/
#if defined(MRV_JPE_VERSION)

/******************************************************************************
 * JPE Module is available.
 *****************************************************************************/

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_JPE_DRV_INFO  , "CAMERIC-JPE-DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_JPE_DRV_WARN  , "CAMERIC-JPE-DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_JPE_DRV_ERROR , "CAMERIC-JPE-DRV: ", ERROR   , 1 );

CREATE_TRACER( CAMERIC_JPE_IRQ_INFO  , "CAMERIC-JPE-IRQ: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_JPE_IRQ_ERROR , "CAMERIC-JPE-IRQ: ", INFO    , 1 );


/******************************************************************************
 * local type definitions
 *****************************************************************************/
static int32_t init = 0;


/******************************************************************************
 * local variable declarations
 *****************************************************************************/

//! DC luma table according to ISO/IEC 10918-1 annex K
const uint8_t DCLumaTableAnnexK[] =
{
    0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
    0x08, 0x09, 0x0a, 0x0b 
};

//! DC chroma table according to ISO/IEC 10918-1 annex K
const uint8_t DCChromaTableAnnexK[] =
{
    0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
    0x08, 0x09, 0x0a, 0x0b 
};

//! AC luma table according to ISO/IEC 10918-1 annex K
const uint8_t ACLumaTableAnnexK[] =
{
    0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
    0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7d,
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 
    0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
    0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0, 
    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
    0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 
    0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 
    0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 
    0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
    0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 
    0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
    0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
    0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 
    0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9,  0xfa
};

//! AC Chroma table according to ISO/IEC 10918-1 annex K
const uint8_t ACChromaTableAnnexK[] =
{
    0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04,
    0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77,
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
    0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
    0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
    0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
    0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
    0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
    0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
    0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
    0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
    0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
    0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
    0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
    0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
    0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
    0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa
};

//#############################################
// 75%
//#############################################
//! luma quantization table 75% quality setting
const uint8_t YQTable75PerCent[] =
{
    0x08, 0x06, 0x06, 0x07, 0x06, 0x05, 0x08, 0x07, 
    0x07, 0x07, 0x09, 0x09, 0x08, 0x0a, 0x0c, 0x14, 
    0x0d, 0x0c, 0x0b, 0x0b, 0x0c, 0x19, 0x12, 0x13, 
    0x0f, 0x14, 0x1d, 0x1a, 0x1f, 0x1e, 0x1d, 0x1a, 
    0x1c, 0x1c, 0x20, 0x24, 0x2e, 0x27, 0x20, 0x22, 
    0x2c, 0x23, 0x1c, 0x1c, 0x28, 0x37, 0x29, 0x2c, 
    0x30, 0x31, 0x34, 0x34, 0x34, 0x1f, 0x27, 0x39, 
    0x3d, 0x38, 0x32, 0x3c, 0x2e, 0x33, 0x34, 0x32
};

//! chroma quantization table 75% quality setting
const uint8_t UVQTable75PerCent[] =
{
    0x09, 0x09, 0x09, 0x0c, 0x0b, 0x0c, 0x18, 0x0d, 
    0x0d, 0x18, 0x32, 0x21, 0x1c, 0x21, 0x32, 0x32, 
    0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 
    0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 
    0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 
    0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 
    0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 
    0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32
};

//#############################################
// constant F2
//#############################################

//! luma quantization table very low compression
//! (about factor 2)
const uint8_t YQTableLowComp1[] =
{
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02
};

//! chroma quantization table very low compression
//! (about factor 2)
const uint8_t UVQTableLowComp1[] =
{
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02
};

/*********************************************************************
 *  \NOTES       The jpg Quantization Tables were parsed by jpeg_parser from
 *               jpg images generated by Jasc PaintShopPro.
 *
 *********************************************************************/
 
//#############################################
// 01%
//#############################################

//! luma quantization table
const uint8_t YQTable01PerCent[] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x01, 0x01, 
    0x02, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 
    0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x02, 0x02, 
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02
};

//! chroma quantization table
const uint8_t UVQTable01PerCent[] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x01, 0x02, 0x01, 0x01, 0x01, 0x02, 0x02, 
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02
};

//#############################################
// 20%
//#############################################

//! luma quantization table
const uint8_t YQTable20PerCent[] = {
    0x06, 0x04, 0x05, 0x06, 0x05, 0x04, 0x06, 0x06, 
    0x05, 0x06, 0x07, 0x07, 0x06, 0x08, 0x0a, 0x10, 
    0x0a, 0x0a, 0x09, 0x09, 0x0a, 0x14, 0x0e, 0x0f, 
    0x0c, 0x10, 0x17, 0x14, 0x18, 0x18, 0x17, 0x14, 
    0x16, 0x16, 0x1a, 0x1d, 0x25, 0x1f, 0x1a, 0x1b, 
    0x23, 0x1c, 0x16, 0x16, 0x20, 0x2c, 0x20, 0x23, 
    0x26, 0x27, 0x29, 0x2a, 0x29, 0x19, 0x1f, 0x2d, 
    0x30, 0x2d, 0x28, 0x30, 0x25, 0x28, 0x29, 0x28
};

//! chroma quantization table
const uint8_t UVQTable20PerCent[] = {
    0x07, 0x07, 0x07, 0x0a, 0x08, 0x0a, 0x13, 0x0a, 
    0x0a, 0x13, 0x28, 0x1a, 0x16, 0x1a, 0x28, 0x28, 
    0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 
    0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 
    0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 
    0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 
    0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 
    0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28
};

//#############################################
// 30%
//#############################################
//! luma quantization table
const uint8_t YQTable30PerCent[] = {
    0x0a, 0x07, 0x07, 0x08, 0x07, 0x06, 0x0a, 0x08, 
    0x08, 0x08, 0x0b, 0x0a, 0x0a, 0x0b, 0x0e, 0x18, 
    0x10, 0x0e, 0x0d, 0x0d, 0x0e, 0x1d, 0x15, 0x16, 
    0x11, 0x18, 0x23, 0x1f, 0x25, 0x24, 0x22, 0x1f, 
    0x22, 0x21, 0x26, 0x2b, 0x37, 0x2f, 0x26, 0x29, 
    0x34, 0x29, 0x21, 0x22, 0x30, 0x41, 0x31, 0x34, 
    0x39, 0x3b, 0x3e, 0x3e, 0x3e, 0x25, 0x2e, 0x44, 
    0x49, 0x43, 0x3c, 0x48, 0x37, 0x3d, 0x3e, 0x3b
};

//! chroma quantization table
const uint8_t UVQTable30PerCent[] = {
    0x0a, 0x0b, 0x0b, 0x0e, 0x0d, 0x0e, 0x1c, 0x10, 
    0x10, 0x1c, 0x3b, 0x28, 0x22, 0x28, 0x3b, 0x3b, 
    0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 
    0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 
    0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 
    0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 
    0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 
    0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b
};

//#############################################
// 40%
//#############################################
//! luma quantization table
const uint8_t YQTable40PerCent[] = {
    0x0d, 0x09, 0x0a, 0x0b, 0x0a, 0x08, 0x0d, 0x0b, 
    0x0a, 0x0b, 0x0e, 0x0e, 0x0d, 0x0f, 0x13, 0x20, 
    0x15, 0x13, 0x12, 0x12, 0x13, 0x27, 0x1c, 0x1e, 
    0x17, 0x20, 0x2e, 0x29, 0x31, 0x30, 0x2e, 0x29, 
    0x2d, 0x2c, 0x33, 0x3a, 0x4a, 0x3e, 0x33, 0x36, 
    0x46, 0x37, 0x2c, 0x2d, 0x40, 0x57, 0x41, 0x46, 
    0x4c, 0x4e, 0x52, 0x53, 0x52, 0x32, 0x3e, 0x5a, 
    0x61, 0x5a, 0x50, 0x60, 0x4a, 0x51, 0x52, 0x4f
};

//! chroma quantization table
const uint8_t UVQTable40PerCent[] = {
    0x0e, 0x0e, 0x0e, 0x13, 0x11, 0x13, 0x26, 0x15, 
    0x15, 0x26, 0x4f, 0x35, 0x2d, 0x35, 0x4f, 0x4f, 
    0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 
    0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 
    0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 
    0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 
    0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 
    0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f
};

//#############################################
// 50%
//#############################################

//! luma quantization table
const uint8_t YQTable50PerCent[] = {
    0x10, 0x0b, 0x0c, 0x0e, 0x0c, 0x0a, 0x10, 0x0e, 
    0x0d, 0x0e, 0x12, 0x11, 0x10, 0x13, 0x18, 0x28, 
    0x1a, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23, 0x25, 
    0x1d, 0x28, 0x3a, 0x33, 0x3d, 0x3c, 0x39, 0x33, 
    0x38, 0x37, 0x40, 0x48, 0x5c, 0x4e, 0x40, 0x44, 
    0x57, 0x45, 0x37, 0x38, 0x50, 0x6d, 0x51, 0x57, 
    0x5f, 0x62, 0x67, 0x68, 0x67, 0x3e, 0x4d, 0x71, 
    0x79, 0x70, 0x64, 0x78, 0x5c, 0x65, 0x67, 0x63
};

//! chroma quantization table
const uint8_t UVQTable50PerCent[] = {
    0x11, 0x12, 0x12, 0x18, 0x15, 0x18, 0x2f, 0x1a, 
    0x1a, 0x2f, 0x63, 0x42, 0x38, 0x42, 0x63, 0x63, 
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63
};

//#############################################
// 60%
//#############################################
//! luma quantization table
const uint8_t YQTable60PerCent[] = {
    0x14, 0x0e, 0x0f, 0x12, 0x0f, 0x0d, 0x14, 0x12, 
    0x10, 0x12, 0x17, 0x15, 0x14, 0x18, 0x1e, 0x32, 
    0x21, 0x1e, 0x1c, 0x1c, 0x1e, 0x3d, 0x2c, 0x2e, 
    0x24, 0x32, 0x49, 0x40, 0x4c, 0x4b, 0x47, 0x40, 
    0x46, 0x45, 0x50, 0x5a, 0x73, 0x62, 0x50, 0x55, 
    0x6d, 0x56, 0x45, 0x46, 0x64, 0x88, 0x65, 0x6d, 
    0x77, 0x7b, 0x81, 0x82, 0x81, 0x4e, 0x60, 0x8d, 
    0x97, 0x8c, 0x7d, 0x96, 0x73, 0x7e, 0x81, 0x7c
};

//! chroma quantization table
const uint8_t UVQTable60PerCent[] = {
    0x15, 0x17, 0x17, 0x1e, 0x1a, 0x1e, 0x3b, 0x21, 
    0x21, 0x3b, 0x7c, 0x53, 0x46, 0x53, 0x7c, 0x7c, 
    0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 
    0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 
    0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 
    0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 
    0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 
    0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c
};

//#############################################
// 70%
//#############################################
//! luma quantization table
const uint8_t YQTable70PerCent[] = {
    0x1b, 0x12, 0x14, 0x17, 0x14, 0x11, 0x1b, 0x17, 
    0x16, 0x17, 0x1e, 0x1c, 0x1b, 0x20, 0x28, 0x42, 
    0x2b, 0x28, 0x25, 0x25, 0x28, 0x51, 0x3a, 0x3d, 
    0x30, 0x42, 0x60, 0x55, 0x65, 0x64, 0x5f, 0x55, 
    0x5d, 0x5b, 0x6a, 0x78, 0x99, 0x81, 0x6a, 0x71, 
    0x90, 0x73, 0x5b, 0x5d, 0x85, 0xb5, 0x86, 0x90, 
    0x9e, 0xa3, 0xab, 0xad, 0xab, 0x67, 0x80, 0xbc, 
    0xc9, 0xba, 0xa6, 0xc7, 0x99, 0xa8, 0xab, 0xa4
};

//! chroma quantization table
const uint8_t UVQTable70PerCent[] = {
    0x1c, 0x1e, 0x1e, 0x28, 0x23, 0x28, 0x4e, 0x2b, 
    0x2b, 0x4e, 0xa4, 0x6e, 0x5d, 0x6e, 0xa4, 0xa4, 
    0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 
    0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 
    0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 
    0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 
    0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 
    0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4
};

//#############################################
// 80%
//#############################################
//! luma quantization table
const uint8_t YQTable80PerCent[] = {
    0x28, 0x1c, 0x1e, 0x23, 0x1e, 0x19, 0x28, 0x23, 
    0x21, 0x23, 0x2d, 0x2b, 0x28, 0x30, 0x3c, 0x64, 
    0x41, 0x3c, 0x37, 0x37, 0x3c, 0x7b, 0x58, 0x5d, 
    0x49, 0x64, 0x91, 0x80, 0x99, 0x96, 0x8f, 0x80, 
    0x8c, 0x8a, 0xa0, 0xb4, 0xe6, 0xc3, 0xa0, 0xaa, 
    0xda, 0xad, 0x8a, 0x8c, 0xc8, 0xff, 0xcb, 0xda, 
    0xee, 0xf5, 0xff, 0xff, 0xff, 0x9b, 0xc1, 0xff, 
    0xff, 0xff, 0xfa, 0xff, 0xe6, 0xfd, 0xff, 0xf8
};

//! chroma quantization table
const uint8_t UVQTable80PerCent[] = {
    0x2b, 0x2d, 0x2d, 0x3c, 0x35, 0x3c, 0x76, 0x41, 
    0x41, 0x76, 0xf8, 0xa5, 0x8c, 0xa5, 0xf8, 0xf8, 
    0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 
    0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 
    0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 
    0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 
    0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 
    0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8
};

//#############################################
// 90%
//#############################################
//! luma quantization table
const uint8_t YQTable90PerCent[] = {
    0x50, 0x37, 0x3c, 0x46, 0x3c, 0x32, 0x50, 0x46, 
    0x41, 0x46, 0x5a, 0x55, 0x50, 0x5f, 0x78, 0xc8, 
    0x82, 0x78, 0x6e, 0x6e, 0x78, 0xf5, 0xaf, 0xb9, 
    0x91, 0xc8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

//! chroma quantization table
const uint8_t UVQTable90PerCent[] = {
    0x55, 0x5a, 0x5a, 0x78, 0x69, 0x78, 0xeb, 0x82, 
    0x82, 0xeb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

//#############################################
// 99%
//#############################################
//! luma quantization table
const uint8_t YQTable99PerCent[] = 
{
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

//! chroma quantization table
const uint8_t UVQTable99PerCent[] = 
{
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};



/******************************************************************************
 * local function prototypes
 *****************************************************************************/
static int32_t CamerIcJpeStatusIrq( void *pArg );
static int32_t CamerIcJpeErrorIrq( void *pArg );



/******************************************************************************
 * local functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * @brief   This function is the JPE status IRQ handler.
 *
 * @param   pArg        context pointer
 *
 * @return              Return the result of the function call.
 * @retval              0
 *
 *****************************************************************************/
static int32_t CamerIcJpeStatusIrq
(
    void *pArg
)
{
    CamerIcDrvContext_t *pDrvCtx;       /* CamerIc Driver Context */
    CamerIcJpeContext_t *pJpeCtx;
    HalIrqCtx_t         *pHalIrqCtx;    /* HAL context */

    TRACE( CAMERIC_JPE_IRQ_INFO, "%s: (enter) \n", __FUNCTION__ );

    /* get IRQ context from args */
    pHalIrqCtx = (HalIrqCtx_t*)(pArg);
    DCT_ASSERT( (pHalIrqCtx != NULL) );

    pDrvCtx = (CamerIcDrvContext_t *)(pHalIrqCtx->OsIrq.p_context);
    DCT_ASSERT( (pDrvCtx != NULL) );

    pJpeCtx = pDrvCtx->pJpeContext;
    DCT_ASSERT( (pJpeCtx != NULL) );

    TRACE( CAMERIC_JPE_IRQ_INFO, "%s: (mis=%08x) \n", __FUNCTION__, pHalIrqCtx->misValue);

    if ( pHalIrqCtx->misValue & MRV_JPE_GEN_HEADER_DONE_MASK )
    {
        if ( pJpeCtx->EventCb.func != NULL ) 
        {
            pJpeCtx->EventCb.func( CAMERIC_JPE_EVENT_HEADER_GENERATED, NULL,  pJpeCtx->EventCb.pUserContext );
        }
        pHalIrqCtx->misValue &= ~( MRV_JPE_GEN_HEADER_DONE_MASK );
    }

    if ( pHalIrqCtx->misValue & MRV_JPE_ENCODE_DONE_MASK )
    {
        if ( pJpeCtx->EventCb.func != NULL ) 
        {
            pJpeCtx->EventCb.func( CAMERIC_JPE_EVENT_DATA_ENCODED, NULL,  pJpeCtx->EventCb.pUserContext );
        }
        pJpeCtx->encState = CAMERIC_JPE_STATE_CONFIGURED;

        pHalIrqCtx->misValue &= ~( MRV_JPE_ENCODE_DONE_MASK );
    }

    TRACE( CAMERIC_JPE_IRQ_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( 0 );
}



/*****************************************************************************/
/**
 * @brief   This function is the JPE error IRQ handler.
 *
 * @param   pArg        context pointer
 *
 * @return              Return the result of the function call.
 * @retval              0
 *
 *****************************************************************************/
static int32_t CamerIcJpeErrorIrq
(
    void *pArg
)
{
    CamerIcDrvContext_t *pDrvCtx;       /* CamerIc Driver Context */
    HalIrqCtx_t         *pHalIrqCtx;    /* HAL context */

    TRACE( CAMERIC_JPE_IRQ_INFO, "%s: (enter) \n", __FUNCTION__ );

    /* get IRQ context from args */
    pHalIrqCtx = (HalIrqCtx_t*)(pArg);
    DCT_ASSERT( (pHalIrqCtx != NULL) );

    pDrvCtx = (CamerIcDrvContext_t *)(pHalIrqCtx->OsIrq.p_context);
    DCT_ASSERT( (pDrvCtx != NULL) );
    DCT_ASSERT( (pDrvCtx->pIspContext != NULL) );

    TRACE( CAMERIC_JPE_IRQ_INFO, "%s: (mis=%08x) \n", __FUNCTION__, pHalIrqCtx->misValue);

    if ( pHalIrqCtx->misValue & MRV_JPE_VLC_TABLE_ERR_MASK )
    {
        pHalIrqCtx->misValue &= ~( MRV_JPE_VLC_TABLE_ERR_MASK );
    }

    if ( pHalIrqCtx->misValue & MRV_JPE_R2B_IMG_SIZE_ERR_MASK )
    {
        pHalIrqCtx->misValue &= ~( MRV_JPE_R2B_IMG_SIZE_ERR_MASK );
    }

    if ( pHalIrqCtx->misValue & MRV_JPE_DCT_ERR_MASK )
    {
        pHalIrqCtx->misValue &= ~( MRV_JPE_DCT_ERR_MASK );
    }

    if ( pHalIrqCtx->misValue & MRV_JPE_VLC_SYMBOL_ERR_MASK )
    {
        pHalIrqCtx->misValue &= ~( MRV_JPE_VLC_SYMBOL_ERR_MASK );
    }

    TRACE( CAMERIC_JPE_IRQ_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( 0 );
}



/******************************************************************************/
/**
 * @brief   This functions resets the CamerIC JPE module. 
 *
 * @param   ctx                 Driver contecxt for getting HalHandle to access
 *                              hardware
 * @param   stay_in_reset       if TRUE than the IE hardware module stays in 
 *                              reset state
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
static RESULT CamerIcJpeReset
(
    CamerIcDrvContext_t *ctx,
    const bool_t        stay_in_reset
)
{
    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t vi_ircl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_ircl) );
        REG_SET_SLICE( vi_ircl, MRV_VI_JPEG_SOFT_RST, 1 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_ircl), vi_ircl );

        if ( BOOL_FALSE == stay_in_reset )
        {
            REG_SET_SLICE( vi_ircl, MRV_VI_JPEG_SOFT_RST, 0 );
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_ircl), vi_ircl );
        }
    }

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/*****************************************************************************/
/**
 * @brief   This function programs q-, AC- and DC-tables.
 *
 * @note    See code in user manual page 43. In general there is no
 *          need to program other AC/DC-tables than the ones proposed
 *          in the ISO/IEC 10918-1 annex K. So we just use static table
 *          data here. Thus MrvJpeSetTables and MrvJpeSelectTables do
 *          not provide a way to switch between different tables
 *          at the moment.
 *
 *          After any reset the internal RAM is filled with 0xFF.
 *          This filling takes approximately 400 clock cycles. So do
 *          not start any table programming during the first 400 clock
 *          cycles after reset is deasserted.
 *
 *
 * @note    The jpg Quantization Tables were parsed by jpeg_parser from
 *          jpg images generated by Jasc PaintShopPro.
 *
 * @param   handle              CamerIc driver handle
 * @param   mode                mode (see @ref CamerIcJpeCompressionLevel_e)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_OUTOFRANGE      At least one perameter of out range 
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *****************************************************************************/
static RESULT CamerIcJpeSetCompressionLevel
(
    CamerIcDrvHandle_t                  handle,
    const CamerIcJpeCompressionLevel_t  level
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;

    const uint8_t *pYQTable     = NULL;
    const uint8_t *pUVQTable    = NULL;

    uint8_t idx; 
    uint8_t size;

    uint32_t jpe_table_id   = 0UL;
    uint32_t jpe_table_data = 0UL;
    uint32_t jpe_tac0_len   = 0UL;
    uint32_t jpe_tac1_len   = 0UL;
    uint32_t jpe_tdc0_len   = 0UL;
    uint32_t jpe_tdc1_len   = 0UL;

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pJpeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    switch ( level )
    {
        case CAMERIC_JPE_COMPRESSION_LEVEL_LOW:
            {
                size = sizeof(YQTableLowComp1) / sizeof(YQTableLowComp1[0]);
                pYQTable  = YQTableLowComp1;
                pUVQTable = UVQTableLowComp1;
                break;
            }

        case CAMERIC_JPE_COMPRESSION_LEVEL_HIGH:
            {
                size = sizeof(YQTable75PerCent) / sizeof(YQTable75PerCent[0]);
                pYQTable  = YQTable75PerCent;
                pUVQTable = UVQTable75PerCent;
                break;
            }

        case CAMERIC_JPE_COMPRESSION_LEVEL_01_PRECENT:
            {
                size = sizeof(YQTable01PerCent) / sizeof(YQTable01PerCent[0]);
                pYQTable  = YQTable01PerCent;
                pUVQTable = UVQTable01PerCent;
                break;
            }

        case CAMERIC_JPE_COMPRESSION_LEVEL_20_PERCENT:
            {
                size = sizeof(YQTable20PerCent) / sizeof(YQTable20PerCent[0]);
                pYQTable  = YQTable20PerCent;
                pUVQTable = UVQTable20PerCent;
                break;
            }

        case CAMERIC_JPE_COMPRESSION_LEVEL_30_PERCENT:
            {
                size = sizeof(YQTable30PerCent) / sizeof(YQTable30PerCent[0]);
                pYQTable  = YQTable30PerCent;
                pUVQTable = UVQTable30PerCent;
                break;
            }

        case CAMERIC_JPE_COMPRESSION_LEVEL_40_PERCENT:
            {
                size = sizeof(YQTable40PerCent) / sizeof(YQTable40PerCent[0]);
                pYQTable  = YQTable40PerCent;
                pUVQTable = UVQTable40PerCent;
                break;
            }

        case CAMERIC_JPE_COMPRESSION_LEVEL_50_PERCENT:
            {
                size = sizeof(YQTable50PerCent) / sizeof(YQTable50PerCent[0]);
                pYQTable  = YQTable50PerCent;
                pUVQTable = UVQTable50PerCent;
                break;
            }

        case CAMERIC_JPE_COMPRESSION_LEVEL_60_PERCENT:
            {
                size = sizeof(YQTable60PerCent) / sizeof(YQTable60PerCent[0]);
                pYQTable  = YQTable60PerCent;
                pUVQTable = UVQTable60PerCent;
                break;
            }

        case CAMERIC_JPE_COMPRESSION_LEVEL_70_PERCENT:
            {
                size = sizeof(YQTable70PerCent) / sizeof(YQTable70PerCent[0]);
                pYQTable  = YQTable70PerCent;
                pUVQTable = UVQTable70PerCent;
                break;
            }

        case CAMERIC_JPE_COMPRESSION_LEVEL_80_PERCENT:
            {
                size = sizeof(YQTable80PerCent) / sizeof(YQTable80PerCent[0]);
                pYQTable  = YQTable80PerCent;
                pUVQTable = UVQTable80PerCent;
                break;
            }

        case CAMERIC_JPE_COMPRESSION_LEVEL_90_PERCENT:
            {
                size = sizeof(YQTable90PerCent) / sizeof(YQTable90PerCent[0]);
                pYQTable  = YQTable90PerCent;
                pUVQTable = UVQTable90PerCent;
                break;
            }

        case CAMERIC_JPE_COMPRESSION_LEVEL_99_PERCENT:
            {
                size = sizeof(YQTable99PerCent) / sizeof(YQTable99PerCent[0]);
                pYQTable  = YQTable99PerCent;
                pUVQTable = UVQTable99PerCent;
                break;
            }

        default:
            {
                TRACE( CAMERIC_JPE_DRV_ERROR, "%s: unsupported JPE compression level\n", __FUNCTION__ );
                return ( RET_NOTSUPP );
            }
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

    // Y q-table 0 programming
    REG_SET_SLICE( jpe_table_id, MRV_JPE_TABLE_ID, MRV_JPE_TABLE_ID_QUANT0 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_table_id), jpe_table_id );
    for ( idx = 0U; idx < (size-1U); idx += 2U )
    {
        REG_SET_SLICE( jpe_table_data, MRV_JPE_TABLE_WDATA_H, pYQTable[idx] );
        REG_SET_SLICE( jpe_table_data, MRV_JPE_TABLE_WDATA_L, pYQTable[idx+1U] );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_table_data), jpe_table_data );
    }

    // U/V q-table 0 programming
    REG_SET_SLICE( jpe_table_id, MRV_JPE_TABLE_ID, MRV_JPE_TABLE_ID_QUANT1 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_table_id), jpe_table_id );
    for ( idx = 0U; idx < (size-1U); idx += 2U )
    {
        REG_SET_SLICE( jpe_table_data, MRV_JPE_TABLE_WDATA_H, pUVQTable[idx] );
        REG_SET_SLICE( jpe_table_data, MRV_JPE_TABLE_WDATA_L, pUVQTable[idx+1U] );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_table_data), jpe_table_data );
    }

    // Y AC-table 0 programming
    size = sizeof(ACLumaTableAnnexK) / sizeof(ACLumaTableAnnexK[0]);
    REG_SET_SLICE( jpe_table_id, MRV_JPE_TABLE_ID, MRV_JPE_TABLE_ID_VLC_AC0 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_table_id), jpe_table_id );
    REG_SET_SLICE( jpe_tac0_len, MRV_JPE_TAC0_LEN, size );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_tac0_len), jpe_tac0_len );
    for ( idx = 0U; idx < (size - 1U); idx += 2U )
    {
        REG_SET_SLICE(jpe_table_data, MRV_JPE_TABLE_WDATA_H, ACLumaTableAnnexK[idx]);
        REG_SET_SLICE(jpe_table_data, MRV_JPE_TABLE_WDATA_L, ACLumaTableAnnexK[idx+1U]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_table_data), jpe_table_data );
    }
    
    // U/V AC-table 1 programming
    size = sizeof(ACChromaTableAnnexK) / sizeof(ACChromaTableAnnexK[0]);
    REG_SET_SLICE( jpe_table_id, MRV_JPE_TABLE_ID, MRV_JPE_TABLE_ID_VLC_AC1 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_table_id), jpe_table_id );
    REG_SET_SLICE( jpe_tac1_len, MRV_JPE_TAC1_LEN, size );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_tac1_len), jpe_tac1_len );
    for ( idx = 0U; idx < (size - 1U); idx += 2U )
    {
        REG_SET_SLICE(jpe_table_data, MRV_JPE_TABLE_WDATA_H, ACChromaTableAnnexK[idx]);
        REG_SET_SLICE(jpe_table_data, MRV_JPE_TABLE_WDATA_L, ACChromaTableAnnexK[idx+1U]);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_table_data), jpe_table_data );
    }
    
    // Y DC-table 0 programming
    size = sizeof(DCLumaTableAnnexK) / sizeof(DCLumaTableAnnexK[0]);
    REG_SET_SLICE( jpe_table_id, MRV_JPE_TABLE_ID, MRV_JPE_TABLE_ID_VLC_DC0 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_table_id), jpe_table_id );
    REG_SET_SLICE( jpe_tdc0_len, MRV_JPE_TDC0_LEN, size );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_tdc0_len), jpe_tdc0_len );
    for ( idx = 0U; idx < (size - 1U); idx += 2U )
    {
        REG_SET_SLICE( jpe_table_data, MRV_JPE_TABLE_WDATA_H, DCLumaTableAnnexK[idx] );
        REG_SET_SLICE( jpe_table_data, MRV_JPE_TABLE_WDATA_L, DCLumaTableAnnexK[idx+1U] );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_table_data), jpe_table_data );
    }
    
    // U/V DC-table 1 programming
    size = sizeof(DCChromaTableAnnexK) / sizeof(DCChromaTableAnnexK[0]);
    REG_SET_SLICE( jpe_table_id, MRV_JPE_TABLE_ID, MRV_JPE_TABLE_ID_VLC_DC1 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_table_id), jpe_table_id );
    REG_SET_SLICE( jpe_tdc1_len, MRV_JPE_TDC1_LEN, size );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_tdc1_len), jpe_tdc1_len );
    for ( idx = 0U; idx < (size - 1U); idx += 2U )
    {
        REG_SET_SLICE( jpe_table_data, MRV_JPE_TABLE_WDATA_H, DCChromaTableAnnexK[idx] );
        REG_SET_SLICE( jpe_table_data, MRV_JPE_TABLE_WDATA_L, DCChromaTableAnnexK[idx+1U] );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_table_data), jpe_table_data );
    }

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/*****************************************************************************/
/**
 * @brief   This function selects tables to be used by encoder
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_OUTOFRANGE      At least one perameter of out range 
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
static RESULT CamerIcJpeSelectTables
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pJpeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t jpe_tq_y_select = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_tq_y_select) );
        uint32_t jpe_tq_u_select = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_tq_u_select) );
        uint32_t jpe_tq_v_select = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_tq_v_select) );

        uint32_t jpe_dc_table_select = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_dc_table_select) );
        uint32_t jpe_ac_table_select = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_ac_table_select) );
    
        /* selects quantization table for Y, U ,V */
        REG_SET_SLICE( jpe_tq_y_select, MRV_JPE_TQ0_SELECT, MRV_JPE_TQ_SELECT_TAB0 );
        REG_SET_SLICE( jpe_tq_u_select, MRV_JPE_TQ1_SELECT, MRV_JPE_TQ_SELECT_TAB1 );
        REG_SET_SLICE( jpe_tq_v_select, MRV_JPE_TQ2_SELECT, MRV_JPE_TQ_SELECT_TAB1 );

        /* selects Huffman DC table */
        REG_SET_SLICE( jpe_dc_table_select, MRV_JPE_DC_TABLE_SELECT_Y, 0);
        REG_SET_SLICE( jpe_dc_table_select, MRV_JPE_DC_TABLE_SELECT_U, 1);
        REG_SET_SLICE( jpe_dc_table_select, MRV_JPE_DC_TABLE_SELECT_V, 1);

        /* selects Huffman AC table */
        REG_SET_SLICE( jpe_ac_table_select, MRV_JPE_AC_TABLE_SELECT_Y, 0);
        REG_SET_SLICE( jpe_ac_table_select, MRV_JPE_AC_TABLE_SELECT_U, 1);
        REG_SET_SLICE( jpe_ac_table_select, MRV_JPE_AC_TABLE_SELECT_V, 1);

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_tq_y_select), jpe_tq_y_select );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_tq_u_select), jpe_tq_u_select );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_tq_v_select), jpe_tq_v_select );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_dc_table_select), jpe_dc_table_select );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_ac_table_select), jpe_ac_table_select );
    }

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver Internal API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcJpeEnableClock()
 *****************************************************************************/
RESULT CamerIcJpeEnableClock
(
    CamerIcDrvHandle_t  handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->HalHandle == NULL) || (ctx->pJpeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t vi_iccl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_iccl) );

        REG_SET_SLICE( vi_iccl, MRV_VI_JPEG_CLK_ENABLE, 1 );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_iccl), vi_iccl );
        
        result = CamerIcJpeReset( ctx, BOOL_FALSE );
    }

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * CamerIcJpeDisableClock()
 *****************************************************************************/
RESULT CamerIcJpeDisableClock
(
    CamerIcDrvHandle_t  handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t vi_iccl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_iccl) );

        REG_SET_SLICE( vi_iccl, MRV_VI_JPEG_CLK_ENABLE, 0 );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_iccl), vi_iccl );

        result = CamerIcJpeReset( ctx, BOOL_TRUE );
    }

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * CamerIcJpeInit()
 *****************************************************************************/
RESULT CamerIcJpeInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pJpeContext= ( CamerIcJpeContext_t *)malloc( sizeof(CamerIcJpeContext_t) );
    if (  ctx->pJpeContext == NULL )
    {
        TRACE( CAMERIC_JPE_DRV_ERROR,  "%s: Can't allocate CamerIC JPE context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pJpeContext, 0, sizeof( CamerIcJpeContext_t ) );

    result = CamerIcJpeEnableClock( ctx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAMERIC_JPE_DRV_ERROR,  "%s: Failed to enable clock for CamerIC JPE module.\n",  __FUNCTION__ );
        return ( result );
    }

    ctx->pJpeContext->encState = CAMERIC_JPE_STATE_INIT;
    ctx->pJpeContext->enabled = BOOL_FALSE;

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamerIcJpeRelease()
 *****************************************************************************/
RESULT CamerIcJpeRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pJpeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pJpeContext, 0, sizeof( CamerIcJpeContext_t ) );
    free ( ctx->pJpeContext );
    ctx->pJpeContext = NULL;

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcJpeStart()
 *****************************************************************************/
RESULT CamerIcJpeStart
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;
    volatile MrvAllRegister_t *ptCamerIcRegMap;

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pJpeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* pointer to register map */
    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

    uint32_t jpe_error_imr  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_error_imr) );
    uint32_t jpe_status_imr = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_status_imr) );

    REG_SET_SLICE( jpe_error_imr, MRV_JPE_VLC_SYMBOL_ERR, 1 );
    REG_SET_SLICE( jpe_error_imr, MRV_JPE_DCT_ERR, 1 );
    REG_SET_SLICE( jpe_error_imr, MRV_JPE_R2B_IMG_SIZE_ERR, 1 );
    REG_SET_SLICE( jpe_error_imr, MRV_JPE_VLC_TABLE_ERR, 1 );

    REG_SET_SLICE( jpe_status_imr, MRV_JPE_GEN_HEADER_DONE, 1 );
    REG_SET_SLICE( jpe_status_imr, MRV_JPE_ENCODE_DONE, 1 );
 
    /* register status IRQ handler */
    ctx->pJpeContext->HalStatusIrqCtx.misRegAddress = ( (ulong_t)&ptCamerIcRegMap->jpe_status_mis );
    ctx->pJpeContext->HalStatusIrqCtx.icrRegAddress = ( (ulong_t)&ptCamerIcRegMap->jpe_status_icr );

    result = HalConnectIrq( ctx->HalHandle, &ctx->pJpeContext->HalStatusIrqCtx, 0, NULL, &CamerIcJpeStatusIrq, ctx );
    DCT_ASSERT( (result == RET_SUCCESS) );

    /* register status IRQ handler */
    ctx->pJpeContext->HalErrorIrqCtx.misRegAddress = ( (ulong_t)&ptCamerIcRegMap->jpe_error_mis );
    ctx->pJpeContext->HalErrorIrqCtx.icrRegAddress = ( (ulong_t)&ptCamerIcRegMap->jpe_error_icr );

    result = HalConnectIrq( ctx->HalHandle, &ctx->pJpeContext->HalErrorIrqCtx, 0, NULL, &CamerIcJpeErrorIrq, ctx );
    DCT_ASSERT( (result == RET_SUCCESS) );

    /* start interrupts of CamerIc JPE module */
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_error_imr), jpe_error_imr );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_status_imr), jpe_status_imr );

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcJpeStop()
 *****************************************************************************/
RESULT CamerIcJpeStop
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pJpeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        /* stop interrupts of CamerIc JPE module */
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_error_imr), 0UL );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_status_imr), 0UL );

        /* disconnect JPE error IRQ handler */
        result = HalDisconnectIrq( &ctx->pJpeContext->HalErrorIrqCtx );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }

        /* disconnect JPE status IRQ handler */
        result = HalDisconnectIrq( &ctx->pJpeContext->HalStatusIrqCtx );
    }

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcJpeRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcJpeRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void                *pUserContext
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pJpeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)  
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( ctx->pJpeContext->EventCb.func != NULL )
    {
        return ( RET_BUSY );
    }

    if ( func == NULL )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        ctx->pJpeContext->EventCb.func          = func;
        ctx->pJpeContext->EventCb.pUserContext  = pUserContext;
    }

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcJpeDeRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcJpeDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pJpeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow cb changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)  
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    ctx->pJpeContext->EventCb.func          = NULL;
    ctx->pJpeContext->EventCb.pUserContext  = NULL;

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcJpeEnable()
 *****************************************************************************/
RESULT CamerIcJpeEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pJpeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t jpe_pic_format     = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_pic_format) );
        uint32_t jpe_enc_hsize      = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_enc_hsize) );
        uint32_t jpe_enc_vsize      = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_enc_vsize) );
        uint32_t jpe_y_scale_en     = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_y_scale_en) );
        uint32_t jpe_cbcr_scale_en  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_cbcr_scale_en) );
        uint32_t jpe_table_flush    = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_table_flush ) );

        result = CamerIcJpeReset( ctx, BOOL_FALSE );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }

        ctx->pJpeContext->hdrType = CAMERIC_JPE_HEADER_JFIF;

        switch ( ctx->pJpeContext->config.yscale )
        {
            case CAMERIC_JPE_LUMINANCE_SCALE_ENABLE:
                {
                    REG_SET_SLICE( jpe_y_scale_en, MRV_JPE_Y_SCALE_EN, 1U );
                    break;
                }

            case CAMERIC_JPE_LUMINANCE_SCALE_DISABLE:
                {
                    REG_SET_SLICE( jpe_y_scale_en, MRV_JPE_Y_SCALE_EN, 0U );
                    break;
                }

            default:
                {
                    TRACE( CAMERIC_JPE_DRV_ERROR,
                            "%s: unsopported luma scaling (%d)\n", __FUNCTION__, ctx->pJpeContext->config.yscale );
                    return ( RET_NOTSUPP );
                }
         }

        switch ( ctx->pJpeContext->config.cscale )
        {
            case CAMERIC_JPE_CHROMINANCE_SCALE_ENABLE:
                {
                    REG_SET_SLICE( jpe_cbcr_scale_en, MRV_JPE_CBCR_SCALE_EN, 1U );
                    break;
                }

            case CAMERIC_JPE_CHROMINANCE_SCALE_DISABLE:
                {
                    REG_SET_SLICE( jpe_cbcr_scale_en, MRV_JPE_CBCR_SCALE_EN, 0U );
                    break;
                }

            default:
                {
                    TRACE( CAMERIC_JPE_DRV_ERROR,
                            "%s: unsopported chroma scaling (%d)\n", __FUNCTION__, ctx->pJpeContext->config.cscale );
                    return ( RET_NOTSUPP );
                }
        }

        /* hsize, vsize */
        REG_SET_SLICE( jpe_enc_hsize, MRV_JPE_ENC_HSIZE, ctx->pJpeContext->config.width );
        REG_SET_SLICE( jpe_enc_vsize, MRV_JPE_ENC_VSIZE, ctx->pJpeContext->config.height );

        /* 422 */
        REG_SET_SLICE( jpe_pic_format, MRV_JPE_ENC_PIC_FORMAT, MRV_JPE_ENC_PIC_FORMAT_422 );

        /* flush */
        REG_SET_SLICE( jpe_table_flush, MRV_JPE_TABLE_FLUSH, 0);

        /* write register values */
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_pic_format), jpe_pic_format );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_y_scale_en), jpe_y_scale_en );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_cbcr_scale_en), jpe_cbcr_scale_en );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_enc_hsize), jpe_enc_hsize );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_enc_vsize), jpe_enc_vsize );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_table_flush), jpe_table_flush );

        result = CamerIcJpeSetCompressionLevel( handle, ctx->pJpeContext->config.level );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }

        result = CamerIcJpeSelectTables( handle );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_config), 0 );
 
        ctx->pJpeContext->encState = CAMERIC_JPE_STATE_CONFIGURED;

        ctx->pJpeContext->enabled = BOOL_TRUE;
    }

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamerIcJpeDisable()
 *****************************************************************************/
RESULT CamerIcJpeDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pJpeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pJpeContext->encState = CAMERIC_JPE_STATE_INIT;
    ctx->pJpeContext->enabled = BOOL_FALSE;

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcJpeConfigure()
 *****************************************************************************/
RESULT CamerIcJpeConfigure
(
    CamerIcDrvHandle_t      handle,
    CamerIcJpeConfig_t      *pConfig
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pJpeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pConfig == NULL )
    {
        return ( RET_NULL_POINTER );
    }
 
    if ( ctx->pJpeContext->enabled == BOOL_TRUE )
    {
        return ( RET_BUSY );
    }

    if ( (pConfig->mode <= CAMERIC_JPE_MODE_INVALID) || (pConfig->mode >= CAMERIC_JPE_MODE_MAX) )
    {
        TRACE( CAMERIC_JPE_DRV_ERROR, "%s: unsopported jpe mode(%d)\n", __FUNCTION__, pConfig->mode );
        return ( RET_NOTSUPP );
    }

    if ( (pConfig->level <= CAMERIC_JPE_COMPRESSION_LEVEL_INVALID)
            || (pConfig->level >= CAMERIC_JPE_COMPRESSION_LEVEL_MAX) )
    {
        TRACE( CAMERIC_JPE_DRV_ERROR, "%s: unsopported compression level(%d)\n", __FUNCTION__, pConfig->level );
        return ( RET_NOTSUPP );
    }

    if ( (pConfig->yscale <= CAMERIC_JPE_LUMINANCE_SCALE_INVALID)
            || (pConfig->yscale >= CAMERIC_JPE_LUMINANCE_SCALE_MAX) )
    {
        TRACE( CAMERIC_JPE_DRV_ERROR, "%s: unsopported luma scaling (%d)\n", __FUNCTION__, pConfig->yscale );
        return ( RET_NOTSUPP );
    }

    if ( (pConfig->cscale <= CAMERIC_JPE_CHROMINANCE_SCALE_INVALID)
            || (pConfig->cscale >= CAMERIC_JPE_CHROMINANCE_SCALE_MAX) )
    {
        TRACE( CAMERIC_JPE_DRV_ERROR, "%s: unsopported chroma scaling (%d)\n", __FUNCTION__, pConfig->cscale );
        return ( RET_NOTSUPP );
    }

    MEMCPY( &ctx->pJpeContext->config, pConfig, sizeof( CamerIcJpeConfig_t ) );

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcJpeStartHeaderGeneration()
 *****************************************************************************/
RESULT CamerIcJpeStartHeaderGeneration
(
    CamerIcDrvHandle_t          handle,
    const CamerIcJpeHeader_t    header
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;
    
    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pJpeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if (  ctx->pJpeContext->encState != CAMERIC_JPE_STATE_CONFIGURED )
    {
        return ( RET_WRONG_STATE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t jpe_header_mode = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_header_mode) );
        uint32_t jpe_gen_header  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_gen_header) );

        switch ( header )
        {
            case CAMERIC_JPE_HEADER_NONE:
                {
                    REG_SET_SLICE( jpe_header_mode, MRV_JPE_HEADER_MODE, MRV_JPE_HEADER_MODE_NONE );
                    break;
                }

            case CAMERIC_JPE_HEADER_JFIF:
                {
                    REG_SET_SLICE( jpe_header_mode, MRV_JPE_HEADER_MODE, MRV_JPE_HEADER_MODE_JFIF );
                    break;
                }

            default:
                {
                    TRACE( CAMERIC_JPE_DRV_ERROR, "%s: unsupported JPE header mode\n", __FUNCTION__ );
                    return ( RET_NOTSUPP );
                }
        }
 
        REG_SET_SLICE( jpe_gen_header,  MRV_JPE_GEN_HEADER,  1 );
        
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_header_mode), jpe_header_mode );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_gen_header), jpe_gen_header );

        ctx->pJpeContext->encState = CAMERIC_JPE_STATE_GEN_HEADER;
    }
    
    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );
    
    return ( RET_PENDING );
}



/******************************************************************************
 * CamerIcJpeStartEncoding()
 *****************************************************************************/
RESULT CamerIcJpeStartEncoding
(
    CamerIcDrvHandle_t      handle,
    const CamerIcJpeMode_t  mode 
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t jpe_encode;
    uint32_t jpe_config;
    uint32_t jpe_init;

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pJpeContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    jpe_encode      = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_encode) );
    jpe_config      = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_config) );

    jpe_init        = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_init) );

    switch ( mode )
    {
        case CAMERIC_JPE_MODE_SINGLE_SHOT:
            {
                REG_SET_SLICE(jpe_config, MRV_JPE_CONT_MODE, MRV_JPE_CONT_MODE_STOP );
                REG_SET_SLICE(jpe_config, MRV_JPE_SPEEDVIEW_EN, MRV_JPE_SPEEDVIEW_EN_DISABLE );
                
                break;
            }

        case CAMERIC_JPE_MODE_SHORT_CONTINUOUS:
            {
                REG_SET_SLICE( jpe_config, MRV_JPE_CONT_MODE, MRV_JPE_CONT_MODE_NEXT );
                REG_SET_SLICE( jpe_config, MRV_JPE_SPEEDVIEW_EN, MRV_JPE_SPEEDVIEW_EN_DISABLE );
                break;
            }

        case CAMERIC_JPE_MODE_LARGE_CONTINUOUS:
            {
                /* motion JPEG with header generation */
                REG_SET_SLICE( jpe_config, MRV_JPE_CONT_MODE, MRV_JPE_CONT_MODE_HEADER );
                REG_SET_SLICE( jpe_config, MRV_JPE_SPEEDVIEW_EN, MRV_JPE_SPEEDVIEW_EN_DISABLE );
                break;
            }

        case CAMERIC_JPE_MODE_SCALADO:
            {
                REG_SET_SLICE( jpe_config, MRV_JPE_CONT_MODE, MRV_JPE_CONT_MODE_STOP );
                REG_SET_SLICE( jpe_config, MRV_JPE_SPEEDVIEW_EN, MRV_JPE_SPEEDVIEW_EN_ENABLE );
                break;
            }

        default:
            {
                TRACE( CAMERIC_JPE_DRV_ERROR, "%s: unsupported JPE working mode\n", __FUNCTION__ );
                
                return ( RET_NOTSUPP );
            }
    }

    REG_SET_SLICE( jpe_encode, MRV_JPE_ENCODE, 1 );
    REG_SET_SLICE( jpe_init, MRV_JPE_JP_INIT, 1);

    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_config), jpe_config );
    
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_encode), jpe_encode );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->jpe_init), jpe_init );

    ctx->pJpeContext->encState = CAMERIC_JPE_STATE_ENC_DATA;

    TRACE( CAMERIC_JPE_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_PENDING );
}


#else  /* #if defined(MRV_JPE_VERSION) */

/******************************************************************************
 * JPE Module is not available.
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 * 
 *****************************************************************************/

/******************************************************************************
 * CamerIcJpeRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcJpeRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void                *pUserContext
)
{
    (void)handle;
    (void)func;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcJpeDeRegisterEventCb()
 *****************************************************************************/
RESULT CamerIcJpeDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcJpeEnable()
 *****************************************************************************/
RESULT CamerIcJpeEnable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcJpeDisable()
 *****************************************************************************/
RESULT CamerIcJpeDisable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcJpeConfigure()
 *****************************************************************************/
RESULT CamerIcJpeConfigure
(
    CamerIcDrvHandle_t      handle,
    CamerIcJpeConfig_t      *pConfig
)
{
    (void)handle;
    (void)pConfig;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcJpeStartHeaderGeneration()
 *****************************************************************************/
RESULT CamerIcJpeStartHeaderGeneration
(
    CamerIcDrvHandle_t          handle,
    const CamerIcJpeHeader_t    header
)
{
    (void)handle;
    (void)header;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcJpeStartEncoding()
 *****************************************************************************/
RESULT CamerIcJpeStartEncoding
(
    CamerIcDrvHandle_t      handle,
    const CamerIcJpeMode_t  mode 
)
{
    (void)handle;
    (void)mode;
    return ( RET_NOTSUPP );
}


#endif /* #if defined(MRV_JPE_VERSION) */

