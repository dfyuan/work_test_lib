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
 * @file    cea_861.c
 *
 * @brief   Implements CEA 861 style video format stuff.
 *
 *****************************************************************************/

#include <ebase/types.h>
#include <common/cea_861.h>

/******************************************************************************
 * local macro definitions
 *****************************************************************************/


/******************************************************************************
 * local type definitions
 *****************************************************************************/


/******************************************************************************
 * local variable declarations
 *****************************************************************************/

/*****************************************************************************
 *          Cea861VideoFormats()
 *****************************************************************************/
const Cea861VideoFormatDetails_t Cea861VideoFormats[] =
{   // Remember: All counting starts at 1 (not 0) in CEA spec and so do we!
    //           All position numbers reflect the first position a condition is true or false, in particular:
    //           - Xstart marks the first line or pixel a signal/data is active/valid
    //           - Xstop  marks the first line or pixel a signal/data is inactive/invalid; _not_ the last line/pixel it is active/valid!
    //
    // Note: *) = Add 1 (one) to the numbers taken from the spec in the table below, as we need the position of the pixel and not the number of pixels before it.
    //
    //           FormatID                                                      szFormatName               Vactive           PixClk         Htotal         HsyncStop *)      HfieldPos    VsyncStart     VactStart     HsyncPolarity   FsyncPolarity
    //                                         FormatIDInfoFrame                                    Hactive     Progressive          PixRep      HsyncStart *)     HactStart *)     Vtotal       VsyncStop   VactStartDelay   VsyncPolarity    EnPolarity
    //                                |                                  |                         |      |      |       |           |    |      |        |        |        |      |      |      |       |      |      |       |       |       |       |
    {CEA_861_VIDEOFORMAT_640x480p60   , CEA_861_VIDEOFORMAT_640x480p60   , "CEA_861D_640x480p60"   ,  640 ,  480 ,  true ,  25200000 ,  1 ,  800 ,   16+1 ,  112+1 ,  160+1 ,    0 ,  525 ,    1 ,     3 ,   36 ,    0 , false , false , false ,  true },
    {CEA_861_VIDEOFORMAT_1280x720p60  , CEA_861_VIDEOFORMAT_1280x720p60  , "CEA_861D_1280x720p60"  , 1280 ,  720 ,  true ,  74250000 ,  1 , 1650 ,  110+1 ,  150+1 ,  370+1 ,    0 ,  750 ,    1 ,     6 ,   30 ,    0 ,  true ,  true , false ,  true },
    {CEA_861_VIDEOFORMAT_1920x1080p24 , CEA_861_VIDEOFORMAT_1920x1080p24 , "CEA_861D_1920x1080p24" , 1920 , 1080 ,  true ,  74250000 ,  1 , 2750 ,  638+1 ,  682+1 ,  830+1 ,    0 , 1125 ,    1 ,     6 ,   42 ,    0 ,  true ,  true , false ,  true },
    {CEA_861_VIDEOFORMAT_1920x1080p25 , CEA_861_VIDEOFORMAT_1920x1080p25 , "CEA_861D_1920x1080p25" , 1920 , 1080 ,  true ,  74250000 ,  1 , 2640 ,  528+1 ,  572+1 ,  720+1 ,    0 , 1125 ,    1 ,     6 ,   42 ,    0 ,  true ,  true , false ,  true },
    {CEA_861_VIDEOFORMAT_1920x1080p30 , CEA_861_VIDEOFORMAT_1920x1080p30 , "CEA_861D_1920x1080p30" , 1920 , 1080 ,  true ,  74250000 ,  1 , 2200 ,   88+1 ,  132+1 ,  280+1 ,    0 , 1125 ,    1 ,     6 ,   42 ,    0 ,  true ,  true , false ,  true },
    {CEA_861_VIDEOFORMAT_1920x1080p50 , CEA_861_VIDEOFORMAT_1920x1080p50 , "CEA_861D_1920x1080p50" , 1920 , 1080 ,  true , 148500000 ,  1 , 2640 ,  528+1 ,  572+1 ,  720+1 ,    0 , 1125 ,    1 ,     6 ,   42 ,    0 ,  true ,  true , false ,  true },
    {CEA_861_VIDEOFORMAT_1920x1080p60 , CEA_861_VIDEOFORMAT_1920x1080p60 , "CEA_861D_1920x1080p60" , 1920 , 1080 ,  true , 148500000 ,  1 , 2200 ,   88+1 ,  132+1 ,  280+1 ,    0 , 1125 ,    1 ,     6 ,   42 ,    0 ,  true ,  true , false ,  true },
     // the next line must be the last one in the array!!!
    {CEA_861_VIDEOFORMAT_INVALID      , CEA_861_VIDEOFORMAT_INVALID      , "<invalid>"             ,    0 ,    0 , false ,         0 ,  0 ,    0 ,      0 ,      0 ,      0 ,    0 ,    0 ,    0 ,     0 ,    0 ,    0 , false , false , false , false }
};



/******************************************************************************
 * local function prototypes
 *****************************************************************************/


/******************************************************************************
 * API Implementation below.
 * See header file for detailed comments.
 *****************************************************************************/

/*****************************************************************************
 *          Cea861GetVideoFormatDetails()
 *****************************************************************************/
const Cea861VideoFormatDetails_t* Cea861GetVideoFormatDetails
(
    Cea861VideoFormat_t FormatID
)
{
    uint32_t index = 0;

    for (index = 0; (Cea861VideoFormats[index].FormatID != (CEA_861_VIDEOFORMAT_INVALID)) && (Cea861VideoFormats[index].FormatID != FormatID); index++);

    return (Cea861VideoFormats[index].FormatID != (CEA_861_VIDEOFORMAT_INVALID)) ? &Cea861VideoFormats[index] : NULL;
}

/*****************************************************************************
 *          Cea861GetVideoFormatDetails()
 *****************************************************************************/
bool_t Cea861AlignVideoFormatDetails
(
    Cea861VideoFormatDetails_t *pVideoFormatDetails
)
{
    if (!pVideoFormatDetails)
    {
        return false;
    }

    // try to determine required horizontal shift so that last active pixel can be moved to the position of the last pixel in line
    int32_t HShift = pVideoFormatDetails->Htotal - (pVideoFormatDetails->HactStart + pVideoFormatDetails->Hactive) + 1;
    if (HShift < 0)
    {
        return false;
    }

    // try to determine required vertical shift so that last active line can be moved to the position of the last line in frame/field
    int32_t VShift = pVideoFormatDetails->Vtotal - (pVideoFormatDetails->VactStart + pVideoFormatDetails->VactStartDelay + pVideoFormatDetails->Vactive) + 1;
    if (VShift < 0)
    {
        return false;
    }

    // do the shift
    pVideoFormatDetails->HsyncStart += HShift;
    pVideoFormatDetails->HsyncStop  += HShift;
    pVideoFormatDetails->HactStart  += HShift;
    pVideoFormatDetails->HfieldPos  += HShift;
    pVideoFormatDetails->VsyncStart += VShift;
    pVideoFormatDetails->VsyncStop  += VShift;
    pVideoFormatDetails->VactStart  += VShift;

    return true;
}