/**
 * @file kderiv_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdint.h>
#include "zwave_s2_key_derivation.h"
#include "mock_control.h"

#define MOCK_FILE "kderiv_mock.c"

sl_status_t zwave_s2_extract_pseudo_random_key(const uint8_t *ecdh_shared_secret, const uint8_t *public_keys, uint8_t *prk)
{
    mock_t *p_mock;

    MOCK_CALL_RETURN_IF_USED_AS_STUB(SL_STATUS_OK);
    MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, SL_STATUS_FAIL);

    MOCK_CALL_ACTUAL(p_mock, ecdh_shared_secret, public_keys, prk);

    MOCK_CALL_COMPARE_INPUT_UINT8_ARRAY(p_mock, ARG0, 32, ecdh_shared_secret, 32);
    MOCK_CALL_COMPARE_INPUT_UINT8_ARRAY(p_mock, ARG1, 64, public_keys, 64);
    if (p_mock->return_code.value == SL_STATUS_OK) {
        MOCK_CALL_SET_OUTPUT_ARRAY(p_mock->output_arg[2].pointer, prk, 16, uint8_t);
    }
    MOCK_CALL_RETURN_VALUE(p_mock, sl_status_t);
}
