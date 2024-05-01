#include <tinyxml2.h>
#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <Exfields.h>

#include <string>
#include <chrono>
#include <vector>
#include <map>

/**
 * This is called on service startup
 */
int tpsvrinit(int argc_, char **argv_)
{
	TP_LOG(log_info, "%s() {", __FUNCTION__);
	auto start = std::chrono::high_resolution_clock::now();

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