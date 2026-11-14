/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * Xaverius PhysicalDevice Registry unit tests (integrated into test/tests).
 */
#undef NDEBUG

#include <dix-config.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "dix/dix_priv.h"
#include "dix/resource_priv.h"
#include "include/dixstruct.h"
#include "include/resource.h"
#include "os/client_priv.h"
#include "tests-common.h"
#include "xaverius_priv.h"

static int test_retain_calls;
static int test_release_calls;
static int test_quiesce_calls;

static int snapshot_calls;
static XaveriusBackendResult snapshot_result_mode;

static XaveriusBackendResult
test_query_snapshot_ok(struct _XaveriusDeviceBackendBinding *binding,
                       XaveriusPhysicalDeviceSnapshot *out)
{
    (void)binding;
    snapshot_calls++;
    if (snapshot_result_mode != XAVERIUS_BACKEND_SUCCESS)
        return snapshot_result_mode;
    out->size = 16;
    out->data = calloc(1, out->size);
    assert(out->data != NULL);
    return XAVERIUS_BACKEND_SUCCESS;
}

static XaveriusBackendResult
test_query_snapshot_bad(struct _XaveriusDeviceBackendBinding *binding,
                        XaveriusPhysicalDeviceSnapshot *out)
{
    (void)binding;
    snapshot_calls++;
    out->data = NULL;
    out->size = 0;
    return XAVERIUS_BACKEND_SUCCESS;
}

static void
counting_retain(XaveriusModuleRole *role)
{
    (void)role;
    test_retain_calls++;
}

static void
counting_release(XaveriusModuleRole *role)
{
    (void)role;
    test_release_calls++;
}

static XaveriusModuleQuiescenceResult
counting_quiesce(XaveriusModuleRole *role)
{
    (void)role;
    test_quiesce_calls++;
    return XAVERIUS_MODULE_QUIESCE_SUCCESS;
}

static void
reset_hook_counts(void)
{
    test_retain_calls = 0;
    test_release_calls = 0;
    test_quiesce_calls = 0;
}

static void
make_valid_desc(XaveriusModuleRoleDescriptor *desc, const char *owner_key)
{
    memset(desc, 0, sizeof(*desc));
    desc->struct_size = sizeof(*desc);
    desc->abi_version = XAVERIUS_MODULE_ROLE_DESCRIPTOR_ABI_VERSION;
    desc->owner_key = owner_key;
    desc->retain_module_role = counting_retain;
    desc->release_module_role = counting_release;
    desc->quiesce_module_role = counting_quiesce;
}

static void
xaverius_module_role_descriptor_validation(void)
{
    XaveriusModuleRoleDescriptor valid_desc;
    make_valid_desc(&valid_desc, "test-role");

    assert(xaverius_module_role_descriptor_validate(&valid_desc) == XAVERIUS_REGISTRY_SUCCESS);
    assert(xaverius_module_role_descriptor_validate(NULL) == XAVERIUS_REGISTRY_INVALID_ARGUMENT);

    {
        XaveriusModuleRoleDescriptor d = valid_desc;
        d.struct_size = sizeof(XaveriusModuleRoleDescriptor) - 1;
        assert(xaverius_module_role_descriptor_validate(&d) == XAVERIUS_REGISTRY_INVALID_ARGUMENT);
    }
    {
        XaveriusModuleRoleDescriptor d = valid_desc;
        d.struct_size = sizeof(XaveriusModuleRoleDescriptor) + 1;
        assert(xaverius_module_role_descriptor_validate(&d) == XAVERIUS_REGISTRY_INVALID_ARGUMENT);
    }
    {
        XaveriusModuleRoleDescriptor d = valid_desc;
        d.abi_version = XAVERIUS_MODULE_ROLE_DESCRIPTOR_ABI_VERSION + 1;
        assert(xaverius_module_role_descriptor_validate(&d) == XAVERIUS_REGISTRY_UNSUPPORTED);
    }
}

static void
xaverius_identity_comparison(void)
{
    {
        XaveriusDeviceIdentity a = { 0 };
        XaveriusDeviceIdentity b = { 0 };
        a.domain = 0; a.bus = 1; a.device = 0; a.function = 0;
        b = a;
        assert(xaverius_identity_equal(&a, &b));
        b.function = 1;
        assert(!xaverius_identity_equal(&a, &b));
    }

    {
        XaveriusDeviceIdentity a = { 0 };
        XaveriusDeviceIdentity b = { 0 };
        a.drm_major = 226; a.drm_minor = 0;
        b = a;
        assert(xaverius_identity_equal(&a, &b));
        b.drm_minor = 1;
        assert(!xaverius_identity_equal(&a, &b));
    }

    {
        XaveriusDeviceIdentity a = { 0 };
        XaveriusDeviceIdentity b = { 0 };
        a.drm_major = 226; a.drm_minor = 0;
        b = a;
        b.has_device_uuid = true;
        memset(b.device_uuid, 0xAB, sizeof(b.device_uuid));
        assert(!xaverius_identity_equal(&a, &b));
        assert(!xaverius_identity_equal(&b, &a));
    }

    {
        XaveriusDeviceIdentity a = { 0 };
        XaveriusDeviceIdentity b = { 0 };
        a.has_device_uuid = true;
        b.has_device_uuid = true;
        memset(a.device_uuid, 0x11, sizeof(a.device_uuid));
        memset(b.device_uuid, 0x11, sizeof(b.device_uuid));
        assert(xaverius_identity_equal(&a, &b));
        b.device_uuid[0] = 0x22;
        assert(!xaverius_identity_equal(&a, &b));
    }

    {
        XaveriusDeviceIdentity pci = { 0 };
        XaveriusDeviceIdentity drm = { 0 };
        pci.bus = 1;
        drm.drm_major = 226;
        assert(!xaverius_identity_equal(&pci, &drm));
    }

    {
        XaveriusDeviceIdentity a = { 0 };
        XaveriusDeviceIdentity b = { 0 };
        a.has_device_uuid = true;
        b.has_device_uuid = true;
        memset(a.device_uuid, 0x33, sizeof(a.device_uuid));
        memset(b.device_uuid, 0x33, sizeof(b.device_uuid));
        assert(xaverius_identity_equal(&a, &b));
        b.has_device_uuid = false;
        memset(b.device_uuid, 0, sizeof(b.device_uuid));
        assert(!xaverius_identity_equal(&a, &b));
    }
}

static void
xaverius_ordering_key(void)
{
    XaveriusDeviceIdentity pci = { 0 };
    pci.bus = 1;
    memset(pci.driver_uuid, 0xAA, sizeof(pci.driver_uuid));

    char buf[128];
    assert(xaverius_create_ordering_key(buf, sizeof(buf), &pci, "test-role") == XAVERIUS_REGISTRY_SUCCESS);
    assert(strncmp(buf, "pci:0000:01:00.0::", 18) == 0);
    assert(strstr(buf, "test-role") != NULL);

    {
        XaveriusDeviceIdentity drm = { 0 };
        drm.drm_major = 226;
        drm.drm_minor = 0;
        memset(drm.driver_uuid, 0xBB, sizeof(drm.driver_uuid));
        char dbuf[128];
        assert(xaverius_create_ordering_key(dbuf, sizeof(dbuf), &drm, "test-role") == XAVERIUS_REGISTRY_SUCCESS);
        assert(strncmp(dbuf, "drm:226.0:", 10) == 0);
        assert(strcmp(buf, dbuf) != 0);
    }

    {
        char tiny[8];
        assert(xaverius_create_ordering_key(tiny, sizeof(tiny), &pci, "long-owner-key") == XAVERIUS_REGISTRY_INVALID_ARGUMENT);
    }

    {
        char a[128], b[128];
        assert(xaverius_create_ordering_key(a, sizeof(a), &pci, "aaa") == XAVERIUS_REGISTRY_SUCCESS);
        assert(xaverius_create_ordering_key(b, sizeof(b), &pci, "bbb") == XAVERIUS_REGISTRY_SUCCESS);
        assert(xaverius_compare_ordering_key(a, b) < 0);
        assert(xaverius_compare_ordering_key(b, a) > 0);
        assert(xaverius_compare_ordering_key(a, a) == 0);
    }
}

static void
xaverius_module_role_ownership(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role = NULL;

    make_valid_desc(&desc, "owner-a");
    reset_hook_counts();
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);
    assert(role != NULL);
    assert(test_retain_calls == 0);
    assert(test_release_calls == 0);

    XaveriusModuleRoleRetain(role);
    assert(test_retain_calls == 1);
    XaveriusModuleRoleRelease(role);
    assert(test_release_calls == 1);

    XaveriusModuleRoleFinalize(role);
    assert(test_release_calls == 2);

    make_valid_desc(&desc, "owner-b");
    desc.abi_version = XAVERIUS_MODULE_ROLE_DESCRIPTOR_ABI_VERSION + 1;
    reset_hook_counts();
    role = NULL;
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_UNSUPPORTED);
    assert(role == NULL);
    assert(test_retain_calls == 0);
    assert(test_release_calls == 0);

    make_valid_desc(&desc, "owner-c");
    desc.retain_module_role = NULL;
    reset_hook_counts();
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_INVALID_ARGUMENT);
    assert(test_retain_calls == 0);
    assert(test_release_calls == 0);
}

static void
setup_min_ops(struct XaveriusDeviceBackendOps *ops, bool snapshot_ok)
{
    memset(ops, 0, sizeof(*ops));
    ops->struct_size = sizeof(*ops);
    ops->abi_version = XAVERIUS_DEVICE_BACKEND_OPS_ABI_VERSION;
    ops->query_physical_snapshot = snapshot_ok ? test_query_snapshot_ok : test_query_snapshot_bad;
}

static void
setup_min_reg(XaveriusDeviceBackendRegistration *reg,
              const XaveriusDeviceIdentity *id,
              struct XaveriusDeviceBackendOps *ops,
              XaveriusModuleRole *role)
{
    memset(reg, 0, sizeof(*reg));
    reg->struct_size = sizeof(*reg);
    reg->abi_version = XAVERIUS_DEVICE_BACKEND_REGISTRATION_ABI_VERSION;
    reg->identity = id;
    reg->ops = ops;
    reg->owner_role = role;
    reg->owner_role_generation = role->role_generation;
}

static void
xaverius_registry_registration_paths(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role = NULL;
    XaveriusDeviceIdentity id = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;

    assert(XaveriusRegistryInit());

    make_valid_desc(&desc, "reg-owner");
    reset_hook_counts();
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 1;
    memset(id.driver_uuid, 0x44, sizeof(id.driver_uuid));

    setup_min_ops(&ops, true);
    ops.query_physical_snapshot = NULL;
    setup_min_reg(&reg, &id, &ops, role);
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_INVALID_ARGUMENT);
    assert(test_retain_calls == 0 && test_release_calls == 0);

    setup_min_ops(&ops, true);
    setup_min_reg(&reg, &id, &ops, role);
    reg.owner_role_generation = role->role_generation + 9999;
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_NOT_REGISTERED);
    assert(test_retain_calls == 0 && test_release_calls == 0);

    setup_min_reg(&reg, &id, &ops, role);
    reg.abi_version = XAVERIUS_DEVICE_BACKEND_REGISTRATION_ABI_VERSION + 1;
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_INVALID_ARGUMENT);
    assert(test_retain_calls == 0 && test_release_calls == 0);

    reg.abi_version = XAVERIUS_DEVICE_BACKEND_REGISTRATION_ABI_VERSION;
    setup_min_ops(&ops, true);
    setup_min_reg(&reg, &id, &ops, role);
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);
    assert(dev->xid == 0);

    XaveriusPhysicalDeviceRelease(dev);

    XaveriusRegistryFini();
}

static void
xaverius_registry_tombstone_lifecycle(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role = NULL;
    XaveriusDeviceIdentity id = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;
    XaveriusPhysicalDevice *obs_dev = NULL;
    uint64_t obs_gen = 0;
    uint64_t obs_id = 0;
    const char source[] = "src-tomb";

    assert(XaveriusRegistryInit());
    make_valid_desc(&desc, "tomb-owner");
    reset_hook_counts();
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 7;
    id.function = 1;
    memset(id.driver_uuid, 0x55, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role);

    reset_hook_counts();
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);
    assert(test_retain_calls == 1 && test_release_calls == 0);
    assert(xaverius_physical_device_count == 1);
    assert(xaverius_tombstone_count == 0);

    obs_id = 1001;
    assert(XaveriusRegistryObserveDevice(NULL, source, 1, 1, &id,
                                         &obs_dev, &obs_gen, &obs_id) == XAVERIUS_REGISTRY_SUCCESS);
    assert(obs_dev == dev);

    assert(XaveriusRegistryConfirmDeviceRemoved(NULL, dev, dev_gen,
         &(XaveriusDeviceRemovalEvidence){
             .source_key = source,
             .source_generation = 1,
             .observation_id = obs_id,
             .source_event_sequence = 1,
             .physical_device_generation = dev_gen
         }) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev->status == XAVERIUS_REGISTRY_REMOVED);
    assert(xaverius_physical_device_count == 0);
    assert(xaverius_tombstone_count == 1);
    assert(dev->xid == 0);

    assert(XaveriusRegistryConfirmDeviceRemoved(NULL, dev, dev_gen,
         &(XaveriusDeviceRemovalEvidence){
             .source_key = source,
             .source_generation = 1,
             .observation_id = obs_id,
             .source_event_sequence = 1,
             .physical_device_generation = dev_gen
         }) == XAVERIUS_REGISTRY_STALE_EVENT);
    assert(xaverius_physical_device_count == 0);
    assert(xaverius_tombstone_count == 1);

    reset_hook_counts();
    XaveriusRegistryFini();
    assert(test_release_calls == 1);
}

static void
xaverius_registry_publish_dix(void)
{
    static ClientRec test_client;
    static int dix_inited;
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role = NULL;
    XaveriusDeviceIdentity id = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;
    void *res = NULL;
    int rc;
    const char source[] = "src-pub";
    uint64_t obs_id = 0;

    if (!dix_inited) {
        dixResetPrivates();
        memset(&test_client, 0, sizeof(test_client));
        test_client.index = 1;
        test_client.clientAsMask = ((XID)1 << CLIENTOFFSET);
        serverClient = &test_client;
        InitClient(serverClient, 0, (void *)NULL);
        assert(InitClientResources(serverClient));
        dix_inited = 1;
    }

    XaveriusPhysicalDeviceType = 0;
    assert(XaveriusRegistryInit());
    make_valid_desc(&desc, "pub-owner");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 3;
    id.device = 2;
    memset(id.driver_uuid, 0x66, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role);

    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);
    assert(dev->xid == 0);

    obs_id = 1;
    assert(XaveriusRegistryObserveDevice(NULL, source, 1, 1, &id,
                                         &dev, &dev_gen, &obs_id) == XAVERIUS_REGISTRY_SUCCESS);

    assert(XaveriusPhysicalDevicePublish(dev) == true);
    assert(dev->xid != 0);
    res = NULL;
    rc = dixLookupResourceByType(&res, dev->xid, XaveriusPhysicalDeviceType,
                                 serverClient, DixReadAccess);
    assert(rc == Success);
    assert(res == dev);

    {
        XID old = dev->xid;
        assert(XaveriusPhysicalDevicePublish(dev) == false);
        assert(dev->xid == old);
        res = NULL;
        rc = dixLookupResourceByType(&res, dev->xid, XaveriusPhysicalDeviceType,
                                     serverClient, DixReadAccess);
        assert(rc == Success);
        assert(res == dev);
    }

    reset_hook_counts();
    XaveriusRegistryFini();
    assert(test_release_calls >= 1);
}

static void
xaverius_registry_registration_contract(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role = NULL;
    XaveriusDeviceIdentity id = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev = NULL;
    XaveriusPhysicalDevice *dev2 = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;
    uint64_t dev_gen2 = 0, bind_gen2 = 0;

    assert(XaveriusRegistryInit());
    make_valid_desc(&desc, "contract-owner");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 11;
    id.device = 5;
    memset(id.driver_uuid, 0x99, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role);

    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);
    assert(xaverius_physical_device_count == 1);
    assert(xaverius_tombstone_count == 0);
    assert(dev->xid == 0);

    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev2, &dev_gen2, &bind_gen2) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev2 == dev);
    assert(dev_gen2 == dev_gen);
    assert(bind_gen2 == bind_gen);
    assert(xaverius_physical_device_count == 1);
    XaveriusPhysicalDeviceRelease(dev2);
    assert(xaverius_physical_device_count == 1);

    {
        XaveriusModuleRoleDescriptor desc2;
        XaveriusModuleRole *role2 = NULL;
        struct XaveriusDeviceBackendOps ops2;
        XaveriusDeviceBackendRegistration reg2;
        XaveriusPhysicalDevice *devc = NULL;
        uint64_t dgc = 0, bgc = 0;
        make_valid_desc(&desc2, "contract-owner-2");
        assert(XaveriusModuleRoleCreate(&desc2, &role2) == XAVERIUS_REGISTRY_SUCCESS);
        setup_min_ops(&ops2, true);
        setup_min_reg(&reg2, &id, &ops2, role2);
        assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg2, &devc, &dgc, &bgc) == XAVERIUS_REGISTRY_IDENTITY_CONFLICT);
        assert(devc == NULL);
        assert(xaverius_physical_device_count == 1);
        assert(xaverius_tombstone_count == 0);
        XaveriusModuleRoleFinalize(role2);
    }

    {
        XaveriusDeviceBackendRegistration bad = reg;
        bad.ops = NULL;
        XaveriusPhysicalDevice *devd = NULL;
        uint64_t dgd = 0, bgd = 0;
        uint32_t count_before = xaverius_physical_device_count;
        assert(XaveriusRegistryRegisterDdxDevice(NULL, &bad, &devd, &dgd, &bgd) == XAVERIUS_REGISTRY_INVALID_ARGUMENT);
        assert(devd == NULL);
        assert(xaverius_physical_device_count == count_before);
    }

    {
        XaveriusDeviceBackendRegistration bad = reg;
        bad.abi_version = XAVERIUS_DEVICE_BACKEND_REGISTRATION_ABI_VERSION + 1;
        XaveriusPhysicalDevice *deve = NULL;
        uint64_t dge = 0, bge = 0;
        uint32_t count_before = xaverius_physical_device_count;
        assert(XaveriusRegistryRegisterDdxDevice(NULL, &bad, &deve, &dge, &bge) == XAVERIUS_REGISTRY_INVALID_ARGUMENT);
        assert(deve == NULL);
        assert(xaverius_physical_device_count == count_before);
    }

    assert(XaveriusRegistryRemoveDeviceBackendBinding(NULL, dev, dev_gen, role->role_generation, bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev->binding == NULL);

    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev2, &dev_gen2, &bind_gen2) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev2 != NULL);
    assert(xaverius_physical_device_count == 1);
    assert(dev2->xid == 0);
    assert(bind_gen2 != bind_gen);
    XaveriusPhysicalDeviceRelease(dev2);

    XaveriusRegistryFini();
}

static void
xaverius_registry_observation_edge_cases(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role = NULL;
    XaveriusDeviceIdentity id_a = { 0 };
    XaveriusDeviceIdentity id_b = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev_a = NULL;
    XaveriusPhysicalDevice *dev_b = NULL;
    XaveriusPhysicalDevice *obs_dev = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;
    uint64_t obs_gen = 0;
    uint64_t obs_id = 0;
    const char source[] = "src-edge";

    assert(XaveriusRegistryInit());
    make_valid_desc(&desc, "edge-owner");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);

    id_a.bus = 13;
    id_a.device = 7;
    memset(id_a.driver_uuid, 0xaa, sizeof(id_a.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id_a, &ops, role);

    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev_a, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev_a != NULL);

    obs_id = 0;
    assert(XaveriusRegistryObserveDevice(NULL, source, 1, 1, &id_b,
                                         &obs_dev, &obs_gen, &obs_id) == XAVERIUS_REGISTRY_PENDING);
    assert(obs_dev == NULL);
    assert(obs_gen == 0);
    assert(obs_id != 0);

    id_b.bus = 13;
    id_b.device = 7;
    memset(id_b.driver_uuid, 0xaa, sizeof(id_b.driver_uuid));
    assert(XaveriusRegistryObserveDevice(NULL, source, 1, 1, &id_a,
                                         &obs_dev, &obs_gen, &obs_id) == XAVERIUS_REGISTRY_SUCCESS);
    assert(obs_dev == dev_a);
    assert(obs_gen == dev_gen);
    assert(obs_id != 0);

    {
        uint64_t prev_obs_id = obs_id;
        assert(XaveriusRegistryObserveDevice(NULL, source, 1, 1, &id_a,
                                             &obs_dev, &obs_gen, &obs_id) == XAVERIUS_REGISTRY_SUCCESS);
        assert(obs_dev == dev_a);
        assert(obs_gen == dev_gen);
        assert(obs_id == prev_obs_id);
        assert(dev_a->caller_refcount == 2);
    }

    id_b.bus = 19;
    id_b.device = 23;
    memset(id_b.driver_uuid, 0xcc, sizeof(id_b.driver_uuid));
    setup_min_reg(&reg, &id_b, &ops, role);
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev_b, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev_b != NULL);
    assert(dev_b != dev_a);

    assert(XaveriusRegistryObserveDevice(NULL, source, 1, 2, &id_b,
                                         &obs_dev, &obs_gen, &obs_id) == XAVERIUS_REGISTRY_SUCCESS);
    assert(obs_dev == dev_b);
    assert(obs_gen == dev_gen);
    assert(obs_id != 0);
    assert(dev_a != dev_b);

    assert(XaveriusRegistryRemoveObservation(NULL, source, 1, obs_id - 1) == XAVERIUS_REGISTRY_SUCCESS);
    assert(XaveriusRegistryRemoveObservation(NULL, source, 1, obs_id - 2) == XAVERIUS_REGISTRY_SUCCESS);
    assert(XaveriusRegistryRemoveObservation(NULL, source, 1, obs_id) == XAVERIUS_REGISTRY_SUCCESS);

    assert(XaveriusRegistryRemoveObservation(NULL, source, 1, obs_id) == XAVERIUS_REGISTRY_NOT_FOUND);

    assert(XaveriusRegistryConfirmDeviceRemoved(NULL, dev_a, dev_gen,
         &(XaveriusDeviceRemovalEvidence){
             .source_key = source,
             .source_generation = 1,
             .observation_id = obs_id - 2,
             .source_event_sequence = 1,
             .physical_device_generation = dev_gen
         }) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev_a->status == XAVERIUS_REGISTRY_REMOVED);
    assert(xaverius_physical_device_count == 0);
    assert(xaverius_tombstone_count == 1);

    assert(XaveriusRegistryConfirmDeviceRemoved(NULL, dev_a, dev_gen,
         &(XaveriusDeviceRemovalEvidence){
             .source_key = source,
             .source_generation = 1,
             .observation_id = obs_id - 2,
             .source_event_sequence = 1,
             .physical_device_generation = dev_gen
         }) == XAVERIUS_REGISTRY_STALE_EVENT);
    assert(xaverius_tombstone_count == 1);

    XaveriusPhysicalDeviceRelease(dev_a);
    XaveriusPhysicalDeviceRelease(dev_b);
    XaveriusRegistryFini();
}

static void
xaverius_module_role_admission_gate(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role = NULL;
    XaveriusDeviceIdentity id = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;

    assert(XaveriusRegistryInit());

    /* NULL role => AdmissionOpen false. */
    assert(XaveriusModuleRoleAdmissionOpen(NULL) == false);

    /* Newly created role => AdmissionOpen true. */
    make_valid_desc(&desc, "adm-owner");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);
    assert(role != NULL);
    assert(XaveriusModuleRoleAdmissionOpen(role) == true);
    assert(role->admission == XAVERIUS_MODULE_ROLE_ADMISSION_OPEN);

    /* Close admission internally (no public mutator exists per spec). */
    role->admission = XAVERIUS_MODULE_ROLE_ADMISSION_CLOSED;
    assert(XaveriusModuleRoleAdmissionOpen(role) == false);
    assert(role->admission == XAVERIUS_MODULE_ROLE_ADMISSION_CLOSED);

    /* Registration using a closed-admission role returns UNAVAILABLE,
     * performs no retain/release/snapshot callback side effects, and
     * does not create a device. */
    id.bus = 21;
    id.device = 31;
    memset(id.driver_uuid, 0xdd, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role);
    reset_hook_counts();
    snapshot_calls = 0;
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_UNAVAILABLE);
    assert(dev == NULL);
    assert(test_retain_calls == 0);
    assert(test_release_calls == 0);
    assert(snapshot_calls == 0);

    XaveriusModuleRoleFinalize(role);
    XaveriusRegistryFini();
}

const testfunc_t*
xaverius_registry_test(void)
{
    static const testfunc_t testfuncs[] = {
        xaverius_module_role_descriptor_validation,
        xaverius_identity_comparison,
        xaverius_ordering_key,
        xaverius_module_role_ownership,
        xaverius_registry_registration_paths,
        xaverius_registry_tombstone_lifecycle,
        xaverius_registry_publish_dix,
        xaverius_registry_registration_contract,
        xaverius_registry_observation_edge_cases,
        xaverius_module_role_admission_gate,
        NULL,
    };

    return testfuncs;
}