#ifndef SERVER_DB_STORAGE_BACKEND_DUMP_H
#define SERVER_DB_STORAGE_BACKEND_DUMP_H


#include <storage/utils.h>
#include <storage/storage_backend.h>
#include <metrics/storage_metrics.h>


namespace uh::dbn::storage {

class dump_storage : public storage_backend {
    public:
        dump_storage(std::filesystem::path db_root, size_t size_bytes, uh::dbn::metrics::storage_metrics& storage_metrics):
        m_root(db_root),
        m_alloc(size_bytes),
        m_free(size_bytes),
        m_used(0),
        m_storage_metrics(storage_metrics)
        {
            if( !std::filesystem::is_directory(db_root) ) {
                std::string msg("Path does not exist: " + db_root.string());
                throw std::runtime_error(msg);
            }
            else{
                update_space_consumption(); //storage metrics initialized here.
            }
        }

        virtual void start() override;

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
        virtual uh::protocol::blob write_block(const uh::protocol::blob &data) override;

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
        virtual uh::protocol::blob read_block(const uh::protocol::blob &hash) override;


        virtual size_t free_space() override {return m_free;}
        size_t free_space_percentage() { return 100 * static_cast<float>(m_free) / static_cast<float>(m_alloc);};


        virtual size_t used_space() override {return m_used;}
        size_t used_space_percentage() { return 100 * static_cast<float>(m_used) / static_cast<float>(m_alloc);};


        virtual size_t allocated_space() override {return m_alloc;}

        virtual std::string backend_type() override {return std::string(m_type);}

        void update_space_consumption();


    private:

        /**
         * Given a block of data, return its hash
         *
         * This function computes a hash string givena block of binary data
         *
         * @return the hash
         * @throw may throw any derivative of exception on error
         */
        uh::protocol::blob hashing_function(const uh::protocol::blob &data);

        /**
         * Given a hash string as a uh::protocol::blob, return the file path
         * to the corresponding data block
         * @param hash: the hash as a std::vector
         * @return a file path as a boost::filesystem::path
         */
        virtual std::filesystem::path get_filepath_from_hash(const uh::protocol::blob &hash);

    protected:

        constexpr static std::string_view m_type = "DumpStorage";
        std::filesystem::path m_root = ""; //root path of the db
        size_t m_alloc = 0; //total space
        size_t m_free = 0;  //free space
        size_t m_used = 0;  //used space
        uh::dbn::metrics::storage_metrics& m_storage_metrics;
    };

} // namespace uh::dbn::storage

#endif
