#ifndef SCOPED_ZEROIZE_HPP
#define SCOPED_ZEROIZE_HPP

#include <cstddef>
#include <span>

#include "secure_memzero.h"

namespace common
{
    /**
     * @brief Clear a buffer when this guard goes out of scope. The guard does not own the buffer.
     */
    class ScopedZeroize
    {
        public:
            /**
             * @brief Register a buffer to clear on destruction.
             *
             * @param buffer Memory to clear; must stay valid until the guard is destroyed. May be null.
             * @param size Number of bytes to clear.
             */
            ScopedZeroize(void *buffer, std::size_t size) noexcept : buffer_(std::span<std::byte>(static_cast<std::byte *>(buffer), size)) {}

            /**
             * @brief Clear the registered buffer.
             */
            ~ScopedZeroize()
            {
                secure_memzero(buffer_.data(), buffer_.size());
            }

            ScopedZeroize(const ScopedZeroize &)            = delete;
            ScopedZeroize &operator=(const ScopedZeroize &) = delete;
            ScopedZeroize(ScopedZeroize &&)                 = delete;
            ScopedZeroize &operator=(ScopedZeroize &&)      = delete;

        private:
            std::span<std::byte> buffer_;
    };
}  // namespace common

#endif
