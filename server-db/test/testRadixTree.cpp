//
// Created by juan on 01.12.22.
//
#define BOOST_TEST_DYN_LINK
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhServerDb Dummy Tests"
#endif

#include <boost/test/unit_test.hpp>
#include "../src/radix_tree.h"

// ------------- Tests Follow --------------
BOOST_AUTO_TEST_CASE( radixTree )
{
    //TODO
    // Create real tests.
        // Driver program for the Trie Data Structure Operations
        TrieNode* root = make_trienode('\0');
        root = insert_trie(root, (char*)"hello");
        root = insert_trie(root, (char*)"hi");
        root = insert_trie(root, (char*)"teabag");
        root = insert_trie(root, (char*)"teacan");
        print_search(root, (char*)"tea");
        print_search(root, (char*)"teabag");
        print_search(root, (char*)"teacan");
        print_search(root, (char*)"hi");
        print_search(root, (char*)"hey");
        print_search(root, (char*)"hello");
        print_trie(root);
        printf("\n");
        root = delete_trie(root, (char*)"hello");
        printf("After deleting 'hello'...\n");
        print_trie(root);
        printf("\n");
        root = delete_trie(root, (char*)"teacan");
        printf("After deleting 'teacan'...\n");
        print_trie(root);
        printf("\n");
        free_trienode(root);
    BOOST_CHECK(true);
}
