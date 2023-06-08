#ifndef LS_LICENSE_MANAGER_H
#define LS_LICENSE_MANAGER_H

#ifdef _MSC_VER
#pragma once
#pragma warning( push )
#pragma warning( disable : 4251 )
#endif

#include "BaseManager.h"

namespace LicenseSpring
{
    /// \brief Class for performing basic requests to LicenseSpring backend and managing licenses.
    /// \details Used mainly for activating licenses (online/offline/trial). Also useful for getting the SSO URL, changing a user's password, 
    /// and getting other information such as list of versions for a product, installation files, and license users.
    class LS_API LicenseManager : public BaseManager
    {
    public:
        using ptr_t = std::shared_ptr<LicenseManager>;

        /// \brief Creates LicenseManager object and initialize it with given configuration.
        /// \param config Configuration object to use
        /// \param storage Storage for local license, by default local license is saved in a file
        /// \throw ConfigurationException If config is null
        /// \return Shared pointer to LicenseManager object.
        static LicenseManager::ptr_t create( Configuration::ptr_t config, LicenseStorage::ptr_t storage = nullptr );

        /// \brief Construct LicenseManager object and initialize it with given configuration.
        /// \throw ConfigurationException If config is null
        LicenseManager( Configuration::ptr_t config, LicenseStorage::ptr_t storage = nullptr );

        /// \brief Destroy license manager and its configuration if there is no more references to the configuration.
        ~LicenseManager() override;

        /// \brief Request trial license from LicenseSpring backend.
        /// \details If you provide user (customer) information please note that user's email is required field.\n
        /// Furthermore, if the initial password for the corresponding user's email has been changed, the license identifier
        /// returned will have an empty string for a password.\n
        /// If user is null you will get anonymous trial (available only for key-based products).
        /// \param user Customer information. Optional param for key-based products, but required for user-based products.
        /// \param licensePolicy Optional license policy code, if omited the default license policy for the product will be used.
        /// \return Trial license identifier ready for activation.
        /// \throw TrialNotAllowedException In case getting trial is not allowed by product license policy
        /// \throw ProductNotFoundException In case configured product does not exist
        /// \throw NoInternetException If cannot reach the server
        /// \throw NetworkTimeoutException In case of network timeout
        /// \throw LicenseServerException In case of internal server error
        /// \throw UnknownLicenseSpringException In rare case if something went wrong, for example unexpected server response
        /// \warning If the initial password for the corresponding user's email has been changed, the license identifier
        /// returned will have an empty string for a password. The password will need to be added manually.\n
        /// \note There are multiple methods with the same name and different parameters.
        LicenseID getTrialLicense( Customer::ptr_t user = nullptr, const std::string& licensePolicy = std::string() );

        /// \brief Request trial license from LicenseSpring backend.
        /// \param userEmail User ID, usually email address
        /// \return Trial license identifier ready for activation.
        /// \throw TrialNotAllowedException In case getting trial is not allowed by product license policy
        /// \throw ProductNotFoundException In case configured product does not exist
        /// \throw NoInternetException If cannot reach the server
        /// \throw NetworkTimeoutException In case of network timeout
        /// \throw LicenseServerException In case of internal server error
        /// \throw UnknownLicenseSpringException In rare case if something went wrong, please contact support
        /// \warning If the initial password for the corresponding user's email has been changed, the license identifier
        /// returned will have an empty string for a password. The password will need to be added manually.\n
        /// \note There are multiple methods with the same name and different parameters.
        LicenseID getTrialLicense( const std::string& userEmail );

        /// \brief Get URL for Single sign-on license activation.
        /// \details You can omit accountCode param if you already set customer account code through the Configuration.
        /// \param accountCode Customer account code
        /// \param useAuthCode Boolean value indicating whether this method should create the redirect_uri with auth code instead of id_token.
        /// \return URL for Single sign-on.
        /// \throw SSOException In case customer was not found or SSO is not enabled
        /// \throw ProductNotFoundException In case configured product does not exist
        /// \throw NoInternetException If cannot reach the server
        /// \throw NetworkTimeoutException In case of network timeout
        /// \throw LicenseServerException In case of internal server error
        /// \throw UnknownLicenseSpringException In rare case if something went wrong, please contact support
        std::string getSSOUrl( const std::string& accountCode = std::string(), bool useAuthCode = true );

        /// \brief Get Activation code for air gap activation.
        /// \param initializationCode Code received from the air gap activation portal.
        /// \param licenseKey License key of air gap license.
        /// \return Air gap Activation code. Empty string in case of error.
        std::string getAirGapActivationCode( const std::string& initializationCode, const std::string& licenseKey );

        /// \brief Verify Confirmation code for air gap activation.
        /// \param confirmationCode Code received from the air gap activation portal.
        /// \param licenseKey License key of air gap license.
        /// \param policyId Policy id received from the air gap activation portal.
        /// \return The result of Confirmation code verification.
        bool verifyConfirmationCode( const std::string& confirmationCode, const std::string& licenseKey, const std::string& policyId );

        /// \brief Activate air gap license using policy file.
        /// \param policyFile License policy file path.
        /// \param licenseKey License key.
        /// \param policyId Id of the license policy.
        /// \return License if activation succeeded.
        /// \throw SignatureMismatchException In case signature inside activation file is not valid.
        /// \throw LicenseActivationException If invalid policy file provided.
        const License::ptr_t activateAirGapLicense( const std::wstring& policyFile, const std::string& licenseKey, uint32_t policyId );

        /// \brief Activate license with given LicenseID.
        /// \details Send activate license request to the backend, creates a local license file if succesful.
        /// \param licenseID License identifier that needs to be activated
        /// \return Shared pointer to License if activation succeeded, throws exceptions in case of errors.
        /// \throw InvalidCredentialException If user and password combination is incorrect
        /// \throw ProductNotFoundException In case configured product does not exist
        /// \throw LicenseNotFoundException In case provided license was not found
        /// \throw LicenseStateException If license is disabled or expired
        /// \throw LicenseNoAvailableActivationsException If license was already activated maximum number of times
        /// \throw CannotBeActivatedNowException If start_date field is set for the license and current date is behind start date
        /// \throw SignatureMismatchException In case signature returned by LicenseSpring server is not valid
        /// \throw NoInternetException If cannot reach the server
        /// \throw NetworkTimeoutException In case of network timeout
        /// \throw LicenseServerException In case of internal server error
        /// \throw UnknownLicenseSpringException In rare case if something went wrong, please contact support
        /// \note There are multiple methods with the same name and different parameters.
        const License::ptr_t activateLicense( const LicenseID& licenseID );

        /// \brief Activate license using SSO.
        /// \details You can omit accountCode param if you already set customer account code through the Configuration.
        /// \param authData id_token or code you get after user authorization with SSO
        /// \param accountCode Customer account code
        /// \param useAuthCode Whether to use auth code instead of id_token for activation
        /// \return Shared pointer to License if activation succeeded, throws exceptions in case of errors.
        /// \throw SSOException In case customer was not found or SSO is not enabled
        /// \throw SSOTokenException In case SSO token is invalid or expired
        /// \throw ProductNotFoundException In case configured product does not exist
        /// \throw LicenseNotFoundException In case provided license was not found
        /// \throw LicenseStateException If license is disabled or experid
        /// \throw LicenseNoAvailableActivationsException If license was already activated maximum number of times
        /// \throw CannotBeActivatedNowException If start_date field is set for the license and current date is behind start date
        /// \throw SignatureMismatchException In case signature returned by LicenseSpring server is not valid
        /// \throw NoInternetException If cannot reach the server
        /// \throw NetworkTimeoutException In case of network timeout
        /// \throw LicenseServerException In case of internal server error
        /// \throw UnknownLicenseSpringException In rare case if something went wrong, please contact support
        /// \note There are multiple methods with the same name and different parameters.
        const License::ptr_t activateLicense( const std::string& authData, const std::string& accountCode = std::string(),
                                              bool useAuthCode = true );

        /// \brief Activate license offline with provided activation file.
        /// \details Default location for activation files is Desktop location, default file name is 'ls_activation.lic'.
        /// \param activationResponseFile Activation response file path, empty means use default location and file name
        /// \return Shared pointer to License if activation succeeded or null if file not found.
        /// \throw SignatureMismatchException In case signature inside activation file is not valid or license does not belong to current device.
        /// \throw LocalLicenseException If invalid activation file provided
        /// \throw ProductMismatchException If license product code does not correspond to configuration product code
        /// \throw DeviceNotLicensedException If license file does not belong to current device.
        const License::ptr_t activateLicenseOffline( const std::wstring& activationResponseFile = std::wstring() );

        /// \brief This method helps upgrade to new device id algorithm. It deactivates current license with old device id and activate with new one. Internet connection is required.
        /// \details Local license is required to call this method. May throw exceptions the same as ActivateLicense and License::deactivate methods.
        /// \param deviceIdAlgorithm Device id algorithm generation you would like to upgrade to
        /// \param userPassword Optional param, user password, meaningful only for user-based licenses
        /// \return License if reactivation succeeded
        /// \throw ProductNotFoundException Thrown when there is no local license exists
        /// \throw ProductMismatchException If license product code does not correspond to configuration product code
        /// \throw DeviceNotLicensedException If reactivation is not possible (e.g. license copied from other device)
        const License::ptr_t relinkLicense( DeviceIDAlgorithm deviceIdAlgorithm, const std::string& userPassword = "" );

        /// \brief Change user password, meaningful only for user-based products.
        /// \param user User ID, usually email in lowercase
        /// \param password User password
        /// \param newPassword New password
        /// \return True if successfully changed.
        /// \throw PasswordChangeNotAllowedException If changing password is forbidden
        /// \throw InvalidCredentialException If user or password is wrong
        /// \throw NoInternetException If cannot reach the server
        /// \throw NetworkTimeoutException In case of network timeout
        /// \throw LicenseServerException In case of internal server error
        /// \throw UnknownLicenseSpringException In rare case if something went wrong, please contact support
        bool changePassword( const std::string& user, const std::string& password, const std::string& newPassword );

        /// \brief Get app version list available for given license.
        /// \param licenseID License identifier
        /// \param filter Optional, you can filter install files by environment and channel
        /// \return List of versions available for the license.
        /// \throw ProductNotFoundException In case configured product does not exist
        /// \throw LicenseNotFoundException In case provided license was not found
        /// \throw LicenseStateException If license is disabled
        /// \throw NoInternetException If cannot reach the server
        /// \throw NetworkTimeoutException In case of network timeout
        /// \throw LicenseServerException In case of internal server error
        /// \throw UnknownLicenseSpringException In rare case if something went wrong, please contact support
        std::vector<std::string> getVersionList( const LicenseID& licenseID,
                                                 const InstallFileFilter& filter = InstallFileFilter() );

        /// \brief Get InstallationFile information for given license.
        /// \param licenseID License identifier
        /// \param version Desired product (app) version, optional, if not provided then last available version assumed
        /// \param filter Optional, you can filter install files by environment and channel
        /// \return Shared pointer to InstallationFile object.
        /// \throw ProductNotFoundException In case configured product does not exist
        /// \throw ProductVersionException If installation file not found for requested app version
        /// \throw LicenseNotFoundException In case provided license was not found
        /// \throw LicenseStateException If license is disabled
        /// \throw NoInternetException If cannot reach the server
        /// \throw NetworkTimeoutException In case of network timeout
        /// \throw LicenseServerException In case of internal server error
        /// \throw UnknownLicenseSpringException In rare case if something went wrong, please contact support
        InstallationFile::ptr_t getInstallationFile( const LicenseID& licenseID,
                                                     const std::string& version = std::string(),
                                                     const InstallFileFilter& filter = InstallFileFilter() );

        /// \brief Creates offline activation file request.
        /// \param licenseID License identifier
        /// \param activationRequestFile Activation request file path, null means use default path (Desktop)/ls_activation.req
        /// \return Path to offline activation request file.
        std::wstring createOfflineActivationFile( const LicenseID& licenseID,
            const std::wstring& activationRequestFile = std::wstring() );

        /// \brief Helper method, it checks connection to the LicenseSpring backend.
        /// \param throwExceptions Indicates whether this method should throw exceptions
        /// \return True if SDK is able to connect to the LicenseSpring backend and false otherwise.
        /// \throw NoInternetException If cannot reach the server
        /// \throw NetworkTimeoutException In case of network timeout
        /// \throw LicenseServerException In case of internal server error
        /// \throw UnknownLicenseSpringException In rare case if something went wrong, please contact support
        bool isOnline( bool throwExceptions = false ) const;

        /// \brief Get license users list by given customer (license owner).
        /// \see https://docs.licensespring.com/docs/apiv4customerlicenseusers
        /// \param customer Customer who ordered the license(s)
        /// \return List of license users for given customer.
        /// \throw ProductNotFoundException In case configured product does not exist
        /// \throw NoInternetException If cannot reach the server
        /// \throw NetworkTimeoutException In case of network timeout
        /// \throw LicenseServerException In case of internal server error
        /// \throw UnknownLicenseSpringException In rare case if something went wrong, please contact support
        /// \note There are multiple methods with the same name and different parameters.
        std::vector<LicenseUser::ptr_t> getLicenseUsers( const Customer& customer );

        /// \brief Overloaded method. See notes above. Get license users list by given customer email.
        /// \param customerEmail Customer email
        /// \return List of license users for given customer email.
        /// \note There are multiple methods with the same name and different parameters.
        std::vector<LicenseUser::ptr_t> getLicenseUsers( const std::string& customerEmail );

    };
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // LS_LICENSE_MANAGER_H
