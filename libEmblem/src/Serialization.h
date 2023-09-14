#pragma once
#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "Error.h"
#include <concepts>
#include <type_traits>

template <typename T, typename TT>
concept is_ErrorOr_or_Value = std::same_as<T, TT> || std::same_as<T, ErrorOr<TT>>;

template <typename T>
concept Serializable = requires(T obj) {
    { obj.serialize(std::declval<BinaryStreamWriter&>()) } -> std::same_as<void>;
};

template <typename T>
concept Deserializable = requires(T obj) {
    { T::deserialize(std::declval<BinaryStreamReader&>()) } -> is_ErrorOr_or_Value<T>;
};

template <Serializable T>
std::vector<uint8_t> serializeToVector(const T& serializable) {
    std::basic_ostringstream<uint8_t> oss;
    BinaryStreamWriter writer{ oss };
    serializable.serialize(writer);
    auto view = oss.rdbuf()->view();
    std::vector<uint8_t> data{ view.data(), view.data() + view.size() };
    return data;
};

class IReadWriteObserver {
public:
    virtual ~IReadWriteObserver() = default;

    virtual void observe(const uint8_t* data, int64_t count) = 0;
};

class AssertFullReadCoverage : public IReadWriteObserver {
public:
    AssertFullReadCoverage(int32_t size) : coverage(size) {
    }

    ~AssertFullReadCoverage() {
        auto it = std::find(coverage.begin(), coverage.end(), 0);
        assert(it == coverage.end() && "Full read coverage assert failed");
    }

    void observe(const uint8_t* data, int64_t count) override {
        if(!begin) // Assuming the first read is at the start
            begin = data;

        memset(&coverage[std::distance(begin, data)], 1, count);
    }

    const uint8_t* begin = nullptr;
    std::vector<uint8_t> coverage; // TODO:Could be done much more compact
};