#ifndef SERIALIZATION_F_SERIALIZATION_H
#define SERIALIZATION_F_SERIALIZATION_H

#include <filesystem>
#include <iostream>
#include <fstream>
#include <logging/logging_boost.h>
#include <uhv/f_meta_data.h>
#include <uhv/job_queue.h>
#include <serialization/buffered_serializer.h>
#include <serialization/deserializer.h>
#include <io/file.h>
#include <io/sstream_device.h>


namespace uh::uhv
{

// ---------------------------------------------------------------------

class f_serialization
{
public:

    f_serialization(std::filesystem::path, uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>&, bool=false);

    ~f_serialization() = default;

    uint64_t serialize(const std::vector<std::filesystem::path>&);
    uint64_t deserialize(const std::filesystem::path&, bool create_files = true);

    static std::vector<char> serialize_f_meta_data(const std::unique_ptr<uh::uhv::f_meta_data>& ptr_f_meta_data,
                                        const std::filesystem::path& relative_path);
    //static std::unique_ptr<uh::uhv::f_meta_data> deserialize_f_meta_data(std::vector<std::uint8_t>& uhv_container,
    //                                                                         std::vector<std::uint8_t>::iterator& it,
    //                                                                         const std::filesystem::path& dest_path);

private:
    std::filesystem::path m_UHV_path;
    uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& m_job_queue;
    bool m_overwrite;

};

// ---------------------------------------------------------------------

} // namespace uh::uhv

#endif
