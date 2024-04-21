#include <cstdint>
#include <byteswap.h>
#include <string.h>

#define MAX_C2_MESSAGE_SIZE		65536

// namespace start
namespace muProcotol {

/*
 * Message header for C1/C3 packets
 */
#pragma pack(push, 1)
class CMsgHeadC1
{
	uint8_t type;
	uint8_t size;
	uint8_t headcode;

public:
	CMsgHeadC1(uint8_t type_, uint8_t size_, uint8_t headcode_) : 
		type(type_), size(size_), headcode(headcode_) {
	}

	~CMsgHeadC1() {
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
class CMsgHeadC1Ex
{
	uint8_t type;
	uint8_t size;
	uint8_t headcode;
	uint8_t subcode;

public:
	CMsgHeadC1Ex(uint8_t type_, uint8_t size_, uint8_t headcode_, uint8_t subcode_) : 
		type(type_), size(size_), headcode(headcode_), subcode(subcode_) {
	}

	~CMsgHeadC1Ex() {
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
class CMsgHeadC2
{
	uint8_t type;
	uint16_t size;
	uint8_t headcode;

public:
	CMsgHeadC2(uint8_t type_, uint16_t size_, uint8_t headcode_) : 
		type(type_), size(bswap_16(size_)), headcode(headcode_) {
	}

	~CMsgHeadC2() {
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
class CMsgHeadC2Ex
{
	uint8_t type;
	uint16_t size;
	uint8_t headcode;
	uint8_t subcode;

public:
	CMsgHeadC2Ex(const uint8_t type_, const uint16_t size_, const uint8_t headcode_, const uint8_t subcode_) :
		type(type_), size(bswap_16(size_)), headcode(headcode_), subcode(subcode_) {
	}

	~CMsgHeadC2Ex() {
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
class CMsgSendServerList : public CMsgHeadC2Ex
{
	uint16_t count;

public:
	CMsgSendServerList(uint16_t count_, bool enc_ = false) :
		CMsgHeadC2Ex(enc_ ? 0xC4 : 0xC2, sizeof(CMsgSendServerList), 0xF4, 0x06),
		count(bswap_16(count_)) {
	}
};

/*
 * Server data for connectserver list
 */
class CMsgCSServerData
{
	uint16_t index;
	uint8_t percent;
	uint8_t type;

public:
	CMsgCSServerData(uint16_t index_, uint8_t percent_, uint8_t type_) :
		index(index_), percent(percent_), type(type_) {
	}
};

/**
 * 
 */
class CMsgReqServerInfo : public CMsgHeadC1Ex
{
	uint16_t server_index;

public:
	CMsgReqServerInfo(bool enc_ = false) :
		CMsgHeadC1Ex(enc_ ? 0xC3 : 0xC1, sizeof(CMsgReqServerInfo), 0xF4, 0x03) {
	};

	CMsgReqServerInfo(uint16_t server_index_, bool enc_ = false) :
		CMsgHeadC1Ex(enc_ ? 0xC3 : 0xC1, sizeof(CMsgReqServerInfo), 0xF4, 0x03),
		server_index(server_index_) {
	}

	uint16_t get_server_index() {
		return server_index;
	}
};

/**
 * 
 */
class CMsgAnsServerInfo : public CMsgHeadC1Ex
{
	char address[16];
	uint16_t port;

public:
	CMsgAnsServerInfo(const char *address_, uint16_t port_, bool enc_ = false) :
		CMsgHeadC1Ex(enc_ ? 0xC3 : 0xC1, sizeof(CMsgAnsServerInfo), 0xF4, 0x03),
		port(port_) {
		strncpy(address, address_, 16);
	}
};

#pragma pack(pop)
// namespace end
}