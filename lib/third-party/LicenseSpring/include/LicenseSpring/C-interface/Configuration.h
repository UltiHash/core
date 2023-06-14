#ifndef LS_C_CONFIGURATION_H
#define LS_C_CONFIGURATION_H

#include "ExtendedOptions.h"

#ifdef __cplusplus
extern "C" {
#endif

    LS_API typedef struct LSConfiguration
    {
        void* inner;

        const char* ( *getApiKey )( struct LSConfiguration* self );

        const char* ( *getSharedKey )( struct LSConfiguration* self );

        const char* ( *getProductCode )( struct LSConfiguration* self );

        const char* ( *getAppName )( struct LSConfiguration* self );

        const char* ( *getAppVersion )( struct LSConfiguration* self );

        const char* ( *getCustomerAccount )( struct LSConfiguration* self );

        const char* ( *getUserInfo )( struct LSConfiguration* self );

        void ( *setCustomerAccount )( struct LSConfiguration* self, const char* accountCode );

        void ( *setUserInfo )( struct LSConfiguration* self, const char* customInfo );

        const wchar_t* ( *getLicenseFilePath )( struct LSConfiguration* self );

        const char* ( *getAlternateServiceUrl )( struct LSConfiguration* self );

        const char* ( *getHardwareID )( struct LSConfiguration* self );

        bool ( *isCollectNetworkInfoEnabled )( struct LSConfiguration* self );

        bool ( *isLoggingEnabled )( struct LSConfiguration* self );

        long ( *getNetworkTimeout )( struct LSConfiguration* self );

        void ( *setNetworkTimeout )( struct LSConfiguration* self, long timeoutValue ); // in seconds, 0 means no timeout

        const LSProxySettings* ( *getProxySettings )( struct LSConfiguration* self );

        unsigned long ( *getLicenseSpringAPIVersion )( struct LSConfiguration* self );

        const char* ( *getSdkVersion )( struct LSConfiguration* self );

        const char* ( *getAirGapPublicKey )( struct LSConfiguration* self );

        void ( *setAirGapPublicKey )( struct LSConfiguration* self, const char* airGapKey );

        const char* ( *getOsVersion )( struct LSConfiguration* self );

        const char* ( *getHostName )( struct LSConfiguration* self ); // note: string is UTF-8 encoded

        const char* ( *getIP )( struct LSConfiguration* self );

        const char* ( *getMAC )( struct LSConfiguration* self );

        bool ( *isVMDetectionEnabled )( struct LSConfiguration* self );

        bool ( *isVM )( struct LSConfiguration* self ); // in case VM detection enabled returns true if app currently running in virtualized environment

        const char* ( *getDetectedVMName )( struct LSConfiguration* self ); // return hypervisor name or empty string

    } LSConfiguration;

    LS_API LSConfiguration* CreateLSConfiguration( const char* apiKey,
        const char* sharedKey,
        const char* productCode,
        const char* appName,
        const char* appVersion,
        LSExtendedOptions* extendedOptions );

    LS_API void FreeLSConfiguration( LSConfiguration* config );

#ifdef __cplusplus
} // end extern "C"
#endif

#endif // LS_C_CONFIGURATION_H
