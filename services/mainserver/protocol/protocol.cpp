#include "protocol.h"
#include "utils.h"

#include <memory>
#include <chrono>
#include <vector>

#include "config/config.h"
#include "dsprotocol/dsprotocol.h"
#include "jsprotocol/jsprotocol.h"
#include "client_info/client_info.h"

/**
 * 
 */
void join_result_send(UBFH *&buffer_, uint16_t index_, uint8_t result_)
{
	muProtocol::ans_join_result ans_msg(result_, index_, M_service_config.client_stripped_version.c_str());
	data_send("TCPGATE_GS", buffer_, (uint8_t*)&ans_msg, ans_msg.get_size());
}

void gc_join_result(uint8_t result_, uint16_t index_)
{
	long connection_id = 0;
	client_info *client = nullptr;
	muProtocol::js_join_result ans_msg(result_);

	try
	{
		client = M_clients.at(index_ - USER_START_INDEX);
	}
	catch (const std::exception &e)
	{
		TP_LOG(log_error, "error-L1: user index(%d) out of range: %s", index_, e.what());
		return;
	}

	UBFH *buffer = (UBFH*)tpalloc((char*)"UBF", NULL, 1024);

	if (EXSUCCEED != Bchg(buffer, EX_NETCONNID, 0, (char*)&connection_id, 0))
	{
		TP_LOG(log_error, "Bchg EX_NETCONNID failed: %s", Bstrerror(Berror));
		tpfree((char*)buffer);
		return;
	}

	data_send("TCPGATE_GS", buffer, (uint8_t*)&ans_msg, ans_msg.get_size());
	tpfree((char*)buffer);
}

void join_id_pass_request(UBFH *&buffer_, uint8_t *message_, uint16_t index_)
{
	muProtocol::req_id_password *req_msg = (muProtocol::req_id_password*)message_;

	if (req_msg->get_version() != M_service_config.client_stripped_version)
	{
		TP_LOG(log_error, "error-L1: version error (%s)(%s)", req_msg->get_account().c_str(), req_msg->get_version().c_str());
		gc_join_result(muProtocol::kJsLoginResult::LOGIN_BAD_CLIENT_VERSION, index_);
		close_client(buffer_, index_);
		return;
	}

	if (req_msg->get_serial() != M_service_config.client_serial)
	{
		TP_LOG(log_error, "error-L1: serial error [%s][%s]", req_msg->get_account().c_str(), req_msg->get_serial().c_str());
		gc_join_result(muProtocol::kJsLoginResult::LOGIN_BAD_CLIENT_VERSION, index_);
		close_client(buffer_, index_);
		return;
	}

	client_info *client = nullptr;

	try
	{
		client = M_clients.at(index_ - USER_START_INDEX);
	}
	catch (std::exception ex)
	{
		TP_LOG(log_error, "error-L1: user index(%ld) out of range: %s", index_, ex.what());
	}

	if (nullptr != client)
	{
		int state = client->get_connection_state();

		if (state != kClientState::CONNECTED)
		{
			TP_LOG(log_error, "error-L1:(%d) client in wrong state: %d", index_, state);
			close_client(buffer_, index_);
			return;
		}

		if (state == kClientState::LOGGING)
		{
			TP_LOG(log_error, "error-L1:(%d) client already logging-in: %d", index_, state);
			return;
		}
	}

	muProtocol::js_id_pass_req js_req_msg(req_msg->get_account(), req_msg->get_password(), index_, "127.0.0.1");
	
	if (nullptr != client)
	{
		client->set_connection_state(kClientState::LOGGING);
		client->incr_login_count();
		client->set_account_id(req_msg->get_account());
	}

	M_joinserver_manager->dispatch(joinserver_send_req, "JOINSERVER", (uint8_t*&)js_req_msg, (uint16_t)js_req_msg.get_size());
	TP_LOG(log_debug, "join send (%d) (%s)", index_, req_msg->get_account().c_str());
}

void protocol_core(UBFH *&buffer_, uint8_t header_code_, uint8_t *message_, uint16_t message_len_, uint16_t index_, bool encrypt_, int serial_)
{
	switch (header_code_)
	{
		case 0xF1:
		{
			switch (message_[3])
			{
				case 0x01:
				{
					join_id_pass_request(buffer_, message_, index_);
				}
				break;
			}
		}
		break;
		case 0xF3:
		{
			switch (message_[3])
			{
			case 0x00:
				dataserver_get_char_list_request(buffer_, index_);
				TP_LOG(log_debug, "DataServerGetCharListRequest(index_)");
				break;
			}
		}
		break;
		case 0xF4:
		{
			switch (message_[3])
			{
				case 0x06:
				{
					char ip_address[16] = { 0 };

					if (EXSUCCEED != Bget(buffer_, EX_NETTHEIRIP, 0, (char*)ip_address, 0))
					{
						TP_LOG(log_error, "Bget EX_NETTHEIRIP failed: %s", Bstrerror(Berror));
						join_result_send(buffer_, index_, 0);
					}
					else
					{
						// Add user info here ???
						join_result_send(buffer_, index_, 1);
						TP_LOG(log_debug, "Client connected [%ld]", index_);
					}
				}
				break;
			}
		}
		break;
	}
}

/**
 * 
 */
void *recv_data_parser(UBFH *&buffer_, int thread_id_)
{
	auto start = std::chrono::high_resolution_clock::now();
	TP_LOG(log_info, "%s() {", __FUNCTION__);
	tplogprintubf(log_debug, (char*)"	recv_data_parser()", buffer_);

	BFLDLEN message_len = Blen(buffer_, EX_NETDATA, 0);
	uint8_t message_data[MAX_C2_MESSAGE_SIZE] = { 0 };
	uint8_t message_header;
	bool encrypt = false;
	uint8_t serial = -1;
	long connection_id = -1;
	long user_index = -1;
	client_info *client_item = nullptr;

	if (EXSUCCEED != Bget(buffer_, EX_NETCONNID, 0, (char*)&connection_id, 0))
	{
		TP_LOG(log_error, "Bget EX_NETCONNID failed: %s", Bstrerror(Berror));
		goto out;
	}

	user_index = (USER_START_INDEX - 1) + GET_PLAIN_CONN_ID(connection_id);

	if (EXSUCCEED != Bget(buffer_, EX_NETDATA, 0, (char*)message_data, &message_len))
	{
		TP_LOG(log_error, "Bget EX_NETDATA failed: %s", Bstrerror(Berror));
		goto out;
	}

	if (message_data[0] == 0xC3)
	{
		uint8_t decrypt_data[MAX_C2_MESSAGE_SIZE] = { 0 };
		int size = M_simple_modulus_cs.decrypt(decrypt_data + 2, message_data + 2, message_len - 2);

		if (size < 0)
		{
			TP_LOG(log_error, "Decrypt failed, checksum failed. Size: %d", size);
			goto out;
		}
		else
		{
			uint8_t decrypt_serial = decrypt_data[2];
			decrypt_data[1] = 0xC1;
			decrypt_data[2] = size + 1;

			packet_engine mu_packet;
			mu_packet.clear();

			if (mu_packet.add_data(decrypt_data + 1, size + 2) == 0)
			{
				TP_LOG(log_error, "error-L1: packet_engine::add_data() error: serial(%X) (%s, %d)",
					decrypt_serial, __FILE__, __LINE__);
				goto out;
			}

			if (mu_packet.extract_packet(decrypt_data) != 0)
			{
				TP_LOG(log_error, "error-L1: packet_engine::extract_packet() error: serial(%X) (%s, %d)",
					decrypt_serial, __FILE__, __LINE__);
				goto out;
			}

			// Print decrypted packet only in dump mode
			if (G_ndrx_debug.level >= log_dump)
			{
				TP_LOG(log_dump, "Decrypted message[%02X]:", decrypt_serial);

				char message_buffer[MAX_C2_MESSAGE_SIZE] = { 0 };
				for (int i = 0; i <= size; ++i)
				{
					char temp_msg_buffer[32] = { 0 };
					snprintf(temp_msg_buffer, 32, "%02X ", decrypt_data[i]);
					strncat(message_buffer, temp_msg_buffer, 32);
				}
				TP_LOG(log_dump, "%s", message_buffer);
			}

			memcpy(message_data, decrypt_data, MAX_C2_MESSAGE_SIZE);
			message_len = size + 1;
			encrypt = true;
			serial = decrypt_serial;
		}
	}
	else if (message_data[0] == 0xC4)
	{
		uint8_t decrypt_data[MAX_C2_MESSAGE_SIZE] = { 0 };
		int size = M_simple_modulus_cs.decrypt(decrypt_data + 3, message_data + 3, message_len - 3);

		if (size < 0)
		{
			TP_LOG(log_error, "Decrypt failed, checksum failed. Size: %d", size);
			goto out;
		}
		else
		{
			uint8_t decrypt_serial = decrypt_data[3];
			decrypt_data[1] = 0xC2;
			uint16_t new_size = size + 2;
			decrypt_data[2] = new_size;
			decrypt_data[3] = new_size;

			packet_engine mu_packet;
			mu_packet.clear();

			if (mu_packet.add_data(decrypt_data + 1, size + 3) == 0)
			{
				TP_LOG(log_error, "error-L1: packet_engine::add_data() error: serial(%X) (%s, %d)",
					decrypt_serial, __FILE__, __LINE__);
				goto out;
			}

			if (mu_packet.extract_packet(decrypt_data) != 0)
			{
				TP_LOG(log_error, "error-L1: packet_engine::extract_packet() error: serial(%X) (%s, %d)",
					decrypt_serial, __FILE__, __LINE__);
				goto out;
			}

			// Print decrypted packet only in dump mode
			if (G_ndrx_debug.level >= log_dump)
			{
				TP_LOG(log_dump, "Decrypted message[%02X]:", decrypt_serial);

				char message_buffer[MAX_C2_MESSAGE_SIZE] = { 0 };
				for (int i = 0; i <= size; ++i)
				{
					char temp_msg_buffer[32] = { 0 };
					snprintf(temp_msg_buffer, 32, "%02X ", decrypt_data[i]);
					strncat(message_buffer, temp_msg_buffer, 32);
				}
				TP_LOG(log_dump, "%s", message_buffer);
			}

			memcpy(message_data, decrypt_data, MAX_C2_MESSAGE_SIZE);
			message_len = size + 2;
			encrypt = true;
			serial = decrypt_serial;
		}
	}
	else
	{
		// Skip connection message, it's not Xor'ed by default
		if (message_data[0] == 0xC1 && message_data[1] == 0x04 && message_data[2] == 0xF4 && message_data[3] == 0x06)
		{
			TP_LOG(log_debug, "Connection message received...");

			try
			{
				auto item = M_clients.at(user_index - USER_START_INDEX);

				if (item->get_connection_state() == kClientState::NONE)
				{
					TP_LOG(log_debug, "This is new connection, set client info. (%ld)(%ld)", connection_id, user_index);
					item->set_connection_info(connection_id, user_index);
				}
				else
				{
					TP_LOG(log_error, "error-L1: Connection (%ld) already exists. Disconnecting...", connection_id);
				}
			}
			catch (std::exception ex)
			{
				TP_LOG(log_error, "error-L1: user_index(%ld) was out of range: %s", user_index, ex.what());
			}
		}
		else
		{
			packet_engine mu_packet;
			mu_packet.clear();

			if (mu_packet.add_data(message_data, message_len) == 0)
			{
				TP_LOG(log_error, "error-L1: packet_engine::add_data() error: (%s, %d)", __FILE__, __LINE__);
				goto out;
			}

			if (mu_packet.extract_packet(message_data) != 0)
			{
				TP_LOG(log_error, "error-L1: packet_engine::extract_packet() error: (%s, %d)", __FILE__, __LINE__);
				goto out;
			}
		}
	}

	// Process protocol message
	message_header = (message_data[0] == 0xC1 || message_data[0] == 0xC3) ? message_data[2] : message_data[3];
	TP_LOG(log_debug, "[MS] Type: %02X, size: %d, header: %02X, id: %ld, enc: %d, serial: %d",
		message_data[0], message_len, message_header, connection_id, encrypt, serial);

	// Redirect message to world if client is logged and world_id is set
	try
	{
		client_item = M_clients.at(user_index - USER_START_INDEX);

		if (client_item->get_connection_state() == kClientState::PLAYING)
		{
			TP_LOG(log_debug, "Redirect -> WORLD_SERVER_<world_id> (0)");
		}
		else
		{
			protocol_core(buffer_, message_header, message_data, message_len, user_index);
		}
	}
	catch(std::exception ex)
	{
		TP_LOG(log_error, "error-L1: user_index(%d) out of range: %s", user_index, ex.what());
	}

out:
	auto elapsed = std::chrono::high_resolution_clock::now() - start;
	TP_LOG(log_debug, "}. Elapsed time: %ld ms.", std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());

	return 0;
}

void close_client(int index_)
{
	try
	{
		auto item = M_clients.at(index_ - USER_START_INDEX);

		// Add error checks
		UBFH *buffer = (UBFH*)tpalloc((char*)"UBF", NULL, 256);
		long connection_id = item->get_connection_id();
		Bchg(buffer, EX_NETCONNID, 0, (char*)&connection_id, 0);

		uint8_t output_msg[2] = { '\0', '\0' };
		data_send("TCPGATE_GS", buffer, (uint8_t*)&output_msg, 2);
		tpfree((char*)buffer);

		// Reset only after buffer is prepared and sended
		item->reset_client();
	}
	catch (std::exception ex)
	{
		TP_LOG(log_error, "error-L1: user_index(%ld) out of range: %s", index_, ex.what());
	}
}

void close_client(UBFH *&buffer, int index_)
{
	try
	{
		auto item = M_clients.at(index_ - USER_START_INDEX);
		item->reset_client();

		uint8_t output_msg[2] = { '\0', '\0' };
		data_send("TCPGATE_GS", buffer, (uint8_t*)&output_msg, 2);
	}
	catch (std::exception ex)
	{
		TP_LOG(log_error, "error-L1: user_index(%ld) out of range: %s", index_, ex.what());
	}
}

int data_send(const std::string &svc_name_, UBFH *&buffer_, uint8_t *msg_, uint16_t msg_size_, uint16_t message_type_)
{
	if (NULL != msg_)
	{
		uint8_t *send_buffer;
		uint8_t *ex_send_buffer = new uint8_t[MAX_C2_MESSAGE_SIZE];

		if (msg_[0] == 0xC3 || msg_[0] == 0xC4)
		{
			long connection_id = -1;

			if (EXSUCCEED != Bget(buffer_, EX_NETCONNID, 0, (char*)&connection_id, 0))
			{
				TP_LOG(log_error, "Bget EX_NETCONNID failed: %s", Bstrerror(Berror));
				delete[] ex_send_buffer;
				return 0;
			}

			int index = GET_PLAIN_CONN_ID(connection_id);
			int enc_size;
			int8_t original_len;

			if (msg_[0] == 0xC3)
			{
				original_len = msg_[1];
				msg_[1] = 0; // serial

				enc_size = M_simple_modulus_sc.encrypt(ex_send_buffer + 2, msg_ + 1, msg_size_ - 1);
				ex_send_buffer[0] = 0xC3;
				ex_send_buffer[1] = enc_size + 2;
				send_buffer = ex_send_buffer;
				msg_size_ = enc_size + 2;
				msg_[1] = original_len;
			}
			else
			{
				original_len = msg_[2];
				msg_[2] = 0; // serial

				enc_size = M_simple_modulus_sc.encrypt(ex_send_buffer + 3, msg_ + 2, msg_size_ - 2);
				ex_send_buffer[0] = 0xC4;
				ex_send_buffer[1] = (enc_size + 3) / 256;
				ex_send_buffer[2] = (enc_size + 3) % 256;
				send_buffer = ex_send_buffer;
				msg_size_ = enc_size + 3;
				msg_[2] = original_len;
			}
		}
		else
		{
			send_buffer = (uint8_t*)msg_;
		}

		if (EXSUCCEED != Bchg(buffer_, EX_NETDATA, 0, (char*)send_buffer, msg_size_))
		{
			if (BNOSPACE == Berror)
			{
				long old_size = Bsizeof(buffer_);
				buffer_ = (UBFH*)tprealloc((char*)buffer_, old_size * 2);
				TP_LOG(log_debug, "Buffer (%p) tprealloc(%ld -> %ld)", buffer_, old_size, old_size * 2);

				if (NULL == buffer_)
				{
					TP_LOG(log_error, "tprealloc() failed: %s", Bstrerror(Berror));
					delete[] ex_send_buffer;
					return 0;
				}

				if (EXSUCCEED != Bchg(buffer_, EX_NETDATA, 0, (char*)send_buffer, msg_size_))
				{
					TP_LOG(log_error, "Bchg EX_NETDATA failed: %s", Bstrerror(Berror));
					delete[] ex_send_buffer;
					return 0;
				}
			}
			else
			{
				TP_LOG(log_error, "Bchg EX_NETDATA failed: %s", Bstrerror(Berror));
				delete[] ex_send_buffer;
				return 0;
			}
		}

		delete[] ex_send_buffer;
	}

	if (EXSUCCEED != Bchg(buffer_, MU_MESSAGE_TYPE, 0, (char*)&message_type_, 0))
	{
		TP_LOG(log_error, "Bchg MU_MESSAGE_TYPE failed: %s", Bstrerror(Berror));
		return 0;
	}

	if (G_tp_debug.level >= log_debug)
	{
		char log_title[128] = { 0 };
		snprintf(log_title, 128, "data_send(%s)", svc_name_.c_str());
		tplogprintubf(log_debug, log_title, buffer_);
	}

	return tpacall((char*)svc_name_.c_str(), (char*)buffer_, 0, TPNOREPLY);
}