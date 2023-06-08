#ifndef LS_VERSION_H
#define LS_VERSION_H

#ifdef _MSC_VER
#pragma once
#pragma warning( push )
#pragma warning( disable : 4251 )
#endif

#include <string>
#include "APIDef.h"

namespace LicenseSpring
{
    /*!
        \brief Base class for handling app versions.
        Contains common functionality for version classes hierarchy.
    */
    class LS_API VersionBase
    {
    public:
        static constexpr int INVALID_VERSION_NUMBER = -1;

        VersionBase();
        VersionBase( int major, int minor, char delimeter = '.' );
        virtual ~VersionBase() = default;

        int majorVersion() const { return m_major; }
        int minorVersion() const { return m_minor; }

        char delimeter() const { return m_delimeter; }
        void setDelimeter( char delimeter ) { m_delimeter = delimeter; }

        size_t size() const;
        virtual size_t length() const;

        virtual std::string toString() const;

        operator std::string() const;

        bool operator==( const VersionBase& version ) const;
        bool operator!=( const VersionBase& version ) const;
        bool operator<( const VersionBase& version ) const;
        bool operator>( const VersionBase& version ) const;
        bool operator<=( const VersionBase& version ) const;
        bool operator>=( const VersionBase& version ) const;

        virtual int compare( const VersionBase& version ) const;

        static int compare( const VersionBase& left, const VersionBase& right );

    protected:
        int m_major;
        int m_minor;
        char m_delimeter;
    };

    LS_API std::ostream& operator<<( std::ostream& out, const VersionBase& version );

    /*!
        \brief Simple class for handling app versions.
        Format: Major.Minor.Patch.Build, where all parts are integer numbers.
        In case you use such version format without alphabetical symbols then it fits your needs.
        For example: "1.3.0.45", "2.13.1.3735"
    */
    class LS_API Version : public VersionBase
    {
    public:
        Version();
        Version( int major, int minor, char delimeter = '.' );
        Version( int major, int minor, int patch, char delimeter = '.' );
        Version( int major, int minor, int patch, int build, char delimeter = '.' );
        Version( const std::string& version, char delimeter = '.' );

        int patchVersion() const { return m_patch; }
        int buildVersion() const { return m_build; }
        bool hasPatchVersion() const { return m_patch >= 0; }
        bool hasBuildVersion() const { return m_build >= 0; }
        void setPatchVersion( int number );
        void setBuildVersion( int number );

        size_t length() const override;

        std::string toString() const override;

        int compare( const VersionBase& baseVer ) const override;

        static Version fromString( const std::string& input, char delimeter = '.' );

        static Version InvalidVersion; //< Invalid Version 0.0.-1.-1

    protected:
        int m_patch;
        int m_build;
    };

    /*!
        \brief Simple class for handling app versions in Microsoft specific file and assembly version format.
        Format: Major.Minor.Build.Revision, where all parts are integer numbers.
        In case you use such version format without alphabetical symbols then it fits your needs.
        For example: "1.3.4573.1", "2.13.3735.0"
    */
    class LS_API MSVersion : public VersionBase
    {
    public:
        MSVersion();
        MSVersion( int major, int minor, char delimeter = '.' );
        MSVersion( int major, int minor, int build, char delimeter = '.' );
        MSVersion( int major, int minor, int build, int revision, char delimeter = '.' );
        MSVersion( const std::string& version, char delimeter = '.' );

        int buildVersion() const { return m_build; }
        int revisionVersion() const { return m_revision; }
        bool hasBuildVersion() const { return m_build >= 0; }
        bool hasRevisionVersion() const { return m_revision >= 0; }
        void setBuildVersion( int number );
        void setRevisionVersion( int number );

        size_t length() const override;

        std::string toString() const override;

        int compare( const VersionBase& baseVer ) const override;

        static MSVersion fromString( const std::string& input, char delimeter = '.' );

        static MSVersion InvalidVersion; //< Invalid Version 0.0.-1.-1

    private:
        int m_build;
        int m_revision;
    };

    enum class PrereleaseType : std::uint8_t
    {
        alpha = 0,
        beta  = 1,
        rc    = 2,
        none  = 3,
    };

    /*!
        \brief Extended class for handling app versions, compliant with Semantic Versioning 2.0.0
        Format: Major.Minor.Patch[PrereleaseIdentifier].Build[BuildMetadata], see https://semver.org
        For example: "1.3.4573-alpha.3+sha:dff8df7gdsvnx", "2.13.3735b2.0-stable"
    */
    class LS_API VersionEx : public Version
    {
    public:
        VersionEx();
        VersionEx( int major, int minor, int patch, char delimeter = '.' );
        VersionEx( int major, int minor, int patch, int build, char delimeter = '.' );
        VersionEx( const std::string& version, char delimeter = '.' );

        void setPrereleaseType( PrereleaseType type ) { m_prereleaseType = type; }
        PrereleaseType prereleaseType() const { return m_prereleaseType; }
        bool isPrerelease() const { return !m_prerelease.empty(); }

        const std::string& prereleaseVersion() const { return m_prerelease; }
        void setPrereleaseVersion( const std::string& version ) { m_prerelease = version; }

        const std::string& buildMetadata() const { return m_buildMetadata; }
        void setBuildMetadata( const std::string& data ) { m_buildMetadata = data; }

        std::string toString() const override;

        int compare( const VersionBase& baseVer ) const override;

        static VersionEx fromString( const std::string& input, char delimeter = '.' );

    private:
        PrereleaseType m_prereleaseType;
        std::string    m_prerelease;
        std::string    m_buildMetadata;
    };

}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // LS_CUSTOMER_H
