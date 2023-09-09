#pragma once
#include "Error.h"
#include "json.h"
#include <cinttypes>
#include <optional>
#include <vector>

struct BoundingBox {
    int minX = (std::numeric_limits<int>::max)();
    int minY = (std::numeric_limits<int>::max)();
    int maxX = (std::numeric_limits<int>::min)();
    int maxY = (std::numeric_limits<int>::min)();

    static BoundingBox fromJson(const nlohmann::json& json);
};

class Emblem {
public:
    static ErrorOr<std::vector<Emblem>> fromJson(const nlohmann::json& json);

    std::vector<uint8_t> serialize() const;

    struct RGBA {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

    enum class DecalType : short {
        SquareSolid  = 100,
        EllipseSolid = 102,
    };

    enum class LayerType : short {
        GroupLayer  = 0,
        SingleLayer = 1,
    };

    struct LayerDesc {
        short subLayerCount = 3; // This value is fixed for LayerType::SingleLayer
        LayerType type      = LayerType::SingleLayer;
        DecalType decalId; // 0x4000 bit encodes 'inverted' toggle
        short posX;        // scaled by 0x10
        short posY;        // scaled by 0x10
        short scaleX;      // scaled by 0x10
        short scaleY;      // scaled by 0x10
        short angle;       // 0-360
        RGBA rgba;
        short toggledMask = 0; // 0x0000 default, 0x0040 toggle once, 0x0080 toggle twice
        short _padding{};
    };
    static_assert(sizeof(LayerDesc) == 0x18);
    static_assert(std::is_trivially_copyable_v<LayerDesc>);

    uint8_t category = 1;             // User slot
    std::optional<int64_t> creatorId; // SteamId, only for downloaded emblems
    std::wstring ugcId;               // Share code for downloaded emblems, otherwise "";
    uint8_t dateTime[0x10] = {
        0x70, 0x96, 0x4A, 0xDA, 0x43, 0xD8, 0xD9, 0x01, 0xE7, 0x77, 0x05, 0x5A, 0x47, 0x12, 0x04, 0x00
    }; // Default value cause lazy + pointless + can't be bothered to figure out the encoding

    std::vector<LayerDesc> layers;

private:
    Emblem() = default;

    static ErrorOr<Emblem> fromJson(const nlohmann::json& json, const BoundingBox& bb);
};
