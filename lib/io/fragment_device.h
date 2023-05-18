//
// Created by benjamin-elias on 15.05.23.
//

#include "io/fragmented_device.h"

#ifndef CORE_FRAGMENT_H
#define CORE_FRAGMENT_H

namespace uh::io{

    enum fragmented_states {
        UNDEFINED_STATE,
        READING_BEGIN,
        READING_COMPLETE
    };

    class fragment_device : public io::fragmented_device{

    public:
        /**
         * a fragment_device uses a device to either only read or only write in its lifetime
         * after reading or writing the fragment_device can tell where to find its content
         * relatively to the device stream.
         * A fragment_device is a serialized device.
         *
         * @param impl input device
         * @param start_pos relative start position on device to distinguish fragments
         */
        explicit fragment_device(io::device& dev);

        /**
         * read un-serialized input and write serialized to device
         *
         * @param buffer input
         * @return number of bytes that were persisted to device
         */

        std::streamsize write(std::span<const char> buffer) override;

        /**
         * read serialized device to un-serialized buffer
         *
         * @param buffer to be read to from device
         * @return number of bytes totally read from device
         */
        std::streamsize read(std::span<char> buffer) override;

        /**
         *
         * @return the state of the underlying device
         */
        [[nodiscard]] bool valid() const override;

        /**
         * with this function the underlying device is read until
         * one position behind the fragment_device content
         *
         * @return counts the entire count a fragment_device fills,
         * together with it's header structure
         */
        std::streamsize skip() override;

    protected:
        [[nodiscard]] fragmented_states getStateMachine() const;

        void setStateMachine(fragmented_states stateMachine);

        [[nodiscard]] std::streamoff getElementsLeftToRead() const;

        void setElementsLeftToRead(std::streamoff elementsLeftToRead);

    private:
        io::device& dev_;
        fragmented_states state_machine = UNDEFINED_STATE;
        std::streamoff elements_left_to_read{};
    };
} // namespace uh::io

#endif //CORE_FRAGMENT_H
