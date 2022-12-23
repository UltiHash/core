//
// Created by benjamin-elias on 17.12.22.
//

#ifndef UHLIBCOMMON_TREE_RADIX_CUSTOM_H
#define UHLIBCOMMON_TREE_RADIX_CUSTOM_H

#include "conceptTypes/conceptTypes.h"
#include "logging/logging_boost.h"
#include <shared_mutex>

namespace uh::trees {
#define N 256
#define MIN_REDUCTION_STRING 8
    typedef struct tree_radix_custom tree_radix_custom;

    struct tree_radix_custom {
    protected:
        tree_radix_custom *children[N]{};
        std::vector<unsigned char> data{};//any binary vector string
    public:
        tree_radix_custom() {
            for (auto &i: children) {
                i = nullptr;
            }
        }

        explicit tree_radix_custom(std::vector<unsigned char> &bin) : tree_radix_custom() {
            add(bin);
        }

        [[nodiscard]] std::size_t size() const {
            return data.size();
        }

        std::vector<unsigned char> data_blob() {
            return data;
        }

        tree_radix_custom *child(char i) {
            return children[(unsigned char) i];
        }

        //add some string into the radix tree, returning the tree nodes where it was compressed and stored along the way
        std::tuple<std::list<tree_radix_custom *>>//TODO: within the output list either return the radix tree pointer or an offset+choose_tree descriptor to jump over a certain section of string
        //TODO: inter-link only to string sections that are larger equal to the current string size, but only for data tree
        add(std::vector<unsigned char> &bin,
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

        /*
         * insert another node and all children strings to this root node, scan through all combined strings,
         * that are formed from the root to every child node; and finally copy all contents of the incoming node to "this"
         */
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

        /*
         * search through this node and return the matching pathway and the depth until the incoming string fit the
         * internals of the node
         */
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

        ~tree_radix_custom() {
            std::free(data);
        }
    };
}

#endif //UHLIBCOMMON_TREE_RADIX_CUSTOM_H
