#include "EmblemImporter.h"
#include <utility>

using namespace libEmblem;

namespace {

    enum class GeometrizeShapeType {
        Rectangle        = 0,
        RotatedRectangle = 1,
        Triangle         = 2,
        Ellipse          = 3,
        RotatedEllipse   = 4,
        Circle           = 5,
    };

    bool shapeHasAngle(GeometrizeShapeType shape) {
        switch(shape) {
        case GeometrizeShapeType::Circle:
        case GeometrizeShapeType::Ellipse:
        case GeometrizeShapeType::Rectangle:
            return false;
        case GeometrizeShapeType::RotatedRectangle:
        case GeometrizeShapeType::RotatedEllipse:
            return true;
        default:
            assert(false);
        }
    }

    bool validateJsonFormat(const nlohmann::json& json) {
        if(!json.is_array())
            return false;

        for(const auto& entry : json) {
            if(!entry.is_object())
                return false;

            // Validate "color"
            {
                if(!entry.contains("color") ||   //
                   !entry["color"].is_array() || //
                   entry["color"].size() != 4)
                    return false;
            }
            // Validate "type" and "data"
            {
                if(!entry.contains("type") ||             //
                   !entry["type"].is_number_unsigned() || //
                   !entry.contains("data") ||             //
                   !entry["data"].is_array())
                    return false;

                switch(entry["type"].get<GeometrizeShapeType>()) {
                case GeometrizeShapeType::Circle:
                    if(entry["data"].size() != 3)
                        return false;
                    break;
                case GeometrizeShapeType::Ellipse:
                case GeometrizeShapeType::Rectangle: {
                    if(entry["data"].size() != 4)
                        return false;
                    break;
                case GeometrizeShapeType::RotatedRectangle:
                case GeometrizeShapeType::RotatedEllipse:
                    if(entry["data"].size() != 5)
                        return false;
                    break;
                default:
                    return false;
                }
                }
            }
        }
        return true;
    }

    struct BoundingBox {
        int minX = (std::numeric_limits<int>::max)();
        int minY = (std::numeric_limits<int>::max)();
        int maxX = (std::numeric_limits<int>::min)();
        int maxY = (std::numeric_limits<int>::min)();

        static BoundingBox fromJson(const nlohmann::json& json);
    };

    BoundingBox BoundingBox::fromJson(const nlohmann::json& json) {
        // TODO: This relays on the first entry in the json being a rectangle covering the whole image. That's a bit fragile and we should rethink this when a better json source comes around
        BoundingBox bb;

        bb.minX = json[0]["data"][0];
        bb.minY = json[0]["data"][1];
        bb.maxX = json[0]["data"][2];
        bb.maxY = json[0]["data"][3];

        return bb;
    }

    ErrorOr<Image> fromJson(const nlohmann::json& json, const BoundingBox& bb) {
        Image emblem;

        float scaleX = 256.f / (bb.maxX - bb.minX);
        float scaleY = 256.f / (bb.maxY - bb.minY);

        for(const auto& shape : json) {
            Image::Layer layer{};
            Image::GroupData& groupData = layer.group.data;

            groupData.rgba.r = shape["color"][0];
            groupData.rgba.g = shape["color"][1];
            groupData.rgba.b = shape["color"][2];
            groupData.rgba.a = 100.f * shape["color"][3].get<float>() / 255.f;
            groupData.angle  = 0;

            auto type = shape["type"].get<GeometrizeShapeType>();
            switch(type) {
            case GeometrizeShapeType::Rectangle:
            case GeometrizeShapeType::RotatedRectangle: {
                groupData.decalId = std::to_underlying(Image::DecalType::SquareSolid);

                auto minX = (shape["data"][0].get<float>() * scaleX - 128) * 0x10;
                auto minY = (shape["data"][1].get<float>() * scaleY - 128) * 0x10;
                auto maxX = (shape["data"][2].get<float>() * scaleX - 128) * 0x10;
                auto maxY = (shape["data"][3].get<float>() * scaleY - 128) * 0x10;

                groupData.posX   = std::midpoint(minX, maxX);
                groupData.posY   = std::midpoint(minY, maxY);
                groupData.scaleX = (maxX - minX);
                groupData.scaleY = (maxY - minY);
            } break;
            case GeometrizeShapeType::Circle:
            case GeometrizeShapeType::Ellipse:
            case GeometrizeShapeType::RotatedEllipse:
                groupData.decalId = std::to_underlying(Image::DecalType::EllipseSolid);
                groupData.posX    = (shape["data"][0] * scaleX - 128) * 0x10;
                groupData.posY    = (shape["data"][1] * scaleY - 128) * 0x10;
                groupData.scaleX  = shape["data"][2] * scaleX * 0x10 * 2;
                switch(type) {
                case GeometrizeShapeType::Circle:
                    groupData.scaleY = groupData.scaleX;
                    break;
                case GeometrizeShapeType::Ellipse:
                case GeometrizeShapeType::RotatedEllipse:
                    groupData.scaleY = shape["data"][3] * scaleY * 0x10 * 2;
                    break;
                }
                break;
            default:
                return Error{ "Invalid GeometrizeShapeType: {}\n"
                              "This tool supports only Rectangles, rotated Rectangles and rotated ellipses",
                              (int)shape["type"].get<GeometrizeShapeType>() };
            }

            if(shapeHasAngle(type)) {
                groupData.angle = shape["data"][4];
            }

            emblem.layers.push_back(layer);
        }
        return emblem;
    }

} // namespace

ErrorOr<std::vector<Image>> GeometrizeImporter::fromJson(const nlohmann::json& json) {
    if(!validateJsonFormat(json))
        return Error{ "Unsupported json format. Make sure to use the Geometrize Web App with the settings described in "
                      "the documentation on github. Json files generated by the desktop app are not supported." };

    BoundingBox bb = BoundingBox::fromJson(json);

    constexpr int perEmblemLayerCountMax = 128;

    auto entries     = json.get<std::vector<nlohmann::json>>();
    auto emblemCount = entries.size() / perEmblemLayerCountMax;
    emblemCount += entries.size() % perEmblemLayerCountMax == 0 ? 0 : 1;

    std::vector<Image> emblems;
    for(int i = 0; i < emblemCount; ++i) {
        nlohmann::json layer = nlohmann::json::array();

        int layerSize = perEmblemLayerCountMax;
        if(i == (emblemCount - 1) && entries.size() % perEmblemLayerCountMax)
            layerSize = entries.size() % perEmblemLayerCountMax;

        for(int j = 0; j < layerSize; ++j)
            layer.push_back(json[i * perEmblemLayerCountMax + j]);

        auto emblemOr = fromJson(layer, bb);
        if(!emblemOr)
            return emblemOr.error();

        emblems.push_back(emblemOr.value());
    }

    return emblems;
}
