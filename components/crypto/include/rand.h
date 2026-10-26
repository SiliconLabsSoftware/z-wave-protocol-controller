#ifndef RAND_H_
#define RAND_H_

#include <stddef.h>
#include <stdint.h>

#include "sl_status.h"

#ifdef __cplusplus
extern "C" {
#endif

sl_status_t crypto_generate_random(uint8_t *output, size_t output_size);

#ifdef __cplusplus
}
#endif

#endif
