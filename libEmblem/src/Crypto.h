#pragma once
#include <cinttypes>

namespace crypto {

    constexpr uint32_t AESBlockLen = 16;

    void decryptInplace(uint8_t* data, uint32_t size, const uint8_t* iv, const uint8_t* key);
    void encryptInplace(uint8_t* data, uint32_t size, const uint8_t* iv, const uint8_t* key);

}; // namespace crypto
