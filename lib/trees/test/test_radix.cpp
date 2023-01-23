//
// Created by benjamin on 18.12.22.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibTrees Radix Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <trees/tree_radix_custom.h>

// ------------- Tests Follow --------------
BOOST_AUTO_TEST_CASE( constructor )
{
    uh::trees::tree_radix_custom<std::vector<unsigned char>> t;
    BOOST_CHECK(t.children_reference().empty());
    std::string hello_string = "Hello World";
    uh::trees::tree_radix_custom t_hello<std::vector<unsigned char>>(std::vector<unsigned char>{hello_string.begin(),hello_string.end()});
    BOOST_CHECK(t_hello.children_reference().empty());
    BOOST_CHECK(t_hello.size()==11);
    BOOST_CHECK(std::strncmp(t_hello.data_vector(),"Hello World",11)==0);
}

BOOST_AUTO_TEST_CASE( add_test )
{
    /*
    uh::trees::tree_radix_custom t_nil1;
    auto address_list1 = t_nil1.add("Hello World",11);
    BOOST_CHECK_EQUAL(&t_nil1 , address_list1.front());
    //identical match
    address_list1 = t_nil1.add("Hello World",11);
    BOOST_CHECK_EQUAL(&t_nil1 , address_list1.front());

    //test overfitting
    uh::trees::tree_radix_custom *t_nil2=t_nil1.copy_recursive();
    auto address_list2=t_nil2->add("Hello World of tomorrow!",24);
    BOOST_CHECK(t_nil2->has_children());
    BOOST_CHECK_EQUAL(std::distance(address_list2.begin(),address_list2.end()),2);
    BOOST_CHECK(std::strncmp(t_nil2->data_blob(),"Hello World",11)==0);
    BOOST_CHECK(std::strncmp(t_nil2->child(' ')->data_blob()," of tomorrow!",13)==0);
    //one more overfit
    address_list2=t_nil2->add("Hello World of tomorrow! I am superman!",39);
    BOOST_CHECK(t_nil2->has_children());
    BOOST_CHECK_EQUAL(std::distance(address_list2.begin(),address_list2.end()),3);
    BOOST_CHECK(std::strncmp(t_nil2->data_blob(),"Hello World",11)==0);
    BOOST_CHECK(std::strncmp(t_nil2->child(' ')->data_blob()," of tomorrow!",13)==0);
    BOOST_CHECK(std::strncmp(t_nil2->child(' ')->child(' ')->data_blob()," I am superman!",15)==0);
    //test underfitting
    address_list2=t_nil2->add("Hello",5);
    BOOST_CHECK(t_nil2->has_children());
    BOOST_CHECK_EQUAL(std::distance(address_list2.begin(),address_list2.end()),1);
    BOOST_CHECK(std::strncmp(t_nil2->data_blob(),"Hello",5)==0);
    BOOST_CHECK(std::strncmp(t_nil2->child(' ')->data_blob()," World",6)==0);
    BOOST_CHECK(std::strncmp(t_nil2->child(' ')->child(' ')->data_blob()," of tomorrow!",13)==0);
    BOOST_CHECK(std::strncmp(t_nil2->child(' ')->child(' ')->child(' ')->data_blob()," I am superman!",15)==0);

    //check if sequence is returned
    address_list2=t_nil2->add("Hello World of tomorrow! I am superman!",39);
    std::vector<uh::trees::tree_radix_custom*> address_seq{t_nil2,t_nil2->child(' '),t_nil2->child(' ')->child(' '),
                                                           t_nil2->child(' ')->child(' ')->child(' ')};
    BOOST_CHECK_EQUAL_COLLECTIONS(address_list2.begin(),address_list2.end(),address_seq.begin(),address_seq.end());

    //check 0 fit integration
    address_list2=t_nil2->add("Bye",3);
    BOOST_CHECK_EQUAL(t_nil2->size(),0);
    BOOST_CHECK(std::strncmp(t_nil2->child('B')->data_blob(),"Bye",3)==0);
    BOOST_CHECK(std::strncmp(t_nil2->child('H')->data_blob(),"Hello",5)==0);
    std::vector<uh::trees::tree_radix_custom*> address_seq2{t_nil2,t_nil2->child('B')};
    BOOST_CHECK_EQUAL_COLLECTIONS(address_list2.begin(),address_list2.end(),address_seq2.begin(),address_seq2.end());

    t_nil2->destroy_recursive();
    delete t_nil2;*/
}

BOOST_AUTO_TEST_CASE( search_test )
{
    /*
    uh::trees::tree_radix_custom t_nil1;
    //check search on empty tree
    auto s_result = t_nil1.search("Hello Wor",9);
    BOOST_CHECK_EQUAL(std::get<0>(s_result).size(),0);
    BOOST_CHECK_EQUAL(std::get<1>(s_result),0);
    //check search on filled tree
    auto address_list1 = t_nil1.add("Hello World",11);
    address_list1 = t_nil1.add("Hello World of tomorrow!",24);
    address_list1 = t_nil1.add("Hello World of tomorrow! I am superman!",39);
    address_list1 = t_nil1.add("Hello",5);
    //underfitting
    auto s_result2 = t_nil1.search("Hello Wor",9);
    BOOST_CHECK_EQUAL(std::get<0>(s_result2).size(),2);
    BOOST_CHECK_EQUAL(std::get<1>(s_result2),9);
    //no fit
    auto s_result3 = t_nil1.search("Bye Wor",7);
    BOOST_CHECK_EQUAL(std::get<0>(s_result3).size(),0);
    BOOST_CHECK_EQUAL(std::get<1>(s_result3),0);
    //overfitting
    auto s_result4 = t_nil1.search("Hello World of tomorrow! I am superman! Your saviour.",53);
    BOOST_CHECK_EQUAL(std::get<0>(s_result4).size(),4);
    BOOST_CHECK_EQUAL(std::get<1>(s_result4),39);
    //search no fit
    auto s_result5 = t_nil1.search("No Wor",6);
    BOOST_CHECK_EQUAL(std::get<0>(s_result5).size(),0);
    BOOST_CHECK_EQUAL(std::get<1>(s_result5),0);

    t_nil1.destroy_recursive();
     */
}

BOOST_AUTO_TEST_CASE( insert_test )
{
    uh::trees::tree_radix_custom<std::vector<unsigned char>> t_nil1;
    uh::trees::tree_radix_custom<std::vector<unsigned char>> t_nil2;
    //check search on empty tree
    //check search on filled tree
    /*
    auto address_list1 = t_nil1.add("Hello World",11);
    address_list1 = t_nil1.add("Hello World of tomorrow!",24);
    address_list1 = t_nil1.add("Hello World of tomorrow! I am superman!",39);
    address_list1 = t_nil1.add("Hello",5);

    auto address_list2 = t_nil2.add("Hello World",11);
    address_list2 = t_nil2.add("Bye World of tomorrow!",22);
    address_list2 = t_nil2.add("Hello World of yesterday! I am superman!",40);//TODO: bug where memcpy copies 30 elements instead of 29
    address_list2 = t_nil2.add("Bye",3);

    t_nil1.insert(&t_nil2);

    auto s_result = t_nil1.search("Bye World of tomorrow!",22);
    BOOST_CHECK_EQUAL(std::get<0>(s_result).size(),3);
    BOOST_CHECK_EQUAL(std::get<1>(s_result),22);

    auto s_result1 = t_nil1.search("Hello World of yesterday! I am superman!",40);//TODO: The word "of" is segmented in t_nil1 for some reason.
    BOOST_CHECK_EQUAL(std::get<0>(s_result).size(),3);
    BOOST_CHECK_EQUAL(std::get<1>(s_result),40);*/
}