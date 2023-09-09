#include "DateTime.h"

DateTime DateTime::fromFileTime(FILETIME fileTime) {
    SYSTEMTIME systemTime{};
    FileTimeToSystemTime(&fileTime, &systemTime);

    // clang-format off
    uint64_t cst{};
    cst = (uint64_t)systemTime.wSecond       & 0x3F;
    cst = (uint64_t)systemTime.wMinute       & 0x3F  | cst << 0x06;
    cst = (uint64_t)systemTime.wHour         & 0x1F  | cst << 0x05;
    cst = (uint64_t)systemTime.wDay          & 0x1F  | cst << 0x05;
    cst = (uint64_t)systemTime.wDayOfWeek    & 0x07  | cst << 0x03;
    cst = (uint64_t)systemTime.wMonth        & 0x0F  | cst << 0x04;
    cst = (uint64_t)systemTime.wMilliseconds & 0x3FF | cst << 0x0A;
    cst = (uint64_t)systemTime.wYear                 | cst << 0x0C;
    // clang-format on

    return { .fileTime = fileTime, .compressedSystemTime = cst };
}

DateTime DateTime::fromCurrentTime() {
    FILETIME fileTime{};
    GetSystemTimeAsFileTime(&fileTime);

    return fromFileTime(fileTime);
}
