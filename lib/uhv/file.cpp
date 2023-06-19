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

void serialize_meta_data(io::device& device, uh::uhv::meta_data& md)
{
    uh::serialization::buffered_serializer serializer(device);

    serializer.write(md.path().string());
    serializer.write(md.type());
    serializer.write(md.permissions());

    if (md.type() == uhv::uh_file_type::regular) {
        serializer.write(md.size());
        serializer.write(md.hashes());
        serializer.write(md.chunk_sizes());
    }

    serializer.sync();
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::uhv::meta_data> deserialize_meta_data(io::device& device)
{
    uh::serialization::sl_deserializer deserializer(device);
    std::unique_ptr<uh::uhv::meta_data> p_meta_data = std::make_unique<uh::uhv::meta_data>();

    p_meta_data->set_path(deserializer.read <std::string> ());
    p_meta_data->set_type(deserializer.read <std::uint8_t> ());
    p_meta_data->set_permissions(deserializer.read <std::uint32_t> ());

    if (p_meta_data->type() == uhv::uh_file_type::regular)
    {
        p_meta_data->set_size(deserializer.read <std::uint64_t> ());
        p_meta_data->set_hashes(deserializer.read <std::vector <char>> ());
        p_meta_data->add_chunk_sizes(deserializer.read<std::vector <uint32_t>>());
    }

    return p_meta_data;
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

file::file(const std::filesystem::path& path)
    : m_path(path)
{
}

// ---------------------------------------------------------------------

void file::serialize(const std::list<std::unique_ptr<uhv::meta_data>>& metadata)
{
    io::file file (m_path, std::ios::out | std::ios::trunc | std::ios::binary);
    uh::serialization::buffered_serializer serialize(file);

    serialize.write(metadata.size());
    serialize.sync();

    for (const auto& item : metadata)
    {
        serialize_meta_data(file, *item);
    }
}

// ---------------------------------------------------------------------

void file::deserialize(uhv::job_queue<std::unique_ptr<uhv::meta_data>>& out_queue)
{
    io::file file(m_path);
    uh::serialization::sl_deserializer deserialize(file);
    auto count = deserialize.read<unsigned long>();

    while (count--)
    {
        out_queue.append_job(deserialize_meta_data(file));
    }
}

// ---------------------------------------------------------------------

std::list<std::unique_ptr<uhv::meta_data>> file::deserialize()
{
    io::file file(m_path);
    uh::serialization::sl_deserializer deserialize(file);
    auto count = deserialize.read<unsigned long>();

    std::list<std::unique_ptr<uhv::meta_data>> rv;
    while (count--)
    {
        rv.push_back(deserialize_meta_data(file));
    }

    return rv;
}

// ---------------------------------------------------------------------

void file::append(std::unique_ptr<uhv::meta_data> md)
{
    io::file file(m_path, std::ios::out | std::ios::in | std::ios::binary);

    uh::serialization::sl_deserializer deserialize(file);
    auto count = deserialize.read<unsigned long>();

    uh::serialization::buffered_serializer serialize(file);
    file.seek(0, std::ios_base::beg);
    serialize.write(count + 1);
    serialize.sync();

    file.seek(0, std::ios_base::end);
    serialize_meta_data(file, *md);
}

// ---------------------------------------------------------------------

} // namespace uh::uhv
