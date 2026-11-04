#include "secure_memzero.h"

void secure_memzero(void *buffer, size_t size)
{
    if (buffer == NULL) {
        return;
    }

    volatile unsigned char *bytes = (volatile unsigned char *)buffer;
    while (size != 0) {
        *bytes++ = 0;
        --size;
    }
}
