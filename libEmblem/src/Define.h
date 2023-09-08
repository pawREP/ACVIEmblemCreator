#pragma once

#define CONCAT_(x, y) x##y
#define CONCAT(x, y) CONCAT_(x, y)

#define ANONYMOUS CONCAT(_anonymous, __COUNTER__)

#define MAKE_NONCOPYABLE(cls)            \
private:                                 \
    cls(const cls&)            = delete; \
    cls& operator=(const cls&) = delete

#define MAKE_NONMOVABLE(cls)        \
private:                            \
    cls(cls&&)            = delete; \
    cls& operator=(cls&&) = delete