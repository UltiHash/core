//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_LICENSED_FEATURE_H
#define CORE_LICENSED_FEATURE_H

#include <string>
#include <utility>

namespace uh::licensing{

    class licensed_feature {

    public:

        explicit licensed_feature(std::string feature_name,bool is_active);

        [[nodiscard]] virtual bool is_active() const = 0;

    private:
        std::string feature_name;
        bool state;
    };

} // namespace uh::licensing

#endif //CORE_LICENSED_FEATURE_H
