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
 * @file readpgmraw.cpp
 *
 * @brief
 *   Implementation of read DCT PGM Raw format.
 *
 *****************************************************************************/
#include "cam_api/readpgmraw.h"

#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <ebase/dct_assert.h>

#include <iostream>
#include <fstream>
#include <string>

/******************************************************************************
 * local macro definitions
 *****************************************************************************/

CREATE_TRACER(CAM_API_READPGMRAW_INFO , "CAM_API_READPGMRAW: ", INFO,  0);
CREATE_TRACER(CAM_API_READPGMRAW_ERROR, "CAM_API_READPGMRAW: ", ERROR, 1);


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
 * ParseFloatArray
 *****************************************************************************/
static int ParseFloatArray
(
    const char  *c_string,          /**< trimmed c string */
    float       *values,            /**< pointer to memory */
    const int   num                 /**< number of expected float values */
)
{
    float *value = values;
    char *str    = (char *)c_string;

    int last = strlen( str );

    /* calc. end adress of string */
    char *str_last = str + (last-1);

    /* skip spaces */
    while ( *str == 0x20 )
    {
        str++;
    }

    int cnt = 0;
    int scanned;
    float f;

    /* parse the c-string */
    while ( (str <= str_last) && (cnt<num) )
    {
        scanned = sscanf(str, "%f", &f);
        if ( scanned != 1 )
        {
            goto err1;
        }
        else
        {
            value[cnt] = f;
            cnt++;
        }

        /* remove detected float */
        while ( (str <= str_last) && (*str != 0x20) )
        {
            str++;
        }

        /* skip spaces between numbers */
        while ( (str <= str_last) && (*str == 0x20) )
        {
            str++;
        }
    }

    return ( cnt );

err1:
    MEMSET( values, 0, (sizeof(float) * num) );

    return ( 0 );

}



/******************************************************************************
 * ParseUshortArray
 *****************************************************************************/
static int ParseUshortArray
(
    const char  *c_string,          /**< trimmed c string */
    uint16_t    *values,            /**< pointer to memory */
    const int   num                 /**< number of expected float values */
)
{
    uint16_t *value = values;
    char *str       = (char *)c_string;

    int last = strlen( str );

    /* calc. end adress of string */
    char *str_last = str + (last-1);

    /* skip spaces */
    while ( *str == 0x20 )
    {
        str++;
    }

    int cnt = 0;
    int scanned;
    uint16_t f;

    /* parse the c-string */
    while ( (str <= str_last) && (cnt<num) )
    {
        scanned = sscanf(str, "%hu", &f);
        if ( scanned != 1 )
        {
            goto err1;
        }
        else
        {
            value[cnt] = f;
            cnt++;
        }

        /* remove detected unsigned short */
        while ( (str <= str_last) && (*str != 0x20) )
        {
            str++;
        }

        /* skip spaces between numbers */
        while ( (str <= str_last) && (*str == 0x20) )
        {
            str++;
        }
    }

    return ( cnt );

err1:
    MEMSET( values, 0, (sizeof(uint16_t) * num) );

    return ( 0 );

}


/******************************************************************************
 * ParseShortArray
 *****************************************************************************/
static int ParseShortArray
(
    const char  *c_string,          /**< trimmed c string */
    int16_t     *values,            /**< pointer to memory */
    const int   num                 /**< number of expected float values */
)
{
    int16_t *value  = values;
    char *str       = (char *)c_string;

    int last = strlen( str );

    /* calc. end adress of string */
    char *str_last = str + (last-1);

    /* skip spaces */
    while ( *str == 0x20 )
    {
        str++;
    }

    int cnt = 0;
    int scanned;
    int16_t f;

    /* parse the c-string */
    while ( (str <= str_last) && (cnt<num) )
    {
        scanned = sscanf(str, "%hd", &f);
        if ( scanned != 1 )
        {
            goto err1;
        }
        else
        {
            value[cnt] = f;
            cnt++;
        }

        /* remove detected short value */
        while ( (str <= str_last) && (*str != 0x20) )
        {
            str++;
        }

        /* skip spaces between numbers */
        while ( (str <= str_last) && (*str == 0x20) )
        {
            str++;
        }
    }

    return ( cnt );

err1:
    MEMSET( values, 0, (sizeof(uint16_t) * num) );

    return ( 0 );

}



static bool readHeader
(
    std::ifstream           &file,
    uint32_t                &width,
    uint32_t                &height,
    uint32_t                &lineSize,
    PicBufType_t            &type,
    PicBufLayout_t          &layout,
    int64_t                 &timeStamp, 
    CamEngineWbGains_t      **wbGains,
    CamEngineCcMatrix_t     **ccMatrix,
    CamEngineCcOffset_t     **ccOffset,
    CamEngineBlackLevel_t   **blvl
)
{
    std::string line;
    std::string delimit = " \t\n";
    std::string word;
    size_t pos = 0;
    int step = 0;
    while ( true )
    {
      std::getline ( file, line);
        pos = 0;

        if ( file.eof() )
        {
            TRACE( CAM_API_READPGMRAW_ERROR, "%s (end of file)\n", __FUNCTION__ );
            return false;
        }

        if ( line[0] == '#' )
        {
            // check for DCT Raw
            {
                size_t n = line.find( "<Type>" );
                if ( std::string::npos != n )
                {
                    size_t k = line.find( "</Type>" );
                    if ( std::string::npos != k )
                    {
                        n += 6;
                        DCT_ASSERT( n < k );
                        word = line.substr( n, k-n );
                        type = (PicBufType_t)atoi ( word.c_str ( ) );
                    }
                }
            }
            {
                size_t n = line.find( "<Layout>" );
                if ( std::string::npos != n )
                {
                    size_t k = line.find( "</Layout>" );
                    if ( std::string::npos != k )
                    {
                        n += 8;
                        DCT_ASSERT( n < k );
                        word = line.substr( n, k-n );
                        layout = (PicBufLayout_t)atoi ( word.c_str ( ) );
                    }
                }
            }
            {
                size_t n = line.find( "<TimeStampUs>" );
                if ( std::string::npos != n )
                {
                    size_t k = line.find( "</TimeStampUs>" );
                    if ( std::string::npos != k )
                    {
                        n += 13;
                        DCT_ASSERT( n < k );
                        word = line.substr( n, k-n );
                        timeStamp = (int64_t)atoll ( word.c_str ( ) );
                    }
                }
            }
            {
                size_t n = line.find( "<WB>" );
                if ( std::string::npos != n )
                {
                    size_t k = line.find( "</WB>" );
                    if ( std::string::npos != k )
                    {
                        int no, i;
                        n += 4;
                        DCT_ASSERT( n < k );
                        word = line.substr( n, k-n );
                        *wbGains = (CamEngineWbGains_t *)malloc( sizeof(CamEngineWbGains_t) );
                        DCT_ASSERT( NULL != (*wbGains) );
                        i = ( sizeof(CamEngineWbGains_t) / sizeof((*wbGains)->Red) );
                        no = ParseFloatArray( word.c_str(), &((*wbGains)->Red), i );
                        DCT_ASSERT( i == no );
                    }
                }
            }
            {
                size_t n = line.find( "<CC>" );
                if ( std::string::npos != n )
                {
                    size_t k = line.find( "</CC>" );
                    if ( std::string::npos != k )
                    {
                        int no, i;
                        n += 4;
                        DCT_ASSERT( n < k );
                        word = line.substr( n, k-n );
                        *ccMatrix = (CamEngineCcMatrix_t *)malloc( sizeof(CamEngineCcMatrix_t) );
                        DCT_ASSERT( NULL != (*ccMatrix) );
                        i = ( sizeof(CamEngineCcMatrix_t) / sizeof((*ccMatrix)->Coeff[0]) );
                        no = ParseFloatArray( word.c_str(), &((*ccMatrix)->Coeff[0]), i );
                        DCT_ASSERT( i == no );
                    }
                }
            }
            {
                size_t n = line.find( "<CC-Offsets>" );
                if ( std::string::npos != n )
                {
                    size_t k = line.find( "</CC-Offsets>" );
                    if ( std::string::npos != k )
                    {
                        int no, i;
                        n += 12;
                        DCT_ASSERT( n < k );
                        word = line.substr( n, k-n );
                        *ccOffset = (CamEngineCcOffset_t *)malloc( sizeof(CamEngineCcOffset_t) );
                        DCT_ASSERT( NULL != (*ccOffset) );
                        i = ( sizeof(CamEngineCcOffset_t) / sizeof((*ccOffset)->Red) );
                        no = ParseShortArray( word.c_str(), &((*ccOffset)->Red), i );
                        DCT_ASSERT( i == no );
                    }
                }
            }
            {
                size_t n = line.find( "<BLS>" );
                if ( std::string::npos != n )
                {
                    size_t k = line.find( "</BLS>" );
                    if ( std::string::npos != k )
                    {
                        int no, i;
                        n += 5;
                        DCT_ASSERT( n < k );
                        word = line.substr( n, k-n );
                        *blvl = (CamEngineBlackLevel_t *)malloc( sizeof(CamEngineBlackLevel_t) );
                        DCT_ASSERT( NULL != (*blvl) );
                        i = ( sizeof(CamEngineBlackLevel_t) / sizeof((*blvl)->Red) );
                        no = ParseUshortArray( word.c_str(), &((*blvl)->Red), i );
                        DCT_ASSERT( i == no );
                    }
                }
            }
   
            continue;
        }

        if ( step == 0 )
        {
            // find word
            size_t n = line.find_first_not_of( delimit, pos );
            if ( std::string::npos == n )
            {
                continue;
            }
            size_t k = line.find_first_of( delimit, n );
            word = line.substr( n, k-n );
            pos = k;

            if ( word != "P5" )
            {
                TRACE( CAM_API_READPGMRAW_ERROR, "%s (wrong magic number)\n", __FUNCTION__ );
                return false;
            }
            step = 1;
        }

        if ( step == 1 )
        {
            // find word
            size_t n = line.find_first_not_of( delimit, pos );
            if ( std::string::npos == n )
            {
                continue;
            }
            size_t k = line.find_first_of( delimit, n );
            word = line.substr( n, k-n );
            pos = k;

            width = atoi ( word.c_str ( ) );
            step = 2;
        }

        if ( step == 2 )
        {
            // find word
            size_t n = line.find_first_not_of( delimit, pos );
            if ( std::string::npos == n )
            {
                continue;
            }
            size_t k = line.find_first_of( delimit, n );
            word = line.substr( n, k-n );
            pos = k;

            height = atoi ( word.c_str ( ) );
            step = 3;
        }

        if ( step == 3 )
        {
            // find word
            size_t n = line.find_first_not_of( delimit, pos );
            if ( std::string::npos == n )
            {
                continue;
            }
            size_t k = line.find_first_of( delimit, n );
            word = line.substr( n, k-n );
            pos = k;

            int max = atoi ( word.c_str ( ) );
            lineSize = ( max > 255 ) ? width << 1 : width;
            if ( PIC_BUF_TYPE_INVALID == type )
            {
                type = ( max > 255 ) ? PIC_BUF_TYPE_RAW16 : PIC_BUF_TYPE_RAW8;
            }
            break;
        }
    }

    return true;
}


static bool readData( std::ifstream &file, uint32_t width, uint32_t height, uint32_t lineSize, uint8_t *pBuffer )
{
    UNUSED_PARAM( width );

    uint8_t *pStart = pBuffer;

    for ( uint32_t j = 0; j < height; ++j, pStart += lineSize )
    {
        file.read ( (char *)pStart, lineSize );

        if ( file.eof() )
        {
            TRACE( CAM_API_READPGMRAW_ERROR, "%s (end of file)\n", __FUNCTION__ );
            return false;
        }
    }

    return true;
}


/******************************************************************************
 * See header file for detailed comment.
 *****************************************************************************/


bool
readRaw( const char* fileName, PicBufMetaData_t *pPicBuf )
{
    MEMSET( pPicBuf, 0, sizeof( PicBufMetaData_t ) );

    std::ifstream input;
    input.open ( fileName, std::ios::binary );

    if ( true != input.good() )
    {
        TRACE( CAM_API_READPGMRAW_ERROR, "%s (can't open the input file %s)\n", __FUNCTION__, fileName );
        return false;
    }

    // read header
    uint32_t width  = 0;
    uint32_t height = 0;
    uint32_t lineSize = 0;
    PicBufType_t    type      = PIC_BUF_TYPE_INVALID;
    PicBufLayout_t  layout    = PIC_BUF_LAYOUT_INVALID;
    int64_t         timeStamp = 0;

    CamEngineWbGains_t      *wbGains    = NULL;
    CamEngineCcMatrix_t     *ccMatrix   = NULL;
    CamEngineCcOffset_t     *ccOffset   = NULL;
    CamEngineBlackLevel_t   *blvl       = NULL;

    if ( true != readHeader( input, width, height, lineSize, type, layout, timeStamp, &wbGains, &ccMatrix, &ccOffset, &blvl ) )
    {
        TRACE( CAM_API_READPGMRAW_ERROR, "%s (can't read the image header)\n", __FUNCTION__ );
        input.close ( );
        return false;
    }

    // allocate memory
    uint8_t *pBuffer = (uint8_t *)malloc( lineSize * height );
    if ( NULL == pBuffer )
    {
        TRACE( CAM_API_READPGMRAW_ERROR, "%s (can't allocate memory)\n", __FUNCTION__ );
        input.close ( );
        return false;
    }

    // read data
    switch ( type )
    {
        case PIC_BUF_TYPE_RAW8:
        case PIC_BUF_TYPE_RAW16:
        {
            if ( true != readData( input, width, height, lineSize, pBuffer ) )
            {
                TRACE( CAM_API_READPGMRAW_ERROR, "%s (can't read the image data)\n", __FUNCTION__ );
                input.close ( );
                free ( pBuffer );
                return false;
            }
            break;
        }
        default:
        {
            TRACE( CAM_API_READPGMRAW_ERROR, "%s (unknown image format)\n", __FUNCTION__ );
            input.close ( );
            free ( pBuffer );
            return false;
        }
    }


    input.close ( );

    if ( ( NULL != wbGains  ) || 
         ( NULL != ccMatrix ) ||
         ( NULL != ccOffset ) || 
         ( NULL != blvl     ) )
    {
        raw_hdr_data_t *hdr_data = NULL;
        hdr_data = (raw_hdr_data_t *)malloc( sizeof( raw_hdr_data_t ) );
        hdr_data->wbGains   = wbGains;
        hdr_data->ccMatrix  = ccMatrix;
        hdr_data->ccOffset  = ccOffset;
        hdr_data->blvl      = blvl;
        pPicBuf->priv       = (void *)hdr_data;
    }
    else
    {
        pPicBuf->priv = NULL;
    }

    pPicBuf->Type = type;
    pPicBuf->Layout = layout;
    pPicBuf->TimeStampUs = timeStamp;

    pPicBuf->Data.raw.pBuffer = pBuffer;
    pPicBuf->Data.raw.PicWidthPixel  = width;
    pPicBuf->Data.raw.PicWidthBytes  = lineSize;
    pPicBuf->Data.raw.PicHeightPixel = height;

    return true;
}


