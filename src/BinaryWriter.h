#pragma once
#include <cinttypes>
#include <type_traits>
#include <vector>

class SequentialBinaryWriter {
public:
    template <typename T>
    void write(const T& t) {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(!std::is_pointer_v<T>);

        auto offset = buffer.size();
        growBufferBy(sizeof(T));
        std::memcpy(buffer.data() + offset, reinterpret_cast<const uint8_t*>(&t), sizeof(T));
    }
    template <typename T>
    void write(const T* arr, int size) {
        static_assert(std::is_trivially_copyable_v<T>);

        auto offset = buffer.size();
        growBufferBy(sizeof(T) * size);
        std::memcpy(buffer.data() + offset, reinterpret_cast<const uint8_t*>(arr), sizeof(T) * size);
    }

    int64_t size() const;
    std::vector<uint8_t>&& release();

private:
    void growBufferBy(size_t size);

    std::vector<uint8_t> buffer;
};
