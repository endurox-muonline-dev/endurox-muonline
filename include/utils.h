#ifndef _utils_h_
#define _utils_h_

#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <string.h>
#include <openssl/sha.h>

#define NDRX_HOMEPATH				getenv("NDRX_APPHOME")
#define GET_PLAIN_CONN_ID(x)		((__uint32_t)(x & 0xFFFFFF))

// Objects limits
#define	MAX_OBJECT					20000
#define	MAX_MONSTER					17800
#define	MAX_CALLMONSTER				1200
#define MAX_USER					1000
#define USER_START_INDEX			(MAX_MONSTER + MAX_CALLMONSTER)

// Helper functions
std::string strip_characters(const std::string &input_, const char remove_character_);
std::vector<std::string> split(const std::string& s, char seperator);
uint8_t parse_hex(int8_t char_);
std::vector<uint8_t> str_to_bin(const std::string &input_, std::vector<uint8_t> &output_);
bool sql_syntax_check(const char *input_);
bool space_syntax_check(const char *input_);
std::string sha256(const std::string &input_);

std::string read_config_str(const std::string &section_, const std::string &name_, const std::string &default_ = "");
int read_config_int(const std::string &section_, const std::string &name_, int default_ = 0);

#endif