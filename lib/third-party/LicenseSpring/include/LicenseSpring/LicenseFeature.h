#ifndef LS_LICENSE_FEATURE_H
#define LS_LICENSE_FEATURE_H

#ifdef _MSC_VER
#pragma once
#pragma warning( push )
#pragma warning( disable : 4251 )
#endif

#include <string>
#include <ctime>
#include "PODTypes.h"
#include "APIDef.h"
#include "ConsumptionPeriod.h"

namespace LicenseSpring
{
    /// \brief Class that encapsulates information about license feature.
    /// \details Includes information for a license feature's typing, code, name, consumption values, and expiry dates.
    class LS_API LicenseFeature
    {
    public:
        /// \brief Constructs invalid license feature with no information.
        LicenseFeature();

        /// \brief Constructs license feature with given code, name and type.
        /// \param code The product feature code
        /// \param name The name of the product feature on this license
        /// \param type Either FeatureTypeNone, FeatureTypeActivation, or FeatureTypeConsumption
        LicenseFeature( const std::string& code, const std::string& name, LSFeatureType type );

        /// \brief Constructs license feature with all of provided info.
        /// \param code The product feature code
        /// \param name The name of the product feature on this license
        /// \param type Either FeatureTypeNone, FeatureTypeActivation, or FeatureTypeConsumption
        /// \param maxConsumption The total amount of consumption allowed for this feature
        /// \param totalConsumption The current amount of consumption used for this feature
        /// \param localConsumption The current local consumption specific to this device
        /// \param expiryDate The expiration date for the product feature on this license
        /// \param allowOverages Is overage allowed for this license feature
        /// \param maxOverages Maximum number of overages
        /// \param resetConsumption Is consuption reset is enabled for this license feature
        /// \param allowUnlimitedConsumptions Is unlimited consumption is enabled for this license feature
        /// \param consumptionPeriod The period of consumption reset
        /// \param metadata The product feature metadata
        LicenseFeature( const std::string& code, const std::string& name, LSFeatureType type,
                        int32_t maxConsumption, int32_t totalConsumption, int32_t localConsumption,
                        const tm& expiryDate, bool allowOverages, int32_t maxOverages,
                        bool resetConsumption, bool allowUnlimitedConsumptions, ConsumptionPeriod consumptionPeriod, const std::string& metadata );

        /// \brief Getter method for license feature code.
        /// \return License feature code.
        const std::string& code() const;

        /// \brief Getter method for license feature name.
        /// \return License feature name.
        const std::string& name() const;

        /// \brief Getter method for license feature type.
        /// \return License feature name.
        LSFeatureType featureType() const;

        /// \brief Getter method for license feature maximum consumption.
        /// \details Meaningful only for consumption feature type.
        /// \return License feature maximum consumption.
        int32_t maxConsumption() const;

        /// \brief Getter method for license feature current total consumption.
        /// \details Please note, it includes localConsumption. Meaningful only for consumption feature type.
        /// \return License feature total consumption + local consumption.
        int32_t totalConsumption() const;

        /// \brief Getter method for license feature local consumption.
        /// \details This is consumption that has not been synchronized with the backend yet.\n
        /// Meaningful only for consumption feature type.
        /// \return License feature local consumption.
        int32_t localConsumption() const;

        /// \brief Returns maximum overage allowed for the license feature.
        /// \details Meaningful only for Consumption feature type, returns 0 in case of unlimited overages.
        int32_t maxOverages() const;

        /// \brief Checks if overage is allowed for Consumption license feature.
        /// \details Meaningful only for Consumption feature type.
        bool isOveragesAllowed() const;

        /// \brief Returns period of time after which consumption is reset.
        /// \details Meaningful only for Consumption feature type.
        ConsumptionPeriod consumptionPeriod() const;

        /// \brief Checks if consumption reset is allowed for Consumption license feature.
        /// \details Meaningful only for Consumption feature type.
        bool isResetConsumptionEnabled() const;

        /// \brief Checks if unlimited consumption is allowed for Consumption license feature.
        /// \details Meaningful only for Consumption feature type.
        bool isUnlimitedConsumptionAllowed() const;

        /// \brief Getter method for license feature expiry date.
        /// \details If expiry date has not been set this method returns maximum date (31.12.9999).\n
        /// In this case it means that feature is perpetual and cannot expire.
        /// \return License feature expiry date.
        tm expiryDate() const;

        /// \brief Getter method for license feature expiry date in UTC.
        /// \details Returns the same as expiryDate, but date time converted to UTC.
        /// \return License feature expiry date in UTC.
        tm expiryDateUtc() const;

        /// \brief Getter method for license feature metadata.
        /// \return License feature metadata.
        const std::string& metadata() const;

        /// \brief Checks if license feature expired.
        /// \return True if feature expired and false otherwise.
        bool isExpired() const;

        /// \brief Helper method, converts string to LSFeatureType.
        /// \details If value == 'activation', returns FeatureTypeActivation.\n
        /// If value == 'consumption', returns FeatureTypeConsumption, otherwise returns FeatureTypeNone.\n
        /// Must be written as above, i.e. all lowercase.
        /// \param value String to convert. 
        /// \return LSFeatureType.
        static LSFeatureType FeatureTypeFromString( const std::string& value );

        /// \brief Helper method, converts LSFeatureType to string.
        /// \details String-value pairs same as FeatureTypeFromString().\n
        /// FeatureTypeNone returns an empty string.
        /// \param value LSFeatureType to convert
        /// \return string representation of LSFeatureType.
        static std::string FeatureTypeToString( LSFeatureType value );

        /// \brief Helper method, compose license feature info into string.
        /// \details This is helper method, you should not rely on its output.
        /// \return String with info on license feature.
        std::string toString() const;

    private:
        std::string       m_code;
        std::string       m_name;
        LSFeatureType     m_featureType;
        int32_t           m_maxConsumption;
        int32_t           m_totalConsumption;
        int32_t           m_localConsumption;
        int32_t           m_maxOverages;
        bool              m_allowOverages;
        bool              m_allowUnlimitedConsumptions;
        bool              m_resetConsumption;
        ConsumptionPeriod m_consumptionPeriod;
        tm                m_expiryDate;
        std::string       m_metadata;
    };
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // LS_LICENSE_FEATURE_H
