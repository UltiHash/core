#ifndef LS_EXTENDED_OPTIONS_H
#define LS_EXTENDED_OPTIONS_H

#ifdef _MSC_VER
#pragma once
#pragma warning( push )
#pragma warning( disable : 4251 )
#endif

#include <vector>
#include "APIDef.h"
#include "CryptoProvider.h"
#include "PODTypes.h"

namespace LicenseSpring
{
    /// \brief Class for storing a device's network information. 
    /// \see The retrieval of some information can be disabled by using ExtendedOptions::collectNetworkInfo()
    class LS_API NetworkInfo
    {
    public:
        /// \brief Constructor for networkInfo.
        NetworkInfo() = default;

        /// \brief Destructor for networkInfo.
        ~NetworkInfo();

        /// \brief Setter for IP address.
        /// \param ip IP address 
        void setIp( const std::string& ip );

        /// \brief Getter for IP address.
        /// \return String of IP address.
        const std::string& ip() const;

        /// \brief Setter for MAC address.
        /// \param mac MAC address
        void setMac( const std::string& mac );

        /// \brief Getter for MAC address.
        /// \return String of MAC address.
        const std::string& mac() const;

        /// \brief Setter for host name.
        /// \param hostName Host name
        void setHostName( const std::string& hostName );

        /// \brief Getter for host name.
        /// \return UTF-8 encoded string of host name.
        const std::string& hostName() const; 

    private:
        std::string m_localIP;
        std::string m_hostName;
        std::string m_macAddress;
    };

    /// \brief Class for storing settings when using a proxy server.
    class LS_API ProxySettings
    {
    public:

        /// \brief Default constructor for proxy settings.
        ProxySettings() = default;

        /// \brief Constructor for proxy settings.
        /// \param proxy String of proxy URI
        /// \param user String of user
        /// \param password String of password
        ProxySettings( const std::string& proxy, const std::string& user, const std::string& password );

        /// \brief Destructor for proxy settings.
        ~ProxySettings();

        /// \brief Setter for proxy URI.
        /// \param uri String of proxy URI
        void setProxyUri( const std::string& uri );

        /// \brief Getter for proxy URI.
        /// \return String of proxy URI.
        const std::string& getProxyUri() const;

        /// \brief Setter for proxy user.
        /// \param user String for proxy user
        void setUser( const std::string& user );

        /// \brief Getter for proxy user.
        /// \return String of proxy user.
        const std::string& getUser() const;

        /// \brief Setter for password.
        /// \param password String for password
        void setPassword( const std::string& password );

        /// \brief Getter for password.
        /// \return String of password.
        const std::string& getPassword() const;

        /// \brief Checker for if proxy URI is specified (i.e. not empty).
        /// \return True if specified, false if empty.
        bool isValid() const; 

    private:
        std::string m_proxyUri; // e.g. "http://local.example.com:1080"
        std::string m_user;
        std::string m_password;
    };

    /// \brief Class for storing useful settings that the Configuration object will inherit.
    class LS_API ExtendedOptions
    {
    public:

        /// \brief Default constructor for ExtendedOptions.
        ExtendedOptions();

        /// \brief Constructor for ExtendedOptions.
        /// \param licenseFilePath Path to license file (obsolete, see setLicenseFilePath())
        ExtendedOptions( const std::wstring& licenseFilePath );

        /// \brief Constructor for ExtendedOptions.
        /// \param licenseFilePath Path to license file (obsolete, see setLicenseFilePath())
        /// \param hardwareID Hardware ID
        /// \param proxy Proxy settings
        ExtendedOptions( const std::wstring& licenseFilePath,
                         const std::string& hardwareID,
                         const ProxySettings& proxy = ProxySettings() );

        /// \brief Destructor for ExtendedOptions.
        ~ExtendedOptions();

        /// \brief Set license file path.
        /// \details This method is obsolete, use BaseManager::setDataLocation() or LicenseFileStorage instead.
        /// \param filePath Absolute or relative path to the license file
        void setLicenseFilePath( const std::wstring& filePath );

        /// \brief Get license file path.
        /// \details This method is obsolete, see setLicenseFilePath().
        /// \return String of license file path.
        const std::wstring& getLicenseFilePath() const;

        /// \brief Setter for hardware ID.
        /// \param ID Hardware ID 
        void setHardwareID( const std::string& ID );

        /// \brief Get hardware ID.
        /// \return String of hardware ID.
        const std::string& getHardwareID() const;

        /// \brief Get device (hardware) id calculation algorithm.
        /// \return Device id algorithm. By default it is DeviceIDAlgorithm::Default.
        DeviceIDAlgorithm getDeviceIdAlgorithm() const;

        /// \brief Allows to choose device (hardware) id calculation algorithm
        /// \details By default it is DeviceIDAlgorithm::Default for backward compatibility.
        /// \param algorithm Device id algorithm.
        void setDeviceIdAlgorithm( DeviceIDAlgorithm algorithm );

        /// \brief Collect network information.
        /// \details When it's set, SDK collects info on MAC address, local IP, host name and sends this info in the requests to the LicenseSpring platform.\n
        /// When not set, network collecting is disabled by default.
        /// \param collect True to collect info, false to not
        void collectNetworkInfo( bool collect );

        /// \brief Checker for whether collectNetworkInfo() is enabled.
        /// \return True if enabled, false if not.
        bool isCollectNetworkInfoEnabled() const;

        /// \brief Enable logging
        /// \details Enabling logging will create a text file: LicenseSpringLog in the same place your local license file is stored (see BaseManager::setDataLocation).\n
        /// When not set, logging is disabled by default.
        /// \param enable True to enable logging, false to not
        /// \warning It is for developers only so make sure it's disabled in your release builds. You can check to see if it is off using\n
        /// Configuration::isLoggingEnabled.
        void enableLogging( bool enable );

        /// \brief Checker for whether enableLogging() is enabled.
        /// \return True if enabled, false if not.
        bool isLoggingEnabled() const;

        /// \brief Enable guard file.
        /// \details Guard files for protecting offline activation process.\n
        /// When not set, guard file is enabled by default.
        /// \param enable True to enable guard file, false to not
        void enableGuardFile( bool enable );

        /// \brief Checker for whether enableGuardFile() is enabled.
        /// \return True if enabled, false if not.
        bool isGuardFileEnabled() const;

        /// \brief Enable VM detection.
        /// \details Control (enable or disable) detection of Virtual machines.\n
        /// When not set, VM detection is disabled by default.
        /// \param enable True to enable VM detection, false to not
        void enableVMDetection( bool enable );

        /// \brief Checker for whether enableVMDetection() is enabled.
        /// \return True if enabled, false if not.
        bool isVMDetectionEnabled() const;

        /// \brief Enable Enable SSL certificate verification.
        /// \details For more details see https://curl.se/libcurl/c/CURLOPT_SSL_VERIFYPEER.html \n
        /// It is recommended to enable this setting, by default it is disabled.
        /// \param enable True to enable SSL certificate verification, false to not
        void enableSSLCheck( bool enable );

        /// \brief Checker for whether SSL certificate verification is enabled.
        /// \return True if enabled, false if not.
        bool isSSLCheckEnabled() const;

        /// \brief Setter for network timeout.
        /// \details If not set, then default timeout will be 5 seconds.
        /// \param timeout Long for timeout
        void setNetworkTimeout( long timeout );

        /// \brief Getter for network timeout.
        /// \return long of timeout.
        long getNetworkTimeout() const;

        /// \brief Setter for proxy settings.
        /// \details If not set, then proxy servers are not used, by default.
        /// \param settings ProxySettings object 
        void setProxySettings( const ProxySettings& settings );

        /// \brief Getter for proxy settings.
        /// \return ProxySettings object.
        const ProxySettings& getProxySettings() const;

        /// \brief Setter for NetworkInfo.
        /// \param networkInfo NetworkInfo object
        void overrideNetworkInfo( const NetworkInfo& networkInfo );

        /// \brief Getter for network info.
        /// \return NetworkInfo object.
        const NetworkInfo& getNetworkInfo() const;

        /// \brief Setter for CryptoProvider.
        /// \details If not set, then default CryptoProvider will be used.
        /// \param cryptoProvider CryptoProvider object
        void overrideCryptoProvider( CryptoProvider::ptr_t cryptoProvider );

        /// \brief Getter for CryptoProvider.
        /// \return CryptoProvider object.
        CryptoProvider::ptr_t getCryptoProvider();

        /// \brief This method can be used if you are planning to deploy self-hosted server.
        /// \details In most cases should be left blank.\n
        /// In order to connect to the LS FloatingServer set its address, for example "http://localhost:8080"
        /// \param URL Alternate License API URL
        void setAlternateServiceURL( const std::string& URL );

        /// \brief Getter for alternate License API URL.
        /// \return Alternate License API URL.
        const std::string& getAlternateServiceURL() const;

        /// \brief This method can be used if you are planning to deploy self-hosted server.
        /// \details Contact support for more details.
        /// \param key Public key for verification
        /// \note There are multiple methods with the same name and different parameters.
        void setAlternateKey( const std::vector<int32_t>& key );

        /// \brief This method can be used if you are planning to deploy self-hosted server.
        /// \details Contact support for more details.
        /// \param key Public key for verification
        /// \param size Size of key to create vector from
        /// \note There are multiple methods with the same name and different parameters.
        void setAlternateKey( const int32_t* key, std::size_t size );

        /// \brief Getter for alternate License API URL.
        /// \return Alternate License API URL.
        const std::vector<int32_t>& getAlternateKey() const;

    private:
        std::wstring  m_licenseFilePath;
        std::string   m_hardwareID;
        bool          m_collectNetworkInfo; // false by default
        bool          m_enableVMDetection;  // false by default
        bool          m_enableGuardFile;    // true by default
        bool          m_enableSSLCheck;     // false by default
        long          m_networkTimeout;     // in seconds, default is 5 seconds
        ProxySettings m_proxySettings;      // by default doesn't use any proxy
        NetworkInfo   m_networkInfo;
        DeviceIDAlgorithm m_deviceIDAlgorithm;
        CryptoProvider::ptr_t m_cryptoProvider;
        std::string           m_serviceURL;
        std::vector<int32_t>  m_key;
    };

} // namespace LicenseSpring

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // LS_EXTENDED_OPTIONS_H
