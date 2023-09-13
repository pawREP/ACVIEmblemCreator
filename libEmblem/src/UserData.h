#pragma once
#include "Define.h"
#include "Error.h"
#include <cassert>
#include <cinttypes>
#include <iosfwd>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <span>

struct UserDataFile {

    static ErrorOr<UserDataFile> create(std::string_view magic, const std::vector<uint8_t>& data);
    static ErrorOr<UserDataFile> deserialize(std::istream& stream);

    std::vector<uint8_t> serialize() const;
    void serialize(std::ostream& ostream) const;

    struct Header {
        char magic[4];
        int unk = 0x00291222;
        int deflatedSize;
        int inflatedSize;
    } header;

    std::vector<uint8_t> data;

private:
    UserDataFile() = default;
};

class UserDataContainer {
    MAKE_NONCOPYABLE(UserDataContainer);

public:
    UserDataContainer(UserDataContainer&&) noexcept            = default;
    UserDataContainer& operator=(UserDataContainer&&) noexcept = default;

    static ErrorOr<UserDataContainer> deserialize(const std::vector<uint8_t>& data);

    std::vector<uint8_t> serialize() const;

    void insertFile(UserDataFile&& file);
    void insertFile(const UserDataFile& file);
    void eraseFile(int64_t index);

    const std::vector<UserDataFile>& files() const;
    const std::vector<UserDataFile>& extraFiles() const;

private:
    UserDataContainer() = default;

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
