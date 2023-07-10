#include "chunk_collection_index_persistent.h"
#include <io/fragment_on_seekable_device.h>

#include <string>

namespace uh::io
{

// ---------------------------------------------------------------------

chunk_collection_index_persistent::chunk_collection_index_persistent(std::unique_ptr<io::file>& chunk_collection_file)
    :
    std::vector<std::pair<serialization::fragment_serialize_size_format, std::streamoff>>(
        maybe_index_persist_chunk_collection(chunk_collection_file)),
    m_index_file(index_path(chunk_collection_file), std::ios_base::app),
    m_workfile(chunk_collection_file)
{}

// ---------------------------------------------------------------------

void chunk_collection_index_persistent::erase_index_items(const std::vector<uint8_t>& at)
{
    if(forgotten){
        maybe_index_persist_chunk_collection(m_workfile);
        m_index_file = io::file(index_path(m_workfile), std::ios_base::app);
        forgotten = false;
    }
    else{
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
                index_pair.second -= (remove_size_struct.header_size + remove_size_struct.content_size);
            });

            delete_pos_list.push_back(to_remove_it->first.index_num);
        }

        io::temp_file erase_tmp(m_index_file.path().parent_path(), std::ios_base::out);
        auto beg = begin();

        std::string read_buffer;
        read_buffer.resize(sizeof(serialization::fragment_serialize_size_format));

        while (io::read(m_index_file, read_buffer))
        {
            if (std::none_of(delete_pos_list.cbegin(), delete_pos_list.cend(), [&beg](auto item)
            {
                return beg->first.index_num == item;
            }))
                io::write(erase_tmp, read_buffer);
            beg++;
        }

        erase_tmp.close();
        m_index_file.close();
        erase_tmp.release_to(erase_tmp.path());
        erase_tmp.rename(m_index_file.path());

        std::erase_if(*this,[&delete_pos_list](auto& item){
            return std::any_of(delete_pos_list.begin(),delete_pos_list.end(),[&item](const auto item2){
                return item.first.index_num == item2;
            });
        });

        m_index_file = io::file(m_index_file.path(), std::ios_base::app);
    }
}

// ---------------------------------------------------------------------

std::vector<uint8_t> chunk_collection_index_persistent::get_index_num_content_list()
{
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

uint16_t chunk_collection_index_persistent::count() const
{
    return static_cast<uint16_t>(std::distance(this->cbegin(), this->cend()));
}

// ---------------------------------------------------------------------

std::size_t chunk_collection_index_persistent::size()
{
    std::size_t accumulated{};

    for (const auto& item : *this)
    {
        accumulated += item.first.header_size + item.first.content_size;
    }

    return accumulated;
}

// ---------------------------------------------------------------------

std::size_t chunk_collection_index_persistent::size(uint8_t index_address)
{
    auto found_address = find_address(index_address, this->begin());

    return found_address->first.header_size + found_address->first.content_size;
}

// ---------------------------------------------------------------------

std::size_t chunk_collection_index_persistent::index_file_size()
{
    return m_index_file.size();
}

// ---------------------------------------------------------------------

std::size_t chunk_collection_index_persistent::content_size(uint8_t index_adress)
{
    auto found_address = find_address(index_adress, this->begin());

    return found_address->first.content_size;
}

// ---------------------------------------------------------------------

bool chunk_collection_index_persistent::full() const
{
    return count() == std::numeric_limits<unsigned char>::max() + 1;
}

// ---------------------------------------------------------------------

uint16_t chunk_collection_index_persistent::free() const
{
    return static_cast<uint16_t>(std::numeric_limits<uint8_t>::max()) + 1 - count();
}

// ---------------------------------------------------------------------

std::filesystem::path chunk_collection_index_persistent::getPath()
{
    return m_index_file.path();
}

// ---------------------------------------------------------------------

uint8_t chunk_collection_index_persistent::next_free_address()
{
    if (full())
    THROW(util::exception,
          "There are no more free addresses on chunk collection index " + m_index_file.path().string() + " !");

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
        " was not found on chunk collection " + m_index_file.path().string());

    return fragment_pos_element;
}

// ---------------------------------------------------------------------

void chunk_collection_index_persistent::forget()
{
    m_index_file.close();
    std::filesystem::remove(m_index_file.path());
    forgotten = true;
}

// ---------------------------------------------------------------------

} // namespace uh::io