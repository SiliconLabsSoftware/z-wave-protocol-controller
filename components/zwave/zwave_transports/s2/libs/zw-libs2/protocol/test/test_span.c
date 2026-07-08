/* © 2025 Silicon Laboratories Inc.
 */
/**
 * @file test_span.c
 * @brief Test suite for SPAN (Singlecast Pre-Agreed Nonce) decryption functionality
 *
 * This test suite validates S2 SPAN-based decryption according to Z-Wave specification 2025A.
 * SPAN enables secure communication without per-message nonce handshakes by using a
 * pre-established nonce generator synchronized between sender and receiver.
 *
 * Key SPAN concepts tested:
 * - SPAN instantiation using Sender's Entropy Input (SEI) and Receiver's Entropy Input (REI)
 * - NextNonce generation using CTR_DRBG_Generate
 * - AES-128-CCM decryption with SPAN as IV (13 MSB bytes)
 * - SPAN retry iterations (1-5 attempts per spec requirement CC:009F.01.00.11.01C/D/E)
 * - SPAN synchronization states (NOT_USED, NO_SEQ, SOS, NEGOTIATED, etc.)
 * - AAD construction for authentication
 * - Security class iteration for SPAN_INSTANTIATE state
 *
 * This test uses the REAL S2 implementation (S2.c) and provides local stubs
 * for external callbacks that S2 requires (S2_msg_received_event, etc.)
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "unity.h"

// Include necessary headers for S2 SPAN testing
#include "S2.h"
#include "s2_protocol.h"
#include "S2_external.h"
#include "nextnonce.h"
#include "ccm.h"
#include "ctr_drbg.h"

/*******************************************************************************
 * External Function Stubs
 *
 * These are the external functions that the real S2 implementation (S2.c) calls.
 * We provide minimal stub implementations for testing purposes.
 ******************************************************************************/

// Test state to capture received decrypted messages
static uint8_t g_received_msg[256];
static uint16_t g_received_msg_len;
static s2_connection_t g_received_connection;
static bool g_msg_received;

// Test state to capture Nonce Report with SOS
static uint8_t g_nonce_report_buffer[32];
static uint16_t g_nonce_report_len;
static bool g_nonce_report_sent;
static uint8_t g_nonce_report_flags;

// Test state to capture resynchronization events
static bool g_resync_event_received;
static node_t g_resync_remote_node;
static sos_event_reason_t g_resync_reason;

/**
 * @brief Stub for S2_msg_received_event - captures decrypted payload
 *
 * This is called by S2_command_handler after successful decryption.
 * We capture the decrypted message for test verification.
 */
void S2_msg_received_event(struct S2 *ctxt, s2_connection_t *peer, uint8_t *buf, uint16_t len)
{
    if (len <= sizeof(g_received_msg)) {
        memcpy(g_received_msg, buf, len);
        g_received_msg_len = len;
        memcpy(&g_received_connection, peer, sizeof(s2_connection_t));
        g_msg_received = true;
    }
}

/**
 * @brief Stub for S2_send_done_event - transmission complete notification
 */
void S2_send_done_event(struct S2 *ctxt, s2_tx_status_t status)
{
    // Not needed for decryption tests
}

/**
 * @brief Stub for S2_send_frame - send raw frame
 */
uint8_t S2_send_frame(struct S2 *ctxt, const s2_connection_t *peer, uint8_t *buf, uint16_t len)
{
    return 1;  // Success
}

/**
 * @brief Stub for S2_send_frame_no_cb - send without callback
 *
 * This captures Nonce Reports with SOS flag for test verification.
 */
uint8_t S2_send_frame_no_cb(struct S2 *ctxt, const s2_connection_t *peer, uint8_t *buf, uint16_t len)
{
    // Check if this is a Nonce Report (CC=0x9F, CMD=0x02)
    if (len >= 4 && buf[0] == 0x9F && buf[1] == 0x02) {
        memcpy(g_nonce_report_buffer, buf, len);
        g_nonce_report_len   = len;
        g_nonce_report_flags = buf[3];
        g_nonce_report_sent  = true;
    }
    return 1;  // Success
}

/**
 * @brief Stub for S2_send_frame_multi - multicast send
 */
uint8_t S2_send_frame_multi(struct S2 *ctxt, s2_connection_t *peer, uint8_t *buf, uint16_t len)
{
    return 1;  // Success
}

/**
 * @brief Stub for S2_set_timeout - timer management
 */
void S2_set_timeout(struct S2 *ctxt, uint32_t interval)
{
    // Not needed for single-message tests
}

/**
 * @brief Stub for S2_stop_timeout - timer management
 */
void S2_stop_timeout(struct S2 *ctxt)
{
    // Not needed for single-message tests
}

/**
 * @brief Stub for S2_get_hw_random - hardware random
 */
void S2_get_hw_random(uint8_t *buf, uint8_t len)
{
    // Provide deterministic "random" bytes for testing
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = i + 0x42;
    }
}

/**
 * @brief Stub for S2_get_commands_supported - command class list
 */
void S2_get_commands_supported(node_t lnode, uint8_t class_id, const uint8_t **cmdClasses, uint8_t *length)
{
    static const uint8_t empty_list[] = {};
    *cmdClasses                       = empty_list;
    *length                           = 0;
}

/**
 * @brief Stub for S2_notify_nls_state_report - NLS notification
 */
void S2_notify_nls_state_report(node_t srcNode, uint8_t class_id, bool nls_capability, bool nls_state)
{
    // Not needed for basic SPAN tests
}

/**
 * @brief Stub for S2_get_nls_node_list - NLS node list
 */
int8_t S2_get_nls_node_list(node_t srcNode, bool request, bool *is_last_node, uint16_t *node_id, uint8_t *granted_keys, bool *nls_state)
{
    return -1;  // Not implemented
}

/**
 * @brief Stub for S2_notify_nls_node_list_report - NLS list report
 */
int8_t S2_notify_nls_node_list_report(node_t srcNode, uint16_t id_of_node, uint8_t keys_node_bitmask, bool nls_state)
{
    return 0;
}

/**
 * @brief Stub for S2_resynchronization_event - SOS event notification
 *
 * Captures resynchronization events triggered by failed decryption.
 */
void S2_resynchronization_event(node_t remote_node, sos_event_reason_t reason, uint8_t seqno, node_t local_node)
{
    g_resync_event_received = true;
    g_resync_remote_node    = remote_node;
    g_resync_reason         = reason;
}

/**
 * @brief Stub for S2_save_nls_state - NLS persistence
 */
void S2_save_nls_state(void)
{
    // Not needed for basic SPAN tests
}

/**
 * @brief Stub for clock_time - timer tick
 */
uint32_t clock_time(void)
{
    return 0;
}

/*******************************************************************************
 * Test Helper Functions
 ******************************************************************************/

/**
 * @brief Reset test state before each test
 */
static void reset_test_state(void)
{
    memset(g_received_msg, 0, sizeof(g_received_msg));
    g_received_msg_len = 0;
    memset(&g_received_connection, 0, sizeof(g_received_connection));
    g_msg_received = false;

    memset(g_nonce_report_buffer, 0, sizeof(g_nonce_report_buffer));
    g_nonce_report_len   = 0;
    g_nonce_report_sent  = false;
    g_nonce_report_flags = 0;

    g_resync_event_received = false;
    g_resync_remote_node    = 0;
    g_resync_reason         = 0;
}

/*******************************************************************************
 * Test Setup and Teardown
 ******************************************************************************/

void setUp(void)
{
    reset_test_state();
}

void tearDown(void)
{
    // Called after each test
}

/*******************************************************************************
 * Test Data - Static test vectors
 ******************************************************************************/

// Test entropy inputs for SPAN instantiation
static uint8_t test_sei_valid[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

static uint8_t test_rei_valid[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};

// Test nonce key (32 bytes) for SPAN instantiation
static uint8_t test_nonce_key[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};

// Edge case: zero entropy (all zeros)
static uint8_t test_sei_zero[16] = {0};
static uint8_t test_rei_zero[16] = {0};

// Edge case: maximum entropy (all 0xFF)
static uint8_t test_sei_max[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static uint8_t test_rei_max[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/*******************************************************************************
 * Test Function Prototypes - SPAN Instantiation & Synchronization
 ******************************************************************************/

/**
 * @brief Test SPAN instantiation with valid SEI and REI
 *
 * Validates:
 * - NextNonce generator instantiation with 32-byte entropy (SEI + REI)
 * - Proper storage of inner SPAN state in SPAN table
 * - Transition from SPAN_SOS_LOCAL_NONCE to SPAN_INSTANTIATE state
 * - SPAN table entry creation with correct node IDs
 *
 * Spec reference: Z-Wave Spec 2025A, Page 881, Step 4.b
 */
void test_span_instantiation_valid_entropy(void)
{
    CTR_DRBG_CTX span_rng_1;
    CTR_DRBG_CTX span_rng_2;
    uint8_t nonce_1[16];
    uint8_t nonce_2[16];

    // Test 1: Instantiate SPAN generator with valid entropy inputs
    // This simulates the receiver creating a SPAN generator from SEI and REI
    next_nonce_instantiate(&span_rng_1, test_sei_valid, test_rei_valid, test_nonce_key);

    // Test 2: Generate a nonce from the instantiated generator
    // This validates that the DRBG is properly initialized
    int result = next_nonce_generate(&span_rng_1, nonce_1);
    TEST_ASSERT_EQUAL_INT(1, result);

    // Test 3: Verify that the same inputs produce the same initial DRBG state
    // by instantiating a second generator with identical inputs
    next_nonce_instantiate(&span_rng_2, test_sei_valid, test_rei_valid, test_nonce_key);

    // Test 4: Generate nonce from second generator
    result = next_nonce_generate(&span_rng_2, nonce_2);
    TEST_ASSERT_EQUAL_INT(1, result);

    // Test 5: Verify both generators produce identical first nonce
    // This confirms deterministic behavior required for SPAN synchronization
    TEST_ASSERT_EQUAL_UINT8_ARRAY(nonce_1, nonce_2, 16);

    // Test 6: Generate second nonce from first generator
    uint8_t nonce_1_second[16];
    result = next_nonce_generate(&span_rng_1, nonce_1_second);
    TEST_ASSERT_EQUAL_INT(1, result);

    // Test 7: Verify second nonce is different from first (uniqueness)
    // This ensures the DRBG advances its internal state
    int nonces_identical = (memcmp(nonce_1, nonce_1_second, 16) == 0);
    TEST_ASSERT_FALSE(nonces_identical);

    // Test 8: Verify the nonce is non-zero (basic sanity check)
    uint8_t zero_nonce[16] = {0};
    int nonce_is_zero      = (memcmp(nonce_1, zero_nonce, 16) == 0);
    TEST_ASSERT_FALSE(nonce_is_zero);
}

/**
 * @brief Test SPAN instantiation with zero entropy inputs
 *
 * Validates:
 * - System behavior with edge case entropy (all zeros)
 * - CTR_DRBG instantiation with minimal entropy
 * - SPAN state machine handling of degenerate case
 */
void test_span_instantiation_zero_entropy(void)
{
    CTR_DRBG_CTX span_rng_1;
    CTR_DRBG_CTX span_rng_2;
    uint8_t nonce_1[16];
    uint8_t nonce_2[16];

    // Test 1: Instantiate SPAN generator with zero entropy inputs
    // Even with all-zero entropy, the system should still produce valid output
    // due to the nonce key providing personalization
    next_nonce_instantiate(&span_rng_1, test_sei_zero, test_rei_zero, test_nonce_key);

    // Test 2: Generate a nonce from the zero-entropy generator
    int result = next_nonce_generate(&span_rng_1, nonce_1);
    TEST_ASSERT_EQUAL_INT(1, result);

    // Test 3: Verify output is non-zero (nonce_key should provide entropy)
    // Even with zero EI, the personalization string (nonce_key) ensures non-zero output
    uint8_t zero_nonce[16] = {0};
    int nonce_is_zero      = (memcmp(nonce_1, zero_nonce, 16) == 0);
    TEST_ASSERT_FALSE(nonce_is_zero);

    // Test 4: Verify deterministic behavior with zero entropy
    // Same zero inputs should produce same output
    next_nonce_instantiate(&span_rng_2, test_sei_zero, test_rei_zero, test_nonce_key);
    result = next_nonce_generate(&span_rng_2, nonce_2);
    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(nonce_1, nonce_2, 16);

    // Test 5: Verify second nonce is different from first
    uint8_t nonce_1_second[16];
    result = next_nonce_generate(&span_rng_1, nonce_1_second);
    TEST_ASSERT_EQUAL_INT(1, result);
    int nonces_identical = (memcmp(nonce_1, nonce_1_second, 16) == 0);
    TEST_ASSERT_FALSE(nonces_identical);
}

/**
 * @brief Test SPAN instantiation with maximum entropy values
 *
 * Validates:
 * - System behavior with maximum entropy (all 0xFF)
 * - Proper handling of edge case entropy values
 * - No overflow or wrap-around issues
 */
void test_span_instantiation_max_entropy(void)
{
    CTR_DRBG_CTX span_rng_1;
    CTR_DRBG_CTX span_rng_2;
    uint8_t nonce_1[16];
    uint8_t nonce_2[16];

    // Test 1: Instantiate SPAN generator with maximum entropy inputs (all 0xFF)
    // This tests the upper boundary condition for entropy values
    next_nonce_instantiate(&span_rng_1, test_sei_max, test_rei_max, test_nonce_key);

    // Test 2: Generate a nonce from the max-entropy generator
    int result = next_nonce_generate(&span_rng_1, nonce_1);
    TEST_ASSERT_EQUAL_INT(1, result);

    // Test 3: Verify output is not all 0xFF (proper mixing occurred)
    // The CKDF and DRBG operations should produce well-distributed output
    uint8_t max_nonce[16];
    memset(max_nonce, 0xFF, 16);
    int nonce_is_max = (memcmp(nonce_1, max_nonce, 16) == 0);
    TEST_ASSERT_FALSE(nonce_is_max);

    // Test 4: Verify output is not all zeros
    uint8_t zero_nonce[16] = {0};
    int nonce_is_zero      = (memcmp(nonce_1, zero_nonce, 16) == 0);
    TEST_ASSERT_FALSE(nonce_is_zero);

    // Test 5: Verify deterministic behavior with max entropy
    // Same maximum inputs should produce same output
    next_nonce_instantiate(&span_rng_2, test_sei_max, test_rei_max, test_nonce_key);
    result = next_nonce_generate(&span_rng_2, nonce_2);
    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(nonce_1, nonce_2, 16);

    // Test 6: Verify second nonce is different from first (no wrap-around issues)
    uint8_t nonce_1_second[16];
    result = next_nonce_generate(&span_rng_1, nonce_1_second);
    TEST_ASSERT_EQUAL_INT(1, result);
    int nonces_identical = (memcmp(nonce_1, nonce_1_second, 16) == 0);
    TEST_ASSERT_FALSE(nonces_identical);

    // Test 7: Verify no overflow - generate multiple nonces
    // If there were overflow issues, this would likely expose them
    uint8_t nonce_prev[16];
    memcpy(nonce_prev, nonce_1_second, 16);
    for (int i = 0; i < 10; i++) {
        uint8_t nonce_next[16];
        result = next_nonce_generate(&span_rng_1, nonce_next);
        TEST_ASSERT_EQUAL_INT(1, result);

        // Each nonce should be different
        int same = (memcmp(nonce_prev, nonce_next, 16) == 0);
        TEST_ASSERT_FALSE(same);

        memcpy(nonce_prev, nonce_next, 16);
    }
}

/*******************************************************************************
 * Test Function Prototypes - NextNonce Generation
 ******************************************************************************/

/**
 * @brief Test NextNonce generation sequence reproducibility
 *
 * Validates:
 * - Same initial entropy produces same NextNonce sequence
 * - Deterministic behavior of CTR_DRBG
 * - SPAN synchronization can be maintained between nodes
 */
void test_nextnonce_sequence_reproducible(void)
{
    CTR_DRBG_CTX span_rng_1;
    CTR_DRBG_CTX span_rng_2;
    uint8_t nonce_seq_1[5][16];  // Store 5 nonces from first generator
    uint8_t nonce_seq_2[5][16];  // Store 5 nonces from second generator

    // Test 1: Instantiate first SPAN generator
    next_nonce_instantiate(&span_rng_1, test_sei_valid, test_rei_valid, test_nonce_key);

    // Test 2: Generate sequence of 5 nonces from first generator
    for (int i = 0; i < 5; i++) {
        int result = next_nonce_generate(&span_rng_1, nonce_seq_1[i]);
        TEST_ASSERT_EQUAL_INT(1, result);
    }

    // Test 3: Instantiate second SPAN generator with identical inputs
    next_nonce_instantiate(&span_rng_2, test_sei_valid, test_rei_valid, test_nonce_key);

    // Test 4: Generate sequence of 5 nonces from second generator
    for (int i = 0; i < 5; i++) {
        int result = next_nonce_generate(&span_rng_2, nonce_seq_2[i]);
        TEST_ASSERT_EQUAL_INT(1, result);
    }

    // Test 5: Verify both generators produce identical sequences
    // This confirms deterministic behavior required for SPAN synchronization
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL_UINT8_ARRAY(nonce_seq_1[i], nonce_seq_2[i], 16);
    }

    // Test 6: Verify nonces within each sequence are unique
    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 5; j++) {
            int nonces_identical = (memcmp(nonce_seq_1[i], nonce_seq_1[j], 16) == 0);
            TEST_ASSERT_FALSE(nonces_identical);
        }
    }

    // Test 7: Continue generating from first generator and verify determinism
    // This simulates SPAN advancing after initial synchronization
    uint8_t nonce_1_next[16];
    uint8_t nonce_2_next[16];

    int result = next_nonce_generate(&span_rng_1, nonce_1_next);
    TEST_ASSERT_EQUAL_INT(1, result);

    result = next_nonce_generate(&span_rng_2, nonce_2_next);
    TEST_ASSERT_EQUAL_INT(1, result);

    // Both generators should still be synchronized
    TEST_ASSERT_EQUAL_UINT8_ARRAY(nonce_1_next, nonce_2_next, 16);
}

/*******************************************************************************
 * Test Function Prototypes - SPAN Decryption with Retry
 ******************************************************************************/

/**
 * @brief Test successful decryption through S2_application_command_handler
 *
 * This test validates the complete decryption flow through the real S2 implementation:
 * - Initialize S2 context with encryption keys and SPAN in NEGOTIATED state
 * - Construct a valid S2_MESSAGE_ENCAPSULATION frame with encrypted payload
 * - Call S2_application_command_handler (the real entry point)
 * - Verify decrypted payload delivered via S2_msg_received_event callback
 * - Verify SPAN state correctly advances after successful decryption
 *
 * Spec reference: Z-Wave Spec 2025A, Page 881, Steps 3-5
 */
void test_span_decrypt_success_first_attempt(void)
{
    struct S2 s2_context;
    s2_connection_t connection;
    uint8_t frame_buffer[128];
    uint8_t plaintext[] = {0x25, 0x03, 0xFF};  // Binary Switch Report: ON
    uint8_t encryption_key[16];
    uint8_t nonce_key[32];
    uint8_t sender_nonce[16];
    uint8_t aad[64];
    uint16_t aad_len;
    uint16_t ciphertext_len;
    uint16_t hdr_len;

    // Constants for test
    const node_t LOCAL_NODE_ID  = 1;
    const node_t REMOTE_NODE_ID = 2;
    const uint32_t HOME_ID      = 0x12345678;
    const uint8_t SEQUENCE_NUM  = 0x01;

    // Step 1: Initialize S2 context
    memset(&s2_context, 0, sizeof(struct S2));
    memset(&connection, 0, sizeof(s2_connection_t));

    // Step 2: Set up encryption key (Ke) and nonce key (Knonce)
    // Use test_nonce_key as the nonce key for SPAN derivation
    memcpy(nonce_key, test_nonce_key, 32);
    // Use first 16 bytes of nonce key as encryption key for simplicity
    memcpy(encryption_key, test_nonce_key, 16);

    // Step 3: Configure security group 0 (S2 Unauthenticated) in S2 context
    memcpy(s2_context.sg[0].enc_key, encryption_key, 16);
    memcpy(s2_context.sg[0].nonce_key, nonce_key, 32);
    s2_context.loaded_keys = 0x01;  // Security class 0 loaded
    s2_context.my_home_id  = HOME_ID;
    s2_context.fsm         = IDLE;

    // Step 4: Set up SPAN in NEGOTIATED state (simulates established connection)
    struct SPAN *span = &s2_context.span_table[0];
    span->lnode       = LOCAL_NODE_ID;
    span->rnode       = REMOTE_NODE_ID;
    span->class_id    = 0;  // Security class 0
    span->state       = SPAN_NEGOTIATED;
    span->rx_seq      = 0;
    span->tx_seq      = 0;

    // Step 5: Instantiate SPAN RNG with known entropy (same as sender would use)
    // The sender and receiver must be synchronized for decryption to succeed
    next_nonce_instantiate(&span->d.rng, test_sei_valid, test_rei_valid, nonce_key);

    // Step 6: Generate SPAN nonce (sender's perspective - we're simulating received message)
    // Save RNG state to generate same nonce the receiver will use
    CTR_DRBG_CTX sender_rng;
    memcpy(&sender_rng, &span->d.rng, sizeof(CTR_DRBG_CTX));
    next_nonce_generate(&sender_rng, sender_nonce);

    // Step 7: Build S2_MESSAGE_ENCAPSULATION frame header
    // Frame format: [CC_SECURITY_2, MSG_ENCAP, seq_props, seq_num, <encrypted_payload>]
    frame_buffer[0] = 0x9F;  // COMMAND_CLASS_SECURITY_2
    frame_buffer[1] = 0x03;  // SECURITY_2_MESSAGE_ENCAPSULATION
    frame_buffer[2] = SEQUENCE_NUM;
    frame_buffer[3] = 0x00;  // Properties: no extensions
    hdr_len         = 4;

    // Step 8: Construct AAD matching S2_make_aad() logic
    // AAD = [sender(1B), receiver(1B), HomeID(4B), msg_len(2B), header_bytes...]
    uint32_t i             = 0;
    uint16_t total_msg_len = hdr_len + sizeof(plaintext) + 8;  // header + plaintext + auth_tag

    aad[i++] = REMOTE_NODE_ID & 0xFF;  // Sender (remote node)
    aad[i++] = LOCAL_NODE_ID & 0xFF;   // Receiver (local node)
    aad[i++] = (HOME_ID >> 24) & 0xFF;
    aad[i++] = (HOME_ID >> 16) & 0xFF;
    aad[i++] = (HOME_ID >> 8) & 0xFF;
    aad[i++] = HOME_ID & 0xFF;
    aad[i++] = (total_msg_len >> 8) & 0xFF;
    aad[i++] = total_msg_len & 0xFF;
    // Copy header bytes starting from index 2 (skip CC and CMD)
    memcpy(&aad[i], &frame_buffer[2], hdr_len - 2);
    aad_len = i + (hdr_len - 2);

    // Step 9: Encrypt plaintext with CCM using SPAN nonce
    memcpy(&frame_buffer[hdr_len], plaintext, sizeof(plaintext));
    ciphertext_len = CCM_encrypt_and_auth(encryption_key, sender_nonce, aad, aad_len, &frame_buffer[hdr_len], sizeof(plaintext));
    TEST_ASSERT_GREATER_THAN(0, ciphertext_len);

    // Total frame length: header + ciphertext (includes auth tag)
    uint16_t frame_len = hdr_len + ciphertext_len;
    TEST_ASSERT_EQUAL_UINT16(total_msg_len, frame_len);

    // Step 10: Set up connection parameters for receiving
    connection.l_node     = LOCAL_NODE_ID;
    connection.r_node     = REMOTE_NODE_ID;
    connection.rx_options = 0;     // Unicast
    connection.class_id   = 0xFF;  // Unknown before decryption

    // Step 11: Call the REAL S2_application_command_handler
    // This exercises the complete decryption path through S2.c
    S2_application_command_handler(&s2_context, &connection, frame_buffer, frame_len);

    // Step 12: Verify decrypted message was received via S2_msg_received_event callback
    TEST_ASSERT_TRUE_MESSAGE(g_msg_received, "S2_msg_received_event was not called - decryption may have failed");
    TEST_ASSERT_EQUAL_UINT16(sizeof(plaintext), g_received_msg_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(plaintext, g_received_msg, sizeof(plaintext));

    // Step 13: Verify connection class_id was set correctly
    TEST_ASSERT_EQUAL_UINT8(0, g_received_connection.class_id);  // Security class 0

    // Step 14: Verify SPAN state remains NEGOTIATED after successful decryption
    TEST_ASSERT_EQUAL(SPAN_NEGOTIATED, span->state);
}

/**
 * @brief Test decryption failure requiring SPAN retry
 *
 * Validates:
 * - Receiver tries current SPAN first (fails authentication)
 * - Receiver advances SPAN and retries (per CC:009F.01.00.11.01C)
 * - Decryption succeeds on second iteration
 * - SPAN state preserved at successful value
 * - Maximum iteration limit enforced (1-5 attempts)
 *
 * Spec reference: Z-Wave Spec 2025A, Page 881, Step 4.a
 *
 * Test scenario:
 * - Sender and receiver start with synchronized SPAN
 * - Sender advances SPAN by 1 (simulating a missed message)
 * - Sender encrypts with nonce N+1
 * - Receiver has nonce N, tries it first (fails)
 * - Receiver advances to N+1 and retries (succeeds)
 */
void test_span_decrypt_with_retry_sync_recovery(void)
{
    struct S2 s2_context;
    s2_connection_t connection;
    uint8_t frame_buffer[128];
    uint8_t plaintext[] = {0x25, 0x03, 0x00};  // Binary Switch Report: OFF
    uint8_t encryption_key[16];
    uint8_t nonce_key[32];
    uint8_t sender_nonce[16];
    uint8_t aad[64];
    uint16_t aad_len;
    uint16_t ciphertext_len;
    uint16_t hdr_len;

    // Constants for test
    const node_t LOCAL_NODE_ID  = 1;
    const node_t REMOTE_NODE_ID = 2;
    const uint32_t HOME_ID      = 0xAABBCCDD;
    const uint8_t SEQUENCE_NUM  = 0x02;

    // Step 1: Initialize S2 context
    memset(&s2_context, 0, sizeof(struct S2));
    memset(&connection, 0, sizeof(s2_connection_t));

    // Step 2: Set up encryption key (Ke) and nonce key (Knonce)
    memcpy(nonce_key, test_nonce_key, 32);
    memcpy(encryption_key, test_nonce_key, 16);

    // Step 3: Configure security group 0 (S2 Unauthenticated) in S2 context
    memcpy(s2_context.sg[0].enc_key, encryption_key, 16);
    memcpy(s2_context.sg[0].nonce_key, nonce_key, 32);
    s2_context.loaded_keys = 0x01;  // Security class 0 loaded
    s2_context.my_home_id  = HOME_ID;
    s2_context.fsm         = IDLE;

    // Step 4: Set up SPAN in NEGOTIATED state
    struct SPAN *span = &s2_context.span_table[0];
    span->lnode       = LOCAL_NODE_ID;
    span->rnode       = REMOTE_NODE_ID;
    span->class_id    = 0;  // Security class 0
    span->state       = SPAN_NEGOTIATED;
    span->rx_seq      = 0;
    span->tx_seq      = 0;

    // Step 5: Instantiate SPAN RNG for receiver (this is the receiver's current state)
    next_nonce_instantiate(&span->d.rng, test_sei_valid, test_rei_valid, nonce_key);

    // Step 6: Create sender's RNG starting from same state
    CTR_DRBG_CTX sender_rng;
    next_nonce_instantiate(&sender_rng, test_sei_valid, test_rei_valid, nonce_key);

    // Step 7: CRITICAL - Advance sender by 1 nonce to simulate desync
    // This simulates a message that was sent but not received by us
    uint8_t discarded_nonce[16];
    next_nonce_generate(&sender_rng, discarded_nonce);  // Sender used this for a message we missed

    // Step 8: Sender generates the nonce for this message (N+1)
    next_nonce_generate(&sender_rng, sender_nonce);

    // At this point:
    // - Receiver's RNG will generate nonce N on first attempt
    // - Sender encrypted with nonce N+1
    // - Receiver should fail with N, then retry with N+1 and succeed

    // Step 9: Build S2_MESSAGE_ENCAPSULATION frame header
    frame_buffer[0] = 0x9F;  // COMMAND_CLASS_SECURITY_2
    frame_buffer[1] = 0x03;  // SECURITY_2_MESSAGE_ENCAPSULATION
    frame_buffer[2] = SEQUENCE_NUM;
    frame_buffer[3] = 0x00;  // Properties: no extensions
    hdr_len         = 4;

    // Step 10: Construct AAD matching S2_make_aad() logic
    uint32_t i             = 0;
    uint16_t total_msg_len = hdr_len + sizeof(plaintext) + 8;  // header + plaintext + auth_tag

    aad[i++] = REMOTE_NODE_ID & 0xFF;  // Sender (remote node)
    aad[i++] = LOCAL_NODE_ID & 0xFF;   // Receiver (local node)
    aad[i++] = (HOME_ID >> 24) & 0xFF;
    aad[i++] = (HOME_ID >> 16) & 0xFF;
    aad[i++] = (HOME_ID >> 8) & 0xFF;
    aad[i++] = HOME_ID & 0xFF;
    aad[i++] = (total_msg_len >> 8) & 0xFF;
    aad[i++] = total_msg_len & 0xFF;
    memcpy(&aad[i], &frame_buffer[2], hdr_len - 2);
    aad_len = i + (hdr_len - 2);

    // Step 11: Encrypt plaintext with CCM using sender's nonce (N+1)
    memcpy(&frame_buffer[hdr_len], plaintext, sizeof(plaintext));
    ciphertext_len = CCM_encrypt_and_auth(encryption_key,
                                          sender_nonce,  // This is nonce N+1
                                          aad,
                                          aad_len,
                                          &frame_buffer[hdr_len],
                                          sizeof(plaintext));
    TEST_ASSERT_GREATER_THAN(0, ciphertext_len);

    uint16_t frame_len = hdr_len + ciphertext_len;
    TEST_ASSERT_EQUAL_UINT16(total_msg_len, frame_len);

    // Step 12: Set up connection parameters for receiving
    connection.l_node     = LOCAL_NODE_ID;
    connection.r_node     = REMOTE_NODE_ID;
    connection.rx_options = 0;     // Unicast
    connection.class_id   = 0xFF;  // Unknown before decryption

    // Step 13: Call S2_application_command_handler
    // The receiver will:
    // 1. Try nonce N (fail - wrong nonce)
    // 2. Advance RNG and try nonce N+1 (succeed!)
    S2_application_command_handler(&s2_context, &connection, frame_buffer, frame_len);

    // Step 14: Verify decryption succeeded via retry
    TEST_ASSERT_TRUE_MESSAGE(g_msg_received, "S2_msg_received_event was not called - SPAN retry may have failed");
    TEST_ASSERT_EQUAL_UINT16(sizeof(plaintext), g_received_msg_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(plaintext, g_received_msg, sizeof(plaintext));

    // Step 15: Verify security class correctly identified
    TEST_ASSERT_EQUAL_UINT8(0, g_received_connection.class_id);

    // Step 16: Verify SPAN state remains NEGOTIATED
    TEST_ASSERT_EQUAL(SPAN_NEGOTIATED, span->state);

    // Step 17: Verify RNG is now synchronized at the correct position
    // After successful decryption, the receiver's RNG should be at N+2
    // (advanced past the nonce that was used for decryption)
    uint8_t receiver_next_nonce[16];
    uint8_t sender_next_nonce[16];

    next_nonce_generate(&span->d.rng, receiver_next_nonce);
    next_nonce_generate(&sender_rng, sender_next_nonce);

    // Both should now be synchronized at the same position
    TEST_ASSERT_EQUAL_UINT8_ARRAY(sender_next_nonce, receiver_next_nonce, 16);
}

// /**
//  * @brief Test SPAN retry with 2-iteration offset
//  *
//  * Validates:
//  * - Receiver can recover when sender is 2 SPAN values ahead
//  * - Multiple NextNonce calls in retry loop
//  * - Authentication succeeds on correct iteration
//  */
// void test_span_decrypt_retry_two_iteration_offset(void);

// /**
//  * @brief Test SPAN retry with maximum iterations (5)
//  *
//  * Validates:
//  * - All 5 retry iterations attempted
//  * - Decryption succeeds on 5th attempt
//  * - SPAN RNG state correctly advanced 5 times
//  * - No over-iteration beyond MAX_SPAN_RETRY_ITERATIONS
//  *
//  * Spec reference: CC:009F.01.00.11.01D - "maximum number of iterations
//  *                 performed by a receiving node MUST be in the range 1..5"
//  */
// void test_span_decrypt_retry_max_iterations(void);

// /**
//  * @brief Test SPAN retry failure after max iterations
//  *
//  * Validates:
//  * - Decryption fails after 5 unsuccessful retry attempts
//  * - Nonce Report with SOS flag must be sent (per CC:009F.01.00.11.01E)
//  * - SPAN table entry invalidated for sender NodeID
//  * - System transitions to resynchronization flow
//  *
//  * Spec reference: CC:009F.01.00.11.01E
//  */
// void test_span_decrypt_retry_exceeds_max_iterations(void);

// /**
//  * @brief Test SPAN retry with RNG state backup and restore
//  *
//  * Validates:
//  * - RNG state saved before retry attempts
//  * - RNG state restored if all retries fail
//  * - Prevents SPAN desynchronization on failed decryption
//  * - Next message can retry from correct SPAN value
//  */
// void test_span_decrypt_retry_rng_state_preservation(void);

// /*******************************************************************************
//  * Test Function Prototypes - SPAN Extension Processing
//  ******************************************************************************/

// /**
//  * @brief Test SPAN Extension (SN) processing for new SPAN setup
//  *
//  * Validates:
//  * - SN extension contains 16-byte Sender's Entropy Input
//  * - Extension length is 18 bytes (2 + 16)
//  * - Critical flag set on SN extension
//  * - SPAN instantiation triggered with SEI + locally stored REI
//  *
//  * Spec reference: Z-Wave Spec 2025A, Page 881, Step 4.b
//  */
// void test_span_extension_sn_processing(void);

// /**
//  * @brief Test SPAN Extension with incorrect length rejected
//  *
//  * Validates:
//  * - Extension length must be exactly 18 bytes (2 header + 16 SEI)
//  * - Parse failure if length incorrect
//  * - System security not compromised by malformed extensions
//  */
// void test_span_extension_sn_invalid_length(void);

// /*******************************************************************************
//  * Test Function Prototypes - Edge Cases & Error Handling
//  ******************************************************************************/

// /**
//  * @brief Test decryption with minimum message length
//  *
//  * Validates:
//  * - Minimum valid length: header (4 bytes) + auth tag (8 bytes) = 12 bytes
//  * - Empty payload (0-byte plaintext) handled correctly
//  * - Parse failure if message shorter than minimum
//  */
// void test_span_decrypt_minimum_message_length(void);

// /**
//  * @brief Test decryption with maximum message length
//  *
//  * Validates:
//  * - Maximum message length per Z-Wave specification
//  * - Large payloads decrypted correctly
//  * - No buffer overflow
//  */
// void test_span_decrypt_maximum_message_length(void);

// /**
//  * @brief Test decryption with corrupted message header
//  *
//  * Validates:
//  * - Invalid command class byte rejected
//  * - Invalid command byte rejected
//  * - PARSE_FAIL return code
//  */
// void test_span_decrypt_corrupted_header(void);

// /*******************************************************************************
//  * Test Function Prototypes - Integration Tests
//  ******************************************************************************/

/**
 * @brief Test complete S2 message reception flow with different plaintext
 *
 * This is a secondary integration test using Basic Get command.
 * Uses the correct AAD format matching S2_make_aad().
 */
void test_span_message_reception_complete_flow(void)
{
    struct S2 s2_context;
    s2_connection_t connection;
    uint8_t frame_buffer[128];
    uint8_t plaintext[] = {0x20, 0x01, 0xFF};  // Basic Get command
    uint8_t encryption_key[16];
    uint8_t nonce_key[32];
    uint8_t nonce[16];
    uint8_t aad[64];
    uint16_t aad_len;
    uint16_t ciphertext_len;
    uint16_t hdr_len;

    // Constants
    const node_t LOCAL_NODE_ID  = 1;
    const node_t REMOTE_NODE_ID = 2;
    const uint32_t HOME_ID      = 0x12345678;
    const uint8_t SEQUENCE_NUM  = 0x01;

    // Reset test state
    reset_test_state();

    // Step 1: Initialize S2 context
    memset(&s2_context, 0, sizeof(struct S2));
    memset(&connection, 0, sizeof(s2_connection_t));

    // Step 2: Set up encryption key and nonce key
    memcpy(encryption_key, test_nonce_key, 16);
    memcpy(nonce_key, test_nonce_key, 32);

    // Configure security group 0 (S2 Unauthenticated)
    memcpy(s2_context.sg[0].enc_key, encryption_key, 16);
    memcpy(s2_context.sg[0].nonce_key, nonce_key, 32);
    s2_context.loaded_keys = 0x01;  // Security class 0 loaded
    s2_context.my_home_id  = HOME_ID;
    s2_context.fsm         = IDLE;

    // Step 3: Set up SPAN in NEGOTIATED state
    struct SPAN *span = &s2_context.span_table[0];
    span->lnode       = LOCAL_NODE_ID;
    span->rnode       = REMOTE_NODE_ID;
    span->class_id    = 0;  // Security class 0
    span->state       = SPAN_NEGOTIATED;
    span->rx_seq      = 0;
    span->tx_seq      = 0;

    // Instantiate SPAN with known entropy
    next_nonce_instantiate(&span->d.rng, test_sei_valid, test_rei_valid, nonce_key);

    // Step 4: Generate SPAN value (sender's perspective)
    CTR_DRBG_CTX sender_rng;
    memcpy(&sender_rng, &span->d.rng, sizeof(CTR_DRBG_CTX));
    next_nonce_generate(&sender_rng, nonce);

    // Step 5: Build S2_MESSAGE_ENCAPSULATION frame
    // Frame format: [CC, CMD, seq_num, properties, <encrypted_payload>]
    frame_buffer[0] = 0x9F;  // COMMAND_CLASS_SECURITY_2
    frame_buffer[1] = 0x03;  // SECURITY_2_MESSAGE_ENCAPSULATION
    frame_buffer[2] = SEQUENCE_NUM;
    frame_buffer[3] = 0x00;  // Properties (no extensions)
    hdr_len         = 4;

    // Step 6: Construct AAD matching S2_make_aad() format
    // AAD = [sender(1B), receiver(1B), HomeID(4B BE), msg_len(2B BE), header_bytes...]
    uint32_t i             = 0;
    uint16_t total_msg_len = hdr_len + sizeof(plaintext) + 8;  // header + plaintext + auth_tag

    aad[i++] = REMOTE_NODE_ID & 0xFF;  // Sender
    aad[i++] = LOCAL_NODE_ID & 0xFF;   // Receiver
    aad[i++] = (HOME_ID >> 24) & 0xFF;
    aad[i++] = (HOME_ID >> 16) & 0xFF;
    aad[i++] = (HOME_ID >> 8) & 0xFF;
    aad[i++] = HOME_ID & 0xFF;
    aad[i++] = (total_msg_len >> 8) & 0xFF;
    aad[i++] = total_msg_len & 0xFF;
    memcpy(&aad[i], &frame_buffer[2], hdr_len - 2);  // Skip CC and CMD
    aad_len = i + (hdr_len - 2);

    // Step 7: Encrypt plaintext with CCM
    memcpy(&frame_buffer[hdr_len], plaintext, sizeof(plaintext));
    ciphertext_len = CCM_encrypt_and_auth(encryption_key, nonce, aad, aad_len, &frame_buffer[hdr_len], sizeof(plaintext));
    TEST_ASSERT_GREATER_THAN(0, ciphertext_len);

    // Total frame length: header + ciphertext
    uint16_t frame_len = hdr_len + ciphertext_len;
    TEST_ASSERT_EQUAL_UINT16(total_msg_len, frame_len);

    // Step 8: Set up connection parameters
    connection.l_node     = LOCAL_NODE_ID;
    connection.r_node     = REMOTE_NODE_ID;
    connection.rx_options = 0;     // Unicast
    connection.class_id   = 0xFF;  // Unknown before decryption

    // Step 9: Call S2_application_command_handler
    S2_application_command_handler(&s2_context, &connection, frame_buffer, frame_len);

    // Step 10: Verify decrypted message was received
    TEST_ASSERT_TRUE_MESSAGE(g_msg_received, "S2_msg_received_event was not called - decryption failed");
    TEST_ASSERT_EQUAL_UINT16(sizeof(plaintext), g_received_msg_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(plaintext, g_received_msg, sizeof(plaintext));

    // Step 11: Verify SPAN state remains NEGOTIATED
    TEST_ASSERT_EQUAL(SPAN_NEGOTIATED, span->state);

    // Step 12: Verify security class correctly identified
    TEST_ASSERT_EQUAL(0, g_received_connection.class_id);
}

/**
 * @brief Test SPAN resynchronization after desync
 *
 * Validates:
 * - Messages missed causing SPAN desync (sender > 5 nonces ahead)
 * - All 5 retry iterations fail
 * - SOS flag triggers Nonce Report with new REI
 * - Sender includes SN extension with new SEI
 * - SPAN reestablished via new entropy
 * - Communication resumes with new SPAN
 *
 * Test flow:
 * 1. Set up synchronized SPAN between sender and receiver
 * 2. Advance sender by 6 nonces (beyond MAX_SPAN_RETRY_ITERATIONS)
 * 3. Receiver tries to decrypt, all 5 retries fail
 * 4. Receiver sends Nonce Report with SOS=1 and new REI
 * 5. Simulate sender response with SN extension containing new SEI
 * 6. SPAN reinstantiated with new entropy
 * 7. Verify new message decrypts successfully
 */
void test_span_resynchronization_after_desync(void)
{
    struct S2 s2_context;
    s2_connection_t connection;
    uint8_t frame_buffer[128];
    uint8_t plaintext[] = {0x25, 0x03, 0xFF};  // Binary Switch Report: ON
    uint8_t encryption_key[16];
    uint8_t nonce_key[32];
    uint8_t sender_nonce[16];
    uint8_t aad[64];
    uint16_t aad_len;
    uint16_t ciphertext_len;
    uint16_t hdr_len;

    // Constants for test
    const node_t LOCAL_NODE_ID  = 1;
    const node_t REMOTE_NODE_ID = 2;
    const uint32_t HOME_ID      = 0xDEADBEEF;
    const uint8_t SEQUENCE_NUM  = 0x05;

    // Reset test state
    reset_test_state();

    // =========================================================================
    // PHASE 1: Set up S2 context with synchronized SPAN
    // =========================================================================

    memset(&s2_context, 0, sizeof(struct S2));
    memset(&connection, 0, sizeof(s2_connection_t));

    // Set up keys
    memcpy(nonce_key, test_nonce_key, 32);
    memcpy(encryption_key, test_nonce_key, 16);

    // Configure security group 0
    memcpy(s2_context.sg[0].enc_key, encryption_key, 16);
    memcpy(s2_context.sg[0].nonce_key, nonce_key, 32);
    s2_context.loaded_keys = 0x01;
    s2_context.my_home_id  = HOME_ID;
    s2_context.fsm         = IDLE;

    // Set up SPAN in NEGOTIATED state
    struct SPAN *span = &s2_context.span_table[0];
    span->lnode       = LOCAL_NODE_ID;
    span->rnode       = REMOTE_NODE_ID;
    span->class_id    = 0;
    span->state       = SPAN_NEGOTIATED;
    span->rx_seq      = 0;
    span->tx_seq      = 0;

    // Instantiate SPAN RNG for receiver
    next_nonce_instantiate(&span->d.rng, test_sei_valid, test_rei_valid, nonce_key);

    // =========================================================================
    // PHASE 2: Create sender's RNG and advance it beyond retry limit
    // =========================================================================

    CTR_DRBG_CTX sender_rng;
    next_nonce_instantiate(&sender_rng, test_sei_valid, test_rei_valid, nonce_key);

    // Advance sender by 6 nonces (beyond MAX_SPAN_RETRY_ITERATIONS = 5)
    // This simulates 6 messages that the receiver missed
    uint8_t discarded_nonce[16];
    for (int i = 0; i < 6; i++) {
        next_nonce_generate(&sender_rng, discarded_nonce);
    }

    // Generate the nonce for this message (N+6)
    next_nonce_generate(&sender_rng, sender_nonce);

    // =========================================================================
    // PHASE 3: Build encrypted message that will fail all retry attempts
    // =========================================================================

    frame_buffer[0] = 0x9F;  // COMMAND_CLASS_SECURITY_2
    frame_buffer[1] = 0x03;  // SECURITY_2_MESSAGE_ENCAPSULATION
    frame_buffer[2] = SEQUENCE_NUM;
    frame_buffer[3] = 0x00;  // Properties: no extensions
    hdr_len         = 4;

    // Construct AAD
    uint32_t i             = 0;
    uint16_t total_msg_len = hdr_len + sizeof(plaintext) + 8;

    aad[i++] = REMOTE_NODE_ID & 0xFF;
    aad[i++] = LOCAL_NODE_ID & 0xFF;
    aad[i++] = (HOME_ID >> 24) & 0xFF;
    aad[i++] = (HOME_ID >> 16) & 0xFF;
    aad[i++] = (HOME_ID >> 8) & 0xFF;
    aad[i++] = HOME_ID & 0xFF;
    aad[i++] = (total_msg_len >> 8) & 0xFF;
    aad[i++] = total_msg_len & 0xFF;
    memcpy(&aad[i], &frame_buffer[2], hdr_len - 2);
    aad_len = i + (hdr_len - 2);

    // Encrypt with sender's nonce (N+6)
    memcpy(&frame_buffer[hdr_len], plaintext, sizeof(plaintext));
    ciphertext_len = CCM_encrypt_and_auth(encryption_key, sender_nonce, aad, aad_len, &frame_buffer[hdr_len], sizeof(plaintext));
    TEST_ASSERT_GREATER_THAN(0, ciphertext_len);

    uint16_t frame_len = hdr_len + ciphertext_len;

    // Set up connection
    connection.l_node     = LOCAL_NODE_ID;
    connection.r_node     = REMOTE_NODE_ID;
    connection.rx_options = 0;
    connection.class_id   = 0xFF;

    // =========================================================================
    // PHASE 4: Attempt decryption - should fail and trigger SOS
    // =========================================================================

    S2_application_command_handler(&s2_context, &connection, frame_buffer, frame_len);

    // Verify decryption failed (message not received)
    TEST_ASSERT_FALSE_MESSAGE(g_msg_received, "Message should NOT have been received - sender was too far ahead");

    // Verify Nonce Report with SOS was sent
    TEST_ASSERT_TRUE_MESSAGE(g_nonce_report_sent, "Nonce Report with SOS should have been sent");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x01, g_nonce_report_flags & 0x01, "SOS flag (bit 0) should be set in Nonce Report");

    // Verify Nonce Report contains REI (16 bytes after 4-byte header)
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(20, g_nonce_report_len, "Nonce Report with SOS should be 20 bytes (4 header + 16 REI)");

    // Verify SPAN state transitioned to SOS_LOCAL_NONCE
    TEST_ASSERT_EQUAL_MESSAGE(SPAN_SOS_LOCAL_NONCE, span->state, "SPAN state should be SOS_LOCAL_NONCE after sending SOS Nonce Report");

    // =========================================================================
    // PHASE 5: Simulate sender response with SN extension
    // The sender receives our SOS Nonce Report and responds with a message
    // containing the SN extension (Sender's Nonce / SEI)
    // =========================================================================

    // Reset test state for next message
    g_msg_received      = false;
    g_nonce_report_sent = false;

    // Extract the new REI from our Nonce Report
    uint8_t new_rei[16];
    memcpy(new_rei, &g_nonce_report_buffer[4], 16);

    // Generate new SEI for sender (simulating sender generating new entropy)
    uint8_t new_sei[16] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA};

    // Sender instantiates new SPAN with new_sei and new_rei
    CTR_DRBG_CTX new_sender_rng;
    next_nonce_instantiate(&new_sender_rng, new_sei, new_rei, nonce_key);

    // Sender generates nonce for this message
    uint8_t new_sender_nonce[16];
    next_nonce_generate(&new_sender_rng, new_sender_nonce);

    // Build message with SN extension
    // Frame format: [CC, CMD, seq, props(Ext=1), SN_ext(18 bytes), encrypted_payload]
    uint8_t new_frame[128];
    uint8_t new_plaintext[]   = {0x25, 0x03, 0x00};  // Binary Switch Report: OFF
    const uint8_t NEW_SEQ_NUM = 0x06;

    new_frame[0] = 0x9F;  // COMMAND_CLASS_SECURITY_2
    new_frame[1] = 0x03;  // SECURITY_2_MESSAGE_ENCAPSULATION
    new_frame[2] = NEW_SEQ_NUM;
    new_frame[3] = 0x01;  // Properties: Extension bit set

    // SN Extension: [length(1), type(1), SEI(16)]
    new_frame[4] = 18;                   // Extension length: 2 header + 16 SEI
    new_frame[5] = 0x01 | 0x40;          // Type = SN (0x01), Critical flag set (0x40), More=0
    memcpy(&new_frame[6], new_sei, 16);  // SEI

    uint16_t new_hdr_len = 4 + 18;  // Base header + SN extension

    // Construct AAD for new message
    uint8_t new_aad[64];
    uint16_t new_aad_len;
    uint32_t j                 = 0;
    uint16_t new_total_msg_len = new_hdr_len + sizeof(new_plaintext) + 8;

    new_aad[j++] = REMOTE_NODE_ID & 0xFF;
    new_aad[j++] = LOCAL_NODE_ID & 0xFF;
    new_aad[j++] = (HOME_ID >> 24) & 0xFF;
    new_aad[j++] = (HOME_ID >> 16) & 0xFF;
    new_aad[j++] = (HOME_ID >> 8) & 0xFF;
    new_aad[j++] = HOME_ID & 0xFF;
    new_aad[j++] = (new_total_msg_len >> 8) & 0xFF;
    new_aad[j++] = new_total_msg_len & 0xFF;
    // Include header from index 2 (seq, props, and all extensions) in AAD
    memcpy(&new_aad[j], &new_frame[2], new_hdr_len - 2);
    new_aad_len = j + (new_hdr_len - 2);

    // Encrypt new plaintext
    memcpy(&new_frame[new_hdr_len], new_plaintext, sizeof(new_plaintext));
    uint16_t new_ciphertext_len = CCM_encrypt_and_auth(encryption_key, new_sender_nonce, new_aad, new_aad_len, &new_frame[new_hdr_len], sizeof(new_plaintext));
    TEST_ASSERT_GREATER_THAN(0, new_ciphertext_len);

    uint16_t new_frame_len = new_hdr_len + new_ciphertext_len;

    // =========================================================================
    // PHASE 6: Process message with SN extension - should resync SPAN
    // =========================================================================

    connection.class_id = 0xFF;  // Reset class_id
    S2_application_command_handler(&s2_context, &connection, new_frame, new_frame_len);

    // =========================================================================
    // PHASE 7: Verify resynchronization succeeded
    // =========================================================================

    // Verify decrypted message was received
    TEST_ASSERT_TRUE_MESSAGE(g_msg_received, "S2_msg_received_event should have been called after SPAN resync");
    TEST_ASSERT_EQUAL_UINT16(sizeof(new_plaintext), g_received_msg_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(new_plaintext, g_received_msg, sizeof(new_plaintext));

    // Verify SPAN state is back to NEGOTIATED
    TEST_ASSERT_EQUAL_MESSAGE(SPAN_NEGOTIATED, span->state, "SPAN state should be NEGOTIATED after successful resync");

    // Verify security class correctly identified
    TEST_ASSERT_EQUAL_UINT8(0, g_received_connection.class_id);

    // =========================================================================
    // PHASE 8: Verify continued communication works
    // =========================================================================

    // Reset for next message
    reset_test_state();

    // Sender sends another message (no SN extension needed now)
    uint8_t continued_plaintext[]   = {0x20, 0x02, 0x32};  // Basic Report
    const uint8_t CONTINUED_SEQ_NUM = 0x07;

    // Generate next nonce from sender's RNG
    uint8_t continued_nonce[16];
    next_nonce_generate(&new_sender_rng, continued_nonce);

    // Build simple message without extensions
    uint8_t continued_frame[128];
    continued_frame[0]         = 0x9F;
    continued_frame[1]         = 0x03;
    continued_frame[2]         = CONTINUED_SEQ_NUM;
    continued_frame[3]         = 0x00;  // No extensions
    uint16_t continued_hdr_len = 4;

    // Construct AAD
    uint8_t continued_aad[64];
    uint16_t continued_aad_len;
    uint32_t k                   = 0;
    uint16_t continued_total_len = continued_hdr_len + sizeof(continued_plaintext) + 8;

    continued_aad[k++] = REMOTE_NODE_ID & 0xFF;
    continued_aad[k++] = LOCAL_NODE_ID & 0xFF;
    continued_aad[k++] = (HOME_ID >> 24) & 0xFF;
    continued_aad[k++] = (HOME_ID >> 16) & 0xFF;
    continued_aad[k++] = (HOME_ID >> 8) & 0xFF;
    continued_aad[k++] = HOME_ID & 0xFF;
    continued_aad[k++] = (continued_total_len >> 8) & 0xFF;
    continued_aad[k++] = continued_total_len & 0xFF;
    memcpy(&continued_aad[k], &continued_frame[2], continued_hdr_len - 2);
    continued_aad_len = k + (continued_hdr_len - 2);

    // Encrypt
    memcpy(&continued_frame[continued_hdr_len], continued_plaintext, sizeof(continued_plaintext));
    uint16_t continued_ciphertext_len = CCM_encrypt_and_auth(encryption_key, continued_nonce, continued_aad, continued_aad_len, &continued_frame[continued_hdr_len], sizeof(continued_plaintext));
    TEST_ASSERT_GREATER_THAN(0, continued_ciphertext_len);

    uint16_t continued_frame_len = continued_hdr_len + continued_ciphertext_len;

    // Process message
    connection.class_id = 0xFF;
    S2_application_command_handler(&s2_context, &connection, continued_frame, continued_frame_len);

    // Verify decryption succeeded
    TEST_ASSERT_TRUE_MESSAGE(g_msg_received, "Continued communication should work after SPAN resync");
    TEST_ASSERT_EQUAL_UINT16(sizeof(continued_plaintext), g_received_msg_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(continued_plaintext, g_received_msg, sizeof(continued_plaintext));

    // SPAN should still be NEGOTIATED
    TEST_ASSERT_EQUAL(SPAN_NEGOTIATED, span->state);
}

/*******************************************************************************
 * Test Function Prototypes - Negative Test Cases
 ******************************************************************************/

/**
 * @brief Test SPAN decrypt with wrong encryption key
 *
 * Validates:
 * - Authentication fails with incorrect KeyCCM
 * - All retry iterations fail (key mismatch, not SPAN desync)
 * - SOS Nonce Report sent to trigger resynchronization
 * - SPAN state transitions to SOS_LOCAL_NONCE
 *
 * This test simulates a scenario where the sender encrypts with a different
 * encryption key than what the receiver has configured. This could happen if:
 * - Keys were not properly exchanged during inclusion
 * - Key corruption occurred
 * - Attacker attempts to inject messages with wrong key
 *
 * The receiver should:
 * 1. Try all SPAN retry iterations (all fail due to wrong key)
 * 2. Send Nonce Report with SOS flag
 * 3. Transition to SOS_LOCAL_NONCE state
 */
void test_span_decrypt_wrong_key(void)
{
    struct S2 s2_context;
    s2_connection_t connection;
    uint8_t frame_buffer[128];
    uint8_t plaintext[] = {0x25, 0x03, 0xFF};  // Binary Switch Report: ON
    uint8_t receiver_encryption_key[16];
    uint8_t sender_encryption_key[16];  // Different from receiver's key
    uint8_t nonce_key[32];
    uint8_t sender_nonce[16];
    uint8_t aad[64];
    uint16_t aad_len;
    uint16_t ciphertext_len;
    uint16_t hdr_len;

    // Constants for test
    const node_t LOCAL_NODE_ID  = 1;
    const node_t REMOTE_NODE_ID = 2;
    const uint32_t HOME_ID      = 0xCAFEBABE;
    const uint8_t SEQUENCE_NUM  = 0x10;

    // Reset test state
    reset_test_state();

    // =========================================================================
    // Step 1: Set up keys - receiver has different encryption key than sender
    // =========================================================================

    memcpy(nonce_key, test_nonce_key, 32);

    // Receiver's encryption key (what S2 context will use for decryption)
    memcpy(receiver_encryption_key, test_nonce_key, 16);

    // Sender's encryption key (DIFFERENT - used to encrypt the message)
    // This simulates a key mismatch scenario
    uint8_t wrong_key_bytes[16] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    memcpy(sender_encryption_key, wrong_key_bytes, 16);

    // =========================================================================
    // Step 2: Initialize S2 context with receiver's key
    // =========================================================================

    memset(&s2_context, 0, sizeof(struct S2));
    memset(&connection, 0, sizeof(s2_connection_t));

    // Configure security group 0 with RECEIVER's encryption key
    memcpy(s2_context.sg[0].enc_key, receiver_encryption_key, 16);
    memcpy(s2_context.sg[0].nonce_key, nonce_key, 32);
    s2_context.loaded_keys = 0x01;  // Security class 0 loaded
    s2_context.my_home_id  = HOME_ID;
    s2_context.fsm         = IDLE;

    // Set up SPAN in NEGOTIATED state
    struct SPAN *span = &s2_context.span_table[0];
    span->lnode       = LOCAL_NODE_ID;
    span->rnode       = REMOTE_NODE_ID;
    span->class_id    = 0;
    span->state       = SPAN_NEGOTIATED;
    span->rx_seq      = 0;
    span->tx_seq      = 0;

    // Instantiate SPAN RNG - both sender and receiver have same SPAN
    // (the problem is the encryption key, not the SPAN synchronization)
    next_nonce_instantiate(&span->d.rng, test_sei_valid, test_rei_valid, nonce_key);

    // =========================================================================
    // Step 3: Create sender's RNG (synchronized with receiver)
    // =========================================================================

    CTR_DRBG_CTX sender_rng;
    next_nonce_instantiate(&sender_rng, test_sei_valid, test_rei_valid, nonce_key);

    // Generate sender's nonce (same as what receiver will try first)
    next_nonce_generate(&sender_rng, sender_nonce);

    // =========================================================================
    // Step 4: Build encrypted message using WRONG encryption key
    // =========================================================================

    frame_buffer[0] = 0x9F;  // COMMAND_CLASS_SECURITY_2
    frame_buffer[1] = 0x03;  // SECURITY_2_MESSAGE_ENCAPSULATION
    frame_buffer[2] = SEQUENCE_NUM;
    frame_buffer[3] = 0x00;  // Properties: no extensions
    hdr_len         = 4;

    // Construct AAD
    uint32_t i             = 0;
    uint16_t total_msg_len = hdr_len + sizeof(plaintext) + 8;

    aad[i++] = REMOTE_NODE_ID & 0xFF;
    aad[i++] = LOCAL_NODE_ID & 0xFF;
    aad[i++] = (HOME_ID >> 24) & 0xFF;
    aad[i++] = (HOME_ID >> 16) & 0xFF;
    aad[i++] = (HOME_ID >> 8) & 0xFF;
    aad[i++] = HOME_ID & 0xFF;
    aad[i++] = (total_msg_len >> 8) & 0xFF;
    aad[i++] = total_msg_len & 0xFF;
    memcpy(&aad[i], &frame_buffer[2], hdr_len - 2);
    aad_len = i + (hdr_len - 2);

    // Encrypt with SENDER's (wrong) encryption key
    memcpy(&frame_buffer[hdr_len], plaintext, sizeof(plaintext));
    ciphertext_len = CCM_encrypt_and_auth(sender_encryption_key,  // WRONG KEY - different from receiver's
                                          sender_nonce,
                                          aad,
                                          aad_len,
                                          &frame_buffer[hdr_len],
                                          sizeof(plaintext));
    TEST_ASSERT_GREATER_THAN(0, ciphertext_len);

    uint16_t frame_len = hdr_len + ciphertext_len;

    // =========================================================================
    // Step 5: Set up connection and attempt decryption
    // =========================================================================

    connection.l_node     = LOCAL_NODE_ID;
    connection.r_node     = REMOTE_NODE_ID;
    connection.rx_options = 0;  // Unicast
    connection.class_id   = 0xFF;

    // Call S2_application_command_handler
    // The receiver will try to decrypt with its key, which will fail
    // Even though SPAN is synchronized, the wrong key causes auth failure
    S2_application_command_handler(&s2_context, &connection, frame_buffer, frame_len);

    // =========================================================================
    // Step 6: Verify decryption failed
    // =========================================================================

    // Message should NOT have been received - wrong key causes auth failure
    TEST_ASSERT_FALSE_MESSAGE(g_msg_received, "Message should NOT have been received - wrong encryption key");

    // =========================================================================
    // Step 7: Verify SOS was triggered
    // =========================================================================

    // Nonce Report with SOS should have been sent
    TEST_ASSERT_TRUE_MESSAGE(g_nonce_report_sent, "Nonce Report with SOS should have been sent after auth failure");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x01, g_nonce_report_flags & 0x01, "SOS flag (bit 0) should be set in Nonce Report");

    // Verify Nonce Report is full length (includes REI)
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(20, g_nonce_report_len, "Nonce Report with SOS should be 20 bytes (4 header + 16 REI)");

    // =========================================================================
    // Step 8: Verify SPAN state transition
    // =========================================================================

    // SPAN should transition to SOS_LOCAL_NONCE (awaiting resync)
    TEST_ASSERT_EQUAL_MESSAGE(SPAN_SOS_LOCAL_NONCE, span->state, "SPAN state should be SOS_LOCAL_NONCE after failed decryption with wrong key");

    // =========================================================================
    // Step 9: Verify recovery with correct key via SN extension
    // After SOS, sender should respond with message containing SN extension
    // =========================================================================

    // The REI is stored in span->d.r_nonce by S2_send_nonce_report
    // We use this to create a new SPAN that matches what receiver will instantiate
    uint8_t new_rei[16];
    memcpy(new_rei, span->d.r_nonce, 16);

    // New SEI from sender
    uint8_t new_sei[16] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00};

    // Reset test state for recovery message
    reset_test_state();

    // Sender instantiates new SPAN with new entropy
    CTR_DRBG_CTX new_sender_rng;
    next_nonce_instantiate(&new_sender_rng, new_sei, new_rei, nonce_key);

    uint8_t new_sender_nonce[16];
    next_nonce_generate(&new_sender_rng, new_sender_nonce);

    // Build message with SN extension and CORRECT encryption key
    uint8_t new_frame[128];
    uint8_t new_plaintext[]   = {0x25, 0x03, 0x00};
    const uint8_t NEW_SEQ_NUM = 0x11;

    new_frame[0] = 0x9F;
    new_frame[1] = 0x03;
    new_frame[2] = NEW_SEQ_NUM;
    new_frame[3] = 0x01;  // Extension bit set

    // SN Extension
    new_frame[4] = 18;
    new_frame[5] = 0x01 | 0x40;  // Type = SN, Critical flag
    memcpy(&new_frame[6], new_sei, 16);

    uint16_t new_hdr_len = 4 + 18;

    // Construct AAD
    uint8_t new_aad[64];
    uint16_t new_aad_len;
    uint32_t j                 = 0;
    uint16_t new_total_msg_len = new_hdr_len + sizeof(new_plaintext) + 8;

    new_aad[j++] = REMOTE_NODE_ID & 0xFF;
    new_aad[j++] = LOCAL_NODE_ID & 0xFF;
    new_aad[j++] = (HOME_ID >> 24) & 0xFF;
    new_aad[j++] = (HOME_ID >> 16) & 0xFF;
    new_aad[j++] = (HOME_ID >> 8) & 0xFF;
    new_aad[j++] = HOME_ID & 0xFF;
    new_aad[j++] = (new_total_msg_len >> 8) & 0xFF;
    new_aad[j++] = new_total_msg_len & 0xFF;
    memcpy(&new_aad[j], &new_frame[2], new_hdr_len - 2);
    new_aad_len = j + (new_hdr_len - 2);

    // Encrypt with CORRECT encryption key this time
    memcpy(&new_frame[new_hdr_len], new_plaintext, sizeof(new_plaintext));
    uint16_t new_ciphertext_len = CCM_encrypt_and_auth(receiver_encryption_key,  // CORRECT KEY now
                                                       new_sender_nonce,
                                                       new_aad,
                                                       new_aad_len,
                                                       &new_frame[new_hdr_len],
                                                       sizeof(new_plaintext));
    TEST_ASSERT_GREATER_THAN(0, new_ciphertext_len);

    uint16_t new_frame_len = new_hdr_len + new_ciphertext_len;

    // Process resync message
    connection.class_id = 0xFF;
    S2_application_command_handler(&s2_context, &connection, new_frame, new_frame_len);

    // =========================================================================
    // Step 10: Verify recovery with correct key works
    // =========================================================================

    TEST_ASSERT_TRUE_MESSAGE(g_msg_received, "Message should be received after resync with correct key");
    TEST_ASSERT_EQUAL_UINT16(sizeof(new_plaintext), g_received_msg_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(new_plaintext, g_received_msg, sizeof(new_plaintext));

    // SPAN should be NEGOTIATED again
    TEST_ASSERT_EQUAL(SPAN_NEGOTIATED, span->state);
}

/**
 * @brief Test SPAN decrypt with modified nonce
 *
 * Validates:
 * - SPAN/nonce value manually corrupted before decryption
 * - Authentication fails due to nonce mismatch
 * - All retry iterations fail (corrupted nonce doesn't match any valid SPAN value)
 * - SOS triggered for resynchronization
 *
 * This test simulates a scenario where the SPAN RNG state has been corrupted
 * (e.g., memory corruption, improper state management). Even though the sender
 * and receiver started with synchronized SPAN, the corruption causes all
 * generated nonces to be wrong.
 *
 * Test approach:
 * 1. Set up synchronized SPAN between sender and receiver
 * 2. Sender encrypts message with correct nonce N
 * 3. Corrupt receiver's RNG state before decryption attempt
 * 4. Receiver generates corrupted nonces, all authentication attempts fail
 * 5. SOS triggered to resynchronize
 */
void test_span_decrypt_modified_nonce(void)
{
    struct S2 s2_context;
    s2_connection_t connection;
    uint8_t frame_buffer[128];
    uint8_t plaintext[] = {0x25, 0x03, 0xFF};  // Binary Switch Report: ON
    uint8_t encryption_key[16];
    uint8_t nonce_key[32];
    uint8_t sender_nonce[16];
    uint8_t aad[64];
    uint16_t aad_len;
    uint16_t ciphertext_len;
    uint16_t hdr_len;

    // Constants for test
    const node_t LOCAL_NODE_ID  = 1;
    const node_t REMOTE_NODE_ID = 2;
    const uint32_t HOME_ID      = 0xFEEDFACE;
    const uint8_t SEQUENCE_NUM  = 0x20;

    // Reset test state
    reset_test_state();

    // =========================================================================
    // Step 1: Initialize S2 context with proper keys
    // =========================================================================

    memset(&s2_context, 0, sizeof(struct S2));
    memset(&connection, 0, sizeof(s2_connection_t));

    memcpy(nonce_key, test_nonce_key, 32);
    memcpy(encryption_key, test_nonce_key, 16);

    // Configure security group 0
    memcpy(s2_context.sg[0].enc_key, encryption_key, 16);
    memcpy(s2_context.sg[0].nonce_key, nonce_key, 32);
    s2_context.loaded_keys = 0x01;
    s2_context.my_home_id  = HOME_ID;
    s2_context.fsm         = IDLE;

    // Set up SPAN in NEGOTIATED state
    struct SPAN *span = &s2_context.span_table[0];
    span->lnode       = LOCAL_NODE_ID;
    span->rnode       = REMOTE_NODE_ID;
    span->class_id    = 0;
    span->state       = SPAN_NEGOTIATED;
    span->rx_seq      = 0;
    span->tx_seq      = 0;

    // =========================================================================
    // Step 2: Create synchronized sender RNG and generate message
    // =========================================================================

    // Instantiate both sender and receiver with same entropy
    CTR_DRBG_CTX sender_rng;
    next_nonce_instantiate(&sender_rng, test_sei_valid, test_rei_valid, nonce_key);
    next_nonce_instantiate(&span->d.rng, test_sei_valid, test_rei_valid, nonce_key);

    // Sender generates nonce for encryption
    next_nonce_generate(&sender_rng, sender_nonce);

    // =========================================================================
    // Step 3: Build encrypted message using correct nonce
    // =========================================================================

    frame_buffer[0] = 0x9F;  // COMMAND_CLASS_SECURITY_2
    frame_buffer[1] = 0x03;  // SECURITY_2_MESSAGE_ENCAPSULATION
    frame_buffer[2] = SEQUENCE_NUM;
    frame_buffer[3] = 0x00;  // Properties: no extensions
    hdr_len         = 4;

    // Construct AAD
    uint32_t i             = 0;
    uint16_t total_msg_len = hdr_len + sizeof(plaintext) + 8;

    aad[i++] = REMOTE_NODE_ID & 0xFF;
    aad[i++] = LOCAL_NODE_ID & 0xFF;
    aad[i++] = (HOME_ID >> 24) & 0xFF;
    aad[i++] = (HOME_ID >> 16) & 0xFF;
    aad[i++] = (HOME_ID >> 8) & 0xFF;
    aad[i++] = HOME_ID & 0xFF;
    aad[i++] = (total_msg_len >> 8) & 0xFF;
    aad[i++] = total_msg_len & 0xFF;
    memcpy(&aad[i], &frame_buffer[2], hdr_len - 2);
    aad_len = i + (hdr_len - 2);

    // Encrypt with correct sender nonce
    memcpy(&frame_buffer[hdr_len], plaintext, sizeof(plaintext));
    ciphertext_len = CCM_encrypt_and_auth(encryption_key, sender_nonce, aad, aad_len, &frame_buffer[hdr_len], sizeof(plaintext));
    TEST_ASSERT_GREATER_THAN(0, ciphertext_len);

    uint16_t frame_len = hdr_len + ciphertext_len;

    // =========================================================================
    // Step 4: CORRUPT the receiver's RNG state before decryption
    // This simulates memory corruption or improper state management
    // =========================================================================

    // Corrupt the RNG internal state - modify the V (counter) value
    // The CTR_DRBG_CTX contains internal state that we'll corrupt
    // We'll XOR some bytes to ensure it's different from expected
    uint8_t *rng_bytes = (uint8_t *)&span->d.rng;
    for (int j = 0; j < 16; j++) {
        rng_bytes[j] ^= 0xAA;  // Corrupt first 16 bytes of RNG state
    }

    // =========================================================================
    // Step 5: Attempt decryption with corrupted nonce
    // =========================================================================

    connection.l_node     = LOCAL_NODE_ID;
    connection.r_node     = REMOTE_NODE_ID;
    connection.rx_options = 0;
    connection.class_id   = 0xFF;

    S2_application_command_handler(&s2_context, &connection, frame_buffer, frame_len);

    // =========================================================================
    // Step 6: Verify decryption failed
    // =========================================================================

    // Message should NOT have been received - corrupted nonce causes auth failure
    TEST_ASSERT_FALSE_MESSAGE(g_msg_received, "Message should NOT have been received - nonce was corrupted");

    // =========================================================================
    // Step 7: Verify SOS was triggered
    // =========================================================================

    // Nonce Report with SOS should have been sent
    TEST_ASSERT_TRUE_MESSAGE(g_nonce_report_sent, "Nonce Report with SOS should have been sent after auth failure");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x01, g_nonce_report_flags & 0x01, "SOS flag (bit 0) should be set in Nonce Report");

    // Verify Nonce Report is full length (includes REI)
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(20, g_nonce_report_len, "Nonce Report with SOS should be 20 bytes (4 header + 16 REI)");

    // =========================================================================
    // Step 8: Verify SPAN state transition
    // =========================================================================

    // SPAN should transition to SOS_LOCAL_NONCE
    TEST_ASSERT_EQUAL_MESSAGE(SPAN_SOS_LOCAL_NONCE, span->state, "SPAN state should be SOS_LOCAL_NONCE after failed decryption with corrupted nonce");

    // =========================================================================
    // Step 9: Verify that a properly constructed message WOULD work
    // by testing that the correct nonce produces correct authentication
    // (This is a sanity check that our test setup was correct)
    // =========================================================================

    // Create fresh RNGs to verify the original setup was correct
    CTR_DRBG_CTX verify_sender_rng;
    CTR_DRBG_CTX verify_receiver_rng;
    uint8_t verify_sender_nonce[16];
    uint8_t verify_receiver_nonce[16];

    next_nonce_instantiate(&verify_sender_rng, test_sei_valid, test_rei_valid, nonce_key);
    next_nonce_instantiate(&verify_receiver_rng, test_sei_valid, test_rei_valid, nonce_key);

    next_nonce_generate(&verify_sender_rng, verify_sender_nonce);
    next_nonce_generate(&verify_receiver_rng, verify_receiver_nonce);

    // These should match - confirming our original sender_nonce was correct
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(verify_sender_nonce, verify_receiver_nonce, 16, "Verification: sender and receiver should generate identical nonces with same entropy");
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(sender_nonce, verify_sender_nonce, 16, "Verification: original sender nonce should match freshly generated nonce");
}

/**
 * @brief Test SPAN decrypt with mismatched HomeID in AAD
 *
 * Validates:
 * - Sender and receiver have different HomeIDs
 * - Message encrypted with sender's HomeID in AAD
 * - Receiver computes AAD with its own HomeID
 * - Authentication fails due to AAD mismatch
 * - SOS triggered for resynchronization
 *
 * This test demonstrates the importance of HomeID in the AAD construction.
 * Per Z-Wave specification, the HomeID is included in the AAD to ensure
 * messages are only valid within the correct network. A message from a
 * different network (different HomeID) will fail authentication.
 */
void test_span_decrypt_mismatched_homeid(void)
{
    struct S2 s2_context;
    s2_connection_t connection;
    uint8_t frame_buffer[128];
    uint8_t plaintext[] = {0x25, 0x03, 0xFF};  // Binary Switch Report: ON
    uint8_t encryption_key[16];
    uint8_t nonce_key[32];
    uint8_t sender_nonce[16];
    uint8_t aad[64];
    uint16_t aad_len;
    uint16_t ciphertext_len;
    uint16_t hdr_len;

    // Constants for test - note the DIFFERENT HomeIDs
    const node_t LOCAL_NODE_ID      = 1;
    const node_t REMOTE_NODE_ID     = 2;
    const uint32_t RECEIVER_HOME_ID = 0x12345678;  // Receiver's HomeID
    const uint32_t SENDER_HOME_ID   = 0xABCDEF01;  // Sender's (wrong) HomeID
    const uint8_t SEQUENCE_NUM      = 0x30;

    // Reset test state
    reset_test_state();

    // =========================================================================
    // Step 1: Initialize S2 context with RECEIVER's HomeID
    // =========================================================================

    memset(&s2_context, 0, sizeof(struct S2));
    memset(&connection, 0, sizeof(s2_connection_t));

    memcpy(nonce_key, test_nonce_key, 32);
    memcpy(encryption_key, test_nonce_key, 16);

    // Configure security group 0
    memcpy(s2_context.sg[0].enc_key, encryption_key, 16);
    memcpy(s2_context.sg[0].nonce_key, nonce_key, 32);
    s2_context.loaded_keys = 0x01;
    s2_context.my_home_id  = RECEIVER_HOME_ID;  // Receiver has different HomeID
    s2_context.fsm         = IDLE;

    // Set up SPAN in NEGOTIATED state
    struct SPAN *span = &s2_context.span_table[0];
    span->lnode       = LOCAL_NODE_ID;
    span->rnode       = REMOTE_NODE_ID;
    span->class_id    = 0;
    span->state       = SPAN_NEGOTIATED;
    span->rx_seq      = 0;
    span->tx_seq      = 0;

    // Instantiate SPAN RNG - synchronized between sender and receiver
    next_nonce_instantiate(&span->d.rng, test_sei_valid, test_rei_valid, nonce_key);

    // =========================================================================
    // Step 2: Create sender's RNG and generate nonce
    // =========================================================================

    CTR_DRBG_CTX sender_rng;
    next_nonce_instantiate(&sender_rng, test_sei_valid, test_rei_valid, nonce_key);
    next_nonce_generate(&sender_rng, sender_nonce);

    // =========================================================================
    // Step 3: Build encrypted message using SENDER's (wrong) HomeID in AAD
    // =========================================================================

    frame_buffer[0] = 0x9F;  // COMMAND_CLASS_SECURITY_2
    frame_buffer[1] = 0x03;  // SECURITY_2_MESSAGE_ENCAPSULATION
    frame_buffer[2] = SEQUENCE_NUM;
    frame_buffer[3] = 0x00;  // Properties: no extensions
    hdr_len         = 4;

    // Construct AAD with SENDER's HomeID (mismatched)
    uint32_t i             = 0;
    uint16_t total_msg_len = hdr_len + sizeof(plaintext) + 8;

    aad[i++] = REMOTE_NODE_ID & 0xFF;
    aad[i++] = LOCAL_NODE_ID & 0xFF;
    // Use SENDER's HomeID in AAD (this is what causes the mismatch)
    aad[i++] = (SENDER_HOME_ID >> 24) & 0xFF;
    aad[i++] = (SENDER_HOME_ID >> 16) & 0xFF;
    aad[i++] = (SENDER_HOME_ID >> 8) & 0xFF;
    aad[i++] = SENDER_HOME_ID & 0xFF;
    aad[i++] = (total_msg_len >> 8) & 0xFF;
    aad[i++] = total_msg_len & 0xFF;
    memcpy(&aad[i], &frame_buffer[2], hdr_len - 2);
    aad_len = i + (hdr_len - 2);

    // Encrypt with sender's nonce and SENDER's HomeID in AAD
    memcpy(&frame_buffer[hdr_len], plaintext, sizeof(plaintext));
    ciphertext_len = CCM_encrypt_and_auth(encryption_key, sender_nonce, aad, aad_len, &frame_buffer[hdr_len], sizeof(plaintext));
    TEST_ASSERT_GREATER_THAN(0, ciphertext_len);

    uint16_t frame_len = hdr_len + ciphertext_len;

    // =========================================================================
    // Step 4: Attempt decryption - receiver will use its own HomeID in AAD
    // =========================================================================

    connection.l_node     = LOCAL_NODE_ID;
    connection.r_node     = REMOTE_NODE_ID;
    connection.rx_options = 0;
    connection.class_id   = 0xFF;

    S2_application_command_handler(&s2_context, &connection, frame_buffer, frame_len);

    // =========================================================================
    // Step 5: Verify decryption failed due to HomeID mismatch
    // =========================================================================

    // Message should NOT have been received - HomeID mismatch in AAD
    TEST_ASSERT_FALSE_MESSAGE(g_msg_received, "Message should NOT have been received - HomeID mismatch in AAD");

    // =========================================================================
    // Step 6: Verify SOS was triggered
    // =========================================================================

    TEST_ASSERT_TRUE_MESSAGE(g_nonce_report_sent, "Nonce Report with SOS should have been sent after auth failure");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x01, g_nonce_report_flags & 0x01, "SOS flag (bit 0) should be set in Nonce Report");

    // =========================================================================
    // Step 7: Verify SPAN state transition
    // =========================================================================

    TEST_ASSERT_EQUAL_MESSAGE(SPAN_SOS_LOCAL_NONCE, span->state, "SPAN state should be SOS_LOCAL_NONCE after HomeID mismatch");
}

/**
 * @brief Test SPAN decrypt replay attack prevention
 *
 * Validates:
 * - Same encrypted message sent twice with same sequence number
 * - First attempt succeeds
 * - Second attempt is rejected by sequence number duplicate detection
 * - Replay attack mitigated without even attempting decryption
 *
 * Z-Wave S2 has TWO layers of replay protection:
 * 1. Sequence number duplicate detection (S2_verify_seq) - rejects messages
 *    with recently seen sequence numbers before attempting decryption
 * 2. SPAN advancement - even if an attacker uses a different sequence number,
 *    the old nonce would fail authentication
 *
 * This test verifies the first layer (sequence number check). The delay attack
 * test (test_span_decrypt_delay_attack) verifies the second layer.
 */
void test_span_decrypt_replay_attack(void)
{
    struct S2 s2_context;
    s2_connection_t connection;
    uint8_t frame_buffer[128];
    uint8_t plaintext[] = {0x25, 0x03, 0xFF};  // Binary Switch Report: ON
    uint8_t encryption_key[16];
    uint8_t nonce_key[32];
    uint8_t sender_nonce[16];
    uint8_t aad[64];
    uint16_t aad_len;
    uint16_t ciphertext_len;
    uint16_t hdr_len;

    // Constants for test
    const node_t LOCAL_NODE_ID  = 1;
    const node_t REMOTE_NODE_ID = 2;
    const uint32_t HOME_ID      = 0x11223344;
    const uint8_t SEQUENCE_NUM  = 0x40;

    // Reset test state
    reset_test_state();

    // =========================================================================
    // Step 1: Initialize S2 context
    // =========================================================================

    memset(&s2_context, 0, sizeof(struct S2));
    memset(&connection, 0, sizeof(s2_connection_t));

    memcpy(nonce_key, test_nonce_key, 32);
    memcpy(encryption_key, test_nonce_key, 16);

    memcpy(s2_context.sg[0].enc_key, encryption_key, 16);
    memcpy(s2_context.sg[0].nonce_key, nonce_key, 32);
    s2_context.loaded_keys = 0x01;
    s2_context.my_home_id  = HOME_ID;
    s2_context.fsm         = IDLE;

    struct SPAN *span = &s2_context.span_table[0];
    span->lnode       = LOCAL_NODE_ID;
    span->rnode       = REMOTE_NODE_ID;
    span->class_id    = 0;
    span->state       = SPAN_NEGOTIATED;
    span->rx_seq      = 0;
    span->tx_seq      = 0;

    // Instantiate synchronized SPAN
    next_nonce_instantiate(&span->d.rng, test_sei_valid, test_rei_valid, nonce_key);

    // =========================================================================
    // Step 2: Create sender's RNG and generate first message
    // =========================================================================

    CTR_DRBG_CTX sender_rng;
    next_nonce_instantiate(&sender_rng, test_sei_valid, test_rei_valid, nonce_key);
    next_nonce_generate(&sender_rng, sender_nonce);

    // =========================================================================
    // Step 3: Build encrypted message
    // =========================================================================

    frame_buffer[0] = 0x9F;
    frame_buffer[1] = 0x03;
    frame_buffer[2] = SEQUENCE_NUM;
    frame_buffer[3] = 0x00;
    hdr_len         = 4;

    uint32_t i             = 0;
    uint16_t total_msg_len = hdr_len + sizeof(plaintext) + 8;

    aad[i++] = REMOTE_NODE_ID & 0xFF;
    aad[i++] = LOCAL_NODE_ID & 0xFF;
    aad[i++] = (HOME_ID >> 24) & 0xFF;
    aad[i++] = (HOME_ID >> 16) & 0xFF;
    aad[i++] = (HOME_ID >> 8) & 0xFF;
    aad[i++] = HOME_ID & 0xFF;
    aad[i++] = (total_msg_len >> 8) & 0xFF;
    aad[i++] = total_msg_len & 0xFF;
    memcpy(&aad[i], &frame_buffer[2], hdr_len - 2);
    aad_len = i + (hdr_len - 2);

    memcpy(&frame_buffer[hdr_len], plaintext, sizeof(plaintext));
    ciphertext_len = CCM_encrypt_and_auth(encryption_key, sender_nonce, aad, aad_len, &frame_buffer[hdr_len], sizeof(plaintext));
    TEST_ASSERT_GREATER_THAN(0, ciphertext_len);

    uint16_t frame_len = hdr_len + ciphertext_len;

    // =========================================================================
    // Step 4: FIRST attempt - should succeed
    // =========================================================================

    connection.l_node     = LOCAL_NODE_ID;
    connection.r_node     = REMOTE_NODE_ID;
    connection.rx_options = 0;
    connection.class_id   = 0xFF;

    S2_application_command_handler(&s2_context, &connection, frame_buffer, frame_len);

    // Verify first attempt succeeded
    TEST_ASSERT_TRUE_MESSAGE(g_msg_received, "First attempt should succeed - message received");
    TEST_ASSERT_EQUAL_UINT16(sizeof(plaintext), g_received_msg_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(plaintext, g_received_msg, sizeof(plaintext));

    // Verify SPAN state is still NEGOTIATED
    TEST_ASSERT_EQUAL(SPAN_NEGOTIATED, span->state);

    // Record rx_seq after first message
    uint8_t rx_seq_after_first = span->rx_seq;
    TEST_ASSERT_EQUAL_UINT8(SEQUENCE_NUM, rx_seq_after_first);

    // =========================================================================
    // Step 5: Reset test state for replay attempt
    // =========================================================================

    reset_test_state();

    // =========================================================================
    // Step 6: REPLAY attempt - send the SAME message again with SAME seq num
    // S2_verify_seq should reject this as a duplicate
    // =========================================================================

    // Reconstruct the exact same frame
    uint8_t replay_frame[128];
    replay_frame[0] = 0x9F;
    replay_frame[1] = 0x03;
    replay_frame[2] = SEQUENCE_NUM;  // SAME sequence number (replay)
    replay_frame[3] = 0x00;

    uint8_t replay_plaintext[] = {0x25, 0x03, 0xFF};
    memcpy(&replay_frame[hdr_len], replay_plaintext, sizeof(replay_plaintext));
    CCM_encrypt_and_auth(encryption_key,
                         sender_nonce,  // SAME nonce as first message (replay)
                         aad,
                         aad_len,
                         &replay_frame[hdr_len],
                         sizeof(replay_plaintext));

    connection.class_id = 0xFF;
    S2_application_command_handler(&s2_context, &connection, replay_frame, frame_len);

    // =========================================================================
    // Step 7: Verify replay attack was blocked by sequence number check
    // =========================================================================

    // Message should NOT have been received - replay blocked by seq check
    TEST_ASSERT_FALSE_MESSAGE(g_msg_received, "Replay attempt should FAIL - duplicate sequence number rejected");

    // NO SOS should be sent - message was silently dropped at seq check level
    // This is more efficient than processing the message and triggering SOS
    TEST_ASSERT_FALSE_MESSAGE(g_nonce_report_sent, "No Nonce Report should be sent - message dropped before decryption attempt");

    // SPAN state should remain NEGOTIATED (message was rejected early)
    TEST_ASSERT_EQUAL_MESSAGE(SPAN_NEGOTIATED, span->state, "SPAN state should remain NEGOTIATED - replay was caught by seq check");

    // rx_seq should remain unchanged (duplicate was rejected)
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(rx_seq_after_first, span->rx_seq, "rx_seq should remain unchanged after replay rejection");
}

/**
 * @brief Test SPAN decrypt delay attack scenario
 *
 * Validates:
 * - Message M1 is captured (but not delivered yet)
 * - Additional messages M2...M7 are exchanged, advancing SPAN
 * - Delayed message M1 is finally delivered
 * - M1 fails decryption (SPAN has advanced > 5 positions)
 * - Security maintained per spec Section 4.2.6.2.2
 *
 * This test simulates a delay attack where an attacker captures a message
 * and holds it, allowing the SPAN to advance with other traffic. When the
 * delayed message is finally delivered, it should fail because its nonce
 * is too far in the past (beyond the 5-iteration retry window).
 */
void test_span_decrypt_delay_attack(void)
{
    struct S2 s2_context;
    s2_connection_t connection;
    uint8_t encryption_key[16];
    uint8_t nonce_key[32];

    // Constants for test
    const node_t LOCAL_NODE_ID  = 1;
    const node_t REMOTE_NODE_ID = 2;
    const uint32_t HOME_ID      = 0x55667788;

    // Reset test state
    reset_test_state();

    // =========================================================================
    // Step 1: Initialize S2 context
    // =========================================================================

    memset(&s2_context, 0, sizeof(struct S2));
    memset(&connection, 0, sizeof(s2_connection_t));

    memcpy(nonce_key, test_nonce_key, 32);
    memcpy(encryption_key, test_nonce_key, 16);

    memcpy(s2_context.sg[0].enc_key, encryption_key, 16);
    memcpy(s2_context.sg[0].nonce_key, nonce_key, 32);
    s2_context.loaded_keys = 0x01;
    s2_context.my_home_id  = HOME_ID;
    s2_context.fsm         = IDLE;

    struct SPAN *span = &s2_context.span_table[0];
    span->lnode       = LOCAL_NODE_ID;
    span->rnode       = REMOTE_NODE_ID;
    span->class_id    = 0;
    span->state       = SPAN_NEGOTIATED;
    span->rx_seq      = 0;
    span->tx_seq      = 0;

    // Instantiate synchronized SPAN
    next_nonce_instantiate(&span->d.rng, test_sei_valid, test_rei_valid, nonce_key);

    // =========================================================================
    // Step 2: Create sender's RNG
    // =========================================================================

    CTR_DRBG_CTX sender_rng;
    next_nonce_instantiate(&sender_rng, test_sei_valid, test_rei_valid, nonce_key);

    // =========================================================================
    // Step 3: Sender creates message M1 (this will be "captured" and delayed)
    // =========================================================================

    uint8_t m1_nonce[16];
    next_nonce_generate(&sender_rng, m1_nonce);

    uint8_t m1_frame[128];
    uint8_t m1_plaintext[] = {0x25, 0x03, 0xFF};  // Binary Switch ON
    const uint8_t M1_SEQ   = 0x50;

    m1_frame[0]         = 0x9F;
    m1_frame[1]         = 0x03;
    m1_frame[2]         = M1_SEQ;
    m1_frame[3]         = 0x00;
    uint16_t m1_hdr_len = 4;

    uint8_t m1_aad[64];
    uint32_t idx          = 0;
    uint16_t m1_total_len = m1_hdr_len + sizeof(m1_plaintext) + 8;

    m1_aad[idx++] = REMOTE_NODE_ID & 0xFF;
    m1_aad[idx++] = LOCAL_NODE_ID & 0xFF;
    m1_aad[idx++] = (HOME_ID >> 24) & 0xFF;
    m1_aad[idx++] = (HOME_ID >> 16) & 0xFF;
    m1_aad[idx++] = (HOME_ID >> 8) & 0xFF;
    m1_aad[idx++] = HOME_ID & 0xFF;
    m1_aad[idx++] = (m1_total_len >> 8) & 0xFF;
    m1_aad[idx++] = m1_total_len & 0xFF;
    memcpy(&m1_aad[idx], &m1_frame[2], m1_hdr_len - 2);
    uint16_t m1_aad_len = idx + (m1_hdr_len - 2);

    memcpy(&m1_frame[m1_hdr_len], m1_plaintext, sizeof(m1_plaintext));
    uint16_t m1_cipher_len = CCM_encrypt_and_auth(encryption_key, m1_nonce, m1_aad, m1_aad_len, &m1_frame[m1_hdr_len], sizeof(m1_plaintext));
    TEST_ASSERT_GREATER_THAN(0, m1_cipher_len);
    uint16_t m1_frame_len = m1_hdr_len + m1_cipher_len;

    // M1 is now "captured" by attacker - NOT delivered to receiver yet

    // =========================================================================
    // Step 4: Send 6 more messages (M2-M7) to advance SPAN beyond retry window
    // =========================================================================

    for (int msg_num = 2; msg_num <= 7; msg_num++) {
        uint8_t msg_nonce[16];
        next_nonce_generate(&sender_rng, msg_nonce);

        uint8_t msg_frame[128];
        uint8_t msg_plaintext[] = {0x20, 0x02, (uint8_t)msg_num};  // Basic Report
        uint8_t msg_seq         = 0x50 + msg_num;

        msg_frame[0]         = 0x9F;
        msg_frame[1]         = 0x03;
        msg_frame[2]         = msg_seq;
        msg_frame[3]         = 0x00;
        uint16_t msg_hdr_len = 4;

        uint8_t msg_aad[64];
        uint32_t j             = 0;
        uint16_t msg_total_len = msg_hdr_len + sizeof(msg_plaintext) + 8;

        msg_aad[j++] = REMOTE_NODE_ID & 0xFF;
        msg_aad[j++] = LOCAL_NODE_ID & 0xFF;
        msg_aad[j++] = (HOME_ID >> 24) & 0xFF;
        msg_aad[j++] = (HOME_ID >> 16) & 0xFF;
        msg_aad[j++] = (HOME_ID >> 8) & 0xFF;
        msg_aad[j++] = HOME_ID & 0xFF;
        msg_aad[j++] = (msg_total_len >> 8) & 0xFF;
        msg_aad[j++] = msg_total_len & 0xFF;
        memcpy(&msg_aad[j], &msg_frame[2], msg_hdr_len - 2);
        uint16_t msg_aad_len = j + (msg_hdr_len - 2);

        memcpy(&msg_frame[msg_hdr_len], msg_plaintext, sizeof(msg_plaintext));
        uint16_t msg_cipher_len = CCM_encrypt_and_auth(encryption_key, msg_nonce, msg_aad, msg_aad_len, &msg_frame[msg_hdr_len], sizeof(msg_plaintext));
        TEST_ASSERT_GREATER_THAN(0, msg_cipher_len);
        uint16_t msg_frame_len = msg_hdr_len + msg_cipher_len;

        // Reset for this message
        g_msg_received = false;

        connection.l_node     = LOCAL_NODE_ID;
        connection.r_node     = REMOTE_NODE_ID;
        connection.rx_options = 0;
        connection.class_id   = 0xFF;

        S2_application_command_handler(&s2_context, &connection, msg_frame, msg_frame_len);

        // Each message should succeed
        TEST_ASSERT_TRUE_MESSAGE(g_msg_received, "Messages M2-M7 should all succeed");
        TEST_ASSERT_EQUAL(SPAN_NEGOTIATED, span->state);
    }

    // At this point, receiver's SPAN has advanced 6 positions
    // M1's nonce is now 6 positions in the past (beyond the 5 retry limit)

    // =========================================================================
    // Step 5: Reset test state and deliver the delayed M1
    // =========================================================================

    reset_test_state();

    connection.l_node     = LOCAL_NODE_ID;
    connection.r_node     = REMOTE_NODE_ID;
    connection.rx_options = 0;
    connection.class_id   = 0xFF;

    // Deliver the delayed M1 message
    S2_application_command_handler(&s2_context, &connection, m1_frame, m1_frame_len);

    // =========================================================================
    // Step 6: Verify delayed message was rejected
    // =========================================================================

    // M1 should NOT be received - nonce is too far in the past
    TEST_ASSERT_FALSE_MESSAGE(g_msg_received, "Delayed message M1 should FAIL - nonce is beyond retry window (6 > 5)");

    // SOS should be triggered
    TEST_ASSERT_TRUE_MESSAGE(g_nonce_report_sent, "Nonce Report with SOS should have been sent for delayed message");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x01, g_nonce_report_flags & 0x01, "SOS flag should be set for delayed message");

    // SPAN transitions to SOS_LOCAL_NONCE
    TEST_ASSERT_EQUAL_MESSAGE(SPAN_SOS_LOCAL_NONCE, span->state, "SPAN state should be SOS_LOCAL_NONCE after delay attack");
}
