#ifndef LS_C_CUSTOMER_H
#define LS_C_CUSTOMER_H

#include "../APIDef.h"

#ifdef __cplusplus
extern "C" {
#endif

    LS_API typedef struct LSCustomer
    {
        const char* ( *email )( struct LSCustomer* self );

        void ( *setEmail )( struct LSCustomer* self, const char* email );

        const char* ( *firstName )( struct LSCustomer* self );

        void ( *setFirstName )( struct LSCustomer* self, const char* name );

        const char* ( *lastName )( struct LSCustomer* self );

        void ( *setLastName )( struct LSCustomer* self, const char* name );

        const char* ( *company )( struct LSCustomer* self );

        void ( *setCompany )( struct LSCustomer* self, const char* company );

        const char* ( *phone )( struct LSCustomer* self );

        void ( *setPhone )( struct LSCustomer* self, const char* phone );

        const char* ( *address )( struct LSCustomer* self );

        void ( *setAddress )( struct LSCustomer* self, const char* address );

        const char* ( *city )( struct LSCustomer* self );

        void ( *setCity )( struct LSCustomer* self, const char* city );

        const char* ( *state )( struct LSCustomer* self );

        void ( *setState )( struct LSCustomer* self, const char* state );

        const char* ( *country )( struct LSCustomer* self );

        void ( *setCountry )( struct LSCustomer* self, const char* country );

        const char* ( *postcode )( struct LSCustomer* postcode );

        void ( *setPostcode )( struct LSCustomer* self, const char* postcode );

        const char* ( *reference )( struct LSCustomer* self );

        void ( *setReference )( struct LSCustomer* self, const char* reference );

        const char* ( *metadata )( struct LSCustomer* self );

        void ( *setMetadata )( struct LSCustomer* self, const char* metadata );

        void* inner;

    } LSCustomer;

    LS_API LSCustomer* CreateLSCustomer();

    LS_API LSCustomer* CreateLSCustomerEx( const char* email );

    LS_API void FreeLSCustomer( LSCustomer* customer );

#ifdef __cplusplus
} // end extern "C"
#endif

#endif // LS_C_CUSTOMER_H
