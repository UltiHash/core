#ifndef LS_C_LICENSE_ID_H
#define LS_C_LICENSE_ID_H

#include "../APIDef.h"

#ifdef __cplusplus
extern "C" {
#endif

    LS_API typedef struct LSLicenseID
    {
        const char* key;
        const char* user;
        const char* password;
    } LSLicenseID;

    LS_API LSLicenseID CreateLicenseIDfromKey( const char* licenseKey );
    LS_API LSLicenseID CreateLicenseIDfromUser( const char* user, const char* password );

#ifdef __cplusplus
} // end extern "C"
#endif

#endif // LS_C_LICENSE_ID_H
