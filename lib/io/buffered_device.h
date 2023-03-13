//
// Created by masi on 13.03.23.
//

#ifndef CORE_BUFFERED_DEVICE_H
#define CORE_BUFFERED_DEVICE_H

#include <type_traits>
#include <cstring>

#include "device.h"

namespace uh::io {

    template<typename DeviceType, size_t buffer_size = 1024> requires std::is_base_of_v<io::device, DeviceType>
    class buffered_device: public io::device {
        char buffer[buffer_size] {};
        unsigned long pointer = 0;
        DeviceType &dev_;
    public:
        explicit buffered_device(DeviceType &dev) : dev_(dev) {}

        std::streamsize write(std::span<const char> data) override {
            if (pointer + data.size() >= buffer_size) {
                sync();
            }
            if (data.size() <= buffer_size) {
                std::memcpy(buffer + pointer, data.data(), data.size());
                pointer += data.size();
                return data.size();
            } else {
                return dev_.write(data);
            }
        }


        std::streamsize read(std::span<char> data) override {
            return dev_.read(data);
        }

        [[nodiscard]] bool valid() const override {
            return dev_.valid();
        }

        inline void sync() {
            if (pointer > 0) {
                dev_.write({buffer, pointer});
                pointer = 0;
            }
        }


        ~buffered_device() override {
            sync();
        }
    };

} // namespace uh::io

#endif //CORE_BUFFERED_DEVICE_H
