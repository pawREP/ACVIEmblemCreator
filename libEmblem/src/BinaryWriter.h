#pragma once
#include "Define.h"
#include <cassert>
#include <ios>
#include <istream>
#include <vector>

class IReadWriteObserver;

class BinaryStreamWriterBase {
    MAKE_NONCOPYABLE(BinaryStreamWriterBase);
    MAKE_NONMOVABLE(BinaryStreamWriterBase);

public:
    BinaryStreamWriterBase(std::basic_ostream<uint8_t>& stream, int exceptions = -1);

    virtual ~BinaryStreamWriterBase();

    void seek(std::streampos pos);
    void seek(std::streamoff off, std::ios_base::seekdir way);
    void seekWithPad(std::streampos pos);
    std::streampos tell() const;
    operator bool();

    void write(const uint8_t* data, int64_t count);

    void registerObserver(IReadWriteObserver* observer);
    void unregisterObeserver(IReadWriteObserver* observer);

private:
    std::vector<IReadWriteObserver*> observers;
    std::basic_ostream<uint8_t>& stream;
    int exceptionMask;
};

class BinaryStreamWriter : public BinaryStreamWriterBase {
    MAKE_NONCOPYABLE(BinaryStreamWriter);
    MAKE_NONMOVABLE(BinaryStreamWriter);

public:
    BinaryStreamWriter(std::basic_ostream<uint8_t>& stream, int exceptions = -1);

    virtual ~BinaryStreamWriter() = default;

    template <typename T>
    void write(const T& v) {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(!std::is_pointer_v<T>);

        write(&v, 1);
    }

    template <typename T>
    void write(T* v, int64_t count) {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(!std::is_pointer_v<T>);

        BinaryStreamWriterBase::write((uint8_t*)v, count * sizeof(T));
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

    void padToNextMultipleOf(int32_t alignment);
};
