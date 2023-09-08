#pragma once
#include "SimpleMath.h"

class ChromaKey {
public:
    bool match(const DirectX::SimpleMath::Vector4& rgba);

    DirectX::SimpleMath::Vector3 elementThresholds = { 0.f, 0.f, 0.f };
    float distanceThreshold                        = 0.f;
    DirectX::SimpleMath::Vector3 key               = { 0.f, 1.f, 0.f };
};