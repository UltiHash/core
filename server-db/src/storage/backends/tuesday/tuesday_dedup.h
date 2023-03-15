#ifndef SERVER_DB_STORAGE_BACKENDS_TUESDAY_TUESDAY_DEDUP_H
#define SERVER_DB_STORAGE_BACKENDS_TUESDAY_TUESDAY_DEDUP_H

#include <storage/backend.h>
#include <metrics/storage_metrics.h>


namespace uh::dbn::storage
{

// ---------------------------------------------------------------------

class tuesday_dedup : public backend
{
public:
    tuesday_dedup(metrics::storage_metrics& metrics, std::size_t alloc_size);
    virtual ~tuesday_dedup();

    virtual std::unique_ptr<io::device> read_block(const uh::protocol::blob& hash) override;
    virtual std::unique_ptr<uh::protocol::allocation> allocate(std::size_t size) override;
    virtual size_t free_space() override;

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
