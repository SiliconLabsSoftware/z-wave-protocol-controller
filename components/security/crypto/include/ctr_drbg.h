#ifndef CTR_DRBH_H
#define CTR_DRBH_H

#include <stdint.h>

/**
 * \ingroup crypto
 * @{
 */

#define OUTLEN  16
#define KEYLEN  16
#define SEEDLEN (OUTLEN + KEYLEN)

#ifndef RANDLEN
/**
 * Number of bytes AES_CTR_DRBG_Generate generates.
 */
#define RANDLEN 16
#endif

/** AES-128 processes data 16 bytes at a time. */
#define AJ_BLOCKSZ 16

/** Context for the Z-Wave AES-128 CTR_DRBG. */
typedef struct _CTR_DRBG_CTX {
        uint8_t df;        /**< Use the derivation function or not. */
        uint8_t v[OUTLEN]; /**< Internal working state. */
        uint8_t k[KEYLEN]; /**< Key working state. */
} CTR_DRBG_CTX;

/**
 * Reseed the DRBG.
 * \param[in,out] ctx The DRBG to reseed.
 * \param[in] seed The 32-byte reseed entropy.
 * \return 1 on success, 0 on failure.
 */
extern int AES_CTR_DRBG_Reseed(CTR_DRBG_CTX *ctx, const uint8_t *seed);

/**
 * Instantiate a DRBG.
 * \param[out] ctx The DRBG to instantiate and seed.
 * \param[in] entropy_input The 32-byte entropy input.
 * \param[in] personalization The 32-byte personalization string.
 * \return 1 on success, 0 on failure.
 */
extern int AES_CTR_DRBG_Instantiate(CTR_DRBG_CTX *ctx, const uint8_t *entropy_input, const uint8_t *personalization);

/**
 * Generate RANDLEN bytes of DRBG output.
 * \param[in,out] ctx The DRBG to generate from.
 * \param[out] rand The RANDLEN-byte output buffer.
 * \return 1 on success, 0 on failure.
 */
extern int AES_CTR_DRBG_Generate(CTR_DRBG_CTX *ctx, uint8_t *rand);

/**
 * @}
 */

#endif
