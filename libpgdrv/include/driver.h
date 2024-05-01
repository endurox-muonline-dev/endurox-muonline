#ifndef _driver_h_
#define _driver_h_

#include <mutex>
#include <string>
#include <vector>
#include <libpq-fe.h>

class pg_driver {
	std::string address;
	std::string db_name;
	std::string login;
	std::string password;
	std::string schema;

	PGconn *connection_handle;
	bool reconnect_enabled;
	PGresult *select_result;
	std::mutex mutexTransLock;

public:
	pg_driver(const std::string &address_, const std::string &db_name_, const std::string &login_,
		const std::string &password_, const std::string &schema_) : address(address_), db_name(db_name_),
		login(login_), password(password_), schema(schema_), connection_handle(nullptr), reconnect_enabled(false),
		select_result(nullptr)
	{
		TP_LOG(log_debug, "%s() server=%s, db=%s, login=%s, schema=%s", __FUNCTION__, address_.c_str(), db_name_.c_str(), login_.c_str(),
			schema_.c_str());
	}

	~pg_driver()
	{
		TP_LOG(log_debug, "%s() object destructor", __FUNCTION__);

		if (nullptr != select_result)
		{
			PQclear(select_result);
			select_result = nullptr;
		}

		if (nullptr != connection_handle)
		{
			PQfinish(connection_handle);
			connection_handle = nullptr;
		}
	}
	
	void connect();
	void disconnect();
	bool reconnect();
	void enable_reconnect(bool state_);

	bool query(const std::string &query_);
	bool select(const std::string &query_);
	void free_result();
	int get_num_rows();
	std::string get_field(int row_, int column_);
	int32_t get_field_int(int row_, int column_);
	long get_field_long(int row_, int column_);
	float get_field_float(int row_, int column_);
	std::vector<uint8_t> get_field_array(int row_, int column_);

protected:
	bool unsafe_query(const std::string &query_);
	bool unsafe_select(const std::string &query_);
};

#endif