#include "BinaryWriter.h"

BinaryStreamWriter::BinaryStreamWriter(std::basic_ostream<uint8_t>& stream, int exceptions) : stream(stream) {
    assert(stream.good());
    exceptionMask = stream.exceptions();
    if(exceptions != -1)
        stream.exceptions(exceptions);
    else
        stream.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
}

BinaryStreamWriter ::~BinaryStreamWriter() {
    if(stream.bad()) // Restore exceptions only if the stream isn't dead
        stream.exceptions(exceptionMask);
}

void BinaryStreamWriter::seek(std::streampos pos) {
    stream.seekp(pos);
    assert(stream.good());
}

void BinaryStreamWriter::seek(std::streamoff off, std::ios_base::seekdir way) {
    stream.seekp(off, way);
    assert(stream.good());
}

std::streampos BinaryStreamWriter::tell() const {
    auto pos = stream.tellp();
    assert(stream.good());
    return pos;
}

BinaryStreamWriter::operator bool() {
    return stream.good();
}
