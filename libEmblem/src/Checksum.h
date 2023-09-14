#include "Define.h"
#include <array>
#include <memory>

class MD5 {
    MAKE_NONCOPYABLE(MD5);

public:
    MD5();
    MD5(MD5&&) noexcept            = default;
    MD5& operator=(MD5&&) noexcept = default;
    ~MD5();

    void update(uint8_t* data, int64_t size);
    std::array<uint8_t, 0x10> finalize();

    template <typename T>
    void update(const T& v) {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(!std::is_pointer_v<T>);

        update((uint8_t*)&v, sizeof(T));
    }

    template <typename T>
    void update(T* v, int64_t count) {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(!std::is_pointer_v<T>);

        update((uint8_t*)v, count * sizeof(T));
    }

    static std::array<uint8_t, 0x10> checksum(const uint8_t* data, int64_t size);

private:
    class MD5Impl;
    std::unique_ptr<MD5Impl> impl;
};
