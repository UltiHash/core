#include "payg.h"

#include "_util.h"
#include "common/etcd/namespace.h"

#include <common/telemetry/log.h>
#include <common/utils/strings.h>
#include <nlohmann/json.hpp>

using nlohmann::json;

namespace uh::cluster {

NLOHMANN_JSON_SERIALIZE_ENUM(payg::type, //
                             {
                                 {payg::FREEMIUM, "freemium"},
                                 {payg::PREMIUM, "premium"},
                             })

void to_json(json& j, const payg& p) {
    j = json{{"customer_id", p.customer_id},
             {"license_type", p.license_type},
             {"storage_cap", p.storage_cap},
             {"ec",
              {{"enabled", p.ec.enabled}, //
               {"max_group_size", p.ec.max_group_size}}},
             {"replication",
              {{"enabled", p.replication.enabled},
               {"max_replicas", p.replication.max_replicas}}}};
}

void from_json(const json& j, payg& p) {
    j.at("customer_id").get_to(p.customer_id);
    j.at("license_type").get_to(p.license_type);
    j.at("storage_cap").get_to(p.storage_cap);
    j.at("ec").at("enabled").get_to(p.ec.enabled);
    j.at("ec").at("max_group_size").get_to(p.ec.max_group_size);
    j.at("replication").at("enabled").get_to(p.replication.enabled);
    j.at("replication").at("max_replicas").get_to(p.replication.max_replicas);
}

payg check_payg_license(std::string_view license, bool skip_verify) {
    nlohmann::json j = nlohmann::json::parse(license);
    auto sign_b64 = j.at("signature").get<std::string>();
    j.erase("signature");

    auto compact_json_str = j.dump().substr(0);
    LOG_INFO() << "compact form: " << compact_json_str;

    auto signature = base64_decode(sign_b64);

    if (!skip_verify && !verify_license(compact_json_str, signature)) {
        throw std::runtime_error("signature of license could not be verified");
    }

    auto rv = j.template get<payg>();
    return rv;
}

void broadcast(etcd_manager& etcd, const payg& license) {
    etcd.put(get_etcd_payg_license_key("customer_id"), license.customer_id);
    etcd.put(get_etcd_payg_license_key("license_type"),
             std::string(magic_enum::enum_name(license.license_type)));
    etcd.put(get_etcd_payg_license_key("storage_cap"),
             std::to_string(license.storage_cap));
    etcd.put(get_etcd_payg_license_key("ec/enabled"),
             license.ec.enabled ? "true" : "false");
    etcd.put(get_etcd_payg_license_key("ec/max_group_size"),
             std::to_string(license.ec.max_group_size));
    etcd.put(get_etcd_payg_license_key("replication/enabled"),
             license.replication.enabled ? "true" : "false");
    etcd.put(get_etcd_payg_license_key("replication/max_replicas"),
             std::to_string(license.replication.max_replicas));
}

} // namespace uh::cluster
