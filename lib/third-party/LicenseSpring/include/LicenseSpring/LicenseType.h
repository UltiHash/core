#ifndef LS_LICENSE_TYPE_H
#define LS_LICENSE_TYPE_H

#ifdef _MSC_VER
#pragma once
#pragma warning( push )
#pragma warning( disable : 4251 )
#endif

#include <string>
#include "APIDef.h"
#include "PODTypes.h"

namespace LicenseSpring
{
    /// \brief Class used for encapsulating the type of license, comparing types of licenses, and converting them to strings.
    /// \details Can be perpetual, subscription, time-limited, or consumption.
    class LS_API LicenseType
    {
    public:
        /// \brief Converts string to a LicenseType.
        /// \details Available strings with corresponding conversions:\n
        /// 'perpetual', 'subscription', 'time-limited', 'consumption'.\n
        /// Must be all lowercase. Any other string will return LicenseTypeUnknown.
        /// \param str String to convert into a LicenseType
        /// \return LicenseType of corresponding string.
        static LicenseType fromString( const std::string& str );

        /// \brief Constructs LicenseType with type LicenseTypeUnknown.
        LicenseType();

        /// \brief Constructs LicenseType given an int.
        /// \param type an int corresponding to the LicenseType
        LicenseType( int type );

        /// \brief Constructs LicenseType with given type.
        /// \param type the type of license given to LicenseType
        LicenseType( LSLicenseType type );

        bool operator == ( LicenseType t ) const;
        bool operator != ( LicenseType t ) const;
        bool operator == ( LSLicenseType t ) const;
        bool operator != ( LSLicenseType t ) const;
        operator LSLicenseType() const;
        operator int() const;

        /// \brief Converts a LicenseType to a string.
        /// \details Possible strings that can be returned include: \n
        /// 'perpetual', 'subscription', 'time-limited', 'consumption' or 'Unknown'.
        /// \return Corresponding string based on type.
        std::string toString() const;

        /// \brief Converts a LicenseType to a string with first letter capitalized.
        /// \return Same as (see toString()), but with first letter capitalized.
        std::string toFormattedString() const;

    private:
        LSLicenseType m_type;
    };
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // LS_LICENSE_TYPE_H
