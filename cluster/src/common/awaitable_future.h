//
// Created by masi on 10/10/23.
//

#ifndef CORE_AWAITABLE_FUTURE_H
#define CORE_AWAITABLE_FUTURE_H

#include <chrono>
#include <coroutine>
#include <exception>
#include <future>
#include <iostream>
#include <thread>
#include <type_traits>

// Source of code: https://en.cppreference.com/w/cpp/coroutine/coroutine_traits


// A program-defined type on which the coroutine_traits specializations below depend
struct as_coroutine { };
/*
// Enable the use of std::future<T> as a coroutine type
// by using a std::promise<T> as the promise type.
template <typename T, typename... Args>
requires(!std::is_void_v<T> && !std::is_reference_v<T>)
struct std::coroutine_traits<std::future<T>, as_coroutine, Args...>
{
    struct promise_type : std::promise<T>
    {
        std::future<T> get_return_object() noexcept
        {
            return this->get_future();
        }

        [[nodiscard]] std::suspend_never initial_suspend() const noexcept { return {}; }
        [[nodiscard]] std::suspend_never final_suspend() const noexcept { return {}; }

        void return_value(const T& value)
        noexcept(std::is_nothrow_copy_constructible_v<T>)
        {
            this->set_value(value);
        }

        void return_value(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
        {
            this->set_value(std::move(value));
        }

        void unhandled_exception() noexcept
        {
            this->set_exception(std::current_exception());
        }
    };
};

// Same for std::future<void>.
template <typename... Args>
struct std::coroutine_traits<std::future<void>, as_coroutine, Args...>
{
    struct promise_type : std::promise<void>
    {
        std::future<void> get_return_object() noexcept
        {
            return this->get_future();
        }

        [[nodiscard]] std::suspend_never initial_suspend() const noexcept { return {}; }
        [[nodiscard]] std::suspend_never final_suspend() const noexcept { return {}; }

        void return_void() noexcept
        {
            this->set_value();
        }

        void unhandled_exception() noexcept
        {
            this->set_exception(std::current_exception());
        }
    };
};

// Allow co_await'ing std::future<T> and std::future<void>
// by naively spawning a new thread for each co_await.
template <typename T>
auto operator co_await(std::future<T> future) noexcept
requires(!std::is_reference_v<T>)
{
    struct awaiter : std::future<T>
    {
        bool await_ready() const noexcept
        {
            using namespace std::chrono_literals;
            return this->wait_for(0s) != std::future_status::timeout;
        }

        void await_suspend(std::coroutine_handle<> cont) const
        {
            std::thread([this, cont]
                        {
                            this->wait();
                            cont();
                        }).detach();
        }

        T await_resume() { return this->get(); }
    };

    return awaiter { std::move(future) };
}
*/
#endif //CORE_AWAITABLE_FUTURE_H
