//
// Created by max on 02.11.22.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibFdb Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <thread>
#include "fdb/fdb.h"

#define CHAR_TO_SPAN(variable, stringLiteral) \
    char* variable = (char*) stringLiteral; \
    const std::span<char> variable##Span = {variable, std::strlen(variable)};

CHAR_TO_SPAN(key1, "key_1");
CHAR_TO_SPAN(key2, "key_2");
CHAR_TO_SPAN(key3, "key_3");
CHAR_TO_SPAN(key4, "key_4");
CHAR_TO_SPAN(val1, "value_1");
CHAR_TO_SPAN(val2, "value_2");
CHAR_TO_SPAN(val3, "value_3");
CHAR_TO_SPAN(val4, "value_4");
CHAR_TO_SPAN(notFound, "key_42")

#if defined(__APPLE__)
static auto fdb = uh::fdb::fdb("/usr/local/etc/foundationdb/fdb.cluster");
#else
static auto fdb = uh::fdb::fdb("/etc/foundationdb/fdb.cluster");
#endif

bool areEqual(const std::vector<char>& vec, const char* cString) {
    std::size_t cStringLength = std::strlen(cString);

    if (vec.size() != cStringLength) {
        return false;
    }

    return std::equal(vec.begin(), vec.end(), cString);
}


// ------------- Tests Follow --------------
BOOST_AUTO_TEST_CASE( setup )
{
    auto trans = fdb.make_transaction();

    trans->del(key1Span);
    trans->del(key2Span);
    trans->del(key3Span);
    trans->del(key4Span);

    auto kv1 = trans->get(key1Span);
    auto kv2 = trans->get(key2Span);
    auto kv3 = trans->get(key3Span);
    auto kv4 = trans->get(key4Span);

    BOOST_CHECK(!kv1.has_value());
    BOOST_CHECK(!kv2.has_value());
    BOOST_CHECK(!kv3.has_value());
    BOOST_CHECK(!kv4.has_value());

    trans->commit();
}

BOOST_AUTO_TEST_CASE( put_get_test )
{
    auto trans = fdb.make_transaction();
    trans->put(key1Span, val1Span);
    trans->put(key2Span, val2Span);
    trans->put(key3Span, val3Span);
    trans->put(key4Span, val4Span);

    auto kv1 = trans->get(key1Span);
    auto kv2 = trans->get(key2Span);
    auto kv3 = trans->get(key3Span);
    auto kv4 = trans->get(key4Span);

    BOOST_CHECK(kv1.has_value());
    BOOST_CHECK(kv2.has_value());
    BOOST_CHECK(kv3.has_value());
    BOOST_CHECK(kv4.has_value());

    BOOST_CHECK(areEqual(kv1->key, "key_1"));
    BOOST_CHECK(areEqual(kv1->value, "value_1"));

    BOOST_CHECK(areEqual(kv2->key, "key_2"));
    BOOST_CHECK(areEqual(kv2->value, "value_2"));

    BOOST_CHECK(areEqual(kv3->key, "key_3"));
    BOOST_CHECK(areEqual(kv3->value, "value_3"));

    BOOST_CHECK(areEqual(kv4->key, "key_4"));
    BOOST_CHECK(areEqual(kv4->value, "value_4"));

    auto kv_not_found = trans->get(notFoundSpan);
    BOOST_CHECK(!kv_not_found.has_value());
}

BOOST_AUTO_TEST_CASE( nothing_committed )
{
    auto trans = fdb.make_transaction();
    auto kv_not_found = trans->get(key1Span);
    BOOST_CHECK(!kv_not_found.has_value());
}

BOOST_AUTO_TEST_CASE( transaction_commit_test )
{
    auto trans = fdb.make_transaction();

    trans->put(key1Span, val1Span);
    trans->put(key2Span, val2Span);
    trans->put(key3Span, val3Span);
    trans->put(key4Span, val4Span);

    {
        auto trans2 = fdb.make_transaction();
        auto kv1 = trans2->get(key1Span);
        auto kv2 = trans2->get(key2Span);
        auto kv3 = trans2->get(key3Span);
        auto kv4 = trans2->get(key4Span);
        BOOST_CHECK(!kv1.has_value());
        BOOST_CHECK(!kv2.has_value());
        BOOST_CHECK(!kv3.has_value());
        BOOST_CHECK(!kv4.has_value());
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    trans->commit();

    {
        auto trans2 = fdb.make_transaction();
        auto kv1 = trans2->get(key1Span);
        auto kv2 = trans2->get(key2Span);
        auto kv3 = trans2->get(key3Span);
        auto kv4 = trans2->get(key4Span);
        BOOST_CHECK(kv1.has_value());
        BOOST_CHECK(kv2.has_value());
        BOOST_CHECK(kv3.has_value());
        BOOST_CHECK(kv4.has_value());
    }
}

BOOST_AUTO_TEST_CASE( teardown )
{
    auto trans = fdb.make_transaction();

    trans->del(key1Span);
    trans->del(key2Span);
    trans->del(key3Span);
    trans->del(key4Span);

    auto kv1 = trans->get(key1Span);
    auto kv2 = trans->get(key2Span);
    auto kv3 = trans->get(key3Span);
    auto kv4 = trans->get(key4Span);

    BOOST_CHECK(!kv1.has_value());
    BOOST_CHECK(!kv2.has_value());
    BOOST_CHECK(!kv3.has_value());
    BOOST_CHECK(!kv4.has_value());

    trans->commit();
}
