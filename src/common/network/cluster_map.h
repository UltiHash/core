//
// Created by masi on 8/22/23.
//

#ifndef CORE_CLUSTER_MAP_H
#define CORE_CLUSTER_MAP_H

#include <boost/bind/bind.hpp>
#include <condition_variable>
#include <boost/asio.hpp>
#include <ranges>
#include <map>
#include <unordered_map>
#include <forward_list>

namespace uh::cluster {
    class cluster_map {

        struct ipv4 {
            uint8_t bytes[4] {};
            explicit ipv4 (const boost::asio::ip::address_v4& address) {

                const auto ip_str = address.to_string();
                auto ip_nums = std::string_view (ip_str)
                           | std::ranges::views::split ('.')
                           | std::ranges::views::transform([](auto && rng) {
                    char* end;
                    return std::strtol (&*rng.begin(), &end, 10);
                });

                int i = 0;
                for (const auto num: ip_nums) {
                    bytes [i++] = num;
                }
            }
            explicit ipv4 (const char* buf) {
                std::memcpy(bytes, buf, sizeof (bytes));
            }

            [[nodiscard]] boost::asio::ip::address_v4 to_address () const {
                return boost::asio::ip::address_v4::from_string(std::to_string(bytes[0]) + "."
                                                            + std::to_string(bytes[1]) + "."
                                                            + std::to_string(bytes[2]) + "."
                                                            + std::to_string(bytes[3]));
            }
        };

    public:


        explicit cluster_map (cluster_config&& cluster_conf):
                m_cluster_conf (std::move (cluster_conf)),
                m_roles_buf ((m_cluster_conf.init_process_count - 1) * PROCESS_DATA_SIZE) {
        }

        cluster_map (cluster_map&& cmap) noexcept:
            m_cluster_conf (cmap.m_cluster_conf),
            m_resp_count (cmap.m_resp_count.load()),
            m_roles_buf (std::move (cmap.m_roles_buf)),
            m_roles (std::move (cmap.m_roles)),
            m_endpoints (std::move(cmap.m_endpoints))
            {
            m_resp_count = 0;
        }

        // only intended to be used for testing
        cluster_map(cluster_config&& config, std::unordered_map <uh::cluster::role, std::map <int, std::string>> roles):
                m_cluster_conf(std::move (config)),
                m_roles_buf ((m_cluster_conf.init_process_count - 1) * PROCESS_DATA_SIZE),
                m_roles(std::move(roles)) {
            m_resp_count = 0;
        }


        void broadcast_init () {
            boost::asio::io_service io;

            boost::asio::ip::udp::socket socket(io);

            socket.open(boost::asio::ip::udp::v4());

            socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
            socket.set_option(boost::asio::socket_base::broadcast(true));

            const auto bc = [this, &socket] (const int port) {

                boost::asio::ip::udp::endpoint sender_endpoint (boost::asio::ip::address_v4::broadcast(), port);
                socket.async_send_to (boost::asio::buffer (INIT_MESSAGE),
                                      sender_endpoint,
                                      boost::bind (&cluster_map::handle_send, this, std::ref (socket)));

            };

            bc (m_cluster_conf.data_node_conf.server_conf.port);
            bc (m_cluster_conf.dedupe_node_conf.server_conf.port);
            bc (m_cluster_conf.directory_node_conf.server_conf.port);
            io.run();

            wait_for_responses();

            for (const auto& endpoint: m_endpoints) {
                socket.send_to(boost::asio::buffer(m_roles_buf.data(), m_roles_buf.size()), endpoint);
            }
        }

        void send_recv_roles (const uh::cluster::role role, const int id) {
            boost::asio::io_service io_service;
            boost::asio::ip::udp::socket socket (io_service,
                            boost::asio::ip::udp::endpoint (boost::asio::ip::udp::v4(), get_port (role)));

            boost::asio::ip::udp::endpoint sender_endpoint;
            char recv_buf [INIT_MESSAGE.size()];
            std::size_t r = socket.receive_from(boost::asio::buffer(recv_buf, INIT_MESSAGE.size()), sender_endpoint);
            if (r != INIT_MESSAGE.size() and std::string_view (recv_buf, INIT_MESSAGE.size()) != INIT_MESSAGE) {
                throw std::runtime_error ("Invalid message");
            }

            auto send_buf = std::make_unique_for_overwrite<char[]> (sizeof (id) + sizeof (role));
            std::memcpy(send_buf.get(), &id, sizeof (id));
            std::memcpy(send_buf.get() + sizeof (id), &role, sizeof (role));
            socket.send_to (boost::asio::buffer(send_buf.get(), sizeof (id) + sizeof (role)), sender_endpoint);

            socket.receive_from(boost::asio::buffer(m_roles_buf.data(), m_roles_buf.size()), sender_endpoint);

            for (int i = 0; i < m_roles_buf.size(); i += PROCESS_DATA_SIZE) {
                const auto pointer = m_roles_buf.data() + i;
                const auto rid = *reinterpret_cast <int*> (pointer);
                const auto rrole = *reinterpret_cast <uh::cluster::role*> (pointer + sizeof (rid));
                const ipv4 ip (pointer + sizeof rid + sizeof rrole);
                m_roles [rrole].emplace(rid, ip.to_address().to_string());
            }
        }

        std::unordered_map <uh::cluster::role, std::map <int, std::string>> m_roles;
        const cluster_config m_cluster_conf;

    private:


        void wait_for_responses () {
            std::unique_lock<std::mutex> lk(m_cv_m);
            m_cv.wait(lk, [this] {return m_resp_count == m_cluster_conf.init_process_count - 1;});
        }

        void handle_send (std::reference_wrapper <boost::asio::basic_datagram_socket<boost::asio::ip::udp>> socket) {

            const auto offset = m_resp_count.fetch_add (1, std::memory_order_relaxed) * PROCESS_DATA_SIZE;
            const auto pointer = m_roles_buf.data() + offset;
            boost::asio::ip::udp::endpoint end_point;
            socket.get().receive_from (boost::asio::buffer(pointer, sizeof (int) + sizeof (role)), end_point);
            std::memcpy (pointer + sizeof (int) + sizeof (role), ipv4 (end_point.address().to_v4()).bytes, sizeof (ipv4));

            const auto id = *reinterpret_cast <int*> (pointer);
            const auto role = *reinterpret_cast <uh::cluster::role*> (pointer + sizeof (id));
            m_roles [role].emplace(id, end_point.address().to_string());
            m_endpoints.emplace_front(std::move (end_point));

        }

        [[nodiscard]] int get_port (const role r) const {
            switch (r) {
                case DEDUPE_NODE:
                    return m_cluster_conf.dedupe_node_conf.server_conf.port;
                case DATA_NODE:
                    return m_cluster_conf.data_node_conf.server_conf.port;
                case DIRECTORY_NODE:
                    return m_cluster_conf.directory_node_conf.server_conf.port;
                default:
                    throw std::invalid_argument ("Invalid role!");
            }
        }

        std::atomic <int> m_resp_count = 0;
        unique_buffer <char> m_roles_buf;
        std::condition_variable m_cv;
        std::mutex m_cv_m;
        std::forward_list <boost::asio::ip::udp::endpoint> m_endpoints;
        constexpr static size_t PROCESS_DATA_SIZE = sizeof (int) + sizeof (role) + sizeof(ipv4);
        constexpr static std::string_view INIT_MESSAGE = "in";

    };
} // end namespace uh::cluster

#endif //CORE_CLUSTER_MAP_H
