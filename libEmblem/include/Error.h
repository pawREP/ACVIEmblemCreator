#pragma once
#include "Define.h"
#include <expected>
#include <format>
#include <string>
#include <string_view>

namespace libEmblem {

#define UNWRAP_OR_PROPAGATE_(expectedValue, expected, varName) \
    auto varName = expected;                                   \
    if(!varName)                                               \
        return std::unexpected{ varName.error() };             \
    decltype(varName)::value_type expectedValue = std::move(varName).value();

#define UNWRAP_OR_PROPAGATE(expectedValue, expected) UNWRAP_OR_PROPAGATE_(expectedValue, expected, ANONYMOUS)

#define PROPAGATE_IF_ERROR_(expected_, varName)                                       \
    {                                                                                 \
        if constexpr(std::_Is_specialization_v<decltype(expected_), std::expected>) { \
            if(!expected_)                                                            \
                return std::unexpected{ expected_.error() };                          \
        } else {                                                                      \
            auto varName = expected_;                                                 \
            if(!varName)                                                              \
                return std::unexpected{ varName.error() };                            \
        }                                                                             \
    }

#define PROPAGATE_IF_ERROR(expected) PROPAGATE_IF_ERROR_(expected, ANONYMOUS)

    class Error {
    public:
        template <int N>
        Error(const char (&literal)[N]) : msg(literal){};

        Error(std::string_view stringView);

        Error(const std::string& str);

        template <typename... Args>
        Error(const std::format_string<Args...> formatString, Args... args) {
            msg = std::format(formatString, std::forward<Args>(args)...);
        }

        auto operator<=>(const Error&) const noexcept = default;

        const std::string& string();

        template <typename T>
        operator std::expected<T, Error>() const noexcept {
            return std::unexpected{ *this };
        }

    private:
        std::string msg;
    };

    template <typename T = void, typename E = Error>
    using ErrorOr = std::expected<T, E>;

} // namespace libEmblem
