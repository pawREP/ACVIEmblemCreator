#pragma once
#include <cinttypes>

namespace libEmblem {

    inline constexpr uint64_t hash(const char* str) {
        uint64_t result = 14695981039346656037ull; // FNV-1a offset basis
        while(*str) {
            result ^= static_cast<uint64_t>(*str);
            result *= 1099511628211ull; // FNV-1a prime
            ++str;
        }
        return result;
    }

    inline consteval uint64_t operator"" _hash(const char* str, size_t size) {
        return hash(str);
    }

} // namespace libEmblem
