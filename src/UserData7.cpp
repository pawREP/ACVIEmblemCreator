#include "UserData7.h"
#include "md5.h"
#include "zlib.h"
#include <array>
#include <span>
#include <spanstream>

namespace {

    std::vector<uint8_t> deflateBuffer(const uint8_t* input, int inputSize) {
        std::vector<uint8_t> result;
        result.resize(inputSize); // TODO: Could probably fail in some extreme cases, like 1 byte input.

        z_stream defstream;
        defstream.zalloc = Z_NULL;
        defstream.zfree  = Z_NULL;
        defstream.opaque = Z_NULL;

        defstream.avail_in  = (uInt)inputSize;
        defstream.next_in   = (Bytef*)input;
        defstream.avail_out = (uInt)inputSize;
        defstream.next_out  = (Bytef*)result.data();

        deflateInit(&defstream, Z_BEST_COMPRESSION);
        deflate(&defstream, Z_FINISH);
        deflateEnd(&defstream);

        result.resize(defstream.total_out);
        return result;
    }

    std::array<uint8_t, 0x10> md5Digest(const uint8_t* data, int64_t size) {
        MD5Context ctx;
        md5Init(&ctx);
        md5Update(&ctx, const_cast<uint8_t*>(data), size); // This md5 library is sussy..
        md5Finalize(&ctx);

        std::array<uint8_t, 0x10> result;
        std::copy(std::begin(ctx.digest), std::end(ctx.digest), result.data());
        return result;
    };

} // namespace

ErrorOr<UserDataContainer> UserDataContainer::deserialize(const std::vector<uint8_t>& data) {
    std::ispanstream inputStream{ { (char*)data.data(), data.size() }, std::ios::in | std::ios::binary };

    UserDataContainer container;
    inputStream.read((char*)container.IV, sizeof(IV));
    static_assert(std::is_trivially_copy_assignable_v<Header>);
    inputStream.read((char*)&container.header, sizeof(Header));

    for(int i = 0; i < container.header.fileCount; ++i) {
        auto fileOr = UserDataFile::deserialize(inputStream);
        PROPAGATE_IF_ERROR(fileOr);
        container.files_.push_back(std::move(fileOr).value());
    }

    return container;
}

std::vector<uint8_t> UserDataContainer::serialize() const {
    size_t finalSize = sizeof(IV) + sizeof(header.size) + header.size;
    if(finalSize % 0x10)
        finalSize = finalSize + 0x10 - (finalSize % 0x10); // Round up to next multiple of 0x10

    std::vector<uint8_t> buffer(finalSize, 0);

    std::ospanstream ostream({ (char*)buffer.data(), buffer.size() }, std::ios::out | std::ios::binary);
    ostream.write((char*)IV, sizeof(IV));
    ostream.write((char*)&header, sizeof(Header));
    for(const auto& file : files_)
        file.serialize(ostream);

    // checsum
    auto checksum = md5Digest(buffer.data() + 0x14, header.size - 0x10);
    ostream.seekp(sizeof(header.size) + header.size);
    ostream.write((char*)checksum.data(), sizeof(checksum));

    return buffer;
}

void UserDataContainer::insertFile(UserDataFile&& file) {
    files_.push_back(std::move(file));
    header.fileCount++;
}

void UserDataContainer::insertFile(const UserDataFile& file) {
    files_.push_back(file);
    header.fileCount++;
}

void UserDataContainer::eraseFile(int64_t index) {
    files_.erase(files_.begin() + index);
    header.fileCount--;
}

const std::vector<UserDataFile>& UserDataContainer::files() const {
    return files_;
}

ErrorOr<UserDataFile> UserDataFile::deserialize(std::istream& stream) {
    UserDataFile file;
    static_assert(std::is_trivially_copy_assignable_v<Header>);
    stream.read((char*)&file.header, sizeof(Header));
    file.data.resize(file.header.deflatedSize);
    stream.read((char*)file.data.data(), file.data.size());

    return file;
}

std::vector<uint8_t> UserDataFile::serialize() const {
    std::vector<uint8_t> result(sizeof(Header) + data.size());
    std::memcpy(result.data(), reinterpret_cast<const uint8_t*>(&header), sizeof(Header));
    std::memcpy(result.data() + sizeof(Header), data.data(), data.size());
    return result;
}

void UserDataFile::serialize(std::ostream& ostream) const {
    ostream.write((char*)&header, sizeof(Header));
    ostream.write((char*)data.data(), data.size());
}

ErrorOr<UserDataFile> UserDataFile::create(std::string_view magic, const std::vector<uint8_t>& data) {
    if(magic.size() != 4)
        return Error{ "Invalid magic value. Has to be 4 character string without terminator" };

    UserDataFile file;
    std::memcpy(&file.header.magic, magic.data(), magic.size());
    file.header.inflatedSize = data.size();
    file.data                = deflateBuffer(data.data(), data.size());
    file.header.deflatedSize = file.data.size();

    return file;
}