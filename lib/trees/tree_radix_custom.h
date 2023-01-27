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

    template<class DataReference>
    struct tree_radix_custom {
        std::vector<unsigned char> *data{};
        //any binary vector string
        DataReference *data_ref{};
        //the first element of data is cut off to children except on root if it's a new tree
        //TODO: use two children and block_swarm_offsets to scan forward and backward
        std::vector<std::tuple<std::vector<tree_radix_custom *>, unsigned char>> *children{};//multiple targets that can follow a node for each letter
        std::size_t block_swarm_offset{};//offset of block beginning from root

        tree_radix_custom() {
            data = new std::vector<unsigned char>{};
            data_ref = new DataReference{};
            children = new std::vector<std::tuple<std::vector<tree_radix_custom *>, unsigned char>>{};
        }

        virtual ~tree_radix_custom() {
            delete data;
            delete data_ref;

            for (auto &item1: *children) {
                for (auto &item2: std::get<0>(item1)) {
                    delete item2;
                }
            }
            delete children;
        }

        explicit tree_radix_custom(const std::vector<unsigned char> &bin) : tree_radix_custom() {
            data->assign(bin.begin(), bin.end());
        }

        [[nodiscard]] std::size_t size() const {
            return data->size();
        }

        std::vector<tree_radix_custom *> child_vector(unsigned char i) {
            if (children->empty())return {};
            for (const auto &item: *children) {
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
                children->emplace_back(std::vector<tree_radix_custom *>{input_tree}, first_letter);
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
                                return lexicographical_compare(std::execution::unseq, a->data->cbegin(),
                                                               a->data->cend(),
                                                               b->data->cbegin(), b->data->cend());
                            });
                        } else {
                            std::sort(child_vec_append.begin(), child_vec_append.end(), [](auto &a, auto &b) {
                                return lexicographical_compare(std::execution::unseq, a->data->crbegin(),
                                                               a->data->crend(), b->data->crbegin(), b->data->crend());
                            });
                        }
                        lock.lock();
                        simd_count -= 1;
                        lock.unlock();
                    } else {
                        lock.unlock();
                        if constexpr (!reverse) {
                            std::sort(child_vec_append.begin(), child_vec_append.end(), [](auto &a, auto &b) {
                                return lexicographical_compare(a->data->cbegin(), a->data->cend(), b->data->cbegin(),
                                                               b->data->cend());
                            });
                        } else {
                            std::sort(child_vec_append.begin(), child_vec_append.end(), [](auto &a, auto &b) {
                                return lexicographical_compare(a->data->crbegin(), a->data->crend(), b->data->crbegin(),
                                                               b->data->crend());
                            });
                        }
                    }
                }
            }
            std::sort(children->begin(), children->end(), [](auto &a, auto &b) {
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
            auto item_beg = children->begin();
            while (item_beg != children->end()) {
                if (std::get<1>(*item_beg) == delete_letter) {
                    children->erase(item_beg);
                    return true;
                }
            }
            return false;
        }

        /*std::vector<std::tuple<std::vector<unsigned char>::const_iterator,std::vector<unsigned char>::const_iterator,std::vector<unsigned char>::const_iterator>>*/
        template<class ContainerData,class ContainerBinary,bool reverse=false>
        auto
        compare_ultihash(ContainerData &data_cont, ContainerBinary &binary_cont) {
            //if input does only fit to a shorter string as a subset of data, count becomes negative, else positive including ß
            //data offset iterator and start and end of input
            std::vector<std::tuple<std::size_t, std::size_t>> matches{};//data offset beginning found, end offset from beginning
            if (data_cont.empty() || binary_cont.empty())return matches;
            std::size_t current_offset = 0;
            //search forward through data
            if constexpr (!reverse){
                do {
                    //first element match
                    std::unique_lock lock(simd_protect);
                    if (simd_count < SIMD_UNITS) {
                        simd_count += 1;
                        lock.unlock();
                        current_offset = std::distance(data_cont.begin(),std::find(std::execution::unseq, data_cont.begin()+current_offset, data_cont.end(), *binary_cont.begin()));
                        lock.lock();
                        simd_count -= 1;
                        lock.unlock();
                    } else {
                        lock.unlock();
                        current_offset = std::distance(data_cont.begin(),std::find(data_cont.begin()+current_offset, data_cont.end(), *binary_cont.begin()));
                    }

                    //search how long input matches
                    std::pair<decltype(data_cont.begin()), decltype(data_cont.end())> found;
                    lock.lock();
                    if (simd_count < SIMD_UNITS) {
                        simd_count += 1;
                        lock.unlock();
                        found = std::mismatch(std::execution::unseq, data_cont.begin()+current_offset, data_cont.end(), binary_cont.begin(), binary_cont.end());
                        lock.lock();
                        simd_count -= 1;
                        lock.unlock();
                    } else {
                        lock.unlock();
                        found = std::mismatch(data_cont.begin()+current_offset, data_cont.end(), binary_cont.begin(), binary_cont.end());
                    }

                    if (std::distance(data_cont.begin(),found.first) >= MINIMUM_MATCH_SIZE) {
                        matches.emplace_back(current_offset, current_offset + std::distance(data_cont.begin(),found.first));
                    }
                    if (data_cont.begin()+current_offset != data_cont.end())current_offset++;
                } while (data_cont.begin()+current_offset != data_cont.end());
            }
            else{
                do {
                    //first element match
                    std::unique_lock lock(simd_protect);
                    if (simd_count < SIMD_UNITS) {
                        simd_count += 1;
                        lock.unlock();
                        current_offset = std::distance(data_cont.rbegin(),std::find(std::execution::unseq, data_cont.rbegin()+current_offset, data_cont.rend(), *binary_cont.begin()));
                        lock.lock();
                        simd_count -= 1;
                        lock.unlock();
                    } else {
                        lock.unlock();
                        current_offset = std::distance(data_cont.rbegin(),std::find(data_cont.rbegin()+current_offset, data_cont.rend(), *binary_cont.begin()));
                    }

                    //search how long input matches
                    std::pair<decltype(data_cont.rbegin()), decltype(data_cont.rend())> found;
                    lock.lock();
                    if (simd_count < SIMD_UNITS) {
                        simd_count += 1;
                        lock.unlock();
                        found = std::mismatch(std::execution::unseq, data_cont.rbegin()+current_offset, data_cont.rend(), binary_cont.begin(), binary_cont.end());
                        lock.lock();
                        simd_count -= 1;
                        lock.unlock();
                    } else {
                        lock.unlock();
                        found = std::mismatch(data_cont.rbegin()+current_offset, data_cont.rend(), binary_cont.begin(), binary_cont.end());
                    }

                    if (std::distance(data_cont.rbegin(),found.first) >= MINIMUM_MATCH_SIZE) {
                        matches.emplace_back(current_offset, current_offset + std::distance(data_cont.rbegin(),found.first));
                    }
                    if (data_cont.rbegin()+current_offset != data_cont.rend())current_offset++;
                } while (data_cont.rbegin()+current_offset != data_cont.rend());
            }

            return matches;
        }

    public:

        //returns total size integrated, new space used uncompressed, new space used compressed, list of tree references of <offset_ELEMENT,modified_LIST,added_LIST> tree nodes
        template<class ContainerBinary,bool reverse=false>
        std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::list<std::tuple<std::set<tree_radix_custom *>, std::set<tree_radix_custom *>>>>>
        add(ContainerBinary &binary_cont) {//TODO:check duplicate matches and eliminate
            //first search existing structure and add into the last tree to insert potentially missing information
            /*std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::vector<unsigned char>::const_iterator, std::vector<unsigned char>::const_iterator, std::vector<unsigned char>::const_iterator>>>>>, std::size_t>>*/

            //uncompressed input
            if (binary_cont.empty()) {
                return {};
            }
            auto search_index = search<ContainerBinary,reverse>(binary_cont);

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
                    std::vector<std::tuple<std::size_t, std::size_t, tree_radix_custom *>> actively_changing_trees{};//data offset and binary range from offset
                    std::for_each(std::get<1>(*one_node_analysis).begin(), std::get<1>(*one_node_analysis).end(),
                                  [&actively_changing_trees, &tree_match_pointer](auto &item) {
                                      actively_changing_trees.emplace_back(std::get<0>(item), std::get<1>(item), tree_match_pointer);
                                  });

                    auto match_beg = actively_changing_trees.begin();//                                         found beginning                             found end                        data on tree begin at found begin
                    auto match_beg_copy = match_beg;
                    while (match_beg !=
                           actively_changing_trees.end()) {//update loop on std::vector<std::tuple<std::vector<unsigned char>::const_iterator, std::vector<unsigned char>::const_iterator, std::vector<unsigned char>::const_iterator>>

                        std::size_t tree_front_data_front_absolute = std::get<0>(*match_beg);

                        ///////////////////////////////////////////////Tree building sequence////////////////////////////////////////////
//checking if children need to be generated before and after the found input peace, reference to data of tree required
                        //child before found, reference data
                        std::vector<unsigned char> child_beg,child_mid,child_end,child_append;

                        if constexpr (!reverse) {
                            child_beg = std::vector<unsigned char>{std::get<2>(*match_beg)->data->cbegin(),std::max(std::get<2>(*match_beg)->data->cbegin()+((long)std::get<0>(*match_beg)-1), std::get<2>(*match_beg)->data->cbegin())};
                            //child data sequence middle, reference data
                            child_mid = std::vector<unsigned char>{std::get<2>(*match_beg)->data->cbegin()+std::get<1>(*match_beg),std::min(std::get<2>(*match_beg)->data->cend(),std::get<2>(*match_beg)->data->cbegin()+std::get<1>(*match_beg)+std::get<0>(*match_beg))};
                            //child data sequence end, reference data
                            child_end = std::vector<unsigned char>{std::min(std::get<2>(*match_beg)->data->cend(), std::min(std::get<2>(*match_beg)->data->cend(),std::get<2>(*match_beg)->data->cbegin()+std::get<1>(*match_beg)+std::get<0>(*match_beg)) + 1),std::get<2>(*match_beg)->data->cend()};
                        } else {
                            child_beg = std::vector<unsigned char>{std::get<2>(*match_beg)->data->crbegin(),std::max(std::get<2>(*match_beg)->data->crbegin()+((long)std::get<0>(*match_beg)-1), std::get<2>(*match_beg)->data->crbegin())};
                            //child data sequence middle, reference data
                            child_mid = std::vector<unsigned char>{std::get<2>(*match_beg)->data->crbegin()+std::get<1>(*match_beg),std::min(std::get<2>(*match_beg)->data->crend(),std::get<2>(*match_beg)->data->crbegin()+std::get<1>(*match_beg)+std::get<0>(*match_beg))};
                            //child data sequence end, reference data
                            child_end = std::vector<unsigned char>{std::min(std::get<2>(*match_beg)->data->crend(), std::min(std::get<2>(*match_beg)->data->crend(),std::get<2>(*match_beg)->data->crbegin()+std::get<1>(*match_beg)+std::get<0>(*match_beg)) + 1),std::get<2>(*match_beg)->data->crend()};
                        }
                        //child after found, reference new input
                        child_append = std::vector<unsigned char>{std::min(binary_cont.begin()+std::get<1>(*match_beg)+1,binary_cont.end()),binary_cont.end()};
                        //only the new append part may be compressed
                        //before splitting or modifying a block it needs to be uncompressed

                        bool first_section_tree = !child_beg.empty();
                        bool last_section_tree = !child_end.empty();

                        bool append_tree = !child_append.empty();
                        bool total_match = child_mid.size() == std::get<2>(*match_beg)->data->size();

                        uh::util::compression_custom comp{};
                        auto out_vector = std::vector<tree_radix_custom *>{};

                        //search function already determined that this is the tree that needs to fill in the data or to split somehow
                        //cases: insert front tree, insert cend tree (same case as having a back insert because the cend tree will just have at least 1 element in case of overflow)
                        if (std::get<2>(*match_beg)->data->empty()) {//how to insert, either empty simple insert or some tree construction anywhere
                            //simple insert into data since this seems to be a new node that can contain simple information

                            std::get<2>(*match_beg)->data->assign(binary_cont.begin(), binary_cont.end());
                            out_vector.push_back(std::get<2>(*match_beg));
                            std::get<0>(out_change_tuple) += std::get<1>(*match_beg);
                            std::get<1>(out_change_tuple) += std::get<1>(*match_beg);
                            std::get<2>(out_change_tuple) += comp.compress(binary_cont).size();
                        } else {
                            std::size_t size_integrated{}, size_compressed{}, size_uncompressed{};
                            if (total_match) {//only a maximum of 1 tree creation or just 0 in case of reference
                                //a total match can still have appending structure
                                out_vector.push_back(std::get<2>(*match_beg));
                                size_integrated += std::get<2>(*match_beg)->data->size();
                                if (append_tree) {
                                    auto *tree_ptr_tmp = new tree_radix_custom(child_append);
                                    if constexpr (!reverse){
                                        size_compressed = comp.compress(tree_ptr_tmp->data).size();
                                    }
                                    else{
                                        auto tmp_vec = *tree_ptr_tmp->data;
                                        std::reverse(tmp_vec.begin(),tmp_vec.end());
                                        size_compressed = comp.compress(tmp_vec).size();
                                    }

                                    size_uncompressed += tree_ptr_tmp->data->size();
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
                                    *tree_ptr_mid->children = *std::get<2>(*match_beg)->children;
                                    std::get<2>(*match_beg)->children->clear();
                                    *tree_ptr_mid->data_ref = *std::get<2>(*match_beg)->data_ref;
                                    tree_ptr_first->data_ref->clear();

                                    size_integrated += child_beg.size();
                                    //try to add the reference entry to middle tree on first tree
                                    tree_ptr_first->child_put(tree_ptr_mid, *child_mid.begin());
                                } else {
                                    //the current tree stays fundament
                                    tree_ptr_mid = std::get<2>(*match_beg);
                                    size_integrated += child_mid.size();
                                }
                                out_vector.push_back(tree_ptr_mid);

                                tree_radix_custom *tree_ptr_last;
                                if (last_section_tree) {
                                    //create last tree
                                    tree_ptr_last = new tree_radix_custom(child_end);
                                    //transfer information of middle tree to last tree and copy the children also to append tree in case it exists
                                    out_vector.push_back(tree_ptr_last);
                                    tree_ptr_last->block_swarm_offset =
                                            tree_ptr_mid->block_swarm_offset + 1;
                                    *tree_ptr_last->children = *tree_ptr_mid->children;
                                    tree_ptr_mid->children->clear();
                                    *tree_ptr_last->data_ref = *tree_ptr_mid->data_ref;
                                    tree_ptr_mid->data_ref->clear();

                                    size_integrated += child_end.size();
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
                                            tree_ptr_first->data->erase(tree_ptr_first->data->begin()+child_beg.size(), tree_ptr_first->data->begin()+child_beg.size()+child_mid.size()+child_end.size());
                                        } else {
                                            tree_ptr_first->data->erase(tree_ptr_first->data->begin(), tree_ptr_first->data->begin()+child_mid.size()+child_end.size());
                                        }
                                    } else {
                                        //delete middle data reference size from tree pointer first
                                        if constexpr (!reverse) {
                                            tree_ptr_first->data->erase(tree_ptr_first->data->begin()+child_beg.size(), tree_ptr_first->data->begin()+child_beg.size()+child_mid.size());
                                        } else {
                                            tree_ptr_first->data->erase(tree_ptr_first->data->begin(), tree_ptr_first->data->begin()+child_mid.size());
                                        }
                                    }
                                } else {
                                    if (last_section_tree) {
                                        //delete last tree reference size from tree pointer middle
                                        if constexpr (!reverse) {
                                            tree_ptr_mid->data->erase(tree_ptr_mid->data->begin()+child_mid.size(),tree_ptr_mid->data->begin()+child_mid.size()+child_end.size());
                                        } else {
                                            tree_ptr_mid->data->erase(tree_ptr_mid->data->begin(),tree_ptr_mid->data->begin()+child_end.size());
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
                        if (total_match) {
                            if (append_tree) {//DO NOT "DEBUG"
                                added.emplace(*append_tree_out);
                            }
                            //this must be the only match so nothing happens
                        } else {
                            //stack helper function for overlapping creation of trees
                            auto overlap_update = [](std::size_t tree_front_data_front_absolute,
                                                             auto &actively_changing_trees, auto match_beg_intern_copy,
                                                             auto match_end_intern, tree_radix_custom *tree_front,
                                                             tree_radix_custom *tree_back) {
                                auto match_beg_intern = match_beg_intern_copy;
                                if (match_beg_intern + 1 >= match_end_intern)return;
                                //update matches offset
                                auto first_data_offset = std::get<0>(*match_beg_intern);
                                std::size_t first_match_size = std::get<1>(*match_beg_intern);
                                std::size_t first_end = tree_front_data_front_absolute + first_match_size;
                                while (match_beg_intern < match_end_intern) {
                                    std::size_t data_offset_between_start_and_current = std::get<0>(*(match_beg_intern +1))-first_data_offset;
                                    std::size_t total_offset_front_for_this_node =
                                            tree_front_data_front_absolute + data_offset_between_start_and_current;
                                    if (data_offset_between_start_and_current < first_end) {
                                        //overlapping match, tree pointer stays at this tree, one to one reference tree front at tree_front_data_front_absolute; re-reference to data due to erase
                                        std::size_t match_size = std::get<1>(*(match_beg_intern + 1));
                                        std::size_t data_offset_between_start_and_last_node = std::get<0>(*(match_beg_intern))-first_data_offset;
                                        std::size_t total_offset_front_for_last_node = tree_front_data_front_absolute +
                                                                                       data_offset_between_start_and_last_node;
                                        if (data_offset_between_start_and_current + match_size >= first_end) {
                                            std::size_t match_size_last = std::get<1>(*(match_beg_intern));
                                            std::size_t total_offset_end_for_last_node =
                                                    total_offset_front_for_last_node + match_size_last;

                                            //recursive overlap reconstruction
                                            std::get<1>(*(match_beg_intern + 1)) -= total_offset_end_for_last_node -
                                                                                    (total_offset_front_for_this_node +
                                                                                     match_size);
                                            std::size_t new_match_size = std::get<1>(*(match_beg_intern + 1));//first tree match
                                            auto new_match_begin = std::get<2>(*(match_beg_intern + 1))->data->begin()+std::get<0>(*(match_beg_intern + 1))+std::get<1>(*(match_beg_intern + 1))+1;
                                            auto new_match_end = new_match_begin + (match_size - new_match_size);
                                            //std::get<2>(*(match_beg_intern+1)) = tree_front->data->begin()+total_offset_end_for_last_node;
                                            //std::get<3>(*(match_beg_intern+1)) = tree_front;

                                            if constexpr (!reverse) {
                                                /*
                                                auto new_partial_match = std::make_tuple(new_match_begin,
                                                                                         new_match_end,
                                                                                         tree_front->data->cbegin() +
                                                                                         (total_offset_front_for_this_node -
                                                                                          (tree_front->data->size() -
                                                                                           new_match_size)),
                                                                                         tree_front);//second tree match*/
                                                auto new_partial_match = std::make_tuple((std::size_t)std::distance(std::get<2>(*(match_beg_intern + 1))->data->begin(),new_match_begin),
                                                                                         (std::size_t)std::distance(new_match_begin,new_match_end),
                                                                                                          tree_front);

                                                // update until end
                                                auto match_insert_check = match_beg_intern;
                                                while (std::get<0>(*match_insert_check) < std::get<0>(new_partial_match) &&
                                                       match_insert_check != match_end_intern){
                                                    match_insert_check++;
                                                }
                                                actively_changing_trees.insert(match_insert_check, new_partial_match);
                                            } else {
                                                auto new_partial_match = std::make_tuple(new_match_begin,
                                                                                         new_match_end,
                                                                                         tree_front->data->crbegin() +
                                                                                         (total_offset_front_for_this_node -
                                                                                          (tree_front->data->size() -
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

                                            active_tree_offset = std::distance(actively_changing_trees.begin(),
                                                                               match_beg_intern_copy);
                                            match_beg_intern =
                                                    actively_changing_trees.begin() + active_tree_offset +
                                                    std::distance(match_beg_intern_copy, match_beg_intern);
                                            match_beg_intern_copy =
                                                    actively_changing_trees.begin() + active_tree_offset;
                                        } else {
                                            //simple build in tree front
                                            std::get<0>(*(match_beg_intern + 1)) = total_offset_front_for_this_node/*-distance from the last node until the current node start*/;
                                            //std::get<3>(*(match_beg_intern+1)) = tree_front;
                                        }
                                    } else {
                                        //total match, new calculated reference on tree_back
                                        std::get<0>(*(match_beg_intern + 1)) = total_offset_front_for_this_node -
                                                                               tree_front->data->size();
                                        std::get<2>(*(match_beg_intern + 1)) = tree_back;
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

        //returns total size integrated, new space used uncompressed, new space used compressed
        //calculates ESTIMATE size of data to be integrated to be communicated to agency to determine optimal storage location
        template<class ContainerBinary,bool reverse = false>
        std::vector<std::tuple<std::size_t, std::size_t, std::size_t>>
        add_test(ContainerBinary &cont_binary) {//add test should copy
            //first search existing structure and add into the last tree to insert potentially missing information

            //uncompressed input
            if (cont_binary.empty()) {
                return {};
            }
            auto search_index = search<ContainerBinary,reverse>(cont_binary);

            //cases for search index: its empty or it has content and with that a last tree element
            //cases for last tree if it exists, binary fit in: match from the beginning on, match in the middle, match until the end, total match
            //all lists contain lists with a last element that had multiple matches; add up all matches
            //std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<decltype(bin_beg), decltype(bin_end), decltype(bin_beg)>>>>>, std::size_t, decltype(bin_end), decltype(bin_beg)>>
            uh::util::compression_custom comp{};
            std::vector<std::tuple<std::size_t, std::size_t, std::size_t>> add_tup_out{};
            if (search_index.empty()) {
                std::tuple<std::size_t, std::size_t, std::size_t> add_tup{};
                std::get<0>(add_tup) += cont_binary.size();
                std::get<1>(add_tup) += cont_binary.size();
                std::get<2>(add_tup) += comp.compress(cont_binary).size();
                add_tup_out.push_back(add_tup);
                return add_tup_out;
            }

            for (auto &single_route: search_index) {
                std::tuple<std::size_t, std::size_t, std::size_t> add_tup{};

                tree_radix_custom *last_tree;
                for (auto &list: std::get<0>(single_route)) {
                    for (auto &tree_tuple: list) {
                        last_tree = std::get<0>(tree_tuple);
                        for (auto &pos_tup: std::get<1>(tree_tuple)) {
                            //auto add_list = tree_test_sequence(std::get<0>(tree_tuple), bin_beg, bin_beg,std::get<0>(pos_tup),
                            //                                   std::get<1>(pos_tup), std::get<2>(pos_tup));//insert into another tree
                            if (std::get<0>(
                                    tree_tuple)->data->empty()) {//how to insert, either empty simple insert or some tree construction anywhere
                                //simple insert into data since this seems to be a new node that can contain simple information
                                auto set_vector = std::vector<unsigned char>{cont_binary.begin()+std::get<0>(pos_tup),cont_binary.begin()+std::get<0>(pos_tup)+std::get<1>(pos_tup)};

                                std::get<0>(add_tup) += std::get<1>(pos_tup);
                                std::get<1>(add_tup) += std::get<1>(pos_tup);
                                std::get<2>(add_tup) += comp.compress(set_vector).size();
                            } else {
                                std::get<0>(add_tup) += std::get<1>(pos_tup);
                            }
                        }
                    }
                }
                if (cont_binary.begin() + std::get<0>(add_tup) < cont_binary.end()) {
                    auto set_vector = std::vector<unsigned char>{cont_binary.begin() + std::get<0>(add_tup),cont_binary.end()};
                    std::get<2>(add_tup) += comp.compress(set_vector).size();
                    std::get<1>(add_tup) += set_vector.size();
                    std::get<0>(add_tup) += set_vector.size();
                }
                add_tup_out.push_back(add_tup);
            }

            return add_tup_out;
        }

        //returns std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<decltype(bin_beg), decltype(bin_end), decltype(bin_beg)>>>>>, std::size_t>>
        template<class ContainerData,class ContainerBinary,bool reverse = false>
        auto
        search_match_filter(ContainerData &data_cont, ContainerBinary &binary_cont,
                            std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t>>>>>, std::size_t,std::size_t>> possibilities =
                            std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t>>>>>, std::size_t,std::size_t>>{}) {
            auto local_matches = compare_ultihash<ContainerData,ContainerBinary,reverse>(data_cont,binary_cont);

            auto legal_match_integration = [&data_cont](auto local_matches) {
                std::sort(local_matches.begin(), local_matches.end(), [](auto &a, auto &b) {
                    return std::get<1>(a) < std::get<1>(b);
                });
                //LEGAL MATCH FILTER
                auto match_beg = local_matches.begin();

                auto legal_check = [&local_matches,&data_cont](auto &match_beg,std::size_t start_val, std::size_t end_val) {
                    //on empty or partial match make new list in list, else append the match results on total match
                    bool legal_split;
                    //if the end was matched too long we can do something about that, but else the algorithm is prefix oriented
                    bool start_size, end_size, begin_reached, end_reached, total_found_size;
                    do {

                        start_size = std::get<0>(*match_beg) <= (long)start_val-MINIMUM_MATCH_SIZE || start_val+MINIMUM_MATCH_SIZE <= std::get<0>(*match_beg);
                        end_size = data_cont.size()-(std::get<0>(*match_beg)+std::get<1>(*match_beg)) <= (long)end_val-MINIMUM_MATCH_SIZE <= (long)start_val-MINIMUM_MATCH_SIZE || MINIMUM_MATCH_SIZE <= data_cont.size()-(std::get<0>(*match_beg)+std::get<1>(*match_beg));
                        total_found_size = MINIMUM_MATCH_SIZE <= std::get<1>(*match_beg);
                        begin_reached = std::get<0>(*match_beg) == start_val;
                        end_reached = data_cont.size()-(std::get<0>(*match_beg)+std::get<1>(*match_beg)) == end_val;
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
                    } while (((!end_size && !end_reached) || (!start_size && !begin_reached)) &&
                             !local_matches.empty());

                    return legal_split;
                };

                while (match_beg != local_matches.end()) {//shrink all matches until the total result is legal, shrink smaller matches first until they vanish
                    bool legal_general = legal_check(match_beg,0,data_cont.size());
                    if(!legal_general){
                        local_matches.erase(match_beg);
                        match_beg = local_matches.begin();
                        continue;
                    }
                    auto match_begin_legal_shift = local_matches.begin();
                    while (match_begin_legal_shift != local_matches.end()) {
                        bool legal = legal_check(match_begin_legal_shift,std::get<0>(*match_beg),std::get<0>(*match_beg)+std::get<1>(*match_beg));
                        if(!legal){
                            local_matches.erase(match_begin_legal_shift);
                            match_begin_legal_shift = local_matches.begin();
                        }
                        else match_begin_legal_shift++;
                    }
                    match_beg++;
                }

                std::sort(local_matches.begin(), local_matches.end(), [](auto &a, auto &b) {
                    return std::get<1>(a) > std::get<1>(b);
                });

                if (local_matches.size() > 1) {
                    std::size_t max_fit{};
                    auto best_beg = local_matches.begin();
                    while (best_beg != local_matches.end()) {
                        max_fit = std::max(max_fit,std::get<0>(*best_beg));
                        if (std::get<0>(*best_beg) < max_fit) {
                            //break and delete until end
                            break;
                        }
                        best_beg++;
                    }
                    local_matches.erase(best_beg, local_matches.end());
                }
                //sort the smallest offset out of the largest match results in case the match sizes are equal
                std::sort(local_matches.begin(), local_matches.end(), [](auto &a, auto &b) {
                    return std::get<0>(a) < std::get<0>(b);
                });
                return local_matches;
            };

            if (local_matches.empty())return possibilities;

            local_matches = legal_match_integration(local_matches);

            decltype(possibilities) out_possibilities;

            auto match_beg = local_matches.begin();
            auto possibilities_manage = [&](auto &input_list_tmp,auto &found_vec) {
                //continue match stream
                if (std::get<0>(input_list_tmp).empty()) {
                    std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t,std::size_t>>>> tmp_list{};
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
                std::size_t advance = std::get<1>(*match_beg);
                std::get<1>(input_list_tmp)++;
                std::get<2>(input_list_tmp) += advance;//binary advance

                out_possibilities.push_back(input_list_tmp);
            };

            while (match_beg != local_matches.end()) {
                std::vector<std::tuple<std::size_t,std::size_t>> found_vec{};
                found_vec.emplace_back(std::get<0>(*match_beg),std::get<1>(*match_beg));
                if (possibilities.empty()) {
                    //new start of possibilities
                    std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t,std::size_t>>>> tmp_list{};
                    tmp_list.emplace_back(this, found_vec);
                    std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t,std::size_t>>>>> outer_list{
                            tmp_list};

                    std::size_t advance = std::get<1>(*match_beg);
                    out_possibilities.emplace_back(outer_list, 0, advance);
                } else
                    for (auto &input_list_tmp: possibilities) {//COPY input list and create different path calculation
                        possibilities_manage(input_list_tmp,found_vec);
                    }
                match_beg++;
            }
            return out_possibilities;
        }

        //returns the path of maximum fit and the match size
        /*std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::vector<unsigned char>::const_iterator, std::vector<unsigned char>::const_iterator, std::vector<unsigned char>::const_iterator>>>>>, std::size_t>>*/
        template<class ContainerBinary,bool reverse=false>
        auto search(ContainerBinary &cont_binary,//                                                                                                  data offset,binary offset after node integration
                    std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t>>>>>, std::size_t,std::size_t>> possibilities =
                    std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::size_t, std::size_t>>>>>, std::size_t,std::size_t>>{},
                    std::vector<tree_radix_custom *> limiter_children = std::vector<tree_radix_custom *>{}) {

            if (cont_binary.empty() ||
                std::any_of(limiter_children.begin(), limiter_children.end(), [this](auto &item_limit) {
                    return item_limit == this;
                })) {
                return possibilities;
            }

            /*std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<decltype(bin_beg), decltype(bin_end), decltype(bin_beg)>>>>>, std::size_t,decltype(bin_end), decltype(bin_beg)>>*/
            //while a possibility still changes on search_match filter, it is continued to be executed
            possibilities = search_match_filter<std::vector<unsigned char>,ContainerBinary,reverse>(*data, cont_binary, possibilities);

            decltype(possibilities) new_recursive{};

            auto single_pos = possibilities.begin();
            std::size_t single_count{};
            while (single_pos != possibilities.end()) {
                if constexpr (!reverse) {
                    if (data->begin()+std::get<1>(*single_pos) == data->end() || cont_binary.begin()+std::get<2>(*single_pos) == cont_binary.end()) {
                        single_count++;
                        single_pos++;
                        continue;
                    }
                } else {
                    if (data->rbegin()+std::get<1>(*single_pos) == data->rend() || cont_binary.begin()+std::get<2>(*single_pos) == cont_binary.end()) {
                        single_count++;
                        single_pos++;
                        continue;
                    }
                }
                decltype(possibilities) tmp;
                std::vector<unsigned char> binary_subset{cont_binary.begin()+std::get<2>(*single_pos), cont_binary.end()};
                if constexpr (!reverse) {
                    std::vector<unsigned char> data_subset{data->begin()+std::get<1>(*single_pos), data->end()};
                    tmp = search_match_filter<decltype(*data),ContainerBinary,reverse>(data_subset, binary_subset ,possibilities);
                } else {
                    std::vector<unsigned char> data_subset{data->begin(), data->end()-(std::get<1>(*single_pos)+1)};
                    tmp = search_match_filter<decltype(*data),ContainerBinary,reverse>(data_subset, binary_subset ,possibilities);
                }
                std::for_each(tmp.begin(), tmp.end(), [&new_recursive](auto &item1) {
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
            for (auto &pos_begin: possibilities) {
                if constexpr (!reverse){
                    if (data->begin()+std::get<1>(pos_begin)==data->end()||cont_binary.begin()+std::get<2>(pos_begin) == cont_binary.end()) {
                        continue;
                    }
                }
                else{
                    if (data->rbegin()+std::get<1>(pos_begin)==data->rend()||cont_binary.begin()+std::get<2>(pos_begin) == cont_binary.end()) {
                        continue;
                    }
                }
                std::vector<unsigned char> binary_subset{cont_binary.begin()+std::get<2>(pos_begin), cont_binary.end()};
                bool total_match = false;
                auto child_vec = child_vector(*(cont_binary.begin()+std::get<2>(pos_begin)));
                if (!child_vec.empty()) {//recursive search
                    for (auto &item: child_vec) {//vector of tree pointers
                        auto tmp = item->template search<ContainerBinary,reverse>(binary_subset, possibilities);
                        if (std::any_of(tmp.begin(), tmp.end(), [&cont_binary](auto &item) {
                            return cont_binary.begin()+std::get<2>(item) == cont_binary.end();
                        }))total_match = true;
                        if (tmp != possibilities) {
                            std::for_each(tmp.begin(), tmp.end(), [&new_recursive](auto &item1) {
                                if (std::none_of(new_recursive.begin(), new_recursive.end(), [&item1](auto &item2) {
                                    return item2 == item1;
                                })) {
                                    new_recursive.push_back(item1);
                                }
                            });
                        }
                    }
                }

                if (total_match)break;
                //do not search other children if a directed match has been found --> fast

                //slow search
                for (auto &c: *children) {
                    if (!child_vec.empty() && std::get<1>(c) == *(cont_binary.begin()+std::get<2>(pos_begin)))continue;
                    for (auto &item: std::get<0>(c)) {//vector of tree pointers
                        if (cont_binary.begin()+std::get<2>(pos_begin) == cont_binary.end()) {
                            continue;
                        }
                        auto tmp = item->template search<ContainerBinary,reverse>(binary_subset, possibilities);
                        if (tmp != possibilities) {
                            std::for_each(tmp.begin(), tmp.end(), [&new_recursive](auto &item1) {
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

            std::for_each(new_recursive.begin(), new_recursive.end(), [&possibilities](auto &item1) {
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
