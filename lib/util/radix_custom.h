//
// Created by benjamin-elias on 17.12.22.
//

#ifndef UHLIBCOMMON_RADIX_CUSTOM_H
#define UHLIBCOMMON_RADIX_CUSTOM_H


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

        radix_custom* copy(){
            auto* tmp = (struct radix_custom*) std::malloc(sizeof(struct radix_custom));
            new (tmp) radix_custom();
            std::memcpy(tmp->children,this->children,N * sizeof(radix_custom*));
            tmp->data = (char*) std::malloc(length*sizeof(char));
            std::memcpy(tmp->data,data,length);
            tmp->length=length;
            return tmp;
        }

        radix_custom* copy_recursive(){
            auto* tmp = copy();
            for(unsigned char i=0;i<(unsigned char)N;i++){
                if(children[i] != nullptr){
                    tmp->children[i] = children[i]->copy_recursive();
                }
            }
            return tmp;
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
        //TODO: insert radix tree and search and check search method and prefix pointer
    };
}

#endif //UHLIBCOMMON_RADIX_CUSTOM_H
