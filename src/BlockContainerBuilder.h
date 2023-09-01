#pragma once
#include <cstdint>
#include <sstream>
#include <stack>
#include <vector>

class BlockContainerBuilder {
public:
    void beginContainer();
    void beginBlock(const std::string& name);
    void endBlock();
    void endContainer();

    template <typename T>
    void writeBlock(const std::string& name, const T& t) {
        beginBlock(name);
        write(t);
        endBlock();
    }

    template <typename T>
    void writeBlock(const std::string& name, const T* arr, int size) {
        beginBlock(name);
        write(arr, size);
        endBlock();
    }

    template <typename T>
    void write(const T& t) {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(!std::is_pointer_v<T>);

        stream.write(reinterpret_cast<const char*>(&t), sizeof(T));
    }

    template <typename T>
    void write(const T* arr, int size) {
        static_assert(std::is_trivially_copyable_v<T>);

        stream.write(reinterpret_cast<const char*>(arr), size * sizeof(T));
    }

    std::vector<uint8_t> build();

private:
    struct Header {
        char name[0x10]{};
        uint32_t dataSize{};
        uint32_t unk{};
        uint64_t _paddding{};
    };

    struct Empty {};

    static inline constexpr char beginBlockName[0x10] = "---- begin ----";
    static inline constexpr char endBlockName[0x10]   = "----  end  ----";

    std::stringstream stream;
    std::stack<std::streampos> blockHeadStack;
    std::stack<Empty> containerBalance;
};
