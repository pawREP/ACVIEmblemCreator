#include "BlockContainerBuilder.h"
#include <cassert>

void BlockContainerBuilder::beginContainer() {
    beginBlock(beginBlockName);
    endBlock();
    containerBalance.push({});
}

void BlockContainerBuilder::beginBlock(const std::string& name) {
    blockHeadStack.push(stream.tellp());

    Header header;
    std::copy(std::begin(name), std::end(name), header.name);
    stream.write(reinterpret_cast<char*>(&header), sizeof(Header));
}

void BlockContainerBuilder::endBlock() {
    assert(blockHeadStack.size());
    auto head = blockHeadStack.top();
    blockHeadStack.pop();
    auto pos      = stream.tellp();
    auto dataSize = static_cast<int64_t>(pos - head) - sizeof(Header);
    stream.seekp(head);
    stream.seekp(offsetof(Header, dataSize), std::ios::cur);
    stream.write(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
    stream.seekp(pos);
}

void BlockContainerBuilder::endContainer() {
    beginBlock(endBlockName);
    endBlock();
    assert(containerBalance.size());
    containerBalance.pop();
}

std::vector<uint8_t> BlockContainerBuilder::build() {
    assert(containerBalance.size() == 0);
    assert(blockHeadStack.size() == 0);

    auto view = stream.view();
    return std::vector<uint8_t>{ view.begin(), view.end() };
}
