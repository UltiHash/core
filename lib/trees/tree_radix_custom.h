//
// Created by benjamin-elias on 17.12.22.
//

#ifndef UHLIBCOMMON_TREE_RADIX_CUSTOM_H
#define UHLIBCOMMON_TREE_RADIX_CUSTOM_H

#include "logging/logging_boost.h"
#include "trees/tree_storage_config.h"
#include "util/compression_custom.h"
#include <vector>
#include <list>
#include <set>
#include <ranges>
#include <algorithm>
#include <openssl/sha.h>

namespace uh::trees {
    //because it takes at least 2 bytes to describe a deeper encoding action
#define MINIMUM_MATCH_SIZE SHA512_DIGEST_LENGTH+sizeof(unsigned long)*TIME_STAMPS_ON_BLOCK+SHA256_DIGEST_LENGTH+3+5//the overhead of storing the block plus the size of basic storage pointer

    template<class DataReference>
    struct tree_radix_custom {
    protected:
        //the first element of data is cut off to children except on root if it's a new tree
        std::vector<std::tuple<std::vector<tree_radix_custom *>, unsigned char>> children{};//multiple targets that can follow a node for each letter
        std::vector<unsigned char> data{};//any binary vector string
        DataReference data_ref{};
        std::size_t block_swarm_offset{};//offset of block beginning from root
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

        DataReference &data_reference() {
            return data_ref;
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

        void child_put(tree_radix_custom* input_tree,unsigned char first_letter){
            auto child_vec_append = child_vector(first_letter);
            if (child_vec_append.empty()) {
                //child would have been created and the append size would have been added to a new tree
                children.emplace_back(std::vector<tree_radix_custom *>{input_tree},first_letter);
            } else {
                //on search there was no match on the tree node, so we assume that a new node referenced by this node will be created carrying append
                //the reason why there is the correct character available but no match detected by search is the MINIMUM_MATCH_SIZE that failed, we will respect that
                if(std::ranges::find(child_vec_append.begin(),child_vec_append.end(),input_tree)==child_vec_append.end()){
                    child_vec_append.emplace_back(input_tree);
                    std::sort(child_vec_append.begin(),child_vec.end(),[](&auto a,&auto b){
                        return lexicographical_compare(a->data_vector().begin(),a->data_vector().end(),b->data_vector().begin(),b->data_vector().end());
                    });
                }
            }
            std::sort(children.begin(),children.end(),[](auto &a, auto &b){
                return std::get<1>(a)<std::get<1>(b);
            });
        }

        bool child_delete(tree_radix_custom* input_tree,unsigned char first_letter){
            auto child_vec_append = child_vector(first_letter);
            if (child_vec_append.empty()) {
                return false;
            } else {
                //on search there was no match on the tree node, so we assume that a new node referenced by this node will be created carrying append
                //the reason why there is the correct character available but no match detected by search is the MINIMUM_MATCH_SIZE that failed, we will respect that
                auto find_beg = std::ranges::find(child_vec_append.begin(),child_vec_append.end(),input_tree);
                if(find_beg==child_vec_append.end()){
                    child_vec_append.erase(find_beg);
                    if(child_vec_append.empty()){
                        return child_delete_letter(first_letter);
                    }
                    return true;
                }
                else return false;
            }
        }

        bool child_delete_letter(unsigned char delete_letter){
            auto item_beg = children.begin();
            while (item_beg != children.end()) {
                if (std::get<1>(*item_beg) == delete_letter) {
                    children.erase(item_beg);
                    return true;
                }
            }
            return false;
        }

    private:
        std::vector<std::tuple<std::vector<unsigned char>::iterator,std::vector<unsigned char>::iterator,std::vector<unsigned char>::iterator>> compare_ultihash(std::vector<unsigned char>::iterator &data_beg, std::vector<unsigned char>::iterator &data_end, std::vector<unsigned char>::iterator &input_beg, std::vector<unsigned char>::iterator &input_end) {
            if (std::distance(input_beg, input_end) > std::distance(data_beg, data_end))
                input_end = input_beg + std::distance(data_beg, data_end);
            //if input does only fit to a shorter string as a subset of data, count becomes negative, else positive including ß
            //data offset iterator and start and end of input
            std::vector<std::tuple<std::vector<unsigned char>::iterator,std::vector<unsigned char>::iterator,std::vector<unsigned char>::iterator>> matches{};
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
                std::vector<unsigned char>::iterator data_beg_tmp = data_beg;
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
                if (std::distance(input_beg, input_beg_tmp - broken) >= MINIMUM_MATCH_SIZE) {
                    matches.emplace_back(data_beg, input_beg, input_beg_tmp - broken);
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

        //returns total size integrated, new space used uncompressed, new space used compressed, list of tree references of <offset_ELEMENT,modified_LIST,added_LIST> tree nodes
        std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::list<std::tuple<std::list<tree_radix_custom *>,std::list<tree_radix_custom *>>>>>
        add(std::vector<unsigned char>::iterator &bin_beg, std::vector<unsigned char>::iterator &bin_end) {
            //first search existing structure and add into the last tree to insert potentially missing information
            std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator>>>>>, std::size_t>> search_index = search(
                    bin_beg, bin_end);
            //uncompressed input
            if (bin_beg == bin_end || std::distance(bin_beg, bin_end) < 1) {
                return {};
            }
            //some element and an end element at least required
            auto tree_building_sequence = [&first_section_tree,&last_section_tree,&append_tree,&total_match](tree_radix_custom *cur_tree, std::vector<unsigned char>::iterator bin_beg_incoming,
                                             std::vector<unsigned char>::iterator bin_end_incoming, std::vector<unsigned char>::iterator bin_beg_found, std::vector<unsigned char>::iterator bin_end_found,
                                             const std::vector<unsigned char>::iterator data_beg_intern) {
                std::size_t tree_front_data_front_absolute = std::distance(cur_tree->data_vector().begin(),data_beg_intern)+1;
                std::size_t matched_size = std::distance(bin_beg_incoming, bin_end_found);
                //checking if children need to be generated before and after the found input peace, reference to data of tree required
                //child before found, reference data
                std::vector<unsigned char>::iterator child_beg_beg = cur_tree->data_vector().begin();
                std::vector<unsigned char>::iterator child_end_beg = std::max(data_beg_intern - 1, child_beg_beg);
                //child data sequence middle, reference data
                std::vector<unsigned char>::iterator child_beg_mid = data_beg_intern;
                std::vector<unsigned char>::iterator child_end_mid = std::min(cur_tree->data_vector().end(),
                                              child_beg_mid + std::distance(bin_beg_found, bin_end_found));
                //child data sequence end, reference data
                std::vector<unsigned char>::iterator child_beg_end = std::min(cur_tree->data_vector().end(), child_end_mid + 1);
                std::vector<unsigned char>::iterator child_end_end = cur_tree->data_vector().end();
                //child after found, reference new input
                std::vector<unsigned char>::iterator child_beg_append = std::min(bin_end_found + 1, bin_end_incoming);
                std::vector<unsigned char>::iterator child_end_append = bin_end_incoming;
                //only the new append part may be compressed
                //before splitting or modifying a block it needs to be uncompressed

                bool first_section_tree = std::distance(child_beg_beg, child_end_beg) > 1;
                bool last_section_tree =
                        child_beg_end == child_end_end && child_end_end == cur_tree->data_vector().end();
                bool append_tree =
                        std::distance(child_beg_append, child_end_append) > 0 && child_beg_append != bin_end_incoming;
                bool total_match = !first_section_tree && !last_section_tree &&
                                   std::distance(child_beg_mid, child_end_mid) == cur_tree->data_vector().size();

                uh::util::compression_custom comp{};
                auto out_list = std::vector<tree_radix_custom *>{};

                //search function already determined that this is the tree that needs to fill in the data or to split somehow
                //cases: insert front tree, insert end tree (same case as having a back insert because the end tree will just have at least 1 element in case of overflow)
                if (cur_tree->data_vector().empty()) {//how to insert, either empty simple insert or some tree construction anywhere
                    //simple insert into data since this seems to be a new node that can contain simple information
                    cur_tree->data_vector() = std::vector<unsigned char>{child_beg_mid, child_end_mid};
                    out_list.push_back(cur_tree);
                    return std::make_tuple(std::distance(child_beg_mid, child_end_mid),
                                           std::distance(child_beg_mid, child_end_mid),
                                           comp.compress(child_beg_mid, child_end_mid).size(),out_list,first_section_tree,last_section_tree,append_tree,total_match,tree_front_data_front_absolute);
                } else {
                    if (total_match) {//only a maximum of 1 tree creation or just 0 in case of reference
                        //a total match can still have appending structure
                        std::size_t append_size_compressed{}, append_size_uncompressed{};
                        out_list.push_back(cur_tree);
                        if (append_tree) {
                            append_size_compressed = comp.compress(child_beg_mid,child_end_mid).size();
                            append_size_uncompressed = std::distance(child_beg_mid, child_end_mid);
                            //either find child is empty and test add tree or add_test to another child tree

                            auto *tree_ptr_tmp = new tree_radix_custom(child_beg_append, child_end_append);
                            out_list.push_back(tree_ptr_tmp);
                            tree_ptr_tmp->block_swarm_offset = cur_tree->block_swarm_offset + cur_tree->data_vector().size();
                            cur_tree->child_put(tree_ptr_tmp,*child_beg_append);
                        }
                        //return implicit 0 with unsigned long
                        return std::make_tuple(
                                (decltype(cur_tree->data_vector().size())) append_size_uncompressed,
                                (decltype(cur_tree->data_vector().size())) append_size_uncompressed,
                                (decltype(cur_tree->data_vector().size())) append_size_compressed,out_list,first_section_tree,last_section_tree,append_tree,total_match,tree_front_data_front_absolute);//nothing to add, only reference
                    } else {
                        //first section tree, after split try
                        //data will split into a maximum of 3 parts and by that will add 2 more tree nodes on front and/or back; start with first section
                        tree_radix_custom *tree_ptr_first;
                        std::size_t offset{};
                        std::size_t size_integrated{}, size_compressed{}, size_uncompressed{};

                        tree_radix_custom *tree_ptr_mid;
                        tree_radix_custom *tree_ptr_append;

                        if (first_section_tree) {
                            //children contents need to be copied to middle tree and any references to this node need to be moved to middle tree
                            //new middle tree required
                            tree_ptr_mid = new tree_radix_custom(child_beg_mid, child_end_mid);
                            tree_ptr_first = cur_tree;
                            out_list.push_back(tree_ptr_first);
                            tree_ptr_mid->block_swarm_offset = tree_ptr_first->block_swarm_offset + tree_ptr_first->data_vector().size();
                            tree_ptr_mid->children = cur_tree->children;
                            cur_tree->children.clear();
                            tree_ptr_mid->data_ref = cur_tree->data_ref;
                            tree_ptr_first->data_ref.clear();

                            size_integrated += std::distance(child_beg_beg, child_end_beg) + 1;
                            //try to add the reference entry to middle tree on first tree
                            tree_ptr_first->child_put(tree_ptr_mid,*child_beg_mid);
                        } else {
                            //the current tree stays fundament
                            tree_ptr_mid = cur_tree;
                            size_integrated += std::distance(child_beg_mid, child_end_mid) + 1;
                        }
                        out_list.push_back(tree_ptr_mid);

                        tree_radix_custom *tree_ptr_last;
                        if (last_section_tree) {
                            //create last tree
                            tree_ptr_last = new tree_radix_custom(child_beg_end, child_end_end);
                            //transfer information of middle tree to last tree and copy the children also to append tree in case it exists
                            out_list.push_back(tree_ptr_last);
                            tree_ptr_last->block_swarm_offset = tree_ptr_mid->block_swarm_offset + tree_ptr_mid->data_vector().size();
                            tree_ptr_last->children = tree_ptr_mid->children;
                            tree_ptr_mid->children.clear();
                            tree_ptr_last->data_ref = tree_ptr_mid->data_ref;
                            tree_ptr_mid->data_ref.clear();

                            size_integrated += std::distance(child_beg_end, child_end_end) + 1;

                            uh::util::compression_custom comp{};
                            //the last tree is the last tree and may append
                            //appending will be added after middle section in case it is available
                            if (append_tree) {
                                tree_ptr_append = new tree_radix_custom(child_beg_append, child_end_append);
                                out_list.push_back(tree_ptr_append);
                                tree_ptr_append->block_swarm_offset = tree_ptr_mid->block_swarm_offset + tree_ptr_mid->data_vector().size();

                                size_integrated += std::distance(child_beg_append, child_end_append) + 1;
                                size_uncompressed += std::distance(child_beg_append, child_end_append) + 1;
                                size_compressed += comp.compress(child_beg_append,child_end_append).size();
                                //put this append tree to the middle tree manually
                                tree_ptr_mid->child_put(tree_ptr_append,*child_beg_append);
                            }
                            //the last tree is still following the middle tree
                            tree_ptr_mid->child_put(tree_ptr_last,*child_beg_end);
                        } else {
                            //the middle tree is the last tree and may append
                            //appending will be added after middle section in case it is available
                            if (append_tree) {
                                tree_ptr_append = new tree_radix_custom(child_beg_append, child_end_append);
                                out_list.push_back(tree_ptr_append);
                                tree_ptr_append->block_swarm_offset = tree_ptr_mid->block_swarm_offset + tree_ptr_mid->data_vector().size();

                                size_integrated += std::distance(child_beg_append, child_end_append) + 1;
                                size_uncompressed += std::distance(child_beg_append, child_end_append) + 1;
                                size_compressed += comp.compress(child_beg_append,child_end_append).size();
                                //put this append tree to the middle tree manually
                                tree_ptr_mid->child_put(tree_ptr_append,*child_beg_append);
                            }
                        }

                        std::size_t del_beg{};
                        std::size_t del_end{};

                        if (first_section_tree) {
                            if(last_section_tree){
                                //delete the referenced data size of middle and end from tree pointer first
                                std::vector<unsigned char>::iterator del_beg = tree_ptr_first->data_vector().begin()+std::distance(child_beg_beg,child_end_beg)+1;
                                std::vector<unsigned char>::iterator del_end = del_beg + std::distance(child_beg_mid,child_end_mid) + std::distance(child_beg_end,child_end_end);
                                tree_ptr_first->data_vector().erase(del_beg,del_end);
                            }
                            else{
                                //delete middle data reference size from tree pointer first
                                std::vector<unsigned char>::iterator del_beg = tree_ptr_first->data_vector().begin()+std::distance(child_beg_beg,child_end_beg)+1;
                                std::vector<unsigned char>::iterator del_end = del_beg + std::distance(child_beg_mid,child_end_mid);
                                tree_ptr_first->data_vector().erase(del_beg,del_end);
                            }
                        }
                        else{
                            if(last_section_tree){
                                //delete last tree reference size from tree pointer middle
                                std::vector<unsigned char>::iterator del_beg = tree_ptr_mid->data_vector().begin()+std::distance(child_beg_mid,child_end_mid)+1;
                                std::vector<unsigned char>::iterator del_end = del_beg + std::distance(child_beg_end,child_end_end);
                                tree_ptr_mid->data_vector().erase(del_beg,del_end);
                            }
                            //else do not delete
                        }

                        return std::make_tuple(
                                (decltype(cur_tree->data_vector().size())) size_integrated,
                                (decltype(cur_tree->data_vector().size())) size_uncompressed,
                                (decltype(cur_tree->data_vector().size())) size_compressed,out_list,first_section_tree,last_section_tree,append_tree,total_match,tree_front_data_front_absolute);
                    }
                }
            };

            //cases for search index: its empty or it has content and with that a last tree element
            //cases for last tree if it exists, binary fit in: match from the beginning on, match in the middle, match until the end, total match
            //all lists contain lists with a last element that had multiple matches; add up all matches

            std::vector<std::tuple<std::size_t, std::size_t, std::size_t, std::list<std::tuple<std::list<tree_radix_custom *>,std::list<tree_radix_custom *>>>>> out_change_tuple_out{};

            auto single_beg = single_search.begin();
            while(single_beg!=search_index.end()){
                std::tuple<std::size_t, std::size_t, std::size_t, std::list<std::tuple<std::list<tree_radix_custom *>,std::list<tree_radix_custom *>>>> out_change_tuple{};

                auto search_element = std::get<0>(search_index).begin();//std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator>>>>, std::size_t>

                std::size_t binary_advance{};
                while (search_element != std::get<0>(*single_beg).end()) {//std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator>>>
                    //master list element contains a list containing tree pointers with vectors that totally matched, last element did not completely match anymore; inner list carries at least one element
                    auto one_node_analysis = search_element->end()-1;//last element did not match completely
                    //the inner list carries elements with a tuple holding the tree pointer and a list of valid matches that should be transformed into a sequence of trees
                    //we need to crawl the data of each tree and advance that many new trees until we would match the coming data offset of the original tree

                    if(std::get<1>(*one_node_analysis).empty()){
                        //delete the containing list, we are done here
                        std::get<0>(*single_beg).erase(one_node_analysis);
                        search_element = std::get<0>(*single_beg).begin();
                        continue;
                    }

                    tree_radix_custom* tree_match_pointer; = std::get<0>(*one_node_analysis);
                    std::set<tree_radix_custom*>modified{},added{};//changes to be written to disk in the form of tree pointers
                    std::vector<std::tuple<std::tuple<std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator,tree_radix_custom *> actively_changing_trees{};
                    std::for_each(std::get<1>(*one_node_analysis).begin(),std::get<1>(*one_node_analysis).end(),[&actively_changing_trees,&tree_match_pointer](auto &item){
                        actively_changing_trees.emplace_back(std::get<0>(item),std::get<1>(item),std::get<2>(item),tree_match_pointer);
                    })


                    auto match_beg = actively_changing_trees.begin();//                                         found beginning                             found end                        data on tree begin at found begin
                    auto match_beg_copy = match_beg;
                    while(match_beg != actively_changing_trees.end()){//update loop on std::vector<std::tuple<std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator>>
                        auto out_size = tree_building_sequence(std::get<3>(*match_beg),bin_beg,bin_end,std::get<0>(*match_beg),std::get<1>(*match_beg),std::get<2>(*match_beg));
                        std::get<0>(out_change_tuple)+=std::get<0>(out_size);
                        std::get<1>(out_change_tuple)+=std::get<1>(out_size);
                        std::get<2>(out_change_tuple)+=std::get<2>(out_size);
                        auto out_vector = std::get<3>(out_size);
                        bool first_section_tree = std::get<4>(out_size);
                        bool last_section_tree = std::get<5>(out_size);
                        bool append_tree = std::get<6>(out_size);
                        bool total_match = std::get<7>(out_size);
                        std::size_t tree_front_data_front_absolute = std::get<8>(out_size);

                        decltype(out_vector.begin()) first_tree_out,middle_tree_out,last_tree_out,append_tree_out;
                        first_section_tree,last_section_tree,append_tree,total_match;
                        if(first_section_tree){
                            first_tree_out = out_vector.begin();
                            middle_tree_out = first_tree_out+1;
                        }
                        else{
                            middle_tree_out = out_vector.begin();
                            first_tree_out = middle_tree_out;
                        }
                        if(last_section_tree){
                            last_tree_out = middle_tree_out+1;
                        }
                        else{
                            last_tree_out = middle_tree_out;
                        }
                        append_tree_out = last_tree_out+1;//end pointer or valid reference
                        //use sets to distinguish what nodes are added and what are modified; note that write back does not happen yet
                        std::size_t current_advance = std::distance(std::get<0>(*match_beg),std::get<1>(*match_beg))+1;
                        binary_advance+=current_advance;
                        std::size_t subtract_tree_data_offset{};//offset adjustment to other matches
                        //find out on what tree the data offset should move to
                        if(total_match){
                            if(append_tree){
                                added.emplace(*append_tree_out);
                            }
                            //this must be the only match so nothing happens
                        }
                        else{
                            if(first_section_tree){
                                modified.emplace(*first_tree_out);
                                added.emplace(*middle_tree_out);
                                //tree_match_pointer must move from first tree to middle tree, and we adjust offsets of all other matches
                                pointer_maintain(match_beg,std::get<1>(*one_node_analysis).end(),*first_tree_out,*middle_tree_out);//update current tree pointer
                            }
                            else{
                                modified.emplace(*middle_tree_out);
                            }
                            //only tree_match_pointer changes and offset changes to last tree if the end of the match exceeds limits and last tree exists
                            if(last_section_tree){
                                added.emplace(*last_tree_out);//section of inner list must be over due to incomplete match
                                //tree_match_pointer must move from middle tree to last tree, and we adjust offsets of all other matches
                                pointer_maintain(match_beg,std::get<1>(*one_node_analysis).end(),*middle_tree_out,*last_tree_out);//update current tree pointer
                            }
                            if(append_tree)added.emplace(*append_tree_out);
                        }
                        auto added_contains_it = [&added](&auto item){
                            return added.contains(item);
                        };
                        std::erase_if(modified,added_contains_it);
                        std::get<3>(out_change_tuple).emplace_back(modified,added);
                        modified.clear();
                        added.clear();
                        std::size_t vector_reset_dist = std::distance(match_beg_copy,match_beg);
                        match_beg=actively_changing_trees.begin()+vector_reset_dist+1;
                        match_beg_copy = actively_changing_trees.begin();
                    }
                    search_element++;
                }
                if(std::get<0>(out_change_list)>0)out_change_tuple_out.push_back(out_change_tuple);//only add change list if there was anything integrated

                //TODO: check add_test if there are total matches on search results skip them for analysis
                // optimize by checking equality of all search index paths and start search off from the last common tree node and the equivalent binary advance (2)
            }

            //deduplicate change list out

            return out_change_tuple_out;
        }
    private:
        //stack helper function for overlapping creation of trees
        void pointer_maintain(std::size_t tree_front_data_front_absolute,auto &actively_changing_trees,auto match_beg_intern_copy, auto match_end_intern,tree_radix_custom * tree_front,tree_radix_custom * tree_back){
            auto match_beg_intern = match_beg_intern_copy;
            if(match_beg_intern+1>=match_end_intern)return;
            //update matches offset
            auto first_data_offset = std::get<2>(*match_beg_intern);
            std::size_t first_match_size = std::distance(std::get<0>(*match_beg_intern),std::get<1>(*match_beg_intern))+1;
            std::size_t first_end = tree_front_data_front_absolute+first_match_size;
            while(match_beg_intern+1 != match_end_intern){
                std::size_t data_offset_between_start_and_current = std::distance(first_data_offset,std::get<2>(*(match_beg_intern+1)));
                std::size_t total_offset_front_for_this_node = tree_front_data_front_absolute+data_offset_between_start_and_current;
                if(data_offset_between_start_and_current<first_end){
                    //overlapping match, tree pointer stays at this tree, one to one reference tree front at tree_front_data_front_absolute; re-reference to data due to erase
                    std::size_t match_size = std::distance(std::get<0>(*(match_beg_intern+1)),std::get<1>(*(match_beg_intern+1)))+1;
                    std::size_t data_offset_between_start_and_last_node = std::distance(first_data_offset,std::get<2>(*(match_beg_intern)));
                    std::size_t total_offset_front_for_last_node = tree_front_data_front_absolute+data_offset_between_start_and_last_node;
                    if(data_offset_between_start_and_current+match_size>=first_end){
                        std::size_t match_size_last = std::distance(std::get<0>(*(match_beg_intern)),std::get<1>(*(match_beg_intern)))+1;
                        std::size_t total_offset_end_for_last_node = total_offset_front_for_last_node+match_size_last;

                        //recursive overlap reconstruction
                        std::get<1>(*(match_beg_intern+1)) -= total_offset_end_for_last_node-(total_offset_front_for_this_node+match_size);
                        std::size_t new_match_size = std::distance(std::get<0>(*(match_beg_intern+1)),std::get<1>(*(match_beg_intern+1)))+1;//first tree match
                        auto new_match_begin = std::get<1>(*(match_beg_intern+1))+1;
                        auto new_match_end = new_match_begin + (match_size-new_match_size);
                        //std::get<2>(*(match_beg_intern+1)) = tree_front->data_vector().begin()+total_offset_end_for_last_node;
                        //std::get<3>(*(match_beg_intern+1)) = tree_front;

                        decltype(*actively_changing_trees.begin()) new_partial_match = std::make_tuple(new_match_begin,
                                                                                                       new_match_end,
                                                                                                       tree_front->data_vector().begin()+(total_offset_front_for_this_node-(tree_front->data_vector().size()-new_match_size)),
                                                                                                       tree_front);//second tree match
                        // update until end
                        auto insert_beg = std::get<2>(match_beg_intern);
                        auto match_insert_check = match_beg_intern;
                        while(std::get<2>(match_insert_check)<std::get<2>(new_partial_match)&&match_insert_check!=match_end_intern)match_insert_check++;
                        actively_changing_trees.insert(match_insert_check,new_partial_match);

                        //vector pointer reset
                        std::size_t active_tree_offset = std::distance(actively_changing_trees.begin(),match_beg_intern_copy);
                        match_beg_intern = actively_changing_trees.begin()+active_tree_offset+std::distance(match_beg_intern_copy,match_beg_intern);
                        match_beg_intern_copy = actively_changing_trees.begin()+active_tree_offset;
                    }
                    else{
                        //simple build in tree front
                        std::get<2>(*(match_beg_intern+1)) = tree_front->data_vector().begin()+total_offset_front_for_this_node/*-distance from the last node until the current node start*/;
                        //std::get<3>(*(match_beg_intern+1)) = tree_front;
                    }
                }
                else{
                    //total match, new calculated reference on tree_back
                    std::get<2>(*(match_beg_intern+1)) = tree_back->data_vector().begin()+(total_offset_front_for_this_node-tree_front->data_vector().size());
                    std::get<3>(*(match_beg_intern+1)) = tree_back;
                }
                //TODO: if last match is a subset from the front of the next match, advance found_beg of the next match behind the prefix (minimum prefix length is match size); if the next match turns zero in size, delete it
                match_beg_intern++;
            }
        };
    public:
        template<class ContainerType>
        std::tuple<std::size_t, std::size_t, std::size_t> add_test(const ContainerType &c) {
            return add_test(c.begin(), c.end());
        }

        //returns total size integrated, new space used uncompressed, new space used compressed
        //calculates ESTIMATE size of data to be integrated to be communicated to agency to determine optimal storage location
        std::tuple<std::size_t, std::size_t, std::size_t>
        add_test(std::vector<unsigned char>::iterator &bin_beg,std::vector<unsigned char>::iterator &bin_end) {
            //first search existing structure and add into the last tree to insert potentially missing information
            std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator>>>>>, std::size_t>> search_index = search(
                    bin_beg, bin_end);
            //uncompressed input
            if (bin_beg == bin_end || std::distance(bin_beg, bin_end) < 1){
                return {};
            }
            //some element and an end element at least required

            auto tree_test_sequence = [&](tree_radix_custom *cur_tree, std::vector<unsigned char>::iterator bin_beg_incoming, std::vector<unsigned char>::iterator bin_end_incoming,
                                         std::vector<unsigned char>::iterator bin_beg_found, std::vector<unsigned char>::iterator bin_end_found,
                                         const std::vector<unsigned char>::iterator data_beg_intern) {
                std::size_t matched_size = std::distance(bin_beg_incoming, bin_end_found);
                //checking if children need to be generated before and after the found input peace, reference to data of tree required
                //child before found, reference data
                std::vector<unsigned char>::iterator child_beg_beg = cur_tree->data_vector().begin();
                std::vector<unsigned char>::iterator child_end_beg = std::max(data_beg_intern - 1, child_beg_beg);
                //child data sequence middle, reference data
                std::vector<unsigned char>::iterator child_beg_mid = data_beg_intern;
                std::vector<unsigned char>::iterator child_end_mid = std::min(cur_tree->data_vector().end(),
                                              child_beg_mid + std::distance(bin_beg_found, bin_end_found));
                //child data sequence end, reference data
                std::vector<unsigned char>::iterator child_beg_end = std::min(cur_tree->data_vector().end(), child_end_mid + 1);
                std::vector<unsigned char>::iterator child_end_end = cur_tree->data_vector().end();
                //child after found, reference new input
                std::vector<unsigned char>::iterator child_beg_append = std::min(bin_end_found + 1, bin_end_incoming);
                std::vector<unsigned char>::iterator child_end_append = bin_end_incoming;
                //only the new append part may be compressed
                //before splitting or modifying a block it needs to be uncompressed

                bool first_section_tree = std::distance(child_beg_beg, child_end_beg) > 1;
                bool last_section_tree =
                        child_beg_end == child_end_end && child_end_end == cur_tree->data_vector().end();
                bool append_tree =
                        std::distance(child_beg_append, child_end_append) > 0 && child_beg_append != bin_end_incoming;
                bool total_match = !first_section_tree && !last_section_tree &&
                                   std::distance(child_beg_mid, child_end_mid) == cur_tree->data_vector().size();

                uh::util::compression_custom comp{};

                //search function already determined that this is the tree that needs to fill in the data or to split somehow
                //cases: insert front tree, insert end tree (same case as having a back insert because the end tree will just have at least 1 element in case of overflow)
                if (cur_tree->data_vector().empty()) {//how to insert, either empty simple insert or some tree construction anywhere
                    //simple insert into data since this seems to be a new node that can contain simple information
                    cur_tree->data_vector() = std::vector<unsigned char>{bin_beg_found, bin_end_found};
                    return std::make_tuple(std::distance(bin_beg, bin_end), std::distance(bin_beg, bin_end),
                                           comp.compress(bin_beg_found, bin_end_found).size());
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
                                    (decltype(cur_tree->data_vector().size())) comp.compress(
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
                                    (decltype(cur_tree->data_vector().size())) comp.compress(
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

            //cases for search index: its empty or it has content and with that a last tree element
            //cases for last tree if it exists, binary fit in: match from the beginning on, match in the middle, match until the end, total match
            //all lists contain lists with a last element that had multiple matches; add up all matches
            std::vector<std::tuple<std::size_t, std::size_t, std::size_t>>add_tup_out{};
            for(const single_route:search_index){
                std::tuple<std::size_t, std::size_t, std::size_t>add_tup{};
                auto outer_most_level = [&](auto list) {
                    auto inner_list_level = [&](auto tree_tuple) {
                        for (const auto &pos_tup: std::get<1>(tree_tuple)) {
                            auto last_tree = std::get<0>(single_route).back();
                            //check if we have a full match and the input is larger than the data of the last tree
                            auto add_list = tree_test_sequence(std::get<0>(last_tree), bin_beg, bin_end,
                                                               std::get<0>(pos_tup), std::get<1>(pos_tup),
                                                               std::get<2>(pos_tup));//insert into another tree
                            std::get<0>(add_tup) += std::get<0>(add_list);
                            std::get<1>(add_tup) += std::get<1>(add_list);
                            std::get<2>(add_tup) += std::get<2>(add_list);
                        }
                    };
                    std::for_each(list.begin(), list.end(), inner_list_level);
                };
                std::for_each(single_route.begin(),single_route.end(),outer_most_level);

                if (std::get<0>(single_route).empty() && std::get<1>(single_route) == 0) {
                    auto append_list = tree_test_sequence(this, bin_beg, bin_beg, data.begin(), data.end(),
                                                          data.end());//insert into this tree, no matches, only first character must match
                    std::get<0>(add_tup) += std::get<0>(append_list);
                    std::get<1>(add_tup) += std::get<1>(append_list);
                    std::get<2>(add_tup) += std::get<2>(append_list);
                }
            }

            return add_tup_out;
        }

        //returns the path of maximum fit and the match size
        std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator>>>>>, std::size_t>>
                search(std::vector<unsigned char>::iterator &bin_beg, std::vector<unsigned char>::iterator &bin_end,
                       std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator>>>>>, std::size_t>> input_list =
                       std::vector<std::tuple<std::list<std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator>>>>>, std::size_t>>{},
                       std::vector<tree_radix_custom*> limiter_children = std::vector<tree_radix_custom*>{}) {
            if (bin_beg == bin_end || std::any_of(limiter_children.begin(),limiter_children.end(),[this](auto &item_limit){
                return item_limit == this;
            })) {
                return input_list;
            }

            auto vanilla_match_last_tree = [&](std::vector<unsigned char>::iterator data_beg, std::vector<unsigned char>::iterator data_end, std::vector<unsigned char>::iterator bin_beg, std::vector<unsigned char>::iterator bin_end) {
                auto local_matches = compare_ultihash(data_beg, data_end, bin_beg, bin_end);
                //LEGAL MATCH FILTER
                //on empty or partial match make new list in list, else append the match results on total match
                bool legal_split;
                //if the end was matched too long we can do something about that, but else the algorithm is prefix oriented
                bool end_size, begin_reached, end_reached;
                //control
                bool broken_legal = false;
                auto legal_check = [&local_matches, &legal_split, &end_size, &begin_reached, &end_reached, &broken_legal](
                        std::vector<unsigned char>::iterator data_beg, std::vector<unsigned char>::iterator data_end, auto current_match) {
                    do {
                        bool start_size = MINIMUM_MATCH_SIZE <= std::distance(data_beg, std::get<0>(*current_match));
                        end_size = MINIMUM_MATCH_SIZE <= std::distance(std::get<0>(*current_match) +
                                                                       std::distance(std::get<1>(*current_match),
                                                                                     std::get<2>(*current_match)),
                                                                       data_end);
                        bool total_found_size = MINIMUM_MATCH_SIZE <= std::distance(std::get<1>(*current_match),
                        std::get<2>(*current_match));
                        begin_reached = data_beg == std::get<0>(*current_match);
                        end_reached = data_end == std::get<0>(*current_match) +
                                                  std::distance(std::get<1>(*current_match),
                                                                std::get<2>(*current_match));
                        legal_split = ((start_size || begin_reached) && (end_size || end_reached) &&
                                       total_found_size);//legal if on split there cannot be a segment that is smaller than the match size
                        if (!end_size && !end_reached) {
                            std::get<2>(*current_match)--;
                            if (std::distance(std::get<1>(*current_match) < std::get<2>(*current_match)) <
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
                while (match_beg != local_matches.end()) {
                    legal_check(data_beg, data_end, match_beg);
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
                std::sort(local_matches.begin(), local_matches.end(), [&data_beg](auto &a, auto &b) {
                    return std::distance(data_beg, std::get<0>(a)) < std::distance(data_beg, std::get<0>(b));
                });

                if (local_matches.empty())return std::vector<decltype(input_list)>{input_list};//TODO: any match must have a distance of MINIMUM_MATCH_SIZE and front AND end to all the other matches

                std::vector<decltype(input_list)> out_possibilities{};//TODO: index list was not fitting to vector adjustment to multiple solutions

                auto match_beginning = local_matches.begin();
                while (match_beginning != local_matches.end()) {
                    auto input_list_tmp = input_list;
                    legal_check(data_beg, data_end, match_beginning);

                    std::vector<std::tuple<std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator>> found_vec{};
                    found_vec.emplace_back(std::get<1>(*match_beginning), std::get<2>(*match_beginning),
                                           std::get<0>(*match_beginning));

                    if (input_list_tmp.empty() || !legal_split) {
                        std::list<std::tuple<tree_radix_custom *, std::vector<std::tuple<std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator, std::vector<unsigned char>::iterator>>>> tmp_list{};
                        tmp_list.emplace_back(this, found_vec);
                        std::get<0>(input_list_tmp).push_back(tmp_list);
                    } else {
                        //if the tree element of the last element is still the same as "this" we append to the vector, else we append a new list
                        auto last_it_outer_list = (--(std::get<0>(input_list_tmp).end()));
                        auto last_it_inner_list = (--(last_it_outer_list->end()));
                        if (std::get<0>(*last_it_inner_list) == this) {//check if tree pointer is the same
                            std::get<1>(*last_it_inner_list).push_back(found_vec[0]);
                        } else {
                            last_it_outer_list->emplace_back(this, found_vec);
                        }
                    }
                    std::get<1>(input_list_tmp) += std::distance(std::get<1>(*match_beginning),
                                                                 std::get<2>(*match_beginning));
                    out_possibilities.push_back(input_list_tmp);
                    match_beginning++;
                }
                return out_possibilities;
            };

            auto possibilities_init = vanilla_match_last_tree(data.begin(), data.end(), bin_beg, bin_end);
            auto possibilities = std::vector<std::tuple<decltype(possibilities), std::vector<unsigned char>::iterator,std::vector<unsigned char>::iterator, bool>>{};//check if the possibility was already checked and save the worked on binary input offset and data offset
            for (const auto &item: possibilities_init) {
                possibilities.emplace_back(item, data.begin(), bin_beg, false);
            }
            possibilities_init.clear();

            auto pos_begin = possibilities.begin();
            std::size_t delete_count{};

            while (!possibilities.empty()) {
                if (std::get<3>(*pos_begin)) {
                    delete_count++;
                    continue;
                }
                if (delete_count > 0) {
                    possibilities.erase(possibilities.begin(), possibilities.begin() + (delete_count - 1));
                    pos_begin = possibilities.begin();
                    delete_count = 0;
                    continue;
                }
                std::get<3>(*pos_begin) = true;

                std::vector<unsigned char>::iterator data_beg_tmp = std::get<1>(*pos_begin);
                std::vector<unsigned char>::iterator bin_beg_tmp = std::get<2>(*pos_begin);

                bool first_time = true;
                bool pos_begin_reset = false;
                while (data_beg_tmp != std::get<1>(*pos_begin) || bin_beg_tmp != std::get<2>(*pos_begin) ||
                       first_time) {
                    if (std::get<1>(*pos_begin) >= data.end())break;
                    if (std::get<2>(*pos_begin) >= bin_end)break;
                    first_time = false;

                    //do work on matching again for subset of data and input
                    //after various match cases as various positions
                    possibilities_init = vanilla_match_last_tree(std::get<1>(*pos_begin), data.end(),
                                                                 std::get<2>(*pos_begin), bin_end);//search here
                    for (const auto &item: possibilities_init) {
                        possibilities.emplace_back(item, std::get<1>(*pos_begin), std::get<2>(*pos_begin), false);
                    }
                    possibilities_init.clear();

                    //check child that deals with searching the far most rest in direction of end to skip the not matching rest
                    auto child_vec = child_vector(*std::get<2>(*pos_begin));

                    if (!child_vec.empty()) {//recursive search
                        for (const auto &item: child_vec) {
                            for (const auto &item2: (item->search(std::get<2>(*pos_begin), bin_end,
                                                                 std::get<0>(*pos_begin)))) {
                                for(const auto &route:item2){
                                    possibilities.emplace_back(route, std::get<1>(*pos_begin), std::get<2>(*pos_begin),
                                                               false);
                                }
                            }
                        }
                    }

                    if (!std::get<0>(std::get<0>(*pos_begin)).empty()) {
                        auto last_it_outer_list = (--(std::get<0>(std::get<0>(*pos_begin)).end()));
                        auto last_it_inner_list = (--(last_it_outer_list->end()));

                        if (std::get<0>(*last_it_inner_list) ==
                            this) {//check if tree pointer is the same of the last element, so we can continue to append results
                            //we still found a match on this data so continue advancing data and binary beginning
                            //set data begin to the position where a subset of the input was found to make sure it advances >=0
                            auto last_position_tuple = std::get<1>(*last_it_inner_list).back();
                            std::get<1>(*pos_begin) = std::get<2>(last_position_tuple) +
                                                      1;//if we do not skip by the minimum match size, we may ultra fragment data
                            std::size_t matched_size =
                                    std::distance(std::get<0>(last_position_tuple), std::get<1>(last_position_tuple)) + 1;
                            std::get<2>(*pos_begin) += matched_size;
                            std::get<1>(std::get<0>(*pos_begin)) += matched_size;
                        }
                    }

                    //total match optimization to get it to front
                    std::sort(possibilities.begin(), possibilities.end(), [](auto &a, auto &b) {
                        return std::get<1>(std::get<0>(a)) >
                               std::get<1>(std::get<0>(b));//sort in descending order on search match size
                    });

                    //if first possibility is same length as binary input, we have a total match and return
                    if (std::get<1>(std::get<0>(possibilities[0])) == std::distance(std::get<2>(*pos_begin), bin_end)) {
                        return std::get<0>(possibilities[0]);
                    }

                    if (!std::get<3>(*pos_begin)) {
                        pos_begin = possibilities.begin();
                        pos_begin_reset = true;
                        break;
                    }
                }
                if (pos_begin_reset)continue;
                pos_begin++;
            }

            if (possibilities.empty())return input_list;

            //return the largest match with the lowest offset on the last tree, as far as there is a last tree...
            std::sort(possibilities.begin(), possibilities.end(), [](auto &a, auto &b) {
                return std::get<1>(std::get<0>(a)) >
                       std::get<1>(std::get<0>(b));//sort in descending order on search match size
            });

            std::size_t max_val{};
            auto poss_beg = possibilities.begin();
            while (poss_beg != possibilities.end()) {
                max_val = std::max(max_val, std::get<1>(std::get<0>(*poss_beg)));
                if (std::get<1>(*poss_beg) < max_val) {
                    possibilities.erase(poss_beg, possibilities.end());
                    break;
                }
                poss_beg++;
            }

            std::sort(possibilities.begin(), possibilities.end(), [](auto &a, auto &b) {
                return std::get<0>(std::get<0>(a)).size() >
                       std::get<1>(std::get<0>(b)).size();//sort in descending order on search match size
            });

            std::vector<decltype(std::get<0>(possibilities[0]))> multi_routing{};
            std::for_each(possibilities.begin(), possibilities.end(), [&multi_routing](auto &item){
                multi_routing.push_back(std::get<0>(item));
            });

            return multi_routing;
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
