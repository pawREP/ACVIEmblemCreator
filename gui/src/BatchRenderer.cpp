#include "BatchRenderer.h"
#include "DirectXHelpers.h"
#include "PlatformHelpers.h"
#include "VertexTypes.h"
#include <d3d11.h>

using Microsoft::WRL::ComPtr;

std::unique_ptr<Geom::BatchRenderer> Geom::BatchRenderer::create(ID3D11Device* device, AAMode aaMode) {
    auto renderer = std::unique_ptr<BatchRenderer>(new BatchRenderer());

    // switch(aaMode) {
    // case AAMode::Msaa4x: {
    //     CD3D11_RASTERIZER_DESC msaaRasterizerDesc(D3D11_FILL_SOLID, D3D11_CULL_NONE, FALSE, D3D11_DEFAULT_DEPTH_BIAS,
    //                                               D3D11_DEFAULT_DEPTH_BIAS_CLAMP,
    //                                               D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS, TRUE, FALSE, TRUE, FALSE);

    //    ThrowIfFailed(device->CreateRasterizerState(&msaaRasterizerDesc, renderer->rasterizer.ReleaseAndGetAddressOf()));
    //} break;
    // case AAMode::None:
    // default:
    //    //throw std::logic_error("Not implemented");
    //}

    renderer->commonStates = std::make_unique<DirectX::CommonStates>(device);

    ID3D11DeviceContext* context;
    device->GetImmediateContext(&context);
    renderer->batch = std::make_unique<DirectX::PrimitiveBatch<VertexType>>(context);
    context->Release();

    renderer->effect = std::make_unique<DirectX::BasicEffect>(device);
    renderer->effect->SetVertexColorEnabled(true);

    DirectX::CreateInputLayoutFromEffect<VertexType>(device, renderer->effect.get(), renderer->inputLayout.ReleaseAndGetAddressOf());

    return renderer;
}

void Geom::BatchRenderer::begin(ID3D11DeviceContext* context) {
    context->OMSetBlendState(commonStates->AlphaBlend(), nullptr, 0xFFFFFFFF);
    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(commonStates->DepthNone(), 0);

    auto sampler = commonStates->LinearClamp();
    context->PSSetSamplers(0, 1, &sampler);

    // context->RSSetState(rasterizer.Get());
    context->RSSetState(commonStates->CullNone());
    context->IASetInputLayout(inputLayout.Get());

    effect->Apply(context);
    batch->Begin();
}

void Geom::BatchRenderer::draw(D3D11_PRIMITIVE_TOPOLOGY topology, const VertexType* vertices, int32_t vertexCount) {
    batch->Draw(topology, vertices, vertexCount);
}

void Geom::BatchRenderer::drawIndexed(D3D11_PRIMITIVE_TOPOLOGY topology,
                                      const uint16_t* indices,
                                      int32_t indexCount,
                                      const VertexType* vertices,
                                      int32_t vertexCount) {
    batch->DrawIndexed(topology, indices, indexCount, vertices, vertexCount);
}

void Geom::BatchRenderer::end() {
    batch->End();
}
