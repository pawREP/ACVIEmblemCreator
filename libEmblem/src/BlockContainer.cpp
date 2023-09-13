#include "BlockContainer.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "Error.h"
#include <vector>

ErrorOr<BlockContainer> BlockContainer::deserialize(BinaryStreamReader& reader) {
    BlockContainer container;
    try {
        bool firstBlock = true;
        while(true) {
            auto name = reader.readStringFromFixedLengthBuffer<0x10>();
            auto size = reader.read<uint32_t>();
            auto unk  = reader.read<uint32_t>();
            reader.readExpected<uint64_t>(0);

            // Delimiter blocks
            if(firstBlock) {
                assert(name == "---- begin ----");
                assert(size == 0);
                firstBlock = false;
                continue;
            }
            if(name == "----  end  ----") {
                assert(size == 0);
                break;
            }

            std::vector<uint8_t> data(size);
            reader.read(data.data(), size);

            container.blocks.emplace_back(std::pair<std::string, std::vector<uint8_t>>{ std::move(name), std::move(data) });
        }
    } catch(const std::ios::failure& e) {
        return Error{ "BlockContainer Parsing Error" };
    }

    return container;
}

void BlockContainer::serialize(BinaryStreamWriter& writer) {
    std::string begin = "---- begin ----";
    std::string end   = "----  end  ----";
    writer.write(begin);
    writer.write<uint64_t>(0);
    writer.write<uint64_t>(0);

    for(const auto& block : blocks) {
        const auto& name = block.first;
        const auto& data = block.second;

        writer.writeStringAsFixedLengthBuffer<0x10>(name);
        writer.write(data.size());
        writer.write<uint64_t>(0);
        writer.write(data.data(), data.size());
    }

    writer.write(end);
    writer.write<uint64_t>(0);
    writer.write<uint64_t>(0);
}

const std::vector<uint8_t>* BlockContainer::getBlockData(const std::string& name) {
    auto it = std::find_if(blocks.begin(), blocks.end(), [name](const auto& pair) { return pair.first == name; });
    if(it == blocks.end())
        return nullptr;
    return &it->second;
}

void BlockContainer::insert(const std::string& name, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> data_{ data };
    blocks.push_back({ name, std::move(data_) });
}

void BlockContainer::insert(const std::string& name, const uint8_t* data, int64_t size) {
    std::vector<uint8_t> data_{ data, data + size };
    blocks.push_back({ name, std::move(data_) });
}
