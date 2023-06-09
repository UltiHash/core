#ifndef LS_LICENSE_MEMORY_STORAGE_H
#define LS_LICENSE_MEMORY_STORAGE_H

#ifdef _MSC_VER
#pragma once
#pragma warning( push )
#pragma warning( disable : 4251 )
#endif

#include <mutex>
#include <string>
#include "LicenseStorage.h"

namespace LicenseSpring
{
    /// \brief Simple class that saves license in a variable, implements <c>LicenseStorage</c> interface.
    /// \details This class is thread safe.
    class LS_API LicenseMemoryStorage : public LicenseStorage
    {
    public:
        using ptr_t = std::shared_ptr<LicenseMemoryStorage>;

        /// \brief Allocates and constructs a LicenseMemoryStorage object and passes args to LicenseMemoryStorage().
        /// \return A shared_ptr object that owns and stores a pointer to a newly allocated LicenseMemoryStorage object.
        static ptr_t create();

        /// \brief Constructor for LicenseMemoryStorage object.
        LicenseMemoryStorage() = default;

        /// \brief Saves license data in a variable.
        /// \param licenseData Encrypted license data string
        void saveLicense( const std::string& licenseData );

        /// \brief Load license data from memory (variable).
        /// \return Encrypted license data string.
        std::string loadLicense();

        /// \brief Purge license data.
        void clear();

    private:
        std::string m_licenseData;
        std::mutex  m_mutex;
    };
} // namespace LicenseSpirng

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // LS_LICENSE_MEMORY_STORAGE_H