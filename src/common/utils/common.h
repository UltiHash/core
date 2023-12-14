//
// Created by masi on 7/19/23.
//

#ifndef CORE_COMMON_H
#define CORE_COMMON_H
#include <string>
#include <vector>
#include <map>
#include "common_types.h"

namespace uh::cluster {

enum message_type: uint8_t {
    READ_REQ = 10,
    READ_RESP = 11,
    WRITE_REQ = 12,
    WRITE_RESP = 13,
    SYNC_REQ = 20,
    SYNC_OK = 21,
    REMOVE_REQ = 22,
    REMOVE_OK = 23,
    USED_REQ = 24,
    USED_RESP = 25,
    DEDUPE_REQ = 26,
    DEDUPE_RESP = 27,
    DIR_PUT_OBJ_REQ = 28,
    DIR_GET_OBJ_REQ = 29,
    DIR_GET_OBJ_RESP = 30,
    DIR_PUT_BUCKET_REQ = 31,
    SUCCESS = 32,
    FAILURE = 33,
    STOP = 34,
    RECOVER_REQ = 35,
    RECOVER_RESP = 36,
    DIR_LIST_BUCKET_REQ = 37,
    DIR_LIST_BUCKET_RESP = 38,
    DIR_LIST_OBJ_REQ = 39,
    DIR_LIST_OBJ_RESP = 40,
    DIR_DELETE_BUCKET_REQ = 41,
    DIR_DELETE_BUCKET_RESP = 42,
    READ_ADDRESS_REQ = 43,
    READ_ADDRESS_RESP = 44,
    DIR_DELETE_OBJ_REQ = 45,
    DIR_BUCKET_EXISTS = 46,
};

enum role: uint8_t {
    STORAGE_SERVICE,
    DEDUPLICATION_SERVICE,
    DIRECTORY_SERVICE,
    ENTRYPOINT_SERVICE,
};

enum ec_type: uint8_t {
    NONE = 0,
    XOR,
};

const std::map<std::string, uh::cluster::role> role_by_abbreviation = {
        {"ds", uh::cluster::STORAGE_SERVICE},
        {"dd", uh::cluster::DEDUPLICATION_SERVICE},
        {"dr", uh::cluster::DIRECTORY_SERVICE},
        {"en", uh::cluster::ENTRYPOINT_SERVICE}
};

const std::map<uh::cluster::role, std::string> abbreviation_by_role = {
        {uh::cluster::STORAGE_SERVICE,       "ds"},
        {uh::cluster::DEDUPLICATION_SERVICE, "dd"},
        {uh::cluster::DIRECTORY_SERVICE,     "dr"},
        {uh::cluster::ENTRYPOINT_SERVICE,    "en"}
};

static uh::cluster::role get_role (const std::string& role_str) {
    if (role_by_abbreviation.contains(role_str))
        return role_by_abbreviation.at(role_str);
    else
        throw std::invalid_argument ("Invalid role!");
}

} // end namespace uh::cluster


#endif //CORE_COMMON_H
