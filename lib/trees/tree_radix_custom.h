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

        //returns match position on data of tree node, relative until end of string, binary advancement, match index (order match was found)
        template<class ContainerData, class ContainerBinary, bool reverse = false>
        std::vector<std::tuple<std::size_t, std::size_t, std::size_t,std::size_t>>
        compare_ultihash(ContainerData &data_cont, ContainerBinary &binary_cont,std::size_t binary_offset = 0) {
            //if input does only fit to a shorter string as a subset of data, count becomes negative, else positive including ß
            //data offset iterator and start and end of input
            std::vector<std::tuple<std::size_t, std::size_t,std::size_t,std::size_t>> matches{};//data offset beginning found, end offset from beginning
            if (data_cont.empty() || binary_cont.empty())return matches;

            std::set<std::size_t> advancements_binary{binary_offset};
            std::size_t match_index{};


            //search forward through data
            if constexpr (!reverse) {

                for(auto adv:advancements_binary){
                    if(adv > binary_cont.size())continue;
                    std::size_t current_offset = 0;
                    do {
                        //first element match
                        std::unique_lock lock(simd_protect);
                        if (simd_count < SIMD_UNITS) {
                            simd_count += 1;
                            lock.unlock();
                            current_offset = std::distance(data_cont.begin(), std::find(std::execution::unseq,
                                                                                        data_cont.begin() + current_offset,
                                                                                        data_cont.end(),
                                                                                        *(binary_cont.begin()+adv)));
                            lock.lock();
                            simd_count -= 1;
                            lock.unlock();
                        } else {
                            lock.unlock();
                            current_offset = std::distance(data_cont.begin(),
                                                           std::find(data_cont.begin() + current_offset, data_cont.end(),
                                                                     *(binary_cont.begin()+adv)));
                        }

                        //search how long input matches
                        std::pair<decltype(data_cont.begin()), decltype(data_cont.end())> found;
                        lock.lock();
                        if (simd_count < SIMD_UNITS) {
                            simd_count += 1;
                            lock.unlock();
                            found = std::mismatch(std::execution::unseq, data_cont.begin() + current_offset,
                                                  data_cont.end(), binary_cont.begin()+adv, binary_cont.end());
                            lock.lock();
                            simd_count -= 1;
                            lock.unlock();
                        } else {
                            lock.unlock();
                            found = std::mismatch(data_cont.begin() + current_offset, data_cont.end(), binary_cont.begin()+adv,
                                                  binary_cont.end());
                        }

                        if (std::max((long) std::distance(data_cont.begin() + current_offset, found.first) - 1, (long) 0) >=
                            MINIMUM_MATCH_SIZE) {
                            long integrate_offset =  std::max(
                                    (long) std::distance(data_cont.begin() + current_offset, found.first) - 1, (long) 0);
                            matches.emplace_back(current_offset,integrate_offset,adv,match_index);
                            advancements_binary.emplace(integrate_offset);
                            match_index++;
                        }
                        if (data_cont.begin() + current_offset != data_cont.end())current_offset++;
                    } while (data_cont.begin() + current_offset != data_cont.end());
                }
            } else {
                for(auto adv:advancements_binary){
                    std::size_t current_offset = 0;

                    do {
                        //first element match
                        std::unique_lock lock(simd_protect);
                        if (simd_count < SIMD_UNITS) {
                            simd_count += 1;
                            lock.unlock();
                            current_offset = std::distance(data_cont.rbegin(), std::find(std::execution::unseq,
                                                                                        data_cont.rbegin() + current_offset,
                                                                                        data_cont.rend(),
                                                                                        *(binary_cont.begin()+adv)));
                            lock.lock();
                            simd_count -= 1;
                            lock.unlock();
                        } else {
                            lock.unlock();
                            current_offset = std::distance(data_cont.rbegin(),
                                                           std::find(data_cont.rbegin() + current_offset, data_cont.rend(),
                                                                     *(binary_cont.begin()+adv)));
                        }

                        //search how long input matches
                        std::pair<decltype(data_cont.begin()), decltype(data_cont.end())> found;
                        lock.lock();
                        if (simd_count < SIMD_UNITS) {
                            simd_count += 1;
                            lock.unlock();
                            found = std::mismatch(std::execution::unseq, data_cont.rbegin() + current_offset,
                                                  data_cont.rend(), binary_cont.begin()+adv, binary_cont.end());
                            lock.lock();
                            simd_count -= 1;
                            lock.unlock();
                        } else {
                            lock.unlock();
                            found = std::mismatch(data_cont.rbegin() + current_offset, data_cont.rend(), binary_cont.begin()+adv,
                                                  binary_cont.end());
                        }

                        if (std::max((long) std::distance(data_cont.rbegin() + current_offset, found.first) - 1, (long) 0) >=
                            MINIMUM_MATCH_SIZE) {
                            long integrate_offset =  std::max(
                                    (long) std::distance(data_cont.rbegin() + current_offset, found.first) - 1, (long) 0);
                            matches.emplace_back(current_offset,integrate_offset,adv,match_index);
                            advancements_binary.emplace(integrate_offset);
                            match_index++;
                        }
                        if (data_cont.rbegin() + current_offset != data_cont.end())current_offset++;
                    } while (data_cont.rbegin() + current_offset != data_cont.end());
                }
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
            std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>,std::size_t, std::size_t>> search_index = search<ContainerBinary, reverse>(binary_cont);//std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>,std::size_t, std::size_t>>>

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
        std::tuple<std::size_t, std::size_t, std::size_t>
        add_test(ContainerBinary &cont_binary) {//add test should copy
            //first search existing structure and add into the last tree to insert potentially missing information

            //uncompressed input
            if (cont_binary.empty()) {
                return {};
            }
            std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>,std::size_t, std::size_t>> search_index = search<ContainerBinary, reverse>(cont_binary);//std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>,std::size_t, std::size_t>>>

            //cases for search index: its empty or it has content and with that a last tree element
            //cases for last tree if it exists, binary fit in: match from the beginning on, match in the middle, match until the end, total match
            //all lists contain lists with a last element that had multiple matches; add up all matches
            uh::util::compression_custom comp{};
            std::tuple<std::size_t, std::size_t, std::size_t> add_tup{};
            if (search_index.empty()) {
                std::get<0>(add_tup) += cont_binary.size();
                std::get<1>(add_tup) += cont_binary.size();
                std::get<2>(add_tup) += comp.compress(cont_binary).size();
                return add_tup;
            }

            std::size_t count{};
            for (auto &single_route: search_index) {
                for (auto &pos_tup: std::get<1>(single_route)) {
                    auto set_vector = std::vector<unsigned char>{};
                    std::copy(std::get<0>(single_route)->data.begin() + std::get<0>(pos_tup), std::min(std::get<0>(single_route)->data.end(),std::get<0>(single_route)->data.begin() + std::get<0>(pos_tup)+ std::get<1>(pos_tup)+1),
                              std::back_inserter(set_vector));

                    std::get<0>(add_tup) += set_vector.size();
                    std::size_t current_new_space = set_vector.size() - (std::get<1>(pos_tup)+1);
                    if(set_vector.empty()||current_new_space == 0){
                        continue;
                    }
                    std::get<1>(add_tup) += current_new_space;
                    std::get<2>(add_tup) += comp.compress(set_vector).size();
                }
                count++;
            }

            if(cont_binary.size()>std::get<0>(add_tup)){
                auto set_vector = std::vector<unsigned char>{};
                std::copy(cont_binary.begin() + std::get<0>(add_tup), cont_binary.end(),
                          std::back_inserter(set_vector));
                std::get<0>(add_tup) += set_vector.size();
                std::get<1>(add_tup) += set_vector.size();
                std::get<2>(add_tup) += comp.compress(set_vector).size();
            }

            return add_tup;
        }

        //returns std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<decltype(bin_beg), decltype(bin_end), decltype(bin_beg)>>>>>, std::size_t>>
        template<class ContainerData, class ContainerBinary, bool reverse = false>
        std::vector<std::tuple<std::size_t, std::size_t,std::size_t, std::size_t>>
        search_match_filter(ContainerData &data_cont,ContainerBinary &binary_cont,
                            std::vector<std::tuple<std::size_t, std::size_t,std::size_t, std::size_t>> possibilities =
                            std::vector<std::tuple<std::size_t, std::size_t,std::size_t, std::size_t>>{},std::size_t binary_offset = 0) {

            auto local_matches = compare_ultihash<ContainerData,ContainerBinary,reverse>(data_cont, binary_cont,binary_offset);//std::vector<std::tuple<std::size_t, std::size_t, std::size_t,std::size_t>>

            //add maximum match index of possibilities onto local matches
            std::size_t max_match_index{};
            for(const auto &index_item:possibilities){
                max_match_index = std::max(std::get<3>(index_item),max_match_index);
            }

            auto match_duplicate = local_matches.begin();
            while(match_duplicate!=local_matches.end()){
                bool dup_found = false;
                for(const auto &dup:possibilities){
                    if(std::get<0>(*match_duplicate)==std::get<0>(dup) && std::get<1>(*match_duplicate)==std::get<1>(dup)){
                        dup_found = true;
                        break;
                    }
                }
                if(dup_found){
                    std::size_t reinvoke_count = std::distance(local_matches.begin(),match_duplicate);
                    local_matches.erase(match_duplicate);
                    match_duplicate = local_matches.begin()+reinvoke_count;
                    continue;
                }
                else{
                    std::get<3>(*match_duplicate)+=max_match_index;//add up match index so the new matches have higher index for sorting
                    possibilities.push_back(*match_duplicate);
                }
                match_duplicate++;
            }

            if (possibilities.empty())return possibilities;

            std::sort(possibilities.begin(), possibilities.end(), [](auto &a, auto &b) {
                return std::get<0>(a) > std::get<0>(b);//for legal checks we try to start from the end to mostly shrink not fitting overlapping beginnings of matches
            });
            //LEGAL MATCH FILTER
            //TODO: also permutate submatches
            auto legal_check = [&data_cont](auto &match_beg, std::size_t start_val,
                                                            std::size_t end_val) {
                //on empty or partial match make new list in list, else append the match results on total match
                bool legal_split;
                bool changed = false;
                //if the end was matched too long we can do something about that, but else the algorithm is prefix oriented
                bool start_size, end_size, begin_reached, end_reached, total_found_size;
                do {

                    start_size = llabs((long)std::get<0>(*match_beg) - (long) start_val) >= MINIMUM_MATCH_SIZE;
                    end_size = llabs((long) end_val-((long)std::get<0>(*match_beg)+(long)std::get<1>(*match_beg)+1)) >= MINIMUM_MATCH_SIZE;
                    total_found_size = MINIMUM_MATCH_SIZE <= std::get<1>(*match_beg);
                    begin_reached = std::get<0>(*match_beg) == start_val;
                    end_reached = std::get<0>(*match_beg)+std::get<1>(*match_beg) + 1 == end_val;
                    legal_split = ((start_size || begin_reached) && (end_size || end_reached) && total_found_size);

                    if (!start_size && !begin_reached) {
                        std::get<0>(*match_beg)++;//relative offset changes
                        std::get<1>(*match_beg)--;
                        changed = true;
                    }
                    if (!end_size && !end_reached) {
                        std::get<1>(*match_beg)--;
                        changed = true;
                    }
                    if (std::get<1>(*match_beg) < MINIMUM_MATCH_SIZE) {
                        return std::make_tuple(legal_split,changed);
                    }
                } while (((!end_size && !end_reached) || (!start_size && !begin_reached)));

                return std::make_tuple(legal_split,changed);
            };

            auto match_beg = possibilities.begin();

            auto change_delete = [&possibilities](auto &match_beg){
                std::size_t delete_binary_shift_higher = std::get<2>(*match_beg);
                std::size_t element_match_index = std::get<3>(*match_beg);
                std::erase_if(possibilities,[&delete_binary_shift_higher,&element_match_index](auto &item){
                    return std::get<2>(item)>delete_binary_shift_higher && std::get<3>(item)>element_match_index;
                });
            };

            while (match_beg != possibilities.end()) {//shrink all matches until the total result is legal, shrink smaller matches first until they vanish
                auto legal_general = legal_check(match_beg, (std::size_t) 0,data_cont.size());
                if(std::get<1>(legal_general)){
                    std::size_t old_size_poss = possibilities.size();
                    change_delete(match_beg);
                    if(old_size_poss!=possibilities.size()){
                        match_beg = possibilities.begin();
                        continue;
                    }
                }
                if (!std::get<0>(legal_general)) {
                    possibilities.erase(match_beg);
                    std::sort(possibilities.begin(),possibilities.end(),[](auto &a,auto &b){
                        return std::get<0>(a) < std::get<1>(b);
                    });
                    match_beg = possibilities.begin();
                    continue;
                }

                auto match_begin_legal_shift = possibilities.begin();
                while (match_begin_legal_shift != possibilities.end()) {
                    auto legal = legal_check(match_begin_legal_shift, std::get<0>(*match_beg),
                                             std::get<0>(*match_beg) + std::get<1>(*match_beg) + 1);
                    if(std::get<1>(legal)){
                        std::size_t old_size_poss = possibilities.size();
                        change_delete(match_begin_legal_shift);
                        if(old_size_poss!=possibilities.size()){
                            match_begin_legal_shift = possibilities.begin();
                            continue;
                        }
                    }
                    if (!std::get<0>(legal)) {
                        possibilities.erase(match_begin_legal_shift);
                        std::sort(possibilities.begin(),possibilities.end(),[](auto &a,auto &b){
                            return std::get<0>(a) < std::get<1>(b);
                        });
                        match_begin_legal_shift = possibilities.begin();
                    } else match_begin_legal_shift++;
                }
                match_beg++;
            }

            //sort the smallest offset out of the largest match results in case the match sizes are equal*/
            std::sort(possibilities.begin(), possibilities.end(), [](auto &a, auto &b) {
                return std::get<0>(a) < std::get<0>(b);
            });
            //all matches are valid as long as they do not collide

            return possibilities;
        }


        //returns the path of maximum fit and the match size
        template<class ContainerBinary, bool reverse = false>
        std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>,std::size_t, std::size_t>>
        search(ContainerBinary &cont_binary, std::vector<tree_radix_custom*> limiter_children = std::vector<tree_radix_custom*>{}) {

            if (cont_binary.empty()||std::any_of(limiter_children.begin(),limiter_children.end(),[this](auto &item){
                return item == this;
            })) {
                return std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>,std::size_t, std::size_t>>{};
            }

            std::vector<std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>,std::size_t, std::size_t>>>possibilities_out{};

            //a list holding possibilities of paths of matches
            //                              tree of results                     matches with offset and size       num matches  binary advance
            std::list<std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>,std::size_t, std::size_t>>>possibilities_work{};
            std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>,std::size_t, std::size_t>> start_node{};
            std::set<std::size_t> advancements{0};

            start_node.emplace_back(this,std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>{},0,0);
            possibilities_work.push_back(start_node);

            auto sum_match_size = [](std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>,std::size_t, std::size_t>> &input){
                auto last_element = input.end();
                std::advance(last_element,-1);

                return std::get<3>(*last_element);
                /*
                return std::accumulate(input.begin(),input.end(),(std::size_t)0,[](std::size_t last_sum,std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>,std::size_t, std::size_t> &item){
                    return last_sum+std::get<3>(item);
                });
                 */
            };

            auto max_match_sort = [&sum_match_size](std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>,std::size_t, std::size_t>> &a,std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>,std::size_t, std::size_t>>&b){
                return sum_match_size(a) > sum_match_size(b);
            };

            auto optimal_multiadvance = [](std::vector<std::tuple<std::size_t, std::size_t,std::size_t, std::size_t>> input2){
                if(input2.empty())return input2;

                std::sort(input2.begin(),input2.end(),[](auto &a, auto &b){
                    return std::get<3>(a)<std::get<3>(b);
                });

                //paths must follow each other so that size and advancement are aligned
                std::vector<std::vector<std::tuple<std::size_t, std::size_t,std::size_t, std::size_t>>> micro_paths{};//second tuple element is current advancement
                //link the paths that match advancements, duplicate path with new ending if there are multiple paths for that offset
                auto input_micro_beg = input2.begin();
                while(input_micro_beg!=input2.end()){
                    //on advancement match append to correct path ending
                    std::size_t num_integrated{};
                    bool integrated = false;
                    for(auto &m:micro_paths){
                        //for all paths in m try to check if the input element matches either a predecessor or next element and insert

                        auto tup_next_check_beg = m.begin();
                        while(tup_next_check_beg!=m.end()){
                            //only add to filter list if the size matches the difference of advancement
                            std::size_t old_integrate_count = num_integrated;
                            //append before
                            if(std::get<2>(*tup_next_check_beg) == std::get<2>(*input_micro_beg)+std::get<1>(*tup_next_check_beg)+1 &&
                               std::get<3>(*tup_next_check_beg) > std::get<3>(*input_micro_beg)){
                                m.push_back(*input_micro_beg);
                                num_integrated++;
                            }
                            //append after
                            if(std::get<2>(*input_micro_beg) == std::get<2>(*tup_next_check_beg)+std::get<1>(*input_micro_beg)+1 && old_integrate_count==num_integrated &&
                               std::get<3>(*input_micro_beg) > std::get<3>(*tup_next_check_beg)){
                                m.push_back(*(input_micro_beg+1));
                                num_integrated++;
                            }
                            if(old_integrate_count!=num_integrated){
                                (void)input2.erase(input_micro_beg);
                                input_micro_beg = input2.begin();
                                integrated = true;
                                break;
                            }
                            tup_next_check_beg++;
                        }
                        if(integrated)break;
                    }
                    if(!num_integrated || micro_paths.empty()){
                        micro_paths.push_back(std::vector<std::tuple<std::size_t, std::size_t,std::size_t, std::size_t>>{*input_micro_beg});
                        (void)input2.erase(input_micro_beg);
                        input_micro_beg = input2.begin();
                    }
                }

                for(auto &input:micro_paths){
                    std::sort(input.begin(),input.end(),[](auto &a, auto &b){
                        return std::get<2>(a)<std::get<2>(b);
                    });
                    //ordered after advancement, take the best result of each advancement
                    auto input_beg = input.begin();
                    std::size_t current_advance = std::get<2>(*input_beg);

                    std::vector<std::tuple<std::size_t, std::size_t,std::size_t, std::size_t>> filter_list{};
                    std::set<std::tuple<std::size_t, std::size_t,std::size_t, std::size_t>> out_list{};
                    //filter large matches for each advancement layer
                    while(input_beg != input.end()){
                        if(std::get<2>(*input_beg)!= current_advance){
                            std::sort(filter_list.begin(),filter_list.end(),[](auto &a, auto &b){
                                return std::get<1>(a)>std::get<1>(b);//sort for largest matches
                            });
                            if(!filter_list.empty()){
                                (void)out_list.emplace(filter_list.front());
                            }
                            filter_list.clear();
                            (void)input.erase(input.begin(), input_beg-1);
                            std::sort(input.begin(),input.end(),[](auto &a, auto &b){
                                return std::get<2>(a)<std::get<2>(b);
                            });
                            (void)input.erase(input_beg);
                            input_beg = input.begin();
                            current_advance = std::get<2>(*input_beg);
                        }
                        if(input_beg==input.end())continue;

                        filter_list.push_back(*input_beg);
                        input_beg++;
                    }

                    if(!filter_list.empty()){
                        std::sort(filter_list.begin(),filter_list.end(),[](auto &a, auto &b){
                            return std::get<1>(a)>std::get<1>(b);//sort for largest matches
                        });
                        (void)out_list.emplace(filter_list.front());
                    }

                    input.assign(out_list.begin(),out_list.end());

                    std::sort(input.begin(),input.end(),[](auto &a, auto &b){
                        return std::get<1>(a)<std::get<1>(b);//sort for largest matches
                    });
                }

                std::ranges::remove_if(micro_paths.begin(),micro_paths.end(),[](auto &a){
                    return a.empty();
                });

                //return biggest advancement difference
                auto max_diff = std::max_element(micro_paths.begin(),micro_paths.end(),[](auto &a,auto &b){
                    auto last_a = a.back();
                    auto last_b = b.back();

                    return (long)std::get<2>(last_a) < (long)std::get<2>(last_b);
                });

                return *max_diff;//return path with maximum advancement
            };

            auto poss_beg = possibilities_work.begin();
            while(poss_beg != possibilities_work.end()){
                if(sum_match_size(*poss_beg) == cont_binary.size()){
                    //immediate total match return
                    return *poss_beg;
                }

                //search data within with all possible advancements on the last node and its children
                for(auto adv:advancements){
                    //read first entry, search with permutation of advancement and append the results to the end of work list
                    std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>,std::size_t, std::size_t>> current_path = *poss_beg;

                    //search within last node of that path
                    auto search_within = current_path.end();//std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>,std::size_t, std::size_t>
                    std::advance(search_within,-1);
                    //search
                    std::size_t new_base_advance = adv + std::get<3>(*search_within);
                    if(new_base_advance>cont_binary.size())continue;
                    std::size_t matches_before = std::get<1>(*search_within).size();
                    //std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t,std::size_t, std::size_t>>
                    //filter best binary advancement on a single node
                    std::get<1>(*search_within) = optimal_multiadvance(search_match_filter(std::get<0>(*search_within)->data,cont_binary,std::get<1>(*search_within),new_base_advance));

                    std::size_t more_found = std::get<1>(*search_within).size()-matches_before;
                    if(!more_found)continue;
                    std::get<2>(*search_within)+=more_found;

                    //from results add likely advancements of binary input
                    for(const auto s:std::get<1>(*search_within)){
                        std::size_t new_advancement = new_base_advance + std::get<1>(s)+1;
                        if(new_advancement>=cont_binary.size()){
                            continue;
                        }
                        advancements.emplace(new_advancement);
                    }
                    //child search
                    if(std::get<0>(*search_within)->children.empty()){
                        //this path reached an end, store
                        if(!std::get<1>(*search_within).empty())possibilities_out.push_back(current_path);
                    }
                    else{
                        //append fresh search requests to the back of the list for all children
                        auto child_vec_append = child_vector(*(cont_binary.begin()+new_base_advance));
                        if(!child_vec_append.empty()){
                            for(const auto &tree:child_vec_append){
                                auto current_copy = current_path;
                                current_copy.emplace_back(tree, std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>{},std::get<2>(*search_within),new_base_advance);
                                possibilities_work.push_back(current_copy);
                            }
                        }
                        //also search all the other children
                        for(const auto &heristic:children){
                            if(child_vec_append.empty() && *(cont_binary.begin()+new_base_advance) == std::get<1>(heristic))continue;
                            for(const auto &tree:std::get<0>(heristic)){
                                auto current_copy = current_path;
                                current_copy.emplace_back(tree, std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>{},std::get<2>(*search_within),new_base_advance);
                                possibilities_work.push_back(current_copy);
                            }
                        }
                    }
                }

                poss_beg++;
                possibilities_work.pop_front();
                possibilities_work.sort(max_match_sort);
            }

            if (possibilities_out.empty()) {
                return std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>,std::size_t, std::size_t>>{};
            }

            std::sort(possibilities_out.begin(),possibilities_out.end(),[&sum_match_size](auto &a, auto &b){//sort for biggest match combination
                return sum_match_size(a) > sum_match_size(b);
            });

            auto poss_out = possibilities_out.begin();
            std::size_t max_advance = sum_match_size(*poss_out);
            while(poss_out != possibilities_out.end()){
                if(sum_match_size(*poss_out)<max_advance){
                    possibilities_out.erase(poss_out,possibilities_out.end());
                    break;
                }
                poss_out++;
            }

            std::sort(possibilities_out.begin(),possibilities_out.end(),[](auto &a, auto &b){//sort for bigger matches, not a lot of fragments
                auto sum_match_count = [](std::vector<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>,std::size_t, std::size_t>> &input){
                    return std::accumulate(input.begin(),input.end(),(std::size_t)0,[](std::size_t last_sum,std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>,std::size_t, std::size_t> &item){
                        return last_sum+std::get<2>(item);
                    });
                };
                return sum_match_count(a) < sum_match_count(b);
            });

            return possibilities_out[0];
        }
    };
}

#endif //UHLIBCOMMON_TREE_RADIX_CUSTOM_H
