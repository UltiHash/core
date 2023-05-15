//
// Created by masi on 5/11/23.
//

#ifndef CORE_PERSISTED_MEM_H
#define CORE_PERSISTED_MEM_H
#include <memory_resource>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <sys/mman.h>
#include <unistd.h>
#include <forward_list>

class logger: public std::pmr::memory_resource {
    std::pmr::memory_resource *m_mem_resource;
    std::pmr::string m_name;
public:
    logger (std::pmr::memory_resource *mem_resource, std::pmr::string name):
            m_name (std::move (name)),
            m_mem_resource (mem_resource) {
    }
private:
    void * do_allocate(size_t bytes, size_t alignment) override {
        void *ptr = m_mem_resource->allocate(bytes, alignment);
        std::cout << m_name << " alloc " << ptr << ":" << bytes << std::endl;
        return ptr;
    }

    void do_deallocate(void *p, size_t bytes, size_t alignment) override {
        m_mem_resource->deallocate(p, bytes, alignment);
        std::cout << m_name << " dealloc: " << p << ":" << bytes << std::endl;
    }

    [[nodiscard]] bool do_is_equal(const memory_resource &other) const noexcept override {
        return m_mem_resource->is_equal(other);
    }
};

class mmaper: public std::pmr::memory_resource {

    constexpr static size_t m_min_size = 16*1024;
    void *m_pin;
    size_t m_size {};
    size_t m_allocated_size {};
    bool m_replying {false};
    std::fstream m_log;

    std::pmr::forward_list <std::pair <char *, size_t>> m_buffers;
    std::pmr::forward_list <std::pmr::monotonic_buffer_resource> m_buffer_resources;
    std::pmr::forward_list <std::pmr::synchronized_pool_resource> m_pool_resources;

    std::pmr::memory_resource *m_resource {nullptr};
    int m_data_fd;

public:
    mmaper (int data_fd, void *pin, size_t size = 2 * m_min_size):
            m_pin (pin),
            m_data_fd (data_fd),
            m_log (create_logger ()) {
        if (!m_log.is_open ()) {
            throw std::exception ();
        }
        size += lseek (m_data_fd, SEEK_END, 0);
        load_new_buffer (size);
        reply_logger ();
    }

    ~mmaper () override {
        for (const auto buf: m_buffers) {
            msync (buf.first, buf.second, MS_SYNC);
        }
    }

private:
    void *do_allocate (size_t bytes, size_t alignment) override {
        if (!m_replying) {
            m_log << "alloc " << bytes << ' ' << alignment << '\n';
        }
        m_allocated_size += bytes;
        if (m_allocated_size >= m_size - m_min_size) {
            load_new_buffer (m_size);
        }
        return m_resource->allocate(bytes, alignment);
    }

    void do_deallocate (void *p, size_t bytes, size_t alignment) override {
        if (!m_replying) {
            m_log << "dealloc " << p << ' ' << bytes << ' ' << alignment << '\n';
        }
        m_allocated_size -= bytes;
        m_resource->deallocate(p, bytes, alignment);
    }

    bool do_is_equal (const memory_resource &other) const noexcept override {
        return m_resource->is_equal(other);
    }

    void load_new_buffer (std::size_t size) {
        if (size < m_min_size) {
            throw std::exception ();
        }
        ftruncate (m_data_fd, m_size + size);
        const auto flags = MAP_SHARED | MAP_FIXED | MAP_FIXED_NOREPLACE;
        const auto addr = reinterpret_cast <void *> (reinterpret_cast <size_t> (m_pin) + m_size);
        const auto mmapped = mmap(addr, size, PROT_READ | PROT_WRITE, flags, m_data_fd, m_size);
        if (mmapped != addr) {
            throw std::exception ();
        }
        m_buffers.emplace_front (static_cast <char *> (mmapped), size);
        m_buffer_resources.emplace_front (m_buffers.front ().first, size, std::pmr::null_memory_resource ());
        m_pool_resources.emplace_front (&m_buffer_resources.front ());
        m_resource = &m_pool_resources.front ();

        m_size += size;
    }

    std::fstream create_logger () {
        std::filesystem::path log_name = "_log_" + std::to_string (reinterpret_cast <size_t> (m_pin));
        auto flags = std::ios::in | std::ios::out;
        if (!std::filesystem::exists (log_name)) {
            flags |= std::ios::trunc;
        }
        return {log_name, flags};
    }

    void reply_logger () {
        m_replying = true;
        std::string token;
        while (m_log >> token) {
            if (token == "alloc") {
                size_t bytes, alignment;
                m_log >> bytes >> alignment;
                do_allocate (bytes, alignment);
            }
            else if (token == "dealloc") {
                size_t addr, bytes, alignment;
                m_log >> addr >> bytes >> alignment;
                do_deallocate (reinterpret_cast <void *> (addr), bytes, alignment);
            }
            else {
                throw std::exception ();
            }
        }
        m_log.clear ();
        m_replying = false;
    }
};


#endif //CORE_PERSISTED_MEM_H
