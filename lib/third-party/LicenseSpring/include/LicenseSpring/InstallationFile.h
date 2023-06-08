#ifndef LS_INSTALLATION_FILE_H
#define LS_INSTALLATION_FILE_H

#ifdef _MSC_VER
#pragma once
#pragma warning( push )
#pragma warning( disable : 4251 )
#endif

#include <string>
#include <memory>
#include "APIDef.h"

namespace LicenseSpring
{
    /// \brief Helper struct for filtering installation files by environment and channel.
    /// \see LicenseManager::getInstallationFile, getVersionList and License::check
    struct LS_API InstallFileFilter
    {
        /// \brief Default constructor, creates empty filter. Corresponds to default channel and environment.
        InstallFileFilter() = default;

        /// \brief Constructs filter with given channel and environment from c-strings.
        /// \param channel Channel to filter by
        /// \param env Environment to filter by
        InstallFileFilter( const char* channel, const char* env = nullptr )
        {
            if( channel != nullptr ) Channel = channel;
            if( env != nullptr ) Environment = env;
        }

        /// \brief Constructs filter with given channel and environment.
        /// \param channel Channel to filter by
        /// \param env Environment to filter by
        InstallFileFilter( const std::string& channel, const std::string& env = std::string() )
            : Channel( channel ), Environment( env ) {}

        /// \brief Channel from where installation file should be requested.
        /// \details For example can be "prod", "dev", whatever. Empty string means default channel.
        std::string Channel;

        /// \brief Environment for which installation file should be requested.
        /// \details For example can be "win", "win32", "win64", "mac", "linux", "linux32", "linux64".
        std::string Environment;
    };


    /// \brief Class that represents information on product installation file.
    /// \details Holds information on product installer or setup file or whatever uploaded
    ///  to the backend using LicenseSpring platform (see Configure Products -> Product Versions) or Management API.
    class LS_API InstallationFile
    {
    public:
        using ptr_t = std::shared_ptr<InstallationFile>;

        /// \brief Default constructor, creates empty installation file.
        InstallationFile() : m_size( 0 ) {}

        /// \brief Constructs InstallationFile with given data.
        /// \param url URL for installation file
        /// \param version Version of installation file
        /// \param requiredVersion Required version to install files
        /// \param md5hash MD5 hash for the installation file
        /// \param releaseDate Release date of installation file
        /// \param environment Environment for installation file
        /// \param eulaLink End-User License Agreement for installation file
        /// \param releaseNotesLink Link to release notes
        /// \param channel Channel from where installation file should be requested 
        /// \param size Size of installation file
        InstallationFile( const std::string& url, const std::string& version,
            const std::string& requiredVersion, const std::string& md5hash,
            const std::string& releaseDate, const std::string& environment,
            const std::string& eulaLink, const std::string& releaseNotesLink,
            const std::string& channel, uint64_t size )
            : m_url( url ), 
            m_version( version ),
            m_requiredVersion( requiredVersion ),
            m_md5hash( md5hash ),
            m_releaseDate( releaseDate ),
            m_environment( environment ),
            m_eulaLink( eulaLink ),
            m_releaseNotesLink( releaseNotesLink ),
            m_channel( channel ),
            m_size( size )
        {};

        /// \brief Getter method for setup file URL.
        /// \return InstallationFile URL.
        const std::string& url() const { return m_url; }

        /// \brief Getter method for setup file version.
        /// \return InstallationFile version.
        const std::string& version() const { return m_version; }

        /// \brief Getter method for setup file required version.
        /// \details This functionality allows you implement intermediate updates.
        /// \return app version that is required to install this InstallationFile.
        const std::string& requiredVersion() const { return m_requiredVersion; }

        /// \brief Getter method for setup file MD5 hash.
        /// \return InstallationFile hash.
        const std::string& md5Hash() const { return m_md5hash; }

        /// \brief Getter method for setup file release date.
        /// \return InstallationFile rlease date in format %Y-%m-%d.
        const std::string& releaseDate() const { return m_releaseDate; }

        /// \brief Getter method for setup file environment.
        /// \return InstallationFile environment.
        const std::string& environment() const { return m_environment; }

        /// \brief Getter method for setup file End-User License Agreement link.
        /// \return InstallationFile End-User License Agreement link.
        const std::string& eulaLink() const { return m_eulaLink; }

        /// \brief Getter method for setup file release notes link.
        /// \return InstallationFile release notes link.
        const std::string& releaseNotesLink() const { return m_releaseNotesLink; }

        /// \brief Getter method for setup file channel.
        /// \return InstallationFile channel.
        const std::string& channel() const { return m_channel; }

        /// \brief Getter method for setup file size.
        /// \return InstallationFile size in bytes.
        uint64_t size() const { return m_size; }

    private:
        std::string m_url;
        std::string m_version;
        std::string m_requiredVersion;
        std::string m_md5hash;
        std::string m_releaseDate;
        std::string m_environment;
        std::string m_eulaLink;
        std::string m_releaseNotesLink;
        std::string m_channel;
        uint64_t    m_size;
    };
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // LS_INSTALLATION_FILE_H
