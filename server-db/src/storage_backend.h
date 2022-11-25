#ifndef SERVER_DB_STORAGE_BACKEND_H
#define SERVER_DB_STORAGE_BACKEND_H

#include <vector>


namespace uh::dbn
{

// ---------------------------------------------------------------------

class storage_backend
{
public:
    virtual ~storage_backend() = default;


    /**
     * Write a data block and return it's hash.
     *
     * This function stores the block contained in `data` in an permanent
     * storage and returns a hash that can be used to retrieve the data
     * at a later point of time.
     *
     * @return the hash
     * @throw may throw any derivative of std::exception on error
     */
    virtual std::vector<char> write_block(const std::vector<char>& data) = 0;

    /**
     * Read a data block identified by it's hash from the storage.
     *
     * This function reads the block identified by `hash` from the storage and
     * returns it in a vector. If the block is not known the function will
     * throw.
     *
     * @return the data block
     * @throw may throw any derivative of std::exception on error
     */
    virtual std::vector<data> read_block(const std::vector<char>& hash) = 0;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn

#endif
