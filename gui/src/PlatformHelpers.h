#pragma once
#include <Windows.h>
#include <string>

class bad_hr_exception : public std::exception {
public:
    bad_hr_exception(HRESULT hr);
    ~bad_hr_exception() = default;

    [[nodiscard]] char const* what() const override;

private:
    std::string msg;
};

void ThrowIfFailed(HRESULT hr);
