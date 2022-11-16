//
// Created by benjamin-elias on 02.05.22.
//

#ifndef CMAKE_BUILD_DEBUG_ULTIHASH_HASHLIBRARIES_H
#define CMAKE_BUILD_DEBUG_ULTIHASH_HASHLIBRARIES_H

#include "conceptTypes.h"
#include "logging_boost.h"

namespace tuple_hash_lib {
    template<typename...Args>
    struct tuple {
    public:
        //unsigned long long int a_var,b_var;
        std::tuple<Args...> var;

        explicit tuple(Args ...args) {
            var = std::make_tuple(args...);
        }
        explicit tuple(std::tuple<Args...> arg) {
            var = arg;
        }
    };

    template<typename T3, T3...Ints, class T1, class T2>
    bool tuple_equals(T1 const &arg1, T2 const &arg2) {
        return std::tuple_size<typename std::decay<T1>::type>::value ==
               std::tuple_size<typename std::decay<T2>::type>::value and
               ((std::get<Ints>(arg1) == std::get<Ints>(arg2)) and ... and true);
    }
    template<typename...Args>
    bool operator==(tuple<Args...> const &a, tuple<Args...> const &b) {
        return tuple_equals<std::make_index_sequence<sizeof...(Args)>>(a.var, b.var);
    }
    template<typename T3, T3...Ints, class T1>
    std::size_t tuple_hash(T1 const &args) {
        return (boost::hash<decltype(std::get<Ints>(args))>(std::get<Ints>(args)) ^ ... ^ (std::size_t) 0);
    }
    template<typename...Args>
    std::size_t hash_value(tuple<Args...> const &t) {
        return tuple_hash<std::make_index_sequence<sizeof...(Args)>>(t.var);
    }
    template<typename T3, T3...Ints, class T1, class T2>
    std::size_t tuple_greater(T1 const &args1,T2 const &args2) {
        return std::tuple_size<typename std::decay<T1>::type>::value ==
               std::tuple_size<typename std::decay<T2>::type>::value and
               (boost::hash<decltype(std::get<Ints>(args1))>(std::get<Ints>(args1)) ^ ... ^ (std::size_t) 0) <
               (boost::hash<decltype(std::get<Ints>(args2))>(std::get<Ints>(args2)) ^ ... ^ (std::size_t) 0);
    }
    template<typename...Args>
    bool operator<(tuple<Args...> const &a, tuple<Args...> const &b) {
        return tuple_greater<std::make_index_sequence<sizeof...(Args)>>(a.var, b.var);
    }
}

namespace container_hash_lib {
    template<class Arg, std::enable_if_t<is_sequential_container<Arg>::value, bool> = true>
    struct container {
    public:
        //unsigned long long int a_var,b_var;
        Arg var;

        explicit container(Arg arg) {
            var = arg;
        }

        container() = default;
    };

    template<class Arg1, class Arg2>
    bool operator==(Arg1 const &a, Arg2 const &b) {
        return boost::range::equal(a, b);
    }

    template<class Arg>
    std::size_t hash_value(Arg const &t) {
        return boost::hash_range(t.begin(), t.end());
    }
}

template<std::size_t nth, std::size_t... Head, std::size_t... Tail, typename... Types>
constexpr auto remove_nth_element_impl(std::index_sequence<Head...>, std::index_sequence<Tail...>, std::tuple<Types...> const& tup) {
return std::tuple{
std::get<Head>(tup)...,
// We +1 to refer one element after the one removed
std::get<Tail + nth + 1>(tup)...
};
}

template<std::size_t nth, typename... Types>
constexpr auto filterTupElement(std::tuple<Types...> const& tup) {
return remove_nth_element_impl<nth>(
        std::make_index_sequence<nth>(), // We -1 to drop one element
std::make_index_sequence<sizeof...(Types) - nth - 1>(),
        tup
);
}

#endif //CMAKE_BUILD_DEBUG_ULTIHASH_HASHLIBRARIES_H
