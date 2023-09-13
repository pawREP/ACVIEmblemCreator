#include "Checksum.h"
#include "md5.h"

std::array<uint8_t, 0x10> md5Checksum(const uint8_t* data, int64_t size) {
    MD5Context ctx;
    md5Init(&ctx);
    md5Update(&ctx, const_cast<uint8_t*>(data), size); // This md5 library is sussy..
    md5Finalize(&ctx);

    std::array<uint8_t, 0x10> result;
    std::copy(std::begin(ctx.digest), std::end(ctx.digest), result.data());
    return result;
};