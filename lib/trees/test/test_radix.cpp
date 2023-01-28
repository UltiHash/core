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

    uh::trees::tree_radix_custom t{};
    auto result = t.search_match_filter(data_string, input_string_begin);
    BOOST_CHECK(result.size() == 1);
    BOOST_CHECK(std::get<1>(result[0]) == 1);//number of matches is 1
    BOOST_CHECK(std::get<2>(result[0]) == 5);

    auto last_it_outer_list = (--(std::get<0>(result[0]).end()));
    auto last_it_inner_list = (--(last_it_outer_list->end()));

    BOOST_CHECK(std::get<0>(std::get<1>(*last_it_inner_list)[0]) == 2);//data iterator
    BOOST_CHECK(std::get<1>(std::get<1>(*last_it_inner_list)[0]) == 4);//input iterator begin

    data_string.erase(data_string.cbegin());
    data_string.erase(data_string.cend() - 1);
    std::string ello_Worl = "Hello World";//Algo strictly keeps front and back distance to prevent fragmentation, Match size limits distance to front and back if it does not exactly match
    input_string_begin = std::vector<unsigned char>{ello_Worl.begin(), ello_Worl.end()};

    result = t.search_match_filter(data_string, input_string_begin);
    BOOST_CHECK(result.size() == 1);
    BOOST_CHECK(std::get<2>(result[0]) == 11);

    last_it_outer_list = (--(std::get<0>(result[0]).end()));
    last_it_inner_list = (--(last_it_outer_list->end()));

    BOOST_CHECK(std::get<0>(std::get<1>(*last_it_inner_list)[0]) == 1);//data iterator
    BOOST_CHECK(std::get<1>(std::get<1>(*last_it_inner_list)[0]) == 10);//input iterator begin
}

BOOST_AUTO_TEST_CASE(search_empty_test)
{
    uh::trees::tree_radix_custom t;
    std::string hello_string = "Hello";
    auto data_string = std::vector<unsigned char>{hello_string.begin(), hello_string.end()};
    auto result = t.search(data_string);
    BOOST_CHECK(result.empty());
}

BOOST_AUTO_TEST_CASE(radix_constructor_test)
{
    auto *t = new uh::trees::tree_radix_custom{};
    BOOST_CHECK(t->children.empty());
    std::string hello_string = "Hello World";
    auto data_string = std::vector<unsigned char>{hello_string.begin(), hello_string.end()};
    delete t;
    //total match test
    t = new uh::trees::tree_radix_custom();
    //empty add test
    auto result_test = t->add_test(data_string);
    BOOST_CHECK(std::get<0>(result_test[0]) == 11 && std::get<1>(result_test[0]) == 11);
    delete t;
    t = new uh::trees::tree_radix_custom(data_string);
    BOOST_CHECK(t->children.empty());
    BOOST_CHECK(t->size() == 11);
    BOOST_CHECK_EQUAL_COLLECTIONS(hello_string.begin(), hello_string.end(), t->data.begin(), t->data.end());
    //adding
    auto result = t->add(data_string);
    BOOST_CHECK(std::get<0>(result[0]) == 11 && std::get<1>(result[0]) == 0);
    BOOST_CHECK(std::get<0>(*std::get<3>(result[0]).begin()).empty() && std::get<1>(*std::get<3>(result[0]).begin()).empty());//tree modified and added stay empty
    result_test = t->add_test(data_string);
    BOOST_CHECK(std::get<0>(result_test[0]) == 11 && std::get<1>(result_test[0]) == 0);
    //total match and append test
    std::string tomorrow_string = "Hello World of tomorrow!";
    data_string = std::vector<unsigned char>{tomorrow_string.begin(), tomorrow_string.end()};
    //add test
    result_test = t->add_test(data_string);
    BOOST_CHECK(std::get<0>(result_test[0]) == 24 && std::get<1>(result_test[0]) == 13);
    //add
    result = t->add(data_string);
    BOOST_CHECK(std::get<0>(result[0]) == 24 && std::get<1>(result[0]) == 13);
    BOOST_CHECK(std::get<0>(*std::get<3>(result[0]).begin()).empty() && std::get<1>(*std::get<3>(result[0]).begin()).size()==1);//tree modified empty and added with one new tree
    result_test = t->add_test(data_string);
    BOOST_CHECK(std::get<0>(result_test[0]) == 24 && std::get<1>(result_test[0]) == 0);//total match expected recursively

    delete t;
}

//test the function add and the function add_test and if they calculate the correct
BOOST_AUTO_TEST_CASE(add_test)
{
    auto data_string1 = std::string{"Hello World of tomorrow, I am nemo of the World and I deduplicate a lot of data!"};

    //first tree test, match something at the back of the string
    auto* t = new uh::trees::tree_radix_custom(data_string1);

    auto back_string = std::string{"a lot of data!"};

    auto result_test = t->add_test(back_string);
    BOOST_CHECK(std::get<0>(result_test[0]) == 14 && std::get<1>(result_test[0]) == 0);
    auto result = t->add(back_string);
    BOOST_CHECK(std::get<0>(result[0]) == 14 && std::get<1>(result[0]) == 0);
    BOOST_CHECK(std::get<0>(*std::get<3>(result[0]).begin()).size()==1 && std::get<1>(*std::get<3>(result[0]).begin()).size()==1);//one tree modified and one added

    delete t;

    //first tree append test, match something where a subset already exists and additional information is added
    t = new uh::trees::tree_radix_custom(data_string1);

    auto back_string_append = std::string{"a lot of data! And I can code."};

    result_test = t->add_test(back_string_append);
    BOOST_CHECK(std::get<0>(result_test[0]) == 30 && std::get<1>(result_test[0]) == 16);
    result = t->add(back_string_append);
    BOOST_CHECK(std::get<0>(result[0]) == 30 && std::get<1>(result[0]) == 16);
    BOOST_CHECK(std::get<0>(*std::get<3>(result[0]).begin()).size()==1 && std::get<1>(*std::get<3>(result[0]).begin()).size()==2);//one tree modified and 2 added

    delete t;

    //last tree test, match something that is the same at the beginning and then is different at a certain spot on
    t = new uh::trees::tree_radix_custom(data_string1);

    auto front_string = std::string{"Hello World"};

    result_test = t->add_test(front_string);
    BOOST_CHECK(std::get<0>(result_test[0]) == 11 && std::get<1>(result_test[0]) == 0);
    result = t->add(front_string);
    BOOST_CHECK(std::get<0>(result[0]) == 11 && std::get<1>(result[0]) == 0);
    BOOST_CHECK(std::get<0>(*std::get<3>(result[0]).begin()).size()==1 && std::get<1>(*std::get<3>(result[0]).begin()).size()==1);//one tree modified and one added

    delete t;

    //last tree append test, match something that is the same in the beginning or totally to the existing information and add some more information at the end of the string going into add function
    t = new uh::trees::tree_radix_custom(data_string1);

    auto front_string_append = std::string{"Hello World from yesterday"};

    result_test = t->add_test(front_string_append);
    BOOST_CHECK(std::get<0>(result_test[0]) == 26 && std::get<1>(result_test[0]) == 14);
    result = t->add(front_string_append);
    BOOST_CHECK(std::get<0>(result[0]) == 26 && std::get<1>(result[0]) == 14);
    BOOST_CHECK(std::get<0>(*std::get<3>(result[0]).begin()).size()==1 && std::get<1>(*std::get<3>(result[0]).begin()).size()==2);//one tree modified and 2 added

    delete t;

    //middle tree test, match something at the middle of the string
    t = new uh::trees::tree_radix_custom(data_string1);

    auto middle_string = std::string{"World of tomorrow"};

    result_test = t->add_test(middle_string);
    BOOST_CHECK(std::get<0>(result_test[0]) == 17 && std::get<1>(result_test[0]) == 0);
    result = t->add(middle_string);
    BOOST_CHECK(std::get<0>(result[0]) == 17 && std::get<1>(result[0]) == 0);
    BOOST_CHECK(std::get<0>(*std::get<3>(result[0]).begin()).size()==1 && std::get<1>(*std::get<3>(result[0]).begin()).size()==2);//one tree modified and two added

    delete t;

    //middle tree append test, match something in the middle where a subset already exists and additional information is added
    t = new uh::trees::tree_radix_custom(data_string1);

    auto middle_string_append = std::string{"World of tomorrow, coming soon!"};

    result_test = t->add_test(middle_string_append);
    BOOST_CHECK(std::get<0>(result_test[0]) == 31 && std::get<1>(result_test[0]) == 14);
    result = t->add(middle_string_append);
    BOOST_CHECK(std::get<0>(result[0]) == 31 && std::get<1>(result[0]) == 16);
    BOOST_CHECK(std::get<0>(*std::get<3>(result[0]).begin()).size()==1 && std::get<1>(*std::get<3>(result[0]).begin()).size()==3);//one tree modified and 3 added

    delete t;

    //cascading add
    auto data_string2 = std::string{"World of tomorrow"};
    auto data_string3 = std::string{"tomorrow, I am nemo"};
    auto data_string4 = std::string{" of tomorrow, I am nemo of the World and"};

    t = new uh::trees::tree_radix_custom(data_string1);

    result_test = t->add_test(data_string2);
    BOOST_CHECK(std::get<0>(result_test[0]) == 17 && std::get<1>(result_test[0]) == 0);
    result = t->add(data_string2);
    BOOST_CHECK(std::get<0>(result[0]) == 17 && std::get<1>(result[0]) == 0);
    BOOST_CHECK(std::get<0>(*std::get<3>(result[0]).begin()).size()==1 && std::get<1>(*std::get<3>(result[0]).begin()).size()==3);//one tree modified and 3 added

    result_test = t->add_test(data_string3);
    BOOST_CHECK(std::get<0>(result_test[0]) == 19 && std::get<1>(result_test[0]) == 0);
    result = t->add(data_string3);
    BOOST_CHECK(std::get<0>(result[0]) == 19 && std::get<1>(result[0]) == 0);
    BOOST_CHECK(std::get<0>(*std::get<3>(result[0]).begin()).size()==2 && std::get<1>(*std::get<3>(result[0]).begin()).size()==2);//2 trees modified and 2 added

    result_test = t->add_test(data_string4);
    BOOST_CHECK(std::get<0>(result_test[0]) == 40 && std::get<1>(result_test[0]) == 0);
    result = t->add(data_string4);
    BOOST_CHECK(std::get<0>(result[0]) == 40 && std::get<1>(result[0]) == 0);
    BOOST_CHECK(std::get<0>(*std::get<3>(result[0]).begin()).size()==3 && std::get<1>(*std::get<3>(result[0]).begin()).size()==3);//one tree modified and 3 added

    delete t;

    //overlap add
    t = new uh::trees::tree_radix_custom(std::string{"aaaaaabbbbbaaaaaabbbbbaaaaaabbbbbaaaaaa"});//matches at position 0,11,22 and fragments into 5 tree segments that can be filled with the same HDD uplink
    auto overlap_string = std::string{"aaaaabbbbbaaaaabbbbbaaaaa"};

    result_test = t->add_test(overlap_string);
    BOOST_CHECK(std::get<0>(result_test[0]) == 27 && std::get<1>(result_test[0]) == 0);
    result = t->add(overlap_string);
    BOOST_CHECK(std::get<0>(result[0]) == 27 && std::get<1>(result[0]) == 0);
    BOOST_CHECK(std::get<0>(*std::get<3>(result[0]).begin()).size()==1 && std::get<1>(*std::get<3>(result[0]).begin()).size()==4);//one tree modified and 4 added from a single node
}
