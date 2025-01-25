#pragma once

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>

namespace uh::cluster::ep {

template <typename Struct, typename MemberType>
MemberType get_member_type(MemberType Struct::*);

template <auto Member> class payg_manager {
public:
    using MemberType = decltype(get_member_type(Member));

    payg_manager(etcd_manager& etcd, std::atomic<MemberType>& storage_cap)
        : m_etcd{etcd} {

        storage_cap.store(
            m_etcd.get<MemberType>(get_etcd_payg_license_key("storage_cap")));
        m_wg = m_etcd.watch(get_etcd_payg_license_key("storage_cap"),
                            [&](etcd::Response) {
                                storage_cap.store(m_etcd.get<MemberType>(
                                    get_etcd_payg_license_key("storage_cap")));
                            });
    }

private:
    etcd_manager& m_etcd;
    etcd_manager::watch_guard m_wg;
};

} // namespace uh::cluster::ep
