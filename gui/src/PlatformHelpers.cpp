#include "PlatformHelpers.h"
#include <stdexcept>

void ThrowIfFailed(HRESULT hr) {
    if(FAILED(hr)) {
        throw std::runtime_error("bad hr");
    }
}
