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
#include "storage/backends/smart_backend/storage_types/fixed_managed_storage.h"
#include <storage/backend.h>
#include <storage/backends/smart_backend/smart_config.h>
#include "storage/backends/smart_backend/fragment_sets/persisted_redblack_tree_set.h"
#include <storage/backends/smart_backend/smart_core.h>
#include <storage/backends/smart_backend/persisted_robinhood_hashmap.h>
#include "storage/backends/smart_storage.h"
#include "storage/backends/smart_backend/storage_types/growing_managed_storage.h"

using namespace uh::dbn::storage::smart;

class files_info_fixture {
public:

    smart_config m_smart_config;

    files_info_fixture():
            m_smart_config {define_test_map_conf (),
                            define_test_set_conf (),
                            define_test_data_store_conf (),
                            define_test_dedupe_conf()} {}


    void cleanup () {
        for (const auto &fi: m_smart_config.data_store_conf.data_store_files) {
            if (std::filesystem::exists (fi)) {
                std::filesystem::remove(fi);
            }
        }
        if (exists(m_smart_config.data_store_conf.log_file)) {
            std::filesystem::remove(m_smart_config.data_store_conf.log_file);
        }

        if (exists(m_smart_config.set_conf.fragment_set_path)) {
            std::filesystem::remove(m_smart_config.set_conf.fragment_set_path);
        }

        if (exists(m_smart_config.map_conf.hashtable_key_path)) {
            std::filesystem::remove(m_smart_config.map_conf.hashtable_key_path);
        }
        if (exists (m_smart_config.map_conf.value_store_log_file)) {
            std::filesystem::remove(m_smart_config.map_conf.value_store_log_file);
        }

        if (exists (m_smart_config.map_conf.hashtable_value_directory)) {
            std::filesystem::remove_all(std::filesystem::absolute(m_smart_config.map_conf.hashtable_value_directory));
        }
        std::filesystem::create_directory(m_smart_config.map_conf.hashtable_value_directory);

    }

    const map_config& get_map_conf () {
        return m_smart_config.map_conf;
    }

    const set_config& get_set_conf () {
        return m_smart_config.set_conf;
    }

    const data_store_config& get_data_store_config () {
        return m_smart_config.data_store_conf;
    }

    const dedupe_config& get_dedupe_config () {
        return m_smart_config.dedupe_conf;
    }

    const smart_config& get_smart_config () {
        return m_smart_config;
    }

private:

    static map_config define_test_map_conf () {
        map_config conf;
        conf.map_maximum_extension_factor = 32;
        conf.map_load_factor = 0.9;
        conf.map_values_maximum_file_size = 16 * 1024;
        conf.map_values_minimum_file_size = 4 * 1024;
        conf.map_key_file_init_size = 4 * 1024;
        conf.key_size = 128;
        conf.hashtable_value_directory = "growing_mmap_storage_directory";
        conf.hashtable_key_path = "hashmap_key_data";
        conf.value_store_log_file = conf.hashtable_value_directory / "log";
        return conf;
    }
    static set_config define_test_set_conf () {
        set_config conf;
        conf.set_minimum_free_space = 512;
        conf.set_init_file_size = 8*1024;
        conf.fragment_set_path = "set/set_data";
        conf.max_empty_hole_size = 512;
        return conf;
    }
    static data_store_config define_test_data_store_conf () {
        data_store_config conf;
        conf.data_store_files = generate_files();
        conf.data_store_file_size = 32 * 1024;
        conf.log_file = "ds/log";
        return conf;
    }
    dedupe_config define_test_dedupe_conf () {
        dedupe_config conf;
        conf.min_fragment_size = 4;
        return conf;
    }
    static std::list <std::filesystem::path> generate_files () {
        std::list <std::filesystem::path> files_info;

        for (int i = 0; i < 10; ++i) {
            std::filesystem::path p {std::string ("ds/file_") + std::to_string(i)};

            files_info.emplace_front (p);
        }

        return files_info;
    }
};
// ------------- Tests Follow --------------

BOOST_FIXTURE_TEST_CASE(test_mmap_storage_basic_allocation, files_info_fixture)
{

    cleanup ();

    fixed_managed_storage ms (get_data_store_config());

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
        fixed_managed_storage ms(get_data_store_config());

        ptr = ms.allocate(size);
        std::memcpy(ptr.m_addr, data, size);
    }

    {
        fixed_managed_storage ms(get_data_store_config());
        void* raw_ptr = ms.get_raw_ptr(ptr.m_offset);
        BOOST_TEST (std::memcmp(data, static_cast <char *> (raw_ptr), size) == 0);
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
        fixed_managed_storage ms(get_data_store_config());

        ptr1 = ms.allocate(size);
        std::memcpy(ptr1.m_addr, data, size);
    }

    {
        fixed_managed_storage ms(get_data_store_config());
        ptr2 = ms.allocate(size);
        BOOST_TEST (ptr1.m_offset + size <= ptr2.m_offset);
    }
    {
        fixed_managed_storage ms(get_data_store_config());
        ms.deallocate(ptr1, size);
    }
    {
        fixed_managed_storage ms(get_data_store_config());
        ptr2 = ms.allocate(size);
        BOOST_TEST (ptr1.m_offset == ptr2.m_offset);

    }
}


uint64_t set_insert (fixed_managed_storage& ms, sets::persisted_redblack_tree_set& set, std::string_view data, uint64_t hint = 2 * sizeof (uint64_t)) {
    auto alloc = ms.allocate(data.size());
    std::memcpy(alloc.m_addr, data.data(), data.size());
    const auto h = set.insert_index(data, alloc.m_offset);
    return h.hint;
}

BOOST_FIXTURE_TEST_CASE(basic_test_mmap_set, files_info_fixture)
{
    cleanup();
    fixed_managed_storage ms(get_data_store_config());

    sets::persisted_redblack_tree_set set {get_set_conf(), ms};

    auto h = set_insert (ms, set, "hello from data 1");
    h = set_insert (ms, set, "data 2 hello from data 2", h);
    set_insert(ms, set, "third data hello from data 3", h);
    set_insert(ms, set, "some other data");
    set_insert(ms, set, "yet again, some other data");
    h = set_insert(ms, set, "yet again, some other data", h);
    set_insert(ms, set, "yet again, some other data");
    set_insert(ms, set, "and even more data");
    set_insert(ms, set, "third data hello from data 3", h);
    set_insert(ms, set, "some other data 2 ");
    set_insert(ms, set, "yet again, some other data 2");
    h = set_insert(ms, set, "yet again, some other data 2", h);
    set_insert(ms, set, "yet again, some other data 2");
    set_insert(ms, set, "and even more data 4");
    set_insert(ms, set, "third data hello from data 3 4", h);
    set_insert(ms, set, "third data hello from data 3 22", h);
    set_insert(ms, set, "some other data 2 342");
    set_insert(ms, set, "yet again, some other data 2 432");
    h = set_insert(ms, set, "yet again, some other data 2 324", h);
    set_insert(ms, set, "yet again, some other data 2234");
    set_insert(ms, set, "and 234even more data 4");
    set_insert(ms, set, "third  234 data hello from data 3 4", h);
    set_insert(ms, set, "yet again, some other data 2 432");
    h = set_insert(ms, set, "dsadyet again, some other data 2 324", h);
    set_insert(ms, set, "34yet again, some other data 2234");
    set_insert(ms, set, "323and 234even more data 4");
    set_insert(ms, set, "432third  234 data hello from data 3 4", h);

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

    auto res6 = set.find("third  234 data hello from data 3 4");
    BOOST_TEST(res6.match->second == "third  234 data hello from data 3 4");

    auto res7 = set.find("432third  234 data hello from data 3 4");
    BOOST_TEST(res7.match->second == "432third  234 data hello from data 3 4");

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

    std::string k1 = "e7b6028fc35d42aa47cad92d1011de1bb220597ace2dd6514430c33c62306e43b4906a1b885a2eae510ff1be7c6ac03d2cb80ac8b7f48abbe1bbfed34a602eb2";
    std::string k2 = "cf910f11ad2cd0e185ee1fdaf8a8237c832e42496eebfc5b7e745ab74935b639c7af4bb7834fb3b61c111a0067151ecea0ad1eb11d68a073fc5e3d7c141c88ec";
    std::string k3 = "3814a2723a484551f5e0be88a973faa8bb17ff8dd16128a6b56fcf273e3d3925963752f3ad8449027986b583727e484d3148307ef2a659b0b389009850f06441";
    std::string k4 = "70ad7f971f1b6215362bb4ae1930922fd31a6fafd548533a86ae32dd8107da0de87260496951067831546f3325bc5bd63fdc9fe18c9a93028e73acc34aaf3f1f";
    std::string k5 = "b0f53d8ed7637474fd2d1fffe3dbd48864a87e268f37598cf06debba485515b2f8fac305362e578baa230d7337da9020668bde144c3463559de5c2ea4d6f6dd3";

    cleanup();
    std::hash <std::string> h;
    {
        smart_core pd{get_smart_config()};


        std::string str1 = "hello from data 1";
        auto res1 = pd.integrate(k1, str1);
        BOOST_TEST (res1 == str1.size());

        std::string str2 = "hello from data 2234";
        auto res2 = pd.integrate(k2, str2);
        BOOST_TEST (res2 == 4);

        std::string str3 = "data 2 hello from data 2";
        auto res3 = pd.integrate(k3, str3);
        BOOST_TEST (res3 == str3.size());

        std::string str4 = "hello from data yet again";
        auto res4 = pd.integrate(k4, str4);
        BOOST_TEST (res4 == 9);

        std::string str5 = "yet again, some other data";
        auto res5 = pd.integrate(k5, str5);
        BOOST_TEST (res5 == 17);
    }

    {
        smart_core pd{get_smart_config()};

        std::string str5 = "yet again, some other data";
        auto res5 = pd.integrate(k5, str5);
        BOOST_TEST (res5 == 0);
    }
}


void insert_in_hm (persisted_robinhood_hashmap& hm, std::string& k, std::string& v) {
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

        persisted_robinhood_hashmap hm (get_map_conf());

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

        persisted_robinhood_hashmap hm (get_map_conf());
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


BOOST_FIXTURE_TEST_CASE(basic_growing_mmap_storage_test, files_info_fixture)
{

    cleanup();
    offset_ptr ptr1;
    offset_ptr ptr2;
    char data [] = "Calculate the length of your string of text or numbers to check the number of characters it contains! Using our online character counting tool is quick and easy! This tool is great for computer programmers, web developers, writers, and other programmers.  ";
    size_t size = 256;

    {
        growing_managed_storage ms(get_map_conf().hashtable_value_directory, get_map_conf().value_store_log_file, 4 * 1024, 8 * 1024);

        ptr1 = ms.allocate(size);
        std::memcpy(ptr1.m_addr, &data, size);

        ms.allocate(size);
        ms.allocate(size);
        ms.allocate(size);
        ms.allocate(size);
        ms.allocate(size);
        ms.allocate(size);
        ptr2 = ms.allocate(size);
        std::memcpy(ptr2.m_addr, &data, size);
        ms.allocate(size);
        ms.allocate(size);
        ms.allocate(size);

    }

    const auto addr = mmap (align_ptr(ptr1.m_addr), 1024, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
    BOOST_TEST(addr == align_ptr(ptr1.m_addr));
    {
        growing_managed_storage ms(get_map_conf().hashtable_value_directory, get_map_conf().value_store_log_file,  4 * 1024, 8 * 1024);
        ms.allocate(size);
        ms.allocate(size);
        ms.allocate(size);
        ms.allocate(size);
        ms.allocate(size);
        ms.allocate(size);
        ms.allocate(size);
        ms.allocate(size);
        ms.allocate(size);
        ms.allocate(size);
        ms.allocate(size);

        BOOST_TEST (ptr1.m_offset + size < ptr2.m_offset);

        void* raw_ptr = ms.get_raw_ptr(ptr1.m_offset);
        BOOST_TEST (std::memcmp(&data, static_cast <char *> (raw_ptr), size) == 0);

        void* raw_ptr2 = ms.get_raw_ptr(ptr2.m_offset);
        BOOST_TEST (std::memcmp(&data, static_cast <char *> (raw_ptr2), size) == 0);
    }
    munmap(align_ptr(ptr1.m_addr), 1024);

}

std::string serialize_spans (std::forward_list <std::span <char>> spans) {
    std::string res;
    for (const auto sp: spans) {
        res += std::string (sp.data(), sp.size());
    }
    return res;
}

BOOST_FIXTURE_TEST_CASE(smart_core_basic_test, files_info_fixture) {
    cleanup();

    std::string k1 = "bba3f3c564f31d8664c5775fbe16580061693f1db21069b58fa448ecbbf397f2264ab1fb8f17f33edbdab52def96fd2b1124d04ba1764b554e0e7b49a24d5574";
    std::string v1 = "hello from data 1645";

    std::string k2 = "ce8cd8c8035651ac62d1f74f6f16f98263998a9cba6183e3afd356bc93e81962432b50e28b2af172576d6d61a18e1b35241db63b3b55d3e023bea671402a3ac8";
    std::string v2 = "hello from data 2234";

    std::string k3 = "521e6c7bf2a02d97f8f055f12afaadf93dbb4b1dd1a4fcd042302143d43e5eb3b7b325bff8af75eb6037cf0adec1eeba7eec19ca8e1e10424454bd5e3356f73a";
    std::string v3 = "third data hello from data 3";

    std::string k4 = "416c19b9f9c498f58685a33f76f73bebb0064525757f5d46682f35c903e8629e0fb16107ddd8f34f9a5a5f0057b13dc77c2daebbc17cf207320790222ad28212";
    std::string v4 = "hello from data yet again";

    std::string k5 = "067cd2591e8a3087596f9c3339325929e1341979d0fe4cd4fdbd2d2376432c745db1beb647f6eace883659ae9418cacb36a7e58bf719f41b5beb962c84dd8873";
    std::string v5 = "yet again, some other data";

    //std::string k6 = "067cd2591e8a3087596f9c3339325929e1341979d0fe4cd4fdbd2d2376432c745db1beb647f6eace883659ae9418cacb36a7e58bf719f41b5beb962c84dd8873";
    //std::string v6 = "hello from data 2234";

    std::string k6 = "e7c22b994c59d9cf2b48e549b1e24666636045930d3da7c1acb299d1c3b7f931f94aae41edda2c2b207a36e10f8bcb8d45223e54878f5b316e7ce3b6bc019629";
    std::string v6 = "hello from data 2234";
    {

        smart_core sm(get_smart_config());

        const auto i1 = sm.integrate(k1, v1);
        BOOST_TEST (i1 == v1.size());

        const auto i2 = sm.integrate(k2, v2);
        BOOST_TEST (i2 == 4);

        const auto i3 = sm.integrate(k3, v3);
        BOOST_TEST (i3 == v3.size());

        const auto i4 = sm.integrate(k4, v4);
        BOOST_TEST (i4 == 9);

        const auto i5 = sm.integrate(k5, v5);
        BOOST_TEST (i5 == 17);

        const auto i6 = sm.integrate(k6, v6);
        BOOST_TEST (i6 == 0);

        const auto r1 = sm.retrieve(k1);
        const auto sr1 = serialize_spans(r1.second);
        BOOST_TEST (sr1.size() == v1.size());
        BOOST_TEST (std::memcmp(sr1.data(), v1.data(), v1.size()) == 0);

        const auto r2 = sm.retrieve(k2);
        const auto sr2 = serialize_spans(r2.second);
        BOOST_TEST (sr2.size() == v2.size());
        BOOST_TEST (std::memcmp(sr2.data(), v2.data(), v2.size()) == 0);

        const auto r4 = sm.retrieve(k4);
        const auto sr4 = serialize_spans(r4.second);
        BOOST_TEST (sr4.size() == v4.size());
        BOOST_TEST (std::memcmp(sr4.data(), v4.data(), v4.size()) == 0);

        const auto r5 = sm.retrieve(k5);
        const auto sr5 = serialize_spans(r5.second);
        BOOST_TEST (sr5.size() == v5.size());
        BOOST_TEST (std::memcmp(sr5.data(), v5.data(), v5.size()) == 0);

        const auto r6 = sm.retrieve(k6);
        const auto sr6 = serialize_spans(r6.second);
        BOOST_TEST (sr6.size() == v6.size());
        BOOST_TEST (std::memcmp (sr6.data(), v6.data(), v6.size()) == 0);

    }

    char* ptr = new char [10*1024*1024];
    {
        smart_core sm(get_smart_config());

        const auto r1 = sm.retrieve(k1);
        const auto sr1 = serialize_spans(r1.second);
        BOOST_TEST (sr1.size() == v1.size());
        BOOST_TEST (std::memcmp(sr1.data(), v1.data(), v1.size()) == 0);

        const auto r2 = sm.retrieve(k2);
        const auto sr2 = serialize_spans(r2.second);
        BOOST_TEST (sr2.size() == v2.size());
        BOOST_TEST (std::memcmp(sr2.data(), v2.data(), v2.size()) == 0);

        const auto r4 = sm.retrieve(k4);
        const auto sr4 = serialize_spans(r4.second);
        BOOST_TEST (sr4.size() == v4.size());
        BOOST_TEST (std::memcmp(sr4.data(), v4.data(), v4.size()) == 0);

        const auto r5 = sm.retrieve(k5);
        const auto sr5 = serialize_spans(r5.second);
        BOOST_TEST (sr5.size() == v5.size());
        BOOST_TEST (std::memcmp(sr5.data(), v5.data(), v5.size()) == 0);

        const auto r6 = sm.retrieve(k6);
        const auto sr6 = serialize_spans(r6.second);
        BOOST_TEST (sr6.size() == v6.size());
        BOOST_TEST (std::memcmp (sr6.data(), v6.data(), v6.size()) == 0);

    }

    delete[] ptr;
}
