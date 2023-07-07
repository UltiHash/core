//
// Created by benjamin-elias on 18.05.23.
//
#include "fragment_on_device.h"

#include <util/exception.h>
#include <serialization/fragment_size_struct.h>
#include <serialization/fragment_serialization.h>
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

    auto return_size_format = frag_serialize.write(buffer, index);
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
        auto return_size_format = frag_serialize.write(buffer, index, alloc);
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

        return {
            0,
            static_cast<uint32_t>(written),
            index
        };
    }
}

// ---------------------------------------------------------------------

uh::serialization::fragment_serialize_size_format
fragment_on_device::read(std::span<char> buffer)
{
    if (state_machine == WRITING_BEGIN)
    THROW(util::exception, "Reading on fragment_on_device corrupted the fragments incomplete writing state!");

    if (state_machine == UNDEFINED_STATE)
    {
        state_machine = READING_BEGIN;
        serialization::fragment_serialize_transit_format header_read_format =
            frag_serialize.get_header_data_size_index();

        elements_left_to_process = header_read_format.content_size;
        control_byte = header_read_format.control_byte;
        index = header_read_format.index;

        header_read_format.content_size = std::min(header_read_format.content_size,
                                                   static_cast<uint32_t>(buffer.size()));

        std::pair<std::vector<char>, serialization::fragment_serialize_size_format> first_read =
            frag_serialize.read<std::vector<char>>(header_read_format);

        std::memcpy(buffer.data(), first_read.first.data(), first_read.first.size());

        elements_left_to_process -= first_read.second.content_size;

        if (elements_left_to_process < 0)
        THROW(util::exception, "Too many elements were read from fragment on device! Allocation was exceeded!");

        if (!elements_left_to_process)
            state_machine = COMPLETE;

        return {
            static_cast<uint8_t>(first_read.second.header_size),
            first_read.second.content_size,
            static_cast<uint8_t>(first_read.second.index_num)
        };
    }
    else
    {
        uint32_t to_read = std::min(
            static_cast<uint32_t>(elements_left_to_process),
            static_cast<uint32_t>(buffer.size())
        );

        serialization::fragment_serialize_transit_format header_read_format(
            0,
            to_read,
            control_byte,
            index);

        std::pair<std::vector<char>, serialization::fragment_serialize_size_format> read_continue =
            frag_serialize.read<std::vector<char>>(header_read_format);

        std::memcpy(buffer.data(), read_continue.first.data(), read_continue.first.size());

        elements_left_to_process -= read_continue.second.content_size;

        if (elements_left_to_process < 0)
        THROW(util::exception, "Too many elements were read from fragment on device! Allocation was exceeded!");

        if (!elements_left_to_process)
            state_machine = COMPLETE;

        return read_continue.second;
    }

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
    return frag_serialize.read<std::vector<char>>().second;
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