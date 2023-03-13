#ifndef SERIALIZATION_F_SERIALIZATION_H
#define SERIALIZATION_F_SERIALIZATION_H

#include <filesystem>
#include <fstream>
#include <logging/logging_boost.h>
#include <utility>

#include "../common/f_meta_data.h"
#include "../common/job_queue.h"
#include "EnDecoder.h"
#include "serialization/buffered_serializer.h"
#include "serialization/deserializer.h"
#include "io/file.h"

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

class f_serialization
{
public:

    f_serialization(std::filesystem::path, common::job_queue<std::unique_ptr<common::f_meta_data>>&);
    ~f_serialization() = default;

    uint64_t serialize(const std::vector<std::filesystem::path>&);
    uint64_t deserialize(const std::filesystem::path&);

private:
    std::filesystem::path m_UHV_path;
    common::job_queue<std::unique_ptr<common::f_meta_data>>& m_job_queue;
};

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

#endif
