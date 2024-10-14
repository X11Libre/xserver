/* Prototypes for functions that the DDX must provide */

#ifndef _DPMSPROC_H_
#define _DPMSPROC_H_

#include "dixstruct.h"

extern int DPMSSet(ClientPtr client, int level);
extern Bool DPMSSupported(void);

extern CARD32 DPMSStandbyTime;
extern CARD32 DPMSSuspendTime;
extern CARD32 DPMSOffTime;
extern CARD16 DPMSPowerLevel;
extern Bool DPMSEnabled;
extern Bool DPMSDisabledSwitch;

#endif
