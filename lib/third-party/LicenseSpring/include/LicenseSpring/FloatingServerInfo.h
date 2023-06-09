#ifndef LS_FLOATING_SERVER_INFO_H
#define LS_FLOATING_SERVER_INFO_H

#ifdef _MSC_VER
#pragma once
#pragma warning( push )
#pragma warning( disable : 4251 )
#endif

#include <string>
#include <memory>
#include <vector>
#include "APIDef.h"

namespace LicenseSpring
{
    /// \brief Class that encapsulates information on Floating server settings.
    /// \details See also https://docs.licensespring.com/docs/apiv4floatingsettings.
    class LS_API FloatingServerInfo
    {
    public:
        using ptr_t = std::shared_ptr<FloatingServerInfo>;

        /// \brief Default constructor, creates empty settings.
        FloatingServerInfo() : m_registrationExpiry( 0 ) {}

        /// \brief Constructs FloatingServerInfo with given data.
        /// \param registrationExpiry The registration expiry time in minutes
        /// \param serverList List of floating servers IP addresses
        FloatingServerInfo( uint32_t registrationExpiry, const std::vector<std::string>& serverList )
            : m_registrationExpiry( registrationExpiry ), m_servers( serverList )
        {}

        /// \brief Getter method for registration timeout.
        /// \return Registration expiry in minutes.
        uint32_t registrationExpiry() const { return m_registrationExpiry; }

        /// \brief Getter method for server list.
        /// \return List of floating servers IP addresses available in the network.
        const std::vector<std::string>& servers() const { return m_servers; }

    private:
        uint32_t                 m_registrationExpiry;
        std::vector<std::string> m_servers;
    };
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // LS_FLOATING_SERVER_INFO_H
