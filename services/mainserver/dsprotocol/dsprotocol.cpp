#include <atmi.h>
#include <ndebug.h>
#include <ubf.h>

#include "utils.h"
#include "Exfields.h"
#include "muprotocol.h"
#include "protocol/protocol.h"
#include "dsprotocol/dsprotocol.h"
#include "client_info/client_info.h"

void dataserver_get_char_list_request(UBFH *&buffer_, uint16_t index_)
{
	client_info *client = nullptr;

	try
	{
		client = M_clients.at(index_ - USER_START_INDEX);
	}
	catch(const std::exception &e)
	{
		TP_LOG(log_error, "error-L1: user index(%d) out of range: %s", index_, e.what());
		return;
	}

	std::string account_id = client->get_account_id();

	if (account_id.empty())
	{
		TP_LOG(log_error, "error-L1: Account is empty.");
		close_client(index_);
		return;
	}

	if (account_id.size() < 1)
	{
		TP_LOG(log_error, "error-L1: Account is one character size.");
		close_client(index_);
		return;
	}

	muProtocol::get_char_list_req req_msg(index_, account_id);
	M_dataserver_manager->dispatch(dataserver_send_req, "DATASERVER", (uint8_t*&)req_msg, (uint16_t)req_msg.get_size());
}

/**
 * 
 */
void *dataserver_send_req(const std::string &service_name_, uint8_t *&buffer_, uint16_t buffer_len_, int thread_id_)
{
	TP_LOG(log_error, "%s(%s)(%d) buffer data: (%p)(%d)", __FUNCTION__, service_name_.c_str(), thread_id_, buffer_, buffer_[0]);

	UBFH *buffer = (UBFH*)tpalloc((char*)"UBF", NULL, 65536);

	if (EXSUCCEED != Bchg(buffer, EX_NETDATA, 0, (char*)buffer_, buffer_len_))
	{
		TP_LOG(log_error, "Bchg EX_NETDATA failed: %s", Bstrerror(Berror));
		tpfree((char*)buffer);
		return 0;
	}

	long output_len = 0;
	if (EXSUCCEED != tpcall((char*)service_name_.c_str(), (char*)buffer, 0, (char**)&buffer, &output_len, 0))
	{
		TP_LOG(log_error, "tpcall failed: %s", tpstrerror(tperrno));
	}
	else
	{
		BFLDLEN message_len = Blen(buffer, EX_NETDATA, 0);
		uint8_t message_data[MAX_C2_MESSAGE_SIZE] = { 0 };
		uint8_t message_header = 0;

		if (EXSUCCEED != Bget(buffer, EX_NETDATA, 0, (char*)message_data, &message_len))
		{
			TP_LOG(log_error, "Bget EX_NETDATA failed: %s", Bstrerror(Berror));
			tpfree((char*)buffer);
			return 0;
		}

		message_header = (message_data[0] == 0xC1) ? message_data[2] : message_data[3];
		//ds_protocol_core(buffer, message_header, message_data, message_len);

		TP_LOG(log_debug, "tpcall(%s) successfull", service_name_.c_str());
		tplogprintubf(log_dump, (char*)"dataserver_send_req()", buffer);
	}

	tpfree((char*)buffer);
	return 0;
}