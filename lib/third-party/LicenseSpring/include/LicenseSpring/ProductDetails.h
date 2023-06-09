#ifndef LS_PRODUCT_DETAILS_H
#define LS_PRODUCT_DETAILS_H

#ifdef _MSC_VER
#pragma once
#pragma warning( push )
#pragma warning( disable : 4251 )
#endif

#include <memory>
#include <string>
#include "APIDef.h"
#include "PODTypes.h"

namespace LicenseSpring
{
    /// \brief Class that encapsulates information about a product
    /// \details Product information can be obtained through the backend using BaseManager::getProductDetails()
    class LS_API ProductDetails
    {
    public:
        using ptr_t = std::shared_ptr<ProductDetails>;

        /// \brief Default constructor, creates empty (invalid) Product details.
        ProductDetails();

        /// \brief Checks whether trial license is allowed for this product or not.
        /// \return True if trial licenses are allowed, false if not.
        bool isTrialAllowed() const;

        /// \brief Checks if running the product on virtual machines is allowed.
        /// \return True if virtual machines are allowed for this product, false if not.
        bool isVMAllowed() const;

        /// \brief Getter for trial period value in days.
        /// \return Integer representing trial period value in days.
        uint32_t trialPeriod() const;

        /// \brief Getter for floating licenses in minutes.
        /// \return Integer representing the default timeout for floating licenses in minutes.
        uint32_t floatingLicenseTimeout() const;

        /// \brief Getter for product name.
        /// \return String of product name.
        const std::string& productName() const;

        /// \brief Getter for product code.
        /// \return String of product code.
        const std::string& productCode() const;

        /// \brief Getter for product authorization method.
        /// \return LSAuthMethod object stating the product authorization method, i.e. AuthMethodKeyBased or AuthMethodUserBased.
        LSAuthMethod authorizationMethod() const;

        /// \brief Helper method, creates ProductDetails object from JSON string.
        /// \return ProductDetails object from JSON string.
        static ProductDetails fromJsonString( const std::string& jsonString );

    private:
        bool                m_isTrialAllowed;
        bool                m_isVMAllowed;
        uint32_t            m_trialPeriod;
        uint32_t            m_floatingTimeout;
        std::string         m_productName;
        std::string         m_productCode;
        LSAuthMethod m_authorizationMethod;
    };
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // LS_PRODUCT_DETAILS_H
