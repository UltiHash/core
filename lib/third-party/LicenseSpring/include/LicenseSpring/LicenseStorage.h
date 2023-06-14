#ifndef LS_LICENSE_STORAGE_H
#define LS_LICENSE_STORAGE_H

#ifdef _MSC_VER
#pragma once
#pragma warning( push )
#pragma warning( disable : 4251 )
#endif

#include "APIDef.h"

namespace LicenseSpring
{
    /// \brief LicenseStorage interface
    /// \details You can implement this interface to provide your custom license storage logic. For example to store license in your DB, Windows registry, remotely etc.
    class LS_API LicenseStorage
    {
    public:
        using ptr_t = std::shared_ptr<LicenseStorage>;

        /// \brief Save license data somewhere.
        /// \param licenseData Encrypted license data string
        /// \details You can throw any appropriate exception in case of errors, it will be wrapped into LocalLicenseException and propagated further.
        virtual void saveLicense( const std::string& licenseData ) = 0;

        /// \brief Load license data.
        /// \details Return empty string if license does not exists.
        /// You can throw any appropriate exception in case of errors, it will be wrapped into LocalLicenseException and propagated further.
        /// \return Encrypted license data string.
        virtual std::string loadLicense() = 0;

        /// \brief Removes local license data.
        virtual void clear() = 0;

        /// \brief Destructor for LicenseStorage object.
        virtual ~LicenseStorage();
    };
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // LS_LICENSE_STORAGE_H