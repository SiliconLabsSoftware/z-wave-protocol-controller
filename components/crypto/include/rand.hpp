#ifndef RAND_HPP
#define RAND_HPP

#include "rand.h"
#include "crypto.hpp"

#include <span>

namespace crypto
{

    sl_status_t generate_random(std::span<uint8_t> output);

}  // namespace crypto

#endif
