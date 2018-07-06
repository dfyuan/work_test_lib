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
 * @file    hdmi_3d.c
 *
 * @brief   Implements HDMI 3D style video format stuff.
 *
 *****************************************************************************/

#include <ebase/types.h>
#include <common/cea_861.h>

#include <string.h> // for memcpy()

#include <common/hdmi_3d.h>

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
 *          Hdmi3DVideoFormats()
 *****************************************************************************/
const Hdmi3DVideoFormatDetails_t Hdmi3DVideoFormats[] =
{   // Remember: All counting starts at 1 (not 0) in CEA spec and so do we!
    //               FormatID                                                          | Progressive         | Interlaced
    //                                                     szFormatName                |   Supported         |   Supported
    //                                      |                                          |         | SubImages |         | SubImages
    {HDMI_3D_VIDEOFORMAT_FRAME_PACKING      , "HDMI_3D_VIDEOFORMAT_FRAME_PACKING"      , { true  ,       2 } , { true  ,       4 } },
    {HDMI_3D_VIDEOFORMAT_FIELD_ALTERNATIVE  , "HDMI_3D_VIDEOFORMAT_FIELD_ALTERNATIVE"  , { false ,       0 } , { true  ,       4 } },
    {HDMI_3D_VIDEOFORMAT_LINE_ALTERNATIVE   , "HDMI_3D_VIDEOFORMAT_LINE_ALTERNATIVE"   , { true  ,       2 } , { true  ,       2 } },
    {HDMI_3D_VIDEOFORMAT_SIDE_BY_SIDE_FULL  , "HDMI_3D_VIDEOFORMAT_SIDE_BY_SIDE_FULL"  , { true  ,       2 } , { true  ,       2 } },
    {HDMI_3D_VIDEOFORMAT_L_DEPTH            , "HDMI_3D_VIDEOFORMAT_L_DEPTH"            , { true  ,       2 } , { false ,       0 } },
    {HDMI_3D_VIDEOFORMAT_L_DEPTH_GRFX_DEPTH , "HDMI_3D_VIDEOFORMAT_L_DEPTH_GRFX_DEPTH" , { true  ,       4 } , { false ,       0 } },
    {HDMI_3D_VIDEOFORMAT_TOP_AND_BOTTOM     , "HDMI_3D_VIDEOFORMAT_TOP_AND_BOTTOM"     , { true  ,       2 } , { true  ,       2 } },
    {HDMI_3D_VIDEOFORMAT_SIDE_BY_SIDE_HALF  , "HDMI_3D_VIDEOFORMAT_SIDE_BY_SIDE_HALF"  , { true  ,       2 } , { true  ,       2 } },
     // the next line must be the last one in the array!!!
    {HDMI_3D_VIDEOFORMAT_INVALID            , "<invalid>"                              , { false ,       0 } , { false ,       0 } }
};

// NOTE: intentional redefinition as a safety net; don't forget to update definition in header file!
#define HDMI_3D_MAX_NUM_SUBIMAGES 4 // Value must match the highest SubImages number in Hdmi3DVideoFormats table above.


/******************************************************************************
 * local function prototypes
 *****************************************************************************/


/******************************************************************************
 * API Implementation below.
 * See header file for detailed comments.
 *****************************************************************************/

/*****************************************************************************
 *          Hdmi3DGetNumSubImages()
 *****************************************************************************/
uint16_t Hdmi3DGetNumSubImages
(
    const Hdmi3DVideoFormat_t        FormatID,
    const Cea861VideoFormatDetails_t *pBaseCeaFormatDetails
)
{
    const Hdmi3DVideoFormatDetails_t *pHdmi3DDetails;

    // check params
    if (pBaseCeaFormatDetails == NULL)
    {
        return 0;
    }

    pHdmi3DDetails = Hdmi3DGet3DFormatDetails( FormatID );
    if (pHdmi3DDetails == NULL)
    {
        return 0;
    }

    bool_t Progressive = pBaseCeaFormatDetails->Progressive;

    if ( ( Progressive && !pHdmi3DDetails->Progressive.Supported)
      || (!Progressive && !pHdmi3DDetails->Interlaced.Supported ) )
    {
        return 0;
    }

    if (Progressive)
    {
        return pHdmi3DDetails->Progressive.SubImages;
    }
    else
    {
        return pHdmi3DDetails->Interlaced.SubImages;
    }
}


/*****************************************************************************
 *          Hdmi3DGetVideoFormatDetails()
 *****************************************************************************/
RESULT Hdmi3DGetVideoFormatDetails
(
    const Hdmi3DVideoFormat_t        FormatID,
    const uint16_t                   SubImageNum,
    const Cea861VideoFormatDetails_t *pBaseCeaFormatDetails,
    Cea861VideoFormatDetails_t       *pSubCeaFormatDetails
)
{
    const Hdmi3DVideoFormatDetails_t *pHdmi3DDetails;

    // check params
    if ((pBaseCeaFormatDetails == NULL) || (pSubCeaFormatDetails == NULL))
    {
        return RET_NULL_POINTER;
    }

    pHdmi3DDetails = Hdmi3DGet3DFormatDetails( FormatID );
    if (pHdmi3DDetails == NULL)
    {
        return RET_INVALID_PARM;
    }

    bool_t Progressive = pBaseCeaFormatDetails->Progressive;

    if ( ( Progressive && !pHdmi3DDetails->Progressive.Supported)
      || (!Progressive && !pHdmi3DDetails->Interlaced.Supported ) )
    {
        return RET_NOTSUPP;
    }

    if ( ( Progressive && (SubImageNum >= pHdmi3DDetails->Progressive.SubImages))
      || (!Progressive && (SubImageNum >= pHdmi3DDetails->Interlaced.SubImages) ) )
    {
        return RET_OUTOFRANGE;
    }

    // copy base timing details into sub image timing details
    *pSubCeaFormatDetails = *pBaseCeaFormatDetails;

    // right & bottom align the active video area in the timing as it is required for the following 3D mode timing adjustments
    if ( !Cea861AlignVideoFormatDetails( pSubCeaFormatDetails ) )
    {
        return RET_WRONG_CONFIG;
    }

    // update sub image timing details as required by format
    switch (FormatID)
    {
        case HDMI_3D_VIDEOFORMAT_FRAME_PACKING:
        {
            if (Progressive)
            {
                pSubCeaFormatDetails->PixClk *= 2; //TODO: maybe better try to reduce any pixel repetition first to keep pixel frequency as low as possible?
                if (SubImageNum == 1)
                {
                    pSubCeaFormatDetails->VsyncStart = 0; // CEA 861 starts counting with 1, so this is an invalid value and may be used by drivers to detect that no VSync has to be generated.
                    pSubCeaFormatDetails->VsyncStop  = 0; // VsyncStart == VsyncStop may be used by drivers to detect that no VSync has to be generated.
                    pSubCeaFormatDetails->VactStartDelay += pSubCeaFormatDetails->VactStart-1; // delay start of real video data
                    pSubCeaFormatDetails->VactStart  = 1; // but immediately start active video area
                }
                break;
            }
            else
            {
                return RET_NOTSUPP;
            }
        }
        case HDMI_3D_VIDEOFORMAT_FIELD_ALTERNATIVE:
        case HDMI_3D_VIDEOFORMAT_LINE_ALTERNATIVE:
        case HDMI_3D_VIDEOFORMAT_SIDE_BY_SIDE_FULL:
        case HDMI_3D_VIDEOFORMAT_L_DEPTH:
        case HDMI_3D_VIDEOFORMAT_L_DEPTH_GRFX_DEPTH:
        case HDMI_3D_VIDEOFORMAT_TOP_AND_BOTTOM:
        case HDMI_3D_VIDEOFORMAT_SIDE_BY_SIDE_HALF:
            return RET_NOTSUPP;
        default:
            return RET_INVALID_PARM;
    }

    // that's it
    return RET_SUCCESS;
}


/******************************************************************************
 * Local function implementation below.
 *****************************************************************************/

/*****************************************************************************
 *          Hdmi3DGet3DFormatDetails()
 *****************************************************************************/
const Hdmi3DVideoFormatDetails_t* Hdmi3DGet3DFormatDetails
(
    Hdmi3DVideoFormat_t FormatID
)
{
    uint32_t index = 0;

    for (index = 0; (Hdmi3DVideoFormats[index].FormatID != (HDMI_3D_VIDEOFORMAT_INVALID)) && (Hdmi3DVideoFormats[index].FormatID != FormatID); index++);

    return (Hdmi3DVideoFormats[index].FormatID != (HDMI_3D_VIDEOFORMAT_INVALID)) ? &Hdmi3DVideoFormats[index] : NULL;
}



