#ifndef SERIALIZATION_F_SERIALIZATION_H
#define SERIALIZATION_F_SERIALIZATION_H

#include <logging/logging_boost.h>
#include <uhv/f_meta_data.h>
#include <uhv/job_queue.h>
#include "EnDecoder.h"
#include <filesystem>
#include <fstream>


namespace uh::uhv
{

// ---------------------------------------------------------------------

class f_serialization
{
public:

    f_serialization(std::filesystem::path, uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>&);

    ~f_serialization() = default;

    uint64_t serialize(const std::vector<std::filesystem::path>&);
    uint64_t deserialize(const std::filesystem::path&, bool create_files = true);

    static std::vector<std::uint8_t> serialize_f_meta_data(const std::unique_ptr<uh::uhv::f_meta_data>& ptr_f_meta_data,
                                        const std::filesystem::path& relative_path);
    static std::unique_ptr<uh::uhv::f_meta_data> deserialize_f_meta_data(std::vector<std::uint8_t>& uhv_container,
                                                                             std::vector<std::uint8_t>::iterator& it,
                                                                             const std::filesystem::path& dest_path);
private:
    std::filesystem::path m_UHV_path;
    uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& m_job_queue;


};

// ---------------------------------------------------------------------

} // namespace uh::uhv

#endif
