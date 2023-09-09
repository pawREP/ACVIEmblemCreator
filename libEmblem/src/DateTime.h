#pragma once
#include <Windows.h>
#include <cinttypes>

/// <summary>
/// Engine internal date time format. Don't change layout.
/// </summary>
struct DateTime {
    static DateTime fromFileTime(FILETIME fileTime);
    static DateTime fromCurrentTime();

    FILETIME fileTime;
    uint64_t packedSystemTime;
};
static_assert(sizeof(DateTime) == 0x10);
