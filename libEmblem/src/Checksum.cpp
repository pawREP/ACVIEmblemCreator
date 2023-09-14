#include "Checksum.h"
#include "md5.h"

std::array<uint8_t, 0x10> MD5::checksum(const uint8_t* data, int64_t size) {
    MD5Context ctx;
    md5Init(&ctx);
    md5Update(&ctx, const_cast<uint8_t*>(data), size); // This md5 library is sussy..
    md5Finalize(&ctx);

    std::array<uint8_t, 0x10> result;
    std::copy(std::begin(ctx.digest), std::end(ctx.digest), result.data());
    return result;
};

class MD5::MD5Impl {
public:
    MD5Impl() {
        md5Init(&ctx);
    }

    void update(uint8_t* data, int64_t size) {
        md5Update(&ctx, const_cast<uint8_t*>(data), size);
    }

    std::array<uint8_t, 0x10> finalize() {
        md5Finalize(&ctx);
        std::array<uint8_t, 0x10> result;
        std::copy(std::begin(ctx.digest), std::end(ctx.digest), result.data());
        return result;
    }

private:
    MD5Context ctx;
};

MD5::MD5() : impl(std::make_unique<MD5Impl>()) {
}

MD5::~MD5() = default;

void MD5::update(uint8_t* data, int64_t size) {
    impl->update(data, size);
}

std::array<uint8_t, 0x10> MD5::finalize() {
    return impl->finalize();
}
