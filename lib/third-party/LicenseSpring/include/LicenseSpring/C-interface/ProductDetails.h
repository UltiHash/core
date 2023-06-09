#ifndef LS_C_PRODUCT_DETAILS_H
#define LS_C_PRODUCT_DETAILS_H

#include "../APIDef.h"
#include "../PODTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

    LS_API typedef struct LSProductDetails
    {
        bool ( *isTrialAllowed )( struct LSProductDetails* self );

        bool ( *isVMAllowed )( struct LSProductDetails* self );

        unsigned long ( *trialPeriod )( struct LSProductDetails* self );

        unsigned long ( *floatingLicenseTimeout )( struct LSProductDetails* self );

        const char* ( *productName )( struct LSProductDetails* self );

        const char* ( *productCode )( struct LSProductDetails* self );

        enum LSAuthMethod( *authorizationMethod )( struct LSProductDetails* self );

        void* inner;

    } LSProductDetails;

    LS_API LSProductDetails* CreateLSProductDetails();

    LS_API void FreeLSProductDetails( LSProductDetails* details );

#ifdef __cplusplus
} // end extern "C"
#endif

#endif // LS_C_PRODUCT_DETAILS_H
