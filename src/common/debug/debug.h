#ifndef UH_CLUSTER_COMMON_UTILS_DEBUG_H
#define UH_CLUSTER_COMMON_UTILS_DEBUG_H

#include "common/telemetry/log.h"
#include "config.h"

#include <boost/algorithm/string.hpp>
#include <iomanip>
#include <ostream>
#include <source_location>

namespace std {

inline ostream& operator<<(ostream& out, const source_location& loc) {
    std::string path = loc.file_name();
    boost::replace_all(path, PROJECT_SOURCE_DIR, "");

    out << loc.function_name() << " -- " << path << ":" << loc.line();
    return out;
}

} // namespace std

class context_logger {
public:
    context_logger(
        const std::string& context = "",
        const std::source_location& loc = std::source_location::current())
        : m_context(context),
          m_loc(loc) {
        LOG_DEBUG() << "ctx enter: " << this->context() << ", loc: " << m_loc;
    }

    ~context_logger() {
        LOG_DEBUG() << "ctx leave: " << context() << ", loc: " << m_loc;
    }

private:
    std::string context() const {
        if (!m_context.empty()) {
            return " " + m_context + ": ";
        }

        return "";
    }

    std::string m_context;
    std::source_location m_loc;
};

#ifdef ENABLE_TRACE
#define LOG_CONTEXT() context_logger __ctx
#else
#define LOG_CONTEXT()
#endif

inline std::string dbg_to_string(std::string v) { return v; }

std::string dbg_to_string(auto v);

template <typename t>
inline std::string dbg_to_string(const std::optional<t>& v) {
    if (!v) {
        return "<std::nullopt>";
    }

    return dbg_to_string(*v);
}

inline std::string dbg_to_string(auto v) { return std::to_string(v); }

/**
 * FOR_EACH(MACRO, ...)
 *
 * Apply a macro to each of the passed parameters.
 *
 * Note: this really hard to read preprocessor foo was simply copied
 * from https://www.scs.stanford.edu/~dm/blog/va-opt.html which also contains
 * some nice explainatory prose.
 */
#define PARENS ()

#define EXPAND(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__

#define FOR_EACH(macro, ...)                                                   \
    __VA_OPT__(EXPAND(FOR_EACH_HELPER(macro, __VA_ARGS__)))
#define FOR_EACH_HELPER(macro, a1, ...)                                        \
    macro(a1) __VA_OPT__(FOR_EACH_AGAIN PARENS(macro, __VA_ARGS__))
#define FOR_EACH_AGAIN() FOR_EACH_HELPER

/**
 * DUMP_VARS(DEST, VAR 1 [, VAR 2 ...])
 *
 * Output values of variables/expressions. Note: parameters will be evaluated
 * multiple times, so use expressions without side-effects.
 */
#define _DUMP_VAR(X) << " " << #X "=" << dbg_to_string(X) << ","

#define DUMP_VARS(DEST, ...)                                                   \
    DEST << std::source_location::current() FOR_EACH(_DUMP_VAR, __VA_ARGS__)

#endif
