#pragma once
#include "Define.h"
#include "Error.h"
#include "Serialization.h"
#include <string>
#include <vector>

struct UserDataFile {

    static ErrorOr<UserDataFile> create(std::string_view magic, const std::vector<uint8_t>& data);
    static ErrorOr<UserDataFile> deserialize(BinaryStreamReader& reader);

    void serialize(BinaryStreamWriter& writer) const;
    void serialize(std::ostream& ostream) const;

    std::vector<uint8_t> data;
    std::string type;

private:
    struct Header {
        char magic[4];
        int unk;
        int deflatedSize;
        int inflatedSize;
    };

    UserDataFile() = default;
};

class UserDataContainer {
public:
    static ErrorOr<UserDataContainer> deserialize(BinaryStreamReader& reader);
    void serialize(BinaryStreamWriter& writer) const;

    void insertFile(UserDataFile&& file);
    void insertFile(const UserDataFile& file);
    void eraseFile(int64_t index);

    const std::vector<UserDataFile>& files() const;
    const std::vector<UserDataFile>& extraFiles() const;

private:
    uint8_t IV[0x10]{};
    struct Header {
        int size; // size of header and data without the size parameter and final padding
        int unk1;
        int unk2;
        int unk3;
        int unk4;
        int fileCount;
    } header{};

    std::vector<UserDataFile> files_;
    std::vector<UserDataFile> extraFiles_; // Those files are not referenced in the header file count. Downloaded emblem files occupy those slots
};
