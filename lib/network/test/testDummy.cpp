//
// Created by max on 02.11.22.
//

#define BOOST_TEST_DYN_LINK
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibNetwork Dummy Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <network/net_client.h>
#include <network/net_common.h>
#include <network/net_connection.h>
#include <network/net_message.h>
#include <network/net_queue_ts.h>
#include <network/net_server.h>
#include <network/net_uhcustom.h>

// ------------- Tests Follow --------------
BOOST_AUTO_TEST_CASE( dummy )
{
    BOOST_CHECK(true);
}