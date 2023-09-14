#include "UserData.h"
#include "Checksum.h"
#include "Compression.h"
#include "md5.h"
#include "zlib.h"
#include <array>
#include <span>
#include <spanstream>

ErrorOr<UserDataContainer> UserDataContainer::deserialize(BinaryStreamReader& reader) {
    UserDataContainer container;
    reader.read(container.IV, sizeof(IV));
    static_assert(std::is_trivially_copy_assignable_v<Header>);
    container.header = reader.read<Header>();

    for(int i = 0; i < container.header.fileCount; ++i) {
        auto fileOr = UserDataFile::deserialize(reader);
        PROPAGATE_IF_ERROR(fileOr);
        container.files_.push_back(std::move(fileOr).value());
    }
    int extraFileCount = reader.read<int>();
    for(int i = 0; i < extraFileCount; ++i) {
        auto fileOr = UserDataFile::deserialize(reader);
        PROPAGATE_IF_ERROR(fileOr);
        container.extraFiles_.push_back(std::move(fileOr).value());
    }

    return container;
}

namespace {
    class MD5WriteObserver : public IReadWriteObserver {
    public:
        void observe(const uint8_t* data, int64_t count) override {
            md5.update(data, count);
        }

        auto finalize() {
            return md5.finalize();
        }

    private:
        MD5 md5;
    };

} // namespace

void UserDataContainer::serialize(BinaryStreamWriter& writer) const {
    auto md5Observer = std::make_unique<MD5WriteObserver>();

    writer.write(IV);
    writer.write(header.size);
    writer.registerObserver(md5Observer.get());
    writer.write((int8_t*)&header.unk1, sizeof(header) - sizeof(header.size)); // Bit of an awkward write but we have to sandwich the md5 in here

    for(const auto& file : files_) {
        file.serialize(writer);
    }

    int extraFilesCount = extraFiles_.size();
    writer.write(extraFilesCount);
    for(const auto& file : extraFiles_)
        file.serialize(writer);

    writer.seekWithPad(sizeof(header.size) + header.size);

    writer.unregisterObeserver(md5Observer.get());
    auto checksum = md5Observer->finalize();
    writer.write(checksum);

    writer.padToNextMultipleOf(0x10);
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

const std::vector<UserDataFile>& UserDataContainer::extraFiles() const {
    return extraFiles_;
}

ErrorOr<UserDataFile> UserDataFile::deserialize(BinaryStreamReader& reader) {
    UserDataFile file;

    auto header = reader.read<Header>();
    assert(header.unk == 0x00291222);
    assert(header.deflatedSize);
    assert(header.inflatedSize);

    file.type = std::string(std::begin(header.magic), std::end(header.magic));

    std::vector<uint8_t> deflatedData(header.deflatedSize);
    reader.read(deflatedData.data(), deflatedData.size());

    file.data = inflate(deflatedData.data(), deflatedData.size(), header.inflatedSize);

    return file;
}

void UserDataFile::serialize(BinaryStreamWriter& writer) const {
    Header header;
    std::copy(type.data(), type.data() + sizeof(header.magic), std::begin(header.magic));
    header.unk          = 0x00291222;
    auto deflatedData   = deflate(data.data(), data.size());
    header.deflatedSize = deflatedData.size();
    header.inflatedSize = data.size();

    writer.write(header);
    writer.write(deflatedData.data(), deflatedData.size());
}

ErrorOr<UserDataFile> UserDataFile::create(std::string_view magic, const std::vector<uint8_t>& data) {
    if(magic.size() != 4)
        return Error{ "Invalid magic value. Has to be 4 character string without terminator" };

    UserDataFile file;
    file.type = magic;
    file.data = data;

    return file;
}
