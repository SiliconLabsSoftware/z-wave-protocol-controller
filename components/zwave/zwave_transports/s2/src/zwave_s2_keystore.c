/******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#include "zwave_s2_keystore.h"
#include "zwave_s2_keystore_int.h"

/// Must be included before S2.h (because of definition of offsetof)
#include "stddef.h"

/// libs2 includes
#include "S2.h"
#include "curve25519.h"
#include "s2_keystore.h"

/// zwave_api includes
#include "zwapi_init.h"
#include "zwapi_protocol_basis.h"
#include "zwapi_protocol_mem.h"

// S0 includes (S0 network key set)
#include "zwave_s0_transport.h"

/// Other
#include "log.h"
#include "zwave_controller_endian.h"

#include <string.h>
#if defined(HAVE_EXPLICIT_BZERO)
#include <strings.h>
#endif

#define LOG_TAG "zwave_s2_keystore"

static void secure_memzero(void *ptr, size_t len)
{
    if (ptr == NULL || len == 0) {
        return;
    }
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    (void)memset_explicit(ptr, 0, len);
#elif defined(HAVE_EXPLICIT_BZERO)
    explicit_bzero(ptr, len);
#elif defined(__APPLE__)
    (void)memset_s(ptr, len, 0, len);
#else
    (void)memset(ptr, 0, len);
#endif
}

#define S2_NUM_KEY_CLASSES (s2_get_key_count()) /* Includes the S0 key */
#define NETWORK_KEY_SIZE   16                   //< Size of a Z-Wave network key.
// Max variable-length YAML line: "<class_name>: '<hex_encoded_key>'\n".
// Longest class name is "S2_AUTHENTICATED_LR" (19 chars).
// => 19 + 2 (": ") + 1 ("'") + 32 (hex) + 1 ("'") + 1 ('\n') = 56 bytes.
#define KEYSTORE_BLOB_MAX_CLASS_NAME_LEN 19U
#define KEYSTORE_BLOB_MAX_LINE_LEN       ((size_t)(KEYSTORE_BLOB_MAX_CLASS_NAME_LEN + 2 + 1 + (NETWORK_KEY_SIZE * 2) + 1 + 1))

uint8_t dynamic_ecdh_private_key[32];
static zwave_s2_keystore_ecdh_key_mode_t ecdh_key_mode;

bool keystore_network_key_clear(uint8_t keyclass)
{
    uint8_t random_bytes[64];
    uint8_t assigned_keys = 0xff;

    AES_CTR_DRBG_Generate(&s2_ctr_drbg, random_bytes);

    if (keyclass == KEY_CLASS_ALL) {
        keystore_network_key_clear(KEY_CLASS_S0);
        keystore_network_key_clear(KEY_CLASS_S2_UNAUTHENTICATED);
        keystore_network_key_clear(KEY_CLASS_S2_AUTHENTICATED);
        keystore_network_key_clear(KEY_CLASS_S2_ACCESS);
        keystore_network_key_clear(KEY_CLASS_S2_AUTHENTICATED_LR);
        keystore_network_key_clear(KEY_CLASS_S2_ACCESS_LR);
        return true;
    }

    if (keystore_network_key_write(keyclass, random_bytes)) {
        nvm_config_get(assigned_keys, &assigned_keys);
        assigned_keys &= ~keyclass;
        nvm_config_set(assigned_keys, &assigned_keys);
        memset(random_bytes, 0, sizeof(random_bytes));
        return true;
    }
    return false;
}

void keystore_dynamic_private_key_read(uint8_t *buf)
{
    return keystore_private_key_read(buf);
}

void keystore_private_key_read(uint8_t *buf)
{
    uint8_t nvr_version;

    if (ecdh_key_mode == ZWAVE_S2_KEYSTORE_DYNAMIC_ECDH_KEY) {
        memcpy(buf, dynamic_ecdh_private_key, 32);
    } else {
        uint8_t my_chip_type = 0;
        uint8_t version      = 0;
        zwapi_get_chip_type_version(&my_chip_type, &version);
        if (ZW_GECKO_CHIP_TYPE(my_chip_type)) {
            zwapi_nvr_get_value(offsetof(NVR_FLASH_STRUCT, aSecurityPrivateKey), NVR_SECURITY_PRIVATE_KEY_SIZE, buf);
        } else {
            zwapi_nvr_get_value(offsetof(NVR_FLASH_STRUCT, bRevision), 1, &nvr_version);
            if ((0xff != nvr_version) && (nvr_version >= 2)) {
                zwapi_nvr_get_value(offsetof(NVR_FLASH_STRUCT, aSecurityPrivateKey), NVR_SECURITY_PRIVATE_KEY_SIZE, buf);
            } else {
                nvm_config_get(ecdh_priv_key, buf);
            }
        }
    }
}

void keystore_public_key_debug_print(void)
{
    uint8_t buf[32];
    keystore_public_key_read(buf);
}

void keystore_dynamic_public_key_read(uint8_t *buf)
{
    return keystore_public_key_read(buf);
}

void keystore_public_key_read(uint8_t *buf)
{
    uint8_t priv_key[32];

    keystore_private_key_read(priv_key);
    crypto_scalarmult_curve25519_base(buf, priv_key);
    memset(priv_key, 0, sizeof(priv_key));
}

bool keystore_network_key_read(uint8_t keyclass, uint8_t *buf)
{
    uint8_t assigned_keys = zwave_s2_keystore_get_assigned_keys();

    if (0 == (keyclass & assigned_keys)) {
        return false;
    }

    switch (keyclass) {
        case KEY_CLASS_S0:
            nvm_config_get(security_netkey, buf);
            break;
        case KEY_CLASS_S2_UNAUTHENTICATED:
            nvm_config_get(security2_key[0], buf);
            break;
        case KEY_CLASS_S2_AUTHENTICATED:
            nvm_config_get(security2_key[1], buf);
            break;
        case KEY_CLASS_S2_ACCESS:
            nvm_config_get(security2_key[2], buf);
            break;
        case KEY_CLASS_S2_AUTHENTICATED_LR:
            nvm_config_get(security2_lr_key[0], buf);
            break;
        case KEY_CLASS_S2_ACCESS_LR:
            nvm_config_get(security2_lr_key[1], buf);
            break;
        default:
            assert(0);
            return false;
    }

    return true;
}

// Format one YAML line "<class_name>: '<hex_encoded_key>'\n" and append to buf.
static sl_status_t append_key_line_to_buffer(uint8_t *buf, size_t cap, size_t *offset, const uint8_t *key, const char *class_name)
{
    if (class_name == NULL) {
        return SL_STATUS_NULL_POINTER;
    }
    const size_t class_name_len = strnlen(class_name, KEYSTORE_BLOB_MAX_CLASS_NAME_LEN + 1);
    if (class_name_len > KEYSTORE_BLOB_MAX_CLASS_NAME_LEN) {
        return SL_STATUS_FAIL;
    }
    const size_t line_len = class_name_len + 3 + (size_t)(NETWORK_KEY_SIZE * 2) + 2;
    if ((*offset) + line_len > cap) {
        return SL_STATUS_WOULD_OVERFLOW;
    }

    char line[KEYSTORE_BLOB_MAX_LINE_LEN + 1] = {0};
    int written                               = snprintf(line, sizeof(line), "%s: '", class_name);
    for (int i = 0; i < NETWORK_KEY_SIZE; i++) {
        written += snprintf(line + written, sizeof(line) - (size_t)written, "%02X", key[i]);
    }
    written += snprintf(line + written, sizeof(line) - (size_t)written, "'\n");

    memcpy(buf + *offset, line, (size_t)written);
    *offset += (size_t)written;
    return SL_STATUS_OK;
}

sl_status_t zwave_s2_collect_security_keys_blob(uint8_t *buf, size_t buf_cap, size_t *out_len)
{
    if (buf == NULL || out_len == NULL) {
        return SL_STATUS_NULL_POINTER;
    }

    uint8_t current_key[NETWORK_KEY_SIZE] = {0};
    size_t offset                         = 0;
    sl_status_t st                        = SL_STATUS_OK;
    uint8_t assigned_keys                 = zwave_s2_keystore_get_assigned_keys();

    if ((st == SL_STATUS_OK) && (KEY_CLASS_S0 & assigned_keys)) {
        nvm_config_get(security_netkey, current_key);
        st = append_key_line_to_buffer(buf, buf_cap, &offset, current_key, "S0");
    }
    if ((st == SL_STATUS_OK) && (KEY_CLASS_S2_UNAUTHENTICATED & assigned_keys)) {
        nvm_config_get(security2_key[0], current_key);
        st = append_key_line_to_buffer(buf, buf_cap, &offset, current_key, "S2_UNAUTHENTICATED");
    }
    if ((st == SL_STATUS_OK) && (KEY_CLASS_S2_AUTHENTICATED & assigned_keys)) {
        nvm_config_get(security2_key[1], current_key);
        st = append_key_line_to_buffer(buf, buf_cap, &offset, current_key, "S2_AUTHENTICATED");
    }
    if ((st == SL_STATUS_OK) && (KEY_CLASS_S2_AUTHENTICATED_LR & assigned_keys)) {
        nvm_config_get(security2_lr_key[0], current_key);
        st = append_key_line_to_buffer(buf, buf_cap, &offset, current_key, "S2_AUTHENTICATED_LR");
    }
    if ((st == SL_STATUS_OK) && (KEY_CLASS_S2_ACCESS & assigned_keys)) {
        nvm_config_get(security2_key[2], current_key);
        st = append_key_line_to_buffer(buf, buf_cap, &offset, current_key, "S2_ACCESS");
    }
    if ((st == SL_STATUS_OK) && (KEY_CLASS_S2_ACCESS_LR & assigned_keys)) {
        nvm_config_get(security2_lr_key[1], current_key);
        st = append_key_line_to_buffer(buf, buf_cap, &offset, current_key, "S2_ACCESS_LR");
    }

    secure_memzero(current_key, sizeof(current_key));

    if (st != SL_STATUS_OK) {
        secure_memzero(buf, offset);
        return st;
    }

    *out_len = offset;
    return SL_STATUS_OK;
}

bool keystore_network_key_write(uint8_t keyclass, const uint8_t *buf)
{
    uint8_t assigned_keys = 0;

    switch (keyclass) {
        case KEY_CLASS_S0:
            nvm_config_set(security_netkey, buf);
            s0_set_key(buf);
            break;
        case KEY_CLASS_S2_UNAUTHENTICATED:
            nvm_config_set(security2_key[0], buf);
            break;
        case KEY_CLASS_S2_AUTHENTICATED:
            nvm_config_set(security2_key[1], buf);
            break;
        case KEY_CLASS_S2_ACCESS:
            nvm_config_set(security2_key[2], buf);
            break;
        case KEY_CLASS_S2_AUTHENTICATED_LR:
            nvm_config_set(security2_lr_key[0], buf);
            break;
        case KEY_CLASS_S2_ACCESS_LR:
            nvm_config_set(security2_lr_key[1], buf);
            break;

        default:
            assert(0);
            return false;
    }
    assigned_keys = zwave_s2_keystore_get_assigned_keys();
    assigned_keys |= keyclass;
    nvm_config_set(assigned_keys, &assigned_keys);
    return true;
}

void zwave_s2_keystore_set_ecdh_key_mode(zwave_s2_keystore_ecdh_key_mode_t mode)
{
    ecdh_key_mode = mode;
}

uint8_t zwave_s2_keystore_get_assigned_keys()
{
    uint8_t assigned_keys = 0;

    nvm_config_get(assigned_keys, &assigned_keys);
    return assigned_keys;
}

void zwave_s2_keystore_reset_assigned_keys()
{
    uint8_t assigned_keys = 0;
    nvm_config_set(assigned_keys, &assigned_keys);
}

void zwave_s2_create_new_dynamic_ecdh_key()
{
    AES_CTR_DRBG_Generate(&s2_ctr_drbg, dynamic_ecdh_private_key);
    AES_CTR_DRBG_Generate(&s2_ctr_drbg, &dynamic_ecdh_private_key[16]);
}

static void zwave_s2_create_new_learn_mode_ecdh_key()
{
    zwave_s2_create_new_dynamic_ecdh_key();
    nvm_config_set(ecdh_priv_key, dynamic_ecdh_private_key);
    zwave_s2_create_new_dynamic_ecdh_key();
}

void zwave_s2_create_new_network_keys()
{
    uint8_t net_key[16] = {0};
    sl_log_info(LOG_TAG, "Creating new network keys\n");
    for (int c = 0; c < S2_NUM_KEY_CLASSES - 1; c++) {
        S2_get_hw_random(net_key, 16);
        keystore_network_key_write(1 << c, net_key);
    }

    // Also create a new S0 key:
    S2_get_hw_random(net_key, 16);
    keystore_network_key_write(KEY_CLASS_S0, net_key);

    memset(net_key, 0, sizeof(net_key));
}

void zwave_s2_keystore_init()
{
    uint32_t magic = 0;

    if (SL_STATUS_OK != nvm_config_get(magic, &magic)) {
        sl_log_error(LOG_TAG, "Failed to read magic from zwapi_memory_get_buffer\n");
        assert(false);
    }
    magic = zwave_controller_ntohl(magic);
    if (magic != NVM_MAGIC) {
        sl_log_warning(LOG_TAG, "NVM magic check failed %08x != %08x, generating new keys\n", magic, NVM_MAGIC);
        zwave_s2_create_new_learn_mode_ecdh_key();
        zwave_s2_create_new_network_keys();

        magic = zwave_controller_ntohl(NVM_MAGIC);
        nvm_config_set(magic, &magic);
    }
    zwave_s2_create_new_dynamic_ecdh_key();
}

void zwave_s2_keystore_get_dsk(zwave_s2_keystore_ecdh_key_mode_t mode, zwave_dsk_t dsk)
{
    uint8_t pub_key[32];
    zwave_s2_keystore_ecdh_key_mode_t mode_save = ecdh_key_mode;
    zwave_s2_keystore_set_ecdh_key_mode(mode);
    keystore_public_key_read(pub_key);
    zwave_s2_keystore_set_ecdh_key_mode(mode_save);
    memcpy(dsk, pub_key, sizeof(zwave_dsk_t));
}
