#ifndef SERIALIZATION_FILE_META_DATA_H
#define SERIALIZATION_FILE_META_DATA_H

#include <iostream>
#include <vector>

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

class file_meta_data
{

    private:
        std::string m_file_path;
        // TODO: a lot of other file meta data
        std::vector<std::string> m_hashes;

};

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

#endif