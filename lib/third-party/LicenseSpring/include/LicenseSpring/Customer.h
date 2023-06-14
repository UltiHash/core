#ifndef LS_CUSTOMER_H
#define LS_CUSTOMER_H

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
    /// \brief Class that encapsulates information about a customer.
    class LS_API Customer
    {
    public:
        using ptr_t = std::shared_ptr<Customer>;

        /// \brief Constructs customer with no information.
        Customer() = default;

        /// \brief Constructs customer with given email.
        /// \param email Email corresponding to customer
        /// \details Note: email is required if you attempt to get trial license for user-based product.
        Customer( const std::string& email );

        /// \brief Getter method for customer email.
        /// \return Customer email.
        const std::string& email() const;

        /// \brief Setter method for customer email.
        /// \param email Customer email.
        void setEmail( const std::string& email );

        /// \brief Getter method for customer first name.
        /// \return Customer first name.
        const std::string& firstName() const;

        /// \brief Setter method for customer first name.
        /// \param firstName Customer first name.
        void setFirstName( const std::string& firstName );

        /// \brief Getter method for customer last name.
        /// \return Customer last name.
        const std::string& lastName() const;

        /// \brief Setter method for customer last name.
        /// \param lastName Customer email.
        void setLastName( const std::string& lastName );

        /// \brief Getter method for customer company.
        /// \return Customer company.
        const std::string& company() const;

        /// \brief Setter method for customer company name.
        /// \param company Customer company.
        void setCompany( const std::string& company );

        /// \brief Getter method for customer phone.
        /// \return Customer phone.
        const std::string& phone() const;

        /// \brief Setter method for customer phone.
        /// \param phone Customer phone.
        void setPhone( const std::string& phone );

        /// \brief Getter method for customer address.
        /// \return Customer address.
        const std::string& address() const;

        /// \brief Setter method for customer address.
        /// \param address Customer address.
        void setAddress( const std::string& address );

        /// \brief Getter method for customer city.
        /// \return Customer city.
        const std::string& city() const;

        /// \brief Setter method for customer city.
        /// \param city Customer city.
        void setCity( const std::string& city );

        /// \brief Getter method for customer state/province/region.
        /// \return Customer state.
        const std::string& state() const;

        /// \brief Setter method for customer state/province/region.
        /// \param state Customer state.
        void setState( const std::string& state );

        /// \brief Getter method for customer country.
        /// \return Customer country.
        const std::string& country() const;

        /// \brief Setter method for customer country.
        /// \param country Customer country.
        void setCountry( const std::string& country );

        /// \brief Getter method for customer postcode/zipcode.
        /// \return Customer postcode.
        const std::string& postcode() const;

        /// \brief Setter method for customer postcode/zipcode.
        /// \param postcode Customer postcode.
        void setPostcode( const std::string& postcode );

        /// \brief Getter method for customer reference.
        /// \return Customer reference.
        const std::string& reference() const;

        /// \brief Setter method for customer reference.
        /// \param reference Customer reference.
        void setReference( const std::string& reference );

        /// \brief Getter method for customer metadata.
        /// \return Customer metadata.
        const std::string& metadata() const;

        /// \brief Setter method for customer metadata.
        /// \param metadata Customer metadata.
        void setMetadata( const std::string& metadata );

    private:
        std::string m_email;
        std::string m_firstName;
        std::string m_lastName;
        std::string m_company;
        std::string m_phone;
        std::string m_address;
        std::string m_city;
        std::string m_state;
        std::string m_country;
        std::string m_postcode;
        std::string m_reference;
        std::string m_metadata;
    };
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // LS_CUSTOMER_H
