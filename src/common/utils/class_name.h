#ifndef UTILS_CLASS_NAME_H
#define UTILS_CLASS_NAME_H

#include <string>
#include <typeinfo>

#ifdef __GNUG__
#include <cxxabi.h>
#endif

namespace uh::cluster {

template <typename T> std::string class_name() {
#ifdef __GNUG__
    int status;
    char* buffer = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
    std::string name = buffer;
    free(buffer);
#else
    std::string name = typeid(T).name();
#endif

    return name;
}

} // namespace uh::cluster

#endif
