//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_METRED_RESOURCE_H
#define CORE_METRED_RESOURCE_H

#include <cstddef>

namespace uh::licensing{

    class metred_resource{

    public:

        ~metred_resource() = default;

        /**
         *
         * @return return if hard limit was reached
         */
        virtual bool hard_limit_allocate(std::size_t alloc) = 0;

        /**
         *
         * @param dealloc a resource
         */
        virtual void deallocate(std::size_t dealloc) = 0;

        /**
         *
         * @return count of free metred elements
         */
        virtual std::size_t free_count() = 0;

    };

}

#endif //CORE_METRED_RESOURCE_H
