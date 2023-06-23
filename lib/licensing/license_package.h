//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_LICENSE_PACKAGE_H
#define CORE_LICENSE_PACKAGE_H

#include <licensing/backend.h>

#include <map>
#include <memory>


namespace uh::licensing
{

// ---------------------------------------------------------------------

class license_package
{

public:

    /**
     * manages features and metered setup on top of the license checker
     *
     * @param config license file input
     * @throws if license is invalid or cannot be loaded
     */
    explicit license_package(std::shared_ptr<backend> check_license);

    /**
     *
     * @param f check feature on or off/true or false
     * @throw feature state not specified by license and by that not listed
     * @return feature state
     */
    [[nodiscard]] bool check(feature f) const;

    /**
     * Check if feature value is within bounds.
     */
    void require(feature f, std::size_t value) const;

    /**
     *
     * @return if license checker returns valid
     */
    bool valid();

private:
    std::map<feature, bool> m_features;
    std::shared_ptr<backend> m_backend;
};

} // namespace uh::licensing

#endif //CORE_LICENSE_PACKAGE_H
