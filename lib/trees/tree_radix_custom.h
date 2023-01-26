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

    template<class DataReference>
    struct tree_radix_custom {
        std::vector<unsigned char> data{};
        //any binary vector string
        DataReference data_ref{};
        //the first element of data is cut off to children except on root if it's a new tree
        //TODO: use two children and block_swarm_offsets to scan forward and backward
        std::vector<std::tuple<std::vector<tree_radix_custom *>, unsigned char>> children{};//multiple targets that can follow a node for each letter
        std::size_t block_swarm_offset{};//offset of block beginning from root

        tree_radix_custom() = default;

        ~tree_radix_custom() {
            for (auto &item1: children) {
                for (auto &item2: std::get<0>(item1)) {
                    delete item2;
                }
            }
        }

        explicit tree_radix_custom(auto &bin) {
            add(bin.begin(), bin.end());
        }

        tree_radix_custom(auto beg, auto end) {
            add(beg, end);
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
                                return lexicographical_compare(std::execution::unseq, a->data.cbegin(), a->data.cend(),
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

        bool child_delete(tree_radix_custom *input_tree, unsigned char first_letter) {
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
        template<typename Const_iterator_data, typename Const_iterator_binary,
                std::enable_if_t<
                        (((std::is_same<std::vector<unsigned char>::const_iterator, Const_iterator_data>::value ||
                           std::is_same<std::list<unsigned char>::const_iterator, Const_iterator_data>::value ||
                           std::is_same<std::deque<unsigned char>::const_iterator, Const_iterator_data>::value) ||
                          (std::is_same<std::vector<unsigned char>::const_reverse_iterator, Const_iterator_data>::value ||
                           std::is_same<std::list<unsigned char>::const_reverse_iterator, Const_iterator_data>::value ||
                           std::is_same<std::deque<unsigned char>::const_reverse_iterator, Const_iterator_data>::value))) &&
                        (((std::is_same<std::vector<unsigned char>::const_iterator, Const_iterator_binary>::value ||
                           std::is_same<std::list<unsigned char>::const_iterator, Const_iterator_binary>::value ||
                           std::is_same<std::deque<unsigned char>::const_iterator, Const_iterator_binary>::value) ||
                          (std::is_same<std::vector<unsigned char>::const_reverse_iterator, Const_iterator_binary>::value ||
                           std::is_same<std::list<unsigned char>::const_reverse_iterator, Const_iterator_binary>::value ||
                           std::is_same<std::deque<unsigned char>::const_reverse_iterator, Const_iterator_binary>::value))), bool> = true
        >
        auto
        compare_ultihash(Const_iterator_data data_beg, Const_iterator_data data_end, Const_iterator_binary input_beg,
                         Const_iterator_binary input_end) {
            //if input does only fit to a shorter string as a subset of data, count becomes negative, else positive including ß
            //data offset iterator and start and end of input
            std::vector<std::tuple<decltype(data_beg), decltype(input_end), decltype(input_beg)>> matches{};
            if (data_beg == data_end || input_beg == input_end)return matches;
            //search forward through data
            do {
                //first element match
                std::unique_lock lock(simd_protect);
                if (simd_count < SIMD_UNITS) {
                    simd_count += 1;
                    lock.unlock();
                    data_beg = std::find(std::execution::unseq, data_beg, data_end, *input_beg);
                    lock.lock();
                    simd_count -= 1;
                    lock.unlock();
                } else {
                    lock.unlock();
                    data_beg = std::find(data_beg, data_end, *input_beg);
                }

                //search how long input matches
                std::pair<decltype(data_beg), decltype(data_end)> found;
                lock.lock();
                if (simd_count < SIMD_UNITS) {
                    simd_count += 1;
                    lock.unlock();
                    found = std::mismatch(std::execution::unseq, data_beg, data_end, input_beg, input_end);
                    lock.lock();
                    simd_count -= 1;
                    lock.unlock();
                } else {
                    lock.unlock();
                    found = std::mismatch(data_beg, data_end, input_beg, input_end);
                }
                //last input count reversed
                std::size_t found_dist = std::distance(data_beg, found.first);
                if (found_dist >= MINIMUM_MATCH_SIZE) {
                    matches.emplace_back(data_beg, input_beg, input_beg + found_dist);
                }
                if (data_beg != data_end)data_beg++;
            } while (data_beg < data_end);

            return matches;
        }

    public:
        template<class ContainerType, bool reverse = false>
        std::tuple<std::size_t, std::size_t, std::size_t> add(const ContainerType &c) {
            if constexpr (!reverse) {
                return add(c.cbegin(), c.cend());
            } else {
                return add(c.crbegin(), c.crend());
            }
        }

        //returns total size integrated, new space used uncompressed, new space used compressed, list of tree references of <offset_ELEMENT,modified_LIST,added_LIST> tree nodes
        template<typename Const_iterator,
                std::enable_if_t<((std::is_same<std::vector<unsigned char>::const_iterator, Const_iterator>::value ||
                                   std::is_same<std::list<unsigned char>::const_iterator, Const_iterator>::value ||
                                   std::is_same<std::deque<unsigned char>::const_iterator, Const_iterator>::value) ||
                                  (std::is_same<std::vector<unsigned char>::const_reverse_iterator, Const_iterator>::value ||
                                   std::is_same<std::list<unsigned char>::const_reverse_iterator, Const_iterator>::value ||
                                   std::is_same<std::deque<unsigned char>::const_reverse_iterator, Const_iterator>::value)), bool> = true
        >
        std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::list<std::tuple<std::set<tree_radix_custom *>, std::set<tree_radix_custom *>>>>>
        add(Const_iterator bin_beg, Const_iterator bin_end) {//TODO:check duplicate matches and eliminate
            //first search existing structure and add into the last tree to insert potentially missing information
            /*std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::vector<unsigned char>::const_iterator, std::vector<unsigned char>::const_iterator, std::vector<unsigned char>::const_iterator>>>>>, std::size_t>>*/

            //uncompressed input
            if (bin_beg >= bin_end) {
                return {};
            }
            auto search_index = search(bin_beg, bin_end);
            constexpr bool reverse = (
                    std::is_same<std::vector<unsigned char>::const_reverse_iterator, decltype(bin_beg)>::value ||
                    std::is_same<std::list<unsigned char>::const_reverse_iterator, decltype(bin_beg)>::value ||
                    std::is_same<std::deque<unsigned char>::const_reverse_iterator, decltype(bin_beg)>::value);

            //some element and an end element at least required
            //TODO: add cross update from forward and backward children
            auto tree_building_sequence = [&reverse](tree_radix_custom *cur_tree,
                                             auto bin_beg_incoming, auto bin_end_incoming, auto data_beg_intern,
                                             auto bin_beg_found, auto bin_end_found) {
                //checking if children need to be generated before and after the found input peace, reference to data of tree required
                //child before found, reference data
                decltype(bin_beg_found) child_beg_beg, child_beg_mid, child_beg_end;
                decltype(bin_end_found) child_end_beg, child_end_mid, child_end_end;

                if constexpr (!reverse) {
                    child_beg_beg = cur_tree->data.cbegin();
                    child_end_beg = std::max(data_beg_intern, child_beg_beg);
                    //child data sequence middle, reference data
                    child_beg_mid = data_beg_intern;
                    child_end_mid = std::min(cur_tree->data.cend(),
                                             child_beg_mid + std::distance(bin_beg_found, bin_end_found));
                    //child data sequence end, reference data
                    child_beg_end = std::min(cur_tree->data.cend(), child_end_mid + 1);
                    child_end_end = cur_tree->data.cend();
                } else {
                    child_beg_beg = cur_tree->data.crbegin();
                    child_end_beg = std::max(data_beg_intern, child_beg_beg);
                    //child data sequence middle, reference data
                    child_beg_mid = data_beg_intern;
                    child_end_mid = std::min(cur_tree->data.crend(),
                                             child_beg_mid + std::distance(bin_beg_found, bin_end_found));
                    //child data sequence end, reference data
                    child_beg_end = std::min(cur_tree->data.crend(), child_end_mid + 1);
                    child_end_end = cur_tree->data.crend();
                }
                //child after found, reference new input
                decltype(bin_beg_found) child_beg_append = std::min(bin_end_found + 1, bin_end_incoming);
                decltype(bin_end_found) child_end_append = bin_end_incoming;
                //only the new append part may be compressed
                //before splitting or modifying a block it needs to be uncompressed

                bool first_section_tree = std::distance(child_beg_beg, child_end_beg) > 1;
                bool last_section_tree = child_beg_end != child_end_end;

                bool append_tree =
                        std::distance(child_beg_append, child_end_append) > 0 && child_beg_append != bin_end_incoming;
                bool total_match = std::distance(child_beg_mid, child_end_mid) == cur_tree->data.size();

                uh::util::compression_custom comp{};
                auto out_list = std::vector<tree_radix_custom *>{};

                if(bin_beg_incoming >= bin_end_incoming){
                    return std::make_tuple((std::size_t)0,(std::size_t)0,(std::size_t)0,out_list,first_section_tree,
                                           last_section_tree, append_tree, total_match);
                }

                //search function already determined that this is the tree that needs to fill in the data or to split somehow
                //cases: insert front tree, insert cend tree (same case as having a back insert because the cend tree will just have at least 1 element in case of overflow)
                if (cur_tree->data.empty()) {//how to insert, either empty simple insert or some tree construction anywhere
                    //simple insert into data since this seems to be a new node that can contain simple information

                    auto set_vector = std::vector<unsigned char>{bin_beg_found, bin_end_found};

                    if constexpr (!reverse) {
                        cur_tree->data = set_vector;
                    } else {
                        std::reverse(set_vector.begin(), set_vector.end());
                        cur_tree->data = set_vector;
                    }
                    out_list.push_back(cur_tree);
                    return std::make_tuple((std::size_t) std::distance(bin_beg_found, bin_end_found),
                                           (std::size_t) std::distance(bin_beg_found, bin_end_found),
                                           (std::size_t) comp.compress(bin_beg_found, bin_end_found).size(), out_list,
                                           first_section_tree, last_section_tree, append_tree, total_match);
                } else {
                    std::size_t size_integrated{}, size_compressed{}, size_uncompressed{};
                    if (total_match) {//only a maximum of 1 tree creation or just 0 in case of reference
                        //a total match can still have appending structure
                        out_list.push_back(cur_tree);
                        size_integrated += cur_tree->data.size();
                        if (append_tree) {
                            auto *tree_ptr_tmp = new tree_radix_custom(child_beg_append, child_end_append);
                            size_compressed = comp.compress(tree_ptr_tmp->data.cbegin(),
                                                            tree_ptr_tmp->data.cend()).size();
                            size_uncompressed += tree_ptr_tmp->data.size();
                            size_integrated += size_uncompressed;
                            //either find child is empty and test add tree or add_test to another child tree
                            out_list.push_back(tree_ptr_tmp);
                            tree_ptr_tmp->block_swarm_offset = cur_tree->block_swarm_offset + cur_tree->data.size();
                            cur_tree->child_put(tree_ptr_tmp, *child_beg_append);
                        }
                        //return implicit 0 with unsigned long
                        return std::make_tuple(
                                (decltype(cur_tree->data.size())) size_integrated,
                                (decltype(cur_tree->data.size())) size_uncompressed,
                                (decltype(cur_tree->data.size())) size_compressed, out_list, first_section_tree,
                                last_section_tree, append_tree, total_match);//nothing to add, only reference
                    } else {
                        //first section tree, after split try
                        //data will split into a maximum of 3 parts and by that will add 2 more tree nodes on front and/or back; start with first section
                        tree_radix_custom *tree_ptr_first;

                        tree_radix_custom *tree_ptr_mid;
                        tree_radix_custom *tree_ptr_append;

                        if (first_section_tree) {
                            //children contents need to be copied to middle tree and any references to this node need to be moved to middle tree
                            //new middle tree required
                            tree_ptr_mid = new tree_radix_custom(child_beg_mid, child_end_mid);
                            tree_ptr_first = cur_tree;
                            out_list.push_back(tree_ptr_first);
                            tree_ptr_mid->block_swarm_offset =
                                    tree_ptr_first->block_swarm_offset + tree_ptr_first->data.size();
                            tree_ptr_mid->children = cur_tree->children;
                            cur_tree->children.clear();
                            tree_ptr_mid->data_ref = cur_tree->data_ref;
                            tree_ptr_first->data_ref.clear();

                            size_integrated += std::distance(child_beg_beg, child_end_beg);
                            //try to add the reference entry to middle tree on first tree
                            tree_ptr_first->child_put(tree_ptr_mid, *child_beg_mid);
                        } else {
                            //the current tree stays fundament
                            tree_ptr_mid = cur_tree;
                            size_integrated += std::distance(child_beg_mid, child_end_mid);
                        }
                        out_list.push_back(tree_ptr_mid);

                        tree_radix_custom *tree_ptr_last;
                        if (last_section_tree) {
                            //create last tree
                            tree_ptr_last = new tree_radix_custom(child_beg_end, child_end_end);
                            //transfer information of middle tree to last tree and copy the children also to append tree in case it exists
                            out_list.push_back(tree_ptr_last);
                            tree_ptr_last->block_swarm_offset =
                                    tree_ptr_mid->block_swarm_offset + tree_ptr_mid->data.size();
                            tree_ptr_last->children = tree_ptr_mid->children;
                            tree_ptr_mid->children.clear();
                            tree_ptr_last->data_ref = tree_ptr_mid->data_ref;
                            tree_ptr_mid->data_ref.clear();

                            size_integrated += std::distance(child_beg_end, child_end_end);
                            //the last tree is the last tree and may append
                            //appending will be added after middle section in case it is available
                            if (append_tree) {
                                tree_ptr_append = new tree_radix_custom(child_beg_append, child_end_append);
                                out_list.push_back(tree_ptr_append);
                                tree_ptr_append->block_swarm_offset =
                                        tree_ptr_mid->block_swarm_offset + tree_ptr_mid->data.size();

                                size_integrated += std::distance(child_beg_append, child_end_append);
                                size_uncompressed += std::distance(child_beg_append, child_end_append);
                                size_compressed += comp.compress(child_beg_append, child_end_append).size();
                                //put this append tree to the middle tree manually
                                tree_ptr_mid->child_put(tree_ptr_append, *child_beg_append);
                            }
                            //the last tree is still following the middle tree
                            tree_ptr_mid->child_put(tree_ptr_last, *child_beg_end);
                        } else {
                            //the middle tree is the last tree and may append
                            //appending will be added after middle section in case it is available
                            if (append_tree) {
                                tree_ptr_append = new tree_radix_custom(child_beg_append, child_end_append);
                                out_list.push_back(tree_ptr_append);
                                tree_ptr_append->block_swarm_offset =
                                        tree_ptr_mid->block_swarm_offset + tree_ptr_mid->data.size();

                                size_integrated += std::distance(child_beg_append, child_end_append);
                                size_uncompressed += std::distance(child_beg_append, child_end_append);
                                size_compressed += comp.compress(child_beg_append, child_end_append).size();
                                //put this append tree to the middle tree manually
                                tree_ptr_mid->child_put(tree_ptr_append, *child_beg_append);
                            }
                        }

                        if (first_section_tree) {
                            if (last_section_tree) {
                                //delete the referenced data size of middle and cend from tree pointer first
                                if constexpr (!reverse) {
                                    auto del_beg =
                                            tree_ptr_first->data.cbegin() + std::distance(child_beg_beg, child_end_beg);
                                    auto del_end = del_beg + std::distance(child_beg_mid, child_end_mid) +
                                                   std::distance(child_beg_end, child_end_end);
                                    tree_ptr_first->data.erase(del_beg, del_end);
                                } else {
                                    auto del_beg = tree_ptr_first->data.crbegin() +
                                                   std::distance(child_beg_beg, child_end_beg);
                                    auto del_end = del_beg + std::distance(child_beg_mid, child_end_mid) +
                                                   std::distance(child_beg_end, child_end_end);
                                    tree_ptr_first->data.erase(del_beg, del_end);
                                }
                            } else {
                                //delete middle data reference size from tree pointer first
                                if constexpr (!reverse) {
                                    auto del_beg =
                                            tree_ptr_first->data.cbegin() + std::distance(child_beg_beg, child_end_beg);
                                    auto del_end = del_beg + std::distance(child_beg_mid, child_end_mid);
                                    tree_ptr_first->data.erase(del_beg, del_end);
                                } else {
                                    auto del_beg = tree_ptr_first->data.crbegin() +
                                                   std::distance(child_beg_beg, child_end_beg);
                                    auto del_end = del_beg + std::distance(child_beg_mid, child_end_mid);
                                    tree_ptr_first->data.erase(del_beg, del_end);
                                }
                            }
                        } else {
                            if (last_section_tree) {
                                //delete last tree reference size from tree pointer middle
                                if constexpr (!reverse) {
                                    auto del_beg =
                                            tree_ptr_mid->data.cbegin() + std::distance(child_beg_mid, child_end_mid);
                                    auto del_end = del_beg + std::distance(child_beg_end, child_end_end);
                                    tree_ptr_mid->data.erase(del_beg, del_end);
                                } else {
                                    auto del_beg =
                                            tree_ptr_mid->data.crbegin() + std::distance(child_beg_mid, child_end_mid);
                                    auto del_end = del_beg + std::distance(child_beg_end, child_end_end);
                                    tree_ptr_mid->data.erase(del_beg, del_end);
                                }
                            }
                            //else do not delete
                        }

                        return std::make_tuple(
                                (decltype(cur_tree->data.size())) size_integrated,
                                (decltype(cur_tree->data.size())) size_uncompressed,
                                (decltype(cur_tree->data.size())) size_compressed, out_list, first_section_tree,
                                last_section_tree, append_tree, total_match);
                    }
                }
            };

            std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::list<std::tuple<std::set<tree_radix_custom *>, std::set<tree_radix_custom *>>>>> out_change_tuple_out{};
            //cases for search index: its empty or it has content and with that a last tree element
            if (search_index.empty()) {
                std::tuple<std::size_t, std::size_t, std::size_t, std::list<std::tuple<std::set<tree_radix_custom *>, std::set<tree_radix_custom *>>>> out_change_tuple{};
                if(!reverse){
                    auto out_size = tree_building_sequence(this, bin_beg, bin_end, data.cbegin(), bin_beg, bin_end);
                    std::get<0>(out_change_tuple) += std::get<0>(out_size);
                    std::get<1>(out_change_tuple) += std::get<1>(out_size);
                    std::get<2>(out_change_tuple) += std::get<2>(out_size);
                    auto out_vector = std::get<3>(out_size);
                    std::get<3>(out_change_tuple).emplace_back(std::set<tree_radix_custom *>{},
                                                               std::set<tree_radix_custom *>{
                                                                       out_vector[0]});//only add a new tree
                    out_change_tuple_out.push_back(out_change_tuple);
                }
                else{
                    auto out_size = tree_building_sequence(this, bin_beg, bin_end, data.crbegin(), bin_beg, bin_end);
                    std::get<0>(out_change_tuple) += std::get<0>(out_size);
                    std::get<1>(out_change_tuple) += std::get<1>(out_size);
                    std::get<2>(out_change_tuple) += std::get<2>(out_size);
                    auto out_vector = std::get<3>(out_size);
                    std::get<3>(out_change_tuple).emplace_back(std::set<tree_radix_custom *>{},
                                                               std::set<tree_radix_custom *>{
                                                                       out_vector[0]});//only add a new tree
                    out_change_tuple_out.push_back(out_change_tuple);
                }
            }
            //cases for last tree if it exists, binary fit in: match from the beginning on, match in the middle, match until the end, total match
            //all lists contain lists with a last element that had multiple matches; add up all matches

            auto single_beg = search_index.begin();
            while (single_beg != search_index.end()) {
                std::tuple<std::size_t, std::size_t, std::size_t, std::list<std::tuple<std::set<tree_radix_custom *>, std::set<tree_radix_custom *>>>> out_change_tuple{};

                auto search_element = std::get<0>(
                        *single_beg).begin();//std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::vector<unsigned char>::const_iterator, std::vector<unsigned char>::const_iterator, std::vector<unsigned char>::const_iterator>>>>, std::size_t>

                while (search_element != std::get<0>(
                        *single_beg).end()) {//std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::vector<unsigned char>::const_iterator, std::vector<unsigned char>::const_iterator, std::vector<unsigned char>::const_iterator>>>
                    //master list element contains a list containing tree pointers with vectors that totally matched, last element did not completely match anymore; inner list carries at least one element
                    auto one_node_analysis = (*search_element).end();//first sublist
                    std::advance(one_node_analysis, -1);//last element did not match completely
                    //the inner list carries elements with a tuple holding the tree pointer and a list of valid matches that should be transformed into a sequence of trees
                    //we need to crawl the data of each tree and advance that many new trees until we would match the coming data offset of the original tree

                    if (std::get<1>(*one_node_analysis).empty()) {
                        //delete the containing list, we are done here
                        (*search_element).erase(one_node_analysis);
                        search_element = std::get<0>(*single_beg).begin();
                        continue;
                    }

                    tree_radix_custom *tree_match_pointer = std::get<0>(*one_node_analysis);
                    std::set<tree_radix_custom *> modified{}, added{};//changes to be written to disk in the form of tree pointers
                    std::vector<std::tuple<decltype(bin_beg), decltype(bin_end), decltype(bin_beg), decltype(tree_match_pointer)>> actively_changing_trees{};
                    std::for_each(std::get<1>(*one_node_analysis).begin(), std::get<1>(*one_node_analysis).end(),
                                  [&actively_changing_trees, &tree_match_pointer](auto &item) {
                                      actively_changing_trees.emplace_back(std::get<0>(item), std::get<1>(item),
                                                                           std::get<2>(item), tree_match_pointer);
                                  });

                    auto match_beg = actively_changing_trees.begin();//                                         found beginning                             found end                        data on tree begin at found begin
                    auto match_beg_copy = match_beg;
                    while (match_beg !=
                           actively_changing_trees.end()) {//update loop on std::vector<std::tuple<std::vector<unsigned char>::const_iterator, std::vector<unsigned char>::const_iterator, std::vector<unsigned char>::const_iterator>>
                        auto out_size = tree_building_sequence(std::get<3>(*match_beg), bin_beg, bin_end,
                                                               std::get<0>(*match_beg), std::get<1>(*match_beg),
                                                               std::get<2>(*match_beg));
                        std::get<0>(out_change_tuple) += std::get<0>(out_size);
                        std::get<1>(out_change_tuple) += std::get<1>(out_size);
                        std::get<2>(out_change_tuple) += std::get<2>(out_size);
                        auto out_vector = std::get<3>(out_size);
                        bool first_section_tree = std::get<4>(out_size);
                        bool last_section_tree = std::get<5>(out_size);
                        bool append_tree = std::get<6>(out_size);
                        bool total_match = std::get<7>(out_size);
                        std::size_t tree_front_data_front_absolute;
                        if constexpr (!reverse){
                            tree_front_data_front_absolute=std::distance(std::get<3>(*match_beg)->data.cbegin(), std::get<0>(*match_beg)) - 1;
                        }
                        else{
                            tree_front_data_front_absolute=std::distance(std::get<3>(*match_beg)->data.crbegin(), std::get<0>(*match_beg)) - 1;
                        }

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
                        if (total_match) {
                            if (append_tree) {//DO NOT "DEBUG"
                                added.emplace(*append_tree_out);
                            }
                            //this must be the only match so nothing happens
                        } else {
                            //stack helper function for overlapping creation of trees
                            auto overlap_update = [&reverse](std::size_t tree_front_data_front_absolute,
                                                             auto &actively_changing_trees, auto match_beg_intern_copy,
                                                             auto match_end_intern, tree_radix_custom *tree_front,
                                                             tree_radix_custom *tree_back) {
                                auto match_beg_intern = match_beg_intern_copy;
                                if (match_beg_intern + 1 >= match_end_intern)return;
                                //update matches offset
                                auto first_data_offset = std::get<0>(*match_beg_intern);
                                std::size_t first_match_size =
                                        std::distance(std::get<1>(*match_beg_intern), std::get<2>(*match_beg_intern)) +
                                        1;
                                std::size_t first_end = tree_front_data_front_absolute + first_match_size;
                                while (match_beg_intern < match_end_intern) {
                                    std::size_t data_offset_between_start_and_current = std::distance(first_data_offset,
                                                                                                      std::get<0>(
                                                                                                              *(match_beg_intern +
                                                                                                                1)));
                                    std::size_t total_offset_front_for_this_node =
                                            tree_front_data_front_absolute + data_offset_between_start_and_current;
                                    if (data_offset_between_start_and_current < first_end) {
                                        //overlapping match, tree pointer stays at this tree, one to one reference tree front at tree_front_data_front_absolute; re-reference to data due to erase
                                        std::size_t match_size = std::distance(std::get<1>(*(match_beg_intern + 1)),
                                                                               std::get<2>(*(match_beg_intern + 1))) +
                                                                 1;
                                        std::size_t data_offset_between_start_and_last_node = std::distance(
                                                first_data_offset, std::get<0>(*(match_beg_intern)));
                                        std::size_t total_offset_front_for_last_node = tree_front_data_front_absolute +
                                                                                       data_offset_between_start_and_last_node;
                                        if (data_offset_between_start_and_current + match_size >= first_end) {
                                            std::size_t match_size_last =
                                                    std::distance(std::get<1>(*(match_beg_intern)),
                                                                  std::get<2>(*(match_beg_intern))) + 1;
                                            std::size_t total_offset_end_for_last_node =
                                                    total_offset_front_for_last_node + match_size_last;

                                            //recursive overlap reconstruction
                                            std::get<2>(*(match_beg_intern + 1)) -= total_offset_end_for_last_node -
                                                                                    (total_offset_front_for_this_node +
                                                                                     match_size);
                                            std::size_t new_match_size =
                                                    std::distance(std::get<1>(*(match_beg_intern + 1)),
                                                                  std::get<2>(*(match_beg_intern + 1))) +
                                                    1;//first tree match
                                            auto new_match_begin = std::get<2>(*(match_beg_intern + 1)) + 1;
                                            auto new_match_end = new_match_begin + (match_size - new_match_size);
                                            //std::get<2>(*(match_beg_intern+1)) = tree_front->data.begin()+total_offset_end_for_last_node;
                                            //std::get<3>(*(match_beg_intern+1)) = tree_front;

                                            if constexpr (!reverse) {
                                                auto new_partial_match = std::make_tuple(new_match_begin,
                                                                                         new_match_end,
                                                                                         tree_front->data.cbegin() +
                                                                                         (total_offset_front_for_this_node -
                                                                                          (tree_front->data.size() -
                                                                                           new_match_size)),
                                                                                         tree_front);//second tree match

                                                // update until end
                                                auto match_insert_check = match_beg_intern;
                                                while (std::get<0>(*match_insert_check) <
                                                       std::get<0>(new_partial_match) &&
                                                       match_insert_check != match_end_intern)
                                                    match_insert_check++;
                                                actively_changing_trees.insert(match_insert_check, new_partial_match);
                                            } else {
                                                auto new_partial_match = std::make_tuple(new_match_begin,
                                                                                         new_match_end,
                                                                                         tree_front->data.crbegin() +
                                                                                         (total_offset_front_for_this_node -
                                                                                          (tree_front->data.size() -
                                                                                           new_match_size)),
                                                                                         tree_front);//second tree match

                                                // update until end
                                                auto match_insert_check = match_beg_intern;
                                                while (std::get<0>(*match_insert_check) <
                                                       std::get<0>(new_partial_match) &&
                                                       match_insert_check != match_end_intern)
                                                    match_insert_check++;
                                                actively_changing_trees.insert(match_insert_check, new_partial_match);
                                            }

                                            //vector pointer reset
                                            std::size_t active_tree_offset;

                                            if constexpr (!reverse) {
                                                active_tree_offset = std::distance(actively_changing_trees.begin(),
                                                                                   match_beg_intern_copy);
                                                match_beg_intern =
                                                        actively_changing_trees.begin() + active_tree_offset +
                                                        std::distance(match_beg_intern_copy, match_beg_intern);
                                                match_beg_intern_copy =
                                                        actively_changing_trees.begin() + active_tree_offset;
                                            } else {
                                                active_tree_offset = std::distance(actively_changing_trees.crbegin(),
                                                                                   match_beg_intern_copy);
                                                match_beg_intern =
                                                        actively_changing_trees.crbegin() + active_tree_offset +
                                                        std::distance(match_beg_intern_copy, match_beg_intern);
                                                match_beg_intern_copy =
                                                        actively_changing_trees.crbegin() + active_tree_offset;
                                            }
                                        } else {
                                            //simple build in tree front
                                            if constexpr (!reverse) {
                                                std::get<0>(*(match_beg_intern + 1)) = tree_front->data.cbegin() +
                                                                                       total_offset_front_for_this_node/*-distance from the last node until the current node start*/;
                                            } else {
                                                std::get<0>(*(match_beg_intern + 1)) = tree_front->data.crbegin() +
                                                                                       total_offset_front_for_this_node/*-distance from the last node until the current node start*/;
                                            }
                                            //std::get<3>(*(match_beg_intern+1)) = tree_front;
                                        }
                                    } else {
                                        //total match, new calculated reference on tree_back
                                        if constexpr (!reverse) {
                                            std::get<0>(*(match_beg_intern + 1)) = tree_back->data.cbegin() +
                                                                                   (total_offset_front_for_this_node -
                                                                                    tree_front->data.size());
                                        } else {
                                            std::get<0>(*(match_beg_intern + 1)) = tree_back->data.crbegin() +
                                                                                   (total_offset_front_for_this_node -
                                                                                    tree_front->data.size());
                                        }
                                        std::get<3>(*(match_beg_intern + 1)) = tree_back;
                                    }
                                    match_beg_intern++;
                                }
                            };

                            if (first_section_tree) {
                                modified.emplace(*first_tree_out);
                                added.emplace(*middle_tree_out);
                                //tree_match_pointer must move from first tree to middle tree, and we adjust offsets of all other matches
                                overlap_update(tree_front_data_front_absolute, actively_changing_trees, match_beg,
                                               match_beg_copy, *first_tree_out,
                                               *middle_tree_out);//update current tree pointer
                            } else {
                                modified.emplace(*middle_tree_out);
                            }
                            //only tree_match_pointer changes and offset changes to last tree if the end of the match exceeds limits and last tree exists
                            if (last_section_tree) {
                                added.emplace(
                                        *last_tree_out);//section of inner list must be over due to incomplete match
                                //tree_match_pointer must move from middle tree to last tree, and we adjust offsets of all other matches
                                overlap_update(tree_front_data_front_absolute, actively_changing_trees, match_beg,
                                               match_beg_copy, *middle_tree_out,
                                               *last_tree_out);//update current tree pointer
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
                        std::size_t vector_reset_dist = std::distance(match_beg_copy, match_beg);
                        match_beg = actively_changing_trees.begin() + vector_reset_dist + 1;
                        match_beg_copy = actively_changing_trees.begin();
                    }
                    search_element++;
                }
                if (std::get<0>(out_change_tuple) > 0)
                    out_change_tuple_out.push_back(
                            out_change_tuple);//only add change list if there was anything integrated
                single_beg++;
            }

            //deduplicate change list out

            return out_change_tuple_out;
        }

        template<class ContainerType, bool reverse = false>
        std::tuple<std::size_t, std::size_t, std::size_t> add_test(const ContainerType &c) {
            if constexpr (!reverse) {
                return add_test(c.cbegin(), c.cend());
            } else {
                return add_test(c.crbegin(), c.crend());
            }
        }

        //returns total size integrated, new space used uncompressed, new space used compressed
        //calculates ESTIMATE size of data to be integrated to be communicated to agency to determine optimal storage location
        template<typename Const_iterator,
                std::enable_if_t<((std::is_same<std::vector<unsigned char>::const_iterator, Const_iterator>::value ||
                                   std::is_same<std::list<unsigned char>::const_iterator, Const_iterator>::value ||
                                   std::is_same<std::deque<unsigned char>::const_iterator, Const_iterator>::value) ||
                                  (std::is_same<std::vector<unsigned char>::const_reverse_iterator, Const_iterator>::value ||
                                   std::is_same<std::list<unsigned char>::const_reverse_iterator, Const_iterator>::value ||
                                   std::is_same<std::deque<unsigned char>::const_reverse_iterator, Const_iterator>::value)), bool> = true
        >
        std::vector<std::tuple<std::size_t, std::size_t, std::size_t>>
        add_test(Const_iterator bin_beg, Const_iterator bin_end) {//add test should copy
            //first search existing structure and add into the last tree to insert potentially missing information

            //uncompressed input
            if (bin_beg == bin_end) {
                return {};
            }
            auto search_index = search(bin_beg, bin_end);
            constexpr bool reverse = (
                    std::is_same<std::vector<unsigned char>::const_reverse_iterator, decltype(bin_beg)>::value ||
                    std::is_same<std::list<unsigned char>::const_reverse_iterator, decltype(bin_beg)>::value ||
                    std::is_same<std::deque<unsigned char>::const_reverse_iterator, decltype(bin_beg)>::value);


            //cases for search index: its empty or it has content and with that a last tree element
            //cases for last tree if it exists, binary fit in: match from the beginning on, match in the middle, match until the end, total match
            //all lists contain lists with a last element that had multiple matches; add up all matches
            //std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<decltype(bin_beg), decltype(bin_end), decltype(bin_beg)>>>>>, std::size_t, decltype(bin_end), decltype(bin_beg)>>
            uh::util::compression_custom comp{};
            std::vector<std::tuple<std::size_t, std::size_t, std::size_t>> add_tup_out{};
            if (search_index.empty()) {
                std::tuple<std::size_t, std::size_t, std::size_t> add_tup{};
                auto set_vector = std::vector<unsigned char>{bin_beg, bin_end};

                if constexpr (reverse) {
                    std::reverse(set_vector.begin(), set_vector.end());
                }
                std::get<0>(add_tup) += set_vector.size();
                std::get<1>(add_tup) += set_vector.size();
                std::get<2>(add_tup) += comp.compress(set_vector.cbegin(),set_vector.cend()).size();
                add_tup_out.push_back(add_tup);
                return add_tup_out;
            }

            for (auto &single_route: search_index) {
                std::tuple<std::size_t, std::size_t, std::size_t> add_tup{};

                tree_radix_custom* last_tree;
                for(auto &list:std::get<0>(single_route)){
                    for(auto &tree_tuple:list){
                        last_tree = std::get<0>(tree_tuple);
                        for(auto &pos_tup:std::get<1>(tree_tuple)){
                            //auto add_list = tree_test_sequence(std::get<0>(tree_tuple), bin_beg, bin_beg,std::get<0>(pos_tup),
                            //                                   std::get<1>(pos_tup), std::get<2>(pos_tup));//insert into another tree
                            if (std::get<0>(tree_tuple)->data.empty()) {//how to insert, either empty simple insert or some tree construction anywhere
                                //simple insert into data since this seems to be a new node that can contain simple information
                                auto set_vector = std::vector<unsigned char>{std::get<1>(pos_tup), std::get<2>(pos_tup)};

                                if constexpr (reverse) {
                                    std::reverse(set_vector.begin(), set_vector.end());
                                }
                                std::get<0>(add_tup) += std::distance(std::get<1>(pos_tup),std::get<2>(pos_tup));
                                std::get<1>(add_tup) += std::distance(std::get<1>(pos_tup),std::get<2>(pos_tup));
                                std::get<2>(add_tup) += comp.compress(set_vector.cbegin(),set_vector.cend()).size();
                            } else {
                                std::get<0>(add_tup) += std::distance(std::get<1>(pos_tup),std::get<2>(pos_tup));
                            }
                        }
                    }
                }
                if(bin_beg+std::get<0>(add_tup)+1<bin_end){
                    std::get<2>(add_tup) += comp.compress(bin_beg+std::get<0>(add_tup)+1,bin_end).size();
                    std::get<1>(add_tup) += std::distance(bin_beg+std::get<0>(add_tup)+1,bin_end);
                    std::get<0>(add_tup) += std::distance(bin_beg+std::get<0>(add_tup)+1,bin_end);
                }
                add_tup_out.push_back(add_tup);
            }

            return add_tup_out;
        }

        //returns std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<decltype(bin_beg), decltype(bin_end), decltype(bin_beg)>>>>>, std::size_t>>
        template<typename Const_iterator_data, typename Const_iterator_binary,
                std::enable_if_t<
                        (((std::is_same<std::vector<unsigned char>::const_iterator, Const_iterator_data>::value ||
                           std::is_same<std::list<unsigned char>::const_iterator, Const_iterator_data>::value ||
                           std::is_same<std::deque<unsigned char>::const_iterator, Const_iterator_data>::value) ||
                          (std::is_same<std::vector<unsigned char>::const_reverse_iterator, Const_iterator_data>::value ||
                           std::is_same<std::list<unsigned char>::const_reverse_iterator, Const_iterator_data>::value ||
                           std::is_same<std::deque<unsigned char>::const_reverse_iterator, Const_iterator_data>::value))) &&
                        (((std::is_same<std::vector<unsigned char>::const_iterator, Const_iterator_binary>::value ||
                           std::is_same<std::list<unsigned char>::const_iterator, Const_iterator_binary>::value ||
                           std::is_same<std::deque<unsigned char>::const_iterator, Const_iterator_binary>::value) ||
                          (std::is_same<std::vector<unsigned char>::const_reverse_iterator, Const_iterator_binary>::value ||
                           std::is_same<std::list<unsigned char>::const_reverse_iterator, Const_iterator_binary>::value ||
                           std::is_same<std::deque<unsigned char>::const_reverse_iterator, Const_iterator_binary>::value))), bool> = true
        >
        auto
        search_match_filter(Const_iterator_data data_beg, Const_iterator_data data_end, Const_iterator_binary bin_beg,
                            Const_iterator_binary bin_end,
                            std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<decltype(data_beg), decltype(bin_end), decltype(bin_beg)>>>>>, std::size_t, decltype(bin_end), decltype(bin_beg)>> possibilities =
                            std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<decltype(data_beg), decltype(bin_end), decltype(bin_beg)>>>>>, std::size_t, decltype(bin_end), decltype(bin_beg)>>{}) {
            auto local_matches = compare_ultihash(data_beg, data_end, bin_beg, bin_end);

            auto legal_match_integration = [&data_beg, &data_end](auto local_matches) {
                std::sort(local_matches.begin(), local_matches.end(), [](auto &a, auto &b) {
                    return std::distance(std::get<1>(a), std::get<2>(a)) <
                           std::distance(std::get<1>(b), std::get<2>(b));
                });
                //LEGAL MATCH FILTER
                auto match_beg = local_matches.begin();

                auto legal_check = [&local_matches](
                        auto data_beg, auto data_end, auto &match_beg, auto returning_offset) {
                    //on empty or partial match make new list in list, else append the match results on total match
                    bool legal_split;
                    //if the end was matched too long we can do something about that, but else the algorithm is prefix oriented
                    bool start_size, end_size, begin_reached, end_reached, total_found_size;
                    do {

                        start_size = MINIMUM_MATCH_SIZE <= llabs(std::distance(data_beg, std::get<0>(*match_beg)));
                        end_size = MINIMUM_MATCH_SIZE <= llabs(std::distance(std::get<0>(*match_beg) +
                                                                             std::distance(std::get<1>(*match_beg),
                                                                                           std::get<2>(*match_beg)),
                                                                             data_end));
                        total_found_size = MINIMUM_MATCH_SIZE <= std::distance(std::get<1>(*match_beg),
                                                                               std::get<2>(*match_beg));
                        begin_reached = data_beg == std::get<0>(*match_beg);
                        end_reached = data_end == std::get<0>(*match_beg) +
                                                  std::distance(std::get<1>(*match_beg), std::get<2>(*match_beg));
                        legal_split = ((start_size || begin_reached) && (end_size || end_reached) && total_found_size);

                        if (!start_size && !begin_reached) {
                            std::get<0>(*match_beg)++;
                            std::get<1>(*match_beg)++;
                            std::get<2>(*match_beg)--;
                        }
                        if (!end_size && !end_reached) {
                            std::get<2>(*match_beg)--;
                        }
                        if (std::distance(std::get<1>(*match_beg), std::get<2>(*match_beg)) <
                            MINIMUM_MATCH_SIZE) {
                            local_matches.erase(match_beg);
                            match_beg = returning_offset;
                            return false;
                        }
                    } while (((!end_size && !end_reached) || (!start_size && !begin_reached)) &&
                             !local_matches.empty());
                    match_beg++;
                    return legal_split;
                };

                while (match_beg !=
                       local_matches.end()) {//shrink all matches until the total result is legal, shrink smaller matches first until they vanish
                    legal_check(data_beg, data_end, match_beg, local_matches.begin());
                    auto match_begin_legal_shift = local_matches.begin();
                    while (match_begin_legal_shift != local_matches.end()) {
                        legal_check(std::get<0>(*match_begin_legal_shift), std::get<0>(*match_begin_legal_shift) +
                                                                           std::distance(std::get<1>(
                                                                                                 *match_begin_legal_shift),
                                                                                         std::get<2>(
                                                                                                 *match_begin_legal_shift)),
                                    match_begin_legal_shift, local_matches.begin());
                    }
                }

                std::sort(local_matches.begin(), local_matches.end(), [](auto &a, auto &b) {
                    return std::distance(std::get<1>(a), std::get<2>(a)) >
                           std::distance(std::get<1>(b), std::get<2>(b));
                });

                if (local_matches.size() > 1) {
                    std::size_t max_fit{};
                    auto best_beg = local_matches.begin();
                    while (best_beg != local_matches.end()) {
                        max_fit = std::max(max_fit,
                                           (std::size_t) std::distance(std::get<1>(*best_beg), std::get<2>(*best_beg)));
                        if (std::distance(std::get<1>(*best_beg), std::get<2>(*best_beg)) < max_fit) {
                            //break and delete until end
                            break;
                        }
                        best_beg++;
                    }
                    local_matches.erase(best_beg, local_matches.end());
                }
                //sort the smallest offset out of the largest match results in case the match sizes are equal
                std::sort(local_matches.begin(), local_matches.end(), [&data_beg](auto &a, auto &b) {
                    return std::distance(data_beg, std::get<0>(a)) < std::distance(data_beg, std::get<0>(b));
                });
                return local_matches;
            };

            if (local_matches.empty())return possibilities;

            local_matches = legal_match_integration(local_matches);

            decltype(possibilities) out_possibilities;

            auto match_beg = local_matches.begin();
            auto possibilities_manage = [&](auto &input_list_tmp) {
                std::vector<std::tuple<decltype(data_beg), decltype(bin_end), decltype(bin_beg)>> found_vec{};
                found_vec.emplace_back(std::get<1>(*match_beg), std::get<2>(*match_beg),
                                       std::get<0>(*match_beg));

                if (std::get<0>(input_list_tmp).empty()) {
                    std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<decltype(data_beg), decltype(bin_end), decltype(bin_beg)>>>> tmp_list{};
                    tmp_list.emplace_back(this, found_vec);
                    std::get<0>(input_list_tmp).push_back(tmp_list);
                } else {
                    //if the tree element of the last element is still the same as "this" we append to the vector, else we append a new list
                    auto last_it_outer_list = (--(std::get<0>(input_list_tmp).end()));
                    auto last_it_inner_list = (--(last_it_outer_list->end()));
                    if (std::get<0>(*last_it_inner_list) == this) {//check if tree pointer is the same
                        std::get<1>(*last_it_inner_list).push_back(found_vec[0]);
                        std::get<1>(*last_it_inner_list) = legal_match_integration(std::get<1>(*last_it_inner_list));
                    } else {
                        last_it_outer_list->emplace_back(this, found_vec);
                    }
                }
                std::size_t advance =
                        (std::size_t) llabs(std::distance(std::get<1>(*match_beg), std::get<2>(*match_beg)));
                std::get<1>(input_list_tmp) += advance;
                std::get<2>(input_list_tmp)++;
                std::get<3>(input_list_tmp) = std::min(std::get<3>(input_list_tmp)+advance,bin_end);

                out_possibilities.push_back(input_list_tmp);
            };

            while (match_beg != local_matches.end()) {
                if (possibilities.empty()) {
                    std::vector<std::tuple<decltype(data_beg), decltype(bin_end), decltype(bin_beg)>> found_vec{};
                    found_vec.emplace_back(std::get<0>(*match_beg), std::get<1>(*match_beg),
                                           std::get<2>(*match_beg));
                    std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<decltype(bin_beg), decltype(bin_end), decltype(bin_beg)>>>> tmp_list{};
                    tmp_list.emplace_back(this, found_vec);
                    std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<decltype(bin_beg), decltype(bin_end), decltype(bin_beg)>>>>> outer_list{
                            tmp_list};

                    std::size_t advance =
                            (std::size_t) llabs(std::distance(std::get<1>(*match_beg), std::get<2>(*match_beg)))-1;
                    out_possibilities.emplace_back(outer_list, advance, std::get<0>(*match_beg) + 1,
                                                   std::min(std::get<1>(*match_beg) + advance,bin_end));
                } else
                    for (auto &input_list_tmp: possibilities) {//COPY input list and create different path calculation
                        possibilities_manage(input_list_tmp);
                    }
                match_beg++;
            }
            return out_possibilities;
        }

        //returns the path of maximum fit and the match size
        /*std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::vector<unsigned char>::const_iterator, std::vector<unsigned char>::const_iterator, std::vector<unsigned char>::const_iterator>>>>>, std::size_t>>*/
        template<typename Const_iterator,
                std::enable_if_t<((std::is_same<std::vector<unsigned char>::const_iterator, Const_iterator>::value ||
                                   std::is_same<std::list<unsigned char>::const_iterator, Const_iterator>::value ||
                                   std::is_same<std::deque<unsigned char>::const_iterator, Const_iterator>::value) ||
                                  (std::is_same<std::vector<unsigned char>::const_reverse_iterator, Const_iterator>::value ||
                                   std::is_same<std::list<unsigned char>::const_reverse_iterator, Const_iterator>::value ||
                                   std::is_same<std::deque<unsigned char>::const_reverse_iterator, Const_iterator>::value)), bool> = true
        >
        auto search(Const_iterator bin_beg, Const_iterator bin_end,
                    std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<decltype(bin_beg), decltype(bin_end), decltype(bin_beg)>>>>>, std::size_t, decltype(bin_end), decltype(bin_beg)>> possibilities =
                    std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<decltype(bin_beg), decltype(bin_end), decltype(bin_beg)>>>>>, std::size_t, decltype(bin_end), decltype(bin_beg)>>{},
                    std::vector<tree_radix_custom *> limiter_children = std::vector<tree_radix_custom *>{}) {

            if (bin_beg == bin_end ||
                std::any_of(limiter_children.begin(), limiter_children.end(), [this](auto &item_limit) {
                    return item_limit == this;
                })) {
                return possibilities;
            }

            constexpr bool reverse = (
                    std::is_same<std::vector<unsigned char>::const_reverse_iterator, decltype(bin_beg)>::value ||
                    std::is_same<std::list<unsigned char>::const_reverse_iterator, decltype(bin_beg)>::value ||
                    std::is_same<std::deque<unsigned char>::const_reverse_iterator, decltype(bin_beg)>::value);

            /*std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<decltype(bin_beg), decltype(bin_end), decltype(bin_beg)>>>>>, std::size_t,decltype(bin_end), decltype(bin_beg)>>*/
            //while a possibility still changes on search_match filter, it is continued to be executed
            if constexpr (!reverse) {
                possibilities = search_match_filter(data.cbegin(), data.cend(), bin_beg, bin_end, possibilities);
            } else {
                possibilities = search_match_filter(data.crbegin(), data.crend(), bin_beg, bin_end, possibilities);
            }

            decltype(possibilities) new_recursive{};

            auto single_pos = possibilities.begin();
            std::size_t single_count{};
            while (single_pos != possibilities.end()) {
                if constexpr (!reverse) {
                    if (std::get<2>(*single_pos) == data.cend() || std::get<3>(*single_pos) == bin_end) {
                        single_count++;
                        single_pos++;
                        continue;
                    }
                } else {
                    if (std::get<2>(*single_pos) == data.crend() || std::get<3>(*single_pos) == bin_end) {
                        single_count++;
                        single_pos++;
                        continue;
                    }
                }
                decltype(possibilities) tmp;
                if constexpr (!reverse) {
                    tmp = search_match_filter(std::get<2>(*single_pos), data.cend(), std::get<3>(*single_pos), bin_end,
                                              possibilities);
                } else {
                    tmp = search_match_filter(std::get<2>(*single_pos), data.crend(), std::get<3>(*single_pos), bin_end,
                                              possibilities);
                }
                std::for_each(tmp.begin(), tmp.end(), [&tmp, &new_recursive](auto &item1) {
                    if (std::none_of(new_recursive.begin(), new_recursive.end(), [&item1](auto &item2) {
                        return item2 == item1;
                    })) {
                        new_recursive.push_back(item1);
                    }
                });

                single_pos = possibilities.begin() + single_count;
                single_count++;
            }
            //check child that deals with searching the far most rest in direction of end to skip the not matching rest
            //only check children with correct continue letter first in case we get a total match
            for (auto pos_begin: possibilities) {
                if (std::get<3>(pos_begin) == bin_end) {
                    continue;
                }
                bool total_match = false;
                auto child_vec = child_vector(*std::get<3>(pos_begin));
                if (!child_vec.empty()) {//recursive search
                    for (auto &item: child_vec) {//vector of tree pointers
                        auto tmp = item->search(std::get<3>(pos_begin), bin_end, possibilities);
                        if(std::any_of(tmp.begin(),tmp.end(),[&bin_beg,&bin_end](auto &item){
                            return std::get<3>(item) == bin_end;
                        }))total_match = true;
                        if (tmp != possibilities){
                            std::for_each(tmp.begin(), tmp.end(), [&tmp, &new_recursive](auto &item1) {
                                if (std::none_of(new_recursive.begin(), new_recursive.end(), [&item1](auto &item2) {
                                    return item2 == item1;
                                })) {
                                    new_recursive.push_back(item1);
                                }
                            });
                        }
                    }
                }

                if(total_match)break;
                //do not search other children if a directed match has been found --> fast

                //slow search
                for (auto &c: children) {
                    if (!child_vec.empty() && std::get<1>(c) == *std::get<3>(pos_begin))continue;
                    for (auto &item: std::get<0>(c)) {//vector of tree pointers
                        if (std::get<3>(pos_begin) >= bin_end) {
                            continue;
                        }
                        auto tmp = item->search(std::get<3>(pos_begin), bin_end,possibilities);
                        if (tmp != possibilities){
                            std::for_each(tmp.begin(), tmp.end(), [&tmp, &new_recursive](auto &item1) {
                                if (std::none_of(new_recursive.begin(), new_recursive.end(), [&item1](auto &item2) {
                                    return item2 == item1;
                                })) {
                                    new_recursive.push_back(item1);
                                }
                            });
                        }
                    }
                }
            }

            std::for_each(new_recursive.begin(), new_recursive.end(), [&possibilities, &new_recursive](auto &item1) {
                if (std::none_of(possibilities.begin(), possibilities.end(), [&item1](auto &item2) {
                    return item2 == item1;
                })) {
                    possibilities.push_back(item1);
                }
            });

            //return the largest match with the lowest offset on the last tree, as far as there is a last tree...
            std::sort(possibilities.begin(), possibilities.end(), [](auto &a, auto &b) {
                return std::get<1>(a) > std::get<1>(b);//sort in descending order on search match size
            });

            std::size_t max_val{};
            auto poss_beg = possibilities.begin();
            while (poss_beg != possibilities.end()) {
                max_val = std::max(max_val, std::get<1>(*poss_beg));
                if (std::get<1>(*poss_beg) < max_val) {
                    possibilities.erase(poss_beg, possibilities.end());
                    break;
                }
                poss_beg++;
            }

            std::sort(possibilities.begin(), possibilities.end(), [](auto &a, auto &b) {
                return std::get<1>(a) > std::get<1>(b);//sort in descending order on search match size
            });

            return possibilities;
        }
    };
}

#endif //UHLIBCOMMON_TREE_RADIX_CUSTOM_H
