#ifndef LS_LICENSE_ID_H
#define LS_LICENSE_ID_H

#ifdef _MSC_VER
#pragma once
#pragma warning( push )
#pragma warning( disable : 4251 )
#endif

#include <string>
#include "APIDef.h"

namespace LicenseSpring
{
    /// \brief Class that represents License identifier.
    /// \details For key-based products it is license key, for user-based products it is user credentials - user identifier (typically email) and password.\n
    /// Please note: password only required for license activation, other API does not require password. This class does not throw exceptions.
    class LS_API LicenseID
    {
    public:
        /// \brief Default constructor, constructs invalid (empty) LicenseID.
        LicenseID() = default;

        /// \brief Default copy constructor
        LicenseID( const LicenseID& ) = default;

        /// \brief Checks if LicenseID is empty.
        /// \details LicenseID considered empty if both license key and user are empty strings.
        /// \return True if empty and false otherwise.
        bool isEmpty() const;

        /// \brief Checks if LicenseID is valid.
        /// \details LicenseID considered valid if one of the members (key or user) is not empty. Equivalent of !isEmpty().
        /// \return True if valid and false otherwise.
        bool isValid() const;

        /// \brief Returns license key or user identifier, depending of what is present.
        /// \return License key or user identifier string.
        const std::string& id() const;

        /// \brief Returns license key or empty string, if key is not set (for example in case of user-based product).
        /// \return License key or empty string.
        const std::string& key() const;

        /// \brief Returns user identifier or empty string, if user is not set (for example in case of key-based product).
        /// \return User identifier or empty string.
        const std::string& user() const;

        /// \brief Returns user password or empty string, if password is not set.
        /// \details SDK does not store password on disk or anywhere else.
        /// \return Password or empty string.
        const std::string& password() const;

        /// \brief Sets user password accordingly to provided string.
        /// \param password Password string.
        void setPassword( const std::string& password );

        /// \brief Remove trailing whitespaces from the end and beginning of ID (key or user and password).
        void trim();

        /// \brief Constructs LicenseID with given license key.
        /// \details The ID will be automatically trimmed, no need to call trim manually if you use this function.
        /// \param licenseKey The license key used to activate your license
        /// \param needTrim Trims whitespaces from beginning and end of ID, if true. True by default.
        /// \return LicenseID object.
        /// \note There are multiple methods with the same name and different parameters.
        static LicenseID fromKey( const std::string& licenseKey, bool needTrim = true );

        /// \brief Constructs LicenseID with given license key.
        /// \details The ID will be automatically trimmed, no need to call trim manually if you use this function.
        /// \param licenseKey The license key used to activate your license
        /// \return LicenseID object.
        /// \note There are multiple methods with the same name and different parameters.
        static LicenseID fromKey( const char* licenseKey );

        /// \brief Constructs LicenseID with given user credentials.
        /// \details The ID will be automatically trimmed, no need to call trim manually if you use this function.
        /// \param user The username for the license
        /// \param password The corresponding password for the username, empty by default
        /// \param needTrim Trims whitespaces from beginning and end of ID, if true. True by default.
        /// \return LicenseID object.
        /// \note There are multiple methods with the same name and different parameters.
        static LicenseID fromUser( const std::string& user, const std::string& password = std::string(),
                                   bool needTrim = true );

        /// \brief Constructs LicenseID with given user credentials.
        /// \details The ID will be automatically trimmed, no need to call trim manually if you use this function.
        /// \param user The username for the license
        /// \param password The corresponding password for the username, empty by default
        /// \return LicenseID object.
        /// \note There are multiple methods with the same name and different parameters.
        static LicenseID fromUser( const char* user, const char* password = nullptr );

    private:
        std::string m_key;
        std::string m_user;
        std::string m_password;
    };
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // LS_LICENSE_ID_H
