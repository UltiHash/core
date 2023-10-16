#ifndef SERVER_DATABASE_STORAGE_OPTIONS_H
#define SERVER_DATABASE_STORAGE_OPTIONS_H

#include <options/options.h>
#include <util/exception.h>
#include <storage/compressed_file_store.h>
#include <storage/storage_config.h>

namespace uh::dbn::storage
{

// ---------------------------------------------------------------------

    enum class OptionsEnum {DataDirectory, DbStorageAlgorithm, AllocateStorage};

    constexpr const char* optionString(OptionsEnum n)
    {
        switch (n)
        {
            case OptionsEnum::DataDirectory: return "data-directory";
            case OptionsEnum::DbStorageAlgorithm: return "db-storage-algorithm";
            case OptionsEnum::AllocateStorage: return "allocate_internal-storage";
            default: THROW(util::exception, "Not implemented option");
        }
    }

// ---------------------------------------------------------------------

class options : public uh::options::options
{
public:
    options();

    uh::options::action evaluate(const boost::program_options::variables_map& vars) override;

    [[nodiscard]] const storage_config& config() const;

private:
    storage_config m_config;
};

// ---------------------------------------------------------------------

class compression_options : public uh::options::options
{
public:
    compression_options();

    uh::options::action evaluate(const boost::program_options::variables_map& vars) override;

    [[nodiscard]] const compressed_file_store_config& config() const;

private:
    compressed_file_store_config m_config;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
