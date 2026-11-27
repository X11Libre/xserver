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
#include "xaverius_intern.h"
#include "xaverius_intern.h"

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

static XaveriusBackendResult
test_query_snapshot_fail(struct _XaveriusDeviceBackendBinding *binding,
                         XaveriusPhysicalDeviceSnapshot *out)
{
    (void)binding;
    (void)out;
    snapshot_calls++;
    return XAVERIUS_BACKEND_EXECUTION_FAILURE;
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

static XaveriusModuleQuiescenceResult
quiesce_failure(XaveriusModuleRole *role)
{
    (void)role;
    test_quiesce_calls++;
    return XAVERIUS_MODULE_QUIESCE_FAILURE;
}

static XaveriusModuleQuiescenceResult
quiesce_pending(XaveriusModuleRole *role)
{
    (void)role;
    test_quiesce_calls++;
    return XAVERIUS_MODULE_QUIESCE_PENDING;
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

    /* Successful creation acquires exactly one role-owned lease retain after
     * all fallible validation/allocation. */
    make_valid_desc(&desc, "owner-a");
    reset_hook_counts();
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);
    assert(role != NULL);
    assert(test_retain_calls == 1);  /* role-owned lease reference acquired */
    assert(test_release_calls == 0);

    XaveriusModuleRoleRetain(role);
    assert(test_retain_calls == 2);  /* binding-level retain added on top */
    XaveriusModuleRoleRelease(role);
    assert(test_release_calls == 1);

    /* Finalize releases the role-owned lease reference exactly once. */
    XaveriusModuleRoleFinalize(role);
    assert(test_release_calls == 2);  /* binding release + role-owned lease release */

    /* Failed validation: no role hook invoked, no retain. */
    make_valid_desc(&desc, "owner-b");
    desc.abi_version = XAVERIUS_MODULE_ROLE_DESCRIPTOR_ABI_VERSION + 1;
    reset_hook_counts();
    role = NULL;
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_UNSUPPORTED);
    assert(role == NULL);
    assert(test_retain_calls == 0);
    assert(test_release_calls == 0);

    /* Missing retain hook: creation fails, no retain. */
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
    assert(test_retain_calls == 1 && test_release_calls == 0); /* role-owned lease retained by create */

    setup_min_ops(&ops, true);
    setup_min_reg(&reg, &id, &ops, role);
    reg.owner_role_generation = role->role_generation + 9999;
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_NOT_REGISTERED);
    assert(test_retain_calls == 1 && test_release_calls == 0);

    setup_min_reg(&reg, &id, &ops, role);
    reg.abi_version = XAVERIUS_DEVICE_BACKEND_REGISTRATION_ABI_VERSION + 1;
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_UNSUPPORTED);
    assert(test_retain_calls == 1 && test_release_calls == 0);

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
    assert(test_release_calls == 2);
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

    {
        XID pub_xid = 0;
        assert(XaveriusPhysicalDevicePublish(dev, dev_gen, &pub_xid) == XAVERIUS_REGISTRY_SUCCESS);
        assert(dev->xid != 0);
        assert(pub_xid == dev->xid);
        res = NULL;
        rc = dixLookupResourceByType(&res, dev->xid, XaveriusPhysicalDeviceType,
                                     serverClient, DixReadAccess);
        assert(rc == Success);
        assert(((XaveriusPhysicalDeviceDIXWrapper *)res)->dev == dev);
    }

    {
        XID old = dev->xid;
        assert(XaveriusPhysicalDevicePublish(dev, dev_gen, &old) == XAVERIUS_REGISTRY_ALREADY_EXISTS);
        assert(dev->xid == old);
        res = NULL;
        rc = dixLookupResourceByType(&res, dev->xid, XaveriusPhysicalDeviceType,
                                     serverClient, DixReadAccess);
        assert(rc == Success);
        assert(((XaveriusPhysicalDeviceDIXWrapper *)res)->dev == dev);
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
        assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg2, &devc, &dgc, &bgc) == XAVERIUS_REGISTRY_AMBIGUOUS);
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
        assert(XaveriusRegistryRegisterDdxDevice(NULL, &bad, &deve, &dge, &bge) == XAVERIUS_REGISTRY_UNSUPPORTED);
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
    assert(XaveriusRegistryObserveDevice(NULL, source, 1, 2, &id_a,
                                         &obs_dev, &obs_gen, &obs_id) == XAVERIUS_REGISTRY_SUCCESS);
    assert(obs_dev == dev_a);
    assert(obs_gen == dev_gen);
    assert(obs_id != 0);

    {
        uint64_t prev_obs_id = obs_id;
        assert(XaveriusRegistryObserveDevice(NULL, source, 1, 2, &id_a,
                                             &obs_dev, &obs_gen, &obs_id) == XAVERIUS_REGISTRY_SUCCESS);
        assert(obs_dev == dev_a);
        assert(obs_gen == dev_gen);
        assert(obs_id == prev_obs_id);
        assert(dev_a->caller_refcount == 3);
    }

    /* Save the observation ID and device generation for dev_a before registering device B. */
    uint64_t dev_a_obs_id = obs_id;
    uint64_t dev_a_gen = dev_gen;

    id_b.bus = 19;
    id_b.device = 23;
    memset(id_b.driver_uuid, 0xcc, sizeof(id_b.driver_uuid));
    setup_min_reg(&reg, &id_b, &ops, role);
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev_b, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev_b != NULL);
    assert(dev_b != dev_a);

    assert(XaveriusRegistryObserveDevice(NULL, source, 1, 3, &id_b,
                                         &obs_dev, &obs_gen, &obs_id) == XAVERIUS_REGISTRY_SUCCESS);
    assert(obs_dev == dev_b);
    assert(obs_gen == dev_gen);
    assert(obs_id != 0);
    assert(dev_a != dev_b);

    /* Confirm device A removed before removing observations. */
    assert(XaveriusRegistryConfirmDeviceRemoved(NULL, dev_a, dev_a_gen,
         &(XaveriusDeviceRemovalEvidence){
              .source_key = source,
              .source_generation = 1,
              .observation_id = dev_a_obs_id,
              .source_event_sequence = 2,
              .physical_device_generation = dev_a_gen
          }) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev_a->status == XAVERIUS_REGISTRY_REMOVED);
    assert(xaverius_physical_device_count == 1); /* dev_b still active */
    assert(xaverius_tombstone_count == 1);

    /* Now remove all observations. */
    assert(XaveriusRegistryRemoveObservation(NULL, source, 1, obs_id) == XAVERIUS_REGISTRY_SUCCESS);
    assert(XaveriusRegistryRemoveObservation(NULL, source, 1, dev_a_obs_id) == XAVERIUS_REGISTRY_SUCCESS);
    assert(XaveriusRegistryRemoveObservation(NULL, source, 1, dev_a_obs_id - 1) == XAVERIUS_REGISTRY_SUCCESS);

    assert(XaveriusRegistryRemoveObservation(NULL, source, 1, dev_a_obs_id) == XAVERIUS_REGISTRY_NOT_FOUND);

    /* Second confirm should return STALE_EVENT. */
    assert(XaveriusRegistryConfirmDeviceRemoved(NULL, dev_a, dev_a_gen,
         &(XaveriusDeviceRemovalEvidence){
              .source_key = source,
              .source_generation = 1,
              .observation_id = dev_a_obs_id,
              .source_event_sequence = 2,
              .physical_device_generation = dev_a_gen
          }) == XAVERIUS_REGISTRY_STALE_EVENT);

    assert(XaveriusRegistryConfirmDeviceRemoved(NULL, dev_a, dev_a_gen,
         &(XaveriusDeviceRemovalEvidence){
              .source_key = source,
              .source_generation = 1,
              .observation_id = obs_id - 1,
              .source_event_sequence = 2,
              .physical_device_generation = dev_a_gen
          }) == XAVERIUS_REGISTRY_STALE_EVENT);
    assert(xaverius_tombstone_count == 1);

    XaveriusPhysicalDeviceRelease(dev_a);
    XaveriusPhysicalDeviceRelease(dev_b);
    XaveriusRegistryFini();
}

static void
xaverius_registry_role_validation_errors(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role = NULL;
    XaveriusDeviceIdentity id = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;

    assert(XaveriusRegistryInit());

    /* Test STALE_GENERATION: role quiesced SUCCESS (stale generation) */
    make_valid_desc(&desc, "stale-owner");
    desc.quiesce_module_role = counting_quiesce; /* returns SUCCESS */
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);
    assert(role != NULL);

    /* Manually quiesce to SUCCESS (simulates stale generation) */
    role->quiesce_state = XAVERIUS_MODULE_QUIESCE_SUCCESS;

    id.bus = 100;
    id.device = 1;
    memset(id.driver_uuid, 0x11, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role);
    reset_hook_counts();
    snapshot_calls = 0;
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_STALE_GENERATION);
    assert(dev == NULL);
    assert(test_retain_calls == 0);
    assert(test_release_calls == 0);
    assert(snapshot_calls == 0);

    XaveriusModuleRoleFinalize(role);
    role = NULL;

    /* Test QUARANTINED: role quiesced FAILURE */
    make_valid_desc(&desc, "quarantine-owner");
    desc.quiesce_module_role = quiesce_failure; /* returns FAILURE */
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);
    assert(role != NULL);

    /* Manually set quiesce state to FAILURE */
    role->quiesce_state = XAVERIUS_MODULE_QUIESCE_FAILURE;

    id.bus = 101;
    id.device = 2;
    memset(id.driver_uuid, 0x22, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role);
    reset_hook_counts();
    snapshot_calls = 0;
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_QUARANTINED);
    assert(dev == NULL);
    assert(test_retain_calls == 0);
    assert(test_release_calls == 0);
    assert(snapshot_calls == 0);

    XaveriusModuleRoleFinalize(role);
    role = NULL;

    /* Test UNAVAILABLE: role quiesced PENDING (temporarily unavailable) */
    make_valid_desc(&desc, "pending-owner");
    desc.quiesce_module_role = quiesce_pending; /* returns PENDING */
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);
    assert(role != NULL);

    /* Manually set quiesce state to PENDING */
    role->quiesce_state = XAVERIUS_MODULE_QUIESCE_PENDING;

    id.bus = 102;
    id.device = 3;
    memset(id.driver_uuid, 0x33, sizeof(id.driver_uuid));
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
    role = NULL;

    /* Test SHUTTING_DOWN: admission CLOSED */
    make_valid_desc(&desc, "shutdown-owner");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);
    assert(role != NULL);

    /* Close admission */
    role->admission = XAVERIUS_MODULE_ROLE_ADMISSION_CLOSED;

    id.bus = 103;
    id.device = 4;
    memset(id.driver_uuid, 0x44, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role);
    reset_hook_counts();
    snapshot_calls = 0;
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_SHUTTING_DOWN);
    assert(dev == NULL);
    assert(test_retain_calls == 0);
    assert(test_release_calls == 0);
    assert(snapshot_calls == 0);

    XaveriusModuleRoleFinalize(role);
    XaveriusRegistryFini();
}

static void
xaverius_registry_sequence_exhaustion(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role = NULL;
    XaveriusDeviceIdentity id = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev = NULL;
    XaveriusPhysicalDevice *obs_dev = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;
    uint64_t obs_gen = 0;
    uint64_t obs_id = 0;
    const char source[] = "src-exhaust";

    assert(XaveriusRegistryInit());
    make_valid_desc(&desc, "exhaust-owner");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 200;
    id.device = 1;
    memset(id.driver_uuid, 0xee, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role);

    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);

    /* Exhaust the sequence: observe with UINT64_MAX - 1, then UINT64_MAX,
     * then any further observe should fail with STALE_EVENT. */

    /* First observe with sequence = UINT64_MAX - 1 */
    obs_id = 0;
    assert(XaveriusRegistryObserveDevice(NULL, source, 1, UINT64_MAX - 1, &id,
                                         &obs_dev, &obs_gen, &obs_id) == XAVERIUS_REGISTRY_SUCCESS);
    assert(obs_dev == dev);

    /* Next observe with sequence = UINT64_MAX (should succeed) */
    obs_id = 0;
    assert(XaveriusRegistryObserveDevice(NULL, source, 1, UINT64_MAX, &id,
                                         &obs_dev, &obs_gen, &obs_id) == XAVERIUS_REGISTRY_SUCCESS);
    assert(obs_dev == dev);

    /* Next observe with any sequence (should fail - exhausted) */
    obs_id = 0;
    obs_dev = NULL;
    assert(XaveriusRegistryObserveDevice(NULL, source, 1, UINT64_MAX, &id,
                                         &obs_dev, &obs_gen, &obs_id) == XAVERIUS_REGISTRY_STALE_EVENT);
    assert(obs_dev == NULL);

    obs_id = 0;
    assert(XaveriusRegistryObserveDevice(NULL, source, 1, 1, &id,
                                         &obs_dev, &obs_gen, &obs_id) == XAVERIUS_REGISTRY_STALE_EVENT);
    assert(obs_dev == NULL);

    XaveriusRegistryFini();
}

static void
xaverius_registry_multi_source_removal(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role = NULL;
    XaveriusDeviceIdentity id = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev = NULL;
    XaveriusPhysicalDevice *obs_dev = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;
    uint64_t obs_gen = 0;
    uint64_t obs_id_a = 0, obs_id_b = 0;
    const char source_a[] = "src-a";
    const char source_b[] = "src-b";

    assert(XaveriusRegistryInit());
    make_valid_desc(&desc, "multi-owner");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 200;
    id.device = 1;
    memset(id.driver_uuid, 0xee, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role);

    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);
    assert(dev->active_observation_count == 0);

    /* Source A observes the device. */
    obs_id_a = 0;
    assert(XaveriusRegistryObserveDevice(NULL, source_a, 1, 1, &id,
                                         &obs_dev, &obs_gen, &obs_id_a) == XAVERIUS_REGISTRY_SUCCESS);
    assert(obs_dev == dev);
    assert(dev->active_observation_count == 1);

    /* Source B observes the same device. */
    obs_id_b = 0;
    assert(XaveriusRegistryObserveDevice(NULL, source_b, 1, 1, &id,
                                         &obs_dev, &obs_gen, &obs_id_b) == XAVERIUS_REGISTRY_SUCCESS);
    assert(obs_dev == dev);
    assert(dev->active_observation_count == 2);

    /* Try to confirm removal with only source A's evidence - should fail BUSY
     * because source B still has active evidence. */
    assert(XaveriusRegistryConfirmDeviceRemoved(NULL, dev, dev_gen,
         &(XaveriusDeviceRemovalEvidence){
              .source_key = source_a,
              .source_generation = 1,
              .observation_id = obs_id_a,
              .source_event_sequence = 1,
              .physical_device_generation = dev_gen
          }) == XAVERIUS_REGISTRY_BUSY);
    assert(dev->status != XAVERIUS_REGISTRY_REMOVED);
    assert(xaverius_physical_device_count == 1);
    assert(xaverius_tombstone_count == 0);
    assert(dev->active_observation_count == 2); /* unchanged */

    /* Remove source A's observation. */
    assert(XaveriusRegistryRemoveObservation(NULL, source_a, 1, obs_id_a) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev->active_observation_count == 1);

    /* Confirm removal with source B's evidence - should succeed as B's
     * evidence is the only remaining observation for this device. */
    assert(XaveriusRegistryConfirmDeviceRemoved(NULL, dev, dev_gen,
         &(XaveriusDeviceRemovalEvidence){
              .source_key = source_b,
              .source_generation = 1,
              .observation_id = obs_id_b,
              .source_event_sequence = 1,
              .physical_device_generation = dev_gen
          }) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev->status == XAVERIUS_REGISTRY_REMOVED);
    assert(xaverius_physical_device_count == 0);
    assert(xaverius_tombstone_count == 1);

    /* Clean up source B's observation (now on removed device). */
    assert(XaveriusRegistryRemoveObservation(NULL, source_b, 1, obs_id_b) == XAVERIUS_REGISTRY_SUCCESS);

    XaveriusPhysicalDeviceRelease(dev);
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

    /* Registration using a closed-admission role returns SHUTTING_DOWN,
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
                                             &bind_gen) == XAVERIUS_REGISTRY_SHUTTING_DOWN);
    assert(dev == NULL);
    assert(test_retain_calls == 0);
    assert(test_release_calls == 0);
    assert(snapshot_calls == 0);

    XaveriusModuleRoleFinalize(role);
    XaveriusRegistryFini();
}

static void
xaverius_registry_registration_null_identity(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role = NULL;
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;

    assert(XaveriusRegistryInit());
    make_valid_desc(&desc, "null-id-owner");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);

    setup_min_ops(&ops, true);
    setup_min_reg(&reg, NULL, &ops, role);
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_INVALID_ARGUMENT);
    assert(dev == NULL);
    assert(xaverius_physical_device_count == 0);

    XaveriusModuleRoleFinalize(role);
    XaveriusRegistryFini();
}

static void
xaverius_registry_registration_snapshot_failure(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role = NULL;
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusDeviceIdentity id = { 0 };
    XaveriusPhysicalDevice *dev = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;

    assert(XaveriusRegistryInit());
    make_valid_desc(&desc, "snap-fail-owner");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 50;
    id.device = 1;
    memset(id.driver_uuid, 0x77, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    ops.query_physical_snapshot = test_query_snapshot_fail;
    snapshot_calls = 0;
    setup_min_reg(&reg, &id, &ops, role);
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_UNAVAILABLE);
    assert(dev == NULL);
    assert(snapshot_calls == 1);
    assert(xaverius_physical_device_count == 0);
    /* binding_alloc retained role; binding_free released it on failure path.
     * Role-owned lease retain from create + binding retain from alloc = 2. */
    assert(test_retain_calls == 2);
    assert(test_release_calls == 1);

    XaveriusModuleRoleFinalize(role);
    XaveriusRegistryFini();
}

static void
xaverius_registry_registration_snapshot_bad_data(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role = NULL;
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusDeviceIdentity id = { 0 };
    XaveriusPhysicalDevice *dev = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;

    assert(XaveriusRegistryInit());
    make_valid_desc(&desc, "snap-bad-owner");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 51;
    id.device = 2;
    memset(id.driver_uuid, 0x88, sizeof(id.driver_uuid));
    setup_min_ops(&ops, false);
    snapshot_calls = 0;
    setup_min_reg(&reg, &id, &ops, role);
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_INVALID_EVIDENCE);
    assert(dev == NULL);
    assert(snapshot_calls == 1);
    assert(xaverius_physical_device_count == 0);

    XaveriusModuleRoleFinalize(role);
    XaveriusRegistryFini();
}

static void
xaverius_registry_binding_remove_wrong_generation(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role = NULL;
    XaveriusDeviceIdentity id = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;

    assert(XaveriusRegistryInit());
    make_valid_desc(&desc, "bind-remove-owner");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 60;
    id.device = 1;
    memset(id.driver_uuid, 0x99, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role);

    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);

    /* Wrong device generation -> STALE_GENERATION. */
    assert(XaveriusRegistryRemoveDeviceBackendBinding(NULL, dev, dev_gen + 1,
                                                      role->role_generation,
                                                      bind_gen) == XAVERIUS_REGISTRY_STALE_GENERATION);
    assert(dev->binding != NULL);

    /* Wrong module role generation -> INVALID_ARGUMENT. */
    assert(XaveriusRegistryRemoveDeviceBackendBinding(NULL, dev, dev_gen,
                                                      role->role_generation + 1,
                                                      bind_gen) == XAVERIUS_REGISTRY_INVALID_ARGUMENT);
    assert(dev->binding != NULL);

    /* Wrong binding generation -> INVALID_ARGUMENT. */
    assert(XaveriusRegistryRemoveDeviceBackendBinding(NULL, dev, dev_gen,
                                                      role->role_generation,
                                                      bind_gen + 1) == XAVERIUS_REGISTRY_INVALID_ARGUMENT);
    assert(dev->binding != NULL);

    /* Correct generations -> SUCCESS. */
    assert(XaveriusRegistryRemoveDeviceBackendBinding(NULL, dev, dev_gen,
                                                      role->role_generation,
                                                      bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev->binding == NULL);

    XaveriusPhysicalDeviceRelease(dev);
    XaveriusModuleRoleFinalize(role);
    XaveriusRegistryFini();
}

static void
xaverius_registry_observe_null_identity(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role = NULL;
    XaveriusDeviceIdentity id = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev = NULL;
    XaveriusPhysicalDevice *obs_dev = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;
    uint64_t obs_gen = 0;
    uint64_t obs_id = 0;
    const char source[] = "src-null-id";

    assert(XaveriusRegistryInit());
    make_valid_desc(&desc, "null-id-obs-owner");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 70;
    id.device = 1;
    memset(id.driver_uuid, 0xaa, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role);

    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);

    /* ObserveDevice with NULL identity_evidence -> PENDING (no match possible). */
    assert(XaveriusRegistryObserveDevice(NULL, source, 1, 1, NULL,
                                         &obs_dev, &obs_gen, &obs_id) == XAVERIUS_REGISTRY_PENDING);
    assert(obs_dev == NULL);
    assert(obs_gen == 0);
    assert(obs_id != 0);

    XaveriusPhysicalDeviceRelease(dev);
    XaveriusModuleRoleFinalize(role);
    XaveriusRegistryFini();
}

static void
xaverius_registry_confirm_removed_wrong_generation(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role = NULL;
    XaveriusDeviceIdentity id = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev = NULL;
    XaveriusPhysicalDevice *obs_dev = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;
    uint64_t obs_gen = 0;
    uint64_t obs_id = 0;
    const char source[] = "src-wrong-gen";

    assert(XaveriusRegistryInit());
    make_valid_desc(&desc, "wrong-gen-owner");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 80;
    id.device = 1;
    memset(id.driver_uuid, 0xbb, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role);

    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);

    obs_id = 1001;
    assert(XaveriusRegistryObserveDevice(NULL, source, 1, 1, &id,
                                         &obs_dev, &obs_gen, &obs_id) == XAVERIUS_REGISTRY_SUCCESS);
    assert(obs_dev == dev);

    /* Wrong device generation -> STALE_GENERATION. */
    assert(XaveriusRegistryConfirmDeviceRemoved(NULL, dev, dev_gen + 1,
         &(XaveriusDeviceRemovalEvidence){
              .source_key = source,
              .source_generation = 1,
              .observation_id = obs_id,
              .source_event_sequence = 1,
              .physical_device_generation = dev_gen
          }) == XAVERIUS_REGISTRY_STALE_GENERATION);
    assert(dev->status != XAVERIUS_REGISTRY_REMOVED);
    assert(xaverius_physical_device_count == 1);

    /* Correct generation, valid evidence -> SUCCESS. */
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

    XaveriusPhysicalDeviceRelease(dev);
    XaveriusModuleRoleFinalize(role);
    XaveriusRegistryFini();
}

static void
xaverius_registry_re_register_snapshot_failure(void)
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
    make_valid_desc(&desc, "re-reg-fail-owner");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 90;
    id.device = 1;
    memset(id.driver_uuid, 0xdd, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role);

    /* Initial registration succeeds with a working snapshot callback. */
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);
    assert(xaverius_physical_device_count == 1);
    assert(dev->binding != NULL);
    assert(dev->caller_refcount == 1);

    /* Save pre-re-registration state. */
    uint64_t saved_gen = dev->device_generation;
    uint64_t saved_refcount = dev->caller_refcount;

    /* Remove the binding to enable re-registration. */
    assert(XaveriusRegistryRemoveDeviceBackendBinding(NULL, dev, dev_gen,
                                                      role->role_generation,
                                                      bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev->binding == NULL);

    /* Re-register with a snapshot callback that fails. Must leave the
     * existing device record unchanged (failure-atomic). */
    ops.query_physical_snapshot = test_query_snapshot_fail;
    snapshot_calls = 0;
    setup_min_reg(&reg, &id, &ops, role);
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev2, &dev_gen2,
                                             &bind_gen2) == XAVERIUS_REGISTRY_UNAVAILABLE);
    assert(dev2 == NULL);
    assert(snapshot_calls == 1);

    /* The existing device must be unchanged. */
    assert(dev->binding == NULL); /* binding was removed, not replaced */
    assert(dev->caller_refcount == saved_refcount);
    assert(dev->device_generation == saved_gen);
    assert(xaverius_physical_device_count == 1);

    XaveriusPhysicalDeviceRelease(dev);
    XaveriusModuleRoleFinalize(role);
    XaveriusRegistryFini();
}

static void
xaverius_registry_re_register_snapshot_bad_data(void)
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
    make_valid_desc(&desc, "re-reg-bad-owner");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 91;
    id.device = 1;
    memset(id.driver_uuid, 0xee, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role);

    /* Initial registration succeeds with a working snapshot callback. */
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);
    assert(xaverius_physical_device_count == 1);

    /* Remove the binding to enable re-registration. */
    assert(XaveriusRegistryRemoveDeviceBackendBinding(NULL, dev, dev_gen,
                                                      role->role_generation,
                                                      bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev->binding == NULL);

    /* Re-register with a snapshot callback that returns bad data.
     * Must leave the existing device record unchanged. */
    setup_min_ops(&ops, false);
    snapshot_calls = 0;
    setup_min_reg(&reg, &id, &ops, role);
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev2, &dev_gen2,
                                             &bind_gen2) == XAVERIUS_REGISTRY_INVALID_EVIDENCE);
    assert(dev2 == NULL);
    assert(snapshot_calls == 1);

    /* The existing device must be unchanged. */
    assert(dev->binding == NULL);
    assert(dev->caller_refcount == 1);
    assert(dev->device_generation == dev_gen);
    assert(xaverius_physical_device_count == 1);

    XaveriusPhysicalDeviceRelease(dev);
    XaveriusModuleRoleFinalize(role);
    XaveriusRegistryFini();
}

static void
xaverius_registry_preference_new_wins(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role1 = NULL;
    XaveriusModuleRole *role2 = NULL;
    XaveriusDeviceIdentity id = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev = NULL;
    XaveriusPhysicalDevice *dev2 = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;
    uint64_t dev_gen2 = 0, bind_gen2 = 0;
    static const char *prefs[] = { "preferred-role", NULL };

    assert(XaveriusRegistryInit());
    xaverius_binding_preference_set(prefs);

    /* Role 1: non-preferred owner_key */
    make_valid_desc(&desc, "other-role");
    assert(XaveriusModuleRoleCreate(&desc, &role1) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 95;
    id.device = 1;
    memset(id.driver_uuid, 0x11, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role1);

    /* First registration with non-preferred role succeeds. */
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);
    assert(xaverius_physical_device_count == 1);

    /* Role 2: preferred owner_key */
    make_valid_desc(&desc, "preferred-role");
    assert(XaveriusModuleRoleCreate(&desc, &role2) == XAVERIUS_REGISTRY_SUCCESS);
    setup_min_reg(&reg, &id, &ops, role2);

    /* Second registration with preferred role replaces the binding. */
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev2, &dev_gen2,
                                             &bind_gen2) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev2 == dev); /* same device */
    assert(dev_gen2 != dev_gen); /* new device generation */
    assert(bind_gen2 != bind_gen); /* new binding generation */
    assert(dev->binding != NULL);
    assert(xaverius_physical_device_count == 1);

    XaveriusPhysicalDeviceRelease(dev2);
    XaveriusModuleRoleFinalize(role1);
    XaveriusModuleRoleFinalize(role2);
    XaveriusRegistryFini();
    xaverius_binding_preference_set(NULL);
}

static void
xaverius_registry_preference_existing_wins(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role1 = NULL;
    XaveriusModuleRole *role2 = NULL;
    XaveriusDeviceIdentity id = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev = NULL;
    XaveriusPhysicalDevice *dev2 = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;
    uint64_t dev_gen2 = 0, bind_gen2 = 0;
    static const char *prefs[] = { "preferred-role", NULL };

    assert(XaveriusRegistryInit());
    xaverius_binding_preference_set(prefs);

    /* Role 1: preferred owner_key, registers first */
    make_valid_desc(&desc, "preferred-role");
    assert(XaveriusModuleRoleCreate(&desc, &role1) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 96;
    id.device = 1;
    memset(id.driver_uuid, 0x22, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role1);

    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);
    assert(xaverius_physical_device_count == 1);

    /* Role 2: non-preferred owner_key, tries to register same identity */
    make_valid_desc(&desc, "other-role");
    assert(XaveriusModuleRoleCreate(&desc, &role2) == XAVERIUS_REGISTRY_SUCCESS);
    setup_min_reg(&reg, &id, &ops, role2);

    /* Non-preferred binding cannot replace preferred: AMBIGUOUS. */
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev2, &dev_gen2,
                                             &bind_gen2) == XAVERIUS_REGISTRY_AMBIGUOUS);
    assert(dev2 == NULL);
    assert(dev->binding != NULL); /* existing binding unchanged */
    assert(dev->device_generation == dev_gen); /* generation unchanged */
    assert(xaverius_physical_device_count == 1);

    XaveriusPhysicalDeviceRelease(dev);
    XaveriusModuleRoleFinalize(role1);
    XaveriusModuleRoleFinalize(role2);
    XaveriusRegistryFini();
    xaverius_binding_preference_set(NULL);
}

static void
xaverius_registry_preference_ambiguous_no_pref(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role1 = NULL;
    XaveriusModuleRole *role2 = NULL;
    XaveriusDeviceIdentity id = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev = NULL;
    XaveriusPhysicalDevice *dev2 = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;
    uint64_t dev_gen2 = 0, bind_gen2 = 0;

    assert(XaveriusRegistryInit());
    /* No preference list configured. */

    make_valid_desc(&desc, "role-a");
    assert(XaveriusModuleRoleCreate(&desc, &role1) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 97;
    id.device = 1;
    memset(id.driver_uuid, 0x33, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role1);

    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);
    assert(xaverius_physical_device_count == 1);

    make_valid_desc(&desc, "role-b");
    assert(XaveriusModuleRoleCreate(&desc, &role2) == XAVERIUS_REGISTRY_SUCCESS);
    setup_min_reg(&reg, &id, &ops, role2);

    /* No preference → multiple eligible bindings → AMBIGUOUS. */
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev2, &dev_gen2,
                                             &bind_gen2) == XAVERIUS_REGISTRY_AMBIGUOUS);
    assert(dev2 == NULL);
    assert(dev->binding != NULL); /* existing binding unchanged */
    assert(xaverius_physical_device_count == 1);

    XaveriusPhysicalDeviceRelease(dev);
    XaveriusModuleRoleFinalize(role1);
    XaveriusModuleRoleFinalize(role2);
    XaveriusRegistryFini();
}


static void
xaverius_registry_admission_closed_prevents_callback(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role = NULL;
    XaveriusDeviceIdentity id = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;

    assert(XaveriusRegistryInit());

    make_valid_desc(&desc, "adm-open");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);
    assert(role != NULL);
    assert(XaveriusModuleRoleAdmissionOpen(role) == true);

    XaveriusModuleRoleAdmissionClose(role);
    assert(role->callback_admission_open == 0);

    assert(XaveriusModuleRoleAcquireExecRef(role) == false);
    assert(role->exec_ref == 0);

    id.bus = 22;
    id.device = 32;
    memset(id.driver_uuid, 0xee, sizeof(id.driver_uuid));
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
    assert(role->exec_ref == 0);

    XaveriusModuleRoleFinalize(role);
    XaveriusRegistryFini();
}

static void
xaverius_registry_dix_lifecycle(void)
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
    XID pub_xid = 0;
    void *res = NULL;
    int rc;

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
    make_valid_desc(&desc, "dix-life");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 4;
    id.device = 5;
    memset(id.driver_uuid, 0xaa, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role);

    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);
    assert(dev->xid == 0);

    assert(XaveriusPhysicalDevicePublish(dev, dev_gen, &pub_xid) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev->xid != 0);
    assert(pub_xid == dev->xid);
    assert(dev->dix_wrapper != NULL);
    assert(dev->dix_wrapper->refcount == 1);
    assert(dev->dix_resource_refcount == 1);

    res = NULL;
    rc = dixLookupResourceByType(&res, dev->xid, XaveriusPhysicalDeviceType,
                                 serverClient, DixReadAccess);
    assert(rc == Success);
    assert(((XaveriusPhysicalDeviceDIXWrapper *)res)->dev == dev);

    FreeResource(dev->xid, RT_NONE);
    dev->xid = 0;
    assert(dev->dix_wrapper == NULL);
    assert(dev->dix_resource_refcount == 0);

    assert(xaverius_physical_device_count == 1);

    assert(XaveriusPhysicalDevicePublish(dev, dev_gen, &pub_xid)
           == XAVERIUS_REGISTRY_ALREADY_EXISTS);
    assert(dev->dix_wrapper == NULL);
    assert(dev->dix_resource_refcount == 0);

    reset_hook_counts();
    XaveriusRegistryFini();
    assert(test_release_calls >= 1);
    assert(xaverius_physical_device_count == 0);
}

static void
xaverius_registry_fini_quiesce_pending_failure(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role_ok = NULL;
    XaveriusModuleRole *role_pend = NULL;
    XaveriusModuleRole *role_fail = NULL;
    XaveriusDeviceIdentity id = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev_ok = NULL;
    XaveriusPhysicalDevice *dev_pend = NULL;
    XaveriusPhysicalDevice *dev_fail = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;
    static const char *prefs[] = { NULL };

    assert(XaveriusRegistryInit());
    xaverius_binding_preference_set(prefs);

    make_valid_desc(&desc, "role-ok");
    assert(XaveriusModuleRoleCreate(&desc, &role_ok) == XAVERIUS_REGISTRY_SUCCESS);

    make_valid_desc(&desc, "role-pend");
    desc.quiesce_module_role = quiesce_pending;
    assert(XaveriusModuleRoleCreate(&desc, &role_pend) == XAVERIUS_REGISTRY_SUCCESS);

    make_valid_desc(&desc, "role-fail");
    desc.quiesce_module_role = quiesce_failure;
    assert(XaveriusModuleRoleCreate(&desc, &role_fail) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 50;
    id.device = 1;
    memset(id.driver_uuid, 0x11, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;

    setup_min_reg(&reg, &id, &ops, role_ok);
    id.device = 1;
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev_ok, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev_ok != NULL);

    id.device = 2;
    setup_min_reg(&reg, &id, &ops, role_pend);
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev_pend, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev_pend != NULL);

    id.device = 3;
    setup_min_reg(&reg, &id, &ops, role_fail);
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev_fail, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev_fail != NULL);

    assert(XaveriusModuleRoleQuiesce(role_pend) == XAVERIUS_MODULE_QUIESCE_PENDING);
    assert(role_pend->quiesce_state != XAVERIUS_MODULE_QUIESCE_SUCCESS);
    assert(XaveriusModuleRoleQuiesce(role_fail) == XAVERIUS_MODULE_QUIESCE_FAILURE);
    assert(role_fail->quiesce_state == XAVERIUS_MODULE_QUIESCE_FAILURE);

    reset_hook_counts();
    XaveriusRegistryFini();

    /* SUCCESS device freed (count decremented). PENDING/FAILURE devices
     * retained — their roles stay alive because retained bindings still
     * hold references via binding->module_role. */
    assert(xaverius_physical_device_count == 2);
    assert(xaverius_tombstone_count == 0);

    /* PENDING/FAILURE roles remain alive and MUST be quarantined so that
     * XaveriusModuleRoleFinalize() will refuse to free them while retained
     * bindings/devices still reference them via binding->module_role. */
    assert(role_pend->quarantined == 1);
    assert(role_fail->quarantined == 1);
    assert(role_pend->quiesce_state != XAVERIUS_MODULE_QUIESCE_SUCCESS);
    assert(role_fail->quiesce_state == XAVERIUS_MODULE_QUIESCE_FAILURE);

    /* A subsequent Init must not overwrite list heads or reset counts
     * while retained objects exist — it must return without mutating. */
    assert(XaveriusRegistryInit() == true);
    assert(xaverius_physical_device_count == 2);
    assert(xaverius_tombstone_count == 0);
    assert(role_pend->quiesce_state != XAVERIUS_MODULE_QUIESCE_SUCCESS);
    assert(role_fail->quiesce_state == XAVERIUS_MODULE_QUIESCE_FAILURE);

    /* Save role generations before attempting Finalize — used to prove
     * exact pointer identity after Finalize (no dereferencing freed roles). */
    uint64_t role_pend_gen = role_pend->role_generation;
    uint64_t role_fail_gen = role_fail->role_generation;

    /* Retained devices/roles remain valid — verify they are still
     * accessible through their bindings and roles. */
    assert(dev_pend->binding != NULL);
    assert(dev_pend->binding->module_role == role_pend);
    assert(dev_fail->binding != NULL);
    assert(dev_fail->binding->module_role == role_fail);

    /* Attempted Finalize on a quarantined role must NOT unlink/free it
     * while retained bindings still point to it. Verify list membership
     * count, exact pointer identity via role_generation lookup, and
     * quarantined flag remain intact. Owner release count unchanged. */
    reset_hook_counts();
    XaveriusModuleRoleFinalize(role_pend);
    XaveriusModuleRoleFinalize(role_fail);
    assert(xaverius_module_role_count == 2);
    /* Look up by generation to prove exact pointer identity without
     * dereferencing potentially freed roles. */
    struct _XaveriusModuleRole *found_pend = xaverius_find_module_role_by_generation(role_pend_gen);
    struct _XaveriusModuleRole *found_fail = xaverius_find_module_role_by_generation(role_fail_gen);
    assert(found_pend == role_pend);
    assert(found_fail == role_fail);
    assert(role_pend->quarantined == 1);
    assert(role_fail->quarantined == 1);
    /* Owner release count unchanged (Finalize on quarantined is no-op). */
    assert(test_retain_calls == 0);
    assert(test_release_calls == 0);

    xaverius_binding_preference_set(NULL);
}

static void
xaverius_registry_quiesce_retry_retirement(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role_pend = NULL;
    XaveriusModuleRole *role_fail = NULL;
    XaveriusDeviceIdentity id = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev_pend = NULL;
    XaveriusPhysicalDevice *dev_fail = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;
    uint64_t role_pend_gen = 0, role_fail_gen = 0;
    static const char *prefs[] = { NULL };

    assert(XaveriusRegistryInit());
    xaverius_binding_preference_set(prefs);

    /* Role that first quiesces PENDING, later retries SUCCESS. */
    make_valid_desc(&desc, "role-pend-retry");
    desc.quiesce_module_role = quiesce_pending;
    assert(XaveriusModuleRoleCreate(&desc, &role_pend) == XAVERIUS_REGISTRY_SUCCESS);
    role_pend_gen = role_pend->role_generation;

    /* Role that permanently quiesces FAILURE (quarantine never cleared). */
    make_valid_desc(&desc, "role-fail-retry");
    desc.quiesce_module_role = quiesce_failure;
    assert(XaveriusModuleRoleCreate(&desc, &role_fail) == XAVERIUS_REGISTRY_SUCCESS);
    role_fail_gen = role_fail->role_generation;

    id.bus = 80;
    id.device = 1;
    memset(id.driver_uuid, 0xaa, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;

    /* Register device bound to PENDING role. */
    setup_min_reg(&reg, &id, &ops, role_pend);
    id.device = 1;
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev_pend, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev_pend != NULL);

    /* Register device bound to FAILURE role. */
    id.device = 2;
    setup_min_reg(&reg, &id, &ops, role_fail);
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev_fail, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev_fail != NULL);

    /* First quiesce: PENDING → PENDING state (not yet quarantined). FAILURE → FAILURE. */
    assert(XaveriusModuleRoleQuiesce(role_pend) == XAVERIUS_MODULE_QUIESCE_PENDING);
    assert(role_pend->quiesce_state != XAVERIUS_MODULE_QUIESCE_SUCCESS);
    assert(role_pend->quarantined == 0); /* not quarantined until Fini */

    assert(XaveriusModuleRoleQuiesce(role_fail) == XAVERIUS_MODULE_QUIESCE_FAILURE);
    assert(role_fail->quiesce_state == XAVERIUS_MODULE_QUIESCE_FAILURE);
    assert(role_fail->quarantined == 0); /* not quarantined until Fini */

    /* RegistryFini: SUCCESS devices freed, PENDING/FAILURE retained + quarantined. */
    reset_hook_counts();
    XaveriusRegistryFini();
    assert(xaverius_physical_device_count == 2); /* both PENDING/FAILURE devices retained */
    assert(xaverius_module_role_count == 2);     /* both roles retained */

    /* After Fini: both roles quarantined (set during Fini, not during Quiesce). */
    assert(role_pend->quarantined == 1);
    assert(role_fail->quarantined == 1);

    /* Before retry: both roles still findable by generation. */
    assert(xaverius_find_module_role_by_generation(role_pend_gen) == role_pend);
    assert(xaverius_find_module_role_by_generation(role_fail_gen) == role_fail);

    /* Simulate retry: change hook to one that returns SUCCESS, then quiesce.
     * Quarantine is NOT cleared by Quiesce() alone — only cleared during
     * RegistryFini() when the role is actually finalized. */
    role_pend->quiesce_module_role = counting_quiesce;
    assert(XaveriusModuleRoleQuiesce(role_pend) == XAVERIUS_MODULE_QUIESCE_SUCCESS);
    assert(role_pend->quiesce_state == XAVERIUS_MODULE_QUIESCE_SUCCESS);
    assert(role_pend->quarantined == 1); /* quarantine NOT cleared by Quiesce() alone */

    /* Direct Finalize on quarantined SUCCESS role must be a NO-OP:
     * role stays findable by generation, binding still points to it,
     * role count unchanged, quarantine remains set. */
    reset_hook_counts();
    XaveriusModuleRoleFinalize(role_pend);
    assert(xaverius_find_module_role_by_generation(role_pend_gen) == role_pend);
    assert(role_pend->quarantined == 1);
    assert(dev_pend->binding != NULL);
    assert(dev_pend->binding->module_role == role_pend);
    assert(test_retain_calls == 0);
    assert(test_release_calls == 0);
    assert(xaverius_module_role_count == 2);

    /* Now Fini again: SUCCESS role finalized (freed), FAILURE role retained.
     * Quarantine is cleared during Fini() when SUCCESS role is finalized. */
    reset_hook_counts();
    XaveriusRegistryFini();
    /* Device/binding released (1), role finalized releasing role-owned lease (1). */
    assert(test_release_calls == 2);  /* binding release + role-owned lease release */
    assert(test_retain_calls == 0);
    assert(xaverius_physical_device_count == 1); /* only FAILURE device retained */
    assert(xaverius_module_role_count == 1);     /* only FAILURE role retained */

    /* Only FAILURE role still findable; PENDING role was finalized. */
    assert(xaverius_find_module_role_by_generation(role_pend_gen) == NULL);
    assert(xaverius_find_module_role_by_generation(role_fail_gen) == role_fail);
    assert(role_fail->quarantined == 1);

    /* Retained role remains valid — verify binding still references it. */
    assert(dev_fail->binding != NULL);
    assert(dev_fail->binding->module_role == role_fail);

    /* Release remaining references. Role stays quarantined (FAILURE roles
     * are never retired). Device and role remain in lists — this is
     * expected for permanently failed roles. */
    XaveriusPhysicalDeviceRelease(dev_fail);
    /* XaveriusModuleRoleFinalize(role_fail) would be a no-op — role is
     * quarantined and stays retained until server shutdown. */
    xaverius_binding_preference_set(NULL);
}

static void
xaverius_registry_preference_cannot_replace_published(void)
{
    static ClientRec test_client;
    static int dix_inited;
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role1 = NULL;
    XaveriusModuleRole *role2 = NULL;
    XaveriusDeviceIdentity id = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev = NULL;
    XaveriusPhysicalDevice *dev2 = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;
    uint64_t dev_gen2 = 0, bind_gen2 = 0;
    static const char *prefs[] = { "preferred-role", NULL };
    XID pub_xid = 0;
    void *res = NULL;
    int rc;

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

    assert(XaveriusRegistryInit());
    xaverius_binding_preference_set(prefs);

    make_valid_desc(&desc, "other-role");
    assert(XaveriusModuleRoleCreate(&desc, &role1) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 60;
    id.device = 1;
    memset(id.driver_uuid, 0x11, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role1);

    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);
    assert(xaverius_physical_device_count == 1);

    assert(XaveriusPhysicalDevicePublish(dev, dev_gen, &pub_xid) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev->xid != 0);

    res = NULL;
    rc = dixLookupResourceByType(&res, dev->xid, XaveriusPhysicalDeviceType,
                                 serverClient, DixReadAccess);
    assert(rc == Success);

    make_valid_desc(&desc, "preferred-role");
    assert(XaveriusModuleRoleCreate(&desc, &role2) == XAVERIUS_REGISTRY_SUCCESS);
    setup_min_reg(&reg, &id, &ops, role2);

    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev2, &dev_gen2,
                                             &bind_gen2) == XAVERIUS_REGISTRY_AMBIGUOUS);
    assert(dev2 == NULL);
    assert(dev->binding != NULL);
    assert(dev->device_generation == dev_gen);
    assert(dev->xid == pub_xid);
    assert(xaverius_physical_device_count == 1);

    XaveriusPhysicalDeviceRelease(dev);
    XaveriusModuleRoleFinalize(role1);
    XaveriusModuleRoleFinalize(role2);
    XaveriusRegistryFini();
    xaverius_binding_preference_set(NULL);
}

static void
xaverius_registry_init_reentry(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role = NULL;
    XaveriusDeviceIdentity id = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;

    assert(XaveriusRegistryInit());
    assert(xaverius_physical_device_count == 0);
    assert(xaverius_module_role_count == 0);

    make_valid_desc(&desc, "reentry-owner");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);
    assert(xaverius_module_role_count == 1);

    id.bus = 70;
    id.device = 1;
    memset(id.driver_uuid, 0x22, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role);
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen,
                                             &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);
    assert(xaverius_physical_device_count == 1);

    assert(XaveriusRegistryInit());
    assert(xaverius_module_role_count == 1);
    assert(xaverius_physical_device_count == 1);

    assert(role->owner_key != NULL);
    assert(dev->device_generation != 0);

    XaveriusModuleRoleQuiesce(role);
    assert(role->quiesce_state == XAVERIUS_MODULE_QUIESCE_SUCCESS);
    XaveriusPhysicalDeviceRelease(dev);
    XaveriusModuleRoleFinalize(role);
    XaveriusRegistryFini();
    assert(xaverius_module_role_count == 0);
    assert(xaverius_physical_device_count == 0);
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
        xaverius_registry_role_validation_errors,
        xaverius_registry_sequence_exhaustion,
        xaverius_registry_multi_source_removal,
        xaverius_module_role_admission_gate,
        /* New focused regression tests for genuine coverage gaps */
        xaverius_registry_registration_null_identity,
        xaverius_registry_registration_snapshot_failure,
        xaverius_registry_registration_snapshot_bad_data,
        xaverius_registry_binding_remove_wrong_generation,
        xaverius_registry_observe_null_identity,
        xaverius_registry_confirm_removed_wrong_generation,
        /* Re-registration failure-atomicity tests */
        xaverius_registry_re_register_snapshot_failure,
        xaverius_registry_re_register_snapshot_bad_data,
        /* Owner-key preference selection tests (W0-153 item 4-5) */
        xaverius_registry_preference_new_wins,
        xaverius_registry_preference_existing_wins,
        xaverius_registry_preference_ambiguous_no_pref,
        /* Blocker 4 regression tests (W0-153 review) */
        xaverius_registry_admission_closed_prevents_callback,
        xaverius_registry_dix_lifecycle,
        xaverius_registry_fini_quiesce_pending_failure,
        xaverius_registry_preference_cannot_replace_published,
        xaverius_registry_init_reentry,
        xaverius_registry_quiesce_retry_retirement,
        NULL,
    };

    return testfuncs;
}