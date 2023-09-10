#pragma once
#include "SimpleMath.h"
#include "json.h"
#include <d3d11.h>
#include <numbers>
#include <variant>
#include <vector>

template <typename VertexT>
struct RenderPrim {
public:
    D3D_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    std::vector<VertexT> vertices;
};

struct RGBA {
    float r;
    float g;
    float b;
    float a;
};

struct RectangleDesc {
    float cx;
    float cy;
    float w;
    float h;
    float angle;
    RGBA rgba;
};

struct EllipseDesc {
    float cx;
    float cy;
    float ax;
    float ay;
    float angle;
    RGBA rgba;
};

void rotatePoint(float& x, float& y, float theta);

template <typename VertexT, int angleResolution>
class EllipseRenderPrim {
public:
    static RenderPrim<VertexT> create(const EllipseDesc& desc) {
        RenderPrim<VertexT> prim;

        prim.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        prim.vertices.resize(3 * 360 / angleResolution);

        DirectX::SimpleMath::Vector2 points[360 / angleResolution + 1]; //+1 for convenience
        getPointsOnEllipse<angleResolution>(desc.cx, desc.cy, desc.ax, desc.ay, desc.angle, points);

        DirectX::XMVECTORF32 color = { desc.rgba.r, desc.rgba.g, desc.rgba.b, desc.rgba.a };
        for(int i = 0; i < prim.vertices.size(); i += 3) {
            prim.vertices[i]     = { { desc.cx, desc.cy, 0.5f }, color };
            prim.vertices[i + 1] = { { points[i / 3].x, points[i / 3].y, 0.5f }, color };
            prim.vertices[i + 2] = { { points[(i / 3 + 1)].x, points[(i / 3 + 1)].y, 0.5f }, color };
        }

        return prim;
    }

private:
    template <int angleResolution>
    static void
    getPointsOnEllipse(float centerX, float centerY, float majorAxis, float minorAxis, float ellipseAngle, DirectX::SimpleMath::Vector2* vertices) {
        int idx = 0;
        for(int angleDeg = 0; angleDeg <= 360; angleDeg += angleResolution) {
            float angleRad = angleDeg * std::numbers::pi / 180.0; // TODO: Make everthing rads

            vertices[idx] = { majorAxis * cos(angleRad), minorAxis * sin(angleRad) };
            rotatePoint(vertices[idx].x, vertices[idx].y, ellipseAngle * std::numbers::pi / 180.0);
            vertices[idx].x += centerX;
            vertices[idx].y += centerY;
            idx++;
        }
    }
};

template <typename VertexT>
class RectangleRenderPrim {
public:
    static RenderPrim<VertexT> create(const RectangleDesc& desc) {
        RenderPrim<VertexT> prim;

        prim.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        prim.vertices.resize(6);

        DirectX::SimpleMath::Vector2 points[4];

        points[0].x = -desc.w / 2;
        points[0].y = -desc.h / 2;
        points[1].x = desc.w / 2;
        points[1].y = -desc.h / 2;
        points[2].x = desc.w / 2;
        points[2].y = desc.h / 2;
        points[3].x = -desc.w / 2;
        points[3].y = desc.h / 2;

        for(auto& p : points)
            rotatePoint(p.x, p.y, desc.angle * std::numbers::pi / 180.0);

        DirectX::XMVECTORF32 color = { desc.rgba.r, desc.rgba.g, desc.rgba.b, desc.rgba.a };

        prim.vertices[0] = { { points[0].x + desc.cx, -(points[0].y + desc.cy), 0.5f }, color };
        prim.vertices[1] = { { points[1].x + desc.cx, -(points[1].y + desc.cy), 0.5f }, color };
        prim.vertices[2] = { { points[2].x + desc.cx, -(points[2].y + desc.cy), 0.5f }, color };
        prim.vertices[3] = { { points[0].x + desc.cx, -(points[0].y + desc.cy), 0.5f }, color };
        prim.vertices[4] = { { points[2].x + desc.cx, -(points[2].y + desc.cy), 0.5f }, color };
        prim.vertices[5] = { { points[3].x + desc.cx, -(points[3].y + desc.cy), 0.5f }, color };

        return prim;
    }
};

using ShapeDesc = std::variant<RectangleDesc, EllipseDesc>;

std::vector<ShapeDesc> fromGeomtrizeJson(const nlohmann::json& json);
