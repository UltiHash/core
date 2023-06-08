#ifndef LS_C_INSTALLATION_FILE_H
#define LS_C_INSTALLATION_FILE_H

#include "../APIDef.h"

#ifdef __cplusplus
extern "C" {
#endif

    LS_API typedef struct LSInstallationFile
    {
        const char* ( *getVersion )( struct LSInstallationFile* self );

        const char* ( *getUrl )( struct LSInstallationFile* self );

        const char* ( *getMd5Hash )( struct LSInstallationFile* self );

        const char* ( *getReleaseDate )( struct LSInstallationFile* self );

        const char* ( *getRequiredVersion )( struct LSInstallationFile* self );

        const char* ( *getEnvironment )( struct LSInstallationFile* self );

        const char* ( *getEulaLink )( struct LSInstallationFile* self );

        const char* ( *getReleaseNotesLink )( struct LSInstallationFile* self );

        const char* ( *getChannel )( struct LSInstallationFile* self );

        unsigned long long ( *getSize )( struct LSInstallationFile* self );

        void* inner;

    } LSInstallationFile;

    LS_API LSInstallationFile* CreateLSInstallationFile();

    LS_API void FreeLSInstallationFile( LSInstallationFile* file );

#ifdef __cplusplus
} // end extern "C"
#endif

#endif // LS_C_INSTALLATION_FILE_H
