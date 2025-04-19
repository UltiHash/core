#define BOOST_TEST_MODULE "path tests"

#include <boost/test/unit_test.hpp>

namespace uh::cluster {

struct path_t {
    std::string m_basename;
    std::string m_key;
    path_t* m_parent;
    path_t(std::string&& basename, path_t* parent = nullptr)
        : m_basename{std::move(basename)},
          m_parent{parent} {
        if (m_parent) {
            m_key = parent->m_key + "/" + m_basename;
        } else {
            m_key = "/" + m_basename;
        }
    }

    operator std::string() const { return m_key; }
};

#define DEFINE_PATH_STRUCTURE_WITH_ID(struct_t, member_t, member)              \
    struct struct_t : public path_t {                                          \
        struct next_t : public path_t {                                        \
            member_t member;                                                   \
            next_t(std::string&& basename, path_t* parent)                     \
                : path_t{std::move(basename), parent},                         \
                  member(#member, this) {}                                     \
        };                                                                     \
        template <std::integral T> auto at(T index) {                          \
            return next_t{std::to_string(index), this};                        \
        }                                                                      \
        struct_t(std::string&& basename, path_t* parent)                       \
            : path_t{std::move(basename), parent} {}                           \
    };

#define DEFINE_PATH_STRUCTURE(struct_t, member_t, member)                      \
    struct struct_t : public path_t {                                          \
        member_t member;                                                       \
                                                                               \
        struct_t(std::string&& basename, path_t* parent)                       \
            : path_t{std::move(basename), parent},                             \
              member(#member, this) {}                                         \
    };

DEFINE_PATH_STRUCTURE_WITH_ID(storage_states_t, path_t, storage_states)
DEFINE_PATH_STRUCTURE_WITH_ID(internal_t, storage_states_t, storage_states)
DEFINE_PATH_STRUCTURE(storage_groups_t, internal_t, internal);
DEFINE_PATH_STRUCTURE(uh_t, storage_groups_t, storage_groups);

inline uh_t ns{"uh", nullptr};

BOOST_AUTO_TEST_CASE(supports_slash_operator) {

    auto path = ns.storage_groups.internal.at(2).storage_states.at(3);
    BOOST_TEST(std::string(path) ==
               "/uh/storage_groups/internal/2/storage_states/3");
}

} // namespace uh::cluster
