#pragma once

#define CONCAT_(x, y) x##y
#define CONCAT(x, y) CONCAT_(x, y)

#define ANONYMOUS CONCAT(_anonymous, __COUNTER__)

#define MAKE_NONCOPYABLE(cls)            \
private:                                 \
    cls(const cls&)            = delete; \
    cls& operator=(const cls&) = delete

#define MAKE_DEFAULTCOPYABLE(cls)         \
public:                                  \
    cls(const cls&)            = default; \
    cls& operator=(const cls&) = default

#define MAKE_NONMOVABLE(cls)                 \
private:                                     \
    cls(cls&&) noexcept            = delete; \
    cls& operator=(cls&&) noexcept = delete

#define MAKE_DEFAULTMOVABLE(cls)              \
public:                                      \
    cls(cls&&) noexcept            = default; \
    cls& operator=(cls&&) noexcept = default
