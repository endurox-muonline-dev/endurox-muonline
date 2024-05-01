#include "client_info/client_info.h"

std::vector<client_info*> M_clients;

void client_info::reset_client()
{
	std::lock_guard<std::mutex> lock(mtx);

	state = kClientState::NONE;
	connection_id = 0;
	user_index = 0;
	world_id = -1;

	account_id.clear();
	login_send_count = 0;
}

int client_info::get_connection_state()
{
	std::lock_guard<std::mutex> lock(mtx);

	return state;
}

void client_info::set_connection_state(enum kClientState state_)
{
	std::lock_guard<std::mutex> lock(mtx);

	state = state_;
}

void client_info::set_connection_info(long connection_id_, long user_index_)
{
	std::lock_guard<std::mutex> lock(mtx);

	connection_id = connection_id_;
	user_index = user_index_;
	state = kClientState::CONNECTED;
}

long client_info::get_connection_id()
{
	std::lock_guard<std::mutex> lock(mtx);

	return connection_id;
}

std::string client_info::get_account_id()
{
	std::lock_guard<std::mutex> lock(mtx);

	return account_id;
}

void client_info::set_account_id(const std::string &account_id_)
{
	std::lock_guard<std::mutex> lock(mtx);

	account_id = account_id_;
}

void client_info::incr_login_count()
{
	std::lock_guard<std::mutex> lock(mtx);

	login_send_count++;
}

int client_info::get_login_send_count()
{
	std::lock_guard<std::mutex> lock(mtx);

	return login_send_count;
}