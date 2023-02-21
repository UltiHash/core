#ifndef SERIALIZATION_F_SERIALIZATION_H
#define SERIALIZATION_F_SERIALIZATION_H

#include <filesystem>
#include <fstream>
#include <logging/logging_boost.h>
#include <sys/stat.h>
#include "../common/f_meta_data.h"
#include "../common/job_queue.h"
#include "EnDecoder.h"

#if defined(BSD)
#define stat_t stat
#else
#define stat_t stat64
#endif

namespace uh::client::serialization
{

// ---------------------------------------------------------------------------------------------------------------------

class f_serialization
{
public:

    f_serialization(const std::filesystem::path& UHV_path, common::job_queue<std::unique_ptr<common::f_meta_data>>&);
    ~f_serialization() = default;

    void serialize();
    void deserialize();

private:
    std::filesystem::path m_UHV_path;
    common::job_queue<std::unique_ptr<common::f_meta_data>>& m_job_queue;

};

// ---------------------------------------------------------------------------------------------------------------------

} // namespace uh::client::serialization

#endif
