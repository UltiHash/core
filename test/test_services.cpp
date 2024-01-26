#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "service_registry tests"
#endif

#define REGISTRY_ENDPOINT "http://127.0.0.1:2379"

#include "common/registry/service_registry.h"

// ------------- Tests Suites Follow --------------

// Create two services, one storage another dedup. Upon adding and removing storage service, the dedup node
// should be able to add/remove the clients in the list -> watcher functionality when storage is not there yet
// non watcher functionality addition/removal -> when sotrage is already there

//namespace uh::cluster {
//
//    BOOST_AUTO_TEST_CASE ( basic_register_retrieve_deregister )
//            {
//                    service_registry querying_registry(uh::cluster::DEDUPLICATOR_SERVICE, 0, REGISTRY_ENDPOINT);
//
////        {
////            auto service_endpoints = querying_registry.get_service_instances(uh::cluster::STORAGE_SERVICE);
////            BOOST_CHECK(service_endpoints.empty());
////        }
//
//            {
//                service_registry registering_registry(uh::cluster::STORAGE_SERVICE, 42, REGISTRY_ENDPOINT);
//                auto reg = registering_registry.register_service({.port = 9200});
//
//                querying_registry.wait_for_dependency(uh::cluster::STORAGE_SERVICE);
//                auto service_endpoints = querying_registry.get_service_instances(uh::cluster::STORAGE_SERVICE);
//                BOOST_CHECK(service_endpoints.size() == 1);
//                BOOST_CHECK(service_endpoints.begin()->role == uh::cluster::STORAGE_SERVICE);
//                BOOST_CHECK(service_endpoints.begin()->id == 42);
//                BOOST_CHECK(service_endpoints.begin()->host == boost::asio::ip::host_name());
//                BOOST_CHECK(service_endpoints.begin()->port == 9200);
//            }
//
//            {
//                auto service_endpoints = querying_registry.get_service_instances(uh::cluster::STORAGE_SERVICE);
//                BOOST_CHECK(service_endpoints.empty());
//            }
//
//            }
//
//    BOOST_AUTO_TEST_CASE( wait_for_dependencies, *boost::unit_test::timeout(30) )
//{
//    service_registry querying_registry(uh::cluster::DEDUPLICATOR_SERVICE, 0, REGISTRY_ENDPOINT);
//
//{
//    auto service_endpoints = querying_registry.get_service_instances(uh::cluster::STORAGE_SERVICE);
//    BOOST_CHECK(service_endpoints.empty());
//}
//
//service_registry registering_registry(uh::cluster::STORAGE_SERVICE, 42, REGISTRY_ENDPOINT);
//auto reg = registering_registry.register_service({.port = 9200});
//
//querying_registry.wait_for_dependency(uh::cluster::STORAGE_SERVICE);
//
//
//auto service_endpoints = querying_registry.get_service_instances(uh::cluster::STORAGE_SERVICE);
//BOOST_CHECK(service_endpoints.size() == 1);
//}
//} // end namespace uh::cluster
