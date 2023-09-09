#pragma once
#include "Define.h"
#include <cassert>
#include <functional>
#include <future>

template <typename>
struct function_traits;

template <typename R, typename C, typename... Args>
struct function_traits<R (C::*)(Args...) const> {
    using type         = R(Args...);
    using return_trype = R;
};

template <typename R, typename C, typename... Args>
struct function_traits<R (C::*)(Args...)> { // for mutable lambdas
    using type         = R(Args...);
    using return_trype = R;
};

template <typename Fn>
class AsyncTask {
    MAKE_NONCOPYABLE(AsyncTask);

public:
    using result_type = std::invoke_result<std::function<Fn>>::type;

    AsyncTask() = default;
    AsyncTask(std::function<Fn>&& fn) : fn(std::move(fn)) {
    }

    AsyncTask(AsyncTask&& other) noexcept            = default;
    AsyncTask& operator=(AsyncTask&& other) noexcept = default;

    ~AsyncTask() {
        assert(!future.valid()); // Destroying while future is still valid would block here and is almost certainly a bug.
    }

    void run() {
        future = std::async(std::launch::async, fn);
    }

    bool valid() {
        return future.valid();
    }

    bool ready() {
        return future._Is_ready();
    }

    result_type get() {
        return future.get();
    }

private:
    std::future<result_type> future{};
    std::function<Fn> fn;
};

template <class Fn>
AsyncTask(Fn) -> AsyncTask<typename function_traits<decltype(&Fn::operator())>::type>;