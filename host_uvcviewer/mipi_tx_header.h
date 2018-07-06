
/* =============================================================================
* Copyright (c) 2013-2014 MM Solutions AD
*
* All rights reserved. Property of MM Solutions AD.
*
* This source code may not be used against the terms and conditions stipulated
* in the licensing agreement under which it has been supplied, or without the
* written permission of MM Solutions. Rights to use, copy, modify, and
* distribute or disclose this source code and its documentation are granted only
* through signed licensing agreement, provided that this copyright notice
* appears in all copies, modifications, and distributions and subject to the
* following conditions:
* THIS SOURCE CODE AND ACCOMPANYING DOCUMENTATION, IS PROVIDED AS IS, WITHOUT
* WARRANTY OF ANY KIND, EXPRESS OR IMPLIED. MM SOLUTIONS SPECIFICALLY DISCLAIMS
* ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN
* NO EVENT SHALL MM SOLUTIONS BE LIABLE TO ANY PARTY FOR ANY CLAIM, DIRECT,
* INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST
* PROFITS, OR OTHER LIABILITY, ARISING OUT OF THE USE OF OR IN CONNECTION WITH
* THIS SOURCE CODE AND ITS DOCUMENTATION.
* =========================================================================== */
/**
* @file
*
* @author Radoslav Ibrishimov ( MM Solutions AD )
*
*/
/* -----------------------------------------------------------------------------
*!
*! Revision History
*! ===================================
*! 30-Oct-2014 : Author Radoslav Ibrishimov ( MM Solutions AD )
*! Created
* =========================================================================== */

#ifndef MIPITXSTILLHEADER_H_
#define MIPITXSTILLHEADER_H_

enum {
    FRAME_DATA_TYPE_PREVIEW,
    FRAME_DATA_TYPE_STILL,
    FRAME_DATA_TYPE_STILL_RAW,
    FRAME_DATA_TYPE_METADATA_PRV,
    FRAME_DATA_TYPE_METADATA_STILL,
};


enum {
    FRAME_DATA_FORMAT_422I,
    FRAME_DATA_FORMAT_NV12,
    FRAME_DATA_FORMAT_NV21,
    FRAME_DATA_FORMAT_MOVIDIUS_RAW, // RAW10 packet
    FRAME_DATA_FORMAT_BINARY,  //  metadata
    FRAME_DATA_FORMAT_RAW_16,   // RAW16 packet
    FRAME_DATA_FORMAT_NV12_Y,
    FRAME_DATA_FORMAT_NV12_UV,
};

enum {
    SLICE_TYPE_Y,
    SLICE_TYPE_UV,
    SLICE_TYPE_YUV,
};

typedef struct {
   long int  version[4];
    long int  channel; //camera idx
    long int  width;
    long int  height;
    long int  bit;
    long int  bayeroder;
    long int  framecounter;
    long int timestamp1;
    long int timestamp2;
    long int headerlength;
} client_tx_frame_header_t;
#if 0
typedef struct {
    uint32_t chunk;
    char client_data[sizeof (client_tx_frame_header_t)];
} mipi_tx_header_still_image_t;
#endif
#define	SENSOR_MODE0 	1
#define	SENSOR_MODE1   	2
#define	SENSOR_MODE2    3
#define	DISK_MODE    	4
#define	BURNING_MODE    5


#endif /* MIPITXSTILLHEADER_H_ */
