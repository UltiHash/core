#ifndef LS_LICENSE_HANDLER_H
#define LS_LICENSE_HANDLER_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Configuration.h"
#include "ProductDetails.h"
#include "License.h"
#include "LicenseStorage.h"
#include "CryptoProvider.h"

namespace LicenseSpring
{
    class LicenseHandlerImpl;

    /*!
    * \brief This class encapsulates functionality of the both LicenseManager and the License.\n
    * \details By default it is exceptions free, but you can enable exceptions.\n
    * It's up to you whether to use LicenseManager and License directly or use this class.\n
    * This class also useful for Unreal Engine clients.\n
    * Because it is free of memory access violation problems due to runtime conflicts with UE.\n
    * \see License and LicenseManager for more details on these methods.
    */
    class LS_API LicenseHandler
    {
    public:

        /// \brief Get singleton object of LicenseHandler
        /// \param globalInit Indicates if curl global initialization should be done first.
        /// Set it to true for the first call if you use floating licensing in order to properly release floating license at the app exit.
        /// Otherwise this param can be safely ignored, as well as if you release floating license manualy.
        /// \warning Automatic floating license releasing may still not work if you link to LicenseSpring dynamically and not do local license check.
        /// In this case we recommend you to release license manualy or you can create you own static object.
        static LicenseHandler& instance( bool globalInit = false );

        /// \brief Constructor for LicenseHandler
        /// \param config Your LicenseSpring configuration
        /// \param storage Optional custom storage implementation for local license
        LicenseHandler( Configuration::ptr_t config = nullptr, LicenseStorage::ptr_t storage = nullptr );
        LicenseHandler( const LicenseHandler& ) = delete;
        LicenseHandler& operator=( const LicenseHandler& ) = delete;
        /// \brief Destructor for LicenseHandler
        ~LicenseHandler();

        /// \see BaseManager::reconfigure
        void reconfigure( Configuration::ptr_t config, LicenseStorage::ptr_t storage = nullptr );
        /// \see BaseManager::currentConfig
        Configuration::ptr_t currentConfig() const;

        /// \see BaseManager::dataLocation
        const std::wstring& dataLocation() const;
        /// \see BaseManager::setDataLocation
        void setDataLocation( const std::wstring& path );
        /// \see BaseManager::licenseFileName
        std::wstring licenseFileName() const;
        /// \see BaseManager::setLicenseFileName
        void setLicenseFileName( const std::wstring& name );
        /// \see BaseManager::licenseFilePath
        std::wstring licenseFilePath() const;

        // ---------- Requests to the LicenseSpring backend ----------

        /// \see LicenseManager::isOnline	
        bool isOnline() const;
        /// \see BaseManager::getProductDetails
        ProductDetails::ptr_t getProductDetails() const;
        /// \see LicenseManager::getTrialLicense
        const LicenseID& getTrialLicense( Customer::ptr_t user = nullptr,
                                          const std::string& licensePolicy = std::string() ) const;
        /// \see LicenseManager::getTrialLicense
        const LicenseID& getTrialLicense( const std::string& userEmail ) const;
        /// \see LicenseManager::getSSOUrl	
        const std::string& getSSOUrl( const std::string& accountCode = std::string(), bool useAuthCode = true );
        /// \see LicenseManager::getAirGapActivationCode	
        const std::string& getAirGapActivationCode( const std::string& initializationCode, const std::string& licenseKey );
        /// \see LicenseManager::verifyConfirmationCode	
        bool verifyConfirmationCode( const std::string& confirmationCode, const std::string& licenseKey, const std::string& policyId );
        /// \see LicenseManager::activateAirGapLicense
        void activateAirGapLicense( const std::wstring& activationResponseFile, const std::string& licenseKey, uint32_t policyId );
        /// \see LicenseManager::getLicenseUsers
        std::vector<LicenseUser::ptr_t> getLicenseUsers( const Customer& customer );
        /// \see LicenseManager::getLicenseUsers
        std::vector<LicenseUser::ptr_t> getLicenseUsers( const std::string& customerEmail );
        /// \see LicenseManager::activateLicense
        void activateLicense( const LicenseID& licenseID );
        /// \see LicenseManager::activateLicense
        void activateLicense( const std::string& authData, const std::string& accountCode = std::string(), bool useAuthCode = true );
        /// \see License::deactivate
        bool deactivateLicense();
        /// \see License::changePassword
        bool changePassword( const std::string& password, const std::string& newPassword,
            const std::string& user = std::string() );
        /// \see LicenseManager::relinkLicense
        void relinkLicense( DeviceIDAlgorithm deviceIdAlgorithm, const std::string& userPassword = std::string() );
        /// \see License::check
        InstallationFile::ptr_t checkLicense( const InstallFileFilter& filter = InstallFileFilter() );
        /// \see LicenseManager::getVersionList	
        const std::vector<std::string>& getVersionList( const LicenseID& licenseID = LicenseID(),
                                                        const InstallFileFilter& filter = InstallFileFilter() );
        /// \see LicenseManager::getInstallationFile
        InstallationFile::ptr_t getInstallFile( const LicenseID& licenseID = LicenseID(),
                                                const std::string& version = std::string(),
                                                const InstallFileFilter& filter = InstallFileFilter() );
        /// \see License::sendDeviceVariables
        void sendDeviceVariables();
        /// \see License::getDeviceVariables
        std::vector<DeviceVariable> getDeviceVariables( bool getFromBackend = false );
        /// \see License::syncConsumption
        void syncConsumption( int32_t requestOverage = -1 );
        /// \see License::syncFeatureConsumption
        void syncFeatureConsumption( const std::string& featureCode = std::string() );
        /// \see License::registerFloatingLicense
        void registerFloatingLicense();
        /// \see License::borrow	
        void borrowFloatingLicense( uint32_t hours, uint32_t days = 0 );
        /// \see License::borrow	
        void borrowFloatingLicense( const std::string& borrowEndDateTime = std::string() );
        /// \see License::releaseFloatingLicense
        void releaseFloatingLicense();
        // ------------------------------------------------------------

        // -------- Local license management and helper methods -------
        bool isInitialized() const;
        bool isLicenseExists() const;
        /// \see License::updateConsumption
        void updateConsumption( int32_t value = 1, bool saveLicense = true );
        /// \see License::updateFeatureConsumption
        void updateFeatureConsumption( const std::string& featureCode, int32_t value = 1, bool saveLicense = true );
        /// \see License::localCheck
        void checkLicenseLocal();
        /// \see BaseManager::clearLocalStorage
        void clearLocalStorage(); // remove local license and also all license information from memory

        /// \see License::addUserData
        void addUserData( const CustomField& data, bool saveLicense = true );
        /// \see License::removeUserData
        void removeUserData( const std::string& key = std::string(), bool saveLicense = true );
        /// \see License::userData
        const std::vector<CustomField>& userData();
        /// \see License::userData
        std::string userData( const std::string& key );

        /// \see License::isAutoReleaseSet
        bool isAutoReleaseSet() const;
        /// \see License::setAutoRelease
        void setAutoRelease( bool autoRelease );
        /// \see License::setupLicenseWatchdog
        void setupLicenseWatchdog( LicenseWatchdogCallback callback, uint32_t timeout = 0 );
        /// \see License::resumeLicenseWatchdog
        void resumeLicenseWatchdog();
        /// \see License::stopLicenseWatchdog
        void stopLicenseWatchdog();

        /// \see License::addDeviceVariable
        void addDeviceVariable( const std::string& name, const std::string& value, bool saveLicense = true );
        /// \see License::addDeviceVariable
        void addDeviceVariable( const DeviceVariable& variable, bool saveLicense = true );
        /// \see License::addDeviceVariable
        void addDeviceVariables( const std::vector<DeviceVariable>& variables );
        /// \see License::deviceVariable
        DeviceVariable deviceVariable( const std::string& name ) const;
        /// \see License::deviceVariable
        const std::string& deviceVariableValue( const std::string& name ) const;
        // ------------------------------------------------------------

        // ---------------- Offline license management ----------------
        /// \see LicenseManager::createOfflineActivationFile
        const std::wstring& createOfflineActivationFile( const LicenseID& licenseID,
            const std::wstring& activationRequestFile = std::wstring() );
        /// \see LicenseManager::activateLicenseOffline
        void activateLicenseOffline( const std::wstring& activationResponseFile = std::wstring() );
        /// \see License::deactivateLiceneseOffline
        const std::wstring& deactivateLicenseOffline( const std::wstring& deactivationRequestFile = std::wstring() );
        /// \see License::updateOffline
        bool updateOffline( const std::wstring& path = std::wstring(), bool resetConsumption = false );
        // ------------------------------------------------------------

        // ----------------------- License data -----------------------
        /// \see License::isValid
        bool isLicenseValid() const;
        /// \see License::isTrial
        bool isLicenseTrial() const;
        /// \see License::isActive
        bool isLicenseActive() const;
        /// \see License::isEnabled
        bool isLicenseEnabled() const;
        /// \see License::isExpired
        bool isLicenseExpired() const;
        /// \see License::isMaintenancePeriodExpired
        bool isLicenseMaintenanceExpired() const;
        /// \see License::isOfflineActivated
        bool isLicenseOfflineActivated() const;
        /// \see License::isFloating
        bool isLicenseFloating() const;
        /// \see License::isBorrowed
        bool isLicenseBorrowed() const;
        /// \see License::isOveragesAllowed
        bool isLicenseOveragesAllowed() const;
        /// \see License::isUnlimitedConsumptionAllowed
        bool isLicenseUnlimitedConsumptionAllowed() const;
        /// \see License::isVMAllowed
        bool isLicenseVMAllowed() const;
        /// \see License::isGracePeriodStarted
        bool isLicenseGracePeriodStarted() const;

        /// \see License::id
        const LicenseID& getLicenseID() const;
        /// \see License::key
        const std::string& getLicenseKey() const;
        /// \see License::user
        const std::string& getLicenseUser() const;
        /// \see License::status
        const std::string& getLicenseStatusStr() const;
        /// \see License::startDate
        const std::string& getLicenseStartDate() const;

        /// \see License::type
        LicenseType getLicenseType() const;

        /// \see License::validityPeriod
        tm getLicenseExpiryDate() const;
        /// \see License::validityPeriodUtc
        tm getLicenseExpiryDateUtc() const;
        /// \see License::maintenancePeriod
        tm getLicenseMaintenancePeriod() const;
        /// \see License::maintenancePeriodUtc
        tm getLicenseMaintenancePeriodUtc() const;
        /// \see License::lastCheckDate
        tm getLicenseLastCheckDate() const;
        /// \see License::lastCheckDateUtc
        tm getLicenseLastCheckDateUtc() const;
        /// \see License::floatingEndDateTime
        tm getLicenseFloatingEndDateTime() const;
        /// \see License::floatingEndDateTimeUtc
        tm getLicenseFloatingEndDateTimeUtc() const;
        /// \see License::daysRemaining
        int getDaysRemaining() const;
        /// \see License::maintenanceDaysRemaining
        int getMaintenanceDaysRemaining() const;
        /// \see License::daysPassedSinceLastCheck
        int getDaysPassedSinceLastCheck() const;

        /// \see License::gracePeriod
        uint32_t getLicenseGracePeriod() const;
        /// \see License::validityWithGracePeriod
        tm validityWithGracePeriod() const;
        /// \see License::validityWithGracePeriodUtc
        tm validityWithGracePeriodUtc() const;

        /// \see License::owner
        const Customer& getLicenseOwner() const;
        /// \see License::licenseUser
        LicenseUser::ptr_t getLicenseUserPtr() const;

        bool hasLicenseFeatures() const;
        /// \see License::feature
        const LicenseFeature& getLicenseFeature( const std::string& featureCode ) const;
        /// \see License::features
        const std::vector<LicenseFeature>& getLicenseFeatures() const;

        bool hasLicenseCustomFields() const;
        /// \see License::customFields
        const std::vector<CustomField>& getLicenseCustomFields() const;

        /// \see License::timesActivated
        uint32_t getLicenseTimesActivated() const;
        /// \see License::maxActivations
        uint32_t getLicenseMaxActivations() const;
        /// \see License::transferCount
        uint32_t getLicenseTransferCount() const;

        // Meaningful only for Consumption license
        /// \see License::totalConsumption
        int32_t getLicenseTotalConsumption() const;
        /// \see License::maxConsumption
        int32_t getLicenseMaxConsumption() const;
        /// \see License::maxOverages
        int32_t getLicenseMaxOverages() const;
        bool isResetLicenseConsumptionEnabled() const;
        ConsumptionPeriod getLicenseConsumptionPeriod() const;

        // Meaningful only for Floating license
        /// \see License::floatingTimeout
        uint32_t getLicenseFloatingTimeout() const;
        /// \see License::floatingInUseCount
        uint32_t getLicenseFloatingInUseCount() const;
        /// \see License::maxFloatingUsers
        uint32_t getLicenseMaxFloatingUsers() const;
        /// \see License::maxBorrowTime
        uint32_t getMaxBorrowTime() const;
        // ------------------------------------------------------------

        // ---------------------- Error handling ----------------------
        void enableExceptions( bool enable ); // By default this class does not throw exceptions
        bool isExceptionsEnabled() const;
        bool wasError() const;
        LSErrorCode getLastError() const;
        const std::string& getLastErrorMsg() const;
        bool isInitializationError() const;
        bool isInternalServerError() const;
        bool isNetworkTimeoutError() const;
        bool isNoInternetError() const;
        bool isActivationFailed() const;
        bool isLicenseExpiredError() const;
        bool isLicenseNotFoundError() const;
        bool isLicenseStateError() const;
        bool isHardwareIDError() const;
        bool isGeneralError() const;
        bool isLicenseFeatureError() const;
        bool canIgnoreError() const; // No Internet, network timeout and server errors can be ignored
        // ------------------------------------------------------------

    private:
        LicenseHandlerImpl* m_impl;
    };

} // namespace LicenseSpirng

#endif // LS_LICENSE_HANDLER_H
