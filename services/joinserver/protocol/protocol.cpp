#include <ndebug.h>
#include <Exfields.h>

#include "utils.h"
#include "driver.h"
#include "muprotocol.h"
#include "protocol/protocol.h"

pg_driver *M_db_handle = nullptr;

enum kAccountFlags {
	NONE = 0,
	ITEM_BLOCK = 1,
	FULL_BLOCK = 2
};

void join_id_pass_request(UBFH *&buffer_, uint8_t *&message_)
{
	muProtocol::js_id_pass_req *req_msg = (muProtocol::js_id_pass_req*)message_;
	muProtocol::js_id_pass_ans ans_msg;
	uint8_t result = muProtocol::kJsLoginResult::LOGIN_SUCCESS;
	int user_guid = 0;
	std::string user_serial;

	if (false == sql_syntax_check(req_msg->get_account().c_str()) || false == sql_syntax_check(req_msg->get_password().c_str()))
	{
		result = muProtocol::kJsLoginResult::LOGIN_INVALID_NOT_EXIST_USERNAME;
	}
	else
	{
		if (false == space_syntax_check(req_msg->get_account().c_str()))
		{
			result = muProtocol::kJsLoginResult::LOGIN_INVALID_NOT_EXIST_USERNAME;
		}
		else
		{
			result = muProtocol::kJsLoginResult::LOGIN_NO_INFORMATION;

			if (!M_db_handle->select("SELECT guid, password, serial_number, flags FROM accounts WHERE name='" + req_msg->get_account() + "'"))
			{
				result = muProtocol::kJsLoginResult::LOGIN_INVALID_NOT_EXIST_USERNAME;
			}
			else
			{
				if (M_db_handle->get_num_rows() <= 0)
				{
					result = muProtocol::kJsLoginResult::LOGIN_INVALID_NOT_EXIST_USERNAME;
				}
				else
				{
					user_guid = M_db_handle->get_field_int(0, 0);
					user_serial = M_db_handle->get_field(0, 2);
					int flags = M_db_handle->get_field_int(0, 3);
					std::string password = M_db_handle->get_field(0, 1);
					std::string pw_hash = sha256(req_msg->get_password());

					if (password != pw_hash)
					{
						result = muProtocol::kJsLoginResult::LOGIN_WRONG_PASSWORD;
					}
					else
					{
						result = muProtocol::kJsLoginResult::LOGIN_SUCCESS;

						// Check flags
						if (flags > 0)
						{
							if ((flags & kAccountFlags::ITEM_BLOCK) == kAccountFlags::ITEM_BLOCK)
							{
								// Set in local storage info about ITEM_BLOCK
								result = muProtocol::kJsLoginResult::LOGIN_SUCCESS;
							}
						}

						/*if (clients_search_user(req_msg->get_account()))
						{
							result = muProtocol::kJsLoginResult::LOGIN_USER_ALREADY_CONNECTED;
							TP_LOG(log_error, "clients_search_user(%s) result: LOGIN_USER_ALREADY_CONNECTED", req_msg->get_account().c_str());
						}*/

						if (muProtocol::kJsLoginResult::LOGIN_SUCCESS == result)
						{
							//result = muProtocol::kJsLoginResult::LOGIN_NO_INFORMATION;

							// Set account information in local storage
							// ->
						}
					}

					TP_LOG(log_debug, "[%s] guid: %d, flags: %d, ip: %s", req_msg->get_account().c_str(), user_guid, flags, req_msg->get_address().c_str());
				}
			}
			
			M_db_handle->free_result();
		}
	}

	// Set message data
	ans_msg.set_result(result);
	ans_msg.set_index(req_msg->get_index());
	ans_msg.set_user_number(0);
	ans_msg.set_db_number(user_guid);
	ans_msg.set_account(req_msg->get_account());
	ans_msg.set_serial(user_serial);

	if (EXSUCCEED != Bchg(buffer_, EX_NETDATA, 0, (char*)&ans_msg, ans_msg.get_size()))
	{
		Binit(buffer_, 256);
		TP_LOG(log_error, "Bchg EX_NETDATA failed: %s", Bstrerror(Berror));
		return;
	}

	TP_LOG(log_debug, "%s(%d): (%s)-(%s)", __FUNCTION__, req_msg->get_index(), req_msg->get_account().c_str(), req_msg->get_password().c_str());
}

void protocol_core(UBFH *&buffer_, uint8_t header_, uint8_t *message_, int len_)
{
	switch (header_)
	{
		case 0x01:
			join_id_pass_request(buffer_, message_);
			break;
	}
}