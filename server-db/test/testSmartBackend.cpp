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
#include "storage/backends/smart_backend/mmap_set.h"
#include "storage/backends/smart_backend/prefix_deduplicator.h"

using namespace uh::dbn::storage::smart;

class files_info_fixture {
public:
    static constexpr int FILES_COUNT = 10;
    static constexpr size_t FILE_SIZE = 24 * 1024;

    const std::filesystem::path m_set_filename = "set_data";

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
        const std::filesystem::path log_name = "log_d4774199341a3721872b";
        if (exists(log_name)) {
            std::filesystem::remove(log_name);
        }

        if (exists(m_set_filename)) {
            std::filesystem::remove(m_set_filename);
        }
    }

private:

    static std::forward_list <file_mmap_info> generate_files_info () {
        std::forward_list <file_mmap_info> files_info;

        for (int i = 0; i < FILES_COUNT; ++i) {
            std::filesystem::path p {std::string ("file_") + std::to_string(i)};

            files_info.push_front ({p, nullptr, FILE_SIZE});
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
    offset_ptr ptr1 = ms.allocate(size1);
    offset_ptr ptr2 = ms.allocate(size2);
    BOOST_TEST(ptr1.m_addr + size1 <= ptr2.m_addr);

    ms.deallocate(ptr1, size1);
    offset_ptr ptr3 = ms.allocate(size1);
    BOOST_TEST(ptr1.m_addr == ptr3.m_addr);

    offset_ptr ptr4 = ms.allocate(size2);
    BOOST_TEST(ptr1.m_addr + size2 <= ptr4.m_addr);
}

BOOST_FIXTURE_TEST_CASE(test_mmap_storage_data_test, files_info_fixture)
{

    cleanup ();
    offset_ptr ptr;
    char data[] = "0123456789";
    size_t size = 10;

    {
        mmap_storage ms(files_info());

        ptr = ms.allocate(size);
        std::memcpy(ptr.m_addr, data, size);
    }

    {
        mmap_storage ms(files_info());
        void* raw_ptr = ms.get_raw_ptr(ptr.m_offset);
        BOOST_TEST (std::strncmp(data, static_cast <char *> (raw_ptr), size) == 0);
    }

}

BOOST_FIXTURE_TEST_CASE(test_mmap_storage_persistet_alloc_test, files_info_fixture)
{

    cleanup ();
    offset_ptr ptr1;
    offset_ptr ptr2;
    char data[] = "0123456789";
    size_t size = 10;

    {
        mmap_storage ms(files_info());

        ptr1 = ms.allocate(size);
        std::memcpy(ptr1.m_addr, data, size);
    }

    {
        mmap_storage ms(files_info());
        ptr2 = ms.allocate(size);
        BOOST_TEST (ptr1.m_offset + size <= ptr2.m_offset);
    }
    {
        mmap_storage ms(files_info());
        ms.deallocate(ptr1, size);
    }
    {
        mmap_storage ms(files_info());
        ptr2 = ms.allocate(size);
        BOOST_TEST (ptr1.m_offset == ptr2.m_offset);

    }
}


uint64_t set_insert (mmap_set& set, std::string_view data, uint64_t hint = 2*sizeof (uint64_t)) {
    const auto data_offset = set.insert_data(data);
    const auto h = set.insert_index(data, data_offset);
    return h;
}

BOOST_FIXTURE_TEST_CASE(basic_test_mmap_set, files_info_fixture)
{
    cleanup();
    mmap_storage ms(files_info());

    mmap_set set {ms, m_set_filename};

    auto h = set_insert (set, "hello from data 1");
    h = set_insert (set, "data 2 hello from data 2", h);
    set_insert(set, "third data hello from data 3", h);
    set_insert(set, "some other data");
    set_insert(set, "yet again, some other data");
    h = set_insert(set, "yet again, some other data", h);
    set_insert(set, "yet again, some other data");
    set_insert(set, "and even more data");
    set_insert(set, "third data hello from data 3", h);



    auto res1 = set.find("and even more data");
    BOOST_TEST(res1.match->second == "and even more data");

    auto res2 = set.find("some other data");
    auto str = res2.match->second;
    BOOST_TEST(str == "some other data");

    auto res3 = set.find("hello from data 1");
    BOOST_TEST(res3.match->second == "hello from data 1");

    auto res4 = set.find("yet again, some other data");
    BOOST_TEST(res4.match->second == "yet again, some other data");

    auto res5 = set.find("third data hello from data 3");
    BOOST_TEST(res5.match->second == "third data hello from data 3");
/*
    auto res6 = set.find("hello from data ");
    std::cout << res2.match << std::endl;

    auto res7 = set.find("some other");
    std::cout << std::get <2> (res7) << std::endl;

    auto res8 = set.find("some other data 2");
    std::cout << std::get <2> (res8) << std::endl;

    auto res9 = set.find("some other something");
    std::cout << std::get <2> (res9) << std::endl
*/

}

BOOST_FIXTURE_TEST_CASE(basic_dedup_test, files_info_fixture)
{
    cleanup();
    mmap_storage ms(files_info());
    prefix_deduplicator pd {ms , m_set_filename};

    std::string str1 = "hello from data 1";
    auto res1 = pd.deduplicate(str1);
    BOOST_TEST (res1.second == str1.size());

    std::string str2 = "hello from data 2234";
    auto res2 = pd.deduplicate(str2);
    BOOST_TEST (res2.second == 4);

    std::string str3 = "data 2 hello from data 2";
    auto res3 = pd.deduplicate(str3);
    BOOST_TEST (res3.second == str3.size());

    std::string str4 = "hello from data yet again";
    auto res4 = pd.deduplicate(str4);
    BOOST_TEST (res4.second == 9);

    std::string str5 = "yet again, some other data";
    auto res5 = pd.deduplicate(str5);
    BOOST_TEST (res5.second == 17);

}