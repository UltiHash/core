#ifndef SERVER_DB_STORAGE_BACKENDS_DUMP_STORAGE_H
#define SERVER_DB_STORAGE_BACKENDS_DUMP_STORAGE_H

#include <storage/utils.h>
#include <storage/backend.h>
#include <metrics/storage_metrics.h>


namespace uh::dbn::storage {

class dump_storage : public backend {
    public:
        dump_storage(std::filesystem::path db_root, size_t size_bytes, uh::dbn::metrics::storage_metrics& storage_metrics):
        m_root(db_root),
        m_alloc(size_bytes),
        m_used(0),
        m_storage_metrics(storage_metrics)
        {
            if( !std::filesystem::is_directory(db_root) ) {
                std::string msg("Path does not exist: " + db_root.string());
                throw std::runtime_error(msg);
            }
            else{
                m_used = get_dir_size(m_root);
                if (m_used >= m_alloc)
                {
                    THROW(util::exception, "database used over limit");
                }
                update_space_consumption();
            }
        }

        virtual void start() override;

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
        virtual std::unique_ptr<io::device> read_block(const uh::protocol::blob& hash) override;


        virtual size_t free_space() override {return m_alloc - m_used;}
        size_t free_space_percentage() { return 100 * static_cast<float>(free_space()) / static_cast<float>(m_alloc);};


        virtual size_t used_space() override {return m_used;}
        size_t used_space_percentage() { return 100 * static_cast<float>(m_used) / static_cast<float>(m_alloc);};


        virtual size_t allocated_space() override {return m_alloc;}

        virtual std::string backend_type() override {return std::string(m_type);}

        virtual std::unique_ptr<uh::protocol::allocation> allocate(std::size_t size);

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
        std::filesystem::path m_root = "";

        // invariant: m_used < m_alloc
        size_t m_alloc = 0;
        std::atomic<size_t> m_used = 0;
        uh::dbn::metrics::storage_metrics& m_storage_metrics;
    };

} // namespace uh::dbn::storage

#endif
