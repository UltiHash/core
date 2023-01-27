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
BOOST_AUTO_TEST_CASE(compare_test)
{
    uh::trees::tree_radix_custom<std::vector<unsigned char>> t;
    BOOST_CHECK(t.children.empty());
    std::string hello_string = "Hello World";
    std::string hello_string_long = "Hello World of tomorrow!";
    std::string hello_string_late = "Do not say \"Hello World!\"";
    auto data_string = std::vector<unsigned char>{hello_string.begin(), hello_string.end()};
    auto data_string_long = std::vector<unsigned char>{hello_string_long.begin(), hello_string_long.end()};
    auto data_string_late = std::vector<unsigned char>{hello_string_late.begin(), hello_string_late.end()};

    auto result1 = t.compare_ultihash(data_string_long, data_string);
    auto result2 = t.compare_ultihash(data_string_late, data_string_long);

    BOOST_CHECK(std::get<0>(result1[0]) == 0);
    BOOST_CHECK(std::get<1>(result1[0]) == 10);

    BOOST_CHECK(std::get<0>(result2[0]) == 12);
    BOOST_CHECK(std::get<1>(result2[0]) == 10);

    std::string hello_world_string = "Hello World";
    while (hello_world_string.size() < 2 * MINIMUM_MATCH_SIZE + 11) {
        hello_world_string.insert(hello_world_string.begin(), '-');
        hello_world_string.insert(hello_world_string.end(), '-');
    }
    std::string hello_string2 = "Hello";//totally matches at front
    auto data_string2 = std::vector<unsigned char>{hello_world_string.begin(), hello_world_string.end()};
    auto input_string_begin2 = std::vector<unsigned char>{hello_string2.begin(), hello_string2.end()};
    auto result3 = t.compare_ultihash(data_string2, input_string_begin2);
    BOOST_CHECK(std::get<0>(result3[0]) == 2);//data iterator
    BOOST_CHECK(std::get<1>(result3[0]) == 4);//input iterator begin
}

BOOST_AUTO_TEST_CASE(search_match_filter_test)
{
    //Test if the algorithm only detects matches that have an offset from 0 to front or back or a distance of match size
    std::string hello_world_string = "Hello World";
    while (hello_world_string.size() < 2 * MINIMUM_MATCH_SIZE + 11) {
        hello_world_string.insert(hello_world_string.begin(), '-');
        hello_world_string.insert(hello_world_string.end(), '-');
    }
    std::string hello_string = "Hello";//totally matches at front
    std::string world_string = "World";//totally matches at back
    auto data_string = std::vector<unsigned char>{hello_world_string.begin(), hello_world_string.end()};
    auto input_string_begin = std::vector<unsigned char>{hello_string.begin(), hello_string.end()};
    auto input_string_end = std::vector<unsigned char>{world_string.begin(), world_string.end()};

    uh::trees::tree_radix_custom<std::vector<unsigned char>> t{};
    auto result = t.search_match_filter<std::vector<unsigned char>,std::vector<unsigned char>,false>(data_string, input_string_begin);
    BOOST_CHECK(result.size() == 1);
    BOOST_CHECK(std::get<1>(result[0]) == 0);
    BOOST_CHECK(std::get<2>(result[0]) == 4);

    auto last_it_outer_list = (--(std::get<0>(result[0]).end()));
    auto last_it_inner_list = (--(last_it_outer_list->end()));

    BOOST_CHECK(std::get<0>(std::get<1>(*last_it_inner_list)[0]) == 2);//data iterator
    BOOST_CHECK(std::get<1>(std::get<1>(*last_it_inner_list)[0]) == 4);//input iterator begin

    data_string.erase(data_string.cbegin());
    data_string.erase(data_string.cend() - 1);
    std::string ello_Worl = "Hello World";//Algo strictly keeps front and back distance to prevent fragmentation, Match size limits distance to front and back if it does not exactly match
    input_string_begin = std::vector<unsigned char>{ello_Worl.begin(), ello_Worl.end()};

    result = t.search_match_filter<std::vector<unsigned char>,std::vector<unsigned char>,false>(data_string, input_string_begin);
    BOOST_CHECK(result.size() == 1);
    BOOST_CHECK(std::get<2>(result[0]) == 10);

    last_it_outer_list = (--(std::get<0>(result[0]).end()));
    last_it_inner_list = (--(last_it_outer_list->end()));

    BOOST_CHECK(std::get<0>(std::get<1>(*last_it_inner_list)[0]) == 1);//data iterator
    BOOST_CHECK(std::get<1>(std::get<1>(*last_it_inner_list)[0]) == 10);//input iterator begin
}

BOOST_AUTO_TEST_CASE(search_empty_test)
{
    uh::trees::tree_radix_custom<std::vector<unsigned char>> t;
    std::string hello_string = "Hello";
    auto data_string = std::vector<unsigned char>{hello_string.begin(), hello_string.end()};
    auto result = t.search<std::vector<unsigned char>,false>(data_string);
    BOOST_CHECK(result.empty());
}

BOOST_AUTO_TEST_CASE(radix_constructor_test)
{
    auto *t = new uh::trees::tree_radix_custom<std::vector<unsigned char>>{};
    BOOST_CHECK(t->children.empty());
    std::string hello_string = "Hello World";
    auto data_string = std::vector<unsigned char>{hello_string.begin(), hello_string.end()};
    delete t;
    //total match test
    t = new uh::trees::tree_radix_custom<std::vector<unsigned char>>();
    //empty add test
    auto result_test = t->add_test<std::vector<unsigned char>,false>(data_string);
    BOOST_CHECK(std::get<0>(result_test[0]) == 11 && std::get<1>(result_test[0]) == 11);
    delete t;
    t = new uh::trees::tree_radix_custom<std::vector<unsigned char>>(data_string);
    BOOST_CHECK(t->children.empty());
    BOOST_CHECK(t->size() == 11);
    BOOST_CHECK_EQUAL_COLLECTIONS(hello_string.begin(), hello_string.end(), t->data.begin(), t->data.end());
    //adding
    auto result = t->add<std::vector<unsigned char>,false>(data_string);
    BOOST_CHECK(std::get<0>(result[0]) == 11 && std::get<1>(result[0]) == 0);
    BOOST_CHECK(std::get<0>(*std::get<3>(result[0]).begin()).empty() && std::get<1>(*std::get<3>(result[0]).begin()).empty());//tree modified and added stay empty
    result_test = t->add_test<std::vector<unsigned char>,false>(data_string);
    BOOST_CHECK(std::get<0>(result_test[0]) == 11 && std::get<1>(result_test[0]) == 0);
    //total match and append test
    std::string tomorrow_string = "Hello World of tomorrow!";
    data_string = std::vector<unsigned char>{tomorrow_string.begin(), tomorrow_string.end()};
    //add test
    result_test = t->add_test(data_string);
    BOOST_CHECK(std::get<0>(result_test[0]) == 24 && std::get<1>(result_test[0]) == 13);
    //add
    result = t->add<std::vector<unsigned char>,false>(data_string);
    BOOST_CHECK(std::get<0>(result[0]) == 24 && std::get<1>(result[0]) == 13);
    BOOST_CHECK(std::get<0>(*std::get<3>(result[0]).begin()).empty() && std::get<1>(*std::get<3>(result[0]).begin()).size()==1);//tree modified empty and added with one new tree
    result_test = t->add_test(data_string);
    BOOST_CHECK(std::get<0>(result_test[0]) == 24 && std::get<1>(result_test[0]) == 0);//total match expected recursively

    delete t;
}

//test the function add and the function add_test and if they calculate the correct
BOOST_AUTO_TEST_CASE(add_test)
{
    auto data_string1 = std::string{"Hello World!"};
    auto data1 = std::vector<unsigned char>{data_string1.begin(),data_string1.end()};
    auto data_string2 = std::string{"Hello World!"};
    auto data2 = std::vector<unsigned char>{data_string2.begin(),data_string2.end()};
    auto data_string3 = std::string{"Hello World!"};
    auto data3 = std::vector<unsigned char>{data_string3.begin(),data_string3.end()};
    auto data_string4 = std::string{"Hello World!"};
    auto data4 = std::vector<unsigned char>{data_string4.begin(),data_string4.end()};
    auto data_string5 = std::string{"Hello World!"};
    auto data5 = std::vector<unsigned char>{data_string5.begin(),data_string5.end()};

    //TODO:

    //first tree test, match something at the back of the string

    //first tree append test, match something where a subset already exists and additional information is added

    //last tree test, match something that is the same at the beginning and then is different at a certain spot on

    //last tree append test, match something that is the same in the beginning or totally to the existing information and add some more information at the end of the string going into add function

    auto *t = new uh::trees::tree_radix_custom<std::vector<unsigned char>>{data1};


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

BOOST_AUTO_TEST_CASE(search_test)
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

BOOST_AUTO_TEST_CASE(insert_test)
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