#include <tinyxml2.h>
#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <Exfields.h>

#include <string>
#include <chrono>
#include <vector>
#include <map>

#include "driver.h"
#include "muprotocol.h"
#include "utils.h"
#include "protocol/protocol.h"

/**
 * 
 */
void DATASERVER_SVC(TPSVCINFO *p_svc_)
{
	int ret = EXSUCCEED;
	UBFH *p_ub = (UBFH*)p_svc_->data;
	auto start = std::chrono::high_resolution_clock::now();
	TP_LOG(log_info, "%s() {", __FUNCTION__);

	tplogprintubf(log_debug, (char*)"	DATASERVER_SVC:", p_ub);

	BFLDLEN message_len = Blen(p_ub, EX_NETDATA, 0);
	uint8_t message_data[MAX_C2_MESSAGE_SIZE] = { 0 };
	uint8_t message_header = 0;

	if (EXSUCCEED != Bget(p_ub, EX_NETDATA, 0, (char*)message_data, &message_len))
	{
		TP_LOG(log_error, "Bget EX_NETDATA failed: %s", Bstrerror(Berror));
		goto out;
	}

	message_header = (message_data[0] == 0xC1) ? message_data[2] : message_data[3];
	protocol_core(p_ub, message_header, message_data, message_len);

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

	M_db_handle = new pg_driver("127.0.0.1", "muonline", "muonline", "muonline1!", "");
	M_db_handle->connect();

	if (EXSUCCEED != tpadvertise_full((char*)"DATASERVER", DATASERVER_SVC, (char*)"DATASERVER"))
	{
		TP_LOG(log_error, "	Failed to advertise 'DATASERVER': %s", tpstrerror(tperrno));
		return EXFAIL;
	}

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

	/*if (nullptr != M_db_handle)
	{
		M_db_handle->disconnect();
		delete M_db_handle;
	}*/

	TP_LOG(log_always, "	Service terminated successfully.");

	auto elapsed = std::chrono::high_resolution_clock::now() - start;
	TP_LOG(log_info, "}. Elapsed time: %ld ms.", std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
}