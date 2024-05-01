#ifndef _dsprotocol_h_
#define _dsprotocol_h_

#include <string>
#include "reply_manager/reply_manager.h"

extern reply_manager *M_dataserver_manager;

void dataserver_get_char_list_request(UBFH *&buffer_, uint16_t index_);
void *dataserver_send_req(const std::string &service_name_, uint8_t *&buffer_, uint16_t buffer_len_, int thread_id_);

#endif