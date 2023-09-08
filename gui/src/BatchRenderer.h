#pragma once
#include "BatchRenderer.h"
#include "CommonStates.h"
#include "Effects.h"
#include "PrimitiveBatch.h"
#include "VertexTypes.h"
#include <d3d11.h>
#include <memory>
#include <wrl.h>

namespace Geom {

    enum class AAMode {
        None,
        Msaa4x,
    };

    class BatchRenderer {
    public:
        using VertexType = DirectX::VertexPositionColor;

        static std::unique_ptr<BatchRenderer> create(ID3D11Device* device, AAMode aaMode);

        void begin(ID3D11DeviceContext* context);
        void draw(D3D11_PRIMITIVE_TOPOLOGY topology, const VertexType* vertices, int32_t vertexCount);
        void drawIndexed(D3D11_PRIMITIVE_TOPOLOGY topology, const uint16_t* indices, int32_t indexCount, const VertexType* vertices, int32_t vertexCount);
        void end();

        //DirectX::PrimitiveBatch<VertexType>* getPrimitiveBatch(); // TODO: remove

    private:
        BatchRenderer() = default;

        Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer = nullptr;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout    = nullptr;

        std::unique_ptr<DirectX::CommonStates> commonStates        = nullptr;
        std::unique_ptr<DirectX::PrimitiveBatch<VertexType>> batch = nullptr;
        std::unique_ptr<DirectX::BasicEffect> effect               = nullptr;
    };
} // namespace Geom
