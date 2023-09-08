#include "ChromaKey.h"
#include "SimpleMath.h"
#include <bit>
#include <cstdlib>

using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;

bool ChromaKey::match(const Vector4& rgba_) {
    Vector3 rgb{ rgba_ };
    auto distance = Vector3::Distance(rgb, key);
    if(distance <= distanceThreshold)
        return true;

    Vector3 absDiff{ XMVectorAbs(rgb - key) };
    Vector3 result{};
    Vector3::Max(absDiff, elementThresholds, result);
    if(result != absDiff)
        return true;

    return false;
}