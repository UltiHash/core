//
// Created by benjamin-elias on 17.12.22.
//

#ifndef UHLIBCOMMON_TREE_RADIX_CUSTOM_H
#define UHLIBCOMMON_TREE_RADIX_CUSTOM_H

#include "logging/logging_boost.h"
#include "trees/tree_storage_config.h"
#include "util/compression_custom.h"
#include <vector>
#include <ranges>
#include <algorithm>

namespace uh::trees {
    //because it takes at least 2 bytes to describe a deeper encoding action
#define MINIMUM_MATCH_SIZE 3
    typedef struct tree_radix_custom tree_radix_custom;

    struct tree_radix_custom {
    protected:
        //the first element of data is cut off to children except on root if it's a new tree
        std::vector<std::tuple<std::vector<tree_radix_custom *>, unsigned char>> children{};//multiple targets that can follow a node for each letter
        std::vector<unsigned char> data{};//any binary vector string
    public:
        tree_radix_custom() = default;

        ~tree_radix_custom() {
            for (auto &item1: children) {
                for (auto &item2: std::get<0>(item1)) {
                    if (item2 != nullptr) delete item2;
                }
            }
        }

        explicit tree_radix_custom(std::vector<unsigned char> &bin) : tree_radix_custom() {
            add(bin);
        }

        template<typename IteratorType>
        tree_radix_custom(IteratorType beg, IteratorType end) : tree_radix_custom() {
            add(beg, end);
        }

        [[nodiscard]] std::size_t size() const {
            return data.size();
        }

        std::vector<unsigned char> &data_vector() {
            return data;
        }

        std::vector<tree_radix_custom *> &child_vector(unsigned char i) {
            if (children.empty())return {};
            for (const auto &item: children) {
                if (std::get<1>(item) == i) {
                    return std::get<0>(item);
                }
            }
            return {};
        }

    private:
        auto compare_ultihash(auto data_beg, auto data_end, auto input_beg, auto input_end) {
            if (std::distance(input_beg, input_end) > std::distance(data_beg, data_end))
                input_end = input_beg + std::distance(data_beg, data_end);
            //if input does only fit to a shorter string as a subset of data, count becomes negative, else positive including ß
            //data offset iterator and start and end of input
            std::vector<std::tuple<decltype(data_beg), decltype(input_beg), decltype(input_end)>> matches{};
            if (std::distance(data_beg, data_end) > std::distance(input_beg, input_end)) {
                DEBUG << "Compare ultihash failed because input was larger than data!";
                return matches;
            }
            if (!std::distance(data_beg, data_end)) {
                DEBUG << "Compare ultihash failed because data was empty!";
                return matches;
            }
            if (!std::distance(input_beg, input_end)) {
                DEBUG << "Compare ultihash failed because data was empty!";
                return matches;
            }
            //advance search scope over data
            //increase data start to the beginning of the input match +1
            decltype(input_end) input_beg_tmp, re_enter;
            //search forward through data
            do {
                bool re_enter_hit = false;
                input_beg_tmp = input_beg;//increase end of input to possibly find a prefix match
                //first element match
                while (*data_beg != *input_beg_tmp && data_beg != data_end) {
                    data_beg++;
                }
                if (data_beg == data_end || input_beg_tmp == input_end)break;
                //search how long input matches
                auto data_beg_tmp = data_beg;
                bool broken = false;
                do {
                    if (*input_beg_tmp != *data_beg_tmp) {
                        broken = true;
                        break;
                    }
                    input_beg_tmp++;
                    data_beg_tmp++;

                    if (!re_enter_hit) {
                        if (*input_beg_tmp != *input_beg)re_enter_hit = true;
                        else re_enter = data_beg + std::distance(input_beg, input_beg_tmp);
                    }
                } while (input_beg_tmp != input_end);
                re_enter = std::max(data_beg + 1, re_enter);
                //last input count reversed
                if (std::distance(input_beg, input_end_tmp - broken) > MINIMUM_MATCH_SIZE) {
                    matches.emplace_back(data_beg, input_beg, input_end_tmp - broken);
                }
                data_beg = re_enter;
            } while (data_beg != data_end);

            return matches;
        }

    public:
        template<class ContainerType>
        std::tuple<std::size_t, std::size_t, std::size_t, std::list<tree_radix_custom *>> add(const ContainerType &c) {
            return add(c.begin(), c.end());
        }

        //returns total size integrated, new space used uncompressed, new space used compressed, reference tree list
        template<typename IteratorIn>
        std::tuple<std::size_t, std::size_t, std::size_t, std::list<tree_radix_custom *>>
        add(IteratorIn bin_beg, IteratorIn bin_end,
            std::tuple<std::size_t, std::size_t, std::size_t, std::list<tree_radix_custom *>> input_list =
            std::tuple<std::size_t, std::size_t, std::size_t, std::list<tree_radix_custom *>>{}) {
            //uncompressed input
            if (bin_beg == bin_end || std::distance(bin_beg, bin_end) < 1)
                return input_list;//some element and an end element at least required

            auto tree_test_sequence = [&input_list](tree_radix_custom *cur_tree, auto bin_beg_incoming,
                                                    auto bin_end_incoming, auto bin_beg_found, auto bin_end_found,
                                                    const std::vector<unsigned char>::iterator data_beg_intern) {
                std::size_t matched_size = std::distance(bin_beg_incoming, bin_end_found);
                //checking if children need to be generated before and after the found input peace, reference to data of tree required
                //child before found, reference data
                auto child_beg_beg = cur_tree->data_vector().begin();
                auto child_end_beg = std::max(data_beg_intern - 1, child_beg_beg);
                //child data sequence middle, reference data
                auto child_beg_mid = data_beg_intern;
                auto child_end_mid = std::min(cur_tree->data_vector().end(),
                                              child_beg_mid + std::distance(bin_beg_found, bin_end_found));
                //child data sequence end, reference data
                auto child_beg_end = std::min(cur_tree->data_vector().end(), child_end_mid + 1);
                auto child_end_end = cur_tree->data_vector().end();
                //child after found, reference new input
                auto child_beg_append = std::min(bin_end_found + 1, bin_end_incoming);
                auto child_end_append = bin_end_incoming;
                //only the new append part may be compressed
                //before splitting or modifying a block it needs to be uncompressed

                bool first_section_tree = std::distance(child_beg_beg, child_end_beg) > 1;
                bool last_section_tree =
                        child_beg_end == child_end_end && child_end_end == cur_tree->data_vector().end();
                bool append_tree =
                        std::distance(child_beg_append, child_end_append) > 0 && child_beg_append != bin_end_incoming;
                bool total_match = !first_section_tree && !last_section_tree &&
                                   std::distance(child_beg_mid, child_end_mid) == cur_tree->data_vector().size();

                auto out_list = std::list<tree_radix_custom *>{cur_tree};

                //search function already determined that this is the tree that needs to fill in the data or to split somehow
                //cases: insert front tree, insert end tree (same case as having a back insert because the end tree will just have at least 1 element in case of overflow)
                if (cur_tree->data_vector().empty()) {//how to insert, either empty simple insert or some tree construction anywhere
                    //simple insert into data since this seems to be a new node that can contain simple information
                    cur_tree->data_vector() = std::vector<unsigned char>{bin_beg_found, bin_end_found};
                    return std::make_tuple(std::distance(bin_beg, bin_end), std::distance(bin_beg, bin_end),
                                           uh::util::compression_custom::compress(bin_beg_found, bin_end_found).size(),
                                           out_list);
                } else {
                    if (total_match) {//only a maximum of 1 tree creation or just 0 in case of reference
                        //a total match can still have appending structure
                        if (append_tree) {
                            //either find child is empty and test add tree or add_test to another child tree
                            auto child_vec_append = child_vector(*child_beg_append);
                            auto *tree_ptr_tmp = new tree_radix_custom(child_beg_append, child_end_append);
                            if (child_vec.empty()) {
                                //child would have been created and the append size would have been added to a new tree
                                children.emplace_back(std::vector<tree_radix_custom *>{tree_ptr_tmp},
                                                      *child_beg_append);
                            } else {
                                //on search there was no match on the tree node, so we assume that a new node referenced by this node will be created carrying append
                                //the reason why there is the correct character available but no match detected by search is the MINIMUM_MATCH_SIZE that failed, we will respect that
                                child_vec_append.push_back(tree_ptr_tmp);
                            }
                            out_list.push_back(tree_ptr_tmp);
                        }
                        //return implicit 0 with unsigned long
                        return std::make_tuple(
                                (decltype(cur_tree->data_vector().size())) std::distance(bin_beg, bin_end),
                                (decltype(cur_tree->data_vector().size())) 0,
                                (decltype(cur_tree->data_vector().size())) 0, out_list);//nothing to add, only reference
                    } else {
                        //first section tree, after split try
                        //data will split into a maximum of 3 parts and by that will add 2 more tree nodes on front and/or back; start with first section
                        tree_radix_custom *tree_ptr_first;
                        std::size_t offset{};
                        if (first_section_tree) {
                            tree_ptr_first = new tree_radix_custom(child_beg_beg, child_end_beg);
                        }

                        tree_radix_custom *tree_ptr_mid = new tree_radix_custom(child_beg_mid, child_end_mid);

                        //append will be added after middle section in case it is available
                        if (append_tree) {
                            tree_radix_custom *tree_ptr_append = new tree_radix_custom(child_beg_append,
                                                                                       child_end_append);
                            //put this append tree to the middle tree manually
                            tree_ptr_mid->children.emplace_back(std::vector<tree_radix_custom *>,)
                        }

                        tree_radix_custom *tree_ptr_last;
                        if (last_section_tree) {
                            tree_ptr_last = new tree_radix_custom(child_beg_end, child_end_end);
                        } else {

                        }

                        if (first_section_tree) {
                            cur_tree->data_vector().erase(child_beg_beg, child_end_beg);//delete front of target tree
                            offset = std::distance
                        }
                        //in any case we must be at the beginning of the vector here because first section is removed
                        cur_tree->data_vector().erase(cur_tree->data_vector().begin(), cur_tree->data_vector().begin() +
                                                                                       std::distance(child_beg_mid,
                                                                                                     child_end_mid));//delete mid of target tree
                        //return implicit 0 with unsigned long
                        //nothing to add on RAM, only splitting up the blocks on disk
                        return std::make_tuple(
                                (decltype(cur_tree->data_vector().size())) std::distance(bin_beg, bin_end),
                                (decltype(cur_tree->data_vector().size())) 0,
                                (decltype(cur_tree->data_vector().size())) 0, out_list);
                    }
                }
            };

            //first search existing structure and add into the last tree to insert potentially missing information
            auto search_index = search(bin_beg, bin_end);

            //cases for search index: its empty or it has content and with that a last tree element
            //cases for last tree if it exists, binary fit in: match from the beginning on, match in the middle, match until the end, total match
            std::tuple<std::size_t, std::size_t, std::size_t, std::list<tree_radix_custom *>> append_list{};

            if (std::get<0>(search_index).empty() && std::get<1>(search_index) == 0) {
                append_list = tree_test_sequence(this, bin_beg, bin_beg, bin_beg, bin_end,
                                                 data.end());//insert into this tree, no matches, only first character must match
            } else {
                auto last_tree = std::get<0>(search_index).back();
                //check if we have a full match and the input is larger than the data of the last tree
                append_list = tree_test_sequence(std::get<0>(last_tree), bin_beg, bin_end, std::get<1>(last_tree),
                                                 std::get<2>(last_tree),
                                                 std::get<3>(last_tree));//insert into another tree
            }

            return append_list;
        }

    protected:
        template<class ContainerType>
        std::tuple<std::size_t, std::size_t, std::size_t> add_test(const ContainerType &c) {
            return add_test(c.begin(), c.end());
        }

        //returns total size integrated, new space used uncompressed, new space used compressed
        template<typename IteratorIn>
        std::tuple<std::size_t, std::size_t, std::size_t>
        add_test(IteratorIn bin_beg, IteratorIn bin_end) {
            //uncompressed input
            if (bin_beg == bin_end || std::distance(bin_beg, bin_end) < 1)
                return {};//some element and an end element at least required

            auto tree_test_sequence = [](tree_radix_custom *cur_tree, auto bin_beg_incoming, auto bin_end_incoming,
                                         auto bin_beg_found, auto bin_end_found,
                                         const std::vector<unsigned char>::iterator data_beg_intern) {
                std::size_t matched_size = std::distance(bin_beg_incoming, bin_end_found);
                //checking if children need to be generated before and after the found input peace, reference to data of tree required
                //child before found, reference data
                auto child_beg_beg = cur_tree->data_vector().begin();
                auto child_end_beg = std::max(data_beg_intern - 1, child_beg_beg);
                //child data sequence middle, reference data
                auto child_beg_mid = data_beg_intern;
                auto child_end_mid = std::min(cur_tree->data_vector().end(),
                                              child_beg_mid + std::distance(bin_beg_found, bin_end_found));
                //child data sequence end, reference data
                auto child_beg_end = std::min(cur_tree->data_vector().end(), child_end_mid + 1);
                auto child_end_end = cur_tree->data_vector().end();
                //child after found, reference new input
                auto child_beg_append = std::min(bin_end_found + 1, bin_end_incoming);
                auto child_end_append = bin_end_incoming;
                //only the new append part may be compressed
                //before splitting or modifying a block it needs to be uncompressed

                bool first_section_tree = std::distance(child_beg_beg, child_end_beg) > 1;
                bool last_section_tree =
                        child_beg_end == child_end_end && child_end_end == cur_tree->data_vector().end();
                bool append_tree =
                        std::distance(child_beg_append, child_end_append) > 0 && child_beg_append != bin_end_incoming;
                bool total_match = !first_section_tree && !last_section_tree &&
                                   std::distance(child_beg_mid, child_end_mid) == cur_tree->data_vector().size();

                //search function already determined that this is the tree that needs to fill in the data or to split somehow
                //cases: insert front tree, insert end tree (same case as having a back insert because the end tree will just have at least 1 element in case of overflow)
                if (cur_tree->data_vector().empty()) {//how to insert, either empty simple insert or some tree construction anywhere
                    //simple insert into data since this seems to be a new node that can contain simple information
                    cur_tree->data_vector() = std::vector<unsigned char>{bin_beg_found, bin_end_found};
                    return std::make_tuple(std::distance(bin_beg, bin_end), std::distance(bin_beg, bin_end),
                                           uh::util::compression_custom::compress(bin_beg_found, bin_end_found).size());
                } else {
                    if (total_match) {
                        //a total match can still have appending structure
                        if (append_tree) {
                            /*
                            //either find child is empty and test add tree or add_test to another child tree
                            auto child_vec = child_vector(*child_beg_append);
                            if(child_vec.empty()){
                                //child would have been created and the append size would have been added to a new tree
                            }
                            else{
                                //on search there was no match on the tree node, so we assume that a new node will be created carrying append
                            }
                             */
                            //either way the appending size will be added and new space will be needed
                            return std::make_tuple(
                                    (decltype(cur_tree->data_vector().size())) std::distance(bin_beg, bin_end),
                                    (decltype(cur_tree->data_vector().size())) std::distance(child_beg_append,
                                                                                             child_end_append),
                                    (decltype(cur_tree->data_vector().size())) uh:::util::compression_custom::compress(
                                    child_beg_append, child_end_append).size());
                        }
                        //return implicit 0 with unsigned long
                        return std::make_tuple(
                                (decltype(cur_tree->data_vector().size())) std::distance(bin_beg, bin_end),
                                (decltype(cur_tree->data_vector().size())) 0,
                                (decltype(cur_tree->data_vector().size())) 0);//nothing to add, only reference
                    } else {
                        //data will split into a maximum of 3 parts and by that will add 2 more tree nodes on front and/or back
                        if (append_tree) {
                            //as on total match in this case
                            return std::make_tuple(
                                    (decltype(cur_tree->data_vector().size())) std::distance(bin_beg, bin_end),
                                    (decltype(cur_tree->data_vector().size())) std::distance(child_beg_append,
                                                                                             child_end_append),
                                    (decltype(cur_tree->data_vector().size())) uh:::util::compression_custom::compress(
                                    child_beg_append, child_end_append).size());
                        }
                        //return implicit 0 with unsigned long
                        //nothing to add on RAM, only splitting up the blocks on disk
                        return std::make_tuple(
                                (decltype(cur_tree->data_vector().size())) std::distance(bin_beg, bin_end),
                                (decltype(cur_tree->data_vector().size())) 0,
                                (decltype(cur_tree->data_vector().size())) 0);
                    }
                }
            };

            //first search existing structure and add into the last tree to insert potentially missing information
            auto search_index = search(bin_beg, bin_end);

            //cases for search index: its empty or it has content and with that a last tree element
            //cases for last tree if it exists, binary fit in: match from the beginning on, match in the middle, match until the end, total match
            std::tuple<std::size_t, std::size_t, std::size_t> append_list{};

            if (std::get<0>(search_index).empty() && std::get<1>(search_index) == 0) {
                append_list = tree_test_sequence(this, bin_beg, bin_beg, bin_beg, bin_end,
                                                 data.end());//insert into this tree, no matches, only first character must match
            } else {
                auto last_tree = std::get<0>(search_index).back();
                //check if we have a full match and the input is larger than the data of the last tree
                append_list = tree_test_sequence(std::get<0>(last_tree), bin_beg, bin_end, std::get<1>(last_tree),
                                                 std::get<2>(last_tree),
                                                 std::get<3>(last_tree));//insert into another tree
            }

            return append_list;
        }

        //returns the path of maximum fit and the match size
        template<typename IteratorIn>
        std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<IteratorIn, IteratorIn, std::vector<unsigned char>::iterator>>>>>, std::size_t>
        search(IteratorIn bin_beg, IteratorIn bin_end,
               std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<IteratorIn, IteratorIn, std::vector<unsigned char>::iterator>>>>>, std::size_t> input_list =
               std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<IteratorIn, IteratorIn, std::vector<unsigned char>::iterator>>>>>, std::size_t>{}) {
            if (bin_beg == bin_end) {
                return input_list;
            }

            auto vanilla_match_last_tree = [&](auto data_beg, auto data_end, auto bin_beg, auto bin_end) {
                auto local_matches = compare_ultihash(data_beg, data_end, bin_beg, bin_end);
                //LEGAL MATCH FILTER
                //on empty or partial match make new list in list, else append the match results on total match
                bool legal_split;
                //if the end was matched too long we can do something about that, but else the algorithm is prefix oriented
                bool end_size, begin_reached, end_reached;
                //control
                bool broken_legal = false;
                auto legal_check = [&local_matches, &legal_split, &end_size, &begin_reached, &end_reached, &broken_legal](
                        auto data_beg, auto data_end, std::vector<unsigned char>::iterator current_match) {
                    do {
                        bool start_size = MINIMUM_MATCH_SIZE < std::distance(data_beg, std::get<0>(*current_match);
                        end_size = MINIMUM_MATCH_SIZE < std::distance(std::get<0>(*current_match) +
                                                                      std::distance(std::get<1>(*current_match),
                                                                                    std::get<2>(*current_match)),
                                                                      data_end);
                        bool total_found_size = MINIMUM_MATCH_SIZE < std::distance(std::get<1>(*current_match),
                                                                                   std::get<2>(*current_match)));
                        begin_reached = data_beg == std::get<0>(*current_match);
                        end_reached = data_end == std::get<0>(*current_match) +
                                                  std::distance(std::get<1>(*current_match),
                                                                std::get<2>(*current_match));
                        legal_split = (start_size && end_size && total_found_size) || begin_reached ||
                                      end_reached;//legal if on split there cannot be a segment that is smaller than the match size
                        if (!end_size && !end_reached) {
                            std::get<2>(*current_match)--;
                            if (std::distance(std::get<1>(*current_match) < std::get<2>(*current_match)) <=
                                MINIMUM_MATCH_SIZE) {
                                local_matches.erase(*current_match);
                                broken_legal = true;
                                return;
                            }
                            std::sort(local_matches.begin(), local_matches.end(), [](auto &a, auto &b) {
                                return std::distance(std::get<1>(a), std::get<2>(a)) >
                                       std::distance(std::get<1>(b), std::get<2>(b));
                            });
                        }
                    } while (!end_size && !end_reached && !local_matches.empty());
                };

                auto match_beg = local_matches.begin();
                std::size_t count_match_pos{};
                while (match_beg != match_end) {
                    legal_check(data_beg,data_end,match_beg);
                    if (broken_legal)match_beg = local_matches.begin() + count_match_pos;
                    else {
                        match_beg++;
                        count_match_pos++;
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
                std::sort(local_matches.begin(), local_matches.end(), [this](auto &a, auto &b) {
                    return std::distance(data_beg, std::get<0>(a)) < std::distance(data_beg, std::get<0>(b));
                });

                if (local_matches.empty())return std::make_tuple(std::get<0>(input_list), std::get<1>(input_list));

                legal_check(data_beg,data_end,local_matches.begin());

                std::vector<std::tuple<IteratorIn, IteratorIn, std::vector<unsigned char>::iterator>> found_vec{};
                found_vec.emplace_back(std::get<1>(local_matches[0]), std::get<2>(local_matches[0]),
                                       std::get<0>(local_matches[0]));

                if (input_list.empty() || !legal_split) {
                    std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<IteratorIn, IteratorIn, std::vector<unsigned char>::iterator>>>> tmp_list{};
                    tmp_list.emplace_back(this, found_vec);
                    std::get<0>(input_list).push_back(tmp_list);
                } else {
                    //if the tree element of the last element is still the same as "this" we append to the vector, else we append a new list
                    auto last_it_outer_list = (--(std::get<0>(input_list).end()));
                    auto last_it_inner_list = (--(last_it_outer_list->end()));
                    if (std::get<0>(*last_it_inner_list) == this) {//check if tree pointer is the same
                        std::get<1>(*last_it_inner_list).push_back(found_vec[0]);
                    } else {
                        last_it_outer_list->emplace_back(this, found_vec);
                    }
                }
                std::get<1>(input_list) += std::distance(std::get<1>(local_matches[0]), std::get<2>(local_matches[0]));
            };

            auto data_beg = data.begin();
            auto data_beg_tmp = data_beg;
            auto bin_beg_tmp = bin_beg;
            do{
                vanilla_match_last_tree(data_beg, data.end(), bin_beg_tmp, bin_end);
                //advance data_beg behind the offset of the last found binary sequence and advance bin_beg behind the size of the found subset
                //stop the loop if manually searching matches for the range fails
                auto last_it_outer_list = (--(std::get<0>(input_list).end()));
                auto last_it_inner_list = (--(last_it_outer_list->end()));

                if (std::get<0>(*last_it_inner_list) == this) {//check if tree pointer is the same
                    std::get<1>(*last_it_inner_list).push_back(found_vec[0]);
                } else {
                    last_it_outer_list->emplace_back(this, found_vec);
                }

                //check child that deals with searching the far most rest in direction of end to skip the not matching rest
                auto child_vec = child_vector(*(++bin_end));
                if (!child_vec.empty()) {//if the input range is too large we else would not get a match
                    std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, IteratorIn, IteratorIn, std::vector<unsigned char>::iterator>>>, std::size_t>> best_search_list{};
                    for (const auto &item: child_vec) {
                        best_search_list.push_back(item->search(bin_beg_tmp, bin_end, input_list));
                    }
                    //return the largest match with the lowest offset on the last tree, as far as there is a last tree...
                    std::sort(best_search_list.begin(), best_search_list.end(), [](auto &a, auto &b) {
                        return std::get<1>(a) > std::get<1>(b);//sort in descending order on search match size
                    });
                    //erase after the matches decrease
                    if (best_search_list.size() > 1) {
                        std::size_t max_fit{};
                        auto best_beg = best_search_list.begin();
                        while (best_beg != best_search_list.end()) {
                            max_fit = std::max(max_fit, std::get<1>(*best_beg));
                            if (std::get<1>(*best_beg) < max_fit) {
                                //break and delete until end
                                break;
                            }
                            best_beg++;
                        }
                        best_search_list.erase(best_beg, best_search_list.end());
                    }

                    std::get<0>(input_list).splice(std::get<0>(input_list).cend(), std::get<0>(best_search_list[0]));
                    std::get<1>(input_list) += std::get<1>(best_search_list[0]);
                }
            }while (data_beg != data.end());//size of data is at least 1
            return input_list;
        }
        /*
        //add some string into the radix tree, returning the tree nodes where it was compressed and stored along the way
        std::tuple<std::list<tree_radix_custom *>> add(std::vector<unsigned char> &bin,
            std::list<tree_radix_custom *> enlist = std::list<tree_radix_custom *>{}) {
            if (!bin.empty()) {
                if (data.empty()) {
                    if (!has_children()) {
                        data.reserve(bin.size());
                        data = bin;
                        enlist.push_back(this);
                        return {enlist,0};
                    } else {
                        if (children[(unsigned char) bin[0]] == nullptr) {
                            // no match, create new node for rest of string
                            children[(unsigned char) bin[0]] = new tree_radix_custom();//(struct tree_radix_custom*) std::malloc(sizeof(struct tree_radix_custom));
                            //new (children[(unsigned char)bin[0]]) tree_radix_custom();
                            auto enlist_append = children[(unsigned char) bin[0]]->add(bin, enlist);
                            enlist.push_back(this);
                            enlist.splice(enlist.cend(), std::get<0>(enlist_append));
                            return {enlist,std::get<1>(enlist_append) + data.size()};
                        } else {
                            enlist.push_back(this);
                            return children[(unsigned char) bin[0]]->add(bin, enlist);
                        }
                    }
                } else {
                    auto found = std::ranges::search(data.begin(),data.end(),bin.begin(),bin.end());
                    const auto first = std::distance(data.begin(), found.begin());
                    const auto last = std::distance(data.begin(), found.end());

                    if (!(first - last)) {
                        children[(unsigned char) data[0]] = new tree_radix_custom(data);
                        std::memcpy(children[(unsigned char) data[0]]->children, this->children, N * sizeof(tree_radix_custom *));
                        data.clear();
                        return add(bin, enlist);
                    }
                    if (first == 0 && last == data.size() - 1) {
                        if (bin.size() > data.size()) {
                            //insert deeper node directly on rest
                            if (children[(unsigned char) bin[data.size()]] == nullptr) {
                                auto *tmp = new tree_radix_custom();//(struct tree_radix_custom*) std::malloc(sizeof(struct tree_radix_custom));
                                //new (tmp) tree_radix_custom();
                                children[(unsigned char) bin[data.size()]] = tmp;
                                enlist.push_back(this);
                                return tmp->add(std::vector<unsigned char>{bin.cbegin() + static_cast<long>(data.size()), bin.cend()}, enlist);
                            } else {
                                enlist.push_back(this);
                                return children[(unsigned char) bin[data.size()]]->add(std::vector<unsigned char>{bin.cbegin() + static_cast<long>(data.size()), bin.cend()}, enlist);
                            }
                        }
                        // direct match, direct redirect
                        enlist.push_back(this);
                        return enlist;
                    }
                    if (i < data.size() - 1) {
                        // match string is too short -> split node
                        std::size_t higher_string_len = i + 1, lower_string_len = data.size() - (i + 1);
                        char *higher_node_string = new char[higher_string_len];//(char*) std::malloc(higher_val*sizeof(char));
                        std::memcpy(higher_node_string, data, higher_string_len);
                        char *lower_node_string = new char[lower_string_len];//(char*) std::malloc(lower_string_len*sizeof(char));
                        std::memcpy(lower_node_string, data + higher_string_len, lower_string_len);
                        length = higher_string_len;
                        std::free(data);
                        data = higher_node_string;
                        auto *new_child_node = (struct tree_radix_custom *) std::malloc(
                                sizeof(struct tree_radix_custom));
                        //new (new_child_node) tree_radix_custom();
                        std::memcpy(new_child_node->children, this->children, N * sizeof(tree_radix_custom *));
                        new_child_node->data = lower_node_string;
                        new_child_node->length = lower_string_len;
                        for (auto &i1: children) {
                            i1 = nullptr;
                        }
                        children[(unsigned char) lower_node_string[0]] = new_child_node;
                        enlist.push_back(new_child_node);
                        return enlist;
                    }
                }
            }
            return enlist;
        }
*/
/*
        //copy one node without children
        tree_radix_custom *copy() {
            auto *tmp = (struct tree_radix_custom *) std::malloc(sizeof(struct tree_radix_custom));
            //new (tmp) tree_radix_custom();
            std::memcpy(tmp->children, this->children, N * sizeof(tree_radix_custom *));
            tmp->data = (char *) std::malloc(length * sizeof(char));
            std::memcpy(tmp->data, data, length);
            tmp->length = length;
            return tmp;
        }

        //copy recursive on one node
        tree_radix_custom *copy_recursive() {
            auto *tmp = copy();
            for (unsigned char i = 0;; i++) {
                if (children[i] != nullptr) {
                    tmp->children[i] = children[i]->copy_recursive();
                }
                if (i == (unsigned char) (N - 1))break;
            }
            return tmp;
        }

        //destroy sub structure of node to prevent memory leaks before deleting the node itself
        void destroy_recursive() {
            for (auto &i: children) {
                if (i != nullptr) {
                    i->destroy_recursive();
                    delete i;
                }
            }
        }

        //destroy one child recursively if it exists
        void destroy_recursive(char child) {
            if (children[child] != nullptr) {
                children[child]->destroy_recursive();
                delete children[child];
                std::free(children[child]);
                children[child] = nullptr;
            }
        }

        //check if the structure has children
        bool has_children() {
            bool has_children = false;
            for (const auto &i: children) {
                if (i != nullptr) {
                    has_children = true;
                    break;
                }
            }
            return has_children;
        }
*/
        /*
         * insert another node and all children strings to this root node, scan through all combined strings,
         * that are formed from the root to every child node; and finally copy all contents of the incoming node to "this"
         */
/*
        void insert(tree_radix_custom *in) {
            std::list<std::tuple<tree_radix_custom *, unsigned char>> concat_string;
            concat_string.emplace_back(in, 0);
            while (!concat_string.empty()) {
                bool has_children = false;
                for (unsigned char &i = std::get<1>(concat_string.back());; i++) {
                    if (std::get<0>(concat_string.back())->children[i] != nullptr) {
                        has_children = true;
                        concat_string.emplace_back(std::get<0>(concat_string.back())->children[i], 0);
                        i++;
                        break;
                    }
                    if (i == (unsigned char) (N - 1))break;
                }
                if (!has_children) {
                    std::size_t concat_size{}, start_step{};
                    std::for_each(concat_string.cbegin(), concat_string.cend(), [&concat_size](auto in) {
                        concat_size += std::get<0>(in)->length;
                    });
                    char *concat_sequence = new char[concat_size];//(char*) std::malloc(concat_size * sizeof(char));
                    std::for_each(concat_string.cbegin(), concat_string.cend(),
                                  [&concat_sequence, &start_step](auto in) {
                                      std::memcpy(concat_sequence + start_step, std::get<0>(in)->data,
                                                  std::get<0>(in)->length);
                                      start_step += std::get<0>(in)->length;
                                  });
                    (void) add(concat_sequence, concat_size);
                    std::free(concat_sequence);
                    concat_string.pop_back();
                }
            }
        }
*/
        /*
         * search through this node and return the matching pathway and the depth until the incoming string fit the
         * internals of the node
         */
/*
        std::tuple<std::list<tree_radix_custom *>, std::size_t>
        search(std::vector<unsigned char> bin,
               std::tuple<std::list<tree_radix_custom *>, std::size_t> enlist = std::tuple<std::list<tree_radix_custom *>, std::size_t>{}) {
            if (len > 0) {
                if (length == 0) {
                    if (children[(unsigned char) bin[0]] == nullptr) {
                        return enlist;
                    } else {
                        std::get<0>(enlist).push_back(this);
                        return children[(unsigned char) bin[0]]->search(bin, len, enlist);
                    }
                } else {
                    std::size_t i = 0;
                    for (; i < std::min(length, len) - 1;) {
                        if (data[i] != bin[i])break;
                        else i++;
                    }
                    if (i == 0) {
                        return enlist;
                    }
                    if (i == length - 1) {
                        std::get<0>(enlist).push_back(this);
                        std::get<1>(enlist) += length;
                        if (len > length && children[(unsigned char) bin[length]] != nullptr)
                            return children[(unsigned char) bin[length]]->search(bin + length, len - length, enlist);
                        // direct match, direct redirect
                        return enlist;
                    }
                    if (i < length - 1) {
                        std::get<0>(enlist).push_back(this);
                        std::get<1>(enlist) += i + 1;
                        return enlist;
                    }
                }
            }
            return enlist;
        }
*/
    };
}

#endif //UHLIBCOMMON_TREE_RADIX_CUSTOM_H
