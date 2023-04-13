//
// Created by masi on 13.03.23.
//

#ifndef CORE_BUFFERED_DEVICE_H
#define CORE_BUFFERED_DEVICE_H

#include <type_traits>
#include <cstring>

#include "device.h"

namespace uh::io {

    template<typename DeviceType> requires std::is_base_of_v<io::device, DeviceType>
    class buffered_device: public io::device {
        const size_t buffer_size_;
        char *buffer {nullptr};
        unsigned long pointer = 0;
        DeviceType &dev_;
    public:
        explicit buffered_device(DeviceType &dev, std::size_t buffer_size = 1024)
            : buffer_size_ {buffer_size},
              buffer{new char[buffer_size]},
              dev_(dev)
        {}

        std::streamsize write(std::span<const char> data) override {
            if (pointer + data.size() >= buffer_size_) {
                sync();
            }
            if (data.size() <= buffer_size_) {
                std::memcpy(buffer + pointer, data.data(), data.size());
                pointer += data.size();
                return data.size();
            } else {
                return serialization::sync_write(dev_, data);

            }
        }


        std::streamsize read(std::span<char> data) override {
            return serialization::sync_read(dev_, data);
        }

        [[nodiscard]] bool valid() const override {
            return dev_.valid();
        }

        inline void sync() {
            if (pointer > 0) {
                serialization::sync_write(dev_, {buffer, pointer});
                pointer = 0;
            }
        }


        ~buffered_device() override {
            try
            {
                sync();
                delete[] buffer;
                buffer = nullptr;
            }
            catch (const std::exception&)
            {
            }
        }
    };

} // namespace uh::io

#endif //CORE_BUFFERED_DEVICE_H
