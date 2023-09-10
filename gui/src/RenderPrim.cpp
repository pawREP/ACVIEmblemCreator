#include "RenderPrim.h"
#include <cmath>
#include <vector>

namespace {

    enum class GeometrizeShapeType {
        Rectangle        = 0,
        RotatedRectangle = 1,
        Triangle         = 2,
        Ellipse          = 3,
        RotatedEllipse   = 4,
        Circle           = 5,
    };

}

void rotatePoint(float& x, float& y, float theta) {
    double tempX = x;
    x            = x * cos(theta) - y * sin(theta);
    y            = tempX * sin(theta) + y * cos(theta);
}

std::vector<ShapeDesc> fromGeomtrizeJson(const nlohmann::json& json) {
    std::vector<ShapeDesc> shapes;

    struct BoundingBox {
        int minX = (std::numeric_limits<int>::max)();
        int minY = (std::numeric_limits<int>::max)();
        int maxX = (std::numeric_limits<int>::min)();
        int maxY = (std::numeric_limits<int>::min)();
    } bb;

    bb.minX = json[0]["data"][0];
    bb.minY = json[0]["data"][1];
    bb.maxX = json[0]["data"][2];
    bb.maxY = json[0]["data"][3];

    float scaleX = bb.maxX - bb.minX;
    float scaleY = bb.maxY - bb.minY;

    for(int i = 0; i < json.size(); ++i) {
        const auto& shapeJson = json[i];

        auto type = shapeJson["type"].get<GeometrizeShapeType>();
        switch(type) {
        case GeometrizeShapeType::Rectangle: {
        case GeometrizeShapeType::RotatedRectangle:
            RectangleDesc desc;
            auto scale = 1.5f;

            auto minX = (scale * (shapeJson["data"][0].get<float>() / scaleX)) - scale * 0.5f;
            auto minY = (scale * (shapeJson["data"][1].get<float>() / scaleY)) - scale * 0.5f;
            auto maxX = (scale * (shapeJson["data"][2].get<float>() / scaleX)) - scale * 0.5f;
            auto maxY = (scale * (shapeJson["data"][3].get<float>() / scaleY)) - scale * 0.5f;

            desc.w = (maxX - minX);
            desc.h = (maxY - minY);

            desc.cx = minX + desc.w / 2;
            desc.cy = minY + desc.h / 2;

            if(type == GeometrizeShapeType::RotatedRectangle)
                desc.angle = shapeJson["data"][4].get<float>();
            else
                desc.angle = 0.f;

            desc.rgba.r = shapeJson["color"][0] / 255.f;
            desc.rgba.g = shapeJson["color"][1] / 255.f;
            desc.rgba.b = shapeJson["color"][2] / 255.f;
            desc.rgba.a = shapeJson["color"][3] / 255.f; 

            shapes.push_back(desc);
        } break;
        case GeometrizeShapeType::Circle:
        case GeometrizeShapeType::Ellipse:
        case GeometrizeShapeType::RotatedEllipse: {
            float scale = 1.5f;
            EllipseDesc desc;

            desc.rgba.r = shapeJson["color"][0] / 255.f;
            desc.rgba.g = shapeJson["color"][1] / 255.f;
            desc.rgba.b = shapeJson["color"][2] / 255.f;
            desc.rgba.a = shapeJson["color"][3] / 255.f; 

            desc.cx = scale * (shapeJson["data"][0].get<float>() / scaleX) - scale * .5f;
            desc.cy = scale * ((1.f - shapeJson["data"][1].get<float>() / scaleY)) - scale * .5f;
            desc.ax = scale * (shapeJson["data"][2].get<float>() / scaleX);

            switch(type) {
            case GeometrizeShapeType::Circle:
                desc.ay    = desc.ax;
                desc.angle = 0.f;
                break;
            case GeometrizeShapeType::Ellipse:
                desc.ay    = scale * (shapeJson["data"][3].get<float>() / scaleY);
                desc.angle = 0.f;
                break;
            case GeometrizeShapeType::RotatedEllipse:
                desc.ay    = scale * (shapeJson["data"][3].get<float>() / scaleY);
                desc.angle = -1.f * shapeJson["data"][4].get<float>();
                break;
            }
            shapes.push_back(desc);
        } break;
        default:
            throw std::runtime_error("Unreachable");
        }
    }
    return shapes;
}
