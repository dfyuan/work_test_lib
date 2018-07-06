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
 * @file    picture_buffer.c
 *
 * @brief   Implements helper functions around picture buffer meta data structure.
 *
 *****************************************************************************/

#include <common/return_codes.h>
#include <common/picture_buffer.h>

/******************************************************************************
 * local macro definitions
 *****************************************************************************/


/******************************************************************************
 * local type definitions
 *****************************************************************************/


/******************************************************************************
 * local variable declarations
 *****************************************************************************/


/******************************************************************************
 * local function prototypes
 *****************************************************************************/


/******************************************************************************
 * API Implementation below.
 * See header file for detailed comments.
 *****************************************************************************/

/*****************************************************************************
 *          PicBufIsConfigValid()
 *****************************************************************************/
RESULT PicBufIsConfigValid( PicBufMetaData_t *pPicBufMetaData)
{
    switch (pPicBufMetaData->Type)
    {
        case PIC_BUF_TYPE_DATA:
            switch (pPicBufMetaData->Layout)
            {
                case PIC_BUF_LAYOUT_COMBINED:
                    return RET_SUCCESS;
                case PIC_BUF_LAYOUT_SEMIPLANAR:
                case PIC_BUF_LAYOUT_PLANAR:
                    return RET_NOTSUPP;
                default:
                    return RET_OUTOFRANGE;
            };
            break;
        case PIC_BUF_TYPE_RAW8:
        case PIC_BUF_TYPE_RAW16:
            switch (pPicBufMetaData->Layout)
            {
                case PIC_BUF_LAYOUT_BAYER_RGRGGBGB:
                case PIC_BUF_LAYOUT_BAYER_GRGRBGBG:
                case PIC_BUF_LAYOUT_BAYER_GBGBRGRG:
                case PIC_BUF_LAYOUT_BAYER_BGBGGRGR:
                case PIC_BUF_LAYOUT_COMBINED:
                    return RET_SUCCESS;
                case PIC_BUF_LAYOUT_SEMIPLANAR:
                case PIC_BUF_LAYOUT_PLANAR:
                    return RET_NOTSUPP;
                default:
                    return RET_OUTOFRANGE;
            };
            break;
        case PIC_BUF_TYPE_JPEG:
            switch (pPicBufMetaData->Layout)
            {
                case PIC_BUF_LAYOUT_COMBINED:
                    return RET_SUCCESS;
                case PIC_BUF_LAYOUT_SEMIPLANAR:
                case PIC_BUF_LAYOUT_PLANAR:
                    return RET_NOTSUPP;
                default:
                    return RET_OUTOFRANGE;
            };
            break;
        case PIC_BUF_TYPE_YCbCr444:
            switch (pPicBufMetaData->Layout)
            {
                case PIC_BUF_LAYOUT_COMBINED:
                case PIC_BUF_LAYOUT_PLANAR:
                    return RET_SUCCESS;
                case PIC_BUF_LAYOUT_SEMIPLANAR:
                    return RET_NOTSUPP;
                default:
                    return RET_OUTOFRANGE;
            };
            break;
        case PIC_BUF_TYPE_YCbCr422:
            switch (pPicBufMetaData->Layout)
            {
                case PIC_BUF_LAYOUT_COMBINED:
                case PIC_BUF_LAYOUT_SEMIPLANAR:
                case PIC_BUF_LAYOUT_PLANAR:
                    return RET_SUCCESS;
                default:
                    return RET_OUTOFRANGE;
            };
            break;
        case PIC_BUF_TYPE_YCbCr420:
            switch (pPicBufMetaData->Layout)
            {
                case PIC_BUF_LAYOUT_SEMIPLANAR:
                case PIC_BUF_LAYOUT_PLANAR:
                    return RET_SUCCESS;
                case PIC_BUF_LAYOUT_COMBINED:
                    return RET_NOTSUPP;
                default:
                    return RET_OUTOFRANGE;
            };
            break;
        case PIC_BUF_TYPE_RGB565:
        case PIC_BUF_TYPE_RGB666:
        case PIC_BUF_TYPE_RGB888:
            switch (pPicBufMetaData->Layout)
            {
                case PIC_BUF_LAYOUT_COMBINED:
                    return RET_SUCCESS;
                case PIC_BUF_LAYOUT_PLANAR:
                case PIC_BUF_LAYOUT_SEMIPLANAR:
                    return RET_NOTSUPP;
                default:
                    return RET_OUTOFRANGE;
            };
            break;
        default:
            break;
    }

    // invalid configuration
    return RET_OUTOFRANGE;
}
