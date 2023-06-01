//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_TIMED_H
#define CORE_CHECK_TIMED_H

namespace uh::licensing {

    class check_timed {

    protected:

        virtual bool valid_timing() = 0;

    };

} // namespace uh::licensing

#endif //CORE_CHECK_TIMED_H
