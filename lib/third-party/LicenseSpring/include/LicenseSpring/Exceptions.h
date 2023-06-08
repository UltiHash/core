#ifndef LS_EXCEPTIONS_H
#define LS_EXCEPTIONS_H

#ifdef _MSC_VER
#pragma once
#pragma warning( push )
#pragma warning( disable : 4275 )
#endif

#include <stdexcept>
#include <string>
#include "APIDef.h"
#include "PODTypes.h"

namespace LicenseSpring
{
    /// \brief Base class for all LicenseSpring exceptions.
    class LS_API LicenseSpringException : public std::runtime_error
    {
    public:
        LicenseSpringException( const std::string& message, LSErrorCode errorCode = LSErrorCode::eGeneralLSError )
            : std::runtime_error( message.c_str() )
            , m_errorCode( errorCode )
        {}
        LSErrorCode getCode() const { return m_errorCode; }
    protected:
        LSErrorCode m_errorCode;
    };


    /// \brief This exception means that SDK cannot reach the server. Because of no internet connection, DNS issues, etc.
    class LS_API NoInternetException : public LicenseSpringException
    {
    public:
        NoInternetException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eNoInternetError ) {}
    };


    /// \brief This exception means that network operation timed out. 
    /// \details See Configuration::setNetworkTimeout() or ExtendedOptions::setNetworkTimeout() for more details.
    class LS_API NetworkTimeoutException : public LicenseSpringException
    {
    public:
        NetworkTimeoutException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eNetworkTimeoutError ) {}
    };


    /// \brief Throws in case of something wrong with Configuration. E.g. wrong API or Shared key.
    class LS_API ConfigurationException : public LicenseSpringException
    {
    public:
        ConfigurationException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eInitializationError ) {}
    };


    /// \brief Can be thrown in case getting trial licenses is restricted by product license policy.
    class LS_API TrialNotAllowedException : public LicenseSpringException
    {
    public:
        TrialNotAllowedException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eTrialNotAllowed ) {}
    };


    /// \brief Throws in case license is in invalid state (disabled, epired or inactive).
    class LS_API LicenseStateException : public LicenseSpringException
    {
    public:
        LicenseStateException( const std::string& message, LSErrorCode errorCode )
            : LicenseSpringException( message, errorCode ) {}
    };


    /// \brief Throws in case floating license timeout or borrowing period is expired and it needs to be registered.
    class LS_API FloatingTimeoutExpiredException : public LicenseStateException
    {
    public:
        FloatingTimeoutExpiredException( const std::string& message )
            : LicenseStateException( message, LSErrorCode::eFloatingTimeoutExpired ) {}
    };


    /// \brief Throws in case computer hardware configuration has been changed or license does not belong to this computer.
    class LS_API DeviceNotLicensedException : public LicenseSpringException
    {
    public:
        DeviceNotLicensedException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eDeviceError ) {}
    };


    /// \brief Throws in case license not found on the server.
    class LS_API LicenseNotFoundException : public LicenseSpringException
    {
    public:
        LicenseNotFoundException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eLicenseNotFound ) {}
    };


    /// \brief Throws in case product not found on the server.
    class LS_API ProductNotFoundException : public LicenseSpringException
    {
    public:
        ProductNotFoundException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eProductError ) {}
    };


    /// \brief Throws in case installation file not found for requested app version.
    class LS_API ProductVersionException : public LicenseSpringException
    {
    public:
        ProductVersionException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eVersionError ) {}
    };


    /// \brief Rare exception, can arise if activation is limited to some device count or license transfer is prohibited.
    /// \details In most cases you can get LicenseStateException,
    /// LicenseNotFoundException, LicenseNoAvailableActivationsException or CannotBeActivatedNowException
    /// if something wrong with license.
    class LS_API LicenseActivationException : public LicenseSpringException
    {
    public:
        LicenseActivationException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eActivationFailed ) {}
    };


    /// \brief Throws when license has already been activated maximum number of times.
    class LS_API LicenseNoAvailableActivationsException : public LicenseActivationException
    {
    public:
        LicenseNoAvailableActivationsException( const std::string& message )
            : LicenseActivationException( message ) { m_errorCode = eLicenseNoAvailableActivations; }
    };


    /// \brief Can be thrown if start date field is set for the license and current date is behind start date
    /// \details See https://docs.licensespring.com/docs/license-start-date
    class LS_API CannotBeActivatedNowException : public LicenseActivationException
    {
    public:
        CannotBeActivatedNowException( const std::string& message )
            : LicenseActivationException( message ) { m_errorCode = eCannotBeActivatedNow; }
    };

    /// \brief Indicates internal server error, please contact support if you face this exception.
    class LS_API LicenseServerException : public LicenseSpringException
    {
    public:
        LicenseServerException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eSreverError ) {}
    };

    /// \brief Authorization failed, please check your proxy settings.
    class LS_API AuthorizationException : public LicenseSpringException
    {
    public:
        AuthorizationException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eAuthorizationError )
        {}
    };

    /// \brief This exceptions means that Consumption license or feature ran out of resources.
    class LS_API NotEnoughConsumptionException : public LicenseSpringException
    {
    public:
        NotEnoughConsumptionException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eNotEnoughConsumptionError ) {}
    };


    /// \brief This exceptions means that license product code doesn't correspond to configuration product code.
    class LS_API ProductMismatchException : public LicenseSpringException
    {
    public:
        ProductMismatchException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eProductMismatchError ) {}
    };


    /// \brief Throws in case signature returned by LicenseSpring server is not valid.
    class LS_API SignatureMismatchException : public LicenseSpringException
    {
    public:
        SignatureMismatchException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eSignatureMismatchError ) {}
    };


    /// \brief This exception means that SDK could not read or write license to the storage.
    class LS_API LocalLicenseException : public LicenseSpringException
    {
    public:
        LocalLicenseException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eLocalLicenseError ) {}
    };


    /// \brief This exception means that detected that system clock has been set back.
    class LS_API ClockTamperedException : public LicenseSpringException
    {
    public:
        ClockTamperedException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eClockTamperedError ) {}
    };


    /// \brief This exception means that user or password is wrong.
    class LS_API InvalidCredentialException : public LicenseSpringException
    {
    public:
        InvalidCredentialException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eInvalidCredential ) {}
    };


    /// \brief Throws when email is missing in API call for user-based product, probably attempt to get trial license without email (user id).
    class LS_API MissingEmailException : public LicenseSpringException
    {
    public:
        MissingEmailException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eMissingEmailError ) {}
    };


    /// \brief Throws when order with specified ID already exists and ShouldAppend flag set to false.
    class LS_API InvalidOrderIDException : public LicenseSpringException
    {
    public:
        InvalidOrderIDException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eInvalidOrderIDError ) {}
    };


    /// \brief Throws when product feature does not exist or wrong feature type
    class LS_API InvalidLicenseFeatureException : public LicenseSpringException
    {
    public:
        InvalidLicenseFeatureException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eInvalidLicenseFeatureError ) {}
    };


    /// \brief Please, contact support or try to update SDK if you got this exception. Normally should never happen.
    class LS_API LicenseSpringInternalException : public LicenseSpringException
    {
    public:
        LicenseSpringInternalException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eLicenseSpringInternalError ) {}
    };


    /// \brief In case LicenseSpring backend returns unexpected error or something went wrong.
    /// \details Rare exception, please contact LicenseSpring support if you faced it. Normally should never happen.
    class LS_API UnknownLicenseSpringException : public LicenseSpringException
    {
    public:
        UnknownLicenseSpringException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eUnknownError ) {}
    };


    /// \brief This exception can arise on virtual machine when VM detection is enabled but product policy prohibits running on VM.
    class LS_API VMIsNotAllowedException : public LicenseSpringException
    {
    public:
        VMIsNotAllowedException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eVMNotAllowed ) {}
    };

    /// \brief Windows only. In case VM detection dll not found, tempered or detection failed.
    class LS_API VMDNotAvailableException : public LicenseSpringException
    {
    public:
        VMDNotAvailableException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eVMDNotAvailable )
        {
        }
    };

    /// \brief This exception can arise during online check of floating license if reached maximum users count for the license
    class LS_API MaxFloatingReachedException : public LicenseSpringException
    {
    public:
        MaxFloatingReachedException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eMaxFloatingReached ) {}
    };


    /// \brief This exception can arise during floating license registration on floating server
    class LS_API LicenseNotFloatingException : public LicenseSpringException
    {
    public:
        LicenseNotFloatingException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eLicenseNotFloating ) {}
    };


    /// \brief This exception can arise during license check if device has been blacklisted by Product manager
    class LS_API DeviceBlacklistedException : public LicenseSpringException
    {
    public:
        DeviceBlacklistedException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eDeviceBlacklisted ) {}
    };

    /// \brief This exception can arise if password change is restricted by company policy
    /// \details See company settings on LicenseSpring platform
    class LS_API PasswordChangeNotAllowedException : public LicenseSpringException
    {
    public:
        PasswordChangeNotAllowedException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::ePasswordChangeNotAllowed ) {}
    };

    /// \brief This exception can arise if the backend cannot create URL for Single sign-on (SSO)
    /// \details Reasons can be different, for example if customer account cannot be found, SSO not enabled or have not been set up.
    class LS_API SSOException : public LicenseSpringException
    {
    public:
        SSOException( const std::string& message )
            : LicenseSpringException( message, LSErrorCode::eSSOError ) {}
    };

    /// \brief This exception can arise if token for Single sign-on (SSO) is invalid or expired
    class LS_API SSOTokenException : public SSOException
    {
    public:
        SSOTokenException( const std::string& message )
            : SSOException( message ) { m_errorCode = eSSOTokenError; }
    };

    /// \brief This exception can arise if provided borrowing period bigger than max possible.
    /// Or if license borrowing is not allowed.
    class LS_API BorrowLicenseException : public LicenseSpringException
    {
    public:
        BorrowLicenseException( const std::string& message, LSErrorCode errorCode )
            : LicenseSpringException( message, errorCode ) {}
    };

    /// \brief This exception can arise if provided api key cannot be used for the request.
    class LS_API ApiKeyException : public LicenseSpringException
    {
    public:
        ApiKeyException( const std::string& message, LSErrorCode errorCode )
            : LicenseSpringException( message, errorCode ) {}
    };
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // LS_EXCEPTIONS_H
