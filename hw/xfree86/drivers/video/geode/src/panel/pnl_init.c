/* Copyright (c) 2005 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * */

/* 
 * File Contents:   This file contains the Geode frame buffer panel
 *                  initialization functions.
 * 
 * SubModule:       Geode FlatPanel library
 * */

#include "panel.h"
#include "gfx_regs.h"
#include "gfx_type.h"

/* defaults 
 * Panel: 		Enabled
 * Platform: 		Centaurus
 * 92xx Chip: 		9211 Rev. A
 * PanelType: 		DSTN
 * XResxYRes: 		800x600
 * Depth: 		16
 * Mono_Color: 		Color
 */
static Pnl_PanelParams sPanelParam = {
    0, 1, CENTAURUS_PLATFORM, PNL_9211_A,
    {PNL_DSTN, 800, 600, 16, PNL_COLOR_PANEL}
};

#if PLATFORM_DRACO
#include "drac9210.c"
#endif

#if PLATFORM_CENTAURUS
#include "cen9211.c"
#endif

#if PLATFORM_DORADO
#include "dora9211.c"
#endif

#if  PLATFORM_GX2BASED
#include "gx2_9211.c"
#endif
#include "platform.c"

/*
 *	return -1 - UnKnown
 *	0 - Draco has 9210
 *	1 - Centaurus has 9211 Rev. A
 *	2 - Dorado has 9211 Rev. C
 */

/*-----------------------------------------------------------------
 * Pnl_SetPlatform
 *
 * Description:		This function sets the panel with hardware platform.
 * 
 * parameters:
 *		platform:	It specify the platform.
 * 
 * return:			none.
 *-----------------------------------------------------------------*/
void
Pnl_SetPlatform(int platform)
{
    /* To Be Implemented */
    sPanelParam.Platform = platform;

}

/*-----------------------------------------------------------------
 * Pnl_GetPlatform
 *
 * Description:		This function returns the panel platform.
 * 
 * parameters:		none.
 * 
 * return:			On success it returns the panel platform.
 *-----------------------------------------------------------------*/
int
Pnl_GetPlatform(void)
{
    sPanelParam.Platform = Detect_Platform();

    return sPanelParam.Platform;

}

/*-----------------------------------------------------------------
 * Pnl_IsPanelPresent
 *
 * Description:		This function specifies whether the panel is prsent
 *					or not.
 *					
 * parameters: 		none.
 * 
 * return: 			On success it returns an integer panel present value.
 *-----------------------------------------------------------------*/
int
Pnl_IsPanelPresent(void)
{
    /* To Be Implemented */
    return sPanelParam.PanelPresent;
}

/*-----------------------------------------------------------------
 * Pnl_SetPanelPresent
 *
 * Description:		This function sets the panel_present parameter.
 * 
 * parameters:
 * 		present:	It specifies the panel present value.
 * 		
 * return:			none.
 *-----------------------------------------------------------------*/
void
Pnl_SetPanelPresent(int present)
{
    /* To Be Implemented */
    sPanelParam.PanelPresent = present;
}

/*-----------------------------------------------------------------
 * Pnl_SetPanelParam
 *
 * Description:		This function sets the panel parameters
 * 
 * parameters:
 *		pParam:		It specifies the elements of the panel parameter
 *					structure.
 * 
 * return:	none.
 *-----------------------------------------------------------------*/
void
Pnl_SetPanelParam(Pnl_PanelParams *pParam)
{
    if (pParam->Flags & PNL_PANELPRESENT) {
        Pnl_SetPanelPresent(pParam->PanelPresent);
    }
    if (pParam->Flags & PNL_PLATFORM) {
        Pnl_SetPlatform(pParam->Platform);
    }
    if (pParam->Flags & PNL_PANELCHIP) {
        Pnl_SetPanelChip(pParam->PanelChip);
    }
    if (pParam->Flags & PNL_PANELSTAT) {
        sPanelParam.PanelStat.XRes = pParam->PanelStat.XRes;
        sPanelParam.PanelStat.YRes = pParam->PanelStat.YRes;
        sPanelParam.PanelStat.Depth = pParam->PanelStat.Depth;
        sPanelParam.PanelStat.MonoColor = pParam->PanelStat.MonoColor;
        sPanelParam.PanelStat.Type = pParam->PanelStat.Type;
    }
}

/*-----------------------------------------------------------------
 * Pnl_PowerUp
 *
 * Description:		This function sets the power based on the 
 *					hardware platforms dorado or centaraus.
 * 
 * parameters:		none.
 * 
 * return:			none.
 *-----------------------------------------------------------------*/
void
Pnl_PowerUp(void)
{
    int Platform;
    unsigned long dcfg, hw_video;

    Platform = Pnl_GetPlatform();

#if PLATFORM_CENTAURUS
    if (Platform == CENTAURUS_PLATFORM) {
        Centaurus_Power_Up();
        return;
    }
#endif

#if PLATFORM_DORADO
    if (Platform == DORADO_PLATFORM) {
        Dorado_Power_Up();
        return;
    }
#endif

#if PLATFORM_GX2BASED
    if (Platform == REDCLOUD_PLATFORM) {
    }
#endif

    hw_video = gfx_detect_video();

    if (hw_video == GFX_VID_CS5530) {
        /* READ DISPLAY CONFIG FROM CX5530 */
        dcfg = READ_VID32(CS5530_DISPLAY_CONFIG);

        /* SET RELEVANT FIELDS */
        dcfg |= (CS5530_DCFG_FP_PWR_EN | CS5530_DCFG_FP_DATA_EN);
        /* Enable the flatpanel power and data */
        WRITE_VID32(CS5530_DISPLAY_CONFIG, dcfg);
    }
    else if (hw_video == GFX_VID_SC1200) {
        /* READ DISPLAY CONFIG FROM SC1200 */
        dcfg = READ_VID32(SC1200_DISPLAY_CONFIG);

        /* SET RELEVANT FIELDS */
        dcfg |= (SC1200_DCFG_FP_PWR_EN | SC1200_DCFG_FP_DATA_EN);
        /* Enable the flatpanel power and data */
        WRITE_VID32(SC1200_DISPLAY_CONFIG, dcfg);
    }
    else if (hw_video == GFX_VID_REDCLOUD) {
        /* READ DISPLAY CONFIG FROM REDCLOUD */
        dcfg = READ_VID32(RCDF_DISPLAY_CONFIG);

        /* SET RELEVANT FIELDS */
        dcfg |= (RCDF_DCFG_FP_PWR_EN | RCDF_DCFG_FP_DATA_EN);
        /* Enable the flatpanel power and data */
        WRITE_VID32(RCDF_DISPLAY_CONFIG, dcfg);
    }

}

/*-----------------------------------------------------------------
 * Pnl_PowerDown
 *
 * Description:		This function make power down based on the 
 *					hardware platforms dorado or centaraus.
 *					
 * parameters:		none.
 * 
 * return:			none.
 *-----------------------------------------------------------------*/
void
Pnl_PowerDown(void)
{
    int Platform;
    unsigned long dcfg, hw_video;

    Platform = Pnl_GetPlatform();

#if PLATFORM_CENTAURUS
    if (Platform == CENTAURUS_PLATFORM) {
        Centaurus_Power_Down();
        return;
    }
#endif
#if PLATFORM_DORADO
    if (Platform == DORADO_PLATFORM) {
        Dorado_Power_Down();
        return;
    }
#endif

#if PLATFORM_GX2BASED
    if (Platform == REDCLOUD_PLATFORM) {
    }
#endif

    hw_video = gfx_detect_video();

    if (hw_video == GFX_VID_CS5530) {
        /* READ DISPLAY CONFIG FROM CX5530 */
        dcfg = READ_VID32(CS5530_DISPLAY_CONFIG);

        /* CLEAR RELEVANT FIELDS */
        dcfg &= ~(CS5530_DCFG_FP_PWR_EN | CS5530_DCFG_FP_DATA_EN);
        /* Disable the flatpanel power and data */
        WRITE_VID32(CS5530_DISPLAY_CONFIG, dcfg);
    }
    else if (hw_video == GFX_VID_SC1200) {
        /* READ DISPLAY CONFIG FROM SC1200 */
        dcfg = READ_VID32(SC1200_DISPLAY_CONFIG);

        /* CLEAR RELEVANT FIELDS */
        dcfg &= ~(SC1200_DCFG_FP_PWR_EN | SC1200_DCFG_FP_DATA_EN);
        /* Disable the flatpanel power and data */
        WRITE_VID32(SC1200_DISPLAY_CONFIG, dcfg);
    }
    else if (hw_video == GFX_VID_REDCLOUD) {
        /* READ DISPLAY CONFIG FROM REDCLOUD */
        dcfg = READ_VID32(RCDF_DISPLAY_CONFIG);

        /* CLEAR RELEVANT FIELDS */
        dcfg &= ~(RCDF_DCFG_FP_PWR_EN | RCDF_DCFG_FP_DATA_EN);
        /* Disable the flatpanel power and data */
        WRITE_VID32(RCDF_DISPLAY_CONFIG, dcfg);
    }
}

/*-----------------------------------------------------------------
 * Pnl_SavePanelState
 *
 * Description:		This function saves the panel state based on the 
 *					hardware platforms dorado or centaraus.
 *					
 * parameters:		none.
 * 
 * return:			none.
 *-----------------------------------------------------------------*/
void
Pnl_SavePanelState(void)
{
    int Platform;

    Platform = Pnl_GetPlatform();

#if PLATFORM_CENTAURUS
    if (Platform == CENTAURUS_PLATFORM) {
        Centaurus_Save_Panel_State();
        return;
    }
#endif

#if PLATFORM_DORADO
    if (Platform == DORADO_PLATFORM) {
        Dorado_Save_Panel_State();
        return;
    }
#endif

#if PLATFORM_GX2BASED
    if (Platform == REDCLOUD_PLATFORM) {
    }
#endif
}

/*-----------------------------------------------------------------
 * Pnl_RestorePanelState
 *
 * Description:		This function restore the panel state based on the 
 *					hardware platforms dorado or centaraus.
 *					
 * parameters:		none.
 * 
 * return:			none.
 *-----------------------------------------------------------------*/
void
Pnl_RestorePanelState(void)
{
    int Platform;

    Platform = Pnl_GetPlatform();
#if PLATFORM_CENTAURUS
    if (Platform == CENTAURUS_PLATFORM) {
        Centaurus_Restore_Panel_State();
        return;
    }
#endif

#if PLATFORM_DORADO
    if (Platform == DORADO_PLATFORM) {
        Dorado_Restore_Panel_State();
        return;
    }
#endif

#if PLATFORM_GX2BASED
    if (Platform == REDCLOUD_PLATFORM) {
    }
#endif
}

/*-----------------------------------------------------------------
 * Pnl_GetPanelParam
 *
 * Description:		This function gets the panel parameters from the
 *					hardware platforms dorado or centaraus.
 *					
 * parameters:
 * 		pParam:		It specifies the elements of the panel parameter
 *					structure.
 *					
 * return:			none.
 *-----------------------------------------------------------------*/
void
Pnl_GetPanelParam(Pnl_PanelParams *pParam)
{
    if (pParam->Flags & PNL_PANELPRESENT) {
        pParam->PanelPresent = Pnl_IsPanelPresent();
    }
    if (pParam->Flags & PNL_PLATFORM) {
        pParam->Platform = Pnl_GetPlatform();
    }
    if ((pParam->Flags & PNL_PANELCHIP) || (pParam->Flags & PNL_PANELSTAT)) {
#if PLATFORM_CENTAURUS
        if (pParam->Platform == CENTAURUS_PLATFORM) {
            Centaurus_Get_9211_Details(pParam->Flags, pParam);
            return;
        }
#endif

#if PLATFORM_DORADO
        if (pParam->Platform == DORADO_PLATFORM) {
            Dorado_Get_9211_Details(pParam->Flags, pParam);
            return;
        }
#endif

#if PLATFORM_GX2BASED
        if (pParam->Platform == REDCLOUD_PLATFORM) {
        }
#endif

        /* if unknown platform put unknown */
        if (pParam->Flags & PNL_PANELCHIP)
            pParam->PanelChip = PNL_UNKNOWN_CHIP;

        if (pParam->Flags & PNL_PANELSTAT) {
            pParam->PanelStat.XRes = 0;
            pParam->PanelStat.YRes = 0;
            pParam->PanelStat.Depth = 0;
            pParam->PanelStat.MonoColor = PNL_UNKNOWN_COLOR;
            pParam->PanelStat.Type = PNL_UNKNOWN_PANEL;
        }
    }
}

/*-----------------------------------------------------------------
 * Pnl_SetPanelChip
 *
 * Description:		This function sets the panelchip parameter of panel
 *					structure.
 *					
 * parameters:
 *   	panelChip:	It specifies the panelChip value.
 *   	
 * return:			none. 
 *-----------------------------------------------------------------*/

void
Pnl_SetPanelChip(int panelChip)
{
    /* To Be Implemented */
    sPanelParam.PanelChip = panelChip;

}

/*-----------------------------------------------------------------
 * Pnl_GetPanelChip
 *
 * Description:		This function gets the panelchip parameter of panel
 *					structure.
 *					
 * parameters:		none
 * 
 * return:			On success it returns the panelchip. 
 *-----------------------------------------------------------------*/
int
Pnl_GetPanelChip(void)
{
    /* To Be Implemented */
    return sPanelParam.PanelChip;
}

/*-----------------------------------------------------------------
 * Pnl_InitPanel
 *
 * Description:		This function initializes the panel with
 *					hardware platforms dorado or centaraus.
 *					
 * parameters:
 * 		pParam:		It specifies the elements of the panel parameter
 *					structure.
 *					
 * return:			none.
 *-----------------------------------------------------------------*/
int
Pnl_InitPanel(Pnl_PanelParams *pParam)
{
    Pnl_PanelParams *pPtr;

    if (pParam == 0x0)          /* NULL  use the static table */
        pPtr = &sPanelParam;
    else
        pPtr = pParam;

    if (!pPtr->PanelPresent) {
        return -1;              /* error */
    }
    else {
        if ((pPtr->PanelChip < 0) || (pPtr->Platform < 0))
            return -1;          /* error */

#if PLATFORM_DRACO
        /* check we are init. the right one */
        if ((pPtr->Platform == DRACO_PLATFORM)
            && (pPtr->PanelChip == PNL_9210)) {
            Draco9210Init(&(pPtr->PanelStat));
        }
#endif

#if PLATFORM_CENTAURUS
        /* check we are init. the right one */
        if (pPtr->Platform == CENTAURUS_PLATFORM) {
            Centaurus_9211init(&(pPtr->PanelStat));
        }
#endif

#if PLATFORM_DORADO
        /* check we are init. the right one */
        if ((pPtr->Platform == DORADO_PLATFORM) &&
            (pPtr->PanelChip == PNL_9211_C)) {
            Dorado9211Init(&(pPtr->PanelStat));
        }
#endif
#if PLATFORM_GX2BASED
        if (pPtr->Platform == REDCLOUD_PLATFORM) {
            Redcloud_9211init(&(pPtr->PanelStat));
        }
#endif
    }                           /* else  end */
    return 1;
}
