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
* @file hal_altera_pci_mem.h
*
* <pre>
*
* Description:
 *   A simple implementation of HAL's malloc() and free() to be used in combination
 *   with the Altera FPGA board. Free'd blocks get combined with adjacent free
 *   blocks into a single larger block immediately.
*
* </pre>
*/
/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif


/******************************************************************************
 * @brief           Create & initialze external memory allocator.
 *****************************************************************************/
RESULT ExtMemCreate( uint32_t BaseAddr, uint32_t Size, uint32_t Alignment );


/******************************************************************************
 * @brief           Destroy external memory allocator.
 *****************************************************************************/
RESULT ExtMemDestroy( void );


/******************************************************************************
 * @brief           allocate a block of external memory
 *****************************************************************************/
uint32_t ExtMemAlloc( uint32_t ReqSize );


/******************************************************************************
 * @brief           free block of external memory
 *****************************************************************************/
void ExtMemFree( uint32_t BlockAddr );


#ifdef __cplusplus
}
#endif
