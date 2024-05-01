#include <ndebug.h>

#include "driver.h"
#include "utils.h"

void pg_driver::connect()
{
	if (db_name.empty())
	{
		TP_LOG(log_error, "Database name is empty");
		return;
	}

	if (address.empty())
	{
		TP_LOG(log_error, "Host name is empty");
		return;
	}

	std::vector<std::string> address_split = split(address, ':');
	std::string host;
	std::string port;

	if (address_split.size() > 1)
	{
		host = address_split[0];
		port = address_split[1];
	}
	else
	{
		host = address_split[0];
	}

	connection_handle = PQsetdbLogin(host.c_str(), port.c_str(), nullptr, nullptr, db_name.c_str(), login.c_str(), password.c_str());

	if (CONNECTION_BAD != PQstatus(connection_handle))
	{
		PGresult *result = PQexec(connection_handle, "SET standard_conforming_strings TO off");
		PQclear(result);

		result = PQexec(connection_handle, "SET escape_string_warning TO off");
		PQclear(result);

		PQsetClientEncoding(connection_handle, "UTF8");

		if (!schema.empty())
		{
			std::string query = "SET search_path=" + schema;

			if (!unsafe_query(query))
			{
				PQfinish(connection_handle);
			}
		}

		TP_LOG(log_debug, "PG connection successful.");
	}
	else
	{
		TP_LOG(log_error, "PG connection failed: %s", PQerrorMessage(connection_handle));
		PQfinish(connection_handle);
		connection_handle = nullptr;
	}

	if (nullptr != connection_handle)
	{
		reconnect_enabled = true;
	}
}

void pg_driver::disconnect()
{
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

void pg_driver::enable_reconnect(bool state_)
{
	reconnect_enabled = state_;
}

bool pg_driver::reconnect()
{
	disconnect();
	connect();

	if (nullptr != connection_handle)
	{
		TP_LOG(log_warn, "%s() successfull.", __FUNCTION__);
		return true;
	}

	TP_LOG(log_error, "%s() failed.", __FUNCTION__);
	return false;
}

bool pg_driver::unsafe_query(const std::string &query_)
{
	int retry_count = 10;

retry:
	PGresult *result = PQexec(connection_handle, query_.c_str());

	if (NULL == result)
	{
		TP_LOG(log_error, "%s() internal error. PQexec failed: %s", __FUNCTION__, PQerrorMessage(connection_handle));
		return false;
	}

	if (PGRES_COMMAND_OK != PQresultStatus(result))
	{
		const char *sql_state = PQresultErrorField(result, PG_DIAG_SQLSTATE);

		if ((CONNECTION_BAD != PQstatus(connection_handle)) &&
			(NULL != sql_state) && (!strcmp(sql_state, "53000") || !strcmp(sql_state, "53200")) && (retry_count > 0))
		{
			usleep(1000000);
			retry_count--;
			PQclear(result);
			goto retry;
		}
		else
		{
			TP_LOG(log_error, "%s() error(%s): %s", __FUNCTION__, sql_state, PQerrorMessage(connection_handle));
		}

		PQclear(result);
		return false;
	}

	PQclear(result);
	return true;
}

bool pg_driver::unsafe_select(const std::string &query_)
{
	int retry_count = 10;

retry:
	PGresult *result = PQexec(connection_handle, query_.c_str());

	if ((PGRES_COMMAND_OK != PQresultStatus(result)) && (PGRES_TUPLES_OK != PQresultStatus(result)))
	{
		const char *sql_state = PQresultErrorField(result, PG_DIAG_SQLSTATE);

		if ((CONNECTION_BAD != PQstatus(connection_handle)) &&
			(nullptr != sql_state) && (!strcmp(sql_state, "53000") || !strcmp(sql_state, "53200")) && (retry_count > 0))
		{
			usleep(1000000);
			retry_count--;
			PQclear(result);
			goto retry;
		}
		else
		{
			TP_LOG(log_error, "%s() error(%s): %s", __FUNCTION__, sql_state, PQerrorMessage(connection_handle));
		}

		PQclear(result);
		return false;
	}

	select_result = result;
	return true;
}

bool pg_driver::query(const std::string &query_)
{
	if (nullptr == connection_handle)
	{
		TP_LOG(log_error, "%s() connection is not established.", __FUNCTION__);
		return false;
	}

	std::unique_lock<std::mutex> lock(mutexTransLock);

	if (unsafe_query(query_))
	{
		return true;
	}
	else
	{
		if (CONNECTION_BAD == PQstatus(connection_handle) && reconnect_enabled)
		{
			TP_LOG(log_warn, "%s() connection lost, trying reconnect...", __FUNCTION__);

			if (reconnect())
			{
				if (unsafe_query(query_))
				{
					return true;
				}
				else
				{
					TP_LOG(log_error, "%s() failed: %s", __FUNCTION__, PQerrorMessage(connection_handle));
				}
			}
		}
		else
		{
			TP_LOG(log_error, "%s() failed: %s", __FUNCTION__, PQerrorMessage(connection_handle));
		}
	}

	return false;
}

bool pg_driver::select(const std::string &query_)
{
	if (nullptr == connection_handle)
	{
		TP_LOG(log_error, "%s() connection is not established.", __FUNCTION__);
		return false;
	}

	std::unique_lock<std::mutex> lock(mutexTransLock);

	if (unsafe_select(query_))
	{
		return true;
	}
	else
	{
		if (CONNECTION_BAD == PQstatus(connection_handle) && reconnect_enabled)
		{
			TP_LOG(log_warn, "%s() connection lost, trying reconnect...", __FUNCTION__);

			if (reconnect())
			{
				if (unsafe_select(query_))
				{
					return true;
				}
				else
				{
					TP_LOG(log_error, "%s() failed: %s", __FUNCTION__, PQerrorMessage(connection_handle));
				}
			}
		}
		else
		{
			TP_LOG(log_error, "%s() failed: %s", __FUNCTION__, PQerrorMessage(connection_handle));
		}
	}

	return false;
}

void pg_driver::free_result()
{
	if (nullptr != select_result)
	{
		PQclear(select_result);
		select_result = nullptr;
	}
}

int pg_driver::get_num_rows()
{
	if (nullptr == select_result)
	{
		return 0;
	}

	return PQntuples(select_result);
}

std::string pg_driver::get_field(int row_, int column_)
{
	if (nullptr == select_result)
	{
		return "";
	}

	if (0 != PQfformat(select_result, column_))
	{
		return "";
	}

	return PQgetvalue(select_result, row_, column_);
}

int32_t pg_driver::get_field_int(int row_, int column_)
{
	if (nullptr == select_result)
	{
		return 0;
	}

	return std::stoi(get_field(row_, column_));
}

long pg_driver::get_field_long(int row_, int column_)
{
	if (nullptr == select_result)
	{
		return 0L;
	}

	return std::stol(get_field(row_, column_));
}

float pg_driver::get_field_float(int row_, int column_)
{
	if (nullptr == select_result)
	{
		return 0.f;
	}

	return std::stof(get_field(row_, column_));
}

std::vector<uint8_t> pg_driver::get_field_array(int row_, int column_)
{
	std::string value = get_field(row_, column_);

	if (value.empty())
	{
		return std::vector<uint8_t>();
	}

	std::vector<uint8_t> result;
	return str_to_bin(value, result);
}