#include "PlatformHelpers.h"
#include <cassert>
#include <comdef.h>
#include <stdexcept>

void ThrowIfFailed(HRESULT hr) {
    if(FAILED(hr)) {
        throw bad_hr_exception(hr);
    }
}

bad_hr_exception::bad_hr_exception(HRESULT hr) {
    char* msgBuf;
    auto size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msgBuf, 0, NULL);
    assert(size);
    msg = msgBuf;
    LocalFree(msgBuf);
}

char const* bad_hr_exception::what() const {
    return msg.c_str();
}
