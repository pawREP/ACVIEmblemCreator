#include "BinaryReader.h"

BinaryStreamReader::BinaryStreamReader(std::basic_istream<uint8_t>& stream, int exceptions) : stream(stream) {
    assert(stream.good());
    exceptionMask = stream.exceptions();
    if(exceptions != -1)
        stream.exceptions(exceptions);
    else
        stream.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
}

BinaryStreamReader ::~BinaryStreamReader() {
    if(stream.bad()) // Restore exceptions only if the stream isn't dead
        stream.exceptions(exceptionMask);
}

void BinaryStreamReader::seek(std::streampos pos) {
    stream.seekg(pos);
    assert(stream.good());
}

void BinaryStreamReader::seek(std::streamoff off, std::ios_base::seekdir way) {
    stream.seekg(off, way);
    assert(stream.good());
}

std::streampos BinaryStreamReader::tell() const {
    auto pos = stream.tellg();
    assert(stream.good());
    return pos;
}

BinaryStreamReader::operator bool() {
    return stream.good();
}

bool BinaryStreamReader::headAtEof() {
    auto ex = stream.exceptions();
    stream.exceptions(0);
    char c;
    stream.read((uint8_t*)&c, sizeof(c));
    auto isEof = stream.eof();
    stream.clear();
    seek(-1 * (int64_t)sizeof(c), std::ios_base::cur);
    stream.exceptions(ex);
    assert(stream.good());
    return isEof;
}

std::string BinaryStreamReader::readString() {
    std::string str;
    while(true) {
        char c = read<char>();
        if(c == '\0')
            break;
        str += c;
    };
    return str;
}

std::wstring BinaryStreamReader::readWString() {
    std::wstring str;
    while(true) {
        wchar_t c = read<wchar_t>();
        if(c == (wchar_t)0)
            break;
        str += c;
    };
    return str;
}
