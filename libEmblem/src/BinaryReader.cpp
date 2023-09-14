#include "BinaryReader.h"
#include "Serialization.h"

libEmblem::BinaryStreamReaderBase::BinaryStreamReaderBase(std::basic_istream<uint8_t>& stream, int exceptions)
: stream(stream) {
    assert(stream.good());
    exceptionMask = stream.exceptions();
    if(exceptions != -1)
        stream.exceptions(exceptions);
    else
        stream.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
}

libEmblem::BinaryStreamReader::BinaryStreamReader(std::basic_istream<uint8_t>& stream, int exceptions)
: BinaryStreamReaderBase(stream, exceptions) {
}

libEmblem::BinaryStreamReaderBase::~BinaryStreamReaderBase() {
    if(stream.bad()) // Restore exceptions only if the stream isn't dead
        stream.exceptions(exceptionMask);
}

void libEmblem::BinaryStreamReaderBase::seek(std::streampos pos) {
    stream.seekg(pos);
    assert(stream.good());
}

void libEmblem::BinaryStreamReaderBase::seek(std::streamoff off, std::ios_base::seekdir way) {
    stream.seekg(off, way);
    assert(stream.good());
}

std::streampos libEmblem::BinaryStreamReaderBase::tell() const {
    auto pos = stream.tellg();
    assert(stream.good());
    return pos;
}

libEmblem::BinaryStreamReaderBase::operator bool() {
    return stream.good();
}

bool libEmblem::BinaryStreamReaderBase::headAtEof() {
    auto ex = stream.exceptions();
    stream.exceptions(0);
    uint8_t c;
    stream.read(&c, 1);
    auto isEof = stream.eof();
    stream.clear();
    seek(-1, std::ios_base::cur);
    stream.exceptions(ex);
    assert(stream.good());
    return isEof;
}

std::string libEmblem::BinaryStreamReader::readCString() {
    std::string str;
    while(true) {
        char c = read<char>();
        if(c == '\0')
            break;
        str += c;
    };
    return str;
}

std::wstring libEmblem::BinaryStreamReader::readCWString() {
    std::wstring str;
    while(true) {
        wchar_t c = read<wchar_t>();
        if(c == (wchar_t)0)
            break;
        str += c;
    };
    return str;
}

void libEmblem::BinaryStreamReaderBase::read(uint8_t* v, int64_t count) {
    stream.read(v, count);
    assert(stream.good());
}

void libEmblem::BinaryStreamReaderBase::registerObserver(IReadWriteObserver* observer) {
    observers.push_back(observer);
}

void libEmblem::BinaryStreamReaderBase::unregisterObeserver(IReadWriteObserver* observer) {
    auto it = std::find(observers.begin(), observers.end(), observer);
    if(it != observers.end())
        observers.erase(it);
}
