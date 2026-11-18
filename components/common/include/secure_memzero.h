#ifndef SECURE_MEMZERO_H
#define SECURE_MEMZERO_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Clear a buffer using writes that cannot be optimized away.
 *
 * @param[out] buffer Memory to clear; NULL is a no-op.
 * @param size Number of bytes to clear; zero is a no-op.
 */
void secure_memzero(void *buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif
