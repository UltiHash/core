#ifndef LS_C_LICENSE_HANDLER_H
#define LS_C_LICENSE_HANDLER_H

#include "Configuration.h"
#include "ProductDetails.h"
#include "Customer.h"
#include "InstallationFile.h"
#include "LicenseFeature.h"
#include "LicenseID.h"

#ifdef __cplusplus
extern "C" {
#endif

    LS_API typedef struct LSCustomField
    {
        char* name;
        char* value;
        int nameSize;
        int valueSize;
    } LSCustomField;

    // handler can be casted to LSLicenseHandler*
    LS_API typedef void ( *LSWatchdogCallback )( const char* message, enum LSErrorCode errorCode, void* handler );

    LS_API typedef struct LSLicenseHandler
    {
        // current configuration if present gets deleted automatically
        void ( *reconfigure )( struct LSLicenseHandler* self, struct LSConfiguration* config );

        // ---------- Requests to the LicenseSpring backend ----------
        bool ( *isOnline )( struct LSLicenseHandler* self );
        LSProductDetails* ( *getProductDetails )( struct LSLicenseHandler* self );
        struct LSLicenseID ( *getTrialLicense )( struct LSLicenseHandler* self, LSCustomer* user, const char* licensePolicy );
        struct LSLicenseID ( *getTrialLicenseByEmail )( struct LSLicenseHandler* self, const char* userEmail );
        const char* ( *getSSOUrl )( struct LSLicenseHandler* self, const char* accountCode, bool useAuthCode );
        void ( *activateLicense )( struct LSLicenseHandler* self, struct LSLicenseID licenseID );
        void ( *activateLicenseViaSSO )( struct LSLicenseHandler* self, const char* authData,
                                         const char* accountCode, bool useAuthCode );
        bool ( *deactivateLicense )( struct LSLicenseHandler* self );
        bool ( *changePassword )( struct LSLicenseHandler* self, const char* password, const char* newPassword, const char* user );
        LSInstallationFile* ( *checkLicense )( struct LSLicenseHandler* self );
        LSInstallationFile* ( *checkLicenseEx )(struct LSLicenseHandler* self, const char* channel, const char* env );
        int ( *getVersionList )( struct LSLicenseHandler* self, struct LSLicenseID licenseID, int* lengths, char** versionList );
        int ( *getVersionListEx )( struct LSLicenseHandler* self, struct LSLicenseID licenseID,
            const char* channel, const char* env, int* lengths, char** versionList );
        LSInstallationFile* ( *getInstallFile )( struct LSLicenseHandler* self, struct LSLicenseID licenseID, const char* version );
        LSInstallationFile* ( *getInstallFileEx )( struct LSLicenseHandler* self, struct LSLicenseID licenseID,
            const char* version, const char* channel, const char* env );
        void ( *sendDeviceData )( struct LSLicenseHandler* self, struct LSCustomField* data, int dataSize );
        void ( *syncConsumption )( struct LSLicenseHandler* self );
        void ( *syncFeatureConsumption )( struct LSLicenseHandler* self, const char* featureCode );
        void ( *registerFloatingLicense )( struct LSLicenseHandler* self );
        void ( *borrowFloatingLicense )( struct LSLicenseHandler* self, uint32_t hours, uint32_t days );
        void ( *borrowFloatingLicenseTill )(struct LSLicenseHandler* self, const char* borrowEndDateTime );
        void ( *releaseFloatingLicense )( struct LSLicenseHandler* self );
        const char* ( *getAirGapActivationCode )( struct LSLicenseHandler* self, const char* initializationCode, const char* licenseKey );
        bool ( *verifyConfirmationCode )( struct LSLicenseHandler* self, const char* confirmationCode, const char* licenseKey, const char* policyId );
        // ------------------------------------------------------------

        // -------- Local license management and helper methods -------
        bool ( *isInitialized )( struct LSLicenseHandler* self );
        bool ( *isLicenseExists )( struct LSLicenseHandler* self );
        void ( *updateConsumption )( struct LSLicenseHandler* self, int value, bool saveLicense );
        void ( *updateFeatureConsumption )( struct LSLicenseHandler* self, const char* featureCode, int value, bool saveLicense );
        void ( *checkLicenseLocal )( struct LSLicenseHandler* self );
        void ( *clearLocalStorage )( struct LSLicenseHandler* self );
        void ( *setupLicenseWatchdog )( struct LSLicenseHandler* self, LSWatchdogCallback callback, unsigned long timeout );
        void ( *resumeLicenseWatchdog )( struct LSLicenseHandler* self );
        void ( *stopLicenseWatchdog )( struct LSLicenseHandler* self );
        void ( *relinkLicense )( struct LSLicenseHandler* self, enum DeviceIDAlgorithm deviceIdAlgorithm, const char* userPassword );
        // ------------------------------------------------------------

        // ---------------- Offline license management ----------------
        const wchar_t* ( *createOfflineActivationFile )( struct LSLicenseHandler* self, struct LSLicenseID licenseID, const wchar_t* activationRequestFile );
        void ( *activateLicenseOffline )( struct LSLicenseHandler* self, const wchar_t* activationResponseFile );
        const wchar_t* ( *deactivateLicenseOffline )( struct LSLicenseHandler* self, const wchar_t* deactivationRequestFile );
        bool ( *updateOffline )( struct LSLicenseHandler* self, const wchar_t* path, bool resetConsumption );
        void ( *activateAirGapLicense )( struct LSLicenseHandler* self, const wchar_t* policyFile, const char* licenseKey, uint32_t policyId );
        // ------------------------------------------------------------

        // ----------------------- License data -----------------------
        bool ( *isLicenseValid )( struct LSLicenseHandler* self );
        bool ( *isLicenseTrial )( struct LSLicenseHandler* self );
        bool ( *isLicenseActive )( struct LSLicenseHandler* self );
        bool ( *isLicenseEnabled )( struct LSLicenseHandler* self );
        bool ( *isLicenseExpired )( struct LSLicenseHandler* self );
        bool ( *isLicenseMaintenanceExpired )( struct LSLicenseHandler* self );
        bool ( *isLicenseOfflineActivated )( struct LSLicenseHandler* self );
        bool ( *isLicenseFloating )( struct LSLicenseHandler* self );
        bool ( *isLicenseBorrowed )( struct LSLicenseHandler* self );
        bool ( *isLicenseVMAllowed )( struct LSLicenseHandler* self );
        bool ( *isLicenseGracePeriodStarted )( struct LSLicenseHandler* self );
        struct LSLicenseID ( *getLicenseID )( struct LSLicenseHandler* self );
        const char* ( *getLicenseKey )( struct LSLicenseHandler* self );
        const char* ( *getLicenseUser )( struct LSLicenseHandler* self );
        const char* ( *getLicenseStatusStr )( struct LSLicenseHandler* self );
        const char* ( *getLicenseStartDate )( struct LSLicenseHandler* self );
        const char* ( *getLicenseMetadata )( struct LSLicenseHandler* self );
        enum LSLicenseType( *getLicenseType )( struct LSLicenseHandler* self );
        struct tm( *getLicenseExpiryDate )( struct LSLicenseHandler* self );
        struct tm( *getLicenseExpiryDateUtc )( struct LSLicenseHandler* self );
        struct tm( *getLicenseMaintenancePeriod )( struct LSLicenseHandler* self );
        struct tm( *getLicenseMaintenancePeriodUtc )( struct LSLicenseHandler* self );
        struct tm( *getLicenseLastCheckDate )( struct LSLicenseHandler* self );
        struct tm( *getLicenseLastCheckDateUtc )( struct LSLicenseHandler* self );
        struct tm( *getLicenseFloatingEndDateTime )( struct LSLicenseHandler* self );
        struct tm( *getLicenseFloatingEndDateTimeUtc )( struct LSLicenseHandler* self );
        int ( *getDaysRemaining )( struct LSLicenseHandler* self );
        int ( *getMaintenanceDaysRemaining )( struct LSLicenseHandler* self );
        int ( *getDaysPassedSinceLastCheck )( struct LSLicenseHandler* self );
        LSCustomer* ( *getLicenseOwner )( struct LSLicenseHandler* self );
        bool ( *hasLicenseFeatures )( struct LSLicenseHandler* self );
        struct LSLicenseFeature* ( *getLicenseFeature )( struct LSLicenseHandler* self, const char* featureCode );
        int ( *getLicenseFeatures )( struct LSLicenseHandler* self, struct LSLicenseFeature* features );
        bool ( *hasLicenseCustomFields )( struct LSLicenseHandler* self );
        int ( *getLicenseCustomFields )( struct LSLicenseHandler* self, struct LSCustomField* fields );
        unsigned long ( *getLicenseTimesActivated )( struct LSLicenseHandler* self );
        unsigned long ( *getLicenseMaxActivations )( struct LSLicenseHandler* self );
        unsigned long ( *getLicenseTransferCount )( struct LSLicenseHandler* self );

        // Meaningful only for Subscription license
        unsigned long ( *getLicenseGracePeriod )( struct LSLicenseHandler* self );
        struct tm ( *validityWithGracePeriod )( struct LSLicenseHandler* self );
        struct tm ( *validityWithGracePeriodUtc )( struct LSLicenseHandler* self );

        // Meaningful only for Consumption license
        int ( *getLicenseTotalConsumption )( struct LSLicenseHandler* self );
        int ( *getLicenseMaxConsumption )( struct LSLicenseHandler* self );
        bool ( *isLicenseOveragesAllowed )( struct LSLicenseHandler* self );
        bool ( *isLicenseUnlimitedConsumptionAllowed )( struct LSLicenseHandler* self );
        int ( *getLicenseMaxOverages )( struct LSLicenseHandler* self );
        bool ( *isResetLicenseConsumptionEnabled )( struct LSLicenseHandler* self );
        enum LSConsumptionPeriod( *getLicenseConsumptionPeriod )( struct LSLicenseHandler* self );

        // Meaningful only for Floating license
        unsigned long ( *getLicenseFloatingTimeout )( struct LSLicenseHandler* self );
        unsigned long ( *getLicenseFloatingInUseCount )( struct LSLicenseHandler* self );
        unsigned long ( *getLicenseMaxFloatingUsers )( struct LSLicenseHandler* self );
        unsigned long (* getMaxBorrowTime )( struct LSLicenseHandler* self );
        // ------------------------------------------------------------

        // ---------------------- Error handling ----------------------
        bool ( *wasError )( struct LSLicenseHandler* self );
        enum LSErrorCode( *getLastError )( struct LSLicenseHandler* self );
        const char* ( *getLastErrorMsg )( struct LSLicenseHandler* self );
        bool ( *isInitializationError )( struct LSLicenseHandler* self );
        bool ( *isInternalServerError )( struct LSLicenseHandler* self );
        bool ( *isNetworkTimeoutError )( struct LSLicenseHandler* self );
        bool ( *isNoInternetError )( struct LSLicenseHandler* self );
        bool ( *isActivationFailed )( struct LSLicenseHandler* self );
        bool ( *isLicenseExpiredError )( struct LSLicenseHandler* self );
        bool ( *isLicenseNotFoundError )( struct LSLicenseHandler* self );
        bool ( *isLicenseStateError )( struct LSLicenseHandler* self );
        bool ( *isHardwareIDError )( struct LSLicenseHandler* self );
        bool ( *isGeneralError )( struct LSLicenseHandler* self );
        bool ( *isLicenseFeatureError )( struct LSLicenseHandler* self );
        bool ( *canIgnoreError )( struct LSLicenseHandler* self ); // No Internet, network timeout and server errors can be ignored
        // ------------------------------------------------------------

        void* inner;

        struct LSConfiguration* currentConfig;

    } LSLicenseHandler;

    // LicenseHandler takes ownership of Configuration, so do not free it
    LS_API struct LSLicenseHandler* CreateLSLicenseHandler( struct LSConfiguration* config );

    // This function will also free current configuration, you do not need to explicitly call FreeLSConfiguration
    LS_API void FreeLSLicenseHandler( struct LSLicenseHandler* handler );

#ifdef __cplusplus
} // end extern "C"
#endif

#endif // LS_C_LICENSE_HANDLER_H
