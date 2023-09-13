#pragma once
#include "Define.h"
#include <cassert>
#include <cinttypes>
#include <istream>

class BinaryStreamReader {
    MAKE_NONCOPYABLE(BinaryStreamReader);
    MAKE_NONMOVABLE(BinaryStreamReader);

public:
    BinaryStreamReader(std::basic_istream<uint8_t>& stream, int expections = -1);
    virtual ~BinaryStreamReader();

    void seek(std::streampos pos);
    void seek(std::streamoff off, std::ios_base::seekdir way);
    std::streampos tell() const;
    operator bool();
    bool headAtEof();

    template <typename T>
    T read() {
        static_assert(std::is_trivially_constructible_v<T>);
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(!std::is_pointer_v<T>);

        T v;
        stream.read((uint8_t*)&v, sizeof(T));
        assert(stream.good());

        return v;
    }

    template <typename T>
    void read(T* v, int64_t count) {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(!std::is_pointer_v<T>);

        stream.read((uint8_t*)v, count * sizeof(T));
        assert(stream.good());
    }

    template <typename T>
    T readExpected(T expected) {
        T v = read<T>();
        if(v != expected)
            throw std::runtime_error("Unexpected read result");
        return v;
    }

    template <typename T, typename Pred>
    T readExpected(T expected, Pred pred) {
        T v = read<T>();
        if(!pred(v))
            throw std::runtime_error("Unexpected read result");
        return v;
    }

    template <typename T>
    [[nodiscard]] T peak() {
        T v = read<T>();
        seek(-1 * (int64_t)sizeof(T), std::ios::cur);
        return v;
    }

    template <typename T>
    void peak(T* v, int64_t count) {
        read(v, count);
        seek(-count * (int64_t)sizeof(T), std::ios::cur);
    }

    template <int maxLength>
    std::string readStringFromFixedLengthBuffer() {
        char buf[maxLength];
        read(buf, maxLength);
        auto len = strnlen_s(buf, maxLength);
        if(len == maxLength)
            throw std::length_error("Bad String Length");
        return std::string{ buf };
    }

    template <int maxLength>
    std::wstring readWStringFromFixedLengthBuffer() {
        wchar_t buf[maxLength];
        read(buf, maxLength);
        auto len = wcsnlen_s(buf, maxLength);
        if(len == maxLength)
            throw std::length_error("Bad String Length");
        return std::wstring{ buf };
    }

    std::string readString();
    std::wstring readWString();

private:
    std::basic_istream<uint8_t>& stream;
    int exceptionMask;
};
