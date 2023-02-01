#ifndef SERIALIZATION_FILE_INFO_H
#define SERIALIZATION_FILE_INFO_H

#include <iostream>
#include <vector>

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

class file_meta_data
{

    private:
        std::string m_file_path;
        std::vector<std::string> m_hashes;

};

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

#endif