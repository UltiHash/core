//
// Created by ankit on 26.10.22.
//

#ifndef INC_3_NETWORK_COMMUNICATION_UHCUSTOM_H
#define INC_3_NETWORK_COMMUNICATION_UHCUSTOM_H

#include "net_common.h"
#include "net_queue_ts.h"
#include "net_message.h"
#include "net_client.h"
#include "net_server.h"
#include "net_connection.h"

enum class CustomMsgTypes: uint32_t {
    ServerAccept,
    ServerDeny,
    Ping,
};

#endif //INC_3_NETWORK_COMMUNICATION_UHCUSTOM_H