#include "xaverius.h"
#include "xaverius_compute.h"
#include "xaverius_proto.h"
#include "dix.h"
#include "resource.h"
#include "extnsionst.h"
#include "globals.h"
#include "dixstruct.h"
#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "os/fmt.h"

/* Resource types for compute objects created by subopcodes 16-24 */

static RESTYPE XaveriusDescriptorSetLayoutType = 0;
static RESTYPE XaveriusDescriptorPoolType = 0;
static RESTYPE XaveriusPipelineLayoutType = 0;
static RESTYPE XaveriusComputePipelineType = 0;
static RESTYPE XaveriusCommandPoolType = 0;

#define XAVERIUS_MAX_DESCRIPTOR_SET_LAYOUTS 64
#define XAVERIUS_MAX_DESCRIPTOR_POOLS 64
#define XAVERIUS_MAX_PIPELINE_LAYOUTS 64
#define XAVERIUS_MAX_COMPUTE_PIPELINES 64
#define XAVERIUS_MAX_COMMAND_POOLS 64

#define ValidateXID(x) ((x) != 0)

typedef struct {
    XID xid;
    XID device_xid;
    uint32_t binding_count;
} XaveriusDescriptorSetLayoutRec, *XaveriusDescriptorSetLayoutPtr;

typedef struct {
    XID xid;
    XID device_xid;
    uint32_t max_sets;
    uint32_t pool_size_count;
} XaveriusDescriptorPoolRec, *XaveriusDescriptorPoolPtr;

typedef struct {
    XID xid;
    XID device_xid;
    uint16_t set_layout_count;
    uint16_t push_constant_range_count;
} XaveriusPipelineLayoutRec, *XaveriusPipelineLayoutPtr;

typedef struct {
    XID xid;
    XID device_xid;
    XID pipeline_layout_xid;
    XID shader_module_xid;
} XaveriusComputePipelineRec, *XaveriusComputePipelinePtr;

typedef struct {
    XID xid;
    XID device_xid;
    uint32_t queue_family_index;
    uint32_t flags;
} XaveriusCommandPoolRec, *XaveriusCommandPoolPtr;

static XaveriusDescriptorSetLayoutRec xaverius_descriptor_set_layouts[XAVERIUS_MAX_DESCRIPTOR_SET_LAYOUTS];
static int xaverius_descriptor_set_layout_count = 0;

static XaveriusDescriptorPoolRec xaverius_descriptor_pools[XAVERIUS_MAX_DESCRIPTOR_POOLS];
static int xaverius_descriptor_pool_count = 0;

static XaveriusPipelineLayoutRec xaverius_pipeline_layouts[XAVERIUS_MAX_PIPELINE_LAYOUTS];
static int xaverius_pipeline_layout_count = 0;

static XaveriusComputePipelineRec xaverius_compute_pipelines[XAVERIUS_MAX_COMPUTE_PIPELINES];
static int xaverius_compute_pipeline_count = 0;

static XaveriusCommandPoolRec xaverius_command_pools[XAVERIUS_MAX_COMMAND_POOLS];
static int xaverius_command_pool_count = 0;

static int
xaverius_register_compute_resource(XID xid, RESTYPE type, void *ptr)
{
    return AddResource(xid, type, ptr) ? Success : BadAlloc;
}

/* ---- Subopcode 16: CreateDescriptorSetLayout ---- */

/* Wire layout:
 * Offset 8: XID device_xid
 * Offset 12: XID descriptor_set_layout_xid
 * Offset 16: CARD32 flags (MUST be zero)
 * Offset 20: CARD32 binding_count
 * Offset 24: binding_count * 16 bytes of BindingRecord
 * Total: 24 + 16 * binding_count
 */

int
ProcXAveriusCreateDescriptorSetLayout(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)(payload + 0);
    XID layout_xid = *(XID *)(payload + 4);
    CARD32 flags = *(CARD32 *)(payload + 8);
    CARD32 binding_count = *(CARD32 *)(payload + 12);
    int expected_len;
    int i;

    (void)stuff;

    expected_len = 24 + 16 * binding_count;
    if (client->req_len * 4 != expected_len)
        return BadLength;

    if (flags != 0)
        return BadValue;

    if (binding_count == 0) {
        /* empty layout is permitted */
    } else {
        for (i = 0; i < binding_count; i++) {
            xAveriusBindingRecord *br = (xAveriusBindingRecord *)(payload + 24 + i * 16);
            if (br->descriptor_count == 0)
                return BadValue;
            if (br->descriptor_type != 1)  /* only STORAGE_BUFFER in v0.1 */
                return BadValue;
            if (br->stage_flags != 1)      /* only COMPUTE in v0.1 */
                return BadValue;
        }
        /* check unique binding numbers */
        for (i = 0; i < binding_count; i++) {
            xAveriusBindingRecord *br_i = (xAveriusBindingRecord *)(payload + 24 + i * 16);
            int j;
            for (j = i + 1; j < binding_count; j++) {
                xAveriusBindingRecord *br_j = (xAveriusBindingRecord *)(payload + 24 + j * 16);
                if (br_i->binding == br_j->binding)
                    return BadValue;
            }
        }
    }

    if (xaverius_descriptor_set_layout_count >= XAVERIUS_MAX_DESCRIPTOR_SET_LAYOUTS)
        return BadAlloc;

    if (!ValidateXID(layout_xid) ||
        dixLookupResourceByType(NULL, layout_xid, XaveriusDescriptorSetLayoutType, client, DixWriteAccess) == Success)
        return BadIDChoice;

    if (!ValidateXID(device_xid))
        return BadValue;

    xaverius_descriptor_set_layouts[xaverius_descriptor_set_layout_count].xid = layout_xid;
    xaverius_descriptor_set_layouts[xaverius_descriptor_set_layout_count].device_xid = device_xid;
    xaverius_descriptor_set_layouts[xaverius_descriptor_set_layout_count].binding_count = binding_count;

    if (!xaverius_register_compute_resource(layout_xid, XaveriusDescriptorSetLayoutType,
                                            &xaverius_descriptor_set_layouts[xaverius_descriptor_set_layout_count]))
        return BadAlloc;

    xaverius_descriptor_set_layout_count++;
    return Success;
}

/* ---- Subopcode 17: CreateDescriptorPool ---- */

/* Wire layout:
 * Offset 8: XID device_xid
 * Offset 12: XID descriptor_pool_xid
 * Offset 16: CARD32 max_sets
 * Offset 20: CARD16 pool_size_count
 * Offset 22: CARD16 reserved (MUST be 0)
 * Offset 24: pool_size_count * 8 bytes of PoolSizeEntry
 * Total: 24 + 8 * pool_size_count
 */

int
ProcXAveriusCreateDescriptorPool(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)(payload + 0);
    XID pool_xid = *(XID *)(payload + 4);
    CARD32 max_sets = *(CARD32 *)(payload + 8);
    CARD16 pool_size_count = *(CARD16 *)(payload + 12);
    CARD16 reserved = *(CARD16 *)(payload + 14);
    int expected_len;
    int i;

    (void)stuff;

    expected_len = 24 + 8 * pool_size_count;
    if (client->req_len * 4 != expected_len)
        return BadLength;

    if (max_sets == 0 || pool_size_count == 0 || reserved != 0)
        return BadValue;

    for (i = 0; i < pool_size_count; i++) {
        xAveriusPoolSizeEntry *entry = (xAveriusPoolSizeEntry *)(payload + 24 + i * 8);
        if (entry->descriptor_type != 1)  /* only STORAGE_BUFFER in v0.1 */
            return BadValue;
        if (entry->descriptor_count == 0)
            return BadValue;
    }

    if (xaverius_descriptor_pool_count >= XAVERIUS_MAX_DESCRIPTOR_POOLS)
        return BadAlloc;

    if (!ValidateXID(pool_xid) ||
        dixLookupResourceByType(NULL, pool_xid, XaveriusDescriptorPoolType, client, DixWriteAccess) == Success)
        return BadIDChoice;

    if (!ValidateXID(device_xid))
        return BadValue;

    xaverius_descriptor_pools[xaverius_descriptor_pool_count].xid = pool_xid;
    xaverius_descriptor_pools[xaverius_descriptor_pool_count].device_xid = device_xid;
    xaverius_descriptor_pools[xaverius_descriptor_pool_count].max_sets = max_sets;
    xaverius_descriptor_pools[xaverius_descriptor_pool_count].pool_size_count = pool_size_count;

    if (!xaverius_register_compute_resource(pool_xid, XaveriusDescriptorPoolType,
                                            &xaverius_descriptor_pools[xaverius_descriptor_pool_count]))
        return BadAlloc;

    xaverius_descriptor_pool_count++;
    return Success;
}

/* ---- Subopcode 18: DestroyDescriptorPool ---- */

/* Wire layout:
 * Offset 8: XID device_xid
 * Offset 12: XID descriptor_pool_xid
 * Total: 16 bytes
 */

int
ProcXAveriusDestroyDescriptorPool(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)(payload + 0);
    XID pool_xid = *(XID *)(payload + 4);
    XaveriusDescriptorPoolPtr pool = NULL;
    int i;

    (void)stuff;

    if (client->req_len * 4 != 16)
        return BadLength;

    if (!ValidateXID(device_xid) || !ValidateXID(pool_xid))
        return BadValue;

    for (i = 0; i < xaverius_descriptor_pool_count; i++) {
        if (xaverius_descriptor_pools[i].xid == pool_xid) {
            pool = &xaverius_descriptor_pools[i];
            break;
        }
    }

    if (!pool)
        return BadValue;

    if (pool->device_xid != device_xid)
        return BadMatch;

    /* Remove from array */
    for (i = 0; i < xaverius_descriptor_pool_count; i++) {
        if (xaverius_descriptor_pools[i].xid == pool_xid) {
            xaverius_descriptor_pools[i] = xaverius_descriptor_pools[xaverius_descriptor_pool_count - 1];
            xaverius_descriptor_pool_count--;
            break;
        }
    }

    FreeResource(pool_xid, XaveriusDescriptorPoolType);
    return Success;
}

/* ---- Subopcode 19: AllocateDescriptorSets ---- */

/* Wire layout:
 * Offset 8: XID device_xid
 * Offset 12: XID descriptor_pool_xid
 * Offset 16: CARD16 descriptor_set_count
 * Offset 18: CARD16 reserved (MUST be 0)
 * Offset 20: descriptor_set_count * 8 bytes of DescriptorSetAllocEntry
 * Total: 20 + 8 * descriptor_set_count
 */

int
ProcXAveriusAllocateDescriptorSets(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)(payload + 0);
    XID pool_xid = *(XID *)(payload + 4);
    CARD16 descriptor_set_count = *(CARD16 *)(payload + 12);
    CARD16 reserved = *(CARD16 *)(payload + 14);
    int expected_len;
    int i;

    (void)stuff;

    expected_len = 20 + 8 * descriptor_set_count;
    if (client->req_len * 4 != expected_len)
        return BadLength;

    if (descriptor_set_count == 0 || reserved != 0)
        return BadValue;

    if (!ValidateXID(device_xid) || !ValidateXID(pool_xid))
        return BadValue;

    for (i = 0; i < descriptor_set_count; i++) {
        xAveriusDescriptorSetAllocEntry *entry = (xAveriusDescriptorSetAllocEntry *)(payload + 20 + i * 8);
        if (!ValidateXID(entry->descriptor_set_xid) ||
            !ValidateXID(entry->descriptor_set_layout_xid))
            return BadIDChoice;
    }

    /* In a full implementation we'd check pool capacity, layout compatibility, etc.
       For now, just validate and return Success. */
    return Success;
}

/* ---- Subopcode 20: UpdateDescriptorSets ---- */

/* Wire layout:
 * Offset 8: XID device_xid
 * Offset 12: CARD16 write_count
 * Offset 14: CARD16 reserved (MUST be 0)
 * Offset 16: CARD32 reserved (MUST be 0)
 * Offset 20: write_count * 36 bytes of DescriptorWrite
 * Total: 20 + 36 * write_count
 */

int
ProcXAveriusUpdateDescriptorSets(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)(payload + 0);
    CARD16 write_count = *(CARD16 *)(payload + 4);
    CARD16 reserved1 = *(CARD16 *)(payload + 6);
    CARD32 reserved2 = *(CARD32 *)(payload + 8);
    int expected_len;
    int i;

    (void)stuff;

    expected_len = 20 + 36 * write_count;
    if (client->req_len * 4 != expected_len)
        return BadLength;

    if (write_count == 0 || reserved1 != 0 || reserved2 != 0)
        return BadValue;

    if (!ValidateXID(device_xid))
        return BadValue;

    for (i = 0; i < write_count; i++) {
        xAveriusDescriptorWrite *w = (xAveriusDescriptorWrite *)(payload + 20 + i * 36);
        if (!ValidateXID(w->descriptor_set_xid) ||
            !ValidateXID(w->buffer_xid))
            return BadValue;
        if (w->descriptor_type != 1)  /* only STORAGE_BUFFER in v0.1 */
            return BadValue;
    }

    return Success;
}

/* ---- Subopcode 21: FreeDescriptorSets ---- */

/* Wire layout:
 * Offset 8: XID device_xid
 * Offset 12: XID descriptor_pool_xid
 * Offset 16: CARD16 descriptor_set_count
 * Offset 18: CARD16 reserved (MUST be 0)
 * Offset 20: descriptor_set_count * 4 bytes of Set XIDs
 * Total: 20 + 4 * descriptor_set_count
 */

int
ProcXAveriusFreeDescriptorSets(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)(payload + 0);
    XID pool_xid = *(XID *)(payload + 4);
    CARD16 descriptor_set_count = *(CARD16 *)(payload + 12);
    CARD16 reserved = *(CARD16 *)(payload + 14);
    int expected_len;
    int i;

    (void)stuff;

    expected_len = 20 + 4 * descriptor_set_count;
    if (client->req_len * 4 != expected_len)
        return BadLength;

    if (descriptor_set_count == 0 || reserved != 0)
        return BadValue;

    if (!ValidateXID(device_xid) || !ValidateXID(pool_xid))
        return BadValue;

    for (i = 0; i < descriptor_set_count; i++) {
        XID set_xid = *(XID *)(payload + 20 + i * 4);
        if (!ValidateXID(set_xid))
            return BadValue;
    }

    return Success;
}

/* ---- Subopcode 22: CreatePipelineLayout ---- */

/* Wire layout:
 * Offset 8: XID device_xid
 * Offset 12: XID pipeline_layout_xid
 * Offset 16: CARD16 set_layout_count
 * Offset 18: CARD16 push_constant_range_count
 * Offset 20: set_layout_count * 4 bytes of Set-Layout-XIDs
 * then: push_constant_range_count * 12 bytes of PushConstantRange
 * Total: 20 + 4 * set_layout_count + 12 * push_constant_range_count
 */

int
ProcXAveriusCreatePipelineLayout(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)(payload + 0);
    XID layout_xid = *(XID *)(payload + 4);
    CARD16 set_layout_count = *(CARD16 *)(payload + 8);
    CARD16 push_constant_range_count = *(CARD16 *)(payload + 10);
    int expected_len;
    int i;

    (void)stuff;

    expected_len = 20 + 4 * set_layout_count + 12 * push_constant_range_count;
    if (client->req_len * 4 != expected_len)
        return BadLength;

    for (i = 0; i < push_constant_range_count; i++) {
        xAveriusPushConstantRange *pcr = (xAveriusPushConstantRange *)(payload + 20 + 4 * set_layout_count + i * 12);
        if (pcr->stage_flags != 1)  /* only COMPUTE in v0.1 */
            return BadValue;
        if (pcr->size == 0)
            return BadValue;
        if (pcr->offset % 4 != 0 || pcr->size % 4 != 0)
            return BadValue;
    }

    if (xaverius_pipeline_layout_count >= XAVERIUS_MAX_PIPELINE_LAYOUTS)
        return BadAlloc;

    if (!ValidateXID(layout_xid) ||
        dixLookupResourceByType(NULL, layout_xid, XaveriusPipelineLayoutType, client, DixWriteAccess) == Success)
        return BadIDChoice;

    if (!ValidateXID(device_xid))
        return BadValue;

    xaverius_pipeline_layouts[xaverius_pipeline_layout_count].xid = layout_xid;
    xaverius_pipeline_layouts[xaverius_pipeline_layout_count].device_xid = device_xid;
    xaverius_pipeline_layouts[xaverius_pipeline_layout_count].set_layout_count = set_layout_count;
    xaverius_pipeline_layouts[xaverius_pipeline_layout_count].push_constant_range_count = push_constant_range_count;

    if (!xaverius_register_compute_resource(layout_xid, XaveriusPipelineLayoutType,
                                            &xaverius_pipeline_layouts[xaverius_pipeline_layout_count]))
        return BadAlloc;

    xaverius_pipeline_layout_count++;
    return Success;
}

/* ---- Subopcode 23: CreateComputePipeline ---- */

/* Wire layout:
 * Offset 8: XID device_xid
 * Offset 12: XID pipeline_xid
 * Offset 16: XID pipeline_layout_xid
 * Offset 20: XID shader_module_xid
 * Offset 24: CARD16 entry_point_length
 * Offset 26: CARD16 reserved (MUST be 0)
 * Offset 28: entry_point_length ASCII bytes, padded to 4
 * Total: 28 + pad4(entry_point_length)
 */

int
ProcXAveriusCreateComputePipeline(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)(payload + 0);
    XID pipeline_xid = *(XID *)(payload + 4);
    XID pipeline_layout_xid = *(XID *)(payload + 8);
    XID shader_module_xid = *(XID *)(payload + 12);
    CARD16 entry_point_length = *(CARD16 *)(payload + 16);
    CARD16 reserved = *(CARD16 *)(payload + 18);
    int padded_len;
    int expected_len;
    int i;

    (void)stuff;

    padded_len = (entry_point_length + 3) & ~3;
    expected_len = 28 + padded_len;
    if (client->req_len * 4 != expected_len)
        return BadLength;

    if (entry_point_length == 0 || reserved != 0)
        return BadValue;

    /* Validate entry point is ASCII, no NUL */
    for (i = 0; i < entry_point_length; i++) {
        CARD8 c = payload[28 + i];
        if (c == 0 || c > 127)
            return BadValue;
    }
    /* Padding bytes must be zero */
    for (i = entry_point_length; i < padded_len; i++) {
        if (payload[28 + i] != 0)
            return BadValue;
    }

    if (xaverius_compute_pipeline_count >= XAVERIUS_MAX_COMPUTE_PIPELINES)
        return BadAlloc;

    if (!ValidateXID(pipeline_xid) ||
        dixLookupResourceByType(NULL, pipeline_xid, XaveriusComputePipelineType, client, DixWriteAccess) == Success)
        return BadIDChoice;

    if (!ValidateXID(device_xid) || !ValidateXID(pipeline_layout_xid) || !ValidateXID(shader_module_xid))
        return BadValue;

    xaverius_compute_pipelines[xaverius_compute_pipeline_count].xid = pipeline_xid;
    xaverius_compute_pipelines[xaverius_compute_pipeline_count].device_xid = device_xid;
    xaverius_compute_pipelines[xaverius_compute_pipeline_count].pipeline_layout_xid = pipeline_layout_xid;
    xaverius_compute_pipelines[xaverius_compute_pipeline_count].shader_module_xid = shader_module_xid;

    if (!xaverius_register_compute_resource(pipeline_xid, XaveriusComputePipelineType,
                                            &xaverius_compute_pipelines[xaverius_compute_pipeline_count]))
        return BadAlloc;

    xaverius_compute_pipeline_count++;
    return Success;
}

/* ---- Subopcode 24: CreateCommandPool ---- */

/* Wire layout:
 * Offset 8: XID device_xid
 * Offset 12: XID command_pool_xid
 * Offset 16: CARD32 queue_family_index
 * Offset 20: CARD32 flags
 * Total: 24 bytes (fixed)
 */

int
ProcXAveriusCreateCommandPool(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)(payload + 0);
    XID pool_xid = *(XID *)(payload + 4);
    CARD32 queue_family_index = *(CARD32 *)(payload + 8);
    CARD32 flags = *(CARD32 *)(payload + 12);

    (void)stuff;

    if (client->req_len * 4 != 24)
        return BadLength;

    /* Reserved bits 2-31 must be zero */
    if (flags & ~0x3)
        return BadValue;

    if (xaverius_command_pool_count >= XAVERIUS_MAX_COMMAND_POOLS)
        return BadAlloc;

    if (!ValidateXID(pool_xid) ||
        dixLookupResourceByType(NULL, pool_xid, XaveriusCommandPoolType, client, DixWriteAccess) == Success)
        return BadIDChoice;

    if (!ValidateXID(device_xid))
        return BadValue;

    xaverius_command_pools[xaverius_command_pool_count].xid = pool_xid;
    xaverius_command_pools[xaverius_command_pool_count].device_xid = device_xid;
    xaverius_command_pools[xaverius_command_pool_count].queue_family_index = queue_family_index;
    xaverius_command_pools[xaverius_command_pool_count].flags = flags;

    if (!xaverius_register_compute_resource(pool_xid, XaveriusCommandPoolType,
                                            &xaverius_command_pools[xaverius_command_pool_count]))
        return BadAlloc;

    xaverius_command_pool_count++;
    return Success;
}

void
xaverius_compute_init(void)
{
    XaveriusDescriptorSetLayoutType = CreateNewResourceType(NULL, "XAveriusDescriptorSetLayout");
    XaveriusDescriptorPoolType = CreateNewResourceType(NULL, "XAveriusDescriptorPool");
    XaveriusPipelineLayoutType = CreateNewResourceType(NULL, "XAveriusPipelineLayout");
    XaveriusComputePipelineType = CreateNewResourceType(NULL, "XAveriusComputePipeline");
    XaveriusCommandPoolType = CreateNewResourceType(NULL, "XAveriusCommandPool");
}

void
xaverius_compute_fini(void)
{
    if (XaveriusDescriptorSetLayoutType)
        FreeResourceByType(0, XaveriusDescriptorSetLayoutType, TRUE);
    if (XaveriusDescriptorPoolType)
        FreeResourceByType(0, XaveriusDescriptorPoolType, TRUE);
    if (XaveriusPipelineLayoutType)
        FreeResourceByType(0, XaveriusPipelineLayoutType, TRUE);
    if (XaveriusComputePipelineType)
        FreeResourceByType(0, XaveriusComputePipelineType, TRUE);
    if (XaveriusCommandPoolType)
        FreeResourceByType(0, XaveriusCommandPoolType, TRUE);

    xaverius_descriptor_set_layout_count = 0;
    xaverius_descriptor_pool_count = 0;
    xaverius_pipeline_layout_count = 0;
    xaverius_compute_pipeline_count = 0;
    xaverius_command_pool_count = 0;
}