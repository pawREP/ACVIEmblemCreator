#pragma once
#include "Define.h"
#include <cassert>
#include <cinttypes>
#include <cstdint>
#include <istream>
#include <vector>

class IReadWriteObserver;

class BinaryStreamReaderBase {
public:
    BinaryStreamReaderBase(std::basic_istream<uint8_t>& stream, int expections = -1);
    virtual ~BinaryStreamReaderBase();

    void seek(std::streampos pos);
    void seek(std::streamoff off, std::ios_base::seekdir way);
    std::streampos tell() const;
    operator bool();
    bool headAtEof();

    void read(uint8_t* v, int64_t count);

    void registerObserver(IReadWriteObserver* observer);
    void unregisterObeserver(IReadWriteObserver* observer);

private:
    std::vector<IReadWriteObserver*> observers;
    std::basic_istream<uint8_t>& stream;
    int exceptionMask;
};

class BinaryStreamReader : public BinaryStreamReaderBase {
    MAKE_NONCOPYABLE(BinaryStreamReader);
    MAKE_NONMOVABLE(BinaryStreamReader);

public:
    BinaryStreamReader(std::basic_istream<uint8_t>& stream, int expections = -1);
    virtual ~BinaryStreamReader() = default;

    template <typename T>
    T read() {
        static_assert(std::is_trivially_constructible_v<T>);
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(!std::is_pointer_v<T>);

        T v;
        read(&v, 1);
        return v;
    }

    template <typename T>
    void read(T* v, int64_t count) {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(!std::is_pointer_v<T>);
        static_assert(!std::is_const_v<T>);

        BinaryStreamReaderBase::read(reinterpret_cast<uint8_t*>(v), count * sizeof(T));
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

    template <int length>
    std::string readFixedLengthString() {
        std::string str(length, ' ');
        read(str.data(), length);
        return str;
    }

    template <int length>
    std::wstring readFixedLengthWString() {
        std::wstring str(length, ' ');
        read(str.data(), length);
        return str;
    }

    template <int maxLength>
    std::string readCStringFromFixedLengthBuffer() {
        char buf[maxLength];
        read(buf, maxLength);
        auto len = strnlen_s(buf, maxLength);
        if(len == maxLength)
            throw std::length_error("Bad String Length");
        return std::string{ buf };
    }

    template <int maxLength>
    std::wstring readCWStringFromFixedLengthBuffer() {
        wchar_t buf[maxLength];
        read(buf, maxLength);
        auto len = wcsnlen_s(buf, maxLength);
        if(len == maxLength)
            throw std::length_error("Bad String Length");
        return std::wstring{ buf };
    }

    std::string readCString();
    std::wstring readCWString();
};

};
