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

    /* Same DRM identity, UUID only on one side => not equal */
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

    /* Both have UUIDs: must compare and differ on mismatch */
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

    /* PCI vs non-PCI never equal */
    {
        XaveriusDeviceIdentity pci = { 0 };
        XaveriusDeviceIdentity drm = { 0 };
        pci.bus = 1;
        drm.drm_major = 226;
        assert(!xaverius_identity_equal(&pci, &drm));
    }

    /* UUID-only identities */
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

    /*
     * Ownership plan (module-owner original ref + binding refs):
     * - XaveriusModuleRoleCreate() creates and registers a role; no hook called.
     *   The module owner holds the original role reference.
     * - XaveriusModuleRoleRetain/Release() are lifetime hook primitives.
     * - xaverius_binding_alloc() retains the role for a binding; xaverius_binding_free()
     *   releases that binding reference (exactly once per binding lifetime).
     * - XaveriusModuleRoleFinalize() invokes the release hook once to drop the
     *   module owner's original reference and frees server-owned role storage.
     *   Caller must invoke finalize exactly once after all binding refs are gone.
     * - XaveriusRegistryFini() finalizes any remaining roles via Finalize() to
     *   perform ordered shutdown cleanup.
     */

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

    /* Finalize releases original owner reference and frees role storage. */
    XaveriusModuleRoleFinalize(role);
    assert(test_release_calls == 2);

    /* Invalid descriptors must not invoke hooks. */
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

/* ---- Registration helpers for ownership/binding tests ---- */
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

    /* Pre-binding validation failures must not call role hooks. */
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

    /* Snapshot callback failure after binding alloc: retain/release balanced. */
    reg.abi_version = XAVERIUS_DEVICE_BACKEND_REGISTRATION_ABI_VERSION;
    snapshot_calls = 0;
    snapshot_result_mode = XAVERIUS_BACKEND_UNAVAILABLE;
    reset_hook_counts();
    dev = NULL; dev_gen = bind_gen = 0;
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_UNAVAILABLE);
    assert(snapshot_calls == 1);
    assert(test_retain_calls == 1);
    assert(test_release_calls == 1);

    /* Invalid snapshot content: retain/release balanced. */
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    ops.query_physical_snapshot = test_query_snapshot_bad;
    reg.ops = &ops;
    snapshot_calls = 0;
    reset_hook_counts();
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_INVALID_EVIDENCE);
    assert(snapshot_calls == 1);
    assert(test_retain_calls == 1);
    assert(test_release_calls == 1);

    /* Successful registration: binding retains role once. */
    setup_min_ops(&ops, true);
    setup_min_reg(&reg, &id, &ops, role);
    snapshot_calls = 0;
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    reset_hook_counts();
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);
    assert(snapshot_calls == 1);
    assert(test_retain_calls == 1);
    assert(test_release_calls == 0);
    assert(dev->binding != NULL);
    assert(dev->xid == 0);

    /* Exact replay: additional caller ref, no new binding/retain. */
    {
        XaveriusPhysicalDevice *dev2 = NULL;
        uint64_t dg2 = 0, bg2 = 0;
        uint32_t ref_before = dev->caller_refcount;
        assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev2, &dg2, &bg2) == XAVERIUS_REGISTRY_SUCCESS);
        assert(dev2 == dev);
        assert(dg2 == dev_gen);
        assert(bg2 == bind_gen);
        assert(dev->caller_refcount == ref_before + 1);
        assert(test_retain_calls == 1);
        assert(test_release_calls == 0);
        XaveriusPhysicalDeviceRelease(dev2);
        assert(dev->caller_refcount == ref_before);
    }

    /* Conflict: same identity, different owning role -> no hook calls. */
    {
        XaveriusModuleRoleDescriptor desc2;
        XaveriusModuleRole *role2 = NULL;
        struct XaveriusDeviceBackendOps ops2;
        XaveriusDeviceBackendRegistration reg2;
        XaveriusPhysicalDevice *devc = NULL;
        uint64_t dgc = 0, bgc = 0;
        make_valid_desc(&desc2, "reg-owner-2");
        reset_hook_counts();
        assert(XaveriusModuleRoleCreate(&desc2, &role2) == XAVERIUS_REGISTRY_SUCCESS);
        assert(test_retain_calls == 0 && test_release_calls == 0);
        setup_min_ops(&ops2, true);
        setup_min_reg(&reg2, &id, &ops2, role2);
        reset_hook_counts();
        assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg2, &devc, &dgc, &bgc) == XAVERIUS_REGISTRY_IDENTITY_CONFLICT);
        assert(test_retain_calls == 0);
        assert(test_release_calls == 0);

        /* Binding removal releases exactly one retained binding role ref. */
        reset_hook_counts();
        assert(XaveriusRegistryRemoveDeviceBackendBinding(NULL, dev, dev_gen, role->role_generation, bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
        assert(dev->binding == NULL);
        assert(test_release_calls == 1);

        XaveriusPhysicalDeviceRelease(dev);

        /* Shutdown finalizes remaining roles (owner original release once each). */
        reset_hook_counts();
        XaveriusRegistryFini();
        assert(test_release_calls == 2); /* role + role2 */
        return;
    }

    /* Fallback (not reached) */
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

    {
        XaveriusPhysicalDevice *obs_dev = NULL;
        uint64_t obs_gen = 0;
        const char source[] = "src-tomb";
        uint64_t obs_id = 1001;
        assert(XaveriusRegistryObserveDevice(NULL, &id, sizeof(id),
                                             source, sizeof(source),
                                             1, obs_id,
                                             &obs_dev, &obs_gen) == XAVERIUS_REGISTRY_SUCCESS);
        assert(obs_dev == dev);

        /* Confirm removal moves device exactly once to tombstone list. */
        assert(XaveriusRegistryConfirmDeviceRemoved(NULL, dev, dev_gen, &id, sizeof(id)) == XAVERIUS_REGISTRY_SUCCESS);
        assert(dev->status == XAVERIUS_REGISTRY_REMOVED);
        assert(xaverius_physical_device_count == 0);
        assert(xaverius_tombstone_count == 1);
        assert(dev->xid == 0); /* unpublished device keeps no XID */

        /* Second removal attempt must not move again. */
        assert(XaveriusRegistryConfirmDeviceRemoved(NULL, dev, dev_gen, &id, sizeof(id)) == XAVERIUS_REGISTRY_STALE_EVENT);
        assert(xaverius_physical_device_count == 0);
        assert(xaverius_tombstone_count == 1);
    }

    /* Ordered shutdown frees tombstones and finalizes remaining role. */
    reset_hook_counts();
    XaveriusRegistryFini();
    assert(test_release_calls == 1); /* binding-retained role ref finalized on shutdown cleanup path */
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

    /* Successful publication with real DIX resource registration. */
    assert(XaveriusPhysicalDevicePublish(dev) == true);
    assert(dev->xid != 0);
    res = NULL;
    rc = dixLookupResourceByType(&res, dev->xid, XaveriusPhysicalDeviceType,
                                 serverClient, DixReadAccess);
    assert(rc == Success);
    assert(res == dev);

    /* Duplicate publication must fail and keep same XID/resource mapping. */
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

    /* Ordered shutdown must release DIX wrapper and registry/tombstone state. */
    reset_hook_counts();
    XaveriusRegistryFini();
    assert(test_release_calls >= 1);
}

static void
xaverius_registry_observe_and_remove(void)
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
    const char source[] = "src-observe";
    uint64_t obs_id = 2001;

    assert(XaveriusRegistryInit());
    make_valid_desc(&desc, "obs-owner");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 5;
    id.device = 1;
    memset(id.driver_uuid, 0x77, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role);

    /* Register a device first. */
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);

    /* NULL identity_evidence -> INVALID_ARGUMENT. */
    assert(XaveriusRegistryObserveDevice(NULL, NULL, 0, source, sizeof(source),
                                         1, obs_id, &obs_dev, &obs_gen) == XAVERIUS_REGISTRY_INVALID_ARGUMENT);

    /* NULL source_key -> INVALID_ARGUMENT. */
    assert(XaveriusRegistryObserveDevice(NULL, &id, sizeof(id), NULL, 0,
                                         1, obs_id, &obs_dev, &obs_gen) == XAVERIUS_REGISTRY_INVALID_ARGUMENT);

    /* Wrong identity_evidence_size -> INVALID_EVIDENCE. */
    assert(XaveriusRegistryObserveDevice(NULL, &id, sizeof(id) - 1, source, sizeof(source),
                                         1, obs_id, &obs_dev, &obs_gen) == XAVERIUS_REGISTRY_INVALID_EVIDENCE);

    /* Successful observation with matching device -> SUCCESS. */
    assert(XaveriusRegistryObserveDevice(NULL, &id, sizeof(id),
                                         source, sizeof(source),
                                         1, obs_id,
                                         &obs_dev, &obs_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(obs_dev == dev);
    assert(obs_gen == dev_gen);

    /* Stale source generation -> STALE_EVENT. */
    assert(XaveriusRegistryObserveDevice(NULL, &id, sizeof(id),
                                         source, sizeof(source),
                                         2, obs_id + 1,
                                         &obs_dev, &obs_gen) == XAVERIUS_REGISTRY_STALE_EVENT);

    /* RemoveObservation with NULL source_key -> INVALID_ARGUMENT. */
    assert(XaveriusRegistryRemoveObservation(NULL, NULL, 1, obs_id) == XAVERIUS_REGISTRY_INVALID_ARGUMENT);

    /* RemoveObservation with non-existent source_key -> NOT_FOUND. */
    assert(XaveriusRegistryRemoveObservation(NULL, "unknown", 1, obs_id) == XAVERIUS_REGISTRY_NOT_FOUND);

    /* RemoveObservation with wrong generation -> NOT_FOUND. */
    assert(XaveriusRegistryRemoveObservation(NULL, source, 2, obs_id) == XAVERIUS_REGISTRY_NOT_FOUND);

    /* RemoveObservation with wrong observation_id -> NOT_FOUND. */
    assert(XaveriusRegistryRemoveObservation(NULL, source, 1, obs_id + 1) == XAVERIUS_REGISTRY_NOT_FOUND);

    /* RemoveObservation with correct source_key/generation/id -> SUCCESS. */
    assert(XaveriusRegistryRemoveObservation(NULL, source, 1, obs_id) == XAVERIUS_REGISTRY_SUCCESS);

    /* RemoveObservation again for same observation -> NOT_FOUND. */
    assert(XaveriusRegistryRemoveObservation(NULL, source, 1, obs_id) == XAVERIUS_REGISTRY_NOT_FOUND);

    /* Ordered shutdown. */
    XaveriusRegistryFini();
}

static void
xaverius_registry_multi_source_observation(void)
{
    XaveriusModuleRoleDescriptor desc;
    XaveriusModuleRole *role = NULL;
    XaveriusDeviceIdentity id = { 0 };
    struct XaveriusDeviceBackendOps ops;
    XaveriusDeviceBackendRegistration reg;
    XaveriusPhysicalDevice *dev = NULL;
    uint64_t dev_gen = 0, bind_gen = 0;
    XaveriusPhysicalDevice *obs_dev_a = NULL;
    XaveriusPhysicalDevice *obs_dev_b = NULL;
    uint64_t obs_gen_a = 0, obs_gen_b = 0;
    const char source_a[] = "src-a";
    const char source_b[] = "src-b";
    uint64_t obs_id_a = 3001;
    uint64_t obs_id_b = 3002;

    assert(XaveriusRegistryInit());
    make_valid_desc(&desc, "multi-src-owner");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);

    id.bus = 9;
    id.device = 3;
    memset(id.driver_uuid, 0x88, sizeof(id.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id, &ops, role);

    /* Register one PhysicalDevice. */
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);
    assert(xaverius_physical_device_count == 1);
    assert(xaverius_tombstone_count == 0);

    /* Observe same identity from source A. */
    assert(XaveriusRegistryObserveDevice(NULL, &id, sizeof(id),
                                         source_a, sizeof(source_a),
                                         1, obs_id_a,
                                         &obs_dev_a, &obs_gen_a) == XAVERIUS_REGISTRY_SUCCESS);
    assert(obs_dev_a == dev);
    assert(obs_gen_a == dev_gen);

    /* Observe same identity from source B -> same canonical device+generation. */
    assert(XaveriusRegistryObserveDevice(NULL, &id, sizeof(id),
                                         source_b, sizeof(source_b),
                                         1, obs_id_b,
                                         &obs_dev_b, &obs_gen_b) == XAVERIUS_REGISTRY_SUCCESS);
    assert(obs_dev_b == dev);
    assert(obs_gen_b == dev_gen);
    assert(obs_dev_a == obs_dev_b);

    /* Remove only source A's observation -> device must NOT be tombstoned. */
    assert(XaveriusRegistryRemoveObservation(NULL, source_a, 1, obs_id_a) == XAVERIUS_REGISTRY_SUCCESS);
    assert(xaverius_physical_device_count == 1);
    assert(xaverius_tombstone_count == 0);
    assert(dev->status != XAVERIUS_REGISTRY_REMOVED);

    /* Stale source generation removal -> NOT_FOUND, no side effects. */
    assert(XaveriusRegistryRemoveObservation(NULL, source_a, 2, obs_id_a) == XAVERIUS_REGISTRY_NOT_FOUND);
    assert(xaverius_physical_device_count == 1);
    assert(xaverius_tombstone_count == 0);

    /* Wrong observation ID removal -> NOT_FOUND, no side effects. */
    assert(XaveriusRegistryRemoveObservation(NULL, source_a, 1, obs_id_a + 1) == XAVERIUS_REGISTRY_NOT_FOUND);
    assert(xaverius_physical_device_count == 1);
    assert(xaverius_tombstone_count == 0);

    /* Unknown source key removal -> NOT_FOUND, no side effects. */
    assert(XaveriusRegistryRemoveObservation(NULL, "unknown", 1, obs_id_a) == XAVERIUS_REGISTRY_NOT_FOUND);
    assert(xaverius_physical_device_count == 1);
    assert(xaverius_tombstone_count == 0);

    /* Source B's observation still present; ConfirmDeviceRemoved works. */
    assert(XaveriusRegistryConfirmDeviceRemoved(NULL, dev, dev_gen, &id, sizeof(id)) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev->status == XAVERIUS_REGISTRY_REMOVED);
    assert(xaverius_physical_device_count == 0);
    assert(xaverius_tombstone_count == 1);

    /* Remove source B's observation after device is tombstoned. */
    assert(XaveriusRegistryRemoveObservation(NULL, source_b, 1, obs_id_b) == XAVERIUS_REGISTRY_SUCCESS);

    /* Repeated confirmation -> STALE_EVENT. */
    assert(XaveriusRegistryConfirmDeviceRemoved(NULL, dev, dev_gen, &id, sizeof(id)) == XAVERIUS_REGISTRY_STALE_EVENT);
    assert(xaverius_tombstone_count == 1);

    /* Ordered shutdown. */
    XaveriusRegistryFini();
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

    /* Register one PhysicalDevice successfully. */
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev != NULL);
    assert(xaverius_physical_device_count == 1);
    assert(xaverius_tombstone_count == 0);
    assert(dev->xid == 0); /* server owns XID allocation; none during registration */

    /* Exact replay: same identity + same role -> additional caller ref, no new device. */
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev2, &dev_gen2, &bind_gen2) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev2 == dev);
    assert(dev_gen2 == dev_gen);
    assert(bind_gen2 == bind_gen);
    assert(xaverius_physical_device_count == 1); /* no second canonical device */
    XaveriusPhysicalDeviceRelease(dev2);
    assert(xaverius_physical_device_count == 1);

    /* Identity conflict: same identity, different owning role -> IDENTITY_CONFLICT,
     * no second canonical device/resource created. */
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
        assert(xaverius_physical_device_count == 1); /* still exactly one canonical device */
        assert(xaverius_tombstone_count == 0);
        XaveriusModuleRoleFinalize(role2);
    }

    /* Invalid registration input: NULL ops -> INVALID_ARGUMENT, no side effects. */
    {
        XaveriusDeviceBackendRegistration bad = reg;
        bad.ops = NULL;
        XaveriusPhysicalDevice *devd = NULL;
        uint64_t dgd = 0, bgd = 0;
        uint32_t count_before = xaverius_physical_device_count;
        assert(XaveriusRegistryRegisterDdxDevice(NULL, &bad, &devd, &dgd, &bgd) == XAVERIUS_REGISTRY_INVALID_ARGUMENT);
        assert(devd == NULL);
        assert(xaverius_physical_device_count == count_before); /* no side effects */
    }

    /* Invalid registration input: wrong abi_version -> INVALID_ARGUMENT, no side effects. */
    {
        XaveriusDeviceBackendRegistration bad = reg;
        bad.abi_version = XAVERIUS_DEVICE_BACKEND_REGISTRATION_ABI_VERSION + 1;
        XaveriusPhysicalDevice *deve = NULL;
        uint64_t dge = 0, bge = 0;
        uint32_t count_before = xaverius_physical_device_count;
        assert(XaveriusRegistryRegisterDdxDevice(NULL, &bad, &deve, &dge, &bge) == XAVERIUS_REGISTRY_INVALID_ARGUMENT);
        assert(deve == NULL);
        assert(xaverius_physical_device_count == count_before); /* no side effects */
    }

    /* Release the binding for the original device. */
    assert(XaveriusRegistryRemoveDeviceBackendBinding(NULL, dev, dev_gen, role->role_generation, bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev->binding == NULL);

    /* Re-register same identity + same role after binding removal -> new registration succeeds. */
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev2, &dev_gen2, &bind_gen2) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev2 != NULL);
    assert(xaverius_physical_device_count == 1); /* still exactly one canonical device */
    assert(dev2->xid == 0);
    assert(bind_gen2 != bind_gen); /* new binding generation */
    XaveriusPhysicalDeviceRelease(dev2);

    /* Ordered shutdown. */
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
    const char source[] = "src-edge";
    uint64_t obs_id = 4001;

    assert(XaveriusRegistryInit());
    make_valid_desc(&desc, "edge-owner");
    assert(XaveriusModuleRoleCreate(&desc, &role) == XAVERIUS_REGISTRY_SUCCESS);

    id_a.bus = 13;
    id_a.device = 7;
    memset(id_a.driver_uuid, 0xaa, sizeof(id_a.driver_uuid));
    setup_min_ops(&ops, true);
    snapshot_result_mode = XAVERIUS_BACKEND_SUCCESS;
    setup_min_reg(&reg, &id_a, &ops, role);

    /* Register device A. */
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev_a, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev_a != NULL);

    /* Observe an identity with no matching registered device → PENDING,
     * no device reference, generation zero. */
    id_b.bus = 17;
    id_b.device = 11;
    memset(id_b.driver_uuid, 0xbb, sizeof(id_b.driver_uuid));
    assert(XaveriusRegistryObserveDevice(NULL, &id_b, sizeof(id_b),
                                         source, sizeof(source),
                                         1, obs_id,
                                         &obs_dev, &obs_gen) == XAVERIUS_REGISTRY_PENDING);
    assert(obs_dev == NULL);
    assert(obs_gen == 0);

    /* Observe the registered identity again → same canonical device,
     * same generation (idempotent replay). */
    assert(XaveriusRegistryObserveDevice(NULL, &id_a, sizeof(id_a),
                                         source, sizeof(source),
                                         1, obs_id + 1,
                                         &obs_dev, &obs_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(obs_dev == dev_a);
    assert(obs_gen == dev_gen);

    /* Observe a second, distinct identity → distinct canonical device. */
    id_b.bus = 19;
    id_b.device = 23;
    memset(id_b.driver_uuid, 0xcc, sizeof(id_b.driver_uuid));
    setup_min_reg(&reg, &id_b, &ops, role);
    assert(XaveriusRegistryRegisterDdxDevice(NULL, &reg, &dev_b, &dev_gen, &bind_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(dev_b != NULL);
    assert(dev_b != dev_a);

    assert(XaveriusRegistryObserveDevice(NULL, &id_b, sizeof(id_b),
                                         source, sizeof(source),
                                         1, obs_id + 2,
                                         &obs_dev, &obs_gen) == XAVERIUS_REGISTRY_SUCCESS);
    assert(obs_dev == dev_b);
    assert(obs_gen == dev_gen);
    assert(dev_a != dev_b);

    /* Remove observations for both sources. */
    assert(XaveriusRegistryRemoveObservation(NULL, source, 1, obs_id) == XAVERIUS_REGISTRY_SUCCESS);
    assert(XaveriusRegistryRemoveObservation(NULL, source, 1, obs_id + 1) == XAVERIUS_REGISTRY_SUCCESS);
    assert(XaveriusRegistryRemoveObservation(NULL, source, 1, obs_id + 2) == XAVERIUS_REGISTRY_SUCCESS);

    /* Removed observations are gone: repeat removal returns NOT_FOUND. */
    assert(XaveriusRegistryRemoveObservation(NULL, source, 1, obs_id) == XAVERIUS_REGISTRY_NOT_FOUND);

    /* Ordered shutdown. */
    XaveriusPhysicalDeviceRelease(dev_a);
    XaveriusPhysicalDeviceRelease(dev_b);
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
        xaverius_registry_observe_and_remove,
        xaverius_registry_multi_source_observation,
        xaverius_registry_registration_contract,
        xaverius_registry_observation_edge_cases,
        NULL,
    };

    return testfuncs;
}
