#include "crypto.hpp"

namespace crypto
{
    Crypto &Crypto::instance()
    {
        static Helpers helpers;
        static Crypto instance(helpers);
        return instance;
    }
}  // namespace crypto
