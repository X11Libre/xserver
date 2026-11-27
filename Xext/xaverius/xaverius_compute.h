#ifndef XAVERIUS_COMPUTE_H
#define XAVERIUS_COMPUTE_H

#include "xaverius.h"

extern void xaverius_compute_init(void);
extern void xaverius_compute_fini(void);

extern int ProcXAveriusCreateDescriptorSetLayout(ClientPtr client);
extern int ProcXAveriusCreateDescriptorPool(ClientPtr client);
extern int ProcXAveriusDestroyDescriptorPool(ClientPtr client);
extern int ProcXAveriusAllocateDescriptorSets(ClientPtr client);
extern int ProcXAveriusUpdateDescriptorSets(ClientPtr client);
extern int ProcXAveriusFreeDescriptorSets(ClientPtr client);
extern int ProcXAveriusCreatePipelineLayout(ClientPtr client);
extern int ProcXAveriusCreateComputePipeline(ClientPtr client);
extern int ProcXAveriusCreateCommandPool(ClientPtr client);

#endif