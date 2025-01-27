#include "test.h"

#include <common/license/internal/util.h>

#include <common/utils/common.h>
#include <common/utils/strings.h>

namespace uh::cluster {

namespace {

license parse(std::string_view data) {
    auto fields = split(data, ':');

    return license{.customer = std::string(fields[0]),
                   .max_data_store_size =
                       std::stoull(std::string(fields[1])) * GIBI_BYTE};
}

} // namespace

license check_license(std::string_view license_code) {
    auto colon = license_code.rfind(':');
    if (colon == std::string::npos) {
        throw std::runtime_error("format error in license");
    }

    auto data = license_code.substr(0, colon);
    auto sign_b64 = license_code.substr(colon + 1);

    auto signature = base64_decode(sign_b64);

    if (!verify_license(data, signature)) {
        throw std::runtime_error(
            "signature of test license could not be verified");
    }

    return parse(data);
}

} // namespace uh::cluster
