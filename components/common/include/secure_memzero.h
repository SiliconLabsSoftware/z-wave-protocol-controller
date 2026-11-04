#ifndef SECURE_MEMZERO_H
#define SECURE_MEMZERO_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void secure_memzero(void *buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif
