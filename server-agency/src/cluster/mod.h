#ifndef SERVER_AGENCY_CLUSTER_MOD_H
#define SERVER_AGENCY_CLUSTER_MOD_H

#include <protocol/client_pool.h>

#include <map>
#include <memory>
#include <string>
#include "routing_interface.h"
#include "protocol/messages.h"


namespace uh::an::cluster
{

// ---------------------------------------------------------------------

/**
 * Type used to store a reference to a remote node. Must be suitable as
 * `Key` for `std::map`.
 */
typedef std::string node_ref;

// ---------------------------------------------------------------------

struct config
{
    enum class connection_method
    {
        plain,
        tls
    };

    struct node_config
    {
        std::string hostname;
        uint16_t port;
        connection_method method = connection_method::plain;
        std::size_t pool_size = 3;
    };

    std::list<node_config> nodes;
};

// ---------------------------------------------------------------------

class mod
{
public:
    /**
     * Configuration: add job queue and worker threads to send to back-end
     * asynnc.
     */
    explicit mod(const config& cfg);
    ~mod();

    void start();

    /**
     * Get a node by giving its reference.
     *
     * @throw node is unknown
     */
    protocol::client_pool& node(const node_ref& ref);

    /**
     * Query status information from all nodes.
     */
    std::list< std::pair<node_ref, std::size_t> > bc_free_space();

    /**
     * Send a read request for a given hash to all nodes.
     * @return the block for the given hash
     * @throw the block was not found
     */
    std::unique_ptr<io::device> bc_read_block(const uh::protocol::blob& hash);

    /**
     * Allocate a chunk of given side on a cluster node and return an allocation
     * to it.
     */
    std::unique_ptr<uh::protocol::allocation> allocate(std::size_t size);

    /**
     * Routes the data to its corresponding data node
     * @param buffer data to be written on one of the data nodes
     * @return the hash of the data
     */
    protocol::block_meta_data write_small_block (std::span <char> buffer);


    /**
     * Routes different blocks to their corresponding data nodes
     * @param buffer array of blocks
     * @return the hash array of the blocks
     */
    uh::protocol::write_xsmall_blocks::response write_xsmall_blocks (const uh::protocol::write_xsmall_blocks::request &req);

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

} // namespace uh::an::cluster

#endif
