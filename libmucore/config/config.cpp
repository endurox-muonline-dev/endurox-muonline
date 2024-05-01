#include <ndebug.h>
#include "config.h"
#include "utils.h"

service_config M_service_config;

service_config::service_config()
{
	client_version = read_config_str("ClientInfo", "Version", "1.00.00");
	client_stripped_version = strip_characters(client_version, '.');

	client_serial = read_config_str("ClientInfo", "Serial", "123456789012345");

	mainserver_name = read_config_str("ServiceDistribution", "MainServerSvc", "MAIN_MESSAGE_SVC");
	mainserver_status_name = read_config_str("ServiceDistribution", "MainServerStatusSvc", "MAIN_STATUS_SVC");
	joinserver_name = read_config_str("ServiceDistribution", "JoinServerSvc", "JOINSERVER");
	dataserver_name = read_config_str("ServiceDistribution", "DataServerSvc", "DATASERVER");
	tcpgate_name = read_config_str("ServiceDistribution", "TcpGateSvc", "TCPGATE_GS");
}

service_config::~service_config()
{

}