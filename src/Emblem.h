#pragma once
#include "Error.h"
#include "json.h"
#include <cinttypes>

class Emblem {
public:
    static ErrorOr<Emblem> fromJson(const nlohmann::json& json);

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

    struct LayerDesc {
        short unk0 = 3;
        short unk1 = 1;
        DecalType decalId; // 0x4000 bit encodes inverted toggle
        short posX;        // scaled by 0x10
        short posY;        // scaled by 0x10
        short scaleX;      // scaled by 0x10
        short scaleY;      // scaled by 0x10
        short angle;
        RGBA rgba;
        int toggledMask = 0; // 0x00400000 when toggled, probably multiple flags
    };
    static_assert(sizeof(LayerDesc) == 0x18);
    static_assert(std::is_trivially_copyable_v<LayerDesc>);

    uint8_t category       = 1; // Unkown purpose
    uint16_t ugcId         = 0; // Unkown purpose
    uint8_t dateTime[0x10] = {
        0x70, 0x96, 0x4A, 0xDA, 0x43, 0xD8, 0xD9, 0x01, 0xE7, 0x77, 0x05, 0x5A, 0x47, 0x12, 0x04, 0x00
    }; // Default value cause lazy + pointless + can't be bothered to figure out the encoding

    std::vector<LayerDesc> layers;

private:
    Emblem() = default;
};
