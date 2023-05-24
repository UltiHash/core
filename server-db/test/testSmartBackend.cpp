//
// Created by masi on 5/16/23.
//
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhServerDb SmartBackend Tests"
#endif

#include <span>
#include <boost/test/unit_test.hpp>
#include <storage/backends/smart_backend/mmap_storage.h>
#include <storage/backend.h>
#include <storage/backends/smart_backend/mmap_set.h>
#include <storage/backends/smart_backend/smart_storage.h>
#include <storage/backends/smart_backend/robin_hood_hashmap.h>


using namespace uh::dbn::storage::smart;

class files_info_fixture {
public:
    static constexpr int FILES_COUNT = 10;
    static constexpr size_t FILE_SIZE = 24 * 1024;

    const std::filesystem::path m_set_filename = "set_data";
    const std::filesystem::path m_hashmap_key_filename = "hashmap_key_data";

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

        if (exists(m_hashmap_key_filename)) {
            std::filesystem::remove(m_hashmap_key_filename);
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


uint64_t set_insert (mmap_storage& ms, mmap_set& set, std::string_view data, uint64_t hint = 2*sizeof (uint64_t)) {
    auto alloc = ms.allocate(data.size());
    std::memcpy(alloc.m_addr, data.data(), data.size());
    const auto h = set.insert_index(data, alloc.m_offset, hint);
    return h;
}

BOOST_FIXTURE_TEST_CASE(basic_test_mmap_set, files_info_fixture)
{
    cleanup();
    mmap_storage ms(files_info());

    mmap_set set {ms, m_set_filename};

    auto h = set_insert (ms, set, "hello from data 1");
    h = set_insert (ms, set, "data 2 hello from data 2", h);
    set_insert(ms, set, "third data hello from data 3", h);
    set_insert(ms, set, "some other data");
    set_insert(ms, set, "yet again, some other data");
    h = set_insert(ms, set, "yet again, some other data", h);
    set_insert(ms, set, "yet again, some other data");
    set_insert(ms, set, "and even more data");
    set_insert(ms, set, "third data hello from data 3", h);



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

//    auto res6 = set.find("hello from data ");
//    std::cout << res2.match << std::endl;
//
//    auto res7 = set.find("some other");
//    std::cout << std::get <2> (res7) << std::endl;
//
//    auto res8 = set.find("some other data 2");
//    std::cout << std::get <2> (res8) << std::endl;
//
//    auto res9 = set.find("some other something");
//    std::cout << std::get <2> (res9) << std::endl


}


BOOST_FIXTURE_TEST_CASE(basic_dedup_test, files_info_fixture)
{

    cleanup();
    std::hash <std::string> h;
    auto hs = [] (unsigned long v) {
        return std::span <char> {reinterpret_cast <char*> (&v), sizeof (v)};
    };

    {
        smart_storage pd{files_info(), m_set_filename, m_hashmap_key_filename};


        std::string str1 = "hello from data 1";
        auto res1 = pd.integrate(hs (h (str1)), str1);
        BOOST_TEST (res1 == str1.size());

        std::string str2 = "hello from data 2234";
        auto res2 = pd.integrate(hs (h (str2)), str2);
        BOOST_TEST (res2 == 4);

        std::string str3 = "data 2 hello from data 2";
        auto res3 = pd.integrate(hs (h (str3)), str3);
        BOOST_TEST (res3 == str3.size());

        std::string str4 = "hello from data yet again";
        auto res4 = pd.integrate(hs (h (str4)), str4);
        BOOST_TEST (res4 == 9);

        std::string str5 = "yet again, some other data";
        auto res5 = pd.integrate(hs (h (str5)), str5);
        BOOST_TEST (res5 == 17);
    }

    {
        smart_storage pd{files_info(), m_set_filename, m_hashmap_key_filename};

        std::string str5 = "yet again, some other data";
        auto res5 = pd.integrate(hs (h (str5)), str5);
        BOOST_TEST (res5 == 0);
    }
}


void insert_in_hm (robin_hood_hashmap& hm, std::string& k, std::string& v) {
    std::span <char> sk {k};
    std::span <char> sv {v};
    hm.insert(sk, sv);
}

BOOST_FIXTURE_TEST_CASE(basic_hashmap_test, files_info_fixture)
{
    cleanup();


    std::string k1 = "bba3f3c564f31d8664c5775fbe16580061693f1db21069b58fa448ecbbf397f2264ab1fb8f17f33edbdab52def96fd2b1124d04ba1764b554e0e7b49a24d5574";
    std::string v1 = "value1";

    std::string k2 = "ce8cd8c8035651ac62d1f74f6f16f98263998a9cba6183e3afd356bc93e81962432b50e28b2af172576d6d61a18e1b35241db63b3b55d3e023bea671402a3ac8";
    std::string v2 = "value2";

    std::string k3 = "521e6c7bf2a02d97f8f055f12afaadf93dbb4b1dd1a4fcd042302143d43e5eb3b7b325bff8af75eb6037cf0adec1eeba7eec19ca8e1e10424454bd5e3356f73a";
    std::string v3 = "value3";

    std::string k4 = "416c19b9f9c498f58685a33f76f73bebb0064525757f5d46682f35c903e8629e0fb16107ddd8f34f9a5a5f0057b13dc77c2daebbc17cf207320790222ad28212";
    std::string v4 = "value4";

    std::string k5 = "067cd2591e8a3087596f9c3339325929e1341979d0fe4cd4fdbd2d2376432c745db1beb647f6eace883659ae9418cacb36a7e58bf719f41b5beb962c84dd8873";
    std::string v5 = "value5";

    {
        mmap_storage ms(files_info());

        robin_hood_hashmap hm (m_hashmap_key_filename, ms);

        insert_in_hm(hm, k1, v1);

        insert_in_hm(hm, k2, v2);

        insert_in_hm(hm, k3, v3);

        insert_in_hm(hm, k4, v4);

        insert_in_hm(hm, k5, v5);

        auto res = hm.get(k1);
        BOOST_TEST(v1 == std::string (res.value().data(), res.value().size()));

        res = hm.get(k2);
        BOOST_TEST(v2 == std::string (res.value().data(), res.value().size()));

        res = hm.get(k5);
        BOOST_TEST(v5 == std::string (res.value().data(), res.value().size()));

        res = hm.get(k4);
        BOOST_TEST(v4 == std::string (res.value().data(), res.value().size()));

        res = hm.get(k3);
        BOOST_TEST(v3 == std::string (res.value().data(), res.value().size()));
    }
    {
        mmap_storage ms(files_info());

        robin_hood_hashmap hm (m_hashmap_key_filename, ms);
        auto res = hm.get(k1);
        BOOST_TEST(v1 == std::string (res.value().data(), res.value().size()));

        res = hm.get(k2);
        BOOST_TEST(v2 == std::string (res.value().data(), res.value().size()));

        res = hm.get(k5);
        BOOST_TEST(v5 == std::string (res.value().data(), res.value().size()));

        res = hm.get(k4);
        BOOST_TEST(v4 == std::string (res.value().data(), res.value().size()));

        res = hm.get(k3);
        BOOST_TEST(v3 == std::string (res.value().data(), res.value().size()));
    }
}