#include "BinaryWriter.h"

std::vector<uint8_t>&& SequentialBinaryWriter::release() {
    return std::move(buffer);
}

int64_t SequentialBinaryWriter::size() const {
    return buffer.size();
}

void SequentialBinaryWriter::growBufferBy(size_t size) {
    buffer.resize(buffer.size() + size);
}
