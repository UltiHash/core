#include <common/utils/time_utils.h>
#include <common/utils/misc.h>
#include <common/service_interfaces/deduplicator_interface.h>
#include <deduplicator/interfaces/local_deduplicator.h>
#include <storage/interfaces/local_storage.h>
#include <common/utils/temp_directory.h>

#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <readline/readline.h>
#include <readline/history.h>

using namespace uh::cluster;

const constexpr char* PROMPT = "$uh> ";

struct state {
    state(unsigned threads = 4)
        : ioc(threads),
          work(ioc.get_executor()),
          storage(std::make_unique<local_storage>(0, ds_config, std::list<std::filesystem::path>{ tmp.path() })),
          deduplicator(std::make_unique<local_deduplicator>(ioc, dd_config, *storage))
    {
        while (threads > 0) {
            m_threads.push_back(std::thread([this]() { ioc.run(); }));
            --threads;
        }
    }

    sn::interface& reset_storage() {
        tmp = temp_directory();
        storage = std::make_unique<local_storage>(0, ds_config, std::list<std::filesystem::path>{ tmp.path() });
        return *storage;
    }

    ~state() {
        work.reset();
        ioc.stop();
        for (auto& t : m_threads) {
            t.join();
        }
    }

    temp_directory tmp;
    bool running = true;
    boost::asio::io_context ioc;
    boost::asio::executor_work_guard<decltype(ioc.get_executor())> work;
    deduplicator_config dd_config;
    data_store_config ds_config = { .max_file_size = 128 * MEBI_BYTE,
                                    .max_data_store_size = 1 * GIBI_BYTE,
                                    .page_size = 8 * KIBI_BYTE };
    std::unique_ptr<sn::interface> storage;
    std::unique_ptr<deduplicator_interface> deduplicator;
    context ctx;

private:
    std::vector<std::thread> m_threads;
};

class command {
public:
    virtual void run(const std::vector<std::string>& args) = 0;
    virtual ~command() = default;
};

struct quit : public command {
    quit(state& s) : m_s(s) {}

    void run(const std::vector<std::string>&) override {
        std::cout << "Bye!\n";
        m_s.running = false;
    }

private:
    state& m_s;
};

struct reset_dd : public command {
    reset_dd(state& s) : m_s(s) {}

    void run(const std::vector<std::string>& args) override {
        if ((args.size() >= 2 && args[1] == "local") || args.size() == 1) {
            std::cout << "resetting deduplicator: local\n";

            auto& storage = m_s.reset_storage();
            m_s.deduplicator = std::make_unique<local_deduplicator>(m_s.ioc, m_s.dd_config, storage);
            return;
        }

        throw std::runtime_error("unknown deduplication implementation: " + args[1]);
    }

private:
    state& m_s;
};

struct load : public command {
    load(state& s) : m_s(s) {}

    void run(const std::vector<std::string>& args) override {
        if (args.size() < 2) {
            throw std::runtime_error("filename required");
        }

        std::cout << "loading and integrating " << args[1] << " ...\n";

        std::string contents = read_file(args[1]);

        timer t;
        auto future = boost::asio::co_spawn(m_s.ioc, m_s.deduplicator->deduplicate(m_s.ctx, contents), boost::asio::use_future);
        auto result = future.get();
        auto seconds = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(t.passed());
        auto mb_s = (contents.size() / MEBI_BYTE) / seconds.count();

        std::cout << "integrated " << contents.size() << " bytes:\n"
            << "fragments: " << result.addr.size () << "\n"
            << "data-size: " << result.addr.data_size() << "\n"
            << "effective size: " << result.effective_size << "\n"
            << "time required: " << seconds << "\n"
            << "throughput: " << mb_s << " MB/s\n";
    }

private:
    state& m_s;
};

struct help : public command {
    void run(const std::vector<std::string>& args) override {
        std::cout
            << "dd-shell: dedupe-shell ... tool to run deduplication in a "
                "controlled way\n"
            << "\n"
            << "commands are:\n"
            << "help    - this page\n"
            << "quit    - leave the shell\n"
            << "\n"
            << "reset [implementation] - reset the deduplication framework to "
                "the given implementation (default = local)\n"
            << "load <filename> - load and deduplicate the given file\n";
    }
};

std::map<std::string, std::unique_ptr<command>> commands;

void fill_commands(state& s) {
    commands["help"] = std::make_unique<help>();
    commands["quit"] = std::make_unique<quit>(s);
    commands["reset"] = std::make_unique<reset_dd>(s);
    commands["load"] = std::make_unique<load>(s);
}

std::optional<std::string> readline() {
    char* cline = ::readline(PROMPT);;
    if (cline == nullptr) {
        return {};
    }

    std::string rv = cline;
    free(cline);
    return rv;
}

std::vector<std::string> parse(std::string_view line) {
    std::vector<std::string> rv;

    bool in_str = false;
    bool escaped = false;
    std::string current;

    for (std::size_t pos = 0ull; pos < line.size(); ++pos) {
        if (escaped) {
            switch (line[pos]) {
                case 'n': current += '\n'; break;
                case 't': current += '\t'; break;
                case 'r': current += '\r'; break;
                default: current += line[pos];
            }
            escaped = false;
            continue;
        }

        if (in_str) {
            if (line[pos] == '"') {
                in_str = false;
            } else {
                current += line[pos];
            }
            continue;
        }

        switch (line[pos]) {
            case '\\': escaped = true; break;
            case '"': in_str = true; break;
            case ' ':
                rv.push_back(current);
                current.clear();
                break;
            default:
                current += line[pos];
        }
    }

    if (!current.empty()) {
        rv.push_back(current);
    }

    return rv;
}

void serve() {

    state s;
    fill_commands(s);

    const char* home = getenv("HOME");
    if (home == nullptr) {
        home = ".";
    }
    std::string history_file = std::string(home) + "/.dd-shell-history";

    using_history();
    read_history(history_file.c_str());

    while (auto line = readline()) {
        try {
            auto parsed = parse(*line);

            auto& cmd = commands.at(parsed.at(0));
            add_history(line->c_str());

            cmd->run(parsed);

            if (!s.running) {
                break;
            }

        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }

    write_history(history_file.c_str());
}


int main(int argc, char** argv) {
    try {
        serve();

    } catch (const std::exception& e) {
        std::cerr << "FATAL: " << e.what() << "\n";
    }
}
