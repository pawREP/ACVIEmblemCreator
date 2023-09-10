#pragma once
#include <d3d11.h>
#include <string>
#include <vector>


struct ImageDimensions {
    int width;
    int height;

    operator bool();
    auto operator<=>(const ImageDimensions& other) const = default;
};

// Adhoc image data load function. Returns 256x256 32bit rgba pixel data from WIC image file, optionally with resizing.
std::vector<uint8_t>
loadWICImageAsRGBAPixelData(ID3D11Device* device, const std::wstring& filePath, ImageDimensions resizeDimensions = {});
