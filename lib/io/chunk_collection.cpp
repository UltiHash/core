#include "chunk_collection.h"

#include "io/file.h"
#include "io/temp_file.h"
#include "io/fragment_on_seekable_device.h"
#include "io/fragment_on_seekable_reset_front_device.h"
#include "serialization/fragment_serialize_size_format.h"

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

#define CHUNK_COLLECTION_CHUNK_SIZE_LIMIT std::numeric_limits<uint32_t>::max()

// ---------------------------------------------------------------------

std::shared_ptr<io::file> maybe_repair_chunk_collection(std::filesystem::path collection_location, bool create_tempfile)
{
    if (create_tempfile)
    {
        io::temp_file file = io::temp_file(collection_location, std::ios_base::binary | std::ios_base::app);
        file.release_to(file.path());
        collection_location = file.path();
    }

    auto collection_file = std::make_shared<io::file>(collection_location, std::ios_base::binary | std::ios_base::app);

    std::filesystem::path corrupted_tempfile_path = collection_file->path().replace_extension(".tmp").string();

    bool temp_file_exists = std::filesystem::exists(corrupted_tempfile_path);

    if (temp_file_exists)
    {
        auto locked_collection_file = collection_file;
        bool file_exists = std::filesystem::exists(locked_collection_file->path());
        bool file_has_content = file_exists and not std::filesystem::is_empty(locked_collection_file->path());

        if (file_exists and not file_has_content)
        {
            locked_collection_file->close();
            std::filesystem::remove(locked_collection_file->path());
            file_exists = false;
        }

        if (file_exists)
        {
            std::filesystem::remove(corrupted_tempfile_path);
            return collection_file;
        }

        std::filesystem::rename(corrupted_tempfile_path, collection_file->path());
        collection_file = std::make_shared<io::file>(locked_collection_file->path(),
                                                     std::ios_base::binary | std::ios_base::app);
    }

    return collection_file;
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

chunk_collection::~chunk_collection()
{
    if (m_workfile->is_open())
        m_workfile->close();

    if (m_behave_like_tempfile or (std::filesystem::exists(getPath()) and std::filesystem::is_empty(getPath())))
        std::filesystem::remove(getPath());
}

// ---------------------------------------------------------------------

chunk_collection::chunk_collection(const std::filesystem::path& collection_temp_directory_else_file_path,
                                   bool create_tempfile)
    :
    m_behave_like_tempfile(create_tempfile),
    m_workfile(maybe_repair_chunk_collection(collection_temp_directory_else_file_path, create_tempfile))
{
    m_index = std::make_unique<chunk_collection_index_persistent>(m_workfile);
}

// ---------------------------------------------------------------------

serialization::fragment_serialize_size_format<>
chunk_collection::write_indexed(std::span<const char> buffer,
                                uint32_t alloc,
                                bool flush_after_operation,
                                int16_t maybe_force_index)
{
    std::lock_guard lock(m_chunk_collection_workmux);

    if (!free())
    THROW(util::exception, "On chunk collection " + m_workfile->path().string() +
        "was no space left to multi write indexed!");

    if (buffer.size() > CHUNK_COLLECTION_CHUNK_SIZE_LIMIT)
    THROW(util::exception, "Incoming writing buffer was too large!");

    if (maybe_force_index < 0)
        maybe_force_index = m_index->next_free_address();
    else
    {
        if (std::any_of(m_index->cbegin(), m_index->cend(), [&maybe_force_index](const auto& index_item)
        {
            return maybe_force_index == index_item.first.index_num;
        }))
        THROW(util::exception, "On chunk collection " + m_workfile->path().string() +
            " forced index position " + std::to_string(maybe_force_index) + " was not available anymore!");

        if (maybe_force_index > std::numeric_limits<uint8_t>::max())
        THROW(util::exception, "On chunk collection " + m_workfile->path().string() +
            " forced index position " + std::to_string(maybe_force_index) + " was out of bounds!");
    }

    maybe_force_mode_flush_reopen(std::ios_base::binary | std::ios_base::app);

    auto temporarily_cached_fragment_on_seekable_device =
        io::fragment_on_seekable_device(*m_workfile, maybe_force_index);

    uint32_t allocate_space = std::max(static_cast<uint32_t>(buffer.size()), alloc);
    serialization::fragment_serialize_size_format written =
        temporarily_cached_fragment_on_seekable_device.write(buffer, allocate_space);

    if (m_index->empty())
        m_index->emplace_back_index(written, 0, flush_after_operation);
    else
        m_index->emplace_back_index(written,
                                    m_index->back().second +
                                        m_index->back().first.serialized_size() +
                                        m_index->back().first.content_size, flush_after_operation);

    if (flush_after_operation)
        m_workfile->close();

    return written;
}

// ---------------------------------------------------------------------

std::pair<std::vector<char>, serialization::fragment_serialize_size_format<>>
chunk_collection::read_indexed(uint8_t at,
                               bool close_after_operation)
{
    std::lock_guard lock(m_chunk_collection_workmux);

    maybe_force_mode_flush_reopen(std::ios_base::binary | std::ios_base::in);
    auto fragment_pos_element = m_index->find_address(at, m_index->begin());

    auto temporarily_cached_fragment_on_seekable_device =
        io::fragment_on_seekable_reset_front_device(*m_workfile,
                                                    fragment_pos_element->first.index_num,
                                                    (std::streamoff) fragment_pos_element->second);
    temporarily_cached_fragment_on_seekable_device.reset();

    serialization::fragment_serialize_size_format read;
    std::vector<char> buffer(CHUNK_COLLECTION_BUFFER_SIZE);
    std::vector<char> output{};

    do
    {
        serialization::fragment_serialize_size_format temp_read =
            temporarily_cached_fragment_on_seekable_device.read(buffer);

        if (at != temp_read.index_num)
        THROW(util::exception,
              "Reading chunk collection was out of bounds on \"" + m_workfile->path().string() + "\" at index position "
                  + std::to_string(read.index_num));

        std::size_t old_size = output.size();

        output.resize(output.size() + temp_read.content_size);
        std::memcpy(output.data() + old_size, buffer.data(), temp_read.content_size);

        read.content_buf_size = std::max(read.content_buf_size, temp_read.content_buf_size);
        read.content_size += temp_read.content_size;
        read.index_num = temp_read.index_num;
    }
    while (temporarily_cached_fragment_on_seekable_device.valid());

    if (close_after_operation)
        m_workfile->close();

    return {output, read};
}

// ---------------------------------------------------------------------

void chunk_collection::remove(const std::vector<uint8_t>& at)
{
    std::lock_guard lock(m_chunk_collection_workmux);

    maybe_force_mode_flush_reopen(std::ios_base::binary | std::ios_base::in);
    chunk_collection cleaned_chunk_collection(getPath().parent_path(), true);
    std::vector<uint8_t> index_list = m_index->get_index_num_content_list(at);

    auto index_list_beg = index_list.begin();

    while (index_list_beg != index_list.end())
    {
        bool is_last = index_list_beg + 1 == index_list.end();
        auto read_from_source_chunk_collection = read_indexed(*index_list_beg, is_last);

        auto current_index_pos = *index_list_beg;

        if (std::none_of(at.cbegin(), at.cend(), [current_index_pos](const uint8_t at_item)
        {
            return current_index_pos == at_item;
        }))
            cleaned_chunk_collection.write_indexed(read_from_source_chunk_collection.first,
                                                   read_from_source_chunk_collection.first.size(),
                                                   is_last, current_index_pos);
        index_list_beg++;
    }

    //TODO: fallback if space of chunk collection could not be allocated -. copy elements one by one and truncate
    //TODO: optimize remove last elements by truncating

    auto work_path = m_workfile->path();
    cleaned_chunk_collection.release_to(m_workfile->path());

    m_index->copy(cleaned_chunk_collection.m_index);
}

// ---------------------------------------------------------------------

std::vector<serialization::fragment_serialize_size_format<>>
chunk_collection::write_indexed_multi(const std::vector<std::span<const char>>& buffer,
                                      bool flush_after_operation)
{
    std::lock_guard lock(m_chunk_collection_workmux);

    if (static_cast<long>(free()) < buffer.size())
    THROW(util::exception, "On chunk collection " + m_workfile->path().string() +
        "was no space left to multi write indexed!");

    std::for_each(buffer.cbegin(), buffer.cend(), [](const auto& item)
    {
        if (item.size() > CHUNK_COLLECTION_CHUNK_SIZE_LIMIT)
        THROW(util::exception, "Incoming writing buffer was too large!");
    });

    std::vector<serialization::fragment_serialize_size_format<>> out_list{};

    std::size_t count_till_end_to_flush{};
    for (const auto& item : buffer)
    {
        count_till_end_to_flush++;
        out_list.push_back(write_indexed(item, 0,
                                         count_till_end_to_flush == buffer.size() and flush_after_operation));
    }

    return out_list;
}

// ---------------------------------------------------------------------

std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format<>>>
chunk_collection::read_indexed_multi(const std::vector<uint8_t>& at)
{
    std::lock_guard lock(m_chunk_collection_workmux);

    for (const auto item : at)
    {
        if (std::count(at.cbegin(), at.cend(), item) > 1)
        {
            THROW(util::exception, "Read indexed multi received dupliate read indexes. This is not allowed!");
        }
    }

    std::vector<uint8_t> filtered_at_list = m_index->filtered_at_list_in_seek_order(at);

    std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format<>>>
        out_list(filtered_at_list.size());

    std::size_t count_operations{};

    for (const auto at_item : filtered_at_list)
    {
        auto [output, read] = read_indexed(at_item, count_operations == filtered_at_list.size());

        std::size_t distance_filtered_projected_to_at =
            std::distance(at.begin(), std::find(at.begin(), at.end(), at_item));

        out_list[distance_filtered_projected_to_at] = std::make_pair(std::move(output), read);
        count_operations++;
    }

    return out_list;
}

// ---------------------------------------------------------------------

uint16_t chunk_collection::count()
{
    std::lock_guard lock(m_chunk_collection_workmux);

    return m_index->count();
}

// ---------------------------------------------------------------------

std::size_t chunk_collection::size()
{
    std::lock_guard lock(m_chunk_collection_workmux);

    return m_index->size();
}

// ---------------------------------------------------------------------

std::size_t chunk_collection::size(uint8_t index_adress)
{
    std::lock_guard lock(m_chunk_collection_workmux);

    return m_index->size(index_adress);
}

// ---------------------------------------------------------------------

std::size_t chunk_collection::content_size(uint8_t index_address)
{
    std::lock_guard lock(m_chunk_collection_workmux);

    return m_index->content_size(index_address);
}

// ---------------------------------------------------------------------

bool chunk_collection::full()
{
    std::lock_guard lock(m_chunk_collection_workmux);

    return m_index->full();
}

// ---------------------------------------------------------------------

uint16_t chunk_collection::free()
{
    std::lock_guard lock(m_chunk_collection_workmux);

    return m_index->free();
}

// ---------------------------------------------------------------------

std::filesystem::path chunk_collection::getPath()
{
    std::lock_guard lock(m_chunk_collection_workmux);

    return m_workfile->path();
}

// ---------------------------------------------------------------------

void chunk_collection::release_to(const std::filesystem::path& release_path)
{
    std::lock_guard lock(m_chunk_collection_workmux);

    m_behave_like_tempfile = false;

    if (release_path == getPath())
        return;

    auto new_index_path = release_path;
    new_index_path.replace_extension(".index");

    if (std::filesystem::exists(new_index_path))
        std::filesystem::remove(new_index_path);

    if (::rename(getPath().c_str(), release_path.c_str()) == -1)
    {
        THROW_FROM_ERRNO();
    }

    m_workfile = std::make_shared<io::file>(release_path, std::ios_base::binary | std::ios_base::in);
    m_index->release_to(new_index_path);
}

// ---------------------------------------------------------------------

std::vector<uint8_t> chunk_collection::get_index_num_content_list()
{
    std::lock_guard lock(m_chunk_collection_workmux);

    return m_index->get_index_num_content_list();
}

// ---------------------------------------------------------------------

void chunk_collection::maybe_forget_chunk_collection_index_file()
{
    std::lock_guard lock(m_chunk_collection_workmux);

    m_index->maybe_forget_index_file();
}

// ---------------------------------------------------------------------

long double chunk_collection::average_chunk_size()
{
    std::lock_guard lock(m_chunk_collection_workmux);

    return m_index->average_chunk_size();
}

// ---------------------------------------------------------------------

void chunk_collection::maybe_force_mode_flush_reopen(std::ios_base::openmode mode)
{
    if (not m_workfile->is_open() or m_workfile->mode() != mode)
        m_workfile = std::make_shared<io::file>(m_workfile->path(), mode);
}


// ---------------------------------------------------------------------

}