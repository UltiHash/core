//
// Created by benjamin-elias on 17.12.22.
//

#ifndef UHLIBCOMMON_TREE_RADIX_CUSTOM_H
#define UHLIBCOMMON_TREE_RADIX_CUSTOM_H

#include "conceptTypes/conceptTypes.h"
#include "logging/logging_boost.h"
#include <shared_mutex>

namespace uh::trees{
#define N 256
    typedef struct tree_radix_custom tree_radix_custom;

    struct tree_radix_custom {
    protected:
        tree_radix_custom* children[N]{};
        char* data{}; // Storing for printing purposes only
        std::size_t length{};
    public:
        tree_radix_custom() {
            for(auto & i : children){
                i = nullptr;
            }
        }

        tree_radix_custom(const char *bin, std::size_t len):tree_radix_custom(){
            add(bin,len);
        }

        tree_radix_custom(const std::string &s):tree_radix_custom(){
            add(s.data(),s.length());
        }

        std::size_t size(){
            return length;
        }

        char* data_blob(){
            return data;
        }

        std::list<tree_radix_custom *>
        add(const char *bin, std::size_t len, std::list<tree_radix_custom *> enlist = std::list<tree_radix_custom *>{}) {
            if(len>0) {
                if (length == 0) {
                    if (!has_children()) {
                        if (data != nullptr)std::free(data);
                        data = static_cast<char *>(malloc(sizeof(char) * len));
                        std::memcpy(data, bin, len);
                        length = len;
                        enlist.push_back(this);
                        return enlist;
                    } else {
                        if(children[(unsigned char)bin[0]] == nullptr){
                            // no match, create new node for rest of string
                            children[(unsigned char)bin[0]] = (struct tree_radix_custom*) std::malloc(sizeof(struct tree_radix_custom));
                            new (children[(unsigned char)bin[0]]) tree_radix_custom();
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
                    for(; i < std::min(length,len)-1; i++){
                        if(data[i] != bin[i])break;
                    }
                    if(i == length-1){
                        if(len>length){
                            //insert deeper node directly on rest
                            if(children[(unsigned char)bin[i+1]] == nullptr){
                                auto* tmp = (struct tree_radix_custom*) std::malloc(sizeof(struct tree_radix_custom));
                                new (tmp) tree_radix_custom();
                                children[(unsigned char)bin[i+1]] = tmp;
                                enlist.push_back(this);
                                return tmp->add(bin+length,len-length,enlist);
                            }
                            else{
                                enlist.push_back(children[(unsigned char)bin[i+1]]);
                                return children[(unsigned char)bin[i+1]]->add(bin+length,len-length,enlist);
                            }
                        }
                        // direct match, direct redirect
                        enlist.push_back(this);
                        return enlist;
                    }
                    if(i < length - 1){
                        // match string is too short -> split node
                        // TODO: notifiy changes in case of depending tree
                        char* higher_node = (char*) std::malloc(i*sizeof(char));
                        std::memcpy(higher_node,data,i);
                        length = i;
                        char* lower_node = (char*) std::malloc((length-i)*sizeof(char));
                        std::memcpy(lower_node,data+i+1,length-i);
                        std::free(data);
                        data = higher_node;
                        auto* tmp = (struct tree_radix_custom*) std::malloc(sizeof(struct tree_radix_custom));
                        new (tmp) tree_radix_custom();
                        std::memcpy(tmp->children,this->children,N * sizeof(tree_radix_custom*));
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
            return enlist;
        }

        tree_radix_custom *copy() {
            auto* tmp = (struct tree_radix_custom*) std::malloc(sizeof(struct tree_radix_custom));
            new (tmp) tree_radix_custom();
            std::memcpy(tmp->children,this->children,N * sizeof(tree_radix_custom*));
            tmp->data = (char*) std::malloc(length*sizeof(char));
            std::memcpy(tmp->data,data,length);
            tmp->length=length;
            return tmp;
        }

        tree_radix_custom *copy_recursive() {
            auto* tmp = copy();
            for(unsigned char i=0;;i++){
                if(children[i] != nullptr){
                    tmp->children[i] = children[i]->copy_recursive();
                }
                if(i==(unsigned char)N)break;
            }
            return tmp;
        }

        void destroy_recursive() {
            for(auto & i : children){
                if(i != nullptr){
                    i -> destroy_recursive();
                    delete i;
                }
            }
        }

        void destroy_recursive(char sub) {
            if(children[sub] != nullptr){
                children[sub] -> destroy_recursive();
                std::free(children[sub] -> data);
                std::free(children[sub]);
                children[sub] = nullptr;
            }
        }

        bool has_children() {
            bool has_children = false;
            for(const auto & i : children){
                if(i != nullptr){
                    has_children = true;
                    break;
                }
            }
            return has_children;
        }

        void insert(tree_radix_custom *in) {
            std::list<std::tuple<tree_radix_custom*,unsigned char>> concat_string;
            concat_string.emplace_back(in,0);
            while(!concat_string.empty()){
                bool has_children = false;
                for(unsigned char &i=std::get<1>(concat_string.back());;i++){
                    if(std::get<0>(concat_string.back())->children[i] != nullptr){
                        has_children=true;
                        concat_string.emplace_back(std::get<0>(concat_string.back())->children[i],0);
                        break;
                    }
                    if(i==(unsigned char)N)break;
                }
                if(!has_children){
                    std::size_t concat_size{},start_step{};
                    std::for_each(concat_string.cbegin(),concat_string.cend(),[&concat_size](auto in){
                        concat_size+=std::get<0>(in) -> length;
                    });
                    char* concat_sequence = (char*) std::malloc(concat_size * sizeof(char));
                    std::for_each(concat_string.cbegin(),concat_string.cend(),[&concat_sequence,&start_step](auto in){
                        std::memcpy(concat_sequence+start_step,std::get<0>(in) -> data,std::get<0>(in) -> length);
                        start_step+=std::get<0>(in) -> length;
                    });
                    (void) add(concat_sequence,concat_size);
                    std::free(concat_sequence);
                    concat_string.pop_back();
                }
            }
        }

        std::tuple<std::list<tree_radix_custom *>, std::size_t>
        search(const char *bin, std::size_t len,
                                             std::tuple<std::list<tree_radix_custom *>, std::size_t> enlist) {
            if(len>0){
                if(length == 0){
                    if(!has_children() || children[(unsigned char)bin[0]] == nullptr){
                        return enlist;
                    }
                    else{
                        std::get<0>(enlist).push_back(this);
                        return children[(unsigned char)bin[0]] -> search(bin,len,enlist);
                    }
                }
                else{
                    std::size_t i = 0;
                    for(; i < std::min(length,len); i++){
                        if(data[i] != bin[i])break;
                    }
                    if(i == length-1){
                        if(len>length && children[(unsigned char)bin[i+1]] != nullptr){
                            std::get<0>(enlist).push_back(children[(unsigned char)bin[i+1]]);
                            std::get<1>(enlist)+=length;
                            return children[(unsigned char)bin[i+1]]->search(bin+std::get<1>(enlist),length-i,enlist);
                        }
                        // direct match, direct redirect
                        std::get<0>(enlist).push_back(this);
                        std::get<1>(enlist)+=length;
                        return enlist;
                    }
                    if(i < length - 1){
                        if(i){
                            std::get<0>(enlist).push_back(this);
                            std::get<1>(enlist)+=i+1;
                        }
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
