#pragma once
#include "Define.h"
#include <cassert>
#include <istream>
#include <vector>

class BinaryStreamWriter {
    MAKE_NONCOPYABLE(BinaryStreamWriter);
    MAKE_NONMOVABLE(BinaryStreamWriter);

public:
    BinaryStreamWriter(std::basic_ostream<uint8_t>& stream, int exceptions = -1);

    virtual ~BinaryStreamWriter();

    void seek(std::streampos pos);
    void seek(std::streamoff off, std::ios_base::seekdir way);
    std::streampos tell() const;
    operator bool();

    template <typename T>
    void write(const T& v) {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(!std::is_pointer_v<T>);

        stream.write((uint8_t*)&v, sizeof(T));
        assert(stream.good());
    }

    template <typename T>
    void write(T* v, int64_t count) {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(!std::is_pointer_v<T>);

        stream.write((uint8_t*)v, count * sizeof(T));
        assert(stream.good());
    }

    template <int Length>
    void writeStringAsFixedLengthBuffer(const std::string& str) {
        assert(str.size() < Length);
        char buf[Length]{};
        memcpy_s(buf, Length, str.c_str(), str.size());
        write(buf, Length);
    }

    template <int Length>
    void writeWStringAsFixedLengthBuffer(const std::wstring& wstr) {
        assert((wstr.size() + 1) * sizeof(wchar_t) <= Length);
        char buf[Length]{};
        memcpy_s(buf, Length, wstr.c_str(), sizeof(wchar_t) * wstr.size());
        write(buf, Length);
    }

    template <>
    void write<std::string>(const std::string& str) {
        write(str.c_str(), str.size() + 1);
    }

    template <>
    void write<std::wstring>(const std::wstring& wstr) {
        write(wstr.c_str(), (wstr.size() + 1));
    }

private:
    std::basic_ostream<uint8_t>& stream;
    int exceptionMask;
};
