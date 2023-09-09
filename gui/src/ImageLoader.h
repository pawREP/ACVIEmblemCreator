#pragma once
#include <d3d11.h>
#include <string>
#include <vector>

std::vector<uint8_t> loadWICImageAsRGBAPixelData(ID3D11Device* device, const std::wstring& filePath);
