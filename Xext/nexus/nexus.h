#ifndef _NEXUS_H_
#define _NEXUS_H_

#include <X11/X.h>
#include "screenint.h"

/* Initialize nexus (called before InitOutput) */
Bool NexusPreInit(void);
Bool NexusPostInit(void);

/* Extension init function for runtime enable/disable via +extension/-extension
   (called from InitExtensions) */
void NexusExtensionInit(void);

/* Register a physical screen with nexus (called from AddScreen) */
void nexus_register_physical_screen(ScreenPtr pScreen);

/* The dummy screen init function (used to identify nexus screen in AddScreen) */
int nexus_dummy_screen_init(ScreenPtr pScreen, int argc, char **argv);

/* Validate all physical screens */
Bool nexus_validate_all_physical(void);

#define NEXUS_DUMMY_ID 0

#endif