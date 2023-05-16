//
// Created by masi on 5/16/23.
//
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhServerDb SmartBackend Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <storage/backends/smart_backend/mmap_storage.h>
#include "storage/backend.h"

using namespace uh::dbn::storage::smart;

class files_info_fixture {
public:
    static constexpr int FILES_COUNT = 10;
    static constexpr size_t FILE_SIZE = 24 * 1024;

    files_info_fixture(): m_files_info (generate_files_info ()) {

    }

    std::forward_list <file_mmap_info> &files_info() {
        return m_files_info;
    }

    void cleanup () {
        for (const auto &fi: m_files_info) {
            if (std::filesystem::exists (fi.path)) {
                std::filesystem::remove(fi.path);
            }
        }
        const std::filesystem::path log_name = "_mmap_allocation_log_";
        if (exists(log_name)) {
            std::filesystem::remove(log_name);
        }

    }

private:

    static std::forward_list <file_mmap_info> generate_files_info () {
        std::forward_list <file_mmap_info> files_info;

        for (int i = 0; i < FILES_COUNT; ++i) {
            std::filesystem::path p {std::string ("file_") + std::to_string(i)};

            files_info.emplace_front (p, nullptr, FILE_SIZE);
        }

        return files_info;
    }


    std::forward_list <file_mmap_info> m_files_info;
};

// ------------- Tests Follow --------------

BOOST_FIXTURE_TEST_CASE(test_mmap_storage_basic_allocation, files_info_fixture)
{

    cleanup ();

    mmap_storage ms (files_info ());

    size_t size1 = 1024, size2 = 512;
    void* ptr1 = ms.allocate(size1);
    void* ptr2 = ms.allocate(size2);
    BOOST_CHECK(static_cast <char*> (ptr1) + size1 <= ptr2);

    ms.deallocate(ptr1, size1);
    void* ptr3 = ms.allocate(size1);
    BOOST_CHECK(ptr1 == ptr3);

    void *ptr4 = ms.allocate(size2);
    BOOST_CHECK(static_cast <char*> (ptr2) + size2 <= ptr4);
}

BOOST_FIXTURE_TEST_CASE(test_mmap_storage_data_test, files_info_fixture)
{

    cleanup ();
    void* ptr;
    char data[] = "0123456789";
    size_t size = 10;

    {
        mmap_storage ms(files_info());

        ptr = ms.allocate(size);
        std::memcpy(ptr, data, size);
    }

    {
        mmap_storage ms(files_info());
        BOOST_CHECK (std::strncmp(data, static_cast <char *> (ptr), size) == 0);
    }

}