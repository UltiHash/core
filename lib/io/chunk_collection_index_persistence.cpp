#include "chunk_collection_index_persistence.h"
#include <io/fragment_on_seekable_device.h>

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
maybe_index_chunk_collection(std::unique_ptr<io::file>& collection_file)
{
    auto filename_index = index_path(collection_file);
    bool is_index_persisted = std::filesystem::exists(filename_index);

    std::vector<std::pair<serialization::fragment_serialize_size_format, std::streamoff>> output_index;
    std::streamoff collection_offset{};

    if (is_index_persisted)
    {
        io::file read_index_file(filename_index, std::ios_base::binary | std::ios_base::in);

        std::string read_buffer;
        read_buffer.resize(sizeof(serialization::fragment_serialize_size_format));

        while (io::read(read_index_file, read_buffer))
        {
            serialization::fragment_serialize_size_format skip_format;
            std::istringstream ss(read_buffer);

            skip_format.deserialize(ss);
            output_index.emplace_back(skip_format,collection_offset);
            collection_offset += skip_format.header_size + skip_format.content_size;
        }

        if(collection_offset < collection_file->size()){
            read_index_file.close();
            std::filesystem::remove(read_index_file.path());
            output_index.clear();
        }
        else
            return output_index;
    }

    auto temporarily_cached_fragment_on_seekable_device =
        io::fragment_on_seekable_device(*collection_file);

    io::file write_index_file(filename_index, std::ios_base::binary | std::ios_base::out);

    if (collection_file->size())
    {
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

            io::write(write_index_file, skip_format.serialize().str());

            collection_offset += skip_format.header_size + skip_format.content_size;
            index_entry_count++;
        }
        while (skip_format.content_size > 0);
    }

    return output_index;
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

chunk_collection_index_persistence::chunk_collection_index_persistence(std::unique_ptr<io::file>& chunk_collection_file)
    :
    std::vector<std::pair<serialization::fragment_serialize_size_format, std::streamoff>>(
        maybe_index_chunk_collection(chunk_collection_file)),
    index_file(index_path(chunk_collection_file), std::ios_base::app)
{}

// ---------------------------------------------------------------------

} // namespace uh::io