//
// Created by benjamin-elias on 17.12.22.
//

#ifndef UHLIBCOMMON_TREE_RADIX_CUSTOM_H
#define UHLIBCOMMON_TREE_RADIX_CUSTOM_H

#include "logging/logging_boost.h"
#include "trees/tree_storage_config.h"
#include "util/compression_custom.h"
#include <vector>
#include <shared_mutex>
#include <mutex>
#include <list>
#include <deque>
#include <array>
#include <set>
#include <ranges>
#include <algorithm>
#include <algorithm>
#include <openssl/sha.h>
#include <type_traits>
#include <execution>
#include <cmath>
#include <iterator>

namespace uh::trees {
    //because it takes at least 2 bytes to describe a deeper encoding action
    std::size_t simd_count{};
    std::shared_mutex simd_protect{};

    template<class DataReference = std::vector<unsigned char>>
    struct tree_radix_custom {
    public:
        std::vector<unsigned char> data{};
        //any binary vector string
        DataReference data_ref{};
        //the first element of data is cut off to children except on root if it's a new tree
        //TODO: use two children and block_swarm_offsets to scan forward and backward
        std::vector<std::tuple<std::vector<tree_radix_custom *>, unsigned char>> children{};//multiple targets that can follow a node for each letter
        std::size_t block_swarm_offset{};//offset of block beginning from root

        tree_radix_custom() = default;

        virtual ~tree_radix_custom() {
            for (auto &item1: children) {
                for (auto &item2: std::get<0>(item1)) {
                    delete item2;
                }
            }
        }

        template<class Container>
        explicit tree_radix_custom(const Container &bin){
            std::copy(bin.begin(), bin.end(),std::back_inserter(data));
        }

        [[nodiscard]] std::size_t size() const {
            return data.size();
        }

        std::vector<tree_radix_custom *> child_vector(unsigned char i) {
            if (children.empty())return {};
            for (const auto &item: children) {
                if (std::get<1>(item) == i) {
                    return std::get<0>(item);
                }
            }
            return {};
        }

        template<bool reverse = false>
        void child_put(tree_radix_custom *input_tree, unsigned char first_letter) {//TODO: choose reverse children
            auto child_vec_append = child_vector(first_letter);
            if (child_vec_append.empty()) {
                //child would have been created and the append size would have been added to a new tree
                children.emplace_back(std::vector<tree_radix_custom *>{input_tree}, first_letter);
            } else {
                //on search there was no match on the tree node, so we assume that a new node referenced by this node will be created carrying append
                //the reason why there is the correct character available but no match detected by search is the MINIMUM_MATCH_SIZE that failed, we will respect that
                decltype(child_vec_append.begin()) find_it;
                std::unique_lock lock(simd_protect);
                if (simd_count < SIMD_UNITS) {
                    simd_count += 1;
                    lock.unlock();
                    find_it = std::find(std::execution::unseq, child_vec_append.begin(), child_vec_append.end(),
                                        input_tree);
                    lock.lock();
                    simd_count -= 1;
                    lock.unlock();
                } else {
                    lock.unlock();
                    find_it = std::find(child_vec_append.begin(), child_vec_append.end(), input_tree);
                }
                if (find_it == child_vec_append.end()) {
                    child_vec_append.emplace_back(input_tree);
                    lock.lock();
                    if (simd_count < SIMD_UNITS) {
                        simd_count += 1;
                        lock.unlock();
                        if constexpr (!reverse) {
                            std::sort(child_vec_append.begin(), child_vec_append.end(), [](auto &a, auto &b) {
                                return lexicographical_compare(std::execution::unseq, a->data.cbegin(),
                                                               a->data.cend(),
                                                               b->data.cbegin(), b->data.cend());
                            });
                        } else {
                            std::sort(child_vec_append.begin(), child_vec_append.end(), [](auto &a, auto &b) {
                                return lexicographical_compare(std::execution::unseq, a->data.crbegin(),
                                                               a->data.crend(), b->data.crbegin(), b->data.crend());
                            });
                        }
                        lock.lock();
                        simd_count -= 1;
                        lock.unlock();
                    } else {
                        lock.unlock();
                        if constexpr (!reverse) {
                            std::sort(child_vec_append.begin(), child_vec_append.end(), [](auto &a, auto &b) {
                                return lexicographical_compare(a->data.cbegin(), a->data.cend(), b->data.cbegin(),
                                                               b->data.cend());
                            });
                        } else {
                            std::sort(child_vec_append.begin(), child_vec_append.end(), [](auto &a, auto &b) {
                                return lexicographical_compare(a->data.crbegin(), a->data.crend(), b->data.crbegin(),
                                                               b->data.crend());
                            });
                        }
                    }
                }
            }
            std::sort(children.begin(), children.end(), [](auto &a, auto &b) {
                return std::get<1>(a) < std::get<1>(b);
            });
        }

        [[maybe_unused]] bool child_delete(tree_radix_custom *input_tree, unsigned char first_letter) {
            auto child_vec_append = child_vector(first_letter);
            if (child_vec_append.empty()) {
                return false;
            } else {
                //on search there was no match on the tree node, so we assume that a new node referenced by this node will be created carrying append
                //the reason why there is the correct character available but no match detected by search is the MINIMUM_MATCH_SIZE that failed, we will respect that
                decltype(child_vec_append.begin()) find_beg;
                std::unique_lock lock(simd_protect);
                if (simd_count < SIMD_UNITS) {
                    simd_count += 1;
                    lock.unlock();
                    find_beg = std::find(std::execution::unseq, child_vec_append.begin(), child_vec_append.end(),
                                         input_tree);
                    lock.lock();
                    simd_count -= 1;
                    lock.unlock();
                } else {
                    lock.unlock();
                    find_beg = std::find(child_vec_append.begin(), child_vec_append.end(), input_tree);
                }
                if (find_beg == child_vec_append.end()) {
                    child_vec_append.erase(find_beg);
                    if (child_vec_append.empty()) {
                        return child_delete_letter(first_letter);
                    }
                    return true;
                } else return false;
            }
        }

        bool child_delete_letter(unsigned char delete_letter) {
            auto item_beg = children.begin();
            while (item_beg != children.end()) {
                if (std::get<1>(*item_beg) == delete_letter) {
                    children.erase(item_beg);
                    return true;
                }
            }
            return false;
        }

        /*std::vector<std::tuple<std::vector<unsigned char>::const_iterator,std::vector<unsigned char>::const_iterator,std::vector<unsigned char>::const_iterator>>*/
        template<class ContainerData, class ContainerBinary, bool reverse = false>
        auto
        compare_ultihash(ContainerData &data_cont, ContainerBinary &binary_cont) {
            //if input does only fit to a shorter string as a subset of data, count becomes negative, else positive including ß
            //data offset iterator and start and end of input
            std::vector<std::tuple<std::size_t, std::size_t>> matches{};//data offset beginning found, end offset from beginning
            if (data_cont.empty() || binary_cont.empty())return matches;
            std::size_t current_offset = 0;
            //search forward through data
            if constexpr (!reverse) {
                do {
                    //first element match
                    std::unique_lock lock(simd_protect);
                    if (simd_count < SIMD_UNITS) {
                        simd_count += 1;
                        lock.unlock();
                        current_offset = std::distance(data_cont.begin(), std::find(std::execution::unseq,
                                                                                    data_cont.begin() + current_offset,
                                                                                    data_cont.end(),
                                                                                    *binary_cont.begin()));
                        lock.lock();
                        simd_count -= 1;
                        lock.unlock();
                    } else {
                        lock.unlock();
                        current_offset = std::distance(data_cont.begin(),
                                                       std::find(data_cont.begin() + current_offset, data_cont.end(),
                                                                 *binary_cont.begin()));
                    }

                    //search how long input matches
                    std::pair<decltype(data_cont.begin()), decltype(data_cont.end())> found;
                    lock.lock();
                    if (simd_count < SIMD_UNITS) {
                        simd_count += 1;
                        lock.unlock();
                        found = std::mismatch(std::execution::unseq, data_cont.begin() + current_offset,
                                              data_cont.end(), binary_cont.begin(), binary_cont.end());
                        lock.lock();
                        simd_count -= 1;
                        lock.unlock();
                    } else {
                        lock.unlock();
                        found = std::mismatch(data_cont.begin() + current_offset, data_cont.end(), binary_cont.begin(),
                                              binary_cont.end());
                    }

                    if (std::max((long) std::distance(data_cont.begin() + current_offset, found.first) - 1, (long) 0) >=
                        MINIMUM_MATCH_SIZE) {
                        matches.emplace_back(current_offset, std::max(
                                (long) std::distance(data_cont.begin() + current_offset, found.first) - 1, (long) 0));
                    }
                    if (data_cont.begin() + current_offset != data_cont.end())current_offset++;
                } while (data_cont.begin() + current_offset != data_cont.end());
            } else {
                do {
                    //first element match
                    std::unique_lock lock(simd_protect);
                    if (simd_count < SIMD_UNITS) {
                        simd_count += 1;
                        lock.unlock();
                        current_offset = std::distance(data_cont.rbegin(), std::find(std::execution::unseq,
                                                                                     data_cont.rbegin() +
                                                                                     current_offset, data_cont.rend(),
                                                                                     *binary_cont.begin()));
                        lock.lock();
                        simd_count -= 1;
                        lock.unlock();
                    } else {
                        lock.unlock();
                        current_offset = std::distance(data_cont.rbegin(),
                                                       std::find(data_cont.rbegin() + current_offset, data_cont.rend(),
                                                                 *binary_cont.begin()));
                    }

                    //search how long input matches
                    std::pair<decltype(data_cont.rbegin()), decltype(data_cont.rend())> found;
                    lock.lock();
                    if (simd_count < SIMD_UNITS) {
                        simd_count += 1;
                        lock.unlock();
                        found = std::mismatch(std::execution::unseq, data_cont.rbegin() + current_offset,
                                              data_cont.rend(), binary_cont.begin(), binary_cont.end());
                        lock.lock();
                        simd_count -= 1;
                        lock.unlock();
                    } else {
                        lock.unlock();
                        found = std::mismatch(data_cont.rbegin() + current_offset, data_cont.rend(),
                                              binary_cont.begin(), binary_cont.end());
                    }

                    if (std::max((long) std::distance(data_cont.rbegin() + current_offset, found.first) - 1,
                                 (long) 0) >= MINIMUM_MATCH_SIZE) {
                        matches.emplace_back(current_offset, std::max(
                                (long) std::distance(data_cont.rbegin() + current_offset, found.first) - 1, (long) 0));
                    }
                    if (data_cont.rbegin() + current_offset != data_cont.rend())current_offset++;
                } while (data_cont.rbegin() + current_offset != data_cont.rend());
            }

            return matches;
        }

    public:

        template<class ContainerString, bool reverse = false,
                std::enable_if_t<std::is_same<std::string, ContainerString>::value, bool> = true>
        auto
        add(ContainerString &cont_string) {
            std::vector<unsigned char> input{};
            std::copy(cont_string.begin(), cont_string.end(),std::back_inserter(input));
            return add(input);
        }

        //returns total size integrated, new space used uncompressed, new space used compressed, list of tree references of <offset_ELEMENT,modified_LIST,added_LIST> tree nodes
        template<class ContainerBinary, bool reverse = false,
                std::enable_if_t<!std::is_same<std::string, ContainerBinary>::value, bool> = true>
        std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::list<std::tuple<std::set<tree_radix_custom *>, std::set<tree_radix_custom *>>>>>
        add(ContainerBinary &binary_cont) {
            //first search existing structure and add into the last tree to insert potentially missing information

            //uncompressed input
            if (binary_cont.empty()) {
                return {};
            }
            std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t>>,std::size_t, std::size_t>> search_index = search<ContainerBinary, reverse>(binary_cont);//std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t>,std::size_t, std::size_t>>>

            //some element and an end element at least required
            //TODO: add cross update from forward and backward children

            std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::list<std::tuple<std::set<tree_radix_custom *>, std::set<tree_radix_custom *>>>>> out_change_tuple_out{};
            //cases for search index: its empty or it has content and with that a last tree element
            //empty should never happen since it stacks new information to the end
            if (search_index.empty()) {
                std::tuple<std::size_t, std::size_t, std::size_t, std::list<std::tuple<std::set<tree_radix_custom *>, std::set<tree_radix_custom *>>>> out_change_tuple{};
                auto tmp_vec = std::vector<unsigned char>{binary_cont.begin(), binary_cont.end()};
                std::get<0>(out_change_tuple) += binary_cont.size();
                std::get<1>(out_change_tuple) += binary_cont.size();
                if constexpr (!reverse) {
                    uh::util::compression_custom comp{};
                    std::get<2>(out_change_tuple) += comp.compress(tmp_vec).size();
                } else {
                    std::reverse(tmp_vec.begin(), tmp_vec.end());
                    uh::util::compression_custom comp{};
                    std::get<2>(out_change_tuple) += comp.compress(tmp_vec).size();
                }
                auto *new_tree = new tree_radix_custom{tmp_vec};
                new_tree->block_swarm_offset += block_swarm_offset + 1;
                std::get<3>(out_change_tuple).emplace_back(std::set<tree_radix_custom *>{},
                                                           std::set<tree_radix_custom *>{
                                                                   new_tree});//only add a new tree
                out_change_tuple_out.push_back(out_change_tuple);
            }
            //cases for last tree if it exists, binary fit in: match from the beginning on, match in the middle, match until the end, total match
            //all lists contain lists with a last element that had multiple matches; add up all matches
            auto tree_node_results = search_index.begin();

            while (tree_node_results != search_index.end()) {
                std::tuple<std::size_t, std::size_t, std::size_t, std::list<std::tuple<std::set<tree_radix_custom *>, std::set<tree_radix_custom *>>>> out_change_tuple{};

                tree_radix_custom *tree_match_pointer = std::get<0>(*tree_node_results);
                std::set<tree_radix_custom *> modified{}, added{};//changes to be written to disk in the form of tree pointers
                std::vector<std::tuple<std::size_t, std::size_t, tree_radix_custom *>> actively_changing_trees{};//data offset and binary range from offset
                std::for_each(std::get<1>(*tree_node_results).begin(), std::get<1>(*tree_node_results).end(),
                              [&actively_changing_trees, &tree_match_pointer](auto &item) {
                                  actively_changing_trees.emplace_back(std::get<0>(item), std::get<1>(item),
                                                                       tree_match_pointer);
                              });

                auto match_beg = actively_changing_trees.begin();//                                         found beginning                             found end                        data on tree begin at found begin
                while (match_beg !=
                       actively_changing_trees.end()) {//update loop on std::vector<std::tuple<std::vector<unsigned char>::const_iterator, std::vector<unsigned char>::const_iterator, std::vector<unsigned char>::const_iterator>>

                    ///////////////////////////////////////////////Tree building sequence////////////////////////////////////////////
//checking if children need to be generated before and after the found input peace, reference to data of tree required
                    //child before found, reference data
                    std::vector<unsigned char> child_beg, child_mid, child_end, child_append;

                    if constexpr (!reverse) {
                        child_beg = std::vector<unsigned char>{};
                        std::copy(std::get<2>(*match_beg)->data.begin(),
                                  std::get<2>(*match_beg)->data.begin() + std::get<0>(*match_beg),
                                  std::back_inserter(child_beg));
                        //child data sequence middle, reference data
                        child_mid = std::vector<unsigned char>{};
                        std::copy(std::get<2>(*match_beg)->data.begin() + std::get<0>(*match_beg),
                                  std::get<2>(*match_beg)->data.begin() + std::get<0>(*match_beg) +
                                  std::get<1>(*match_beg) + 1, std::back_inserter(child_mid));
                        //child data sequence end, reference data
                        child_end = std::vector<unsigned char>{};
                        std::copy(std::get<2>(*match_beg)->data.begin() + std::get<0>(*match_beg) +
                                  std::get<1>(*match_beg) + 1, std::get<2>(*match_beg)->data.end(),
                                  std::back_inserter(child_end));
                    } else {
                        child_beg = std::vector<unsigned char>{};
                        std::copy(std::get<2>(*match_beg)->data.rbegin(),
                                  std::get<2>(*match_beg)->data.rbegin() + std::get<0>(*match_beg),
                                  std::back_inserter(child_beg));
                        //child data sequence middle, reference data
                        child_mid = std::vector<unsigned char>{};
                        std::copy(std::get<2>(*match_beg)->data.rbegin() + std::get<0>(*match_beg),
                                  std::get<2>(*match_beg)->data.rbegin() + std::get<0>(*match_beg) +
                                  std::get<1>(*match_beg) + 1, std::back_inserter(child_mid));
                        //child data sequence end, reference data
                        child_end = std::vector<unsigned char>{};
                        std::copy(std::get<2>(*match_beg)->data.rbegin() + std::get<0>(*match_beg) +
                                  std::get<1>(*match_beg) + 1, std::get<2>(*match_beg)->data.rend(),
                                  std::back_inserter(child_end));
                    }
                    //child after found, reference new input
                    child_append = std::vector<unsigned char>{};
                    std::copy(binary_cont.begin() + std::get<1>(*match_beg) + 1, binary_cont.end(),
                              std::back_inserter(child_append));
                    //only the new append part may be compressed
                    //before splitting or modifying a block it needs to be uncompressed

                    bool first_section_tree = child_beg.size() > 1;
                    bool last_section_tree = !child_end.empty();

                    bool append_tree = !child_append.empty();
                    bool total_match = child_mid.size() == std::get<2>(*match_beg)->data.size();

                    uh::util::compression_custom comp{};
                    auto out_vector = std::vector<tree_radix_custom *>{};

                    //search function already determined that this is the tree that needs to fill in the data or to split somehow
                    //cases: insert front tree, insert cend tree (same case as having a back insert because the cend tree will just have at least 1 element in case of overflow)
                    if (std::get<2>(
                            *match_beg)->data.empty()) {//how to insert, either empty simple insert or some tree construction anywhere
                        //simple insert into data since this seems to be a new node that can contain simple information

                        std::get<2>(*match_beg)->data.assign(binary_cont.begin(), binary_cont.end());
                        out_vector.push_back(std::get<2>(*match_beg));
                        std::get<0>(out_change_tuple) += std::get<1>(*match_beg);
                        std::get<1>(out_change_tuple) += std::get<1>(*match_beg);
                        std::get<2>(out_change_tuple) += comp.compress(binary_cont).size();
                    } else {
                        std::size_t size_integrated{}, size_compressed{}, size_uncompressed{};
                        if (total_match) {//only a maximum of 1 tree creation or just 0 in case of reference
                            //a total match can still have appending structure
                            out_vector.push_back(std::get<2>(*match_beg));
                            size_integrated += std::get<2>(*match_beg)->data.size();
                            if (append_tree) {
                                auto *tree_ptr_tmp = new tree_radix_custom(child_append);
                                if constexpr (!reverse) {
                                    size_compressed = comp.compress((tree_ptr_tmp->data)).size();
                                } else {
                                    auto tmp_vec = *tree_ptr_tmp->data;
                                    std::reverse(tmp_vec.begin(), tmp_vec.end());
                                    size_compressed = comp.compress(tmp_vec).size();
                                }

                                size_uncompressed += tree_ptr_tmp->data.size();
                                size_integrated += size_uncompressed;
                                //either find child is empty and test add tree or add_test to another child tree
                                out_vector.push_back(tree_ptr_tmp);
                                tree_ptr_tmp->block_swarm_offset = std::get<2>(*match_beg)->block_swarm_offset + 1;
                                std::get<2>(*match_beg)->child_put(tree_ptr_tmp, *child_append.begin());
                            }
                            //return implicit 0 with unsigned long
                            std::get<0>(out_change_tuple) += size_integrated;
                            std::get<1>(out_change_tuple) += size_uncompressed;
                            std::get<2>(out_change_tuple) += size_compressed;
                        } else {
                            //first section tree, after split try
                            //data will split into a maximum of 3 parts and by that will add 2 more tree nodes on front and/or back; start with first section
                            tree_radix_custom *tree_ptr_first;
                            tree_radix_custom *tree_ptr_mid;
                            tree_radix_custom *tree_ptr_append;

                            if (first_section_tree) {
                                //children contents need to be copied to middle tree and any references to this node need to be moved to middle tree
                                //new middle tree required
                                tree_ptr_mid = new tree_radix_custom(child_mid);
                                tree_ptr_first = std::get<2>(*match_beg);
                                out_vector.push_back(tree_ptr_first);
                                tree_ptr_mid->block_swarm_offset =
                                        tree_ptr_first->block_swarm_offset + 1;
                                tree_ptr_mid->children = std::get<2>(*match_beg)->children;
                                std::get<2>(*match_beg)->children.clear();
                                tree_ptr_mid->data_ref = std::get<2>(*match_beg)->data_ref;
                                tree_ptr_first->data_ref.clear();
                                //try to add the reference entry to middle tree on first tree
                                tree_ptr_first->child_put(tree_ptr_mid, *child_mid.begin());
                            } else {
                                //the current tree stays fundament
                                tree_ptr_mid = std::get<2>(*match_beg);

                            }
                            out_vector.push_back(tree_ptr_mid);
                            size_integrated += child_mid.size();

                            tree_radix_custom *tree_ptr_last;
                            if (last_section_tree) {
                                //create last tree
                                tree_ptr_last = new tree_radix_custom(child_end);
                                //transfer information of middle tree to last tree and copy the children also to append tree in case it exists
                                out_vector.push_back(tree_ptr_last);
                                tree_ptr_last->block_swarm_offset =
                                        tree_ptr_mid->block_swarm_offset + 1;
                                tree_ptr_last->children = tree_ptr_mid->children;
                                tree_ptr_mid->children.clear();
                                tree_ptr_last->data_ref = tree_ptr_mid->data_ref;
                                tree_ptr_mid->data_ref.clear();

                                //the last tree is the last tree and may append
                                //appending will be added after middle section in case it is available
                                if (append_tree) {
                                    tree_ptr_append = new tree_radix_custom(child_append);
                                    out_vector.push_back(tree_ptr_append);
                                    tree_ptr_append->block_swarm_offset =
                                            tree_ptr_mid->block_swarm_offset + 1;

                                    size_integrated += child_append.size();
                                    size_uncompressed += child_append.size();
                                    size_compressed += comp.compress(child_append).size();
                                    //put this append tree to the middle tree manually
                                    tree_ptr_mid->child_put(tree_ptr_append, *child_append.begin());
                                }
                                //the last tree is still following the middle tree
                                tree_ptr_mid->child_put(tree_ptr_last, *child_end.begin());
                            } else {
                                //the middle tree is the last tree and may append
                                //appending will be added after middle section in case it is available
                                if (append_tree) {
                                    tree_ptr_append = new tree_radix_custom(child_append);
                                    out_vector.push_back(tree_ptr_append);
                                    tree_ptr_append->block_swarm_offset =
                                            tree_ptr_mid->block_swarm_offset + 1;

                                    size_integrated += child_append.size();
                                    size_uncompressed += child_append.size();
                                    size_compressed += comp.compress(child_append).size();
                                    //put this append tree to the middle tree manually
                                    tree_ptr_mid->child_put(tree_ptr_append, *child_append.begin());
                                }
                            }

                            if (first_section_tree) {
                                if (last_section_tree) {
                                    //delete the referenced data size of middle and cend from tree pointer first
                                    if constexpr (!reverse) {
                                        tree_ptr_first->data.erase(tree_ptr_first->data.begin() + child_beg.size(),
                                                                   tree_ptr_first->data.begin() + child_beg.size() +
                                                                   child_mid.size() + child_end.size());
                                    } else {
                                        tree_ptr_first->data.erase(tree_ptr_first->data.begin(),
                                                                   tree_ptr_first->data.begin() + child_mid.size() +
                                                                   child_end.size());
                                    }
                                } else {
                                    //delete middle data reference size from tree pointer first
                                    if constexpr (!reverse) {
                                        tree_ptr_first->data.erase(tree_ptr_first->data.begin() + child_beg.size(),
                                                                   tree_ptr_first->data.begin() + child_beg.size() +
                                                                   child_mid.size());
                                    } else {
                                        tree_ptr_first->data.erase(tree_ptr_first->data.begin(),
                                                                   tree_ptr_first->data.begin() + child_mid.size());
                                    }
                                }
                            } else {
                                if (last_section_tree) {
                                    //delete last tree reference size from tree pointer middle
                                    if constexpr (!reverse) {
                                        tree_ptr_mid->data.erase(tree_ptr_mid->data.begin() + child_mid.size(),
                                                                 tree_ptr_mid->data.begin() + child_mid.size() +
                                                                 child_end.size());
                                    } else {
                                        tree_ptr_mid->data.erase(tree_ptr_mid->data.begin(),
                                                                 tree_ptr_mid->data.begin() + child_end.size());
                                    }
                                }
                                //else do not delete
                            }
                            std::get<0>(out_change_tuple) += size_integrated;
                            std::get<1>(out_change_tuple) += size_uncompressed;
                            std::get<2>(out_change_tuple) += size_compressed;
                        }
                    }
                    ///////////////////////////////////////////////Tree building sequence end////////////////////////////////////////

                    decltype(out_vector.begin()) first_tree_out, middle_tree_out, last_tree_out, append_tree_out;

                    if (first_section_tree) {
                        first_tree_out = out_vector.begin();
                        middle_tree_out = first_tree_out + 1;
                    } else {
                        middle_tree_out = out_vector.begin();
                        //first_tree_out = middle_tree_out;
                    }
                    if (last_section_tree) {
                        last_tree_out = middle_tree_out + 1;
                    } else {
                        last_tree_out = middle_tree_out;
                    }
                    append_tree_out = last_tree_out + 1;//end pointer or valid reference
                    //use sets to distinguish what nodes are added and what are modified; note that write back does not happen yet
                    //find out on what tree the data offset should move to

                    //stack helper function for overlapping creation of trees
                    auto overlap_update = [](std::vector<std::tuple<std::size_t, std::size_t, tree_radix_custom *>> &actively_changing_trees, auto &match_beg_intern,tree_radix_custom *tree_front,
                                             tree_radix_custom *tree_back) {
                        //first check if any of the matches are scaling into the back tree; build two matches and insert
                        auto match_overlap = match_beg_intern;
                        while(match_overlap<actively_changing_trees.end()){
                            std::size_t initial_offset_range_end = std::get<0>(*match_overlap) + std::get<1>(*match_overlap);
                            if(initial_offset_range_end+1 > tree_front->data.size() && std::get<0>(*match_overlap) < tree_front->data.size()){
                                std::size_t first_match_offset = std::get<0>(*match_overlap);
                                std::size_t first_match_size = std::min(tree_front->data.size(), initial_offset_range_end+1);
                                std::size_t second_match_offset = first_match_size + 1;
                                std::size_t second_match_size = std::get<1>(*match_overlap) - first_match_size;

                                std::get<0>(*match_overlap) = first_match_offset;
                                std::get<1>(*match_overlap) = first_match_size;

                                std::size_t reinvoke_offset = std::distance(actively_changing_trees.begin(),match_overlap);
                                actively_changing_trees.insert(match_overlap+1,std::make_tuple(second_match_offset, second_match_size,tree_back));
                                match_overlap = actively_changing_trees.begin()+reinvoke_offset+1;
                            }
                            match_overlap++;
                        }

                        //from current match begin start update all entries of matches until the end
                        auto match_shifter = match_beg_intern;
                        while(match_shifter<actively_changing_trees.end()){
                            if(std::get<0>(*match_shifter)>=tree_front->data.size()){
                                std::get<0>(*match_shifter) -= tree_front->data.size();
                                std::get<2>(*match_shifter) = tree_back;
                            }
                            match_shifter++;
                        }
                    };
                    if (total_match) {
                        if (append_tree) {//DO NOT "DEBUG"
                            added.emplace(*append_tree_out);
                        }
                        //this must be the only match so nothing happens
                    } else {
                        if (first_section_tree) {
                            modified.emplace(*first_tree_out);
                            added.emplace(*middle_tree_out);
                            //tree_match_pointer must move from first tree to middle tree, and we adjust offsets of all other matches
                            std::size_t reinvoke_count = std::distance(actively_changing_trees.begin(),match_beg);
                            overlap_update(actively_changing_trees, match_beg, *first_tree_out,*middle_tree_out);//update current tree pointer
                            match_beg = actively_changing_trees.begin()+reinvoke_count;
                        } else {
                            modified.emplace(*middle_tree_out);
                        }
                        //only tree_match_pointer changes and offset changes to last tree if the end of the match exceeds limits and last tree exists
                        if (last_section_tree) {
                            added.emplace(*last_tree_out);//section of inner list must be over due to incomplete match
                            //tree_match_pointer must move from middle tree to last tree, and we adjust offsets of all other matches
                            std::size_t reinvoke_count = std::distance(actively_changing_trees.begin(),match_beg);
                            overlap_update(actively_changing_trees, match_beg,*middle_tree_out,*last_tree_out);//update current tree pointer
                            match_beg = actively_changing_trees.begin()+reinvoke_count;
                        }
                        if (append_tree)added.emplace(*append_tree_out);
                    }
                    auto added_contains_it = [&added](auto &item) {
                        return added.contains(item);
                    };
                    std::erase_if(modified, added_contains_it);

                    std::get<3>(out_change_tuple).emplace_back(modified, added);
                    modified.clear();
                    added.clear();
                    match_beg++;
                }
                if (std::get<0>(out_change_tuple) > 0)
                    out_change_tuple_out.push_back(
                            out_change_tuple);//only add change list if there was anything integrated
                tree_node_results++;
            }


            //deduplicate change list out

            return out_change_tuple_out;
        }

        template<class ContainerString, bool reverse = false,
                std::enable_if_t<std::is_same<std::string, ContainerString>::value, bool> = true>
        auto
        add_test(ContainerString &cont_string) {
            std::vector<unsigned char> input{};
            std::copy(cont_string.begin(), cont_string.end(),std::back_inserter(input));
            return add_test(input);
        }

        //returns total size integrated, new space used uncompressed, new space used compressed
        //calculates exact size for a single integration of data, this could be communicated to agency to determine optimal storage location
        template<class ContainerBinary, bool reverse = false,
                std::enable_if_t<!std::is_same<std::string, ContainerBinary>::value, bool> = true>
        std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>
        add_test(ContainerBinary &cont_binary) {//add test should copy
            //first search existing structure and add into the last tree to insert potentially missing information

            //uncompressed input
            if (cont_binary.empty()) {
                return {};
            }
            std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t>>,std::size_t, std::size_t>> search_index = search<ContainerBinary, reverse>(cont_binary);//std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t>,std::size_t, std::size_t>>>

            //cases for search index: its empty or it has content and with that a last tree element
            //cases for last tree if it exists, binary fit in: match from the beginning on, match in the middle, match until the end, total match
            //all lists contain lists with a last element that had multiple matches; add up all matches
            uh::util::compression_custom comp{};
            std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>> add_tup_out{};
            if (search_index.empty()) {
                std::tuple<std::size_t, std::size_t, std::size_t, std::size_t> add_tup{};
                std::get<0>(add_tup) += cont_binary.size();
                std::get<2>(add_tup) += cont_binary.size();
                std::get<3>(add_tup) += comp.compress(cont_binary).size();
                add_tup_out.push_back(add_tup);
                return add_tup_out;
            }

            for (auto &single_route: search_index) {
                std::tuple<std::size_t, std::size_t, std::size_t, std::size_t> add_tup{};
                for (auto &pos_tup: std::get<1>(single_route)) {
                    std::get<0>(add_tup) += std::get<1>(pos_tup) + 1;
                }
                if (cont_binary.begin() + std::get<0>(add_tup) < cont_binary.end()) {
                    auto set_vector = std::vector<unsigned char>{};
                    std::copy(cont_binary.begin() + std::get<0>(add_tup), cont_binary.end(),
                              std::back_inserter(set_vector));
                    std::get<3>(add_tup) += comp.compress(set_vector).size();
                    std::get<2>(add_tup) += set_vector.size();
                    std::get<0>(add_tup) += set_vector.size();
                }
                long extra_savings = std::get<0>(add_tup);
                extra_savings -= cont_binary.size();
                extra_savings = std::max(extra_savings, (long) 0);
                std::get<1>(add_tup) = (std::size_t) extra_savings;
                std::get<0>(add_tup) -= std::get<1>(add_tup);
                add_tup_out.push_back(add_tup);
            }

            return add_tup_out;
        }

        //returns std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<decltype(bin_beg), decltype(bin_end), decltype(bin_beg)>>>>>, std::size_t>>
        template<class ContainerData, class ContainerBinary, bool reverse = false>
        auto
        search_match_filter(ContainerData &data_cont,ContainerBinary &binary_cont,
                            std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t>>> possibilities =
                            std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t>>>{}) {

            auto local_matches = compare_ultihash<ContainerData,ContainerBinary,reverse>(data_cont, binary_cont);//std::vector<std::tuple<std::size_t, std::size_t, tree_radix_custom *>>

            auto match_duplicate = local_matches.begin();
            while(match_duplicate!=local_matches.end()){
                if(std::count(std::get<1>(possibilities).begin(),std::get<1>(possibilities).end(),*match_duplicate)){
                    std::size_t reinvoke_count = std::distance(local_matches.begin(),match_duplicate);
                    local_matches.erase(match_duplicate);
                    match_duplicate = local_matches.begin()+reinvoke_count;
                    continue;
                }
                match_duplicate++;
            }

            if (local_matches.empty() && std::get<1>(possibilities).empty())return possibilities;

            std::get<1>(possibilities).insert(std::get<1>(possibilities).end(),local_matches.begin(),local_matches.end());

            std::sort(local_matches.begin(), local_matches.end(), [](auto &a, auto &b) {
                return std::get<1>(a) < std::get<1>(b);
            });
            //LEGAL MATCH FILTER
            auto legal_check = [&data_cont](auto &match_beg, std::size_t start_val,
                                                            std::size_t end_val) {
                //on empty or partial match make new list in list, else append the match results on total match
                bool legal_split;
                //if the end was matched too long we can do something about that, but else the algorithm is prefix oriented
                bool start_size, end_size, begin_reached, end_reached, total_found_size;
                do {

                    start_size = std::get<0>(*match_beg) <= (long) start_val - MINIMUM_MATCH_SIZE ||
                                 start_val + MINIMUM_MATCH_SIZE <= std::get<0>(*match_beg);
                    end_size = data_cont.size() - (std::get<0>(*match_beg) + std::get<1>(*match_beg)) <=
                               (long) end_val - MINIMUM_MATCH_SIZE <= (long) start_val - MINIMUM_MATCH_SIZE ||
                               MINIMUM_MATCH_SIZE <=
                               data_cont.size() - (std::get<0>(*match_beg) + std::get<1>(*match_beg));
                    total_found_size = MINIMUM_MATCH_SIZE <= std::get<1>(*match_beg);
                    begin_reached = std::get<0>(*match_beg) == start_val;
                    end_reached = data_cont.begin() + (std::get<0>(*match_beg) + std::get<1>(*match_beg)) + 1 ==
                                  data_cont.begin() + end_val + 1;
                    legal_split = ((start_size || begin_reached) && (end_size || end_reached) && total_found_size);

                    if (!start_size && !begin_reached) {
                        std::get<0>(*match_beg)++;//relative offset changes
                        std::get<1>(*match_beg)--;
                    }
                    if (!end_size && !end_reached) {
                        std::get<1>(*match_beg)--;
                    }
                    if (std::get<1>(*match_beg) < MINIMUM_MATCH_SIZE) {
                        return false;
                    }
                } while (((!end_size && !end_reached) || (!start_size && !begin_reached)));

                return legal_split;
            };

            auto match_beg = std::get<1>(possibilities).begin();

            while (match_beg !=
                    std::get<1>(possibilities).end()) {//shrink all matches until the total result is legal, shrink smaller matches first until they vanish
                bool legal_general = legal_check(match_beg, (std::size_t) 0,
                                                 std::max(data_cont.size() - 1, (std::size_t) 0));
                if (!legal_general) {
                    std::get<1>(possibilities).erase(match_beg);
                    match_beg = std::get<1>(possibilities).begin();
                    continue;
                }
                auto match_begin_legal_shift = std::get<1>(possibilities).begin();
                while (match_begin_legal_shift != std::get<1>(possibilities).end()) {
                    bool legal = legal_check(match_begin_legal_shift, std::get<0>(*match_beg),
                                             std::get<0>(*match_beg) + std::get<1>(*match_beg));
                    if (!legal) {
                        std::get<1>(possibilities).erase(match_begin_legal_shift);
                        match_begin_legal_shift = std::get<1>(possibilities).begin();
                    } else match_begin_legal_shift++;
                }
                match_beg++;
            }

            //sort the smallest offset out of the largest match results in case the match sizes are equal*/
            std::sort(std::get<1>(possibilities).begin(), std::get<1>(possibilities).end(), [](auto &a, auto &b) {
                return std::get<0>(a) < std::get<0>(b);
            });
            //all matches are valid as long as they do not collide

            return possibilities;
        }


        //returns the path of maximum fit and the match size
        template<class ContainerBinary, bool reverse = false>
        std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t>>,std::size_t, std::size_t>>
        search(ContainerBinary &cont_binary) {

            if (cont_binary.empty()) {
                return std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t>>,std::size_t, std::size_t>>{};
            }

            std::vector<std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t>>,std::size_t, std::size_t>>>possibilities_out{};

            //a list holding possibilities of paths of matches
            //                              tree of results                     matches with offset and size       num matches  binary advance
            std::list<std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t>>,std::size_t, std::size_t>>>possibilities_work{};
            std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t>>,std::size_t, std::size_t>> start_node{};
            start_node.emplace_back(this,std::vector<std::tuple<std::size_t, std::size_t>>{},0,0);
            possibilities_work.push_back(start_node);

            std::set<std::size_t> advancements{0};

            auto poss_beg = possibilities_work.begin();
            while(possibilities_work.end()){
                std::set<std::size_t> advancements_already_checked{};
                //read first entry, search with permutation of advancement and append the results to the end of work list

                std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t>>,std::size_t, std::size_t>> current_path = *poss_beg;

                //search within last node of that path
                auto search_within = current_path.end();//std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t>>,std::size_t, std::size_t>
                std::advance(search_within,-1);

                //TODO: search data within with all possible advancements

                if(std::get<0>(*search_within)->children.empty()){
                    //this path reached an end, store
                    possibilities_out.push_back(current_path);
                }
                poss_begin++;
                possibilities_work.pop_front();
            }

            if (possibilities_work.empty()) {
                return std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t>>,std::size_t, std::size_t>>{};
            }

            std::sort(possibilities_out.begin(),possibilities_out.end(),[](auto &a, auto &b){//sort for biggest match combination
                std::get<3>(a) > std::get<3>(b);
            });

            auto poss_out = possibilities_out.begin();
            std::size_t max_advance = std::get<3>(*poss_out);
            while(poss_out != possibilities_out.end()){
                if(std::get<3>(*poss_out)<max_advance){
                    possibilities_out.erase(poss_out,possibilities_out.end());
                    break;
                }
                poss_out++;
            }

            std::sort(possibilities_out.begin(),possibilities_out.end(),[](auto &a, auto &b){//sort for bigger matches, less fragments
                std::get<2>(a) > std::get<2>(b);
            });

            return possibilities_out[0];
        }
    };
}

#endif //UHLIBCOMMON_TREE_RADIX_CUSTOM_H
