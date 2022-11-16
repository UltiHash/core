//
// Created by benjamin-elias on 22.07.22.
//

#ifndef INC_20_DATABASE_SUPPORT_GLOBAL_CUSTOM_FUNCTIONS_H
#define INC_20_DATABASE_SUPPORT_GLOBAL_CUSTOM_FUNCTIONS_H

#include "conceptTypes.h"

namespace custom_function{
    template<typename Args>
    auto unpack_variadic(Args s) {
        std::list<boost::mp11::mp_first<boost::mp11::mp_list<Args>>> res;
        res.push_back(s);
        return res;
    }

    template<typename Arg1,typename... Args>
    auto unpack_variadic(Arg1 s0, Args...s) {
        std::list<boost::mp11::mp_first<boost::mp11::mp_list<Args...>>> res{s0};
        auto toEnd = unpack_variadic(s...);
        res.splice(res.end(),toEnd);
        return res;
    }

    template<typename F,typename R>
    std::string replace_string_occurencies(const std::string& s, F find,R replace){
        std::string output=s;
        if constexpr(std::is_same_v<F,char> and std::is_same_v<R,char>){
            std::ranges::replace(s,find,replace);
            return output;
        }
        std::string::size_type pos = output.find(find);
        while (pos != std::string::npos) {
            if constexpr (std::is_same_v<R,char>){
                output.replace(pos, s.size(), reinterpret_cast<const char *>(replace));
            }
            else{
                output.replace(pos, s.size(), replace);
            }
            output.replace(pos, output.size(), replace);
            pos = output.find(find, pos);
        }
        return s;
    }
}

#endif //CMAKE_BUILD_DEBUG_ULTIHASH_GLOBAL_CUSTOM_FUNCTIONS_H
