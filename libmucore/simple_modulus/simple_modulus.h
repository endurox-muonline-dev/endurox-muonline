#ifndef _simple_modulus_h_
#define _simple_modulus_h_

#include <cstdint>
#include <string>

#define SIZE_ENCRYPTION_BLOCK	8
#define SIZE_ENCRYPTION_KEY		4
#define SIZE_ENCRYPTED			11

#pragma pack(1)
struct s_encdec_fileheader
{
	uint16_t file_header;
	int size;
};
#pragma pack()

class simple_modulus
{
public:
		simple_modulus();
		~simple_modulus();

		void init();

protected:
	uint32_t modulus[SIZE_ENCRYPTION_KEY];
	uint32_t encryption_key[SIZE_ENCRYPTION_KEY];
	uint32_t decryption_key[SIZE_ENCRYPTION_KEY];
	uint32_t xor_key[SIZE_ENCRYPTION_KEY];

	uint32_t save_load_xor[SIZE_ENCRYPTION_KEY];

public:
	int encrypt(void *target_, void *source_, int size_);
	int decrypt(void *target_, void *source_, int size_);

protected:
	int encrypt_block(void *target_, void *source_, int size_);
	int decrypt_block(void *target_, void *source_);
	int add_bits(void *buffer_, int num_buffer_bits, void *bits_, int initial_bit_, int num_bits_);
	void shift(void *buffer_, int size_, int shift_);
	int get_byte_of_bit(int byte_);

public:
	bool save_all_key(const std::string &file_name_);
	bool load_all_key(const std::string &file_name_);
	bool save_encryption_key(const std::string &file_name_);
	bool load_encryption_key(const std::string &file_name_);
	bool save_decryption_key(const std::string &file_name_);
	bool load_decryption_key(const std::string &file_name_);

protected:
	bool save_key(const std::string &file_name_, uint16_t file_header_, bool save_modulus_, bool save_enc_key, bool save_dec_key_, bool save_xor_key_);
	bool load_key(const std::string &file_name_, uint16_t file_header_, bool load_modulus_, bool load_enc_key, bool load_dec_key_, bool load_xor_key_);
};

#endif