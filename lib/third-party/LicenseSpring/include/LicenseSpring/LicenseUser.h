#ifndef LS_LICENSE_USER_H
#define LS_LICENSE_USER_H

#ifdef _MSC_VER
#pragma once
#pragma warning( push )
#pragma warning( disable : 4251 )
#endif

#include <memory>
#include <string>
#include "APIDef.h"

namespace LicenseSpring
{
    /// \brief Class that encapsulates information about a license user.
    /// \details Used to hold information for user-based licenses.
    class LS_API LicenseUser
    {
    public:
        using ptr_t = std::shared_ptr<LicenseUser>;

        /// \brief Constructs a license user with no information.
        LicenseUser();

        /// \brief Constructs a license user with given information.
        /// \param isActive Boolean for whether user license is active
        /// \param isInitialPassword Boolean for whether user uses initial password still
        /// \param id User ID
        /// \param email User email
        /// \param firstName User first name
        /// \param lastName User last name
        /// \param phoneNumber User phone number
        /// \param initialPassword User initial password
        /// \param licenseId User license ID
        /// \param orderId Order ID of user
        /// \param orderStoreId Order store ID
        LicenseUser( bool isActive, bool isInitialPassword, uint64_t id, const std::string& email,
                     const std::string& firstName, const std::string& lastName,
                     const std::string& phoneNumber = std::string(),
                     const std::string& initialPassword = std::string(),
                     uint64_t licenseId = 0, uint64_t orderId = 0,
                     const std::string& orderStoreId = std::string() );

        /// \brief Getter method for user status (active/inactive).
        /// \return User status.
        bool isActive() const;

        /// \brief Getter method for password status, has it been changed by user or not.
        /// \return Password status.
        bool isInitialPassword() const;

        /// \brief Getter method for user id.
        /// \return User id.
        uint64_t id() const;

        /// \brief Getter method for user email.
        /// \return User email.
        const std::string& email() const;

        /// \brief Getter method for user first name.
        /// \return User first name.
        const std::string& firstName() const;

        /// \brief Getter method for user last name.
        /// \return User last name.
        const std::string& lastName() const;

        /// \brief Getter method for user phone number.
        /// \return User phone number.
        const std::string& phoneNumber() const;

        /// \brief Getter method for user initial password.
        /// \details It may be empty string if user changed the password.
        /// \return User initial password.
        const std::string& initialPassword() const;

        /// \brief Getter method for license internal identifier.
        /// \details Do not confuse this id with LicenseID. It is license id in the DB.
        /// \return User license id.
        uint64_t licenseId() const;

        /// \brief Getter method for order internal identifier.
        /// \return Order id.
        uint64_t orderId() const;

        /// \brief Getter method for order store id.
        /// \return Order store id.
        const std::string& orderStoreId() const;

    private:
        bool        m_isActive;
        bool        m_isInitialPassword;
        uint64_t    m_id;
        std::string m_email;
        std::string m_firstName;
        std::string m_lastName;
        std::string m_phoneNumber;
        std::string m_initialPassword;
        uint64_t    m_licenseId;
        uint64_t    m_orderId;
        std::string m_orderStoreId;
    };
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // LS_LICENSE_USER_H
