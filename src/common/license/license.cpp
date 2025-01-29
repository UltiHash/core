#include <common/license/license.h>

#include <common/license/internal/util.h>
#include <common/telemetry/log.h>
#include <common/utils/strings.h>
#include <nlohmann/json.hpp>

using nlohmann::json;

namespace uh::cluster {

NLOHMANN_JSON_SERIALIZE_ENUM(license::type, //
                             {
                                 {license::FREEMIUM, "freemium"},
                                 {license::PREMIUM, "premium"},
                             })

void to_json(json& j, const license& p) {
    j = json{
        {"version", p.version},
        {"customer_id", p.customer_id},
        {"license_type", p.license_type},
        {"storage_cap", p.storage_cap},
    };
}

void from_json(const json& j, license& p) {
    j.at("version").get_to(p.version);
    j.at("customer_id").get_to(p.customer_id);
    j.at("license_type").get_to(p.license_type);
    j.at("storage_cap").get_to(p.storage_cap);
}

license license::create(std::string_view json_str, verify option) {
    auto j = nlohmann::ordered_json::parse(json_str);

    auto rv = j.template get<license>();

    auto compact_json = j.dump();

    if (option == verify::VERIFY) {
        if (!j.contains("signature")) {
            throw std::runtime_error("missing key: signature");
        }

        auto sign_b64 = j.at("signature").get<std::string>();
        j.erase("signature");

        auto signature_removed = j.dump();

        auto signature = base64_decode(sign_b64);

        if (!verify_license(signature_removed, signature)) {
            throw std::runtime_error("signature of rv could not be verified");
        }
    }

    rv.m_compact_json = std::move(compact_json);

    return rv;
}

} // namespace uh::cluster
