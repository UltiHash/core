#ifndef LS_C_LICENSE_FEATURE_H
#define LS_C_LICENSE_FEATURE_H

#include "../APIDef.h"
#include "../PODTypes.h"
#include <time.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    LS_API typedef struct LSLicenseFeature
    {
        const char* ( *toString )( struct LSLicenseFeature* self );

        const char* ( *code )( struct LSLicenseFeature* self );

        const char* ( *name )( struct LSLicenseFeature* self );

        enum LSFeatureType( *featureType )( struct LSLicenseFeature* self );

        enum LSFeatureType( *featureTypeFromString )( struct LSLicenseFeature* self, const char* value );

        int ( *maxConsumption )( struct LSLicenseFeature* self );

        int ( *totalConsumption )( struct LSLicenseFeature* self );

        int ( *localConsumption )( struct LSLicenseFeature* self );

        bool ( *isOveragesAllowed )( struct LSLicenseFeature* self );

        bool ( *isUnlimitedConsumptionAllowed )( struct LSLicenseFeature* self );

        int ( *maxOverages )( struct LSLicenseFeature* self );

        bool ( *isResetConsumptionEnabled )( struct LSLicenseFeature* self );

        enum LSConsumptionPeriod( *consumptionPeriod )( struct LSLicenseFeature* self );

        struct tm( *expiryDate )( struct LSLicenseFeature* self );

        struct tm( *expiryDateUtc )( struct LSLicenseFeature* self );

        bool ( *isExpired )( struct LSLicenseFeature* self );

        void* inner;

    } LSLicenseFeature;

    LS_API LSLicenseFeature* CreateLSLicenseFeature();

    LS_API void FreeLSLicenseFeature( LSLicenseFeature* feature );

#ifdef __cplusplus
} // end extern "C"
#endif

#endif // LS_C_LICENSE_FEATURE_H
