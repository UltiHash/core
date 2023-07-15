#include "chunk_collection_index_persistent.h"
#include <io/fragment_on_seekable_device.h>
#include <io/buffer.h>

#include <string>

namespace uh::io
{

// ---------------------------------------------------------------------


namespace
{

// ---------------------------------------------------------------------

std::filesystem::path index_path(std::unique_ptr<io::file>& collection_file)
{
    return collection_file->path().replace_extension(".index");
}

// ---------------------------------------------------------------------

std::vector<std::pair<serialization::fragment_serialize_size_format, std::streamoff>>
maybe_index_persist_chunk_collection(std::unique_ptr<io::file>& collection_file, std::unique_ptr<io::file>& index_file)
{
    auto filename_index = index_path(collection_file);
    bool is_index_persisted = std::filesystem::exists(filename_index);

    std::vector<std::pair<serialization::fragment_serialize_size_format, std::streamoff>> output_index;
    std::streamoff collection_offset{};

    if (is_index_persisted)
    {
        index_file = std::make_unique<io::file>(filename_index, std::ios_base::in);
        auto index_file_size = (std::streamoff) index_file->size();

        std::size_t parse_count{};

        while (index_file->valid())
        {
            if (index_file_size == parse_count)
                break;

            serialization::fragment_serialize_size_format index_parse;
            index_parse.deserialize(*index_file);

            parse_count += index_parse.serialized_size();

            output_index.emplace_back(index_parse, collection_offset);
            collection_offset += index_parse.serialized_size() + index_parse.content_size;
        }

        return output_index;
    }

    if (collection_file->size())
    {
        collection_offset = 0;

        collection_file = std::make_unique<io::file>(collection_file->path(),
                                                     std::ios_base::binary | std::ios_base::in);
        auto collection_file_size = (std::streamoff) collection_file->size();

        index_file = std::make_unique<io::file>(filename_index, std::ios_base::binary | std::ios_base::out);

        auto temporarily_cached_fragment_on_seekable_device =
            io::fragment_on_seekable_device(*collection_file);

        uint16_t index_entry_count{};
        serialization::fragment_serialize_size_format skip_format;

        do
        {
            if (collection_file_size == collection_offset)
                break;

            skip_format = temporarily_cached_fragment_on_seekable_device.skip();

            if (index_entry_count > std::numeric_limits<unsigned char>::max() + 1)
            THROW(util::exception,
                  "Indexing of chunk collection " + collection_file->path().string() + " exceeded limits");

            output_index.emplace_back(skip_format, collection_offset);

            auto frag_ser_size_format = skip_format.serialize();
            io::write(*index_file, frag_ser_size_format);

            collection_offset += skip_format.serialized_size() + skip_format.content_size;
            index_entry_count++;
        }
        while (true);
    }

    return output_index;
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

chunk_collection_index_persistent::~chunk_collection_index_persistent()
{
    std::lock_guard lock(m_index_work_mux);

    if (!std::filesystem::exists(m_workfile->path())
        or (std::filesystem::exists(m_index_file->path()) and std::filesystem::is_empty(m_index_file->path())))
        maybe_forget_index_file();
}

// ---------------------------------------------------------------------

chunk_collection_index_persistent::chunk_collection_index_persistent(std::unique_ptr<io::file>& chunk_collection_file)
    :
    m_index_file(std::make_unique<io::file>(index_path(chunk_collection_file),
                                            std::ios_base::binary | std::ios_base::app)),
    std::vector<std::pair<serialization::fragment_serialize_size_format, std::streamoff>>{
        maybe_index_persist_chunk_collection(chunk_collection_file, m_index_file)},
    m_workfile(chunk_collection_file),
    m_index_file_size(m_index_file->size())
{}

// ---------------------------------------------------------------------

std::pair<serialization::fragment_serialize_size_format,
          std::streamoff> chunk_collection_index_persistent::emplace_back_index(serialization::fragment_serialize_size_format write_format,
                                                                                std::size_t emplace_size,
                                                                                bool flush_after_operation)
{
    std::lock_guard lock(m_index_work_mux);

    auto tmp = emplace_back(write_format, emplace_size);

    maybe_recreate_index_file();

    std::ios_base::openmode write_mode = std::ios_base::binary | std::ios_base::app;

    if (not m_index_file->is_open() or m_index_file->mode() != write_mode)
        m_index_file = std::make_unique<io::file>(index_path(m_workfile), write_mode);

    auto frag_ser_size_format = back().first.serialize();
    m_index_file_size += io::write(*m_index_file, frag_ser_size_format);

    if (flush_after_operation)
        m_index_file->close();

    return tmp;
}

// ---------------------------------------------------------------------

std::vector<uint8_t> chunk_collection_index_persistent::get_index_num_content_list(const std::vector<uint8_t>& without)
{
    std::lock_guard lock(m_index_work_mux);

    std::vector<uint8_t> out_list(count() - without.size());

    std::size_t counter{};

    for (const auto& index_item : *this)
    {
        if (std::none_of(without.cbegin(), without.cend(), [&index_item](const uint8_t& without_item)
        {
            return index_item.first.index_num == without_item;
        }))
        {
            out_list[counter] = index_item.first.index_num;
            counter++;
        }
    }

    return out_list;
}

// ---------------------------------------------------------------------

uint16_t chunk_collection_index_persistent::count()
{
    std::lock_guard lock(m_index_work_mux);

    return static_cast<uint16_t>(std::distance(this->cbegin(), this->cend()));
}

// ---------------------------------------------------------------------

std::size_t chunk_collection_index_persistent::size()
{
    std::lock_guard lock(m_index_work_mux);

    std::size_t accumulated{};

    for (const auto& item : *this)
    {
        accumulated += item.first.serialized_size() + item.first.content_size;
    }

    return accumulated;
}

// ---------------------------------------------------------------------

std::size_t chunk_collection_index_persistent::size(uint8_t index_address)
{
    std::lock_guard lock(m_index_work_mux);

    auto found_address = find_address(index_address, this->begin());

    return found_address->first.serialized_size() + found_address->first.content_size;
}

// ---------------------------------------------------------------------

std::size_t chunk_collection_index_persistent::index_file_size()
{
    std::lock_guard lock(m_index_work_mux);

    return m_index_file_size;
}

// ---------------------------------------------------------------------

std::size_t chunk_collection_index_persistent::content_size(uint8_t index_adress)
{
    std::lock_guard lock(m_index_work_mux);

    auto found_address = find_address(index_adress, this->begin());

    return found_address->first.content_size;
}

// ---------------------------------------------------------------------

bool chunk_collection_index_persistent::full()
{
    std::lock_guard lock(m_index_work_mux);

    return count() == std::numeric_limits<unsigned char>::max() + 1;
}

// ---------------------------------------------------------------------

uint16_t chunk_collection_index_persistent::free()
{
    std::lock_guard lock(m_index_work_mux);

    return static_cast<uint16_t>(std::numeric_limits<uint8_t>::max()) + 1 - count();
}

// ---------------------------------------------------------------------

uint8_t chunk_collection_index_persistent::next_free_address()
{
    std::lock_guard lock(m_index_work_mux);

    if (full())
    THROW(util::exception,
          "There are no more free addresses on chunk collection index " + m_index_file->path().string() + " !");

    std::sort(this->begin(), this->end(), [](const auto& a, const auto& b)
    {
        return a.first.index_num < b.first.index_num;
    });

    auto index_beg = std::begin(*this);
    for (unsigned short i = 0; i < std::numeric_limits<unsigned char>::max() + 1; i++)
    {
        if (index_beg == std::end(*this) || index_beg->first.index_num < i)
        {
            return i;
        }
        else
            index_beg++;
    }

    return 0;
}

// ---------------------------------------------------------------------

std::vector<uint8_t> chunk_collection_index_persistent::filtered_at_list_in_seek_order(const std::vector<uint8_t>& at)
{
    std::lock_guard lock(m_index_work_mux);

    std::vector<uint8_t> index_num_list = get_index_num_content_list();
    std::vector<uint8_t> filtered_at_list_in_seek_order;

    for (const auto item : index_num_list)
    {
        if (std::any_of(at.cbegin(), at.cend(), [item](const auto item2)
        {
            return item == item2;
        }))
            filtered_at_list_in_seek_order.push_back(item);
    }

    return filtered_at_list_in_seek_order;
}

// ---------------------------------------------------------------------

std::vector<std::pair<serialization::fragment_serialize_size_format, std::streamoff>>::iterator
chunk_collection_index_persistent::find_address(uint8_t at,
                                                std::vector<std::pair<serialization::fragment_serialize_size_format,
                                                                      std::streamoff>>::iterator start_pos)
{
    std::lock_guard lock(m_index_work_mux);

    auto fragment_pos_element = std::find_if(start_pos, this->end(),
                                             [&at](const std::pair<
                                                 serialization::fragment_serialize_size_format,
                                                 std::streamoff
                                             >& elem)
                                             {
                                                 return elem.first.index_num == at;
                                             });

    if (fragment_pos_element == this->end())
    THROW(util::exception, "Fragment " + std::to_string(at) +
        " was not found on chunk collection " + m_index_file->path().string());

    return fragment_pos_element;
}

// ---------------------------------------------------------------------

void chunk_collection_index_persistent::release_to(const std::filesystem::path& release_path)
{
    if (m_index_file_forgotten or m_index_file->path() == release_path)
    {
        return;
    }

    m_index_file->close();

    if (::link(this->m_index_file->path().c_str(), release_path.c_str()) == -1)
    {
        THROW_FROM_ERRNO();
    }
    m_index_file = std::make_unique<io::file>(release_path, std::ios_base::binary | std::ios_base::app);
    m_index_file->close();
}

// ---------------------------------------------------------------------

void chunk_collection_index_persistent::maybe_forget_index_file()
{
    std::lock_guard lock(m_index_work_mux);

    if (not m_index_file_forgotten)
    {
        if (m_index_file->is_open())
            m_index_file->close();
        std::filesystem::remove(m_index_file->path());
        m_index_file_forgotten = true;
    }
}

// ---------------------------------------------------------------------

void chunk_collection_index_persistent::maybe_recreate_index_file()
{
    std::lock_guard lock(m_index_work_mux);

    if (m_index_file_forgotten)
    {
        auto reindex = maybe_index_persist_chunk_collection(m_workfile, m_index_file);
        assign(reindex.cbegin(), reindex.cend());
        m_index_file = std::make_unique<io::file>(index_path(m_workfile), std::ios_base::binary | std::ios_base::app);
        m_index_file_forgotten = false;
    }
}

// ---------------------------------------------------------------------

} // namespace uh::io