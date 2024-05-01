#include <tinyxml2.h>
#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <string>
#include <chrono>
#include <vector>
#include <map>

#include <Exfields.h>
#include "MuOnline.fd.h"

#include "utils.h"
#include "muprotocol.h"
#include "packet_engine.h"
#include "config/config.h"
#include "protocol/protocol.h"
#include "simple_modulus/simple_modulus.h"
#include "thread_manager/thread_manager.h"
#include "reply_manager/reply_manager.h"
#include "client_info/client_info.h"

/**
 * Defines, variable definition
 */
simple_modulus M_simple_modulus_cs;
simple_modulus M_simple_modulus_sc;
thread_manager *M_protocol_manager = nullptr;
reply_manager *M_joinserver_manager = nullptr;
reply_manager *M_dataserver_manager = nullptr;

/**
 * 
 */
void MAIN_STATUS_SVC(TPSVCINFO *p_svc_)
{
	int ret = EXSUCCEED;
	UBFH *p_ub = (UBFH*)p_svc_->data;
	auto start = std::chrono::high_resolution_clock::now();
	TP_LOG(log_info, "%s() {", __FUNCTION__);

	tplogprintubf(log_debug, (char*)"	MAIN_STATUS_SVC:", p_ub);

	// This is here, because tpreturn() will call a long jump, thus variables will not call destructors properly
	{
		long connection_id = -1;
		char net_flags[128] = { 0 };
		BFLDLEN net_flags_len = 128;

		if (EXSUCCEED != Bget(p_ub, EX_NETCONNIDCOMP, 0, (char*)&connection_id, 0))
		{
			TP_LOG(log_error, "Bget EX_NETCONNIDCOMP failed: %s", Bstrerror(Berror));
			goto out;
		}

		if (EXSUCCEED != Bget(p_ub, EX_NETFLAGS, 0, net_flags, &net_flags_len))
		{
			TP_LOG(log_error, "Bget EX_NETFLAGS failed: %s", Bstrerror(Berror));
			goto out;
		}

		if (-1 != connection_id && 'D' == net_flags[0])
		{
			long user_index = (USER_START_INDEX - 1) + GET_PLAIN_CONN_ID(connection_id);
			TP_LOG(log_error, "Reset connection (%ld)(%ld) in client list.", connection_id, user_index);

			try
			{
				auto item = M_clients.at(user_index - USER_START_INDEX);
				item->reset_client();
			}
			catch (std::exception ex)
			{
				TP_LOG(log_error, "error-L1: user_index(%ld) out of range: %s", user_index, ex.what());
			}
		}
	}

out:
	auto elapsed = std::chrono::high_resolution_clock::now() - start;
	TP_LOG(log_debug, "}. Elapsed time: %ld ms.", std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());

	tpreturn(ret == EXSUCCEED ? TPSUCCESS : TPFAIL, 0, (char*)p_ub, Bused(p_ub), 0);
}

/**
 * 
 */
void MAIN_MESSAGE_SVC(TPSVCINFO *p_svc_)
{
	int ret = EXSUCCEED;
	UBFH *p_ub = (UBFH*)p_svc_->data;
	auto start = std::chrono::high_resolution_clock::now();
	TP_LOG(log_info, "%s() {", __FUNCTION__);

	tplogprintubf(log_debug, (char*)"	MAIN_MESSAGE_SVC:", p_ub);

	// This is here, because tpreturn() will call a long jump, thus variables will not call destructors properly
	{
		short mu_message_type = 0;

		if (EXSUCCEED != Bget(p_ub, MU_MESSAGE_TYPE, 0, (char*)&mu_message_type, 0))
		{
			if (BNOTPRES != Berror)
			{
				TP_LOG(log_error, "error-L1: Can't get message type, failed with: %s", Bstrerror(Berror));
				ret = EXFAIL;
				goto out;
			}
		}

		switch (mu_message_type)
		{
			case muProtocol::kMessageType::MAINSERVER_TYPE:
				TP_LOG(log_debug, "[C -> G] Message received");
				M_protocol_manager->dispatch(recv_data_parser, p_ub);
				break;
			default:
				TP_LOG(log_warn, "Undefined message type: %d", mu_message_type);
				break;
		}
	}

out:
	auto elapsed = std::chrono::high_resolution_clock::now() - start;
	TP_LOG(log_debug, "}. Elapsed time: %ld ms.", std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());

	tpreturn(ret == EXSUCCEED ? TPSUCCESS : TPFAIL, 0, (char*)p_ub, Bused(p_ub), 0);
}

/**
 * This is called on service startup
 */
int tpsvrinit(int argc_, char **argv_)
{
	TP_LOG(log_info, "%s() {", __FUNCTION__);
	auto start = std::chrono::high_resolution_clock::now();

	// Load configuration
	TP_LOG(log_debug, "	Client version: %s", M_service_config.client_version.c_str());
	TP_LOG(log_debug, "	Client serial: %s", M_service_config.client_serial.c_str());

	if (false == M_simple_modulus_cs.load_decryption_key(std::string(NDRX_HOMEPATH) + "/protocol/dec1.dat"))
	{
		TP_LOG(log_error, "	'dec1.dat' file loading error.");
		return EXFAIL;
	}

	if (false == M_simple_modulus_sc.load_encryption_key(std::string(NDRX_HOMEPATH) + "/protocol/enc2.dat"))
	{
		TP_LOG(log_error, "	'enc2.dat' file loading error.");
		return EXFAIL;
	}

	// Initialize clients vector
	for (int i = 0; i < MAX_USER; i++)
	{
		M_clients.push_back(new client_info());
	}

	// Create threads
	M_protocol_manager = new thread_manager("Received data parser", 4);
	M_joinserver_manager = new reply_manager("JoinServer manager", 2);
	M_dataserver_manager = new reply_manager("DataServer manager", 2);

	if (EXSUCCEED != tpadvertise_full((char*)M_service_config.mainserver_name.c_str(), MAIN_MESSAGE_SVC, (char*)M_service_config.mainserver_name.c_str()))
	{
		TP_LOG(log_error, "	Failed to advertise '%s': %s", M_service_config.mainserver_name.c_str(), tpstrerror(tperrno));
		return EXFAIL;
	}

	if (EXSUCCEED != tpadvertise_full((char*)M_service_config.mainserver_status_name.c_str(), MAIN_STATUS_SVC, (char*)M_service_config.mainserver_status_name.c_str()))
	{
		TP_LOG(log_error, "	Failed to advertise '%s': %s", M_service_config.mainserver_status_name.c_str(), tpstrerror(tperrno));
		return EXFAIL;
	}

	TP_LOG(log_always, "	Client info size: %d", M_clients.size());
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

	if (nullptr != M_protocol_manager)
	{
		delete M_protocol_manager;
	}

	if (nullptr != M_joinserver_manager)
	{
		delete M_joinserver_manager;
	}

	if (nullptr != M_dataserver_manager)
	{
		delete M_dataserver_manager;
	}

	TP_LOG(log_always, "	Service terminated successfully.");

	auto elapsed = std::chrono::high_resolution_clock::now() - start;
	TP_LOG(log_info, "}. Elapsed time: %ld ms.", std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
}