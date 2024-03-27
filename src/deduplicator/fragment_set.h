#ifndef UH_CLUSTER_FRAGMENT_SET_H
#define UH_CLUSTER_FRAGMENT_SET_H

#include "common/utils/common.h"
#include "fragment_set_element.h"
#include "fragment_set_log.h"
#include <common/global_data/global_data_view.h>
#include <queue>
#include <set>
#include <utility>

namespace uh::cluster {

class fragment_set {

public:
    /**
     * @brief response structure used to communicate the results of the #find
     * method
     */
    struct response {
        /**
         * @brief fragment_set_element indicating the preceding lexicographic
         * neighbour
         */
        std::optional<const std::reference_wrapper<const fragment_set_element>>
            low;
        /**
         * @brief fragment_set_element indicating the succeeding lexicographic
         * neighbour
         */
        std::optional<const std::reference_wrapper<const fragment_set_element>>
            high;
        /**
         * @brief iterator used as a placement hint to reduce the complexity of
         * an insert call
         */
        const std::set<fragment_set_element>::const_iterator hint;
    };

    /**
     * @brief Creates a fragment_set instance
     * Upon construction, any existing log in #set_log_path is replayed to
     * reconstruct the set. If no prior log exists in #set_log_path, a new one
     * is created. The fragment_set holds fragment_set_elements, which only
     * contain the address and the prefix and not the full body of a fragment to
     * enable space-efficient prefix-lookup.
     * @param set_log_path A path specifying the location of the log file.
     * @param storage The #global_data_view instance used for looking
     * up full fragment content beyond the prefix.
     * @param enable_replay Temporary, optional switch for disabling replay of
     * the fragment_set_log. Default value: true.
     */
    fragment_set(const std::filesystem::path& set_log_path,
                 global_data_view& storage,
                 bool enable_replay = true);

    /**
     * @brief Searches the system for lexicographic neighbours of #data
     * The lexicographic neighbours of #data retrieved by this operation are
     * required to identify fragments with the longest common prefix..
     * @param data The full fragment content
     * @return A response structure containing the lexicographic neighbours of
     * #data as wall as an iterator used as a hint for the insert operation.
     */
    response find(std::string_view data);

    /**
     * @brief Inserts the provided fragment into the fragment_set
     * The fragment provided in #data is inserted into the fragment_set.
     * @param pointer A constant reference to a uint128_t with the address of
     * the full fragment
     * @param data Full fragment content
     * @param hint A constant reference to the std::set::const_iterator yielded
     * by the #find method
     */
    void insert(const uint128_t& pointer, const std::string_view& data,
                const std::set<fragment_set_element>::const_iterator& hint);

    /**
     * @brief synchronizes the fragment_set log file with the underlying storage
     * device
     *
     * Calls std::fstream::flush() on log file to synchronize cached writes
     * to persistent storage.
     */
    void flush();

private:
    global_data_view& m_storage;
    std::set<fragment_set_element> m_set;
    std::shared_mutex m_mutex;
    fragment_set_log m_set_log;
};

} // end namespace uh::cluster

#endif // UH_CLUSTER_FRAGMENT_SET_H
