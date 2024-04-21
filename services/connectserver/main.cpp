#include <tinyxml2.h>
#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <Exfields.h>

#include <string>
#include <vector>
#include <map>

#include "muprotocol.h"

/**
 * Defines, variable definition
 */
#define NDRX_HOMEPATH	getenv("NDRX_APPHOME")

/**
 * Structure for server info (probably type is missing)
 */
struct MainServerInfo {
	std::string name;
	std::string address;
	int port = -1;
	int id = -1;
	bool visible = false;
};

std::map<int, MainServerInfo> M_serverList;

/**
 * 
 */
void CLGetServerInfo(uint8_t *message_data_, UBFH *buffer_)
{
	muProcotol::CMsgReqServerInfo *req_msg = (muProcotol::CMsgReqServerInfo*)message_data_;

	auto it = M_serverList.find(req_msg->get_server_index());

	if (it == M_serverList.end())
	{
		TP_LOG(log_error, "Server index: %d not found.", req_msg->get_server_index());
		return;
	}

	muProcotol::CMsgAnsServerInfo ans_msg(it->second.address.c_str(), (uint16_t)it->second.port);

	if (EXSUCCEED != Bchg(buffer_, EX_NETDATA, 0, (char*)&ans_msg, ans_msg.get_size()))
	{
		TP_LOG(log_error, "Bchg EX_NETDATA failed: %s.", Bstrerror(Berror));
		return;
	}

	tplogprintubf(log_debug, (char*)"CLGetServerInfo:", buffer_);
	tpacall((char*)"TCPGATE_CS", (char*)buffer_, Bused(buffer_), TPNOREPLY);
}

/**
 * 
 */
void CLGetServerList(UBFH *buffer_)
{
	std::vector<uint8_t> message_output(MAX_C2_MESSAGE_SIZE);
	uint8_t *message_data = message_output.data();

	int message_offset = sizeof(muProcotol::CMsgSendServerList);
	int count = 0;

	for (auto it : M_serverList)
	{
		if (it.second.visible == false)
		{
			continue;
		}

		muProcotol::CMsgCSServerData msg_body(it.second.id, 0, 0);
		memcpy(&message_data[message_offset], &msg_body, sizeof(msg_body));
		message_offset += sizeof(msg_body);
		count++;
	}

	muProcotol::CMsgSendServerList ans_msg(count);
	ans_msg.set_size(message_offset); // Set final message size
	memcpy(&message_data[0], &ans_msg, sizeof(ans_msg));

	if (EXSUCCEED != Bchg(buffer_, EX_NETDATA, 0, (char*)message_data, message_offset))
	{
		TP_LOG(log_error, "Bchg EX_NETDATA failed: %s.", Bstrerror(Berror));
		return;
	}

	tplogprintubf(log_debug, (char*)"CLGetServerList:", buffer_);
	tpacall((char*)"TCPGATE_CS", (char*)buffer_, Bused(buffer_), TPNOREPLY);
}

/**
 * 
 */
void MUCS_MESSAGE_SVC(TPSVCINFO *p_svc_)
{
	int ret = EXSUCCEED;
	UBFH *p_ub = (UBFH*)p_svc_->data;

	tplogprintubf(log_debug, (char*)"CS_MESSAGE_SVC:", p_ub);

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
		TP_LOG(log_debug, "Packet received, type: %02X, size: %d, header: %02X", message_data[0], message_len, message_header);

		switch (message_header)
		{
			case 0x05:
			{
				TP_LOG(log_debug, "Version receive");
			}
			break;
			case 0x06:
			{
				TP_LOG(log_debug, "Version HTTP receive");
			}
			break;
			case 0xF4:
			{
				TP_LOG(log_debug, "Client request, subcode [%02X]", message_data[3]);

				switch (message_data[3])
				{
					case 0x03:
					{
						TP_LOG(log_debug, "Get server info");
						CLGetServerInfo(message_data, p_ub);
					}
					break;
					case 0x06:
					{
						TP_LOG(log_debug, "Get servers list");
						CLGetServerList(p_ub);
					}
					break;
					case 0x07:
					{
						TP_LOG(log_debug, "Get server (group) list");
					}
					break;
				}
			}
			break;
			default:
			{
				TP_LOG(log_warn, "Received unknown header [%02X]", message_header);
			}
			break;
		}
	}

out:
	tpreturn(ret == EXSUCCEED ? TPSUCCESS : TPFAIL, 0, (char*)p_ub, Bused(p_ub), 0);
}

bool LoadServerListConfiguration()
{
	std::string serverListFile = NDRX_HOMEPATH;
	serverListFile += "/settings/serverlist.xml";

	// Load server list
	tinyxml2::XMLDocument serverList;
	serverList.LoadFile(serverListFile.c_str());

	if (serverList.Error())
	{
		TP_LOG(log_error, "Error loading '%s' file.", serverListFile.c_str());
		return false;
	}

	tinyxml2::XMLElement *pListElement = serverList.FirstChild()->FirstChildElement("server");

	while (NULL != pListElement)
	{
		MainServerInfo serverInfo;
		pListElement->QueryIntAttribute("id", &serverInfo.id);
		pListElement->QueryIntAttribute("port", &serverInfo.port);
		serverInfo.name = pListElement->Attribute("name");
		serverInfo.address = pListElement->Attribute("address");
		std::string serverStatus = pListElement->Attribute("status");

		if (serverStatus == "SHOW")
		{
			serverInfo.visible = true;
		}
		else
		{
			serverInfo.visible = false;
		}

		TP_LOG(log_debug, "Server object created - name: %s, address: %s, port: %d, id: %d, visible: %d", 
			serverInfo.name.c_str(), serverInfo.address.c_str(), serverInfo.port, serverInfo.id, serverInfo.visible);

		M_serverList.insert({ serverInfo.id, serverInfo });
		pListElement = pListElement->NextSiblingElement("server");
	}

	return true;
}

/**
 * This is called on service startup
 */
int tpsvrinit(int argc_, char **argv_)
{
	if (!LoadServerListConfiguration())
	{
		TP_LOG(log_error, "Failed to load server list configuration.");
		return EXFAIL;
	}

	if (EXSUCCEED != tpadvertise_full((char*)"MUCS_MESSAGE_SVC", MUCS_MESSAGE_SVC, (char*)"MUCS_MESSAGE_SVC"))
	{
		TP_LOG(log_error, "Failed to advertise 'MUCS_MESSAGE_SVC': %s", tpstrerror);
		return EXFAIL;
	}

	TP_LOG(log_debug, "Server object count: %d", M_serverList.size());
	TP_LOG(log_always, "Service started successfully.");
	return EXSUCCEED;
}

/**
 * This is called on service exit
 */
void tpsvrdone()
{
	TP_LOG(log_always, "Service terminated successfully.");
}