#pragma once

#include <memory>
#include <vector>
#include "ts_unordered_map.h"
#include "ts_map.h"
#include "ts_vector.h"
#include "common/common_types.h"

namespace uh::cluster::rest::utils
{

    class ts_address_size
    {
    public:
        ts_address_size(const address& addr, std::size_t size, uint32_t part_num)
        {
            emplace(addr, size, part_num);
        }

        void emplace(const address& addr, std::size_t size, uint32_t part_num)
        {
            std::lock_guard<std::mutex> lock(mutex);
            m_addr.emplace(part_num, addr);
            m_sizes.push_back(size);
        }

        const std::vector<size_t>& get_eff_sizes() const
        {
            return  m_sizes;
        }

        const std::map<uint32_t , address>& get_map_address() const
        {
            return m_addr;
        }


    private:
        std::mutex mutex {};
        std::map<uint32_t ,address> m_addr;
        std::vector<std::size_t> m_sizes ;
    };

    class state
    {
    public:
        state() = default;
        ~state() = default;

        std::string generate_and_add_upload_id(const std::string&, const std::string&);

        ts_unordered_map<std::string, std::shared_ptr<ts_map<uint16_t, std::pair<std::string, std::string>>>>&
        get_multipart_container();

        ts_unordered_map<std::string, std::shared_ptr<ts_map<std::string, std::shared_ptr<ts_vector<std::string>>>>>&
        get_bucket_multiparts();

        rest::utils::ts_unordered_map<std::string, std::shared_ptr<ts_address_size>>&
        get_mp_address_container();

    private:
        /* { upload_id, { part_number, < etag, body > } } } */
        ts_unordered_map<std::string, std::shared_ptr<ts_map<uint16_t, std::pair<std::string, std::string>>>> m_parts_container;

        /* { bucket_name, { keys, [ upload_id, ... ] } } */
        ts_unordered_map<std::string, std::shared_ptr<ts_map<std::string, std::shared_ptr<ts_vector<std::string>>>>> m_current_bucket_multiparts;

        /* { upload_id, { address, effective_size } } } */
        rest::utils::ts_unordered_map<std::string, std::shared_ptr<ts_address_size>> m_addr;

    };

} // uh::cluster::rest::utils
