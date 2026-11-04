#ifndef SCOPED_ZEROIZE_HPP
#define SCOPED_ZEROIZE_HPP

#include <cstddef>

#include "secure_memzero.h"

namespace common
{
    class ScopedZeroize
    {
        public:
            ScopedZeroize(void *buffer, std::size_t size) noexcept : buffer_(buffer), size_(size) {}

            ~ScopedZeroize()
            {
                secure_memzero(buffer_, size_);
            }

            ScopedZeroize(const ScopedZeroize &)            = delete;
            ScopedZeroize &operator=(const ScopedZeroize &) = delete;
            ScopedZeroize(ScopedZeroize &&)                 = delete;
            ScopedZeroize &operator=(ScopedZeroize &&)      = delete;

        private:
            void *buffer_;
            std::size_t size_;
    };
}  // namespace common

#endif
