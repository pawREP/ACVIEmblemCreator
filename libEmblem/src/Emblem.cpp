#include "Emblem.h"
#include "BlockContainer.h"
#include "Hash.h"
#include <Windows.h>
#include <span>
#include <spanstream>

libEmblem::Image libEmblem::Image::deserialize(BinaryStreamReader& reader) {
    Image image;
    reader.readExpected<short>(0);
    auto layerCount = reader.read<short>();
    while(layerCount--) {
        image.layers.push_back(deserializeLayer(reader));
    }

    return image;
}

void libEmblem::Image::serialize(BinaryStreamWriter& writer) const {
    writer.write<short>(0);
    writer.write<short>(layers.size());
    for(const auto& layer : layers)
        serializeLayer(writer, layer);
}

libEmblem::Image::Group libEmblem::Image::deserializeGroup(BinaryStreamReader& reader) {
    Group group;
    group.data = reader.read<GroupData>();
    if((group.data.decalId & 0x3F00) == 0x3F00) {
        auto childCount = reader.read<int>();
        group.children.resize(childCount);
        for(int i = 0; i < childCount; ++i) {
            group.children[i] = deserializeGroup(reader);
        }
    }
    return group;
}

libEmblem::Image::Layer libEmblem::Image::deserializeLayer(BinaryStreamReader& reader) {
    Layer layer;

    reader.readExpected<short>(3);
    reader.readExpected<short>(1);
    layer.group = deserializeGroup(reader);

    return layer;
}

void libEmblem::Image::serializeGroup(BinaryStreamWriter& writer, const Group& group) const {
    writer.write(group.data);
    if(group.children.size())
        writer.write((int)group.children.size());
    for(const auto& child : group.children)
        serializeGroup(writer, child);
}

void libEmblem::Image::serializeLayer(BinaryStreamWriter& writer, const Layer& layer) const {
    writer.write<short>(3);
    writer.write<short>(1);
    serializeGroup(writer, layer.group);
}

libEmblem::Category& libEmblem::Category::operator=(uint8_t category) {
    this->category = category;
    return *this;
}

libEmblem::CreatorID& libEmblem::CreatorID::operator=(int64_t steamdId) {
    this->steamId = steamId;
    return *this;
}

libEmblem::UgcID& libEmblem::UgcID::operator=(const wchar_t* shareCode) {
    this->shareCode = shareCode;
    return *this;
}

libEmblem::DateTime libEmblem::DateTime::deserialize(BinaryStreamReader& reader) {
    DateTime datetime;
    datetime.filetime = reader.read<FILETIME>();
    reader.read<uint64_t>();
    return datetime;
}

libEmblem::DateTime libEmblem::DateTime::fromCurrentTime() {
    DateTime dateTime;
    GetSystemTimeAsFileTime(&dateTime.filetime);
    return dateTime;
}

void libEmblem::DateTime::serialize(BinaryStreamWriter& writer) const {
    SYSTEMTIME systemTime;
    FileTimeToSystemTime(&filetime, &systemTime);

    auto pst = packSystemTime(systemTime);

    writer.write(filetime);
    writer.write(pst);
}

constexpr uint64_t libEmblem::DateTime::packSystemTime(const SYSTEMTIME& systemTime) {
    // clang-format off
        uint64_t cst{};
        cst = (uint64_t)systemTime.wSecond       & 0x3F;
        cst = (uint64_t)systemTime.wMinute       & 0x3F  | cst << 0x06;
        cst = (uint64_t)systemTime.wHour         & 0x1F  | cst << 0x05;
        cst = (uint64_t)systemTime.wDay          & 0x1F  | cst << 0x05;
        cst = (uint64_t)systemTime.wDayOfWeek    & 0x07  | cst << 0x03;
        cst = (uint64_t)systemTime.wMonth        & 0x0F  | cst << 0x04;
        cst = (uint64_t)systemTime.wMilliseconds & 0x3FF | cst << 0x0A;
        cst = (uint64_t)systemTime.wYear         & 0xFFF | cst << 0x0C;
    // clang-format on
    return cst;
}

libEmblem::Category libEmblem::Category::deserialize(BinaryStreamReader& reader) {
    Category category;
    category.category = reader.read<int8_t>();
    return category;
}

void libEmblem::Category::serialize(BinaryStreamWriter& writer) const {
    writer.write(category);
}

libEmblem::CreatorID libEmblem::CreatorID::deserialize(BinaryStreamReader& reader) {
    CreatorID creatorId;
    creatorId.steamId = reader.read<int64_t>();
    return creatorId;
}

void libEmblem::CreatorID::serialize(BinaryStreamWriter& writer) const {
    writer.write(steamId);
}

libEmblem::UgcID libEmblem::UgcID::deserialize(BinaryStreamReader& reader) {
    UgcID ugcId;
    ugcId.shareCode = reader.readCWString();
    return ugcId;
}

void libEmblem::UgcID::serialize(BinaryStreamWriter& writer) const {
    writer.write(shareCode);
}

libEmblem::ErrorOr<libEmblem::EMBC> libEmblem::EMBC::deserialize(BinaryStreamReader& reader) {
    auto container = BlockContainer::deserialize(reader);
    if(!container)
        return std::unexpected{ container.error() };

    EMBC embc;
    for(const auto& block : container->blocks) {
        std::basic_ispanstream<uint8_t> ss{ std::span{ block.second.data(), block.second.size() } };
        BinaryStreamReader reader(ss);

        auto name = hash(block.first.c_str());
        switch(name) {
        case "Category"_hash:
            embc.category = Category::deserialize(reader);
            break;
        case "UgcID"_hash:
            embc.ugcId = UgcID::deserialize(reader);
            break;
        case "CreatorID"_hash:
            embc.creatorId = CreatorID::deserialize(reader);
            break;
        case "DateTime"_hash:
            embc.dateTime = DateTime::deserialize(reader);
            break;
        case "Image"_hash:
            embc.image = Image::deserialize(reader);
            break;
        }
    }
    return embc;
}

void libEmblem::EMBC::serialize(BinaryStreamWriter& writer) const {
    BlockContainer container;

    container.insert(category.name, category);
    container.insert(ugcId.name, ugcId);
    if(creatorId)
        container.insert(creatorId->name, creatorId.value());
    container.insert(dateTime.name, dateTime);
    container.insert(image.name, image);

    container.serialize(writer);
}
