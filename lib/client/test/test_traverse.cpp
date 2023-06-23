#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uh-cli Traverse Tests"
#endif

#include <filesystem>
#include <fstream>
#include <boost/test/unit_test.hpp>

#include <client/traverse.h>
#include "uhv/job_queue.h"
#include "uhv/meta_data.h"

// ------------- Tests Suites Follow --------------
using namespace uh::client;
namespace fs = std::filesystem;

namespace
{

BOOST_AUTO_TEST_SUITE(TraverseSuite)

// ---------------------------------------------------------------------

    struct fs_fixture
    {
        fs_fixture()
        {
            fs::path dir_path = "./mock_dir/mock_subdir";
            fs::create_directories(dir_path);
            fs::path filepath = dir_path / "mock_file.txt";
            std::ofstream outfile(filepath);
            outfile << "Hello, world!" << std::endl;
            outfile.close();
        }

        ~fs_fixture()
        {
            fs::path path_to_delete = "./mock_dir";
            fs::remove_all(path_to_delete);
        }
    };

// ---------------------------------------------------------------------

    BOOST_FIXTURE_TEST_CASE(traverseTest, fs_fixture)
    {
        std::filesystem::path path = "./mock_dir/mock_subdir";

        uh::uhv::job_queue<std::unique_ptr<
                uh::uhv::meta_data>>
                output_jq;
        traverse traverse(path, output_jq);
        traverse.run();

        std::vector<std::filesystem::path> all_metadata;
        output_jq.stop();

        while (auto item = output_jq.get_job())
        {
            if (item == std::nullopt)
                break;
            else
                all_metadata.push_back(item.value()->path());
        }

        BOOST_TEST(all_metadata[0] == "./mock_dir/mock_subdir");
        BOOST_TEST(all_metadata[1] == "./mock_dir/mock_subdir/mock_file.txt");
    }

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE_END()

// ---------------------------------------------------------------------

} // namespace

