//
// Created by ankit on 26.10.22.
//

#ifndef INC_3_NETWORK_COMMUNICATION_NET_MESSAGE_H
#define INC_3_NETWORK_COMMUNICATION_NET_MESSAGE_H

#include <cstring>
#include "net_common.h"

/**
 * @brief Namespace for custom networking implementations.
 */
namespace uh::net {

    /**
     * @brief A struct representing the message header which appears at the start of all messages.
     * @tparam T Data type of the element.
     *
     *
     * It is used to identify the type of a message as well the size in bytes of an entire message including the header and the body.
     */
    template <typename T>
    struct message_header
    {
        T id{};
        uint32_t size = 0;
    };

    /**
     * @brief Custom message structure encapsulating the message header and a message body.
     * @tparam T Data type of the element.
     *
     * A whole message struct contains a header as well as a body which is represented as a vector of bytes. It also contains custom operator overloading for better programming experience.
     */
    template <typename T>
    struct message {
        message_header<T> header{};
        std::vector<uint8_t> body{};

        // size of the entire message packets in bytes
        [[nodiscard]] size_t size() const {
            return body.size();
        }

        // overloading the << operator for easily outputting the message struct
        friend std::ostream& operator << (std::ostream& os, const message<T>& msg) {
            os << "ID:" << int(msg.header.id) << "Size:" << msg.header.size();
            return os;
        }

        // pushing the messages using << operator
        template<typename DataType>
        friend message<T>& operator << (message<T>& msg, const DataType& data) {
            // check if the data type being pushed is trivially copyable
            static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");
            size_t i = msg.body.size();
            msg.body.resize(msg.body.size() + sizeof(DataType));
            std::memcpy(msg.body.data() + i, &data, sizeof(DataType));
            msg.header.size = msg.size();
            // for chaining the target messages
            return msg;
        }

        //getting the elements out of the body vector using the >> operator
        template<typename DataType>
        friend message<T>& operator >> (message<T>& msg, DataType& data) {
            // handle overflow and underflow
            // check if the data type being pushed is trivially copyable
            static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pulled from vector");
            size_t i = msg.body.size() - sizeof(DataType);
            std::memcpy(&data, msg.body.data() + i, sizeof(DataType));
            msg.body.resize(i);
            msg.header.size = msg.size();
            return msg;
        }
    };

    /**
     * @brief A struct encapsulating a message and a connection object for identifying the message origin.
     * @tparam T Data type of an element.
     *
     * Multiple messages stored in queues have different origins. This is especially true for a server holding only one queue of messages from multiple clients. Thus, to respond to the correct client, owned_message struct is necessary which holds the connection to respond to.
     */

    template<typename T>
    class connection;

    template<typename T>
    struct owned_message
    {
        std::shared_ptr<connection<T>> remote = nullptr;
        message<T> msg;

        // overloading the << operator for easily outputting the message struct
        friend std::ostream& operator << (std::ostream& os, const owned_message<T>& msg) {
            os << msg.msg;
            return os;
        }
    };
}

#endif //INC_3_NETWORK_COMMUNICATION_NET_MESSAGE_H