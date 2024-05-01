#ifndef _protocol_h_
#define _protocol_h_

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <Exfields.h>

#include <string>
#include <mutex>
#include <map>

#include "MuOnline.fd.h"
#include "muprotocol.h"
#include "packet_engine.h"
#include "simple_modulus/simple_modulus.h"

extern simple_modulus M_simple_modulus_cs;
extern simple_modulus M_simple_modulus_sc;

void gc_join_result(uint8_t result_, uint16_t index_);

void protocol_core(UBFH *&buffer_, uint8_t header_code_, uint8_t *message_, uint16_t message_len_, uint16_t index_, bool encrypt_ = false, int serial_ = -1);
void *recv_data_parser(UBFH *&buffer_, int thread_id_);
void close_client(int index_);
void close_client(UBFH *&buffer, int index_);
int data_send(const std::string &svc_name_, UBFH *&buffer_, uint8_t *msg_, uint16_t msg_size_, uint16_t message_type_ = 0);

#endif