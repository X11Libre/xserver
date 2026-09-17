#ifndef _NEXUS_H_
#define _NEXUS_H_

#include <stdbool.h>
#include <X11/X.h>

/* Initialize nexus (called before InitOutput) */
bool NexusPreInit(void);
bool NexusPostInit(void);

/* Extension init function for runtime enable/disable via +extension/-extension
   (called from InitExtensions) */
void NexusExtensionInit(void);

bool nexus_validate_all_physical(void);

#endif
