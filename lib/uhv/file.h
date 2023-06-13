#ifndef UHV_FILE_H
#define UHV_FILE_H

#include <filesystem>
#include <uhv/meta_data.h>
#include <uhv/job_queue.h>


namespace uh::uhv
{

// ---------------------------------------------------------------------

class file
{
public:
    /**
     * Open a UHV file at the given path.
     */
    file(const std::filesystem::path& path);

    /**
     * Overwrite the contents of the UHV file with the list of metadata.
     */
    void serialize(const std::list<std::unique_ptr<uhv::meta_data>>& metadata);

    /**
     * Read the current contents of the UHV file and return it via `out_queue`.
     */
    void deserialize(uhv::job_queue<std::unique_ptr<uhv::meta_data>>& out_queue);

    /**
     * Read the current contents of the UHV file and return it in a list.
     */
    std::list<std::unique_ptr<uhv::meta_data>> deserialize();

    /**
     * Append a new entry to the end of the UHV file.
     */
    void append(std::unique_ptr<uhv::meta_data> md);

private:
    std::filesystem::path m_path;
};

// ---------------------------------------------------------------------

} // namespace uh::uhv

#endif
