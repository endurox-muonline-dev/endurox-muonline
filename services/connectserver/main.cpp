#include <tinyxml2.h>
#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <Exfields.h>

#include <string>
#include <chrono>
#include <vector>
#include <map>

#include "utils.h"
#include "muprotocol.h"

/**
 * Structure for server info (probably type is missing)
 */
struct s_main_server_info {
	std::string name;
	std::string address;
	int port = -1;
	int id = -1;
	bool visible = false;
};

std::map<int, s_main_server_info> M_server_list;

/**
 * 
 */
void get_server_info(uint8_t *message_data_, UBFH *buffer_)
{
	muProtocol::req_server_info *req_msg = (muProtocol::req_server_info*)message_data_;

	auto it = M_server_list.find(req_msg->get_server_index());

	if (it == M_server_list.end())
	{
		TP_LOG(log_error, "	Server index: %d not found.", req_msg->get_server_index());
		return;
	}

	muProtocol::ans_server_info ans_msg(it->second.address.c_str(), (uint16_t)it->second.port);

	if (EXSUCCEED != Bchg(buffer_, EX_NETDATA, 0, (char*)&ans_msg, ans_msg.get_size()))
	{
		TP_LOG(log_error, "	Bchg EX_NETDATA failed: %s.", Bstrerror(Berror));
		return;
	}

	tplogprintubf(log_debug, (char*)"	get_server_info:", buffer_);
	tpacall((char*)"TCPGATE_CS", (char*)buffer_, Bused(buffer_), TPNOREPLY);
}

/**
 * 
 */
void get_server_list(UBFH *buffer_)
{
	std::vector<uint8_t> message_output(MAX_C2_MESSAGE_SIZE);
	uint8_t *message_data = message_output.data();

	int message_offset = sizeof(muProtocol::send_server_list);
	int count = 0;

	for (auto it : M_server_list)
	{
		if (it.second.visible == false)
		{
			continue;
		}

		muProtocol::cs_server_data msg_body(it.second.id, 0, 0);
		memcpy(&message_data[message_offset], &msg_body, sizeof(msg_body));
		message_offset += sizeof(msg_body);
		count++;
	}

	muProtocol::send_server_list ans_msg(count);
	ans_msg.set_size(message_offset); // Set final message size
	memcpy(&message_data[0], &ans_msg, sizeof(ans_msg));

	if (EXSUCCEED != Bchg(buffer_, EX_NETDATA, 0, (char*)message_data, message_offset))
	{
		TP_LOG(log_error, "	Bchg EX_NETDATA failed: %s.", Bstrerror(Berror));
		return;
	}

	tplogprintubf(log_debug, (char*)"	get_server_list:", buffer_);
	tpacall((char*)"TCPGATE_CS", (char*)buffer_, Bused(buffer_), TPNOREPLY);
}

/**
 * 
 */
void MUCS_STATUS_SVC(TPSVCINFO *p_svc_)
{
	int ret = EXSUCCEED;
	UBFH *p_ub = (UBFH*)p_svc_->data;
	auto start = std::chrono::high_resolution_clock::now();
	TP_LOG(log_info, "%s() {", __FUNCTION__);

	tplogprintubf(log_debug, (char*)"	MUCS_STATUS_SVC:", p_ub);

out:
	auto elapsed = std::chrono::high_resolution_clock::now() - start;
	TP_LOG(log_debug, "}. Elapsed time: %ld ms.", std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());

	tpreturn(ret == EXSUCCEED ? TPSUCCESS : TPFAIL, 0, (char*)p_ub, Bused(p_ub), 0);
}

/**
 * 
 */
void MUCS_MESSAGE_SVC(TPSVCINFO *p_svc_)
{
	int ret = EXSUCCEED;
	UBFH *p_ub = (UBFH*)p_svc_->data;
	auto start = std::chrono::high_resolution_clock::now();
	TP_LOG(log_info, "%s() {", __FUNCTION__);

	tplogprintubf(log_debug, (char*)"	MUCS_MESSAGE_SVC:", p_ub);

	// This is here, because tpreturn() will call a long jump, thus variables will not call destructors properly
	{
		BFLDLEN message_len = Blen(p_ub, EX_NETDATA, 0);
		std::vector<uint8_t> message_vector(MAX_C2_MESSAGE_SIZE);
		uint8_t *message_data = message_vector.data();
		uint8_t message_header;

		if (EXSUCCEED != Bget(p_ub, EX_NETDATA, 0, (char*)message_data, &message_len))
		{
			TP_LOG(log_error, "Bget EX_NETDATA failed: %s.", Bstrerror(Berror));
			ret = EXFAIL;
			goto out;
		}

		message_header = (message_data[0] == 0xC1 || message_data[0] == 0xC3) ? message_data[2] : message_data[3];
		TP_LOG(log_debug, "	Message received, type: %02X, size: %d, header: %02X", message_data[0], message_len, message_header);

		switch (message_header)
		{
			case 0xF4:
			{
				TP_LOG(log_debug, "	Client request, subcode [%02X]", message_data[3]);

				switch (message_data[3])
				{
					case 0x03:
					{
						TP_LOG(log_debug, "	Get server info");
						get_server_info(message_data, p_ub);
					}
					break;
					case 0x06:
					{
						TP_LOG(log_debug, "	Get servers list");
						get_server_list(p_ub);
					}
					break;
				}
			}
			break;
			default:
			{
				TP_LOG(log_warn, "	Received unknown header [%02X]", message_header);
			}
			break;
		}
	}

out:
	auto elapsed = std::chrono::high_resolution_clock::now() - start;
	TP_LOG(log_debug, "}. Elapsed time: %ld ms.", std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());

	tpreturn(ret == EXSUCCEED ? TPSUCCESS : TPFAIL, 0, (char*)p_ub, Bused(p_ub), 0);
}

bool load_server_list_configuration()
{
	std::string server_list_file = NDRX_HOMEPATH;
	server_list_file += "/settings/serverlist.xml";

	// Load server list
	tinyxml2::XMLDocument server_list;
	server_list.LoadFile(server_list_file.c_str());

	if (server_list.Error())
	{
		TP_LOG(log_error, "	Error loading '%s' file.", server_list_file.c_str());
		return false;
	}

	tinyxml2::XMLElement *list_element = server_list.FirstChild()->FirstChildElement("server");

	while (NULL != list_element)
	{
		s_main_server_info server_info;
		list_element->QueryIntAttribute("id", &server_info.id);
		list_element->QueryIntAttribute("port", &server_info.port);
		server_info.name = list_element->Attribute("name");
		server_info.address = list_element->Attribute("address");
		std::string server_status = list_element->Attribute("status");

		if (server_status == "SHOW")
		{
			server_info.visible = true;
		}
		else
		{
			server_info.visible = false;
		}

		TP_LOG(log_debug, "	Server info parsed -> name: %s, address: %s, port: %d, id: %d, visible: %d",
			server_info.name.c_str(), server_info.address.c_str(), server_info.port, server_info.id, server_info.visible);

		M_server_list.insert({ server_info.id, server_info });
		list_element = list_element->NextSiblingElement("server");
	}

	return true;
}

/**
 * This is called on service startup
 */
int tpsvrinit(int argc_, char **argv_)
{
	TP_LOG(log_info, "%s() {", __FUNCTION__);
	auto start = std::chrono::high_resolution_clock::now();

	if (!load_server_list_configuration())
	{
		TP_LOG(log_error, "	Failed to load server list configuration.");
		return EXFAIL;
	}

	if (EXSUCCEED != tpadvertise_full((char*)"MUCS_MESSAGE_SVC", MUCS_MESSAGE_SVC, (char*)"MUCS_MESSAGE_SVC"))
	{
		TP_LOG(log_error, "	Failed to advertise 'MUCS_MESSAGE_SVC': %s", tpstrerror(tperrno));
		return EXFAIL;
	}

	if (EXSUCCEED != tpadvertise_full((char*)"MUCS_STATUS_SVC", MUCS_STATUS_SVC, (char*)"MUCS_STATUS_SVC"))
	{
		TP_LOG(log_error, "	Failed to advertise 'MUCS_STATUS_SVC': %s", tpstrerror(tperrno));
		return EXFAIL;
	}

	TP_LOG(log_debug, "	Server list size: %d", M_server_list.size());
	TP_LOG(log_always, "	Service started successfully.");

	auto elapsed = std::chrono::high_resolution_clock::now() - start;
	TP_LOG(log_info, "}. Elapsed time: %ld ms.", std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
	return EXSUCCEED;
}

/**
 * This is called on service exit
 */
void tpsvrdone()
{
	TP_LOG(log_info, "%s() {", __FUNCTION__);
	auto start = std::chrono::high_resolution_clock::now();

	TP_LOG(log_always, "	Service terminated successfully.");

	auto elapsed = std::chrono::high_resolution_clock::now() - start;
	TP_LOG(log_info, "}. Elapsed time: %ld ms.", std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
}