#include "Crypto.h"
#include "aes.hpp"

void crypto::decryptInplace(uint8_t* data, uint32_t size, const uint8_t* iv, const uint8_t* key) {
    AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_decrypt_buffer(&ctx, data, size);
}

void crypto::encryptInplace(uint8_t* data, uint32_t size, const uint8_t* iv, const uint8_t* key) {
    AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_encrypt_buffer(&ctx, data, size);
}
