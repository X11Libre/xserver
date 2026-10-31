#ifndef XAVERIUS_H
#define XAVERIUS_H

#include "xorg-server.h"
#include "xf86.h"
#include "xf86_OSproc.h"
#include "compiler.h"

#define XAVERIUS_EXTENSION_NAME "XAVERIUS"

#define XAVERIUS_SUBOPCODE_ENUMERATE_PHYSICAL_DEVICES       1
#define XAVERIUS_SUBOPCODE_GET_PHYSICAL_DEVICE_PROPERTY     2
#define XAVERIUS_SUBOPCODE_GET_PHYSICAL_DEVICE_QUEUE_FAMILY_PROPERTIES 3
#define XAVERIUS_SUBOPCODE_CREATE_DEVICE                    4
#define XAVERIUS_SUBOPCODE_CREATE_DEFAULT_DEVICE            5
#define XAVERIUS_SUBOPCODE_DESTROY_DEVICE                   6
#define XAVERIUS_SUBOPCODE_GET_PHYSICAL_DEVICE_MEMORY_PROPERTIES 7

typedef struct {
    XID xid;
    uint32_t type;
} XaveriusResourceRec, *XaveriusResourcePtr;

typedef struct {
    XID physical_device_xid;
    uint32_t device_type;
    uint32_t vendor_id;
    uint32_t device_id;
    uint32_t api_version;
    uint32_t driver_version;
    uint32_t compute_queue_family_count;
    char name[256];
} PhysicalDeviceInfoRec, *PhysicalDeviceInfoPtr;

typedef struct {
    XID device_xid;
    XID physical_device_xid;
    uint32_t queue_family_index;
    uint32_t queue_count;
    float *queue_priorities;
    uint32_t feature_count;
    uint32_t *feature_ids;
    uint32_t extension_count;
    char **extension_names;
} CreateDeviceRequestRec, *CreateDeviceRequestPtr;

typedef struct {
    XID device_xid;
    uint32_t queue_count;
    float *priorities;
} CreateDefaultDeviceRequestRec, *CreateDefaultDeviceRequestPtr;

typedef struct {
    XID device_xid;
} DestroyDeviceRequestRec, *DestroyDeviceRequestPtr;

void xaverius_extension_init(void);
void xaverius_extension_fini(void);

int ProcXAveriusDispatch(ClientPtr client);
int SProcXAveriusDispatch(ClientPtr client);

extern int XAveriusExtensionMajorOpcode;

#endif