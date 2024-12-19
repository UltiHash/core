#pragma once

#include <random>
#include <string>

class random_char_generator {
public:
    struct promise_type {
        char current_char;
        std::suspend_always yield_value(char c) {
            current_char = c;
            return {};
        }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        random_char_generator get_return_object() {
            return random_char_generator(
                std::coroutine_handle<promise_type>::from_promise(*this));
        }
        void unhandled_exception() {}
        void return_void() {}
    };

    random_char_generator(std::coroutine_handle<promise_type> h)
        : handle(h) {}
    random_char_generator(const random_char_generator&) = delete;
    random_char_generator(random_char_generator&& other)
        : handle(other.handle) {
        other.handle = nullptr;
    }
    ~random_char_generator() {
        if (handle)
            handle.destroy();
    }

    random_char_generator& operator++() {
        if (!handle.done()) {
            handle.resume();
        }
        return *this;
    }

    char operator*() const { return handle.promise().current_char; }

    explicit operator bool() const { return !handle.done(); }

private:
    std::coroutine_handle<promise_type> handle;
};

random_char_generator get_random_char_generator() {
    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 255);
    while (true) {
        co_yield static_cast<char>(dist(gen));
    }
}

std::string generate_random_string(size_t length) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(0, 255);

    auto g = get_random_char_generator();
    std::string random_string(length, '\0');
    for (size_t i = 0; i < length; ++i, ++g) {
        random_string[i] = *g;
    }
    return random_string;
}
