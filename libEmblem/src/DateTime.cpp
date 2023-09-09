#include "DateTime.h"

namespace {

    constexpr uint64_t packSystemTime(const SYSTEMTIME& systemTime) {
        // clang-format off
        uint64_t cst{};
        cst = (uint64_t)systemTime.wSecond       & 0x3F;
        cst = (uint64_t)systemTime.wMinute       & 0x3F  | cst << 0x06;
        cst = (uint64_t)systemTime.wHour         & 0x1F  | cst << 0x05;
        cst = (uint64_t)systemTime.wDay          & 0x1F  | cst << 0x05;
        cst = (uint64_t)systemTime.wDayOfWeek    & 0x07  | cst << 0x03;
        cst = (uint64_t)systemTime.wMonth        & 0x0F  | cst << 0x04;
        cst = (uint64_t)systemTime.wMilliseconds & 0x3FF | cst << 0x0A;
        cst = (uint64_t)systemTime.wYear         & 0xFFF | cst << 0x0C;
        // clang-format on
        return cst;
    }

} // namespace

DateTime DateTime::fromFileTime(FILETIME fileTime) {
    SYSTEMTIME systemTime{};
    FileTimeToSystemTime(&fileTime, &systemTime);

    return { .fileTime = fileTime, .packedSystemTime = packSystemTime(systemTime) };
}

DateTime DateTime::fromCurrentTime() {
    FILETIME fileTime{};
    GetSystemTimeAsFileTime(&fileTime);

    return fromFileTime(fileTime);
}
