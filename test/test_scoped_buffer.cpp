#define BOOST_TEST_MODULE "scoped buffer tests"

#include <boost/test/unit_test.hpp>
#include <common/types/scoped_buffer.h>

using namespace uh::cluster;

namespace {

template <typename T> bool check_simple() {
    unique_buffer<T> buf(10);
    const auto b1 = buf.size() == 10;
    const auto b2 = buf.capacity() == 10;
    const auto b3 = buf.span().size() == 10;
    const auto b4 = buf.string_view().size() == 10 * sizeof(T);
    return b1 and b2 and b3 and b4;
}

template <typename T> bool check_resize() {
    unique_buffer<T> buf(10);
    buf.resize(20);
    const auto b1 = buf.size() == 20;
    const auto b2 = buf.capacity() == 20;
    const auto b3 = buf.span().size() == 20;
    const auto b4 = buf.string_view().size() == 20 * sizeof(T);
    return b1 and b2 and b3 and b4;
}

template <typename T> bool check_reserve() {
    unique_buffer<T> buf(10);
    buf.reserve(20);
    const auto b1 = buf.size() == 10;
    const auto b2 = buf.capacity() == 20;
    const auto b3 = buf.span().size() == 10;
    const auto b4 = buf.string_view().size() == 10 * sizeof(T);
    return b1 and b2 and b3 and b4;
}

template <typename T> bool check_reserve_resize() {
    unique_buffer<T> buf(10);
    buf.reserve(20);
    buf.resize(30);
    const auto b1 = buf.size() == 30;
    const auto b2 = buf.capacity() == 30;
    const auto b3 = buf.span().size() == 30;
    const auto b4 = buf.string_view().size() == 30 * sizeof(T);
    return b1 and b2 and b3 and b4;
}

template <typename T> bool check_reserve_zero() {
    unique_buffer<T> buf(10);
    buf.reserve(0);
    const auto b1 = buf.size() == 10;
    const auto b2 = buf.capacity() == 10;
    const auto b3 = buf.span().size() == 10;
    const auto b4 = buf.string_view().size() == 10 * sizeof(T);
    return b1 and b2 and b3 and b4;
}

template <typename T> bool check_resize_zero() {
    unique_buffer<T> buf(10);
    buf.resize(0);
    const auto b1 = buf.size() == 0;
    const auto b2 = buf.capacity() == 10;
    const auto b3 = buf.span().size() == 0;
    const auto b4 = buf.string_view().size() == 0;
    return b1 and b2 and b3 and b4;
}

template <typename T> bool check_empty() {
    unique_buffer<T> buf;
    const auto b1 = buf.size() == 0;
    const auto b2 = buf.capacity() == 0;
    const auto b3 = buf.span().size() == 0;
    const auto b4 = buf.string_view().size() == 0;
    return b1 and b2 and b3 and b4;
}

template <typename T> bool check_empty_resize() {
    unique_buffer<T> buf;
    buf.resize(10);
    const auto b1 = buf.size() == 10;
    const auto b2 = buf.capacity() == 10;
    const auto b3 = buf.span().size() == 10;
    const auto b4 = buf.string_view().size() == 10 * sizeof(T);
    return b1 and b2 and b3 and b4;
}

template <typename T> bool check_empty_reserve() {
    unique_buffer<T> buf;
    buf.reserve(10);
    const auto b1 = buf.size() == 0;
    const auto b2 = buf.capacity() == 10;
    const auto b3 = buf.span().size() == 0;
    const auto b4 = buf.string_view().size() == 0;
    return b1 and b2 and b3 and b4;
}

template <typename T> bool check_empty_reserve_resize() {
    unique_buffer<T> buf;
    buf.reserve(10);
    buf.resize(20);
    const auto b1 = buf.size() == 20;
    const auto b2 = buf.capacity() == 20;
    const auto b3 = buf.span().size() == 20;
    const auto b4 = buf.string_view().size() == 20 * sizeof(T);
    return b1 and b2 and b3 and b4;
}

template <typename T> void do_checks() {
    BOOST_CHECK(check_simple<T>());
    BOOST_CHECK(check_resize<T>());
    BOOST_CHECK(check_reserve<T>());
    BOOST_CHECK(check_reserve_resize<T>());
    BOOST_CHECK(check_reserve_zero<T>());
    BOOST_CHECK(check_resize_zero<T>());
    BOOST_CHECK(check_empty<T>());
    BOOST_CHECK(check_empty_resize<T>());
    BOOST_CHECK(check_empty_reserve<T>());
    BOOST_CHECK(check_empty_reserve_resize<T>());
}

BOOST_AUTO_TEST_CASE(buffer_checks) {
    do_checks<char>();
    do_checks<unsigned char>();
    do_checks<int>();
    do_checks<unsigned int>();
    do_checks<long>();
    do_checks<unsigned long>();
    do_checks<float>();
    do_checks<double>();
    struct S {
        int v1;
        int v2;
        float v3;
        char v4;
    };
    do_checks<S>();
}

} // namespace