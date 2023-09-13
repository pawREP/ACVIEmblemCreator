#pragma once
#include "Serialization.h"
#include "Error.h"
#include <vector>

class BlockContainer {
public:
    static ErrorOr<BlockContainer> deserialize(BinaryStreamReader& reader);

    void serialize(BinaryStreamWriter& writer);

    const std::vector<uint8_t>* getBlockData(const std::string& name);

    template <Serializable T>
    void insert(const std::string& name, const T& serializable) {
        std::basic_ostringstream<uint8_t> ss;
        {
            BinaryStreamWriter writer{ ss };
            serializable.serialize(writer);
        }
        auto view = ss.rdbuf()->view();
        std::vector<uint8_t> data({ view.data(), view.data() + view.size() });

        blocks.push_back({ name, std::move(data) });
    }

    void insert(const std::string& name, const std::vector<uint8_t>& data);
    void insert(const std::string& name, const uint8_t* data, int64_t size);

    std::vector<std::pair<std::string, std::vector<uint8_t>>> blocks;
};
