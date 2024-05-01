#ifndef _config_h_
#define _config_h_

#include <string>

class service_config
{
public:
	// Service parameters
	std::string client_version;
	std::string client_stripped_version;
	std::string client_serial;
	std::string mainserver_name;
	std::string mainserver_status_name;
	std::string joinserver_name;
	std::string dataserver_name;
	std::string tcpgate_name;

public:
	service_config();
	~service_config();
};

extern service_config M_service_config;

#endif