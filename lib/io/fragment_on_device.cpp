#include "fragment_on_device.h"

#include <util/exception.h>
#include <serialization/fragment_size_struct.h>
#include <io/device.h>

#include <span>

namespace uh::io
{

// ---------------------------------------------------------------------

fragment_on_device::fragment_on_device(io::device& dev, uint8_t index)
    :
    dev_fragment(dev), frag_serialize(dev), index(index)
{}

// ---------------------------------------------------------------------

uh::serialization::fragment_serialize_size_format
fragment_on_device::write(std::span<const char> buffer)
{
    if (state_machine == READING_BEGIN)
    THROW(util::exception, "Writing on fragment_on_device corrupted the fragments incomplete reading state!");

    auto return_size_format = uh::serialization::fragment_serialize_size_format(index,buffer.size());
    state_machine = COMPLETE;
    return return_size_format;
}

// ---------------------------------------------------------------------

uh::serialization::fragment_serialize_size_format
fragment_on_device::write(std::span<const char> buffer, uint32_t alloc)
{
    if (state_machine == READING_BEGIN)
    THROW(util::exception, "Writing on fragment_on_device corrupted the fragments incomplete reading state!");

    if (state_machine == UNDEFINED_STATE)
    {
        state_machine = WRITING_BEGIN;
        auto return_size_format = uh::serialization::fragment_serialize_size_format(index,
                                                                                    std::max((uint64_t) alloc,
                                                                                             buffer.size()));
        elements_left_to_process = static_cast<int64_t>(alloc) - return_size_format.content_size;

        if (elements_left_to_process < 0)
        THROW(util::exception, "Too many elements were written to fragment on device! Allocation was exceeded!");

        if (!elements_left_to_process)
            state_machine = COMPLETE;

        return return_size_format;
    }
    else
    {
        std::size_t left_to_write = std::min(static_cast<std::size_t>(elements_left_to_process),
                                             static_cast<std::size_t>(buffer.size()));
        std::streamsize written = dev_fragment.write({buffer.data(), left_to_write});
        elements_left_to_process -= written;

        if (elements_left_to_process < 0)
        THROW(util::exception, "Too many elements were written to fragment on device! Allocation was exceeded!");

        if (!elements_left_to_process)
            state_machine = COMPLETE;

        return {index, static_cast<uint32_t>(written)};
    }
}

// ---------------------------------------------------------------------

uh::serialization::fragment_serialize_size_format
fragment_on_device::read(std::span<char> buffer)
{
    if (state_machine == WRITING_BEGIN)
    THROW(util::exception, "Reading on fragment_on_device corrupted the fragments incomplete writing state!");

    uh::serialization::fragment_serialize_size_format header_read_format;
    header_read_format.deserialize(dev_fragment);

    if (state_machine == UNDEFINED_STATE)
    {
        state_machine = READING_BEGIN;

        elements_left_to_process = header_read_format.content_size;
        index = header_read_format.index_num;

        header_read_format.content_size = std::min(header_read_format.content_size,
                                                   static_cast<uint32_t>(buffer.size()));


    }
    else
    {
        header_read_format.content_size = std::min((uint32_t)elements_left_to_process,
                                                   static_cast<uint32_t>(buffer.size()));

        header_read_format.content_buf_size = 0;
    }

    io::read(dev_fragment,std::span{buffer.begin(),buffer.begin()+header_read_format.content_size});

    elements_left_to_process -= header_read_format.content_size;

    if (elements_left_to_process < 0)
    THROW(util::exception, "Too many elements were read from fragment on device! Allocation was exceeded!");

    if (!elements_left_to_process)
        state_machine = COMPLETE;

    return header_read_format;

}

// ---------------------------------------------------------------------

bool fragment_on_device::valid() const
{
    return dev_fragment.valid() and state_machine != COMPLETE;
}

// ---------------------------------------------------------------------

uh::serialization::fragment_serialize_size_format
fragment_on_device::skip()
{
    uh::serialization::fragment_serialize_size_format read_over;
    read_over.deserialize(dev_fragment);

    std::vector<char> unused_buffer(read_over.content_size);
    io::read(dev_fragment,unused_buffer);

    return read_over;
}

// ---------------------------------------------------------------------

uint8_t fragment_on_device::getIndex() const
{
    return index;
}

// ---------------------------------------------------------------------

bool fragment_on_device::reset()
{
    state_machine = UNDEFINED_STATE;
    return valid();
}

// ---------------------------------------------------------------------

} // namespace uh::io