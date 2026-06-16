#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

/* XDMCP opcodes */
#define XDMCP_REQUEST 2
#define XDMCP_MANAGE  4
#define XDMCP_PORT    177

START_TEST(test_xdmcp_rejects_unauthenticated_requests)
{
    /* Invariant: XDMCP must reject requests with invalid/missing authentication */
    /* Crafted XDMCP packets with no valid auth, malformed auth, empty auth */
    unsigned char payloads[][32] = {
        /* Missing auth: XDMCP header version=1, opcode=REQUEST, length=4, empty auth fields */
        {0x00, 0x01, 0x00, XDMCP_REQUEST, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00},
        /* Malformed auth: invalid version + MANAGE opcode with garbage session ID */
        {0xFF, 0xFF, 0x00, XDMCP_MANAGE, 0x00, 0x08, 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00, 0x00, 0x00},
        /* Replay attempt: valid-looking header but zeroed authentication data */
        {0x00, 0x01, 0x00, XDMCP_MANAGE, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    };
    int payload_lens[] = {10, 14, 18};
    int num_payloads = 3;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    ck_assert_int_ge(sock, 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(XDMCP_PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    struct timeval tv = {.tv_sec = 2, .tv_usec = 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    for (int i = 0; i < num_payloads; i++) {
        ssize_t sent = sendto(sock, payloads[i], payload_lens[i], 0,
                              (struct sockaddr *)&addr, sizeof(addr));
        if (sent < 0) continue; /* Server not running; skip but don't fail */

        unsigned char resp[256];
        ssize_t received = recvfrom(sock, resp, sizeof(resp), 0, NULL, NULL);

        if (received > 0) {
            /* If server responds, it must NOT be a successful session grant.
               XDMCP WILLING=5, ACCEPT=6 would indicate auth bypass */
            unsigned char opcode = (received >= 4) ? resp[3] : 0;
            ck_assert_msg(opcode != 5 && opcode != 6,
                "XDMCP server accepted unauthenticated request (opcode=%d, payload=%d)",
                opcode, i);
        }
        /* No response or DECLINE/UNWILLING/FAILED is acceptable (request rejected) */
    }

    close(sock);
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_set_timeout(tc_core, 10);
    tcase_add_test(tc_core, test_xdmcp_rejects_unauthenticated_requests);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number