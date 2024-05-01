#ifndef _muprotocol_h_
#define _muprotocol_h_

#include <cstdint>
#include <byteswap.h>
#include <string>

#define MAX_C2_MESSAGE_SIZE		65536

// namespace start
namespace muProtocol {

enum kJsLoginResult {
	LOGIN_WRONG_PASSWORD				= 0,
	LOGIN_SUCCESS						= 1,
	LOGIN_INVALID_NOT_EXIST_USERNAME	= 2,
	LOGIN_USER_ALREADY_CONNECTED		= 3,
	LOGIN_SERVER_IS_FULL				= 4,
	LOGIN_ACCOUNT_BLOCKED				= 5,
	LOGIN_BAD_CLIENT_VERSION			= 6,
	LOGIN_CONNECTION_REFUSED			= 8,
	LOGIN_NO_INFORMATION				= 9,
	LOGIN_UNK_410						= 10,
	LOGIN_UNK_411						= 11,
	LOGIN_UNK_412						= 12,
	LOGIN_UNK_413						= 13,
	LOGIN_SUCCESS_ITEMBLOCK				= 15
};

enum kMessageType {
	MAINSERVER_TYPE = 0,
	JOINSERVER_TYPE,
	DATASERVER_TYPE
};

static uint8_t bux_code[3] = { 0xFC, 0xCF, 0xAB };

static std::string bux_convert(char *input_, int size_)
{
	char output[size_] = { 0 };

	for (int i = 0; i < size_; i++)
	{
		output[i] = input_[i] ^ bux_code[i % 3];
	}

	return std::string(output);
}

class mu_packet
{
	uint16_t size;
	uint8_t message_buffer[MAX_C2_MESSAGE_SIZE];

	void add_data(uint8_t data_, bool enc_ = false) {
		memcpy(&message_buffer[size], &data_, sizeof(data_));
	}

	void add_data(uint16_t data_, bool enc_ = false) {
		uint16_t swap_data = bswap_16(data_);
		memcpy(&message_buffer[size], &swap_data, sizeof(data_));
	}

	void add_data(uint32_t data_, bool enc_ = false) {
		uint32_t swap_data = bswap_32(data_);
		memcpy(&message_buffer[size], &swap_data, sizeof(data_));
	}

	void add_data(uint64_t data_, bool enc_ = false) {
		uint64_t swap_data = bswap_64(data_);
		memcpy(&message_buffer[size], &swap_data, sizeof(data_));
	}

	void set_data(uint16_t pos_, uint8_t data_, bool enc_ = false) {
		memcpy(&message_buffer[pos_], &data_, sizeof(data_));
	}

	void set_data(uint16_t pos_, uint16_t data_, bool enc_ = false) {
		uint16_t swap_data = bswap_16(data_);
		memcpy(&message_buffer[pos_], &swap_data, sizeof(data_));
	}

	void set_data(uint16_t pos_, uint32_t data_, bool enc_ = false) {
		uint32_t swap_data = bswap_32(data_);
		memcpy(&message_buffer[pos_], &swap_data, sizeof(data_));
	}

	void set_data(uint16_t pos_, uint64_t data_, bool enc_ = false) {
		uint64_t swap_data = bswap_64(data_);
		memcpy(&message_buffer[pos_], &swap_data, sizeof(data_));
	}

public:
	mu_packet(uint8_t type_, uint8_t headcode_) {
		clear();
		add_data(&type_, sizeof(type_));

		switch (type_)
		{
			case 0xC1:
			case 0xC3:
				add_data(&size, 1);
				break;
			case 0xC2:
			case 0xC4:
				add_data(&size, 2);
				break;
			default:
				TP_LOG(log_error, "%s() wrong type: %d", __FUNCTION__, type_);
				break;
		}

		add_data(&headcode_, sizeof(headcode_));
	}

	mu_packet(uint8_t type_, uint8_t headcode_, uint8_t subcode_) : mu_packet(type_, headcode_) {
		add_data(&subcode_, sizeof(subcode_));
	}

	~mu_packet() {
	}

	void clear() {
		size = 0;
	}

	uint8_t *end() {
		uint16_t swap_size = bswap_16(size);
		switch (message_buffer[0])
		{
			case 0xC1:
			case 0xC3:
				memcpy(&message_buffer[1], &size, 1);
				break;
			case 0xC2:
			case 0xC4:
				memcpy(&message_buffer[1], &swap_size, 2);
				break;
			default:
				TP_LOG(log_error, "%s() wrong type: %d", __FUNCTION__, message_buffer[0]);
				break;
		}

		return message_buffer;
	}

	template<class T> void set_data(uint16_t pos_, T data_, uint16_t size_, bool is_string_ = false, bool enc_ = false) {
		if (pos_ + size_ > MAX_C2_MESSAGE_SIZE)
		{
			TP_LOG(log_error, "%s() buffer overflow. size(%d), input size(%d)", __FUNCTION__, size, size_);
			return;
		}

		if (is_string_)
		{
			memcpy(&message_buffer[pos_], data_, size_);
		}
		else
		{
			switch(size_)
			{
				case 1:
					set_data(pos_, *((uint8_t*)data_), enc_);
					break;
				case 2:
					set_data(pos_, *((uint16_t*)data_), enc_);
					break;
				case 4:
					set_data(pos_, *((uint32_t*)data_), enc_);
					break;
				case 8:
					set_data(pos_, *((uint64_t*)data_), enc_);
					break;
			}
		}

		if (enc_)
		{
			//XorData(size, size + size_);
		}
	}

	template<class T> void add_data(T data_, uint16_t size_, bool is_string_ = false, bool enc_ = false) {
		if (size + size_ > MAX_C2_MESSAGE_SIZE)
		{
			TP_LOG(log_error, "%s() buffer overflow. size(%d), input size(%d)", __FUNCTION__, size, size_);
			return;
		}
		
		if (is_string_)
		{
			memcpy(&message_buffer[size], data_, size_);
		}
		else
		{
			switch(size_)
			{
				case 1:
					add_data(*((uint8_t*)data_), enc_);
					break;
				case 2:
					add_data(*((uint16_t*)data_), enc_);
					break;
				case 4:
					add_data(*((uint32_t*)data_), enc_);
					break;
				case 8:
					add_data(*((uint64_t*)data_), enc_);
					break;
			}
		}
		
		if (enc_)
		{
			//XorData(size, size + size_);
		}

		size += size_;
	}

	uint16_t get_size() {
		return size;
	}
};

/*
 * Message header for C1/C3 packets
 */
#pragma pack(push, 1)
class head_c1
{
	uint8_t type;
	uint8_t size;
	uint8_t headcode;

public:
	head_c1(uint8_t type_, uint8_t size_, uint8_t headcode_) : 
		type(type_), size(size_), headcode(headcode_) {
	}

	~head_c1() {
	}

	uint8_t get_type() {
		return type;
	}

	uint8_t get_size() {
		return size;
	}

	void set_size(uint8_t size_) {
		size = size_;
	}

	uint8_t get_headcode() {
		return headcode;
	}
};

/*
 * Extended message header for C1/C3 packets
 */
class head_c1_ex
{
	uint8_t type;
	uint8_t size;
	uint8_t headcode;
	uint8_t subcode;

public:
	head_c1_ex(uint8_t type_, uint8_t size_, uint8_t headcode_, uint8_t subcode_) : 
		type(type_), size(size_), headcode(headcode_), subcode(subcode_) {
	}

	~head_c1_ex() {
	}

	uint8_t get_type() {
		return type;
	}

	uint8_t get_size() {
		return size;
	}

	void set_size(uint8_t size_) {
		size = size_;
	}

	uint8_t get_headcode() {
		return headcode;
	}

	uint8_t get_subcode() {
		return subcode;
	}
};

/*
 * Message header for C2/C4 packets
 */
class head_c2
{
	uint8_t type;
	uint16_t size;
	uint8_t headcode;

public:
	head_c2(uint8_t type_, uint16_t size_, uint8_t headcode_) : 
		type(type_), size(bswap_16(size_)), headcode(headcode_) {
	}

	~head_c2() {
	}

	uint8_t get_type() {
		return type;
	}

	uint16_t get_size() {
		return bswap_16(size);
	}

	void set_size(uint16_t size_) {
		size = bswap_16(size_);
	}

	uint8_t get_headcode() {
		return headcode;
	}
};

/*
 * Extended message header for C2/C4 packets
 */
class head_c2_ex
{
	uint8_t type;
	uint16_t size;
	uint8_t headcode;
	uint8_t subcode;

public:
	head_c2_ex(const uint8_t type_, const uint16_t size_, const uint8_t headcode_, const uint8_t subcode_) :
		type(type_), size(bswap_16(size_)), headcode(headcode_), subcode(subcode_) {
	}

	~head_c2_ex() {
	}

	uint8_t get_type() {
		return type;
	}

	uint16_t get_size() {
		return bswap_16(size);
	}

	void set_size(uint16_t size_) {
		size = bswap_16(size_);
	}

	uint8_t get_headcode() {
		return headcode;
	}

	uint8_t get_subcode() {
		return subcode;
	}
};
#pragma pack(pop)

/*
 * Server list message, used for sending connectserver list
 */
#pragma pack(push, 1)
class send_server_list : public head_c2_ex
{
	uint16_t count;

public:
	send_server_list(uint16_t count_, bool enc_ = false) :
		head_c2_ex(enc_ ? 0xC4 : 0xC2, sizeof(send_server_list), 0xF4, 0x06),
		count(bswap_16(count_)) {
	}
};

/*
 * Server data for connectserver list
 */
class cs_server_data
{
	uint16_t index;
	uint8_t percent;
	uint8_t type;

public:
	cs_server_data(uint16_t index_, uint8_t percent_, uint8_t type_) :
		index(index_), percent(percent_), type(type_) {
	}
};

/**
 * 
 */
class req_server_info : public head_c1_ex
{
	uint16_t server_index;

public:
	req_server_info(bool enc_ = false) :
		head_c1_ex(enc_ ? 0xC3 : 0xC1, sizeof(req_server_info), 0xF4, 0x03) {
	}

	req_server_info(uint16_t server_index_, bool enc_ = false) :
		head_c1_ex(enc_ ? 0xC3 : 0xC1, sizeof(req_server_info), 0xF4, 0x03),
		server_index(server_index_) {
	}

	uint16_t get_server_index() {
		return server_index;
	}
};

/**
 * 
 */
class ans_server_info : public head_c1_ex
{
	char address[16];
	uint16_t port;

public:
	ans_server_info(const char *address_, uint16_t port_, bool enc_ = false) :
		head_c1_ex(enc_ ? 0xC3 : 0xC1, sizeof(ans_server_info), 0xF4, 0x03),
		port(port_) {
		strncpy(address, address_, 16);
	}
};

class ans_join_result : public head_c1_ex
{
	uint8_t result;
	uint16_t index;
	char version[5];

public:
	ans_join_result(uint8_t result_, uint16_t index_, const std::string &version_, bool enc_ = false) :
		head_c1_ex(enc_ ? 0xC3 : 0xC1, sizeof(ans_join_result), 0xF1, 0x00),
		result(result_), index(bswap_16(index_)) {
		strncpy(version, version_.c_str(), 5);
	}
};

class req_id_password : public head_c1_ex
{
	char account[10];
	char password[20];
	uint32_t tick_count;
	char version[5];
	char serial[16];

public:
	req_id_password(bool enc_ = false) :
		head_c1_ex(enc_ ? 0xC3 : 0xC1, sizeof(req_id_password), 0xF1, 0x01) {
	}

	const std::string get_account() {
		return bux_convert(account, 10);
	}

	const std::string get_password() {
		return bux_convert(password, 20);
	}

	const uint32_t get_tick_count() {
		return tick_count;
	}

	const std::string get_version() {
		return std::string(version, 5).c_str();
	}

	const std::string get_serial() {
		return std::string(serial, 16).c_str();
	}
};

class js_id_pass_req : public head_c1
{
	char account[10];
	char password[20];
	uint16_t index;
	char address[16];

public:
	js_id_pass_req(const std::string &account_, const std::string &password_, uint16_t index_, const std::string &address_, bool enc_ = false) :
		head_c1(enc_ ? 0xC3 : 0xC1, sizeof(js_id_pass_req), 0x01) {
			strncpy(account, account_.c_str(), 10);
			strncpy(password, password_.c_str(), 20);
			index = bswap_16(index_);
			strncpy(address, address_.c_str(), 16);
		}

	const std::string get_account() {
		return std::string(account, 10).c_str();
	}

	const std::string get_password() {
		return std::string(password, 20).c_str();
	}

	uint16_t get_index() {
		return bswap_16(index);
	}

	const std::string get_address() {
		return std::string(address, 16).c_str();
	}
};

class js_id_pass_ans : public head_c1
{
	uint8_t result;
	uint16_t index;
	char account[10];
	int	user_number;
	int db_number;
	char serial[13];

public:
	js_id_pass_ans(bool enc_ = false) :
		head_c1(enc_ ? 0xC3 : 0xC1, sizeof(js_id_pass_ans), 0x01) {
	}

	void set_result(uint8_t result_) {
		result = result_;
	}

	uint8_t get_result() {
		return result;
	}

	void set_index(uint16_t index_) {
		index = bswap_16(index_);
	}

	uint16_t get_index() {
		return bswap_16(index);
	}

	void set_account(const std::string &account_) {
		strncpy(account, account_.c_str(), 10);
	}

	const std::string get_account() {
		return std::string(account, 10).c_str();
	}

	void set_user_number(int user_number_) {
		user_number = bswap_32(user_number_);
	}

	int get_user_number() {
		return bswap_32(user_number);
	}

	void set_db_number(int db_number_) {
		db_number = bswap_32(db_number_);
	}

	int get_db_number() {
		return bswap_32(db_number);
	}

	void set_serial(const std::string &serial_) {
		strncpy(serial, serial_.c_str(), 13);
	}

	const std::string get_serial() {
		return std::string(serial, 13).c_str();
	}
};

class js_join_result : public head_c1_ex {
	uint8_t result;

public:
	js_join_result(uint8_t result_, bool enc_ = false) : 
		head_c1_ex(enc_ ? 0xC3 : 0xC1, sizeof(js_join_result), 0xF1, 0x01), result(result_) {
	}
};

class get_char_list_req : public head_c1 {
	uint16_t index;
	char account[10];

public:
	get_char_list_req(uint16_t index_, const std::string &account_, bool enc_ = false) :
		head_c1(enc_ ? 0xC3 : 0xC1, sizeof(get_char_list_req), 0x01), index(bswap_16(index_)) {
		strncpy(account, account_.c_str(), 10);
	}

	uint16_t get_index() {
		return bswap_16(index);
	}

	const std::string get_account() {
		return std::string(account, 10).c_str();
	}
};

#pragma pack(pop)
// namespace end
}

#endif