//
// Created by ankit on 26.10.22.
//

#ifndef INC_3_NETWORK_COMMUNICATION_NET_QUEUE_TS_H
#define INC_3_NETWORK_COMMUNICATION_NET_QUEUE_TS_H

#include "net_common.h"

/**
 * @brief Namespace for custom networking implementations.
 */
namespace uh::net {
    /**
     * @brief A custom thread-safe and templated double ended queue structure.
     * @tparam T Data type of the element.
     *
     * A thread safe double ended queue structure is necessary since multiple clients/servers will access the queue at the same time. Likewise, it has to be templated as it should be able to store data of any type which is especially helpful when there are custom message structures,
     */
    template<typename T>
    class tsqueue {
    public:
        tsqueue() = default;
        tsqueue(const tsqueue<T> &) = delete;
        virtual ~tsqueue() { clear(); }

        // making the basic operations of the double ended queue thread-safe following the RAII programming technique

        // returns reference to the first element in the queue
        const T& front() {
            std::scoped_lock lock(muxQueue);
            return deqQueue.front();
        }

        // returns reference to the last element in the queue
        const T& back() {
            std::scoped_lock lock(muxQueue);
            return deqQueue.back();
        }

        // adds an item to the end of the queue
        void push_back(const T& item) {
            std::scoped_lock lock(muxQueue);
            deqQueue.emplace_back(std::move(item));
        }

        // returns boolean values for knowing if the queue is populated
        bool empty() {
            std::scoped_lock lock(muxQueue);
            return deqQueue.empty();
        }

        // returns number of items in the queue
        size_t count() {
            std::scoped_lock lock(muxQueue);
            return deqQueue.size();
        }

        // delete all the items in the queue
        void clear() {
            std::scoped_lock lock(muxQueue);
            deqQueue.clear();
        }

        // removing the first item in the queue
        T pop_front() {
            std::scoped_lock lock(muxQueue);
            auto t = std::move(deqQueue.front());
            deqQueue.pop_front();
            return t;
        }

        // removing the last item in the queue
        T pop_back() {
            std::scoped_lock lock(muxQueue);
            auto t = std::move(deqQueue.back());
            deqQueue.pop_back();
            return t;
        }
    protected:
        std::mutex muxQueue;
        std::deque<T> deqQueue;
    private:
    };
}

#endif //INC_3_NETWORK_COMMUNICATION_NET_QUEUE_TS_H