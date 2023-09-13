#include "Compression.h"
#include "zlib.h"

std::vector<uint8_t> deflate(const uint8_t* data, int32_t inflatedSize) {
    std::vector<uint8_t> result;
    result.resize(inflatedSize); // TODO: Could probably fail in some extreme cases, like 1 byte input.

    z_stream defstream;
    defstream.zalloc = Z_NULL;
    defstream.zfree  = Z_NULL;
    defstream.opaque = Z_NULL;

    defstream.avail_in  = (uInt)inflatedSize;
    defstream.next_in   = (Bytef*)data;
    defstream.avail_out = (uInt)inflatedSize;
    defstream.next_out  = (Bytef*)result.data();

    deflateInit(&defstream, Z_BEST_COMPRESSION);
    deflate(&defstream, Z_FINISH);
    deflateEnd(&defstream);

    result.resize(defstream.total_out);

    return result;
}

std::vector<uint8_t> inflate(const uint8_t* data, int32_t deflatedSize, int32_t inflatedSize) {
    std::vector<uint8_t> result;
    result.resize(inflatedSize);

    z_stream defstream;
    defstream.zalloc = Z_NULL;
    defstream.zfree  = Z_NULL;
    defstream.opaque = Z_NULL;

    defstream.avail_in  = (uInt)deflatedSize;
    defstream.next_in   = (Bytef*)data;
    defstream.avail_out = (uInt)inflatedSize;
    defstream.next_out  = (Bytef*)result.data();

    inflateInit(&defstream);
    inflate(&defstream, Z_FINISH);
    inflateEnd(&defstream);

    return result;
}