#pragma once
#include "Error.h"
#include "Serialization.h"
#include <Windows.h>
#include <cinttypes>
#include <optional>
#include <span>
#include <spanstream>
#include <vector>

namespace libEmblem {

    struct DateTime {
        static constexpr char const* name = "DateTime";

        static DateTime deserialize(BinaryStreamReader& reader);
        static DateTime fromCurrentTime();

        void serialize(BinaryStreamWriter& writer) const;

        FILETIME filetime{};

    private:
        static constexpr uint64_t packSystemTime(const SYSTEMTIME& systemTime);
    };

    struct Category {
        static constexpr char const* name = "Category";

        Category& operator=(uint8_t category);

        static Category deserialize(BinaryStreamReader& reader);

        void serialize(BinaryStreamWriter& writer) const;

        int8_t category{};
    };

    struct CreatorID {
        static constexpr char const* name = "CreatorID";

        CreatorID& operator=(int64_t steamdId);

        static CreatorID deserialize(BinaryStreamReader& reader);

        void serialize(BinaryStreamWriter& writer) const;

        int64_t steamId{};
    };

    struct UgcID {
        static constexpr char const* name = "UgcID";

        UgcID& operator=(const wchar_t* shareCode);

        static UgcID deserialize(BinaryStreamReader& reader);
        void serialize(BinaryStreamWriter& writer) const;

        std::wstring shareCode;
    };

    class Image {
    public:
        static constexpr char const* name = "Image";

        struct RGBA {
            byte r;
            byte g;
            byte b;
            byte a;
        };

        enum class DecalType : short {
            SquareSolid  = 100,
            EllipseSolid = 102,
        };

        struct alignas(4) GroupData {
            short decalId; // 0x4000 bit encodes 'inverted' toggle, 0x3F00 mask indicates layerGroup
            short posX;    // scaled by 0x10
            short posY;    // scaled by 0x10
            short scaleX;  // scaled by 0x10
            short scaleY;  // scaled by 0x10
            short angle;   // 0-360
            RGBA rgba;
            short maskMode; // 0x000 default, 0x0040 toggle once, 0x0080 toggle twice
        };
        static_assert(sizeof(GroupData) == 20);

        struct Group {
            GroupData data;
            std::vector<Group> children;
        };

        struct Layer {
            Group group;
        };

        static Image deserialize(BinaryStreamReader& reader);
        void serialize(BinaryStreamWriter& writer) const;

        std::vector<Layer> layers;

    private:
        void serializeGroup(BinaryStreamWriter& writer, const Group& group) const;
        void serializeLayer(BinaryStreamWriter& writer, const Layer& layer) const;

        static Group deserializeGroup(BinaryStreamReader& reader);
        static Layer deserializeLayer(BinaryStreamReader& reader);
    };

    class EMBC {
    public:
        static ErrorOr<EMBC> deserialize(BinaryStreamReader& reader);
        void serialize(BinaryStreamWriter& writer) const;

        Category category;
        UgcID ugcId;
        std::optional<CreatorID> creatorId;
        DateTime dateTime;
        Image image;
    };

} // namespace libEmblem
