#ifndef LS_POD_TYPES_H
#define LS_POD_TYPES_H

#ifdef _MSC_VER
#pragma once
#endif

/// \brief Product authorization method
enum LSAuthMethod
{
    AuthMethodUnknown = 0, ///< Invalid method
    AuthMethodKeyBased = 1, ///< Authorization method based on license keys
    AuthMethodUserBased = 2  ///< Authorization method based on user credentials
};


/// \brief Product feature type
enum LSFeatureType
{
    FeatureTypeNone,       ///< Invalid type
    FeatureTypeActivation, ///< Activation feature type
    FeatureTypeConsumption ///< Consumption feature type
};


/// \brief License type
enum LSLicenseType
{
    LicenseTypePerpetual,    ///< Perpetual License type
    LicenseTypeSubscription, ///< Subscription License type
    LicenseTypeTimeLimited,  ///< TimeLimited License type
    LicenseTypeConsumption,  ///< Consumption License type
    LicenseTypeUnknown = 100 ///< Unknown (invalid) License type
};

/// \brief Consumption period
enum LSConsumptionPeriod
{
    ConsumptionPeriodNone = 0, ///< No consumption period, consumption reset disabled
    ConsumptionPeriodDaily,    ///< Daily consumption reset
    ConsumptionPeriodWeekly,   ///< Weekly consumption reset
    ConsumptionPeriodMonthly,  ///< Monthly consumption reset
    ConsumptionPeriodAnnually  ///< Annually consumption reset
};

/// \brief LicenseSpring Error codes enum
enum LSErrorCode
{
    eNoError = 0, ///< No error, everything is ok
    eUnknownError, ///< Unknown error encountered
    eStdError, ///< C++ standard exception encountered
    eGeneralLSError, ///< General LicenseSpring error
    eInitializationError, ///< Invalid Configuration settings
    eLicenseNotFound, ///< License not found
    eLicenseExpired, ///< License Expired
    eLicenseDisabled, ///< License disabled
    eLicenseInactive, ///< Could not activate license
    eActivationFailed, ///< Internal server error
    eSreverError, ///< Internal server error
    eDeviceError, ///< Device from which the call is made is not licensed (Hardware ID mismatch)
    eVersionError, ///< App version not found on the backend
    eProductError, ///< Product not found on the backend
    eNoInternetError, ///< Problems with Internet connection
    eNetworkTimeoutError, ///< Request to the backend has timed out
    eNotEnoughConsumptionError, ///< Consumption license or feature run out of resources
    eProductMismatchError, ///< License product code doesn't correspond to configuration product code
    eSignatureMismatchError, ///< Server signature is not valid
    eLocalLicenseError, ///< SDK could not read or write license to the storage
    eClockTamperedError, ///< Detected that system clock has been set back
    eInvalidCredential, ///< Username does not exist or wrong password provided
    eLicenseSpringInternalError, ///< Internal SDK error, please contact support or try to update SDK
    eMissingEmailError, ///< Missing email in API call for user-based product, probably attempt to get trial license with empty email
    eInvalidOrderIDError, ///< Order with specified ID already exists and ShouldAppend flag set to false
    eInvalidLicenseFeatureError, ///< Feature does not exist or wrong feature type
    eLicenseNoAvailableActivations, ///< The license has already been activated maximum number of times
    eVMNotAllowed, ///< Running on Virtual machine not allowed
    eMaxFloatingReached, ///< Reached max users count for floating license
    eLicenseNotFloating, ///< The license is not floating
    eDeviceBlacklisted, ///< Device has been added to the blacklist by admin on LicenseSpring platform
    ePasswordChangeNotAllowed, ///< Password change is restricted by product policy
    eTrialNotAllowed, ///< Getting trial licenses is restricted by product license policy
    eCannotBeActivatedNow, ///< Current date is behind license start date
    eSSOError, ///< Customer account not found or SSO is not set up properly
    eSSOTokenError, ///< Token for SSO is not valid or expired
    eBorrowLicenseError, ///< Borrowing period is not valid (it is bigger than max possible)
    eBorrowingNotAllowed, ///< License borrowing is not allowed by license policy
    eAuthorizationError, ///< Authorization failed
    eVMDNotAvailable, ///< Windows only, in case VM detection dll not found or tempered
    eInvalidApiKey, ///< Provided api key is not valid
    eReadOnlyApiKey, ///< Provided api key is read_only
    eRevokedApiKey, ///< Provided api key is revoked
    eApiKeyProductNotAllowed, ///< Provided api key does not allow access to configured product
    eFloatingTimeoutExpired ///< Floating license borrowing period or floating timeout expired
};

/// \brief Type of Device ID algorithm used by the SDK
enum DeviceIDAlgorithm
{
    /// \brief Default device ID calculation algorithm
    Default = 0,
    /// \brief Generation 2 of device ID algorithm for Windows
    /// \details Safer and shorter than default. Can be useful for air gap licenses.
    Gen2,
    /// \brief Cloud platforms instance ID algorithm
    /// \details Get instance ID for cloud platforms, supported: Azure, AWS, GCP.
    CloudPlatformsId,
    /// \brief Device ID algorithm with auto-detection of the platform
    /// \details Same as CloudPlatformsId on Azure, AWS, or GCP, otherwise same as Default.
    AutoId,
    /// \brief Alternative device ID algorithm for Windows
    /// \details Get UUID of the Windows machine using Computer System Product Id through WMI, on other platforms same as default.
    WinCSProductId,
    /// \brief Alternative device ID algorithm for Windows
    /// \details Get Microsoft Cryptography MachineGuid from Windows registry, on other platforms same as default.
    WinCryptographyId,
    /// \brief Device ID algorithm with auto-detection of the platform
    /// \details Same as CloudPlatformsId on Azure, AWS, or GCP, otherwise same as Gen2.
    AutoIdGen2
};

#endif // LS_POD_TYPES_H
