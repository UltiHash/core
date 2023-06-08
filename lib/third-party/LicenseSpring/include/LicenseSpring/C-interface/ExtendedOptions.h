#ifndef LS_C_EXTENDED_OPTIONS_H
#define LS_C_EXTENDED_OPTIONS_H

#include "../APIDef.h"
#include "../PODTypes.h"
#include <stdbool.h>
#include <wchar.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    LS_API typedef struct LSProxySettings
    {
        void* inner;

        void ( *setProxyUri )( struct LSProxySettings* self, const char* uri );

        const char* ( *getProxyUri )( struct LSProxySettings* self );

        void ( *setUser )( struct LSProxySettings* self, const char* user );

        const char* ( *getUser )( struct LSProxySettings* self );

        void ( *setPassword )( struct LSProxySettings* self, const char* password );

        const char* ( *getPassword )( struct LSProxySettings* self );

        bool ( *isValid )( struct LSProxySettings* self );

    } LSProxySettings;

    LS_API LSProxySettings* CreateLSProxySettings( const char* proxy, const char* user, const char* password );

    LS_API void FreeLSProxySettings( LSProxySettings* settings );


    LS_API typedef struct LSExtendedOptions
    {
        void* inner;

        void ( *setLicenseFilePath )( struct LSExtendedOptions* self, const wchar_t* filePath );

        const wchar_t* ( *getLicenseFilePath )( struct LSExtendedOptions* self );

        void ( *setHardwareID )( struct LSExtendedOptions* self, const char* ID );

        const char* ( *getHardwareID )( struct LSExtendedOptions* self );

        void ( *collectNetworkInfo )( struct LSExtendedOptions* self, bool collect );

        bool ( *isCollectNetworkInfoEnabled )( struct LSExtendedOptions* self );

        void ( *enableLogging )( struct LSExtendedOptions* self, bool enable );

        bool ( *isLoggingEnabled )( struct LSExtendedOptions* self );

        void ( *enableVMDetection )( struct LSExtendedOptions* self, bool enable );

        bool ( *isVMDetectionEnabled )( struct LSExtendedOptions* self );

        void ( *setNetworkTimeout )( struct LSExtendedOptions* self, long timeout );

        long ( *getNetworkTimeout )( struct LSExtendedOptions* self );

        void ( *setProxySettings )( struct LSExtendedOptions* self, const struct LSProxySettings* settings );

        const struct LSProxySettings* ( *getProxySettings )( struct LSExtendedOptions* self );

        void ( *setAlternateServiceURL )( struct LSExtendedOptions* self, const char* URL );

        const char* ( *getAlternateServiceURL )( struct LSExtendedOptions* self );

        void ( *setAlternateKey )( struct LSExtendedOptions* self, int32_t* key, int32_t size );

        enum DeviceIDAlgorithm ( *getDeviceIdAlgorithm )( struct LSExtendedOptions* self );

        void ( *setDeviceIdAlgorithm )( struct LSExtendedOptions* self, enum DeviceIDAlgorithm algorithm );

    } LSExtendedOptions;

    LS_API LSExtendedOptions* CreateLSExtendedOptions();

    LS_API LSExtendedOptions* CreateLSExtendedOptionsByPath( const wchar_t* licenseFilePath );

    LS_API LSExtendedOptions* CreateLSExtendedOptionsEx( const wchar_t* licenseFilePath, const char* hardwareID, const LSProxySettings* proxy );

    LS_API void FreeLSExtendedOptions( LSExtendedOptions* options );

#ifdef __cplusplus
} // end extern "C"
#endif

#endif // LS_C_EXTENDED_OPTIONS_H
