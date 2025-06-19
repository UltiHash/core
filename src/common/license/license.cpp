#include <common/license/license.h>

#include <common/license/internal/util.h>
#include <common/telemetry/log.h>
#include <common/utils/strings.h>
#include <nlohmann/json.hpp>

using nlohmann::json;

namespace uh::cluster {

std::vector<std::pair<std::string, std::string>>
license::to_key_value_iterable() const {
    return {
        {"version", version},
        {"customer_id", customer_id},
        {"license_type", std::string(magic_enum::enum_name(license_type))},
        {"storage_cap_gib", std::to_string(storage_cap_gib)},
    };
}

NLOHMANN_JSON_SERIALIZE_ENUM(license::type, //
                             {
                                 {license::NONE, ""},
                                 {license::FREEMIUM, "freemium"},
                                 {license::PREMIUM, "premium"},
                             })

void to_json(json& j, const license& p) {
    j = json{
        {"version", p.version},
        {"customer_id", p.customer_id},
        {"license_type", p.license_type},
        {"storage_cap_gib", p.storage_cap_gib},
    };
}

void from_json(const json& j, license& p) {
    j.at("version").get_to(p.version);
    j.at("customer_id").get_to(p.customer_id);
    j.at("license_type").get_to(p.license_type);
    if (j.contains("storage_cap_gib")) {
        j.at("storage_cap_gib").get_to(p.storage_cap_gib);
    } else {
        if (p.license_type == license::FREEMIUM)
            throw std::runtime_error(
                "Freemium license should have storage_cap_gib field");
        p.storage_cap_gib = 0;
    }
}

license license::create(const std::string& json_str, verify option) {
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
            throw std::runtime_error(
                "signature of license could not be verified");
        }
    }

    rv.m_compact_json = std::move(compact_json);

    return rv;
}

} // namespace uh::cluster
