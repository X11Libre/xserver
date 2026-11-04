#include "xaverius.h"
#include "xaverius_device.h"
#include "xaverius_proto.h"
#include "dix.h"
#include "resource.h"
#include "extnsionst.h"
#include "globals.h"
#include "dix/dix_priv.h"
#include "dix/request_priv.h"

#define XAVERIUS_PROP_COUNT 5

typedef struct {
    uint32_t property_id;
    uint32_t type;
    uint32_t value_length;
} XaveriusPropertyDefRec, *XaveriusPropertyDefPtr;

static const XaveriusPropertyDefRec xaverius_property_registry[XAVERIUS_PROP_COUNT] = {
    { 0, XAVERIUS_PROP_TYPE_BYTE_ARRAY, 16 },
    { 1, XAVERIUS_PROP_TYPE_BYTE_ARRAY, 16 },
    { 2, XAVERIUS_PROP_TYPE_CARD32, 4 },
    { 3, XAVERIUS_PROP_TYPE_CARD32_ARRAY, 12 },
    { 4, XAVERIUS_PROP_TYPE_CARD32, 4 },
};

static const XaveriusPropertyDefRec *
xaverius_find_property_by_id(uint32_t property_id)
{
    int i;
    for (i = 0; i < XAVERIUS_PROP_COUNT; i++) {
        if (xaverius_property_registry[i].property_id == property_id)
            return &xaverius_property_registry[i];
    }
    return NULL;
}

static int
xaverius_send_property_reply(ClientPtr client, uint32_t status,
                             uint32_t property_type, uint32_t value_length,
                             const void *value)
{
    unsigned char reply[32];
    int total_len = 32 + ((value_length + 3) & ~3);
    uint32_t *w32 = (uint32_t *)reply;

    reply[0] = X_Reply;
    reply[1] = 0;
    w32[1] = bytes_to_int32(total_len - 32);
    w32[2] = client->sequence & 0xFFFF;
    w32[3] = status;
    w32[4] = property_type;
    w32[5] = value_length;
    memset(reply + 20, 0, 12);

    WriteToClient(client, 32, reply);

    if (status == XAVERIUS_PROP_STATUS_SUCCESS && value_length > 0 && value)
        WriteToClient(client, value_length, value);

    return Success;
}
