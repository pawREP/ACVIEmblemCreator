#pragma once
#include "Define.h"
#include <functional>

namespace libEmblem {

#define SCOPE_EXIT const auto ANONYMOUS = ScopeExitHelper{} += [&]()

    template <typename Fn>
    class ScopeExit {
        MAKE_NONCOPYABLE(ScopeExit);
        MAKE_NONMOVABLE(ScopeExit);

    public:
        explicit ScopeExit(Fn&& callable) : fn(callable){};

        ~ScopeExit() {
            fn();
        }

    private:
        Fn fn;
    };

    struct ScopeExitHelper {
        template <typename F>
        ScopeExit<F> operator+=(F&& fn) {
            return ScopeExit<F>(std::move(fn));
        }
    };

} // namespace libEmblem