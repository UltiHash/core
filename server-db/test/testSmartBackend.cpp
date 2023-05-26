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
#include <storage/backends/smart_backend/growing_mmap_storage.h>


using namespace uh::dbn::storage::smart;

class files_info_fixture {
public:
    static constexpr int FILES_COUNT = 10;
    static constexpr size_t FILE_SIZE = 24 * 1024;

    const std::filesystem::path m_set_filename = "set_data";
    const std::filesystem::path m_hashmap_key_filename = "hashmap_key_data";
    const std::filesystem::path m_growing_directory = "growing_mmap_storage_directory";


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

        if (exists (m_growing_directory)) {
            std::filesystem::remove_all(m_growing_directory);
        }
        std::filesystem::create_directories(m_growing_directory);

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

    std::string k1 = "e7b6028fc35d42aa47cad92d1011de1bb220597ace2dd6514430c33c62306e43b4906a1b885a2eae510ff1be7c6ac03d2cb80ac8b7f48abbe1bbfed34a602eb2";
    std::string k2 = "cf910f11ad2cd0e185ee1fdaf8a8237c832e42496eebfc5b7e745ab74935b639c7af4bb7834fb3b61c111a0067151ecea0ad1eb11d68a073fc5e3d7c141c88ec";
    std::string k3 = "3814a2723a484551f5e0be88a973faa8bb17ff8dd16128a6b56fcf273e3d3925963752f3ad8449027986b583727e484d3148307ef2a659b0b389009850f06441";
    std::string k4 = "70ad7f971f1b6215362bb4ae1930922fd31a6fafd548533a86ae32dd8107da0de87260496951067831546f3325bc5bd63fdc9fe18c9a93028e73acc34aaf3f1f";
    std::string k5 = "b0f53d8ed7637474fd2d1fffe3dbd48864a87e268f37598cf06debba485515b2f8fac305362e578baa230d7337da9020668bde144c3463559de5c2ea4d6f6dd3";

    cleanup();
    std::hash <std::string> h;
    auto hs = [] (unsigned long v) {
        return std::span <char> {reinterpret_cast <char*> (&v), sizeof (v)};
    };

    {
        smart_storage pd{files_info(), m_set_filename, m_hashmap_key_filename, m_growing_directory};


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
        smart_storage pd{files_info(), m_set_filename, m_hashmap_key_filename, m_growing_directory};

        std::string str5 = "yet again, some other data";
        auto res5 = pd.integrate(k5, str5);
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

        robin_hood_hashmap hm (64, m_hashmap_key_filename, m_growing_directory);

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

        robin_hood_hashmap hm (64, m_hashmap_key_filename, m_growing_directory);
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
    char data [1024];
    size_t size = 1024;

    {
        growing_mmap_storage ms(m_growing_directory, 4*1024, 32*1024);

        ptr1 = ms.allocate(size);
        std::memcpy(ptr1.m_addr, data, size);

        ms.allocate(size);
        ms.allocate(size);
        ms.allocate(size);
        ms.allocate(size);
        ms.allocate(size);
        ms.allocate(size);
        ptr2 = ms.allocate(size);
        std::memcpy(ptr2.m_addr, data, size);
        ms.allocate(size);
        ms.allocate(size);
        ms.allocate(size);

    }

    {
        growing_mmap_storage ms(m_growing_directory, 4*1024, 32*1024);
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
        BOOST_TEST (std::memcmp(data, static_cast <char *> (raw_ptr), size) == 0);

        void* raw_ptr2 = ms.get_raw_ptr(ptr2.m_offset);
        BOOST_TEST (std::memcmp(data, static_cast <char *> (raw_ptr2), size) == 0);
    }

}

BOOST_FIXTURE_TEST_CASE(smart_storage_basic_test, files_info_fixture) {
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
        smart_storage sm(files_info(), m_set_filename, m_hashmap_key_filename, m_growing_directory);

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
        const auto sr1 = sm.serialize_fragments(r1);
        BOOST_TEST (sr1.size() == v1.size());
        BOOST_TEST (std::memcmp(sr1.data(), v1.data(), v1.size()) == 0);

        const auto r2 = sm.retrieve(k2);
        const auto sr2 = sm.serialize_fragments(r2);
        BOOST_TEST (sr2.size() == v2.size());
        BOOST_TEST (std::memcmp(sr2.data(), v2.data(), v2.size()) == 0);

        const auto r4 = sm.retrieve(k4);
        const auto sr4 = sm.serialize_fragments(r4);
        BOOST_TEST (sr4.size() == v4.size());
        BOOST_TEST (std::memcmp(sr4.data(), v4.data(), v4.size()) == 0);

        const auto r5 = sm.retrieve(k5);
        const auto sr5 = sm.serialize_fragments(r5);
        BOOST_TEST (sr5.size() == v5.size());
        BOOST_TEST (std::memcmp(sr5.data(), v5.data(), v5.size()) == 0);

        const auto r6 = sm.retrieve(k6);
        const auto sr6 = sm.serialize_fragments(r6);
        BOOST_TEST (sr6.size() == v6.size());
        BOOST_TEST (std::memcmp (sr6.data(), v6.data(), v6.size()) == 0);
    }

    {
        smart_storage sm(files_info(), m_set_filename, m_hashmap_key_filename, m_growing_directory);
        
        const auto r1 = sm.retrieve(k1);
        const auto sr1 = sm.serialize_fragments(r1);
        BOOST_TEST (sr1.size() == v1.size());
        BOOST_TEST (std::memcmp(sr1.data(), v1.data(), v1.size()) == 0);

        const auto r2 = sm.retrieve(k2);
        const auto sr2 = sm.serialize_fragments(r2);
        BOOST_TEST (sr2.size() == v2.size());
        BOOST_TEST (std::memcmp(sr2.data(), v2.data(), v2.size()) == 0);

        const auto r4 = sm.retrieve(k4);
        const auto sr4 = sm.serialize_fragments(r4);
        BOOST_TEST (sr4.size() == v4.size());
        BOOST_TEST (std::memcmp(sr4.data(), v4.data(), v4.size()) == 0);

        const auto r5 = sm.retrieve(k5);
        const auto sr5 = sm.serialize_fragments(r5);
        BOOST_TEST (sr5.size() == v5.size());
        BOOST_TEST (std::memcmp(sr5.data(), v5.data(), v5.size()) == 0);

        const auto r6 = sm.retrieve(k6);
        const auto sr6 = sm.serialize_fragments(r6);
        BOOST_TEST (sr6.size() == v6.size());
        BOOST_TEST (std::memcmp (sr6.data(), v6.data(), v6.size()) == 0);
    }
}
