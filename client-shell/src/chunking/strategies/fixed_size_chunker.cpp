#include <fstream>
#include "fixed_size_chunker.h"

namespace uh::client::chunking {

// ---------------------------------------------------------------------

void fixed_size_chunker::start()
{
    INFO << "       ------ Chunking strategy ------ ";
    INFO << "        backend type   : " << chunking_type();
    INFO << "        chunk size     : " << chunk_size();
}

// ---------------------------------------------------------------------

std::vector<uh::protocol::blob> fixed_size_chunker::chunk_files(std::unique_ptr<common::f_meta_data>& f_meta_data)
{
        std::vector<uh::protocol::blob> chunks;
        std::ifstream input_file(f_meta_data->f_path(),
                                 std::ios::in | std::ios::binary);

        if (!input_file.is_open())
            throw std::ios_base::failure("Error: Could not open file "
            + f_meta_data->f_path().string() + "\n");

        if (input_file.peek() != std::ifstream::traits_type::eof())
        {
            std::uint64_t buf_size = chunk_size();
            uh::protocol::blob tmp_buffer (std::min<std::uint64_t>(f_meta_data->f_size(), buf_size));
            unsigned long total = 0;

            while (input_file)
            {
                const auto remaining_size = f_meta_data->f_size() - total;
                if (remaining_size < tmp_buffer.size())
                    tmp_buffer.resize(std::min<std::size_t>(remaining_size, buf_size));

                input_file.read((tmp_buffer.data()),
                                    static_cast<std::streamsize>(tmp_buffer.size())
                                    );
                std::streamsize bytes_read = input_file.gcount();

                if (input_file.bad() || input_file.fail())
                {
                    throw std::ios_base::failure("Error reading from file");
                }
                else if (!bytes_read)
                {
                    break;
                }

                total += bytes_read;
                chunks.push_back(tmp_buffer);
            }
        }
        input_file.close();
        return chunks;
}

// ---------------------------------------------------------------------

} // namespace uh::client::chunking
