//
// Created by masi on 5/19/23.
//

#ifndef CORE_MMAP_SET_H
#define CORE_MMAP_SET_H

#include <span>
#include <cstring>
#include <atomic>
#include <optional>

#include "mmap_storage.h"

namespace uh::dbn::storage::smart {


struct fragment {
    uint64_t data_offset;
    size_t size;
};

class mmap_set {


    enum color_t : uint8_t
    {
        RED = 0,
        BLACK = 1
    };

    struct mmap_node {
        fragment frag;
        uint64_t parent;
        uint64_t left;
        uint64_t right;
        color_t color;
    };
    struct node {
        uint64_t offset;
        mmap_node* m_node;
    };

public:

    struct search_result {
        std::optional <std::pair <uint64_t, std::string_view>> lower;
        std::optional <std::pair <uint64_t, std::string_view>> match;
        std::optional <std::pair <uint64_t, std::string_view>> upper;
        uint64_t hint {};
    private:
        int comp {};
        friend mmap_set;
    };

    mmap_set(mmap_storage& data_store, std::filesystem::path file);

    uint64_t insert_data (const std::string_view& frag);
    uint64_t insert_index (const std::string_view& frag, uint64_t data_offset, uint64_t hint = NILL_OFFSET);



    search_result find (const std::string_view &frag, uint64_t hint = NILL_OFFSET);

    uint64_t remove (fragment frag, uint64_t hint);


private:

    std::pair <uint64_t, bool> resolve_hint (uint64_t hint, const std::string_view& frag);

    void extend_mapping ();

    void balance (node& z);

    void left_rotate (node& x);

    void right_rotate (node& x);

    inline node get_node (uint64_t offset) noexcept;

    inline node add_node () noexcept;

    inline void set_root (node &x) noexcept;

    inline int comp (const std::string_view& new_fragment, const fragment& f);

    constexpr static size_t SET_INIT_FILE_SIZE = 1024ul;
    constexpr static size_t SET_FILE_EXTEND_LIMIT = 256ul;
    constexpr static uint64_t NILL_OFFSET = 2 * sizeof (uint64_t);

    mmap_storage& m_data_store;
    size_t m_file_size = SET_INIT_FILE_SIZE;
    std::filesystem::path m_file_path;
    node m_nil {};
    std::atomic <uint64_t*> m_root;
    std::atomic <uint64_t*> m_end;
    char* m_index_store = nullptr;

};

} // end namespace uh::dbn::storage::smart
#endif //CORE_MMAP_SET_H
