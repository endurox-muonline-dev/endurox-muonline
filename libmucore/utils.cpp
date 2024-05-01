#include <ubf.h>
#include <atmi.h>
#include <ndebug.h>
#include <algorithm>
#include <Exfields.h>

#include "utils.h"

std::string strip_characters(const std::string &input_, const char remove_character_)
{
	std::string result = input_;
	result.erase(std::remove(result.begin(), result.end(), remove_character_), result.end());

	return result;
}

std::vector<std::string> split(const std::string& s, char seperator)
{
	std::vector<std::string> output;
	std::string::size_type prev_pos = 0, pos = 0;

	while((pos = s.find(seperator, pos)) != std::string::npos)
	{
		std::string substring( s.substr(prev_pos, pos - prev_pos) );
		output.push_back(substring);
		prev_pos = ++pos;
	}

	output.push_back(s.substr(prev_pos, pos - prev_pos)); // Last word
	return output;
}

uint8_t parse_hex(int8_t char_)
{
	if ('0' <= char_ && char_ <= '9') return char_ - '0';
	if ('A' <= char_ && char_ <= 'F') return char_ - 'A' + 10;
	if ('a' <= char_ && char_ <= 'f') return char_ - 'a' + 10;

	return '0'; // ???
}

std::vector<uint8_t> str_to_bin(const std::string &input_, std::vector<uint8_t> &output_)
{
	for (size_t i = 0; i != input_.size() / 2; ++i)
	{
		output_.push_back(16 * parse_hex(input_[2 * i]) + parse_hex(input_[2 * i + 1]));
	}

	return output_;
}

bool sql_syntax_check(const char *input_)
{
	const char *tmp;
	tmp = strchr(input_, '\'');

	if (NULL == tmp)
	{
		tmp = strchr(input_, ' ');

		if (NULL == tmp)
		{
			return true;
		}

		return false;
	}

	return false;
}

bool space_syntax_check(const char *input_)
{
	const char *tmp;
	tmp = strchr(input_, ' ');

	if (NULL == tmp)
	{
		return true;
	}

	return false;
}

/**
 * 
 */
std::string to_hex(uint8_t s)
{
	std::stringstream ss;
	ss << std::hex << std::setw(2) << std::setfill('0') << (int)s;
	return ss.str();
}

std::string sha256(const std::string &input_)
{
	uint8_t hash[SHA256_DIGEST_LENGTH] = { 0 };
	const uint8_t *data = (const uint8_t*)input_.c_str();
	SHA256(data, input_.size(), hash);

	std::string output;

	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		output += to_hex(hash[i]);
	}

	return output;
}

std::string read_config_str(const std::string &section_, const std::string &name_, const std::string &default_)
{
	UBFH *buffer = (UBFH*)tpalloc((char*)"UBF", NULL, 2048);
	char *cctag = getenv("NDRX_CCTAG");
	std::string lookup_section = section_;

	if (NULL != cctag)
	{
		lookup_section += "/";
		lookup_section += cctag;
	}

	if (EXSUCCEED != Bchg(buffer, EX_CC_LOOKUPSECTION, 0, (char*)lookup_section.c_str(), 0))
	{
		TP_LOG(log_error, "Bchg EX_CC_LOOKUPSECTION failed: %s", Bstrerror(Berror));
		tpfree((char*)buffer);
		return default_;
	}

	long rsp_len;
	int occ;

	if (EXSUCCEED != tpcall((char*)"@CCONF", (char*)buffer, 0, (char**)&buffer, &rsp_len, 0))
	{
		tpfree((char*)buffer);
		return default_;
	}

	if (EXFAIL == (occ = Bfindocc(buffer, EX_CC_KEY, (char*)name_.c_str(), 0)))
	{
		TP_LOG(log_error, "Bfindocc EX_CC_KEY failed: %s", Bstrerror(Berror));
		tpfree((char*)buffer);
		return default_;
	}

	char value_buffer[4096] = { 0 };
	int value_len = 4096;

	if (EXSUCCEED != Bget(buffer, EX_CC_VALUE, occ, (char*)value_buffer, &value_len))
	{
		TP_LOG(log_error, "Bget EX_CC_VALUE failed: %s", Bstrerror(Berror));
		tpfree((char*)buffer);
		return default_;
	}

	tpfree((char*)buffer);
	return value_buffer;
}

int read_config_int(const std::string &section_, const std::string &name_, int default_)
{
	std::string result = read_config_str(section_, name_).c_str();

	if (result.empty())
	{
		return default_;
	}

	return atoi(result.c_str());
}