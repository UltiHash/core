#ifndef LS_CONFIGURATION_H
#define LS_CONFIGURATION_H

#ifdef _MSC_VER
#pragma once
#pragma warning( push )
#pragma warning( disable : 4251 )
#endif

#include <memory>
#include "ExtendedOptions.h"

namespace LicenseSpring
{
    /// \brief Class for configuring application. 
    /// \details Used to store information regarding the options related to the application configuration.
    class LS_API Configuration
    {
    public:
        using ptr_t = std::shared_ptr<Configuration>;

        /// \brief Constructor for internal needs, use Create instead.
        Configuration() = default; 

        /// \brief Creates shared pointer for configuration object.
        /// \details Acts as constructor for configuration class.
        /// \param apiKey Vendor API key (UUID)
        /// \param sharedKey Vendor shared key
        /// \param productCode Product code\n
        /// If you configuring FloatingServer v1.1.9 or earlier provide Product name instead of Product code
        /// \param appName Name of application
        /// \param appVersion %Version of application
        /// \param extendedOptions ExtendedOptions object to attach to configuration
        /// \throw ConfigurationException If API/shared key or product code are empty
        static ptr_t Create( const std::string& apiKey,
            const std::string& sharedKey,
            const std::string& productCode,
            const std::string& appName,
            const std::string& appVersion,
            const ExtendedOptions& extendedOptions = ExtendedOptions() );

        /// \brief Getter for API key (UUID).
        /// \return String of API key (UUID).
        const std::string& getApiKey() const;

        /// \brief Getter for shared key.
        /// \return String of shared key.
        const std::string& getSharedKey() const;

        /// \brief Getter for product code.
        /// \return String of product code.
        const std::string& getProductCode() const;

        /// \brief Getter for application name.
        /// \return String of application name.
        const std::string& getAppName() const;

        /// \brief Getter for application version. 
        /// \return String of application version.
        const std::string& getAppVersion() const;

        /// \brief Returns customer group (account) code that is required for SSO.
        /// \details By default empty. Contact support to get more details implementing SSO.
        /// \return URL for single sign-on.
        const std::string& getCustomerAccount() const;

        /// \brief Set customer group (account) code that is required for SSO.
        /// \details Meaningful for user-based auth only. If you don't plan to provide SSO you can ignore this method.
        /// \param accountCode Customer account code
        void setCustomerAccount( const std::string& accountCode );

        /// \brief Getter for air gap public key.
        /// \return String of air gap public key.
        const std::string& getAirGapPublicKey() const;

        /// \brief Setter for air gap public key.
        /// \param key Air gap public key.
        void setAirGapPublicKey( const std::string& key );

        /// \brief This method is obsolete! Return license file path that was set through ExtendedOptions or empty string.
        /// \details Use BaseManager::dataLocation, BaseManager::licenseFilePath or LicenseFileStorage instead
        /// \return License file path.
        const std::wstring& getLicenseFilePath() const;

        /// \brief Getter for device id.
        /// \details Either returns device id passed by ExtendedOptions or, if not provided, creates a unique id for device.
        /// \return String of device id.
        /// \throw LicenseSpringException General LicenseSpring exception, read warning to see possible reason
        /// \warning Needs at least 2 valid, readable hardware to create the hardware ID. These are your device's CPU, motherboard and disk.\n
        /// May throw an exception if it can only read 1 or less pieces of listed hardware.
        const std::string& getHardwareID();

        /// \brief Setter for device id.
        /// \details You can override device id with your own device id generator.
        /// \param deviceId Device id.
        void setHardwareID( const std::string& deviceId );

        /// \brief Setter for DeviceIDAlgorithm.
        /// \param algorithm new DeviceIDAlgorithm.
        void setDeviceIdAlgorithm( DeviceIDAlgorithm algorithm );

        /// \brief Checker for if collection of network information is enabled in ExtendedOptions.
        /// \return True if enabled, false if not.
        bool isCollectNetworkInfoEnabled() const;

        /// \brief Checker for if logging is enabled in ExtendedOptions.
        /// \return True if enabled, false if not.
        bool isLoggingEnabled() const;

        /// \brief Checker for whether guard file usage is enabled in ExtendedOptions.
        /// \return True if enabled, false if not.
        bool isGuardFileEnabled() const;

        /// \brief Checker for whether SSL certificate verification is enabled.
        /// \details See ExtendedOptions::enableSSLCheck for more details.
        /// \return True if enabled, false if not.
        bool isSSLCheckEnabled() const;

        /// \brief Getter for network timeout.
        /// \details If not previously set through ExtendedOptions, default timeout will be 5 seconds.
        /// \return Network timeout value in seconds.
        long getNetworkTimeout() const;

        /// \brief Setter for network timeout.
        /// \param timeoutValue Timeout given in seconds, 0 means no timeout
        void setNetworkTimeout( long timeoutValue );  

        /// \brief Getter for ProxySettings object.
        /// \return ProxySettings object.
        const ProxySettings& getProxySettings() const;

        /// \brief Getter for LicenseSpring API version.
        /// \return Integer representing LicenseSpring API version.
        uint32_t getLicenseSpringAPIVersion() const;

        /// \brief Getter for LicenseSpring SDK version.
        /// \return String of LicenseSpring SDK version.
        const std::string& getSdkVersion() const;

        /// \brief Getter for Operating System version.
        /// \return String of Operating System version.
        const std::string& getOsVersion() const;

        /// \brief Getter for Host Name.
        /// \return String of Host Name.
        const std::string& getHostName() const; // note: string is UTF-8 encoded

        /// \brief Getter for IP Address.
        /// \return String of IP Address.
        const std::string& getIP() const;

        /// \brief Getter for MAC Address.
        /// \return String of MAC Address.
        const std::string& getMAC() const;

        /// \brief Getter for Network Information.
        /// \return NetworkInfo object.
        const NetworkInfo& getNetworkInfo() const;

        /// \brief Checker for whether Virtual Machine Detection is enabled.
        /// \return True if enabled, false if not.
        bool isVMDetectionEnabled() const;

        /// \brief Checker for whether the user is on a virtual machine.
        /// \details For Windows builds LSVMD.dll should be deployed along with your software if you use VM detection.
        /// \return True if VM detection is enabled and device is currently using a VM, otherwise false.
        /// \throw VMDNotAvailableException Windows only. In case VM detection dll not found, tempered or detection failed.
        bool isVM() const; 

        /// \brief Getter for hypervisor name, calls isVM first if not yet called.
        /// \details Return an empty string if VM detection is not enabled.
        /// \return String of hypervisor name or empty string.
        /// \throw VMDNotAvailableException Windows only. In case VM detection dll not found, tempered or detection failed.
        const std::string& getDetectedVMName() const; 

        /// \brief Getter for crypto provider.
        /// \return Shared pointer for CryptoProvider set by vendor or default CryptoProvider if not set.
        CryptoProvider::ptr_t getCryptoProvider();

        /// \brief Getter for alternate service URL.
        /// \return String of alternate service URL.
        const std::string& getAlternateServiceUrl() const;

        /// \brief Getter for alternate key.
        /// \return String of alternate key.
        const std::vector<int32_t>& getAlternateKey() const;

        /// \brief Returns custom information to be passed along with all API requests.
        /// \details Representing user defined generic field to be passed to the backend.
        /// \return String of user defined info.
        const std::string& getUserInfo() const;

        /// \brief Set custom information to be passed to the backend with all API requests.
        /// \param customInfo User information, aka generic data field to be passed to the backend
        void setUserInfo( const std::string& customInfo );

    private:
        std::string m_apiKey;
        std::string m_sharedKey;
        std::string m_productCode;
        std::string m_customerAccount;
        std::string m_appName;
        std::string m_appVersion;
        std::string m_userInfo;
        std::string m_airGapPublicKey;
        ExtendedOptions m_extOptions;
    };
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // LS_CONFIGURATION_H
