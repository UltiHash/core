#ifndef SERVER_DB_STORAGE_BACKENDS_TUESDAY_DEVICES_H
#define SERVER_DB_STORAGE_BACKENDS_TUESDAY_DEVICES_H

#include <io/device.h>
#include "tree.h"


namespace uh::dbn::storage
{

// ---------------------------------------------------------------------

class tuesday_write_device : public io::device
{
public:
    tuesday_write_device(tree& t, tree_path& path, std::size_t& alloc_size);

    virtual std::streamsize write(std::span<const char> buffer) override;

    virtual std::streamsize read(std::span<char> buffer) override;

    virtual bool valid() const override;

private:
    tree& m_tree;
    tree_path& m_path;
    std::size_t& m_alloc_size;
    std::size_t m_offset = 0u;
};

// ---------------------------------------------------------------------

class tuesday_read_device : public io::device
{
public:
    tuesday_read_device(const tree_path& p);

    virtual std::streamsize write(std::span<const char> buffer) override;

    virtual std::streamsize read(std::span<char> buffer) override;

    virtual bool valid() const override;

private:
    tree_path m_p;

    std::size_t m_offset;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
