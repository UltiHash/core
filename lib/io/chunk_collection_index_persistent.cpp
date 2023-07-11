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
maybe_index_persist_chunk_collection(std::unique_ptr<io::file>& collection_file)
{
    auto filename_index = index_path(collection_file);
    bool is_index_persisted = std::filesystem::exists(filename_index);

    std::vector<std::pair<serialization::fragment_serialize_size_format, std::streamoff>> output_index;
    std::streamoff collection_offset{};

    if (is_index_persisted)
    {
        io::file read_index_file(filename_index, std::ios_base::in);

        std::string read_buffer;
        read_buffer.resize(sizeof(serialization::fragment_serialize_size_format));

        while (io::read(read_index_file, read_buffer))
        {
            serialization::fragment_serialize_size_format skip_format;

            io::buffer read_buffer_device;
            io::write_from_buffer(read_buffer_device, read_buffer);
            skip_format.deserialize(read_buffer_device);

            output_index.emplace_back(skip_format, collection_offset);
            collection_offset += skip_format.content_buf_size + skip_format.content_size;
        }

        if (collection_offset < collection_file->size())
        {
            read_index_file.close();
            std::filesystem::remove(read_index_file.path());
            output_index.clear();
        }
        else
            return output_index;
    }

    if (collection_file->size())
    {
        collection_file = std::make_unique<io::file>(filename_index, std::ios_base::binary | std::ios_base::in);
        collection_file->seek(0, std::ios_base::beg);

        auto temporarily_cached_fragment_on_seekable_device =
            io::fragment_on_seekable_device(*collection_file);

        io::file write_index_file(filename_index, std::ios_base::binary | std::ios_base::out);

        uint16_t index_entry_count{};
        serialization::fragment_serialize_size_format skip_format;

        do
        {
            skip_format = temporarily_cached_fragment_on_seekable_device.skip();

            if (skip_format.content_size == 0)
                break;

            if (index_entry_count > std::numeric_limits<unsigned char>::max() + 1)
            THROW(util::exception,
                  "Indexing of chunk collection " + collection_file->path().string() + " exceeded limits");

            output_index.emplace_back(skip_format, collection_offset);

            auto frag_ser_size_format = skip_format.serialize();
            io::write(write_index_file, frag_ser_size_format);

            collection_offset += skip_format.content_buf_size + skip_format.content_size;
            index_entry_count++;
        }
        while (skip_format.content_size > 0);
    }

    return output_index;
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

chunk_collection_index_persistent::chunk_collection_index_persistent(std::unique_ptr<io::file>& chunk_collection_file)
    :
    std::vector<std::pair<serialization::fragment_serialize_size_format, std::streamoff>>(
        maybe_index_persist_chunk_collection(chunk_collection_file)),
    m_index_file(std::make_unique<io::file>(index_path(chunk_collection_file),
                                            std::ios_base::binary | std::ios_base::app)),
    m_workfile(chunk_collection_file),
    m_index_file_size(m_index_file->size())
{}

// ---------------------------------------------------------------------

void chunk_collection_index_persistent::erase_index_items(const std::vector<uint8_t>& at)
{
    maybe_recreate_index_file();

    std::vector<uint16_t> delete_pos_list = update_offset_calculate_delete_pos_list(at);

    remove_persistent_index_file_items(delete_pos_list);

    m_index_file_size -= update_erase_size(delete_pos_list);
}

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

std::vector<uint8_t> chunk_collection_index_persistent::get_index_num_content_list()
{
    std::lock_guard lock(m_index_work_mux);

    std::vector<uint8_t> out_list(count());

    std::size_t counter{};

    for (const auto& item : *this)
    {
        out_list[counter] = item.first.index_num;
        counter++;
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

void chunk_collection_index_persistent::maybe_forget_index_file()
{
    std::lock_guard lock(m_index_work_mux);

    if (not m_index_file_forgotten)
    {
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
        maybe_index_persist_chunk_collection(m_workfile);
        m_index_file = std::make_unique<io::file>(index_path(m_workfile), std::ios_base::binary | std::ios_base::app);
        m_index_file_forgotten = false;
    }
}

// ---------------------------------------------------------------------

std::vector<uint16_t> chunk_collection_index_persistent::update_offset_calculate_delete_pos_list(const std::vector<
    uint8_t>& at)
{
    std::lock_guard lock(m_index_work_mux);

    auto seek_order_at_list = filtered_at_list_in_seek_order(at);
    auto to_remove_it = begin();

    std::vector<uint16_t> delete_pos_list;

    for (const auto at_item : seek_order_at_list)
    {
        to_remove_it = find_address(at_item, to_remove_it);
        auto remove_size_struct = to_remove_it->first;

        std::for_each(to_remove_it, end(), [&remove_size_struct](
            std::pair<serialization::fragment_serialize_size_format, std::streamoff>& index_pair)
        {
            index_pair.second -= (remove_size_struct.content_buf_size + remove_size_struct.content_size);
        });

        delete_pos_list.push_back(to_remove_it->first.index_num);
    }

    return delete_pos_list;
}

// ---------------------------------------------------------------------

void chunk_collection_index_persistent::remove_persistent_index_file_items(const std::vector<uint16_t>& delete_pos_list)
{
    std::lock_guard lock(m_index_work_mux);

    io::temp_file erase_tmp(m_index_file->path().parent_path(), std::ios_base::binary | std::ios_base::out);
    auto beg = begin();

    std::string read_buffer;
    read_buffer.resize(sizeof(serialization::fragment_serialize_size_format));

    m_index_file = std::make_unique<io::file>(index_path(m_workfile), std::ios_base::binary | std::ios_base::in);

    while (io::read(*m_index_file, read_buffer))
    {
        if (std::none_of(delete_pos_list.cbegin(), delete_pos_list.cend(), [&beg](auto item)
        {
            return beg->first.index_num == item;
        }))
            io::write(erase_tmp, read_buffer);
        beg++;
    }

    erase_tmp.close();
    m_index_file->close();
    erase_tmp.release_to(erase_tmp.path());
    erase_tmp.rename(m_index_file->path());
}

std::size_t chunk_collection_index_persistent::update_erase_size(const std::vector<uint16_t>& delete_pos_list)
{
    std::lock_guard lock(m_index_work_mux);

    std::size_t deleted_index_file_size{};

    std::erase_if(*this,
                  [&delete_pos_list, &deleted_index_file_size](std::pair<serialization::fragment_serialize_size_format,
                                                                         std::streamoff>& item)
                  {
                      bool to_be_deleted = std::any_of(delete_pos_list.begin(),
                                                       delete_pos_list.end(),
                                                       [&item](const auto item2)
                                                       {
                                                           return item.first.index_num == item2;
                                                       });

                      if (to_be_deleted)
                      {
                          deleted_index_file_size += item.first.content_buf_size + item.first.content_size;
                      }

                      return to_be_deleted;
                  });

    return deleted_index_file_size;
}

// ---------------------------------------------------------------------

} // namespace uh::io