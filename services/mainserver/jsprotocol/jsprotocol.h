#ifndef _jsprotocol_h_
#define _jsprotocol_h_

#include <string>
#include "reply_manager/reply_manager.h"

extern reply_manager *M_joinserver_manager;

void *joinserver_send_req(const std::string &service_name_, uint8_t *&buffer_, uint16_t buffer_len_, int thread_id_);

#endif