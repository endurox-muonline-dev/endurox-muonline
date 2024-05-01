#ifndef _client_info_h_
#define _client_info_h_

#include <mutex>
#include <vector>

enum kClientState {
	NONE = 0,
	CONNECTED,
	LOGGING,
	LOGGED,
	PLAYING
};

class client_info
{
	int state;
	long connection_id;
	long user_index;
	int world_id;
	std::mutex mtx;

	std::string account_id;
	int login_send_count;

public:
	client_info() : state(0), connection_id(0), user_index(0), world_id(-1) {
		login_send_count = 0;
	}

	~client_info() {
	}

	void reset_client();

	int get_connection_state();
	void set_connection_state(enum kClientState state_);

	void set_connection_info(long connection_id_, long user_index_);
	long get_connection_id();

	std::string get_account_id();
	void set_account_id(const std::string &account_id_);
	
	void incr_login_count();
	int get_login_send_count();
};

extern std::vector<client_info*> M_clients;

#endif