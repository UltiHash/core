//
// Created by benjamin-elias on 17.12.22.
//

#ifndef UHLIBCOMMON_RADIX_CUSTOM_H
#define UHLIBCOMMON_RADIX_CUSTOM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The number of children for each node
// We will construct a N-ary tree and make it
// a Trie
// Since we have 26 english letters, we need
// 26 children per node

#include "conceptTypes/conceptTypes.h"
#include "logging/logging_boost.h"
#include <shared_mutex>

namespace uh::util{
#define N 256
    typedef struct radix_custom radix_custom;

    struct radix_custom {
    protected:
        radix_custom* children[N]{};
        char* data{}; // Storing for printing purposes only
        std::size_t length{};
        std::shared_mutex local_m{};
    public:
        radix_custom(){
            for(auto & i : children){
                i = nullptr;
            }
        };

        std::list<radix_custom*> add(const char* bin,std::size_t len,std::list<radix_custom*> enlist = std::list<radix_custom*>{}){
            if(len>0) {
                std::shared_lock lock(local_m);
                if (length == 0) {
                    bool has_children = false;
                    for(const auto & i : children){
                        if(i != nullptr){
                            has_children = true;
                            break;
                        }
                    }
                    if (!has_children) {
                        if (data != nullptr)std::free(data);
                        data = static_cast<char *>(malloc(sizeof(char) * len));
                        std::memcpy(data, bin, len);
                        length = len;
                    } else {
                        if(children[(unsigned char)bin[0]] == nullptr){
                            // no match, create new node for rest of string
                            children[(unsigned char)bin[0]] = (struct radix_custom*) std::malloc(sizeof(struct radix_custom));
                            new (children[(unsigned char)bin[0]]) radix_custom();
                            auto enlist_append = children[(unsigned char)bin[0]] -> add(bin,len,enlist);
                            enlist.push_back(children[(unsigned char)bin[0]]);
                            enlist.splice(enlist.end(),enlist_append);
                            return enlist;
                        }
                        else{
                            enlist.push_back(children[(unsigned char)bin[0]]);
                            return children[(unsigned char)bin[0]]->add(bin,len,enlist);
                        }
                    }
                } else {
                    std::size_t i = 0;
                    for(; i < std::min(length,len); i++){
                        if(data[i] != bin[i])break;
                    }
                    if(i == length-1){
                        if(len>length){
                            //insert deeper node directly on rest
                            if(children[(unsigned char)bin[i+1]] == nullptr){
                                auto* tmp = (struct radix_custom*) std::malloc(sizeof(struct radix_custom));
                                new (tmp) radix_custom();
                                children[(unsigned char)bin[i+1]] = tmp;
                                enlist.push_back(tmp);
                                return tmp->add(bin+i+1,length-i,enlist);
                            }
                            else{
                                enlist.push_back(children[(unsigned char)bin[i+1]]);
                                return children[(unsigned char)bin[i+1]]->add(bin+i+1,length-i,enlist);
                            }
                        }
                        // direct match, direct redirect
                        enlist.push_back(this);
                        return enlist;
                    }
                    if(i < length - 1){
                        // match string is too short -> split node
                        // TODO: notifiy changes
                        char* higher_node = (char*) std::malloc(i*sizeof(char));
                        std::memcpy(higher_node,data,i);
                        length = i;
                        char* lower_node = (char*) std::malloc((length-i)*sizeof(char));
                        std::memcpy(lower_node,data+i+1,length-i);
                        std::free(data);
                        data = higher_node;
                        auto* tmp = (struct radix_custom*) std::malloc(sizeof(struct radix_custom));
                        new (tmp) radix_custom();
                        std::memcpy(tmp->children,this->children,N * sizeof(radix_custom*));
                        tmp->data = lower_node;
                        tmp->length = length-i;
                        for(auto & i1 : children){
                            i1 = nullptr;
                        }
                        children[(unsigned char)lower_node[0]] = tmp;
                        enlist.push_back(tmp);
                        enlist.push_back(this);
                        return enlist;
                    }
                }
            }
        }

        void destroy_recursive(){
            for(auto & i : children){
                if(i != nullptr){
                    i -> destroy_recursive();
                    delete i;
                }
            }
        }

        void destroy_recursive(char sub){
            if(children[sub] != nullptr){
                children[sub] -> destroy_recursive();
                std::free(children[sub] -> data);
                std::free(children[sub]);
                children[sub] = nullptr;
            }
        }

        ~radix_custom(){
            std::free(data);
        }
    };
}

/*
radix_custom* make_radix_custom(char data) {
    // Allocate memory for a radix_custom
    auto* node = (radix_custom*) calloc (1, sizeof(radix_custom));
    for (auto & i : node->children)
        i = nullptr;
    node->is_leaf = 0;
    node->data = data;
    return node;
}

void free_radix_custom(radix_custom* node) {
    // Free the radix_custom sequence
    for(auto & i : node->children) {
        if (i != nullptr) {
            free_radix_custom(i);
        }
        else {
            continue;
        }
    }
    free(node);
}

radix_custom* insert_trie(radix_custom* root, const char* word) {
    // Inserts the word onto the Trie
    // ASSUMPTION: The word only has lower case characters
    radix_custom* temp = root;

    for (int i=0; word[i] != '\0'; i++) {
        // Get the relative position in the alphabet list
        int idx = (int) word[i] - 'a';
        if (temp->children[idx] == nullptr) {
            // If the corresponding child doesn't exist,
            // simply create that child!
            temp->children[idx] = make_radix_custom(word[i]);
        }
        else {
            // Do nothing. The node already exists
        }
        // Go down a level, to the child referenced by idx
        // since we have a prefix match
        temp = temp->children[idx];
    }
    // At the end of the word, mark this node as the leaf node
    temp->is_leaf = 1;
    return root;
}

int search_trie(radix_custom* root, const char* word)
{
    // Searches for word in the Trie
    radix_custom* temp = root;

    for(int i=0; word[i]!='\0'; i++)
    {
        int position = word[i] - 'a';
        if (temp->children[position] == nullptr)
            return 0;
        temp = temp->children[position];
    }
    if (temp != nullptr && temp->is_leaf == 1)
        return 1;
    return 0;
}

int check_divergence(radix_custom* root, char* word) {
    // Checks if there is branching at the last character of word
    // and returns the largest position in the word where branching occurs
    radix_custom* temp = root;
    int len = strlen(word);
    if (len == 0)
        return 0;
    // We will return the largest index where branching occurs
    int last_index = 0;
    for (int i=0; i < len; i++) {
        int position = word[i] - 'a';
        if (temp->children[position]) {
            // If a child exists at that position
            // we will check if there exists any other child
            // so that branching occurs
            for (int j=0; j<N; j++) {
                if (j != position && temp->children[j]) {
                    // We've found another child! This is a branch.
                    // Update the branch position
                    last_index = i + 1;
                    break;
                }
            }
            // Go to the next child in the sequence
            temp = temp->children[position];
        }
    }
    return last_index;
}

char* find_longest_prefix(radix_custom* root, char* word) {
    // Finds the longest common prefix substring of word
    // in the Trie
    if (!word || word[0] == '\0')
        return nullptr;
    // Length of the longest prefix
    int len = strlen(word);

    // We initially set the longest prefix as the word itself,
    // and try to back-tracking from the deepst position to
    // a point of divergence, if it exists
    char* longest_prefix = (char*) calloc (len + 1, sizeof(char));
    for (int i=0; word[i] != '\0'; i++)
        longest_prefix[i] = word[i];
    longest_prefix[len] = '\0';

    // If there is no branching from the root, this
    // means that we're matching the original string!
    // This is not what we want!
    int branch_idx  = check_divergence(root, longest_prefix) - 1;
    if (branch_idx >= 0) {
        // There is branching, We must update the position
        // to the longest match and update the longest prefix
        // by the branch index length
        longest_prefix[branch_idx] = '\0';
        longest_prefix = (char*) realloc (longest_prefix, (branch_idx + 1) * sizeof(char));
    }

    return longest_prefix;
}

int is_leaf_node(radix_custom* root, const char* word) {
    // Checks if the prefix match of word and root
    // is a leaf node
    radix_custom* temp = root;
    for (int i=0; word[i]; i++) {
        int position = (int) word[i] - 'a';
        if (temp->children[position]) {
            temp = temp->children[position];
        }
    }
    return temp->is_leaf;
}

radix_custom* delete_trie(radix_custom* root, char* word) {
    // Will try to delete the word sequence from the Trie only it
    // ends up in a leaf node
    if (!root)
        return nullptr;
    if (!word || word[0] == '\0')
        return root;
    // If the node corresponding to the match is not a leaf node,
    // we stop
    if (!is_leaf_node(root, word)) {
        return root;
    }
    radix_custom* temp = root;
    // Find the longest prefix string that is not the current word
    char* longest_prefix = find_longest_prefix(root, word);
    //printf("Longest Prefix = %s\n", longest_prefix);
    if (longest_prefix[0] == '\0') {
        free(longest_prefix);
        return root;
    }
    // Keep track of position in the Trie
    int i;
    for (i=0; longest_prefix[i] != '\0'; i++) {
        int position = (int) longest_prefix[i] - 'a';
        if (temp->children[position] != nullptr) {
            // Keep moving to the deepest node in the common prefix
            temp = temp->children[position];
        }
        else {
            // There is no such node. Simply return.
            free(longest_prefix);
            return root;
        }
    }
    // Now, we have reached the deepest common node between
    // the two strings. We need to delete the sequence
    // corresponding to word
    size_t len = strlen(word);
    for (; i < len; i++) {
        int position = (int) word[i] - 'a';
        if (temp->children[position]) {
            // Delete the remaining sequence
            radix_custom* rm_node = temp->children[position];
            temp->children[position] = nullptr;
            free_radix_custom(rm_node);
        }
    }
    free(longest_prefix);
    return root;
}

void print_trie(radix_custom* root) {
    // Prints the nodes of the trie
    if (!root)
        return;
    radix_custom* temp = root;
    printf("%c -> ", temp->data);
    printf("\n ", temp->data);
    for (auto & i : temp->children) {
        print_trie(i);
    }
}

void print_search(radix_custom* root, char* word) {
    printf("Searching for %s: ", word);
    if (search_trie(root, word) == 0)
        printf("Not Found\n");
    else
        printf("Found!\n");
}
*/

#endif //UHLIBCOMMON_RADIX_CUSTOM_H
