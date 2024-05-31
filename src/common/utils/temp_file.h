
#ifndef UH_CLUSTER_TEMP_FILE_H
#define UH_CLUSTER_TEMP_FILE_H

#include <filesystem>
#include <string_view>
#include <unistd.h>

class temp_file {
public:
    temp_file(const std::filesystem::path& directory)
        : m_path(open_temp_file(directory)) {}

    auto get_path() const noexcept { return m_path; }

    void release_to(const std::filesystem::path& path) {
        std::filesystem::rename(m_path, path);
        m_remove = false;
    }
    ~temp_file() {
        if (m_remove) {
            std::filesystem::remove(m_path);
        }
    }

private:
    std::filesystem::path
    open_temp_file(const std::filesystem::path& temp_dir) {
        auto path = (temp_dir / FILENAME_TEMPLATE).string();

        int fd = mkstemp(path.data());
        if (fd == -1) {
            throw std::runtime_error("Could not create a temporary file");
        }
        close(fd);
        return path;
    }

    std::filesystem::path m_path;
    bool m_remove = true;

    constexpr static std::string_view FILENAME_TEMPLATE = "tempfile-XXXXXX";
};
#endif // UH_CLUSTER_TEMP_FILE_H
