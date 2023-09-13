#pragma once
#include <vector>

std::vector<uint8_t> deflate(const uint8_t* data, int32_t inflatedSize);
std::vector<uint8_t> inflate(const uint8_t* data, int32_t deflatedSize, int32_t inflatedSize);
