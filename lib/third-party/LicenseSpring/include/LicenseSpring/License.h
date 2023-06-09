#ifndef LS_LICENSE_H
#define LS_LICENSE_H

#ifdef _MSC_VER
#pragma once
#pragma warning( push )
#pragma warning( disable : 4251 )
#endif

#include <vector>
#include <functional>
#include "LicenseType.h"
#include "CustomField.h"
#include "InstallationFile.h"
#include "LicenseID.h"
#include "LicenseFeature.h"
#include "DeviceVariable.h"
#include "Customer.h"
#include "LicenseUser.h"
#include "Exceptions.h"

#pragma push_macro("check") // Workaround for Unreal Engine
#undef check

namespace LicenseSpring
{
    /// \brief Callback function which being called in case of error in background thread (watchdog thread).
    using LicenseWatchdogCallback = std::function<void( const LicenseSpringException& ex )>;

    /// \brief Class that encapsulates license information
    /// \details Used for checking, editing, and deleting characteristics of licenses
    class LS_API License
    {
    public:
        using ptr_t = std::shared_ptr<License>;

        /// \brief Destructor for license
        virtual ~License() {};

        /// \brief Getter for license ID.
        /// \return license ID object.
        virtual const LicenseID& id() const = 0;

        /// \brief Getter for license key.
        /// \return String of license key.
        virtual const std::string& key() const = 0;

        /// \brief Getter for user.
        /// \return String of user.
        virtual const std::string& user() const = 0;

        /// \brief Getter for license type.
        /// \return LicenseType object.
        virtual LicenseType type() const = 0;

        /// \brief Getter for license owner.
        /// \return Information about license owner, commonly it's a person or organization associated with the license.
        virtual Customer owner() const = 0;

        /// \brief Getter for license user, it's a person assigned to the license.
        /// \details Currently this method make sense only for user-based licenses.
        /// \return Information about license owner, commonly it's a person or organization associated with the license.
        virtual LicenseUser::ptr_t licenseUser() const = 0;

        /// \brief Getter for license status.
        /// \return String of license status, either 'Active', 'Disabled', 'Expired' or 'Inactive'.
        virtual std::string status() const = 0;

        /// \brief Checker for if the license is active.
        /// \return True if active, false if not.
        virtual bool isActive() const = 0;

        /// \brief Checker for if the license is enabled.
        /// \return True if enabled, false if not.
        virtual bool isEnabled() const = 0;

        /// \brief Checker for if the license is valid.
        /// \return True if isActive() && isEnabled() && !isExpired() are all true, false if not.
        virtual bool isValid() const = 0;

        /// \brief Checker for if the license is a trial.
        /// \return True if it is a trial, false if not.
        virtual bool isTrial() const = 0;

        /// \brief Checker for if license is offline activated.
        /// \return True if offline activated, false if not.
        virtual bool isOfflineActivated() const = 0;

        /// \brief Checker for whether the license is allowed to work under virtual machine or not.
        /// \details See product and license settings on the LicenseSpring platform for more details.
        /// \return True if VM is allowed, false if not.
        virtual bool isVMAllowed() const = 0;

        /// \brief Checker for whether the license is floating or not.
        /// \return True if floating, false if not.
        virtual bool isFloating() const = 0;

        /// \brief Checker for whether the license is borrowed.
        /// \details Meaningful only for Floating license, see License::borrow
        /// \return True if borrowed, false if not.
        virtual bool isBorrowed() const = 0;

        /// \brief Check if grace period started for subscription license.
        /// \details Currently grace period meaningful only for Subscription licenses.
        /// \return Returns true if grace period started and false otherwise.
        virtual bool isGracePeriodStarted() const = 0;

        /// \brief Getter for max simultaneous license users (devices or instances).
        /// \details Meaningful only for floating license.
        /// \return Integer of max simutaneous license users.
        virtual uint32_t maxFloatingUsers() const = 0;

        /// \brief Getter for current floatings slots in use count, including this user (instance).
        /// \details Meaningful only for Floating license,
        /// please query this value only after online license check or registration (for Offline floating).
        /// \return Integer of current floating slots in use.
        virtual uint32_t floatingInUseCount() const = 0;

        /// \brief Getter for timeout of the floating license in minutes.
        /// \return Integer of timeout of the floating license in minutes.
        virtual uint32_t floatingTimeout() const = 0;
        
        /// \brief Getter for floating user (client) or instance id.
        /// \details Meaningful only for offline floating license.
        /// \return String of floating client identifier that was set during registration, see FloatingClient::register.
        virtual const std::string& floatingClientId() const = 0;

        /// \brief Getter for license validity period in local time.
        /// \return Time structure of license validity period in local time.
        virtual tm validityPeriod() const = 0;

        /// \brief Getter for license validity period in UTC.
        /// \return Time structure of validity period in UTC.
        virtual tm validityPeriodUtc() const = 0;

        /// \brief Getter for license validity period including grace period in local time.
        /// \details Currently grace period meaningful only for Subscription licenses.
        /// \return Time structure of license validity period in local time.
        virtual tm validityWithGracePeriod() const = 0;

        /// \brief Getter for license validity period including grace period in UTC.
        /// \details Currently grace period meaningful only for Subscription licenses.
        /// \return Time structure of validity period in UTC.
        virtual tm validityWithGracePeriodUtc() const = 0;

        /// \brief Getter for grace period of the Subscription license in hours.
        /// \details For other type of licenses or if grace period is not set it returns 0.
        /// \return Unsigned integer of grace period of the Subscription license in hours.
        virtual uint32_t gracePeriod() const = 0;

        /// \brief Maximum borrowing period in hours.
        /// \details Meaningful only for floating license.
        /// \return Max borrow time or 0 if license borrowing is not allowed.
        virtual uint32_t maxBorrowTime() const = 0;

        /// \brief Getter for license maintenance period in local time.
        /// \return Time structure of the maintenance period in local time.
        virtual tm maintenancePeriod() const = 0;

        /// \brief Getter for license maintenance period in UTC.
        /// \return Time structure of the maintenance period in UTC.
        virtual tm maintenancePeriodUtc() const = 0;

        /// \brief Getter for license last check date in local time.
        /// \return Time structure of the last check date in local time.
        virtual tm lastCheckDate() const = 0;

        /// \brief Getter for license last check date in UTC.
        /// \return Time structure of the last check date in UTC.
        virtual tm lastCheckDateUtc() const = 0;

        /// \brief Getter for floating license validity end date time in local time zone.
        /// \details Meaningful only for floating licenses.
        /// \return Time structure representing end datetime of the floating license in local time.
        virtual tm floatingEndDateTime() const = 0;

        /// \brief Floating license validity end date time in UTC.
        /// \details Meaningful only for floating licenses.
        /// \return Time structure representing end datetime of the floating license.
        virtual tm floatingEndDateTimeUtc() const = 0;

        /// \brief Getter method license start date.
        /// \details See https://docs.licensespring.com/docs/license-start-date
        /// \return License start date in a format %Y-%m-%d, if start date is not set for the License the string is empty.
        virtual const std::string& startDate() const = 0;

        /// \brief Getter method for license feature
        /// \param featureCode Feature code of the feature to get.
        /// \return LicenseFeature structure with the provided feature code.
        virtual LicenseFeature feature( const std::string& featureCode ) const = 0;

        /// \brief Getter for list of license features associated with the license.
        /// \return List of LicenseFeature objects associated with the license.
        virtual std::vector<LicenseFeature> features() const = 0;

        /// \brief Getter for list of custom fields associated with the license.
        /// \return List of CustomField objects associated with the license.
        virtual std::vector<CustomField> customFields() const = 0;

        /// \brief Returns user data stored within license object.
        /// \return Reference to const std::vector of CustomField objects.
        /// \note There are multiple methods with the same name and different parameters.
        virtual const std::vector<CustomField>& userData() const = 0;

        /// \brief Getter for user data by given key.
        /// \param key Key of data field to search.
        /// \return Data field value or empty string if provided key was not found.
        /// \note There are multiple methods with the same name and different parameters.
        virtual std::string userData( const std::string& key ) const = 0;

        /// \brief Adds user data to the license object. This method allows you to store custom data securely inside license file.
        /// \param data Key/value entry to be saved.
        /// \param saveLicense Optional parameter indicating whether license should be saved to the file after modification.
        virtual void addUserData( const CustomField& data, bool saveLicense = true ) = 0;

        /// \brief Removes user data from the license.
        /// \param key Key of data field that should be removed, if key is empty this method clears entire user data.
        /// \param saveLicense Optional parameter indicating whether license should be saved to the file after modification.
        virtual void removeUserData( const std::string& key = std::string(), bool saveLicense = true ) = 0;

        /// \brief Returns license current total consumption.
        /// \details Meaningful only for Consumption license type.
        virtual int32_t totalConsumption() const = 0;

        /// \brief Returns license maximum consumption.
        /// \details Meaningful only for Consumption license type.
        virtual int32_t maxConsumption() const = 0;

        /// \brief Returns maximum overage allowed for the license.
        /// \details Meaningful only for Consumption license type, returns maximum possible value in case of unlimited overages.
        virtual int32_t maxOverages() const = 0;

        /// \brief Checks if overage is allowed for Consumption license.
        /// \details Meaningful only for Consumption license type.
        virtual bool isOveragesAllowed() const = 0;

        /// \brief Checks if unlimited consumption is allowed for Consumption license.
        /// \details Meaningful only for Consumption license type.
        virtual bool isUnlimitedConsumptionAllowed() const = 0;

        /// \brief Returns period of time after which consumption is reset
        /// \details Meaningful only for Consumption license type
        virtual ConsumptionPeriod consumptionPeriod() const = 0;

        /// \brief Checks if consumption reset is allowed for Consumption license
        /// \details Meaningful only for Consumption license type
        virtual bool isResetConsumptionEnabled() const = 0;

        /// \brief How many times license already been activated.
        /// \return Current number of activations for the license.
        virtual uint32_t timesActivated() const = 0;

        /// \brief How many times license can be activated.
        /// \return Maximum number of activations for the license.
        virtual uint32_t maxActivations() const = 0;

        /// \brief How many times license has been transferred between devices.
        /// \return Transfer count of the license.
        virtual uint32_t transferCount() const = 0;

        /// \brief Returns auto release flag. Meaningful only for floating license.
        /// \details By default this flag is true for floating license.
        /// \return Boolean value indicating if license will be automatically released in the destructor.
        virtual bool isAutoReleaseSet() const = 0;

        /// \brief Sets auto release flag. Meaningful only for floating license.
        /// \param autoRelease Indicates whether SDK should automatically release floating license in the destructor.
        virtual void setAutoRelease( bool autoRelease ) = 0;

        /// \brief Increase or decrease license consumption by given value.
        /// \param value Consumption value to be added, note it can be negative, so you can decrease consumption.
        /// \param saveLicense Controls whether license should be saved to file.
        /// \throw NotEnoughConsumptionException If license run out of resource.
        /// \throw LicenseSpringException If license is not of Consumption type.
        virtual void updateConsumption( int32_t value = 1, bool saveLicense = true ) = 0;

        /// \brief Increase or decrease feature consumption by given value.
        /// \param featureCode Feature code.
        /// \param value Consumption value to be added, note it can be negative, so you can decrease consumption.
        /// \param saveLicense Controls whether license should be saved to file.
        /// \throw NotEnoughConsumptionException If license run out of resource.
        /// \throw LicenseSpringException If license is not of Consumption type.
        virtual void updateFeatureConsumption( const std::string& featureCode, int32_t value = 1, bool saveLicense = true ) = 0;

        // Local checks
        /// \brief Compare system date time with license validity date time
        /// \details This method also returns true if floating period expired for floating license.
        /// \return True if expired (system date time is later than validity date time), false if not.
        virtual bool isExpired() const = 0;

        /// \brief Compare system date time with maintenance period time
        /// \return True if maintenance period expired (system date time is later than maintenance period time), false if not.
        virtual bool isMaintenancePeriodExpired() const = 0;

        /// \brief How many days remaining till license expires.
        /// \details For Subscription license it also counts grace period if set.
        /// \return Integer representing days remaining till license expires in UTC.
        virtual int daysRemainingUtc() const = 0;

        /// \brief Almost the same as UTC counterpart, but days counter will be changed on local midnight.
        /// \details For Subscription license it also counts grace period if set.
        /// \return Integer representing days remaining till license expires in local time.
        virtual int daysRemaining() const = 0;

        /// \brief How many days remaining till end of maintenance days.
        /// \return Integer representing days remaining till end of maintenance days.
        virtual int maintenanceDaysRemaining() const = 0;

        /// \brief Maximum value for daysRemaining, e.g. in case of perpetual license.
        /// \return Integer representing max value for daysRemaining.
        static const int MaxDaysRemainingValue;

        /// \brief How many days passed from last online check.
        /// \return Integer representing days passed since last online check.
        virtual int daysPassedSinceLastCheck() const = 0;

        /// \brief Check license local without connection to the backend
        /// \throw LicenseStateException In case license disabled, inactive or expired
        /// \throw FloatingTimeoutExpiredException In case floating period expired
        /// \throw ProductMismatchException In case license does not belong to configured product
        /// \throw DeviceNotLicensedException In case license does not belong to current computer
        /// \throw VMIsNotAllowedException If currently running on VM but it is not allowed
        /// \throw ClockTamperedException Detected cheating with system clock
        virtual void localCheck() = 0;

        // Requests to LicenseSpring server

        /// \brief Sends deactivate license request to the backend
        /// \param removeLocalData If true this method will remove local license file and folders created by the SDK\n
        /// in other words setting this param true is equivalent to call LicenseManager::clearLocalStorage after deactivation.
        /// \throw LicenseNotFoundException In case provided license was not found, 
        /// for example removed by admin using the platform or management API)
        /// \throw LicenseStateException In case license is disabled or already inactive
        /// \throw DeviceNotLicensedException In case license does not belong to current computer
        /// \throw DeviceBlacklistedException If device has been blacklisted by product manager
        /// \throw NoInternetException If cannot reach the server
        /// \throw NetworkTimeoutException In case of network timeout
        /// \throw LicenseServerException In case of internal server error, the backend returned http error code >= 500
        /// \throw UnknownLicenseSpringException In rare case if something went wrong, please contact support 
        virtual bool deactivate( bool removeLocalData = false ) = 0;

        /// \brief Change password for user based license.
        /// \param password Old password for user.
        /// \param newPassword New password to change to.
        /// \return True if successfully changed and false otherwise.
        virtual bool changePassword( const std::string& password, const std::string& newPassword ) = 0; // may throw exceptions

        /// \brief Online license check, sync the license with the backend, throws exceptions in case of errors.
        /// \param filter Optional, you can filter install files by environment and channel.
        /// \return The most recent InstallationFile available for the license (useful for managing software updates)
        /// \throw LicenseStateException In case license disabled, was reset or expired
        /// \throw DeviceNotLicensedException In case license does not belong to current computer
        /// \throw DeviceBlacklistedException If device has been blacklisted by product manager (for Floating license)
        /// \throw MaxFloatingReachedException If no more floating licenses are available
        /// \throw NoInternetException If cannot reach the server
        /// \throw NetworkTimeoutException In case of network timeout
        /// \throw LicenseServerException In case of internal server error
        /// \throw UnknownLicenseSpringException In rare case if something went wrong, please contact support
        virtual InstallationFile::ptr_t check( const InstallFileFilter& filter = InstallFileFilter() ) = 0;
        
        /// \brief Sync Consumption license with the server
        /// \param requestOverage Optional argument that allows to request consumption overage.\n
        /// If argument value is negative (default) this means do nothing, if value is 0 it means disable consumption overage.\n
        /// If value is positive it means allow overage and set max overage to provided value.
        /// \throw NotEnoughConsumptionException If consumption license run out of resource
        /// \throw DeviceBlacklistedException If device has been blacklisted by product manager (for Floating license)
        /// \throw NoInternetException If cannot reach the server
        /// \throw NetworkTimeoutException In case of network timeout
        /// \throw LicenseServerException In case of internal server error
        /// \throw UnknownLicenseSpringException In rare case if something went wrong, please contact support
        virtual void syncConsumption( int32_t requestOverage = -1 ) = 0;

        /// \brief Sync Consumption Feature(s) with the backend, if code not provided sync all consumption features
        /// \param featureCode Code of the license feature that need to be synchronized.\n
        /// Pass empty string to sync all consumption features (default).
        /// \throw NotEnoughConsumptionException If consumption license feature run out of resource
        /// \throw InvalidLicenseFeatureException If license feature with given code not found or not of consumption type
        /// \throw DeviceBlacklistedException If device has been blacklisted by product manager (for Floating license)
        /// \throw NoInternetException If cannot reach the server
        /// \throw NetworkTimeoutException In case of network timeout
        /// \throw LicenseServerException In case of internal server error
        /// \throw UnknownLicenseSpringException In rare case if something went wrong, please contact support
        virtual void syncFeatureConsumption( const std::string& featureCode = std::string() ) = 0;

        /// \brief Add new or update existing device variable to license daata
        /// \details Variable will be stored in local license,
        /// in order to send device variables to the backend see License::sendDeviceVariables
        /// \param name Device variable name
        /// \param value Device variable value
        /// \param saveLicense Optional parameter indicating whether license should be saved localy after modification
        /// \note There are multiple methods with the same name and different parameters.
        virtual void addDeviceVariable( const std::string& name, const std::string& value,
                                        bool saveLicense = true ) = 0;

        /// \brief Add new or update existing device variable to license daata
        /// \details Variable will be stored in local license,
        /// in order to send device variables to the backend see License::sendDeviceVariables
        /// \param variable Device variable
        /// \param saveLicense Optional parameter indicating whether license should be saved localy after modification
        /// \note There are multiple methods with the same name and different parameters.
        virtual void addDeviceVariable( const DeviceVariable& variable, bool saveLicense = true ) = 0;

        /// \brief Add new or update existing device variables to license daata
        /// \details Variables will be stored in local license
        /// \param variables Device variables
        virtual void addDeviceVariables( const std::vector<DeviceVariable>& variables ) = 0;

        /// \brief Send current device variables list to the backend (see device variables on the platform)
        /// \details This method does nothing if local license does not have any variables.
        /// \throw NoInternetException If cannot reach the server
        /// \throw NetworkTimeoutException In case of network timeout
        /// \throw UnknownLicenseSpringException In rare case if something went wrong
        virtual void sendDeviceVariables() = 0;

        /// \brief Get device variables from the local license or from the backend
        /// \details This method does not throw exceptions if you getting variables list from local license.
        /// \param getFromBackend Indicates if you want to request device variables list from local license or backend
        /// \return Variable list for current device
        /// \throw NoInternetException If cannot reach the server
        /// \throw NetworkTimeoutException In case of network timeout
        /// \throw UnknownLicenseSpringException In rare case if something went wrong
        virtual std::vector<DeviceVariable> getDeviceVariables( bool getFromBackend = false ) = 0;

        /// \brief Get device variable by name
        /// \details Returns empty device variable if the variable does not exist
        /// \return Device variable
        virtual DeviceVariable deviceVariable( const std::string& name ) const = 0;

        /// \brief Get device variable value by name
        /// \param name Device variable name
        /// \return Device variable value or empty string if variable does not exist
        virtual const std::string& deviceVariableValue( const std::string& name ) const = 0;

        /// \brief Setup and run license watchdog, a background thread which periodically checks the license.
        /// \details This is especially useful for floating licenses, but can be used for any.\n
        /// Provided callback will be called in case of errors.\n
        /// Timeout in minutes, 0 means use default (for Floating license it's floatingTimeout, for others - 1 hour).
        virtual void setupLicenseWatchdog( LicenseWatchdogCallback callback, uint32_t timeout = 0 ) = 0;

        /// \brief Resume background thread, you can call this within your callback
        /// in order to resume periodic license checks (registrations).
        virtual void resumeLicenseWatchdog() = 0;

        /// \brief Stops license watchdog background thread.
        /// \details This method does not revoke floating license, just stops the watchdog.
        virtual void stopLicenseWatchdog() = 0;

        /// \brief Register floating license using floatingClientId
        /// \details In order to keep floating license alive you should check in (register) it periodically.
        /// This is equivalent to check license online
        /// See also License::setupLicenseWatchdog. Alternatively you can register floating license by calling License::check.
        /// \throw MaxFloatingReachedException If no more floating licenses are available
        virtual void registerFloatingLicense() = 0;

        /// \brief Revoke floating license, typically should be called at the end of app execution.
        /// \details This API call will also release (return) borrowed license.
        /// \see See also License::setAutoRelease, when set license will be automatically revoked when destroyed.
        /// \throw LicenseStateException If license is invalid, disabled or expired
        /// \throw NoInternetException If cannot reach the server
        /// \throw NetworkTimeoutException In case of network timeout
        /// \throw LicenseServerException In case of internal server error
        /// \throw UnknownLicenseSpringException In rare case if something went wrong, please contact support
        virtual void releaseFloatingLicense( bool throwExceptions = false ) = 0;

        /// \brief Borrow floating license for provided time interval.
        /// \details Meaningful only for floating licenses.\n
        /// If borrowing succeeded watchdog (if set) will be stopped and license auto release flag will be set to false.
        /// During borrow period there is no need to register floating license to keep it alive.
        /// \param hours Count of hours from now you want borrow license for.
        /// \param days Count of days from now you want borrow license for.
        /// \throw NoInternetException If cannot reach the server
        /// \throw NetworkTimeoutException In case of network timeout
        /// \throw BorrowLicenseException If borrowing is not allowed or provided borrowing period is bigger than max.
        /// \note There are multiple methods with the same name and different parameters.
        virtual void borrow( uint32_t hours, uint32_t days = 0 ) = 0;

        /// \brief Borrow floating license till provided end date time.
        /// \details Meaningful only for floating licenses.\n
        /// If borrowing succeeded watchdog (if set) will be stopped and license auto release flag will be set to false.
        /// \param borrowEndDateTime Borrow end date time in UTC in format "%Y-%m-%dT%H:%M:%SZ", for example "2022-05-28T15:30:00Z"
        /// If it's empty the license will be borrowed for max possible period.
        /// \throw NoInternetException If cannot reach the server
        /// \throw NetworkTimeoutException In case of network timeout
        /// \throw BorrowLicenseException If borrowing is not allowed or provided borrowing period is bigger than max.
        /// \note There are multiple methods with the same name and different parameters.
        virtual void borrow( const std::string& borrowEndDateTime = std::string() ) = 0;

        /// \brief Creates offline deactivation file request
        /// \param deactivationRequestFile Deactivation request file path, empty means use default path (Desktop)
        /// \throw LocalLicenseException Thrown if could not read license file or if local license corrupted
        virtual std::wstring deactivateOffline( const std::wstring& deactivationRequestFile = std::wstring() ) = 0;

        /// \brief Update license data accordingly to provided file.
        /// \param path Update file path
        /// \param resetConsumption False by default, set to true to reset consumption.
        /// \return True if license successfully updated and false otherwise
        /// \throw LocalLicenseException Thrown if could not read license file or if local license corrupted.
        /// \throw SignatureMismatchException Thrown if license signature is not valid.
        /// \throw ProductMismatchException If license product code does not correspond to configuration product code
        /// \throw DeviceNotLicensedException If license refresh file does not correspond to current device.
        virtual bool updateOffline( const std::wstring& path, bool resetConsumption = false ) = 0;

        /// \brief Unlinks license from cuurent device ID.
        /// \details Checks if license belongs to this device and deactivates it. May throw exceptions same as deactivate method.
        /// \throw ProductMismatchException If license product code does not correspond to configuration product code
        /// \throw DeviceNotLicensedException If license does not correspond to current device.
        virtual void unlinkFromDevice() = 0;

        /// \brief Checks if license belongs to this DeviceIDAlgorithm.
        /// \param deviceIDAlgorithm DeviceIDAlgorithm value
        virtual bool isLicenseBelongsToThisDevice( DeviceIDAlgorithm deviceIDAlgorithm ) = 0;

        /// \brief Checks if license belongs to any of the DeviceIDAlgorithm values.
        virtual bool checkLicenseBelongsToThisDevice() = 0;

    };

    /// \brief Helper function, converts datetime to string using given format. See strftime reference for format details.
    std::string LS_API TmToString( const tm& dateTime, const std::string& format = std::string( "%d-%m-%Y" ) );

} // namespace LicenseSpring

#pragma pop_macro("check")

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // LS_LICENSE_H
