//
// Created by ankit on 14.12.22.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibUtil Recompilation Tree Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <serialization/recomp_tree.h>

// ------------- Tests Suites Follow --------------
using namespace uh::client::serialization;

BOOST_AUTO_TEST_SUITE( RecompilationTreeSuite )

BOOST_AUTO_TEST_CASE( rootNodeTest )
{
    treeNode<std::string, std::uint64_t> rootNode{};
    BOOST_TEST(rootNode.getChildren().size()==0);
    BOOST_TEST(rootNode.getParent()==nullptr);
    BOOST_TEST(rootNode.getData().size()==0);
    BOOST_TEST(rootNode.getSeek()==0);
}

BOOST_AUTO_TEST_CASE( rootwChildTest )
{
    treeNode<std::string, std::uint32_t> rootNode("/home/ankit/Downloads/", 32);
    rootNode.addChild("Videos", 64);
    BOOST_TEST(rootNode.getChildren().size()==1);
    BOOST_TEST(rootNode.getChild("Videos")->getParent()==&rootNode);
    BOOST_TEST(rootNode.getChild("Videos")->getData()=="Videos");
    BOOST_TEST(rootNode.getChild("Videos")->getChildren().size()==0);
    BOOST_TEST(rootNode.getChild("Videos")->getSeek()==64);
    rootNode.getChild("Videos")->addChild("MyDog", 332);
    rootNode.getChild("Videos")->addChild("MyCat", 332);
    rootNode.getChild("Videos")->addChild("MyTurtle", 332);
    rootNode.getChildByRPath("Videos/MyDog")->addChild("BeachDogPictures", 332);
}

BOOST_AUTO_TEST_CASE( relPathFuncCheck )
{
    treeNode<std::string, std::uint32_t> rootNode("/home/ankit/Downloads/", 32);
    rootNode.addChild("Videos", 64);
    rootNode.getChild("Videos")->addChild("MyDog", 100);
    rootNode.getChild("Videos")->addChild("MyCat", 200);
    rootNode.getChild("Videos")->addChild("MyTurtle", 222);
    rootNode.getChildByRPath("Videos/MyDog")->addChild("BeachDogPictures", 332);
    BOOST_TEST(rootNode.getChildByRPath("Videos/MyDog/BeachDogPictures")->getData()=="BeachDogPictures");
    BOOST_TEST(rootNode.getChildByRPath("Videos/MyDog/BeachDogPictures")->getSeek()==332);
}

BOOST_AUTO_TEST_CASE( BFSCheck )
{
    std::shared_ptr<treeNode<std::string,std::uint64_t>> rootNodePtr = std::make_shared<treeNode<std::string,std::uint64_t>>("/home/Downloads",0);
    rootNodePtr->addChild("Videos", 64);
    rootNodePtr->getChild("Videos")->addChild("MyDog", 100);
    rootNodePtr->getChildByRPath("Videos/MyDog")->addChild("BeachDogPictures", 332);
    rootNodePtr->getChild("Videos")->addChild("MyCat", 200);
    rootNodePtr->getChild("Videos")->addChild("MyTurtle", 222);
}

BOOST_AUTO_TEST_SUITE_END()