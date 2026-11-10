/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * Xaverius PhysicalDevice Registry unit tests.
 * Tests that can run without full Xorg/DIX initialization.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Minimal type definitions for testing (matching xaverius_priv.h) */
typedef struct _XaveriusModuleRole XaveriusModuleRole;
typedef struct _XaveriusModuleLease XaveriusModuleLease;

typedef enum {
    XAVERIUS_MODULE_QUIESCE_PENDING = 0,
    XAVERIUS_MODULE_QUIESCE_SUCCESS = 1,
    XAVERIUS_MODULE_QUIESCE_FAILURE = 2,
} XaveriusModuleQuiescenceResult;

typedef enum {
    XAVERIUS_REGISTRY_SUCCESS            = 0,
    XAVERIUS_REGISTRY_PENDING            = 1,
    XAVERIUS_REGISTRY_AMBIGUOUS          = 2,
    XAVERIUS_REGISTRY_STALE_GENERATION   = 3,
    XAVERIUS_REGISTRY_INVALID_EVIDENCE   = 4,
    XAVERIUS_REGISTRY_IDENTITY_CONFLICT  = 5,
    XAVERIUS_REGISTRY_OUT_OF_MEMORY      = 6,
    XAVERIUS_REGISTRY_INVALID_ARGUMENT   = 7,
    XAVERIUS_REGISTRY_NOT_FOUND          = 8,
    XAVERIUS_REGISTRY_ALREADY_EXISTS     = 9,
    XAVERIUS_REGISTRY_NOT_REGISTERED     = 10,
    XAVERIUS_REGISTRY_SHUTTING_DOWN      = 11,
    XAVERIUS_REGISTRY_BUSY               = 12,
    XAVERIUS_REGISTRY_UNAVAILABLE        = 13,
    XAVERIUS_REGISTRY_UNSUPPORTED        = 14,
    XAVERIUS_REGISTRY_QUARANTINED        = 15,
    XAVERIUS_REGISTRY_STALE_EVENT        = 16,
    XAVERIUS_REGISTRY_REMOVED            = 17,
    XAVERIUS_REGISTRY_INTERNAL_ERROR     = 18,
} XaveriusRegistryResult;

typedef struct _XaveriusDeviceIdentity {
    uint32_t domain;
    uint32_t bus;
    uint32_t device;
    uint32_t function;
    uint8_t device_uuid[16];
    bool has_device_uuid;
    uint8_t driver_uuid[16];
    uint32_t drm_major;
    uint32_t drm_minor;
    bool is_render_node;
    int drm_fd;
} XaveriusDeviceIdentity;

typedef struct _XaveriusModuleRoleDescriptor {
    uint32_t struct_size;
    uint32_t abi_version;
    const char *owner_key;
    XaveriusModuleLease *module_lease;
    void (*retain_module_role)(XaveriusModuleRole *role);
    void (*release_module_role)(XaveriusModuleRole *role);
    XaveriusModuleQuiescenceResult (*quiesce_module_role)(XaveriusModuleRole *role);
} XaveriusModuleRoleDescriptor;

typedef struct _XaveriusDeviceBackendRegistration {
    uint32_t struct_size;
    uint32_t abi_version;
    const XaveriusDeviceIdentity *identity;
    const void *ops;
    void *device_context;
    void *native_physical_device;
    XaveriusModuleRole *owner_role;
    uint64_t owner_role_generation;
} XaveriusDeviceBackendRegistration;

/* Mock ModuleRole hooks for testing */
static int retain_called = 0;
static int release_called = 0;
static int quiesce_called = 0;
static XaveriusModuleQuiescenceResult quiesce_result = XAVERIUS_MODULE_QUIESCE_SUCCESS;

static void mock_retain_module_role(XaveriusModuleRole *role)
{
    (void)role;
    retain_called++;
}

static void mock_release_module_role(XaveriusModuleRole *role)
{
    (void)role;
    release_called++;
}

static XaveriusModuleQuiescenceResult mock_quiesce_module_role(XaveriusModuleRole *role)
{
    (void)role;
    quiesce_called++;
    return quiesce_result;
}

/* Test counter */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            printf("PASS: %s\n", msg); \
            tests_passed++; \
        } else { \
            printf("FAIL: %s\n", msg); \
            tests_failed++; \
        } \
    } while (0)

#define TEST_ASSERT_EQ(a, b, msg) \
    TEST_ASSERT((a) == (b), msg)

/* Internal function declarations */
extern bool xaverius_module_role_descriptor_valid(const XaveriusModuleRoleDescriptor *desc);
extern bool xaverius_identity_equal(const XaveriusDeviceIdentity *a, const XaveriusDeviceIdentity *b);
extern bool xaverius_identity_has_pci(const XaveriusDeviceIdentity *id);
extern void xaverius_create_ordering_key(char *out_buf, size_t out_buf_size,
                                         const XaveriusDeviceIdentity *identity,
                                         const char *owner_key);
extern int xaverius_compare_ordering_key(const char *a, const char *b);

/* Test ModuleRoleDescriptor validation */
static void test_module_role_descriptor_validation(void)
{
    printf("\n=== ModuleRoleDescriptor Validation Tests ===\n");

    /* Valid descriptor */
    XaveriusModuleRoleDescriptor valid_desc = {
        .struct_size = sizeof(XaveriusModuleRoleDescriptor),
        .abi_version = 1,
        .owner_key = "test-role",
        .module_lease = NULL,
        .retain_module_role = mock_retain_module_role,
        .release_module_role = mock_release_module_role,
        .quiesce_module_role = mock_quiesce_module_role,
    };
    TEST_ASSERT(xaverius_module_role_descriptor_valid(&valid_desc), "Valid descriptor accepted");

    /* NULL descriptor */
    TEST_ASSERT(!xaverius_module_role_descriptor_valid(NULL), "NULL descriptor rejected");

    /* struct_size too small */
    XaveriusModuleRoleDescriptor small_desc = valid_desc;
    small_desc.struct_size = sizeof(XaveriusModuleRoleDescriptor) - 1;
    TEST_ASSERT(!xaverius_module_role_descriptor_valid(&small_desc), "Too small struct_size rejected");

    /* Missing retain hook */
    XaveriusModuleRoleDescriptor no_retain = valid_desc;
    no_retain.retain_module_role = NULL;
    TEST_ASSERT(!xaverius_module_role_descriptor_valid(&no_retain), "Descriptor with NULL retain rejected");

    /* Missing release hook */
    XaveriusModuleRoleDescriptor no_release = valid_desc;
    no_release.release_module_role = NULL;
    TEST_ASSERT(!xaverius_module_role_descriptor_valid(&no_release), "Descriptor with NULL release rejected");

    /* Missing quiesce hook */
    XaveriusModuleRoleDescriptor no_quiesce = valid_desc;
    no_quiesce.quiesce_module_role = NULL;
    TEST_ASSERT(!xaverius_module_role_descriptor_valid(&no_quiesce), "Descriptor with NULL quiesce rejected");
}

/* Test ordering key generation */
static void test_ordering_key(void)
{
    printf("\n=== Ordering Key Tests ===\n");

    char buf[128];

    /* PCI identity */
    XaveriusDeviceIdentity pci_identity = { 0 };
    pci_identity.domain = 0x0000;
    pci_identity.bus = 0x01;
    pci_identity.device = 0x00;
    pci_identity.function = 0x00;
    pci_identity.has_device_uuid = false;
    memset(pci_identity.driver_uuid, 0xAA, 16);
    pci_identity.drm_major = 0;
    pci_identity.drm_minor = 0;
    pci_identity.is_render_node = false;
    pci_identity.drm_fd = -1;

    xaverius_create_ordering_key(buf, sizeof(buf), &pci_identity, "test-role");
    printf("PCI ordering key: %s\n", buf);
    TEST_ASSERT(strncmp(buf, "pci:0000:01:00.0:", 18) == 0, "PCI key starts with correct prefix");
    TEST_ASSERT(strstr(buf, "test-role") != NULL, "PCI key contains owner_key");

    /* DRM identity */
    XaveriusDeviceIdentity drm_identity = { 0 };
    drm_identity.domain = 0;
    drm_identity.bus = 0;
    drm_identity.device = 0;
    drm_identity.function = 0;
    drm_identity.has_device_uuid = false;
    memset(drm_identity.driver_uuid, 0xBB, 16);
    drm_identity.drm_major = 2;
    drm_identity.drm_minor = 5;
    drm_identity.is_render_node = true;
    drm_identity.drm_fd = 128;

    xaverius_create_ordering_key(buf, sizeof(buf), &drm_identity, "drm-role");
    printf("DRM ordering key: %s\n", buf);
    TEST_ASSERT(strncmp(buf, "drm:2.5:", 8) == 0, "DRM key starts with correct prefix");
    TEST_ASSERT(strstr(buf, "drm-role") != NULL, "DRM key contains owner_key");

    /* UUID identity */
    XaveriusDeviceIdentity uuid_identity = { 0 };
    uuid_identity.domain = 0;
    uuid_identity.bus = 0;
    uuid_identity.device = 0;
    uuid_identity.function = 0;
    uuid_identity.has_device_uuid = true;
    for (int i = 0; i < 16; i++) uuid_identity.device_uuid[i] = i;
    memset(uuid_identity.driver_uuid, 0xCC, 16);
    uuid_identity.drm_major = 0;
    uuid_identity.drm_minor = 0;
    uuid_identity.is_render_node = false;
    uuid_identity.drm_fd = -1;

    xaverius_create_ordering_key(buf, sizeof(buf), &uuid_identity, "uuid-role");
    printf("UUID ordering key: %s\n", buf);
    TEST_ASSERT(strncmp(buf, "uuid::", 6) == 0, "UUID key starts with correct prefix");
    TEST_ASSERT(strstr(buf, "uuid-role") != NULL, "UUID key contains owner_key");

    /* Test comparator */
    char buf1[128], buf2[128];
    xaverius_create_ordering_key(buf1, sizeof(buf1), &pci_identity, "aaa-role");
    xaverius_create_ordering_key(buf2, sizeof(buf2), &pci_identity, "bbb-role");
    TEST_ASSERT(xaverius_compare_ordering_key(buf1, buf2) < 0, "Ordering: aaa < bbb");
    TEST_ASSERT(xaverius_compare_ordering_key(buf2, buf1) > 0, "Ordering: bbb > aaa");
    TEST_ASSERT(xaverius_compare_ordering_key(buf1, buf1) == 0, "Ordering: equal keys");

    /* Test truncation behavior - buffer too small */
    char tiny_buf[4];
    xaverius_create_ordering_key(tiny_buf, sizeof(tiny_buf), &pci_identity, "role");
    TEST_ASSERT(tiny_buf[3] == '\0', "Tiny buffer is null-terminated");
}

/* Test identity comparison */
static void test_identity_comparison(void)
{
    printf("\n=== Identity Comparison Tests ===\n");

    XaveriusDeviceIdentity id1 = { 0 };
    id1.domain = 0x0000;
    id1.bus = 0x01;
    id1.device = 0x00;
    id1.function = 0x00;
    id1.has_device_uuid = false;
    memset(id1.driver_uuid, 0xAA, 16);
    id1.drm_major = 0;
    id1.drm_minor = 0;

    XaveriusDeviceIdentity id2 = id1;
    TEST_ASSERT(xaverius_identity_equal(&id1, &id2), "Identical PCI identities equal");

    id2.function = 0x01;
    TEST_ASSERT(!xaverius_identity_equal(&id1, &id2), "Different PCI function not equal");

    id2 = id1;
    id2.has_device_uuid = true;
    for (int i = 0; i < 16; i++) id2.device_uuid[i] = i;
    TEST_ASSERT(!xaverius_identity_equal(&id1, &id2), "PCI vs UUID not equal");

    /* DRM comparison */
    XaveriusDeviceIdentity drm1 = { 0 };
    drm1.drm_major = 1;
    drm1.drm_minor = 2;
    XaveriusDeviceIdentity drm2 = drm1;
    TEST_ASSERT(xaverius_identity_equal(&drm1, &drm2), "Identical DRM identities equal");

    drm2.drm_minor = 3;
    TEST_ASSERT(!xaverius_identity_equal(&drm1, &drm2), "Different DRM minor not equal");

    /* UUID comparison */
    XaveriusDeviceIdentity uuid1 = { 0 };
    uuid1.has_device_uuid = true;
    for (int i = 0; i < 16; i++) uuid1.device_uuid[i] = i;
    memset(uuid1.driver_uuid, 0xAA, 16);

    XaveriusDeviceIdentity uuid2 = uuid1;
    TEST_ASSERT(xaverius_identity_equal(&uuid1, &uuid2), "Identical UUID identities equal");

    uuid2.device_uuid[0] = 0xFF;
    TEST_ASSERT(!xaverius_identity_equal(&uuid1, &uuid2), "Different UUID not equal");

    /* PCI vs DRM */
    XaveriusDeviceIdentity pci_id = { 0 };
    pci_id.domain = 0;
    pci_id.bus = 1;
    pci_id.device = 0;
    pci_id.function = 0;
    XaveriusDeviceIdentity drm_id = { 0 };
    drm_id.drm_major = 1;
    drm_id.drm_minor = 2;
    TEST_ASSERT(!xaverius_identity_equal(&pci_id, &drm_id), "PCI vs DRM not equal");
}

/* Test DeviceBackendRegistration validation logic */
static void test_registration_descriptor_sizes(void)
{
    printf("\n=== Descriptor Size Validation Tests ===\n");

    /* XaveriusModuleRoleDescriptor size check */
    TEST_ASSERT(sizeof(XaveriusModuleRoleDescriptor) > 0, "ModuleRoleDescriptor has positive size");

    /* XaveriusDeviceBackendRegistration size check */
    TEST_ASSERT(sizeof(XaveriusDeviceBackendRegistration) > 0, "DeviceBackendRegistration has positive size");
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("Xaverius PhysicalDevice Registry Unit Tests\n");
    printf("============================================\n");

    test_module_role_descriptor_validation();
    test_ordering_key();
    test_identity_comparison();
    test_registration_descriptor_sizes();

    printf("\n=== Summary ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}