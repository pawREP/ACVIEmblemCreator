#include "BinaryWriter.h"
#include "Serialization.h"

libEmblem::BinaryStreamWriterBase::BinaryStreamWriterBase(std::basic_ostream<uint8_t>& stream, int exceptions)
: stream(stream) {
    assert(stream.good());
    exceptionMask = stream.exceptions();
    if(exceptions != -1)
        stream.exceptions(exceptions);
    else
        stream.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
}

libEmblem::BinaryStreamWriterBase ::~BinaryStreamWriterBase() {
    if(stream.bad()) // Restore exceptions only if the stream isn't dead
        stream.exceptions(exceptionMask);
}

void libEmblem::BinaryStreamWriterBase::seek(std::streampos pos) {
    stream.seekp(pos);
    assert(stream.good());
}

void libEmblem::BinaryStreamWriterBase::seek(std::streamoff off, std::ios_base::seekdir way) {
    stream.seekp(off, way);
    assert(stream.good());
}

void libEmblem::BinaryStreamWriterBase::seekWithPad(std::streampos pos) {
    auto cur = tell();
    assert(pos >= cur && "Tried to seek backwards with padding");

    const uint8_t buf[0x10]{};
    auto distance = pos - cur;
    for(int i = 0; i < distance / sizeof(buf); ++i)
        write(buf, sizeof(buf));
    for(int i = 0; i < distance % sizeof(buf); ++i)
        write(buf, 1);
}

std::streampos libEmblem::BinaryStreamWriterBase::tell() const {
    auto pos = stream.tellp();
    assert(stream.good());
    return pos;
}

libEmblem::BinaryStreamWriterBase::operator bool() {
    return stream.good();
}

void libEmblem::BinaryStreamWriterBase::write(const uint8_t* data, int64_t count) {
    for(auto observer : observers)
        observer->observe(data, count);

    stream.write(data, count);
    assert(stream.good());
}

void libEmblem::BinaryStreamWriterBase::registerObserver(IReadWriteObserver* observer) {
    observers.push_back(observer);
}

void libEmblem::BinaryStreamWriterBase::unregisterObeserver(IReadWriteObserver* observer) {
    auto it = std::find(observers.begin(), observers.end(), observer);
    if(it != observers.end())
        observers.erase(it);
}

libEmblem::BinaryStreamWriter::BinaryStreamWriter(std::basic_ostream<uint8_t>& stream, int exceptions)
: BinaryStreamWriterBase(stream, exceptions) {
}

void libEmblem::BinaryStreamWriter::padToNextMultipleOf(int32_t alignment) {
    if(tell() / alignment == 0)
        return;

    auto padLen = alignment - (tell() % alignment);
    seekWithPad(tell() + padLen);
}
