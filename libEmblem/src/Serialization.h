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