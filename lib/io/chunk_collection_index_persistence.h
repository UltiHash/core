#ifndef CHUNK_COLLECTION_INDEX_PERSISTENCE_H
#define CHUNK_COLLECTION_INDEX_PERSISTENCE_H

#include <serialization/fragment_size_struct.h>
#include <io/file.h>

#include <vector>
#include <ios>
#include <memory>

namespace uh::io
{

// ---------------------------------------------------------------------

class chunk_collection_index_persistence:
    public std::vector<std::pair<serialization::fragment_serialize_size_format, std::streamoff>>
{
public:

    explicit chunk_collection_index_persistence(std::unique_ptr<io::file>& chunk_collection_file);

private:
    io::file index_file;
};

// ---------------------------------------------------------------------

} // namespace uh::io

#endif //CHUNK_COLLECTION_INDEX_PERSISTENCE_H
