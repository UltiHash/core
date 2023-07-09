#include "chunk_collection.h"

#include "io/file.h"
#include "io/temp_file.h"
#include "io/fragment_on_seekable_device.h"
#include "serialization/fragment_size_struct.h"

#include <utility>
#include <filesystem>
#include <algorithm>
#include <numeric>
#include <vector>
#include <span>

namespace uh::io
{

// ---------------------------------------------------------------------

namespace
{

// ---------------------------------------------------------------------

#define TREE_STORAGE_CHUNK_LIMIT std::numeric_limits<uint32_t>::max()

// ---------------------------------------------------------------------

std::unique_ptr<io::file> create_chunk_collection_file(std::filesystem::path& collection_location, bool create_tempfile)
{
    if (create_tempfile)
    {
        std::unique_ptr<io::temp_file>
            file = std::make_unique<io::temp_file>(collection_location, std::ios_base::binary | std::ios_base::out);
        file->release_to(file->path());
        collection_location = file->path();
    }

    return std::make_unique<io::file>(io::file(collection_location, std::ios_base::binary | std::ios_base::out));
}

// ---------------------------------------------------------------------

std::unique_ptr<io::file> maybe_repair_chunk_collection(std::unique_ptr<io::file> collection_file)
{
    std::filesystem::path corrupted_tempfile_path = collection_file->path().string() + ".tmp";

    bool temp_file_exists = std::filesystem::exists(corrupted_tempfile_path);

    if (temp_file_exists)
    {
        bool file_exists = std::filesystem::exists(collection_file->path());
        bool file_has_content = file_exists and not std::filesystem::is_empty(collection_file->path());

        if (file_exists and not file_has_content)
        {
            collection_file->close();
            std::filesystem::remove(collection_file->path());
            file_exists = false;
        }

        if (file_exists)
        {
            std::filesystem::remove(corrupted_tempfile_path);
            return collection_file;
        }

        std::filesystem::rename(corrupted_tempfile_path, collection_file->path());
        collection_file = std::make_unique<io::file>(io::file(collection_file->path(),
                                                              std::ios_base::binary | std::ios_base::out));
    }

    return collection_file;
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

chunk_collection::~chunk_collection()
{
    m_workfile->close();

    if (m_behave_like_tempfile or (std::filesystem::exists(getPath()) and std::filesystem::is_empty(getPath())))
        std::filesystem::remove(getPath());
}

// ---------------------------------------------------------------------

chunk_collection::chunk_collection(std::filesystem::path collection_temp_directory_else_file_path, bool create_tempfile)
    :
    m_behave_like_tempfile(create_tempfile),
    m_workfile(maybe_repair_chunk_collection(
        create_chunk_collection_file(collection_temp_directory_else_file_path, create_tempfile))
    ),
    m_index(m_workfile)
{}

// ---------------------------------------------------------------------

serialization::fragment_serialize_size_format
chunk_collection::write_indexed(std::span<const char> buffer, uint32_t alloc)
{
    std::lock_guard lock(m_readmux);

    if (!free())
    THROW(util::exception, "On chunk collection " + m_workfile->path().string() +
        "was no space left to multi write indexed!");

    if (buffer.size() > TREE_STORAGE_CHUNK_LIMIT)
    THROW(util::exception, "Incoming writing buffer was too large!");

    const auto write_mode = std::ios_base::binary | std::ios_base::app;

    if (m_workfile->mode() != m_workfile->size() ? write_mode : std::ios_base::binary | std::ios_base::out)
        m_workfile = std::make_unique<io::file>(m_workfile->path(), write_mode);

    auto temporarily_cached_fragment_on_seekable_device =
        io::fragment_on_seekable_device(*m_workfile,
                                        m_index.next_free_address());

    uint32_t allocate_space = std::max(static_cast<uint32_t>(buffer.size()), alloc);
    serialization::fragment_serialize_size_format written =
        temporarily_cached_fragment_on_seekable_device.write(buffer, allocate_space);

    if (m_index.empty())
        m_index.emplace_back_index(written, 0);
    else
        m_index.emplace_back_index(written,
                                   m_index.back().second +
                                       m_index.back().first.header_size +
                                       m_index.back().first.content_size);

    return written;
}

// ---------------------------------------------------------------------

std::pair<std::vector<char>, serialization::fragment_serialize_size_format>
chunk_collection::read_indexed(uint8_t at)
{
    std::lock_guard lock(m_readmux);

    const auto read_mode = std::ios_base::binary | std::ios_base::in;

    if (m_workfile->mode() != read_mode)
        m_workfile = std::make_unique<io::file>(m_workfile->path(), read_mode);

    auto fragment_pos_element = m_index.find_address(at, m_index.begin());

    auto temporarily_cached_fragment_on_seekable_device =
        io::fragment_on_seekable_device(*m_workfile);

    m_workfile->seek((*fragment_pos_element).second, std::ios_base::beg);

    serialization::fragment_serialize_size_format read;
    std::vector<char> buffer(CHUNK_COLLECTION_BUFFER_SIZE);
    std::vector<char> output{};

    do
    {
        serialization::fragment_serialize_size_format temp_read =
            temporarily_cached_fragment_on_seekable_device.read(buffer);

        std::size_t old_size = output.size();

        output.resize(output.size() + temp_read.content_size);
        std::memcpy(output.data() + old_size, buffer.data(), temp_read.content_size);

        read.header_size = std::max(read.header_size, temp_read.header_size);
        read.content_size += temp_read.content_size;
        read.index_num = read.index_num;

    }
    while (temporarily_cached_fragment_on_seekable_device.valid());

    return {output, read};
}

// ---------------------------------------------------------------------

void chunk_collection::remove(const std::vector<uint8_t>& at)
{
    std::lock_guard lock(m_readmux);

    const auto read_mode = std::ios_base::binary | std::ios_base::in;
    m_workfile = std::make_unique<io::file>(m_workfile->path(), read_mode);

    {
        io::temp_file cc_tmp(getPath().parent_path());
        {
            for (const auto item : m_index.get_index_num_content_list())
            {
                if (std::none_of(at.cbegin(), at.cend(), [&item](const auto& item2)
                {
                    return item == item2;
                }))
                {
                    fragment_on_seekable_device reader(*m_workfile);
                    auto read_from_this = read_indexed(item);

                    fragment_on_seekable_device writer(cc_tmp, reader.getIndex());
                    writer.write(read_from_this.first);
                }
            }
        }

        m_workfile->close();
        cc_tmp.close();
        cc_tmp.rename(m_workfile->path());
    }

    m_workfile = std::make_unique<io::file>(m_workfile->path(), read_mode);

    m_index.erase_index_items(at);
}

// ---------------------------------------------------------------------

std::vector<serialization::fragment_serialize_size_format>
chunk_collection::write_indexed_multi(const std::vector<std::span<const char>>& buffer)
{
    std::lock_guard lock(m_readmux);

    if (static_cast<long>(free()) - buffer.size() < 0)
    THROW(util::exception, "On chunk collection " + m_workfile->path().string() +
        "was no space left to multi write indexed!");

    std::for_each(buffer.cbegin(), buffer.cend(), [](const auto& item)
    {
        if (item.size() > TREE_STORAGE_CHUNK_LIMIT)
        THROW(util::exception, "Incoming writing buffer was too large!");
    });

    std::vector<serialization::fragment_serialize_size_format> out_list{};

    for (const auto& item : buffer)
        out_list.push_back(write_indexed(item));

    return out_list;
}

// ---------------------------------------------------------------------

std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format>>
chunk_collection::read_indexed_multi(const std::vector<uint8_t>& at)
{
    std::lock_guard lock(m_readmux);

    for (const auto item : at)
    {
        if (std::count(at.cbegin(), at.cend(), item) > 1)
        {
            THROW(util::exception, "Read indexed multi received dupliate read indexes. This is not allowed!");
        }
    }

    std::vector<uint8_t> filtered_at_list = m_index.filtered_at_list_in_seek_order(at);

    std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format>>
        out_list(filtered_at_list.size());

    std::vector<char> buffer(CHUNK_COLLECTION_BUFFER_SIZE);

    for (const auto at_item : filtered_at_list)
    {
        auto [output, read] = read_indexed(at_item);

        std::streamoff distance_filtered_projected_to_at =
            std::distance(at.begin(), std::find(at.begin(), at.end(), at_item));

        out_list[distance_filtered_projected_to_at] = std::move(std::make_pair(std::move(output), read));
    }

    return out_list;
}

// ---------------------------------------------------------------------

uint16_t chunk_collection::count()
{
    std::lock_guard lock(m_readmux);

    return m_index.count();
}

// ---------------------------------------------------------------------

std::size_t chunk_collection::size()
{
    std::lock_guard lock(m_readmux);

    return m_index.size();
}

// ---------------------------------------------------------------------

std::size_t chunk_collection::size(uint8_t index_adress)
{
    std::lock_guard lock(m_readmux);

    return m_index.size(index_adress);
}

// ---------------------------------------------------------------------

std::size_t chunk_collection::content_size(uint8_t index_address)
{
    std::lock_guard lock(m_readmux);

    return m_index.content_size(index_address);
}

// ---------------------------------------------------------------------

bool chunk_collection::full() const
{
    return m_index.full();
}

// ---------------------------------------------------------------------

uint16_t chunk_collection::free()
{
    std::lock_guard lock(m_readmux);

    return m_index.free();
}

// ---------------------------------------------------------------------

std::filesystem::path chunk_collection::getPath()
{
    std::lock_guard lock(m_readmux);

    return m_workfile->path();
}

// ---------------------------------------------------------------------

void chunk_collection::release_to(const std::filesystem::path& release_path)
{
    std::lock_guard lock(m_readmux);

    m_behave_like_tempfile = false;

    if (release_path == getPath())
        return;

    m_workfile->close();

    if (::link(getPath().c_str(), release_path.c_str()) == -1)
    {
        THROW_FROM_ERRNO();
    }

    m_workfile = std::make_unique<io::file>(release_path, std::ios_base::binary | std::ios_base::in);
}

// ---------------------------------------------------------------------

}