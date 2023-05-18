//
// Created by benjamin-elias on 18.05.23.
//

#include "multi_devices.h"

namespace uh::io {

// ---------------------------------------------------------------------

template<typename Device>
std::vector<std::vector<char>> io::read_to_buffer(const std::vector<Device &>& dev, std::streamsize chunk_size) {
    std::vector<std::vector<char>> buffers;

    for(const auto&item:dev){
        buffers.push_back(read_to_buffer(item,chunk_size));
    }

    return buffers;
}

// ---------------------------------------------------------------------

template<typename Device>
std::vector<std::size_t> io::write_from_buffer(const std::vector<Device &>& dev,
                                               const std::vector<std::span<char>>& buffer) {
    std::vector<std::size_t> sizes;
    sizes.reserve(std::min(dev.size(),buffer.size()));

    auto buf_beg = std::begin(buffer);

    for(const auto&item:dev){
        if(buf_beg == std::end(buffer))break;
        sizes.push_back(write_from_buffer(item,*buf_beg));
        buf_beg++;
    }

    return sizes;
}

// ---------------------------------------------------------------------

template<typename Device>
std::vector<std::size_t> io::copy(const std::vector<Device &> &dev, const std::vector<std::ostream> &out) {
    std::vector<std::size_t> sizes;
    sizes.reserve(std::min(dev.size(),out.size()));

    auto buf_beg = std::begin(out);

    for(const auto&item:dev){
        if(buf_beg == std::end(out))break;
        sizes.push_back(copy(item,*buf_beg));
        buf_beg++;
    }

    return sizes;
}

// ---------------------------------------------------------------------

template<typename Device1,typename Device2>
std::vector<std::size_t> io::copy(const std::vector<Device1 &> &in, const std::vector<Device2 &> &out) {
    std::vector<std::size_t> sizes;
    sizes.reserve(std::min(in.size(),out.size()));

    auto buf_beg = std::begin(out);

    for(const auto&item:in){
        if(buf_beg == std::end(out))break;
        sizes.push_back(copy(item,*buf_beg));
        buf_beg++;
    }

    return sizes;
}

// ---------------------------------------------------------------------

template<typename Device1,typename Device2>
std::vector<std::size_t> io::copy(const std::vector<Device1 &>& in, Device2 &out) {
    std::vector<std::size_t> sizes;
    sizes.reserve(std::min(in.size(),out.size()));

    for(const auto&item:in){
        sizes.push_back(copy(item,out));
    }

    return sizes;
}

// ---------------------------------------------------------------------

template<typename Device>
std::vector<std::streamsize> io::write(const std::vector<Device &>& dev,
                                       const std::vector<std::span<const char>>& buffer) {
    std::vector<std::streamsize> sizes;
    sizes.reserve(std::min(dev.size(),buffer.size()));

    auto buf_beg = std::begin(buffer);

    for(const auto&item:dev){
        if(buf_beg == std::end(buffer))break;
        sizes.push_back(copy(item,*buf_beg));
        buf_beg++;
    }

    return sizes;
}

// ---------------------------------------------------------------------

template<typename Device>
std::vector<std::streamsize> io::read(const std::vector<Device &>& dev,
                                      const std::vector<std::span<char>>& buffer) {
    std::vector<std::streamsize> sizes;
    sizes.reserve(std::min(dev.size(),buffer.size()));

    auto buf_beg = std::begin(buffer);

    for(const auto&item:dev){
        if(buf_beg == std::end(buffer))break;
        sizes.push_back(copy(item,*buf_beg));
        buf_beg++;
    }

    return sizes;
}

// ---------------------------------------------------------------------

} // namespace uh::io