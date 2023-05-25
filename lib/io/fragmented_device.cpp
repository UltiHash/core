//
// Created by benjamin-elias on 25.05.23.
//

#include "fragmented_device.h"

#include <ios>

namespace uh::io{

    // ---------------------------------------------------------------------

    namespace
    {

    // ---------------------------------------------------------------------

        constexpr std::streamsize BUFFER_SIZE = 1024 * 1024;

    // ---------------------------------------------------------------------

    } // namespace

    // ---------------------------------------------------------------------

    std::size_t copy(device &in, fragmented_device &out)
    {
        auto buffer_all = io::read_to_buffer(in);

        return out.write({ buffer_all.data(), buffer_all.size() }).content_size;
    }

    // ---------------------------------------------------------------------

    std::size_t copy(fragmented_device &in, device &out)
    {
        std::array<char, BUFFER_SIZE> buffer;
        std::size_t rv = 0;

        while (in.valid())
        {
            std::size_t read = in.read(buffer).content_size;
            rv += read;
            if (!read)
            {
                break;
            }
            out.write({ buffer.data(), read });
        }

        return rv;
    }

    // ---------------------------------------------------------------------

    std::size_t copy(fragmented_device &in, fragmented_device &out)
    {
        std::array<char, BUFFER_SIZE> buffer;
        std::size_t rv = 0;

        while (in.valid())
        {
            std::size_t read = in.read(buffer).content_size;
            rv += read;
            if (!read)
            {
                break;
            }
            out.write({ buffer.data(), read });
        }

        return rv;
    }

    // ---------------------------------------------------------------------

    serialization::fragment_serialize_size_format write
    (fragmented_device &dev, std::span<const char> buffer, uint32_t alloc)
    {
        auto out_frag_size = serialization::fragment_serialize_size_format(0,0,0);

        while (out_frag_size.content_size < alloc)
        {
            auto write_frag_size =
                    dev.write({buffer.data(), buffer.size()},alloc);

            out_frag_size.header_size = std::max(out_frag_size.header_size,write_frag_size.header_size);
            out_frag_size.content_size += write_frag_size.content_size;
            out_frag_size.index_num = write_frag_size.index_num;
        }

        return out_frag_size;
    }

    // ---------------------------------------------------------------------

    serialization::fragment_serialize_size_format write
            (fragmented_device &dev, std::span<const char> buffer)
    {
        auto out_frag_size = serialization::fragment_serialize_size_format(0,0,0);

        while (out_frag_size.content_size < buffer.size())
        {
            auto write_frag_size =
                    dev.write({buffer.data(), buffer.size()});

            out_frag_size.header_size = std::max(out_frag_size.header_size,write_frag_size.header_size);
            out_frag_size.content_size += write_frag_size.content_size;
            out_frag_size.index_num = write_frag_size.index_num;
        }

        return out_frag_size;
    }

    // ---------------------------------------------------------------------

    serialization::fragment_serialize_size_format read(fragmented_device &dev, std::span<char> buffer)
    {
        auto out_frag_size = serialization::fragment_serialize_size_format(0,0,0);

        const auto total = buffer.size();
        while (out_frag_size.content_size < total)
        {
            auto read_frag_size =
                    dev.read({buffer.data() + out_frag_size.content_size,
                              total - out_frag_size.content_size});
            if (read_frag_size.content_size == 0)
            {
                break;
            }

            out_frag_size.header_size = std::max(out_frag_size.header_size,read_frag_size.header_size);
            out_frag_size.content_size += read_frag_size.content_size;
            out_frag_size.index_num = read_frag_size.index_num;
        }

        return out_frag_size;
    }

    // ---------------------------------------------------------------------

} // namespace uh::io