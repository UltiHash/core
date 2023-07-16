//
// Created by benjamin-elias on 15.05.23.
//

#include "io/fragmented_device.h"
#include "io/device.h"
#include "serialization/fragment_size_struct.h"
#include "serialization/index_fragment_serialization.h"

#include <cstdint>
#include <span>

#ifndef CORE_FRAGMENT_H
#define CORE_FRAGMENT_H

namespace uh::io
{

class fragment_on_device: public fragmented_device
{

public:
    /**
     * A fragment_on_device uses a device to either only read or only write in its lifetime to a section of a file.
     * Initialize state.
     *
     * @param dev input device
     * @param index index of fragment needs to be set in case fragment is supposed to be written
     */
    explicit fragment_on_device(io::device& dev, uint8_t index = 0);

    /**
     * read un-serialized input and write serialized to device
     *
     * @param buffer input
     * @return struct with header size written, number of bytes that were persisted to device
     * and the index of the fragment
     */

    uh::serialization::fragment_serialize_size_format write(std::span<const char> buffer) override;

    /**
     * read un-serialized input and write serialized to device
     *
     * @param buffer input
     * @param alloc allocate space on fragment, append multiple times to fragment or limit write
     * @return struct with header size written, number of bytes that were persisted to device
     * and the index of the fragment
     */

    uh::serialization::fragment_serialize_size_format write(std::span<const char> buffer,
                                                            uint32_t alloc) override;

    /**
     * read serialized device to un-serialized buffer
     *
     * @param buffer to be read to from device
     * @return number of bytes totally read from device
     */
    uh::serialization::fragment_serialize_size_format read(std::span<char> buffer) override;

    /**
     *
     * WARNING: seekable devices are valid if the stream pointer is not one step beyond the last character,
     * instead of pointing exactly to the last element
     *
     * @return the state of the underlying device and if reading or writing was not completed or started once
     */
    [[nodiscard]] bool valid() const override;

    /**
     * with this function the underlying device is read until
     * one position behind the fragment_on_device content
     *
     * @return struct{header size, content size, index}
     */
    uh::serialization::fragment_serialize_size_format skip() override;

    /**
     * reset the state machine to a new fragment beginning
     * @return if the underlying device is still valid to deliver the next fragment structure
     */
    void reset() override;

    /**
     *
     * @return valid index after at least reading once or writing once
     */
    [[nodiscard]] uint8_t getIndex() const;

private:
    enum
    {
        UNDEFINED_STATE,
        READING_BEGIN,
        WRITING_BEGIN,
        COMPLETE
    } state_machine = UNDEFINED_STATE;
    int64_t elements_left_to_process{};
    io::device& dev_fragment;
    uint8_t index;

protected:
    serialization::index_fragment_serialization<> frag_serialize;
};
} // namespace uh::io

#endif //CORE_FRAGMENT_H
