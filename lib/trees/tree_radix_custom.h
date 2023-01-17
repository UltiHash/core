//
// Created by benjamin-elias on 17.12.22.
//

#ifndef UHLIBCOMMON_TREE_RADIX_CUSTOM_H
#define UHLIBCOMMON_TREE_RADIX_CUSTOM_H

#include "logging/logging_boost.h"
#include "trees/tree_storage_config.h"
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
        std::vector<std::tuple<std::vector<tree_radix_custom*>,unsigned char>>children{};//multiple targets that can follow a node for each letter
        std::vector<unsigned char> data{};//any binary vector string
    public:
        tree_radix_custom() = default;

        explicit tree_radix_custom(std::vector<unsigned char> &bin) : tree_radix_custom() {
            add(bin);
        }

        [[nodiscard]] std::size_t size() const {
            return data.size();
        }

        std::vector<unsigned char> &data_vector() {
            return data;
        }

        std::vector<tree_radix_custom*> child_vector(unsigned char i) {
            if(children.empty())return {};
            for(const auto &item:children){
                if(std::get<1>(item) == i){
                    return std::get<0>(item);
                }
            }
            return {};
        }
    private:
        auto compare_ultihash(auto data_beg, auto data_end, auto input_beg, auto input_end){
            if(std::distance(input_beg,input_end)>std::distance(data_beg,data_end))input_end = input_beg + std::distance(data_beg,data_end);
            //if input does only fit to a shorter string as a subset of data, count becomes negative, else positive including ß
            //data offset iterator and start and end of input
            std::vector<std::tuple<decltype(data_beg),decltype(input_beg),decltype(input_end)>> matches{};

            //advance search scope over data
            //increase data start to the beginning of the input match +1
            decltype(input_end) input_end_tmp;
            //search forward through data
            do{
                input_end_tmp = input_beg;//increase end of input to possibly find a prefix match
                //first element match
                while(*data_beg != *input_beg && data_beg != data_end){
                    data_beg++;
                }
                if(data_beg == data_end || input_end_tmp == input_end)break;
                //search how long input matches
                decltype(std::ranges::search(data_beg,data_end,input_beg,input_end_tmp)) found;
                do{
                    found = std::ranges::search(data_beg,data_end,input_beg,input_end_tmp);
                    input_end_tmp++;
                }
                while(input_end_tmp != input_end && std::distance(found.begin(),found.end())==std::distance(input_beg,input_end_tmp));
                //last input count reversed
                if(std::distance(input_beg,input_end_tmp)>MINIMUM_MATCH_SIZE){
                    input_end_tmp--;
                    matches.emplace_back(data_beg,input_beg,input_end_tmp);
                }
                data_beg++;
            }
            while(data_beg != data_end);

            return matches;
        }
    public:

        template<class ContainerType>
        std::tuple<std::size_t,std::size_t,std::size_t,std::list<tree_radix_custom*>> add(const ContainerType &c){
            return add(c.begin(),c.end());
        }
        //returns total size integrated, new space used uncompressed, new space used compressed, reference tree list
        template<typename IteratorIn>
        std::tuple<std::size_t,std::size_t,std::size_t,std::list<tree_radix_custom*>>
        add(IteratorIn bin_beg,IteratorIn bin_end,
            std::tuple<std::size_t,std::size_t,std::size_t,std::list<tree_radix_custom*>> input_list =
                    std::tuple<std::size_t,std::size_t,std::size_t,std::list<tree_radix_custom*>>{}){

            if(bin_beg == bin_end || std::distance(bin_beg,bin_end)<1)return input_list;//some element and an end element at least required

            auto tree_construction_sequence = [](tree_radix_custom* cur_tree,auto bin_beg,auto bin_end) {
                auto compare = compare_ultihash(cur_tree->data_vector.begin(),cur_tree->data_vector.end(),bin_beg,bin_end);
                bool match_start = std::get<1>(cur_tree) == bin_beg;//determine how to build the tree in case of binary fit in
                bool match_end = std::get<2>(cur_tree) == bin_end;
                //checking oversize

                auto child_vec = cur_tree->child_vector(*bin_beg);

                //search function already determined that this is the tree that needs to fill in the data or to split somehow
                //cases: no tree, insert front tree, insert middle tree (same case as having a back insert because the end tree will just be empty

                if (cur_tree->data_vector.empty()) {//how to insert, either empty simple insert or some tree construction anywhere
                    //simple insert, check children
                    if (child_vec.empty()) {
                        //simple insert into data since this seems to be a new node that can contain simple information
                        //
                        cur_tree->data_vector = std::vector<unsigned char>{bin_beg, bin_end};
                    } else {
                        //the parent sequence must be a total match plus one extra element that landed here, just return
                        if (bin_beg != bin_end)
                            DEBUG
                                << "On adding a total match with an extra element there were more elements, which should be impossible.";
                    }
                    return {cur_tree->data_vector.size()+!child_vec.empty(), cur_tree->data_vector.size()+!child_vec.empty(), cur_tree->data_vector.size()+!child_vec.empty(),
                            std::list<tree_radix_custom *>>{ cur_tree }};
                } else {
                    //insert into the data of the incoming tree and split into multiple nodes

                }
            };

            //first search existing structure and add into the last tree to insert potentially missing information
            auto search_index = search(bin_beg,bin_end);

            //cases for search index: its empty or it has content and with that a last tree element
            //cases for last tree if it exists, binary fit in: match from the beginning on, match in the middle, match until the end, total match
            std::tuple<std::size_t,std::size_t,std::size_t,std::list<tree_radix_custom*>> append_list{};

            if(std::get<0>(search_index).empty() && std::get<1>(search_index) == 0){
                append_list = tree_construction_sequence(this, bin_beg, bin_end);//insert into this tree
            }
            else{
                auto last_tree = search_index.back();
                //check if we have a full match and the input is larger than the data of the last tree
                append_list = tree_construction_sequence(std::get<0>(last_tree),std::get<1>(last_tree),bin_end);//insert into another tree
            }

            std::get<3>(input_list).splice(std::get<3>(input_list).cend(),std::get<3>(append_list));
            return {std::get<0>(input_list)+std::get<0>(append_list),std::get<1>(input_list)+std::get<1>(append_list),
                    std::get<2>(input_list)+std::get<2>(append_list),std::get<3>(input_list)};
        }

        //returns the path of maximum fit and the match size
        template<typename IteratorIn>
        std::tuple<std::list<std::tuple<tree_radix_custom *,IteratorIn,IteratorIn,std::vector<unsigned char>::iterator>>, std::size_t>
        search(IteratorIn bin_beg,IteratorIn bin_end,
               std::tuple<std::list<std::tuple<tree_radix_custom *,IteratorIn,IteratorIn,std::vector<unsigned char>::iterator>>, std::size_t> input_list =
               std::tuple<std::list<std::tuple<tree_radix_custom *,IteratorIn,IteratorIn,std::vector<unsigned char>::iterator>>, std::size_t>{}){
            if(bin_beg==bin_end){
                return input_list;
            }

            auto bin_end_tmp = bin_end;
            auto input_size = std::distance(bin_beg,bin_end);
            auto data_size = std::distance(data.begin(),data.end());
            if(input_size>data_size)bin_end_tmp = bin_beg + data_size;//limit to max fit if possible

            auto vanilla_match_last_tree = [&](auto bin_beg,auto bin_end){
                auto local_matches = compare_ultihash(data.begin(),data.end(),bin_beg,bin_end);
                std::sort(local_matches.begin(),local_matches.end(),[](auto &a,auto &b){
                    return std::distance(std::get<1>(a),std::get<2>(a))>std::distance(std::get<1>(b),std::get<2>(b));
                });

                if(local_matches.size()>1){
                    std::size_t max_fit{};
                    auto best_beg = local_matches.begin();
                    while(best_beg!=local_matches.end()){
                        max_fit = std::max(max_fit,std::distance(std::get<1>(*best_beg),std::get<2>(*best_beg)));
                        if(std::distance(std::get<1>(*best_beg),std::get<2>(*best_beg))<max_fit){
                            //break and delete until end
                            break;
                        }
                        best_beg++;
                    }
                    best_search_list.erase(best_beg,local_matches.end());
                }
                //sort smallest offset
                std::sort(local_matches.begin(),local_matches.end(),[&data](auto &a,auto &b){
                    return std::distance(data.begin(),std::get<0>(a))<std::distance(data.begin(),std::get<0>(b));
                });
                //single search find
                if(local_matches.empty())return {std::get<0>(input_list),std::get<1>(input_list)};
                std::get<0>(input_list).emplace_back(this,std::get<1>(local_matches[0]),std::get<2>(local_matches[0]),std::get<0>(local_matches[0]));
                return {std::get<0>(input_list),std::get<1>(input_list)+std::distance(std::get<1>(local_matches[0]),std::get<2>(local_matches[0]))};
            };

            auto child_vec = child_vector(*(++bin_end_tmp));

            if(!child_vec.empty()){//if the input range is too large we else would not get a match
                std::vector<std::tuple<std::list<std::tuple<tree_radix_custom *,IteratorIn,IteratorIn,std::vector<unsigned char>::iterator>>, std::size_t>> best_search_list{};
                for(const auto&item:child_vec){
                    best_search_list.push_back(item->search(bin_end_tmp,bin_end,input_list));
                }
                //return the largest match with the lowest offset on the last tree, as far as there is a last tree...
                std::sort(best_search_list.begin(),best_search_list.end(),[](auto &a, auto &b){
                    return std::get<1>(a) > std::get<1>(b);//sort in descending order on search match size
                });
                //erase after the matches decrease
                if(best_search_list.size()>1){
                    std::size_t max_fit{};
                    auto best_beg = best_search_list.begin();
                    while(best_beg!=best_search_list.end()){
                        max_fit = std::max(max_fit,std::get<1>(*best_beg));
                        if(std::get<1>(*best_beg)<max_fit){
                            //break and delete until end
                            break;
                        }
                        best_beg++;
                    }
                    best_search_list.erase(best_beg,best_search_list.end());
                }
                //duplicates are possible and should be sorted to get the smallest offset on the largest matches, this reduces tree depth
                std::vector<std::tuple<std::tuple<std::list<std::tuple<tree_radix_custom *,IteratorIn,IteratorIn,std::vector<unsigned char>::iterator>>, std::size_t>,std::size_t>> lowest_offset_list{};

                for(const auto&item:best_search_list){
                    auto* tree_ptr = std::get<0>(item).back();
                    auto other_tree_vec_beg = (tree_ptr->data_vector()).begin();
                    auto local_matches = compare_ultihash(other_tree_vec_beg,(tree_ptr->data_vector()).end(),bin_end_tmp,bin_end);//do the compare again within this perspective
                    auto start_offset_match_other_tree_beg = std::get<0>(local_matches[0]);
                    std::size_t offset_dist = std::distance(other_tree_vec_beg,start_offset_match_other_tree_beg);
                    lowest_offset_list.emplace_back(item,offset_dist);//save the input range and the tree it was checked with, the successful offset and the start pointer on the data that the input points at
                }

                std::sort(lowest_offset_list.begin(),lowest_offset_list.end(),[](auto &a,auto &b){
                    return std::get<1>(a) < std::get<1>(b);//sort in ascending order on search match size
                });

                std::get<0>(input_list).splice(std::get<0>(input_list).cend(),std::get<0>(std::get<0>(lowest_offset_list[0])));
                std::get<1>(input_list)+=std::get<1>(std::get<0>(lowest_offset_list[0]));
                return {std::get<0>(input_list),std::get<1>(input_list),std::get<2>(lowest_offset_list[0])};
            }
            else{
                return vanilla_match_last_tree(bin_beg,bin_end_tmp-1);
            }
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
