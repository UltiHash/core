#include "file.h"

#include <serialization/buffered_serializer.h>
#include <serialization/deserializer.h>
#include <io/file.h>

#include <ios>


namespace uh::uhv
{

namespace
{

// ---------------------------------------------------------------------

void serialize_f_meta_data(io::device& device, uh::uhv::f_meta_data& md)
{
    uh::serialization::buffered_serializer serializer(device);

    serializer.write(md.f_path().string());
    serializer.write(md.f_type());
    serializer.write(md.f_permissions());

    if (md.f_type() == uhv::uh_file_type::regular) {
        serializer.write(md.f_size());
        serializer.write(md.f_hashes());
        serializer.write(md.f_chunk_sizes());
    }

    serializer.sync();
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::uhv::f_meta_data> deserialize_f_meta_data(io::device& device)
{
    uh::serialization::sl_deserializer deserializer(device);
    std::unique_ptr<uh::uhv::f_meta_data> p_f_meta_data = std::make_unique<uh::uhv::f_meta_data>();

    p_f_meta_data->set_f_path(deserializer.read <std::string> ());
    p_f_meta_data->set_f_type(deserializer.read <std::uint8_t> ());
    p_f_meta_data->set_f_permissions(deserializer.read <std::uint32_t> ());

    if (p_f_meta_data->f_type() == uhv::uh_file_type::regular)
    {
        p_f_meta_data->set_f_size(deserializer.read <std::uint64_t> ());
        p_f_meta_data->set_f_hashes(deserializer.read <std::vector <char>> ());
        p_f_meta_data->add_chunk_sizes(deserializer.read<std::vector <uint32_t>>());
    }

    return p_f_meta_data;
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

file::file(const std::filesystem::path& path)
    : m_path(path)
{
}

// ---------------------------------------------------------------------

void file::serialize(const std::list<std::unique_ptr<uhv::f_meta_data>>& metadata)
{
    io::file file (m_path, std::ios::out | std::ios::trunc | std::ios::binary);
    uh::serialization::buffered_serializer serialize(file);

    serialize.write(metadata.size());
    serialize.sync();

    for (const auto& item : metadata)
    {
        serialize_f_meta_data(file, *item);
    }
}

// ---------------------------------------------------------------------

void file::deserialize(uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& out_queue)
{
    io::file file(m_path);
    uh::serialization::sl_deserializer deserialize(file);
    auto count = deserialize.read<unsigned long>();

    while (count--)
    {
        out_queue.append_job(deserialize_f_meta_data(file));
    }
}

// ---------------------------------------------------------------------

std::list<std::unique_ptr<uhv::f_meta_data>> file::deserialize()
{
    io::file file(m_path);
    uh::serialization::sl_deserializer deserialize(file);
    auto count = deserialize.read<unsigned long>();

    std::list<std::unique_ptr<uhv::f_meta_data>> rv;
    while (count--)
    {
        rv.push_back(deserialize_f_meta_data(file));
    }

    return rv;
}

// ---------------------------------------------------------------------

void file::append(std::unique_ptr<uhv::f_meta_data> md)
{
    io::file file(m_path, std::ios::out | std::ios::ate | std::ios::binary);

    uh::serialization::sl_deserializer deserialize(file);
    auto count = deserialize.read<unsigned long>();

    uh::serialization::buffered_serializer serialize(file);
    file.seek(0, std::ios_base::beg);
    serialize.write(count + 1);
    serialize.sync();

    file.seek(0, std::ios_base::end);
    serialize_f_meta_data(file, *md);
}

// ---------------------------------------------------------------------

} // namespace uh::uhv
