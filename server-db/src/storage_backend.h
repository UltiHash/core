#ifndef SERVER_DB_STORAGE_BACKEND_H
#define SERVER_DB_STORAGE_BACKEND_H

#include <vector>
#include <string>
#include <boost/filesystem.hpp>


namespace fs = boost::filesystem;

namespace uh::dbn {

// ---------------------------------------------------------------------

    struct db_config{
        fs::path db_dir = "./DB_ROOT";
        fs::path db_filename = "block_test_file";
        fs::path db_file_fullpath = db_dir / db_filename;
    };

// ---------------------------------------------------------------------

    class storage_backend_interface {
    public:
        virtual ~storage_backend_interface() = default;


        /**
         * Write a data block and return it's hash.
         *
         * This function stores the block contained in `data` in an permanent
         * storage and returns a hash that can be used to retrieve the data
         * at a later point of time.
         *
         * @return the hash
         * @throw may throw any derivative of exception on error
         */
        virtual std::vector<char> write_block(const std::vector<char> &data) = 0;

        /**
         * Read a data block identified by it's hash from the storage.
         *
         * This function reads the block identified by `hash` from the storage and
         * returns it in a vector. If the block is not known the function will
         * throw.
         *
         * @return the data block
         * @throw may throw any derivative of exception on error
         */
        virtual std::vector <char> read_block(const std::vector<char> &hash) = 0;
    };

// ---------------------------------------------------------------------

    class storage_backend : public storage_backend_interface {
    public:
        storage_backend(const db_config &some_config):
        m_config(some_config)
        {
            if(!(boost::filesystem::exists(some_config.db_dir))) {
                std::string msg("Path does not exist: " + some_config.db_dir.string());
                throw std::runtime_error(msg);
            }
        }

        /**
         * Write a data block and return it's hash.
         *
         * This function stores the block contained in `data` in an permanent
         * storage and returns a hash that can be used to retrieve the data
         * at a later point of time.
         *
         * @return the hash
         * @throw may throw any derivative of exception on error
         */
        virtual std::vector<char> write_block(const std::vector<char> &data) override;

        /**
         * Read a data block identified by it's hash from the storage.
         *
         * This function reads the block identified by `hash` from the storage and
         * returns it in a vector. If the block is not known the function will
         * throw.
         *
         * @return the data block
         * @throw may throw any derivative of exception on error
         */
        virtual std::vector <char> read_block(const std::vector<char> &hash) override;

    private:

        db_config m_config;
    };

// ---------------------------------------------------------------------

} // namespace uh::dbn

#endif
