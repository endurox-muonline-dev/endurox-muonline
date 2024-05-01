#include <atmi.h>
#include <ndebug.h>
#include <ubf.h>

#include "utils.h"
#include "Exfields.h"
#include "muprotocol.h"
#include "protocol/protocol.h"
#include "jsprotocol/jsprotocol.h"
#include "client_info/client_info.h"

void join_id_pass_result(uint8_t *&message_)
{
	muProtocol::js_id_pass_ans *req_msg = (muProtocol::js_id_pass_ans*)message_;
	client_info *client = nullptr;

	try
	{
		client = M_clients.at(req_msg->get_index() - USER_START_INDEX);
	}
	catch (const std::exception& e)
	{
		TP_LOG(log_error, "error-L1: user index(%d) out of range: %s", req_msg->get_index(), e.what());
		return;
	}

	if (nullptr != client)
	{
		if (req_msg->get_result() == muProtocol::kJsLoginResult::LOGIN_SUCCESS || 
			req_msg->get_result() == muProtocol::kJsLoginResult::LOGIN_SUCCESS_ITEMBLOCK)
		{
			//set_account_login()

			if (req_msg->get_user_number() == 0 && req_msg->get_db_number() == 0)
			{
				TP_LOG(log_debug, "User (%s) with number (%d) and guid (%d)", req_msg->get_account().c_str(), req_msg->get_user_number(), req_msg->get_db_number());
			}
		}

		if (muProtocol::kJsLoginResult::LOGIN_WRONG_PASSWORD == req_msg->get_result())
		{
			req_msg->set_result(muProtocol::kJsLoginResult::LOGIN_INVALID_NOT_EXIST_USERNAME);
		}

		if (req_msg->get_result() == muProtocol::kJsLoginResult::LOGIN_SUCCESS_ITEMBLOCK)
		{
			req_msg->set_result(muProtocol::kJsLoginResult::LOGIN_SUCCESS);
			//client->set_account_item_block(true);
		}

		client->set_connection_state(kClientState::CONNECTED);

		if (client->get_login_send_count() >= 3)
		{
			req_msg->set_result(muProtocol::kJsLoginResult::LOGIN_CONNECTION_REFUSED);
		}

		gc_join_result(req_msg->get_result(), req_msg->get_index());

		if (req_msg->get_result() != muProtocol::kJsLoginResult::LOGIN_SUCCESS)
		{
			if (client->get_login_send_count() > 3)
			{
				TP_LOG(log_error, "error-L1: user index(%d) failed to login more than 3 times...", req_msg->get_index());
				close_client(req_msg->get_index());
				return;
			}
		}
	}

	TP_LOG(log_debug, "%s(%d) result: %d, account (%s)", __FUNCTION__, req_msg->get_index(), req_msg->get_result(), req_msg->get_account().c_str());
}

/**
 * 
 */
void js_protocol_core(UBFH *&buffer_, uint8_t header_, uint8_t *message_, int len_)
{
	switch (header_)
	{
		case 0x01:
			join_id_pass_result(message_);
			break;
	}
}

/**
 * 
 */
void *joinserver_send_req(const std::string &service_name_, uint8_t *&buffer_, uint16_t buffer_len_, int thread_id_)
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
		js_protocol_core(buffer, message_header, message_data, message_len);

		TP_LOG(log_debug, "tpcall(%s) successfull", service_name_.c_str());
		tplogprintubf(log_dump, (char*)"joinserver_send_req()", buffer);
	}

	tpfree((char*)buffer);
	return 0;
}