//
// Created by masi on 8/24/23.
//

#ifndef CORE_MESSENGER_H
#define CORE_MESSENGER_H

#include <boost/asio/ip/tcp.hpp>


namespace uh::cluster {

class messenger {
public:
    explicit messenger (boost::asio::ip::tcp::socket&& socket): m_socket (std::move (socket)) {

    }

    messenger (messenger&& m) noexcept: m_socket(std::move (m.m_socket)) {

    }

    boost::asio::ip::tcp::socket m_socket;
};

} // end namespace uh::cluster


#endif //CORE_MESSENGER_H
