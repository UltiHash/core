//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_METRED_RESOURCE_H
#define CORE_METRED_RESOURCE_H

namespace uh::licensing{

    class metred_resource{

        /**
         *
         * @return return if hard limit was reached
         */
        virtual bool hard_limit_allocate(std::size_t alloc) = 0;

        /**
         *
         */
        virtual void deallocate(std::size_t dealloc) = 0;

    };

}

#endif //CORE_METRED_RESOURCE_H
