#ifndef CLIENT_SHELL_COMMON_H
#define CLIENT_SHELL_COMMON_H

#include <iostream>

//container libraries general
#include <vector>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <stack>
#include <unordered_set>
#include <bitset>

//boost libraries general
#include <boost/mp11.hpp>
#include <boost/container/stable_vector.hpp>
#include <boost/algorithm/string/predicate.hpp>

//others
#include <sys/stat.h>

//static type reader, transforming a type infomatiotion into a string that can be processed
template<class T>
constexpr std::string_view
type_name() {
    using namespace std;
#ifdef __clang__
    string_view p = __PRETTY_FUNCTION__;
    return {p.data() + 34, p.size() - 34 - 1};
#elif defined(__GNUC__)
    string_view p = __PRETTY_FUNCTION__;
#if __cplusplus < 201402
    return string_view(p.data() + 36, p.size() - 36 - 1);
#else
    return {p.data() + 49, p.find(';', 49) - 49};
#endif
#elif defined(_MSC_VER)
    string_view p = __FUNCSIG__;
    return string_view(p.data() + 84, p.size() - 84 - 7);
#endif
}

//concept detecting deques
template<typename T1>
struct is_deque {
    using decayed = typename std::decay<T1>::type;
    [[maybe_unused]] static const bool value = boost::mp11::mp_similar<std::deque<decayed>, decayed>::value or
                                               boost::mp11::mp_similar<boost::container::deque<decayed>, decayed>::value or
                                               boost::mp11::mp_similar<boost::container::devector<decayed>, decayed>::value;
};


template<class...Args>
concept String = std::conjunction<std::is_convertible<Args, std::string_view>...>::value;

//summary types
//concept detecting any fundamental type
template<typename... Args>
concept ArithmeticFundamental = std::conjunction<std::conjunction<std::is_arithmetic<Args>>...>::value;

//concept detecting any arithmetic fundamental type that is signed/unsigned integer base types or floating point base types
template<typename... Args>
concept Arithmetic = std::conjunction<std::disjunction<std::is_arithmetic<Args>, std::conjunction<std::is_arithmetic<typename std::decay<Args>::type>, std::is_lvalue_reference<Args>>>...>::value;

//concept detecting forward_lists
template<typename T1>
struct is_forward_list {
    using decayed = typename std::decay<T1>::type;
    [[maybe_unused]] static const bool value = boost::mp11::mp_similar<std::forward_list<decayed>, decayed>::value or
                                               boost::mp11::mp_similar<boost::container::slist<decayed>, decayed>::value;
};

//concept detecting lists
template<typename T1>
struct is_list {
    using decayed = typename std::decay<T1>::type;
    [[maybe_unused]] static const bool value = boost::mp11::mp_similar<std::list<decayed>, decayed>::value or
                                               boost::mp11::mp_similar<boost::container::list<decayed>, decayed>::value;
};

//concept detecting the special boost stable vector that has tree structure inside with stable flag
template<typename T1>
struct is_vector_stable {
    using decayed = typename std::decay<T1>::type;
    [[maybe_unused]] static const bool value = boost::mp11::mp_similar<boost::container::stable_vector<decayed>, decayed>::value;
};

//concept detecting standard vectors with no special properties
template<typename T1>
struct is_vector_standard {
    using decayed = typename std::decay<T1>::type;
    [[maybe_unused]] static const bool value = boost::mp11::mp_similar<std::vector<decayed>, decayed>::value or
                                               boost::mp11::mp_similar<boost::container::vector<decayed>, decayed>::value;
};


template<typename... T1>
concept VectorStandard = std::conjunction<is_vector_standard<T1>...>::value;

//concept detecting if the type is any sequential container type
template<typename... Args>
struct is_sequential_container {
    static const bool value = std::conjunction<std::disjunction<is_vector_standard<Args>, is_vector_stable<Args>, is_deque<Args>, is_forward_list<Args>, is_list<Args>>...>::value;
};

template<typename... Args>
concept SequentialContainer = is_sequential_container<Args...>::value;

#endif
