#include <ndebug.h>
#include <Exfields.h>

#include "utils.h"
#include "driver.h"
#include "muprotocol.h"
#include "protocol/protocol.h"

pg_driver *M_db_handle = nullptr;

void protocol_core(UBFH *&buffer_, uint8_t header_, uint8_t *message_, int len_)
{
	switch (header_)
	{
		case 0x01:
			muProtocol::get_char_list_req *req_msg = (muProtocol::get_char_list_req*)message_;
			TP_LOG(log_debug, "GJPCharacterListRequestCS(): %s (%d)", req_msg->get_account().c_str(), req_msg->get_index());
			break;
	}
}