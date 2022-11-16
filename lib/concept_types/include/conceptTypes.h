//
// Created by benjamin-elias on 09.05.22.
//
#pragma once

//std libraries general
#include <fstream>
#include <thread>
#include <stack>
#include <functional>
#include <utility>
#include <tr2/type_traits>
#include <concepts>
#include <iostream>
#include <mutex>
#include <random>
#include <bits/stdc++.h>
#include <sstream>
#include <cmath>
#include <filesystem>
#include <bitset>
#include <iterator>
#include <algorithm>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>

//low level filesystem, sha
#include <sys/stat.h>
#include "openssl/sha.h"

//boost libraries general
//#include "boost/filesystem/operations.hpp" // includes boost/filesystem/path.hpp
#include <boost/property_tree/json_parser.hpp>
#include <boost/chrono.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <boost/mp11.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/tti/tti.hpp>
#include <boost/phoenix.hpp>
#include <boost/function.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/cxx14/equal.hpp>
#include <boost/algorithm/clamp.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/range/algorithm/reverse.hpp>
#include <boost/sort/sort.hpp>
#include <boost/heap/heap_concepts.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/container_hash/hash.hpp>
#include <boost/assert/source_location.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/version.hpp>

//container libraries general
#include <vector>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <boost/unordered_map.hpp>
#include <boost/fusion/container/map.hpp>
#include <boost/container/map.hpp>
#include <boost/container/vector.hpp>
#include <boost/container/stable_vector.hpp>
#include <boost/container/deque.hpp>
#include <boost/container/devector.hpp>
#include <boost/container/slist.hpp>
#include <boost/container/list.hpp>

//module concepttypes start
#ifndef INTEGRATION_PROJECT_CONCEPTTYPES_H
#define INTEGRATION_PROJECT_CONCEPTTYPES_H

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
    return string_view(p.data() + 49, p.find(';', 49) - 49);
#endif
#elif defined(_MSC_VER)
    string_view p = __FUNCSIG__;
    return string_view(p.data() + 84, p.size() - 84 - 7);
#endif
}

//concepts used for LEVEL1

//static false typename flag
template<class>
[[maybe_unused]] inline constexpr bool always_false_v = false;

//the indexer can find i-th element type within a tuple
template<typename T1, typename Tuple>
struct indexer;

template<typename T1, typename... Types>
struct indexer<T1, boost::tuple<T1, Types...>> {
    static const std::size_t value = 0;
};

template<typename T1, typename U, typename... Types>
struct indexer<T1, boost::tuple<U, Types...>> {
    static const std::size_t value = 1 + indexer<T1, boost::tuple<Types...>>::value;
};

template<typename T1, typename... Types>
struct indexer<T1, std::tuple<T1, Types...>> {
    static const std::size_t value = 0;
};

template<typename T1, typename U, typename... Types>
struct indexer<T1, std::tuple<U, Types...>> {
    static const std::size_t value = 1 + indexer<T1, std::tuple<Types...>>::value;
};

//concept detecting unreferenced and not pointed to types
template<typename... T1>
concept ClearValue = std::conjunction<std::negation<std::is_reference<T1>>..., std::negation<std::is_pointer<T1>>...>::value;

//concept detecting lvalues
template<typename... T1>
concept LValue = std::conjunction<std::is_lvalue_reference<T1>...>::value;

//concept detecting rvalues
template<typename... T1>
concept RValue = std::conjunction<std::is_rvalue_reference<T1>...>::value;

//concept detecting standard vectors with no special properties
template<typename T1>
struct is_vector_standard {
    using decayed = typename std::decay<T1>::type;
    [[maybe_unused]] static const bool value = boost::mp11::mp_similar<std::vector<decayed>, decayed>::value or
                                               boost::mp11::mp_similar<boost::container::vector<decayed>, decayed>::value;
};

template<typename... T1>
concept VectorStandard = std::conjunction<is_vector_standard<T1>...>::value;

//concept detecting the special boost stable vector that has tree structure inside with stable flag
template<typename T1>
struct is_vector_stable {
    using decayed = typename std::decay<T1>::type;
    [[maybe_unused]] static const bool value = boost::mp11::mp_similar<boost::container::stable_vector<decayed>, decayed>::value;
};

template<typename... T1>
concept Vector = std::conjunction<std::disjunction<is_vector_standard<T1>, is_vector_stable<T1>>...>::value;

//concept detecting deques
template<typename T1>
struct is_deque {
    using decayed = typename std::decay<T1>::type;
    [[maybe_unused]] static const bool value = boost::mp11::mp_similar<std::deque<decayed>, decayed>::value or
                                               boost::mp11::mp_similar<boost::container::deque<decayed>, decayed>::value or
                                               boost::mp11::mp_similar<boost::container::devector<decayed>, decayed>::value;
};

template<typename... T1>
concept Deque = std::conjunction<is_deque<T1>...>::value;

//concept detecting forward_lists
template<typename T1>
struct is_forward_list {
    using decayed = typename std::decay<T1>::type;
    [[maybe_unused]] static const bool value = boost::mp11::mp_similar<std::forward_list<decayed>, decayed>::value or
                                               boost::mp11::mp_similar<boost::container::slist<decayed>, decayed>::value;
};

template<typename... T1>
concept ForwardList = std::conjunction<is_forward_list<T1>...>::value;

//concept detecting lists
template<typename T1>
struct is_list {
    using decayed = typename std::decay<T1>::type;
    [[maybe_unused]] static const bool value = boost::mp11::mp_similar<std::list<decayed>, decayed>::value or
                                               boost::mp11::mp_similar<boost::container::list<decayed>, decayed>::value;
};

template<typename... T1>
concept List = std::conjunction<is_list<T1>...>::value;

//concept detecting if the type is any sequential container type
template<typename... Args>
struct is_sequential_container {
    static const bool value = std::conjunction<std::disjunction<is_vector_standard<Args>, is_vector_stable<Args>, is_deque<Args>, is_forward_list<Args>, is_list<Args>>...>::value;
};

template<typename... Args>
concept SequentialContainer = is_sequential_container<Args...>::value;

template<class...Args>
concept String = std::conjunction<std::is_convertible<Args, std::string_view>...>::value;

//concept detecting if the type is a string iterator
template<typename T1>
struct is_string_iterator{
    [[maybe_unused]] static const bool value = boost::mp11::mp_same<typename std::string::iterator,T1>::value;
};

template<typename... Args>
concept StringIterator = std::conjunction<is_string_iterator<Args>...>::value;

//concept detecting if the type is any iterator at all
template<typename T, typename = void>
struct is_iterator {
    [[maybe_unused]] static const bool value = false;
};

template<typename T>
struct is_iterator<T, typename std::enable_if<!std::is_same<typename std::iterator_traits<T>::value_type, void>::value>::type> {
    [[maybe_unused]] static const bool value = true;
};

template<typename... T1>
concept IteratorType = std::conjunction<std::conjunction<std::negation<std::is_convertible<T1, std::string_view>>, is_iterator<typename std::decay<T1>::type>>...>::value;

//concept detecting if the type is any pointer type
template<typename...T1>
concept Pointer = std::conjunction<std::is_pointer<T1>...>::value;

//concept detecting if the type has a reference
template<typename...Args>
concept Reference = std::conjunction<std::is_reference<Args>...>::value;

//integral concepts
//concept detecting fundamental integral types like signed/unsigned int,char, short, long or long versions of them
template<typename... Args>
concept IntegralFundamental = std::conjunction<std::conjunction<std::is_integral<Args>>...>::value;

//concept detecting fundamental integral types that are referenced to
template<typename... Args>
concept IntegralLValue = std::conjunction<std::conjunction<std::is_integral<typename std::decay<Args>::type>, std::is_lvalue_reference<Args>>...>::value;

//concept detecting fundamental integral types that are pointed to
template<typename T1>
struct is_integral_pointer {
    [[maybe_unused]] static const bool value = false;
};

template<typename T1>
struct is_integral_pointer<T1 *> {
    [[maybe_unused]] static const bool value = std::is_integral<T1>::value;
};

template<typename... Args>
concept IntegralPointer = std::conjunction<is_integral_pointer<Args>...>::value;

//concept detecting any referenced or pointed to integral type
template<typename... Args>
concept Integral = std::conjunction<std::disjunction<std::is_integral<Args>, std::conjunction<std::is_integral<typename std::decay<Args>::type>, std::is_lvalue_reference<Args>>>...>::value;

//unsigned integral concepts
//concept detecting fundamental integral types like unsigned int,char, short, long or long versions of them
template<typename... Args>
concept UnsignedIntegralFundamental = std::conjunction<std::is_unsigned<Args>...>::value;

//concept detecting unsigned integral lvalue
template<typename... Args>
concept UnsignedIntegralLValue = std::conjunction<std::conjunction<std::is_unsigned<typename std::decay<Args>::type>, std::is_lvalue_reference<Args>>...>::value;

//concept detecting unsigned integral pointer
template<typename T1>
struct is_unsigned_integral_pointer {
    [[maybe_unused]] static const bool value = false;
};

template<typename T1>
struct is_unsigned_integral_pointer<T1 *> {
    [[maybe_unused]] static const bool value = std::is_unsigned<T1>::value;
};

template<typename... Args>
concept UnsignedIntegralPointer = std::conjunction<is_unsigned_integral_pointer<Args>...>::value;

//concept detecting any unsigned integral that is referenced or pointed to
template<typename... Args>
concept UnsignedIntegral = std::conjunction<std::disjunction<std::is_unsigned<Args>, std::conjunction<std::is_unsigned<typename std::decay<Args>::type>, std::is_lvalue_reference<Args>>>...>::value;

//signed integral concepts
//concept detecting fundamental integral types like signed int,char, short, long or long versions of them
template<typename... Args>
concept SignedIntegralFundamental = std::conjunction<std::conjunction<std::is_integral<Args>, std::is_signed<Args>>...>::value;

//concept detecting signed integral lvalues
template<typename... Args>
concept SignedIntegralLValue = std::conjunction<std::conjunction<std::is_integral<typename std::decay<Args>::type>, std::is_signed<typename std::decay<Args>::type>, std::is_lvalue_reference<Args>>...>::value;

//concept detecting signed integral pointers
template<typename T1>
struct is_signed_integral_pointer {
    [[maybe_unused]] static const bool value = false;
};

template<typename T1>
struct is_signed_integral_pointer<T1 *> {
    [[maybe_unused]] static const bool value = std::is_integral<T1>::value and std::is_signed<T1>::value;
};

template<typename... Args>
concept SignedIntegralPointer = std::conjunction<is_signed_integral_pointer<Args>...>::value;

//concept detecting any signed integral that is referenced or pointed to
template<typename... Args>
concept SignedIntegral = std::conjunction<std::disjunction<std::conjunction<std::conjunction<std::is_integral<Args>, std::is_signed<Args>>...>, std::conjunction<std::conjunction<std::is_integral<typename std::decay<Args>::type>, std::is_signed<typename std::decay<Args>::type>, std::is_lvalue_reference<Args>>>>...>::value;

//floating point concepts
//concept detecting floating point types
template<typename... Args>
concept FloatingPointFundamental = std::conjunction<std::conjunction<std::is_floating_point<Args>>...>::value;

//concept detecting lvalue floating point types
template<typename... Args>
concept FloatingPointLValue = std::conjunction<std::conjunction<std::is_floating_point<typename std::decay<Args>::type>, std::is_lvalue_reference<Args>>...>::value;

//concept detecting pointers to floating point types
template<typename T1>
struct is_floating_point_pointer {
    [[maybe_unused]] static const bool value = false;
};

template<typename T1>
struct is_floating_point_pointer<T1 *> {
    [[maybe_unused]] static const bool value = std::is_floating_point<T1>::value;
};

template<typename... Args>
concept FloatingPointPointer = std::conjunction<is_floating_point_pointer<Args>...>::value;

//concept detecting any type of floating point number
template<typename... Args>
concept FloatingPoint = std::conjunction<std::disjunction<std::is_floating_point<Args>, std::conjunction<std::is_floating_point<typename std::decay<Args>::type>, std::is_lvalue_reference<Args>>>...>::value;

//summary types
//concept detecting any fundamental type
template<typename... Args>
concept ArithmeticFundamental = std::conjunction<std::conjunction<std::is_arithmetic<Args>>...>::value;

//concept detecting any lvalue fundamental type
template<typename... Args>
concept ArithmeticLValue = std::conjunction<std::conjunction<std::is_arithmetic<typename std::decay<Args>::type>, std::is_lvalue_reference<Args>>...>::value;

//concept detecting any pointer to a fundamental type
template<typename T1>
struct is_arithmetic_pointer {
    [[maybe_unused]] static const bool value = false;
};

template<typename T1>
struct is_arithmetic_pointer<T1 *> {
    [[maybe_unused]] static const bool value = std::is_arithmetic<T1>::value;
};

template<typename... Args>
concept ArithmeticPointer = std::conjunction<is_arithmetic_pointer<Args>...>::value;

//concept detecting any arithmetic fundamental type that is signed/unsigned integer base types or floating point base types
template<typename... Args>
concept Arithmetic = std::conjunction<std::disjunction<std::is_arithmetic<Args>, std::conjunction<std::is_arithmetic<typename std::decay<Args>::type>, std::is_lvalue_reference<Args>>>...>::value;

//negation types
//concept making sure the type is not a sequential container or any arithmetic, so it may be for example a std::variant type
template<typename... Args>
concept NoSequentialContainerNoArithmetic = std::conjunction<std::conjunction<std::negation<is_sequential_container<Args>>, std::negation<std::disjunction<std::is_arithmetic<Args>, std::conjunction<std::is_arithmetic<typename std::decay<Args>::type>, std::is_lvalue_reference<Args>>>>>...>::value;

//concept making sure the type is not Arithmetic, so it may be a container or std::variant type
template<typename... Args>
concept NoArithmetic = !Arithmetic<Args...>;

//concept making sure the type is not Arithmetic or an iterator
template<typename...Args>
concept NoArithmeticIterator = !Arithmetic<Args...> and !IteratorType<Args...>;

//concept using the standard implementation to recursively check if T was derived from U
template<class T, class U>
concept Derived = std::is_base_of<U, T>::value;

//map concepts
//concept checking if the type is an ordered map
template<typename T1>
struct is_ordered_map {
    using decayed = typename std::decay<T1>::type;
    [[maybe_unused]] static const bool value = boost::mp11::mp_similar<std::map<decayed, decayed>, decayed>::value or
                                               boost::mp11::mp_similar<boost::container::map<decayed, decayed>, decayed>::value or
                                               boost::mp11::mp_similar<boost::fusion::map<decayed, decayed>, decayed>::value;
};

template<typename... Args>
concept OrderedMap = std::conjunction<is_ordered_map<Args>...>::value;

//concept checking if the type is an unordered map
template<typename T1>
struct is_unordered_map {
    using decayed = typename std::decay<T1>::type;
    [[maybe_unused]] static const bool value = boost::mp11::mp_similar<std::unordered_map<decayed, decayed>, decayed>::value or
                                               boost::mp11::mp_similar<boost::unordered_map<decayed, decayed>, decayed>::value;
};

template<typename... Args>
concept UnorderedMap = std::conjunction<is_unordered_map<Args>...>::value;

//concept checking if the type is a map at all
template<typename... Args>
concept Map = std::conjunction<std::disjunction<is_unordered_map<Args>,is_ordered_map<Args>>...>::value;

//set concepts
//concept checking if the type is an ordered set
template<typename T1>
struct is_ordered_set {
    using decayed = typename std::decay<T1>::type;
    [[maybe_unused]] static const bool value = boost::mp11::mp_similar<std::set<decayed, decayed>, decayed>::value or
                                               boost::mp11::mp_similar<boost::container::set<decayed, decayed>, decayed>::value or
                                               boost::mp11::mp_similar<boost::fusion::set<decayed, decayed>, decayed>::value;
};

template<typename... Args>
concept OrderedSet = std::conjunction<is_ordered_set<Args>...>::value;

//concept checking if the type is an unordered set
template<typename T1>
struct is_unordered_set {
    using decayed = typename std::decay<T1>::type;
    [[maybe_unused]] static const bool value = boost::mp11::mp_similar<std::unordered_set<decayed, decayed>, decayed>::value;
};

template<typename... Args>
concept UnorderedSet = std::conjunction<is_unordered_set<Args>...>::value;

//concept type checking if the type is a set at all
template<typename... Args>
concept Set = std::conjunction<std::disjunction<is_unordered_set<Args>,is_ordered_set<Args>>...>::value;

//concept type checking if the type is a set or a map
template<typename... Args>
concept SetContainer = Set<Args...> or Map<Args...>;

//variant concepts
//concept checking if the type is a std::variant with one recursion and no more std::variants within it
template<typename T1>
struct is_variant_single {
    [[maybe_unused]] static const bool value = false;
};

template<template<typename> typename T1, typename... Args>
struct is_variant_single<T1<Args...>> {
    [[maybe_unused]] static const bool value = sizeof...(Args) > 0 and
                                               std::conjunction<std::is_same<std::variant<Args...>, typename std::decay<T1<Args...>>::type>, std::negation<is_variant_single<Args>>...>::value;
};

template<typename... T1>
concept VariantSingleRecursion = std::conjunction<is_variant_single<T1>...>::value;

//concept checking if the type is a std::variant with possibly a recursion of std::variants or sequential containers within it
template<typename T1>
struct is_deep_variant_container {
    static const bool value = false;
};

template<template<typename> typename T1, typename... Args>
struct is_deep_variant_container<T1<Args...>> {
    static const bool value = VariantSingleRecursion<T1<Args...>> or (sizeof...(Args) > 0 and
                                                                      std::conjunction<std::disjunction<is_sequential_container<Args>, is_deep_variant_container<Args>>...>::value);
};

template<typename... Args>
concept DeepVariantContainer = std::conjunction<is_deep_variant_container<Args>...>::value;//or is_deep_variant_container<>

//concept of DeepVariantContainer that can be referenced or pointed to, but will still be detected
template<typename... Args>
concept DeepVariantContainerDecay = std::conjunction<is_deep_variant_container<typename std::decay<Args>::type>...>::value;//or is_deep_variant_container<>

//concept detecting a DeepVariantContainer or sequential container type
template<class... Args>
concept OptionalRecursiveContainer = std::conjunction<std::disjunction<is_sequential_container<Args>, is_deep_variant_container<Args>>...>::value;

//concept of OptionalRecursiveContainer that can be referenced or pointed to, but will still be detected
template<class... Args>
concept OptionalRecursiveContainerDecay = std::conjunction<std::disjunction<is_sequential_container<typename std::decay<Args>::type>, is_deep_variant_container<typename std::decay<Args>::type>>...>::value;

//concept detecting a DeepVariantContainer or sequential container type or an Arithmetic type
template<class... Args>
concept OptionalRecursiveArithmeticContainer = std::conjunction<std::disjunction<std::conjunction<std::disjunction<std::is_arithmetic<Args>, std::conjunction<std::is_arithmetic<typename std::decay<Args>::type>, std::is_lvalue_reference<Args>>>...>, is_sequential_container<Args>, is_deep_variant_container<Args>>...>::value;

//concept of OptionalRecursiveArithmeticContainer that can be referenced or pointed to, but will still be detected
template<class... Args>
concept OptionalRecursiveArithmeticContainerDecay = std::conjunction<std::disjunction<std::conjunction<std::disjunction<std::is_arithmetic<typename std::decay<Args>::type>, std::conjunction<std::is_arithmetic<typename std::decay<Args>::type>, std::is_lvalue_reference<typename std::decay<Args>::type>>>...>, is_sequential_container<typename std::decay<Args>::type>, is_deep_variant_container<typename std::decay<Args>::type>>...>::value;

//higher concept types of LEVEL1

//concept detecting the reference operator within a module
template<typename T1>
using innerVariant = decltype(&T1::operator*);

//concept checking if a container holds a certain type as first type
template<typename T0, typename T1>
struct is_container_type {
    [[maybe_unused]] static const bool value = false;
};

template<typename T0, template<typename> class T1, typename...Args>
struct is_container_type<T0, T1<Args...>> {
    [[maybe_unused]] static const bool value =
            SequentialContainer<T1<Args...>> and std::is_same<boost::mp11::mp_first<T1<Args...>>, T0>::value;
};
template<class T0, class... T1>
concept InnerContainerTypeOf = std::conjunction<is_container_type<T0, T1>...>::value;

//concept checking if a container holds a certain type as first type, removing references and pointers
template<typename T0, typename T1>
struct is_container_type_decay {
    [[maybe_unused]] static const bool value = false;
};

template<typename T0, template<typename> class T1, typename...Args>
struct is_container_type_decay<T0, T1<Args...>> {
    [[maybe_unused]] static const bool value =
            SequentialContainer<typename std::decay<T1<typename std::decay<Args>::type...>>::type> and
            (std::is_same<boost::mp11::mp_first<typename std::decay<T1<typename std::decay<Args>::type...>>::type>, typename std::decay<T0>::type>::value);
};

template<class T0, class... T1>
concept InnerContainerTypeOfDecay = std::conjunction<is_container_type_decay<T0, T1>...>::value;

//concept checking if two container types are equal
template<typename T0, typename T1>
struct is_similar_container {
    [[maybe_unused]] static const bool value = false;
};

template<template<typename> typename T0, typename...Args1,
        template<typename> typename T1, typename...Args2>
struct is_similar_container<T0<Args1...>, T1<Args2...>> {
    [[maybe_unused]] static const bool value =
            SequentialContainer<T0<Args1...>, T1<Args2...>> and boost::mp11::mp_similar<T0<Args1...>, T1<Args2...>>::value;
};

template<typename T1, typename... Args>
concept SimilarContainer = std::conjunction<is_similar_container<T1, Args>...>::value;

//concept like SimilarContainer removing references and pointers
template<typename T1, typename... Args>
concept SimilarContainerDecay = std::conjunction<is_similar_container<typename std::decay<T1>::type, typename std::decay<Args>::type>...>::value;

//concept to detect equality between two sequential containers or deep variants or mixed
template<typename T0, typename T1>
struct is_same_deep_container {
    [[maybe_unused]] static const bool value = false;
};

template<OptionalRecursiveContainer T0, OptionalRecursiveContainer T1>
struct is_same_deep_container<T0, T1> {
    [[maybe_unused]] static const bool value = boost::mp11::mp_similar<T0, T1>::value;
};

template<typename T1, typename... Args>
concept SimilarDeepContainer = std::conjunction<is_same_deep_container<T1, Args>...>::value;

//concept detecting SimilarDeepContainer even if it is a reference or pointer
template<typename T1, typename... Args>
concept SimilarDeepContainerDecay = std::conjunction<is_same_deep_container<typename std::decay<T1>::type, typename std::decay<Args>::type>...>::value;

//containerCreator concepts
//Hint: if its not for a ForwardList it's a filter of that type because all containers can append at the end, but ForwardList can not
//concept detecting if the container members are both vectors with the same types, enabling them to be copied in parallel via GPU or AVX
template<typename T0, typename T1>
struct is_vector_copy_arithmetic {
    [[maybe_unused]] static const bool value = false;
};

template<template<typename> typename T1, typename...Args1,
        template<typename> typename T2, typename... Args2>
struct is_vector_copy_arithmetic<T1<Args1...>, T2<Args2...>> {
    [[maybe_unused]] static const bool value = VectorStandard<T1<Args1...>, T2<Args2...>> and
                                               std::is_same<boost::mp11::mp_first<T1<Args1...>>, boost::mp11::mp_first<T2<Args2...>>>::value and
                                               Arithmetic<boost::mp11::mp_first<T1<Args1...>>>;
};

template<typename T0, typename...T1>
concept VectorCopyArithmetic = std::conjunction<is_vector_copy_arithmetic<T0, T1>...>::value;

//concept handling referenced or pointed to types of VectorCopyArithmetic
template<typename T0, typename...T1>
concept VectorCopyArithmeticDecay = std::conjunction<is_vector_copy_arithmetic<typename std::decay<T0>::type, typename std::decay<T1>::type>...>::value;

//concept detecting if the two chosen containers are capable of copying to the end of one container using a linear copy method, because the inner types are equal
template<typename T0, typename T1>
struct is_insert_copy {
    [[maybe_unused]] static const bool value = false;
};

template<template<typename> typename T1, typename...Args1,
        template<typename> typename T2, typename... Args2>
struct is_insert_copy<T1<Args1...>, T2<Args2...>> {
    [[maybe_unused]] static const bool value = SequentialContainer<T1<Args1...>, T2<Args2...>> and !ForwardList<T1<Args1...>> and
                                               std::is_same<boost::mp11::mp_first<T1<Args1...>>, boost::mp11::mp_first<T2<Args2...>>>::value;
};

template<typename T0, typename...T1>
concept InsertCopy = std::conjunction<is_insert_copy<T0, T1>...>::value;

//concept handling referenced or pointed to types of InsertCopy
template<typename T0, typename...T1>
concept InsertCopyDecay = std::conjunction<is_insert_copy<typename std::decay<T0>::type, typename std::decay<T1>::type>...>::value;

//concept detecting if the two chosen containers are capable of copying to the front of one container using a linear copy method for ForwardLists, because the inner types are equal
template<typename T0, typename T1>
struct is_insert_copy_forward {
    [[maybe_unused]] static const bool value = false;
};

template<template<typename> typename T1, typename...Args1,
        template<typename> typename T2, typename... Args2>
struct is_insert_copy_forward<T1<Args1...>, T2<Args2...>> {
    [[maybe_unused]] static const bool value = SequentialContainer<T1<Args1...>, T2<Args2...>> and ForwardList<T1<Args1...>> and
                                               std::is_same<boost::mp11::mp_first<T1<Args1...>>, boost::mp11::mp_first<T2<Args2...>>>::value;
};

template<typename T0, typename...T1>
concept InsertCopyForward = std::conjunction<is_insert_copy_forward<T0, T1>...>::value;

//concept handling referenced or pointed to types of InsertCopyForward
template<typename T0, typename...T1>
concept InsertCopyForwardDecay = std::conjunction<is_insert_copy_forward<typename std::decay<T0>::type, typename std::decay<T1>::type>...>::value;

//concept detecting if the two chosen containers are capable of copying with conversion between a multiple of 1 byte (copying unsigned char to be represented as unsigned short, unsigned int or unsigned long; and vice versa), because the inner types are unsigned
template<typename T0, typename T1>
struct is_unsigned_integral_convert_copy {
    [[maybe_unused]] static const bool value = false;
};

template<template<typename> typename T1, typename...Args1,
        template<typename> typename T2, typename... Args2>
struct is_unsigned_integral_convert_copy<T1<Args1...>, T2<Args2...>> {
    [[maybe_unused]] static const bool value = SequentialContainer<T1<Args1...>, T2<Args2...>> and !ForwardList<T1<Args1...>> and
                                               !std::is_same<boost::mp11::mp_first<T1<Args1...>>, boost::mp11::mp_first<T2<Args2...>>>::value and
                                               UnsignedIntegral<boost::mp11::mp_first<T1<Args1...>>, boost::mp11::mp_first<T2<Args2...>>>;
};

template<typename T0, typename...T1>
concept UnsignedIntegralConvertCopy = std::conjunction<is_unsigned_integral_convert_copy<T0, T1>...>::value;

//concept handling referenced or pointed to types of UnsignedIntegralConvertCopy
template<typename T0, typename...T1>
concept UnsignedIntegralConvertCopyDecay = std::conjunction<is_unsigned_integral_convert_copy<typename std::decay<T0>::type, typename std::decay<T1>::type>...>::value;

//concept detecting if the two chosen containers are capable of copying to the front with conversion between a multiple of 1 byte (copying unsigned char to be represented as unsigned short, unsigned int or unsigned long; and vice versa), because the inner types are unsigned, only for two ForwardLists
template<typename T0, typename T1>
struct is_unsigned_integral_convert_copy_forward {
    [[maybe_unused]] static const bool value = false;
};

template<template<typename> typename T1, typename...Args1,
        template<typename> typename T2, typename... Args2>
struct is_unsigned_integral_convert_copy_forward<T1<Args1...>, T2<Args2...>> {
    [[maybe_unused]] static const bool value = SequentialContainer<T1<Args1...>, T2<Args2...>> and ForwardList<T1<Args1...>> and
                                               !std::is_same<boost::mp11::mp_first<T1<Args1...>>, boost::mp11::mp_first<T2<Args2...>>>::value and
                                               UnsignedIntegral<boost::mp11::mp_first<T1<Args1...>>, boost::mp11::mp_first<T2<Args2...>>>;
};

template<typename T0, typename...T1>
concept UnsignedIntegralConvertCopyForward = std::conjunction<is_unsigned_integral_convert_copy_forward<T0, T1>...>::value;

//concept handling referenced or pointed to types of UnsignedIntegralConvertCopyForward
template<typename T0, typename...T1>
concept UnsignedIntegralConvertCopyForwardDecay = std::conjunction<is_unsigned_integral_convert_copy_forward<typename std::decay<T0>::type, typename std::decay<T1>::type>...>::value;

//concept detecting if the two chosen containers are capable of copying to the back, both having signed internal types
template<typename T0, typename T1>
struct is_signed_integral_convert_copy {
    [[maybe_unused]] static const bool value = false;
};

template<template<typename> typename T1, typename...Args1,
        template<typename> typename T2, typename... Args2>
struct is_signed_integral_convert_copy<T1<Args1...>, T2<Args2...>> {
    [[maybe_unused]] static const bool value = SequentialContainer<T1<Args1...>, T2<Args2...>> and !ForwardList<T1<Args1...>> and
                                               !std::is_same<boost::mp11::mp_first<T1<Args1...>>, boost::mp11::mp_first<T2<Args2...>>>::value and
                                               SignedIntegral<boost::mp11::mp_first<T1<Args1...>>, boost::mp11::mp_first<T2<Args2...>>>;
};

template<typename T0, typename...T1>
concept SignedIntegralConvertCopy = std::conjunction<is_signed_integral_convert_copy<T0, T1>...>::value;

//concept handling referenced or pointed to types of SignedIntegralConvertCopy
template<typename T0, typename...T1>
concept SignedIntegralConvertCopyDecay = std::conjunction<is_signed_integral_convert_copy<typename std::decay<T0>::type, typename std::decay<T1>::type>...>::value;

//concept detecting if the two chosen containers are capable of copying to the front, both having signed internal types, ForwardList only
template<typename T0, typename T1>
struct is_signed_integral_convert_copy_forward {
    [[maybe_unused]] static const bool value = false;
};

template<template<typename> typename T1, typename...Args1,
        template<typename> typename T2, typename... Args2>
struct is_signed_integral_convert_copy_forward<T1<Args1...>, T2<Args2...>> {
    [[maybe_unused]] static const bool value = SequentialContainer<T1<Args1...>, T2<Args2...>> && ForwardList<T1<Args1...>> &&
                                               !std::is_same<boost::mp11::mp_first<T1<Args1...>>, boost::mp11::mp_first<T2<Args2...>>>::value &&
                                               SignedIntegral<boost::mp11::mp_first<T1<Args1...>>, boost::mp11::mp_first<T2<Args2...>>>;
};

template<typename T0, typename...T1>
concept SignedIntegralConvertCopyForward = std::conjunction<is_signed_integral_convert_copy_forward<T0, T1>...>::value;

//concept handling referenced or pointed to types of SignedIntegralConvertCopyForward
template<typename T0, typename...T1>
concept SignedIntegralConvertCopyForwardDecay = std::conjunction<is_signed_integral_convert_copy_forward<typename std::decay<T0>::type, typename std::decay<T1>::type>...>::value;

//concept detecting two container types with any floating point internal types to be copied to the back or converted into each other
template<typename T0, typename T1>
struct is_floating_point_convert_copy {
    [[maybe_unused]] static const bool value = false;
};

template<template<typename> typename T1, typename...Args1,
        template<typename> typename T2, typename... Args2>
struct is_floating_point_convert_copy<T1<Args1...>, T2<Args2...>> {
    [[maybe_unused]] static const bool value = SequentialContainer<T1<Args1...>, T2<Args2...>> and !ForwardList<T1<Args1...>> and
                                               !std::is_same<boost::mp11::mp_first<T1<Args1...>>, boost::mp11::mp_first<T2<Args2...>>>::value and
                                               FloatingPoint<boost::mp11::mp_first<T1<Args1...>>>;
};

template<typename T0, typename...T1>
concept FloatingPointConvertCopy = std::conjunction<is_floating_point_convert_copy<T0, T1>...>::value;

//concept handling referenced or pointed to types of FloatingPointConvertCopy
template<typename T0, typename...T1>
concept FloatingPointConvertCopyDecay = std::conjunction<is_floating_point_convert_copy<typename std::decay<T0>::type, typename std::decay<T1>::type>...>::value;

//concept detecting two container types with any floating point internal types to be copied to the front or converted into each other, ForwardList only
template<typename T0, typename T1>
struct is_floating_point_convert_copy_forward {
    [[maybe_unused]] static const bool value = false;
};

template<template<typename> typename T1, typename...Args1,
        template<typename> typename T2, typename... Args2>
struct is_floating_point_convert_copy_forward<T1<Args1...>, T2<Args2...>> {
    [[maybe_unused]] static const bool value = SequentialContainer<T1<Args1...>, T2<Args2...>> and ForwardList<T1<Args1...>> and
                                               !std::is_same<boost::mp11::mp_first<T1<Args1...>>, boost::mp11::mp_first<T2<Args2...>>>::value and
                                               FloatingPoint<boost::mp11::mp_first<T1<Args1...>>>;
};

template<typename T0, typename...T1>
concept FloatingPointConvertCopyForward = std::conjunction<is_floating_point_convert_copy_forward<T0, T1>...>::value;

//concept handling referenced or pointed to types of FloatingPointConvertCopyForward
template<typename T0, typename...T1>
concept FloatingPointConvertCopyForwardDecay = std::conjunction<is_floating_point_convert_copy_forward<typename std::decay<T0>::type, typename std::decay<T1>::type>...>::value;

//concept detecting two container types with the first internal type being a signed or unsigned type and the second input type is a floating point that will be copied and rounded into the first type, copy to the back
template<typename T0, typename T1>
struct is_round_convert_copy {
    [[maybe_unused]] static const bool value = false;
};

template<template<typename> typename T1, typename...Args1,
        template<typename> typename T2, typename... Args2>
struct is_round_convert_copy<T1<Args1...>, T2<Args2...>> {
    [[maybe_unused]] static const bool value = SequentialContainer<T1<Args1...>, T2<Args2...>> and !ForwardList<T1<Args1...>> and
                                               !std::is_same<boost::mp11::mp_first<T1<Args1...>>, boost::mp11::mp_first<T2<Args2...>>>::value and
                                               FloatingPoint<boost::mp11::mp_first<T2<Args2...>>> and
                                               !FloatingPoint<boost::mp11::mp_first<T1<Args1...>>>;
};

template<typename T0, typename...T1>
concept RoundConvertCopy = std::conjunction<is_round_convert_copy<T0, T1>...>::value;

//concept handling referenced or pointed to types of RoundConvertCopy
template<typename T0, typename...T1>
concept RoundConvertCopyDecay = std::conjunction<is_round_convert_copy<typename std::decay<T0>::type, typename std::decay<T1>::type>...>::value;

//concept detecting two container types with the first internal type being a signed or unsigned type and the second input type is a floating point that will be copied and rounded into the first type, copy to the front, ForwardList
template<typename T0, typename T1>
struct is_round_convert_copy_forward {
    [[maybe_unused]] static const bool value = false;
};

template<template<typename> typename T1, typename...Args1,
        template<typename> typename T2, typename... Args2>
struct is_round_convert_copy_forward<T1<Args1...>, T2<Args2...>> {
    [[maybe_unused]] static const bool value = SequentialContainer<T1<Args1...>, T2<Args2...>> and ForwardList<T1<Args1...>> and
                                               !std::is_same<boost::mp11::mp_first<T1<Args1...>>, boost::mp11::mp_first<T2<Args2...>>>::value and
                                               FloatingPoint<boost::mp11::mp_first<T2<Args2...>>> and
                                               !FloatingPoint<boost::mp11::mp_first<T1<Args1...>>>;
};

template<typename T0, typename...T1>
concept RoundConvertCopyForward = std::conjunction<is_round_convert_copy_forward<T0, T1>...>::value;

//concept handling referenced or pointed to types of RoundConvertCopyForward
template<typename T0, typename...T1>
concept RoundConvertCopyForwardDecay = std::conjunction<is_round_convert_copy_forward<typename std::decay<T0>::type, typename std::decay<T1>::type>...>::value;

//concept detecting two container types with the both internal objects being classes that can be converted into each other with a dynamic_cast
template<typename T0, typename T1>
struct is_dynamic_cast_convert_copy {
    [[maybe_unused]] static const bool value = false;
};

template<template<typename> typename T1, typename...Args1,
        template<typename> typename T2, typename... Args2>
struct is_dynamic_cast_convert_copy<T1<Args1...>, T2<Args2...>> {
    [[maybe_unused]] static const bool value = SequentialContainer<T1<Args1...>, T2<Args2...>> and !ForwardList<T1<Args1...>> and
                                               std::is_base_of<boost::mp11::mp_first<T1<Args1...>>, boost::mp11::mp_first<T2<Args2...>>>::value and
                                               !std::is_same<boost::mp11::mp_first<T1<Args1...>>, boost::mp11::mp_first<T2<Args2...>>>::value;
};

template<typename T0, typename...T1>
concept DynamicCastConvertCopy = std::conjunction<is_dynamic_cast_convert_copy<T0, T1>...>::value;

//concept handling referenced or pointed to types of DynamicCastConvertCopy
template<typename T0, typename...T1>
concept DynamicCastConvertCopyDecay = std::conjunction<is_dynamic_cast_convert_copy<typename std::decay<T0>::type, typename std::decay<T1>::type>...>::value;

//concept detecting two container types with the both internal objects being classes that can be converted into each other with a dynamic_cast, only ForwardList
template<typename T0, typename T1>
struct is_dynamic_cast_convert_copy_forward {
    [[maybe_unused]] static const bool value = false;
};

template<template<typename> typename T1, typename...Args1,
        template<typename> typename T2, typename... Args2>
struct is_dynamic_cast_convert_copy_forward<T1<Args1...>, T2<Args2...>> {
    [[maybe_unused]] static const bool value = SequentialContainer<T1<Args1...>, T2<Args2...>> and ForwardList<T1<Args1...>> and
                                               std::is_base_of<boost::mp11::mp_first<T1<Args1...>>, boost::mp11::mp_first<T2<Args2...>>>::value and
                                               !std::is_same<boost::mp11::mp_first<T1<Args1...>>, boost::mp11::mp_first<T2<Args2...>>>::value;
};

template<typename T0, typename...T1>
concept DynamicCastConvertCopyForward = std::conjunction<is_dynamic_cast_convert_copy_forward<T0, T1>...>::value;

//concept handling referenced or pointed to types of DynamicCastConvertCopyForward
template<typename T0, typename...T1>
concept DynamicCastConvertCopyForwardDecay = std::conjunction<is_dynamic_cast_convert_copy_forward<typename std::decay<T0>::type, typename std::decay<T1>::type>...>::value;

template<typename T0>
struct is_sequential_container_extended {
    [[maybe_unused]] static const bool value = false;
};

//extended container detection mechanisms
//concept detecting a sequential container including strings
template<template<typename> class T1, typename...Args1>
struct is_sequential_container_extended<T1<Args1...>> {
    [[maybe_unused]] static const bool value = !Map<T1<Args1...>> and !Set<T1<Args1...>> and (SequentialContainer<T1<Args1...>> or String<T1<Args1...>>);
};

template<typename... Args>
concept SequentialContainerExtended = std::conjunction<is_sequential_container_extended<Args>...>::value;

//concept handling referenced or pointed to types of SequentialContainerExtendedDecay
template<typename... Args>
concept SequentialContainerExtendedDecay = std::conjunction<is_sequential_container_extended<typename std::decay<Args>::type>...>::value;

//concept handling the case that some internal types match
template<typename T0, typename T1>
struct is_any_similar_inside {
    [[maybe_unused]] static const bool value = false;
};

template<template<typename> typename T0, typename...Args,typename T1>
struct is_any_similar_inside<T0<Args...>, T1> {
    [[maybe_unused]] static const bool value = std::disjunction<boost::mp11::mp_similar<T1,Args>...>::value;
};

template<typename T0, typename ...T1>
concept AnySimilarInside = std::conjunction<is_any_similar_inside<T0, T1>...>::value;

//complex datatype concepts for LEVEL2

//pre definitions of LEVEL2 base types
template<OptionalRecursiveContainer... T_values>
class aiHeuristicVariant;

class BigBase;

class BigUnsignedInteger;

class BigInteger;

class BigQuotient;

class BigFloat;

class BigComplex;

class BigMatrix;

class BigNumber;

//concept to detect an aiHeuristic
template<typename T1>
struct is_ai_heuristic {
    static const bool value = false;
};

template<template<typename> typename T1, typename... Args>
struct is_ai_heuristic<T1<Args...>> {
    static const bool value = boost::mp11::mp_similar<aiHeuristicVariant<>,T1<Args...>>::value;
};

template<typename... T1>
concept AIHeristicConcept = std::conjunction<is_ai_heuristic<T1>...>::value;

//concept to detect an BigBase
template<typename T1>
struct is_big_base {
    static const bool value = boost::mp11::mp_same<T1,BigBase>::value;
};

template<typename... T1>
concept BigBaseConcept = std::conjunction<is_big_base<T1>...>::value;

//concept to detect a BigUnsignedInteger
template<typename T1>
struct is_big_unsigned_integer {
    static const bool value = boost::mp11::mp_same<T1,BigUnsignedInteger>::value;
};

template<typename... T1>
concept BigUnsignedIntegerConcept = std::conjunction<is_big_unsigned_integer<T1>...>::value;

//concept to detect a BigInteger (signed/unsigned)
template<typename T1>
struct is_big_integer {
    static const bool value = boost::mp11::mp_same<T1,BigInteger>::value;
};

template<typename... T1>
concept BigIntegerConcept = std::conjunction<is_big_integer<T1>...>::value;

//concept to detect a BigQuotient
template<typename T1>
struct is_big_quotient {
    static const bool value = boost::mp11::mp_same<T1,BigQuotient>::value;
};

template<typename... T1>
concept BigQuotientConcept = std::conjunction<is_big_quotient<T1>...>::value;

//concept to detect a BigFloat
template<typename T1>
struct is_big_float {
    static const bool value = boost::mp11::mp_same<T1,BigFloat>::value;
};

template<typename... T1>
concept BigFloatConcept = std::conjunction<is_big_float<T1>...>::value;

//concept to detect a BigComplex
template<typename T1>
struct is_big_complex {
    static const bool value = boost::mp11::mp_same<T1,BigComplex>::value;
};

template<typename... T1>
concept BigComplexConcept = std::conjunction<is_big_float<T1>...>::value;

//concept to detect a BigMatrix that contains n-dimensional numbers
template<typename T1>
struct is_big_matrix {
    static const bool value = boost::mp11::mp_same<T1,BigMatrix>::value;
};

template<typename... T1>
concept BigMatrixConcept = std::conjunction<is_big_matrix<T1>...>::value;

//concept to detect BigNumber
template<typename T1>
struct is_big_number {
    static const bool value = boost::mp11::mp_same<T1,BigNumber>::value;
};

//concept to detect any higher scale numerical type
template<typename... T1>
concept BigNumberConcept = std::conjunction<std::disjunction<is_ai_heuristic<T1>,is_big_base<T1>,is_big_unsigned_integer<T1>,is_big_integer<T1>,is_big_quotient<T1>,is_big_float<T1>,is_big_complex<T1>,is_big_matrix<T1>,is_big_number<T1>>...>::value;

//concept detecting a sequential container, a deep variant or any number container
template<class... T1>
concept OptionalRecursiveNumberContainer = std::conjunction<std::disjunction<is_sequential_container<T1>, is_deep_variant_container<T1>,is_ai_heuristic<T1>,is_big_base<T1>,is_big_unsigned_integer<T1>,is_big_integer<T1>,is_big_quotient<T1>,is_big_float<T1>,is_big_complex<T1>,is_big_matrix<T1>,is_big_number<T1>>...>::value;

//concept doing OptionalRecursiveNumberContainer that can still be detected as reference or pointer
template<class... T1>
concept OptionalRecursiveNumberContainerDecay = std::conjunction<std::disjunction<is_sequential_container<typename std::decay<T1>::type>, is_deep_variant_container<typename std::decay<T1>::type>,is_ai_heuristic<typename std::decay<T1>::type>,is_big_base<typename std::decay<T1>::type>,is_big_unsigned_integer<typename std::decay<T1>::type>,is_big_integer<typename std::decay<T1>::type>,is_big_quotient<typename std::decay<T1>::type>,is_big_float<typename std::decay<T1>::type>,is_big_complex<typename std::decay<T1>::type>,is_big_matrix<typename std::decay<T1>::type>,is_big_number<typename std::decay<T1>::type>>...>::value;

template< typename ... Args >
std::string stringer(Args & ... args ){
    std::ostringstream stream;
    using List= int[];
    (void)List{0, ( (void)(stream << args), 0 ) ... };

    return stream.str();
}

#endif //INTEGRATION_PROJECT_CONCEPTTYPES_H
