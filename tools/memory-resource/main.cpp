//
// Created by masi on 5/11/23.
//


#include <iostream>
#include <unordered_set>
#include <memory_resource>
#include "persisted_mem.h"
#include <vector>
#include <iomanip>
#include <fcntl.h>
#include <cstring>

void print (char *buf, size_t size) {
    auto buf_start = static_cast <void *> (buf);
    std::cout << "buffer start " << buf_start << std::endl;

    for (size_t i = 0; i < size; i+=sizeof(int)) {
        if (i % 64 == 0) {
            std::cout << std::endl;
        }
        else {
            std::cout << std::setw (16);
        }
        std::cout << *reinterpret_cast <int *> (buf + i);
        //std::cout << std::string(buf + i, 4);
    }
}

int main () {

    bool integrate = false;
    static void* BASE_MEMORY_ADDR = (void*)0xf00000000lu;
    std::pmr::string str {"jorvcxvcxerwsaadadada"};

    int fd = open ("test", O_CREAT | O_RDWR, S_IRWXU);
    mmaper mmemm (fd, BASE_MEMORY_ADDR);
    close (fd);

    if (integrate) {
        auto ptr = mmemm.allocate(128);
        std::cout << ptr << std::endl;
        std::memcpy(ptr, str.c_str(), str.size());
        mmemm.deallocate(ptr, 128);
    }
    else {
        //auto ptr1 = mmemm.allocate(1024);
        //std::cout << ptr1 << std::endl;
        //std::memcpy(ptr1, str.c_str(), str.size());
        //mmemm.deallocate(ptr1, 1024);
        auto ptr = 0xf00000480;
        std::cout.write (reinterpret_cast <char *> (ptr), str.size ());
    }
}