#include <logging/logging_boost.h>
#include <net/plain_socket.h>
#include <protocol/client_factory.h>
#include <protocol/client_pool.h>
#include <client/upload.h>
#include <client/download.h>
#include <client/traverse.h>
#include <options/app_config.h>
#include <options/chunking_options.h>
#include <uhv/file.h>

#include <config.hpp>
#include <client_options/client_options.h>
#include <client_options/agency_connection.h>

// ---------------------------------------------------------------------

using namespace uh;
using namespace uh::uhv;
using namespace uh::client;

// ---------------------------------------------------------------------

APPLICATION_CONFIG
(
    (client, uh::client::option::client_options),
    (agency, uh::client::option::agency_connection),
    (chunking, uh::options::chunking_options)
);

// ---------------------------------------------------------------------

void handle_errors(const std::string& message,
                   const std::map<std::filesystem::path, std::optional<std::string>>& results)
{
    bool errors = false;

    for (const auto& result : results)
    {
        if (!result.second)
        {
            continue;
        }

        if (!errors)
        {
            std::cerr << message << "\n";
            errors = true;
        }

        std::cerr << result.first << ": " << *result.second << "\n";
    }

    if (errors)
    {
        throw std::runtime_error(message);
    }
}

// ---------------------------------------------------------------------

void integrate(protocol::client_pool& pool,
               unsigned worker_count,
               const chunking::config& chunker_config,
               const std::filesystem::path& input,
               const std::filesystem::path& output,
               bool overwrite)
{

    if (!std::filesystem::is_directory(input)) {
        throw std::runtime_error ("The input path must be a directory!");
    }

    auto time_start = std::chrono::system_clock::now();

    uhv::job_queue<std::unique_ptr<uhv::meta_data>> q_meta_data;
    std::list<std::future<std::unique_ptr<uhv::meta_data>>> metadata;

    std::size_t effective_size = 0u;

    {
        uh::chunking::mod chunking_module(chunker_config);

        upload upload_class(pool, q_meta_data,
                            metadata, chunking_module,
                            output, worker_count);
        upload_class.spawn_threads();

        traverse traverse_class(input, q_meta_data);

        upload_class.join();
        effective_size = upload_class.m_total_effective_size;
        handle_errors("there were errors during upload", upload_class.results());
    }

    std::size_t size = 0u;

    std::list<std::unique_ptr<uhv::meta_data>> files;
    for (auto& next : metadata)
    {
        auto md = next.get();
        if (md->type() == uhv::uh_file_type::regular)
        {
            size += md->size();
            effective_size += md->effective_size();
        }

        if (input != md->path()) [[likely]]
        {
            md->set_path(std::filesystem::relative(md->path(), input));
        }
        else
        {
            md->set_path(std::filesystem::path());
        }

        files.push_back(std::move(md));
    }

    files.sort([](auto& ml, auto& mr)
               { return ml->path() < mr->path(); });

    uhv::file file(output);
    file.serialize(files);

    auto time_end = std::chrono::system_clock::now();

    auto time_diff = std::chrono::duration<double>(time_end - time_start);
    double seconds = time_diff.count();
    double mbytes = static_cast<double>(size) / (1024 * 1024);

    double dedup_ratio {1};
    if(size)dedup_ratio = (double) effective_size / (double) size;

    std::cout << "space saving: " << (double) 1 - dedup_ratio << std::endl;
    std::cout << "encoding speed: " << (mbytes / seconds) << " Mb/s" << std::endl;
}

// ---------------------------------------------------------------------

void retrieve(protocol::client_pool& pool,
              unsigned worker_count,
              const std::filesystem::path& input_path,
              const std::filesystem::path& output_path)
{
    uint64_t size = 0;
    auto time_start = std::chrono::system_clock::now();

    std::filesystem::create_directories(output_path);
    uhv::job_queue<std::unique_ptr<uhv::meta_data>> q_meta_data;

    {
        download download_class(pool, q_meta_data, output_path, worker_count);
        download_class.spawn_threads();

        uhv::file file(input_path);
        file.deserialize(q_meta_data);

        download_class.join();
        size = download_class.size();
        handle_errors("there were errors during download", download_class.results());
    }

    auto time_end = std::chrono::system_clock::now();

    auto time_diff = std::chrono::duration<double>(time_end - time_start);
    double seconds = time_diff.count();
    double mbytes = static_cast<double>(size) / (1024 * 1024);

    std::cout << "decoding speed: " << (mbytes / seconds) << " Mb/s" << std::endl;
}

// ---------------------------------------------------------------------

int main(int argc, const char* argv[])
{
    try
    {
        application_config config;
        config.add_desc(
            "General Usage:\n(integrate) - ./uh-cli --integrate <destination_uh_file_name>.uh <origin_directory> --agency-node <host>:<port>\n"
            "(retrieve)  - ./uh-cli --retrieve <destination_uh_file_name>.uh --target <target_directory> --agency-node <host>:<port>");
        if (config.evaluate(argc, argv) == uh::options::action::exit)
        {
            return 0;
        }

        boost::asio::io_context io;
        std::stringstream s;
        s << PROJECT_NAME << " " << PROJECT_VERSION;
        uh::protocol::client_factory_config cf_config
            {
                .client_version = s.str()
            };

        uh::protocol::client_pool client_pool(
            std::make_unique<protocol::client_factory>(
                std::make_unique<net::plain_socket_factory>(io, config.agency().hostname, config.agency().port),
                cf_config),
            config.agency().pool_size);

        const auto& client_config = config.client();
        switch (client_config.m_option)
        {
            case option::options_chosen::integrate:
                integrate(client_pool,
                          client_config.m_worker_count,
                          config.chunking(),
                          client_config.m_inputPaths[0],
                          client_config.m_outputPath,
                          client_config.m_overwrite);
                break;

            default:
            case option::options_chosen::retrieve:
                retrieve(client_pool,
                         client_config.m_worker_count,
                         client_config.m_inputPaths[0],
                         client_config.m_outputPath);
                break;
        }
    }
    catch (const std::exception& e)
    {
        FATAL << e.what() << '\n';
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    catch (...)
    {
        FATAL << "unknown exception occurred";
        std::cerr << "Error: unknown error\n";
        return 1;
    }
    return EXIT_SUCCESS;
}

// ---------------------------------------------------------------------
