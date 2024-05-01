#include "simple_modulus.h"
#include <string.h>
#include "ndebug.h"

simple_modulus::simple_modulus()
{
	this->save_load_xor[0] = 0x3F08A79B;
	this->save_load_xor[1] = 0xE25CC287;
	this->save_load_xor[2] = 0x93D27AB9;
	this->save_load_xor[3] = 0x20DEA7BF;
	this->init();
}

simple_modulus::~simple_modulus()
{

}

void simple_modulus::init()
{
	memset(this->encryption_key, 0, sizeof(this->encryption_key));
	memset(this->modulus, 0, sizeof(this->modulus));
	memset(this->decryption_key, 0, sizeof(this->decryption_key));
}

int simple_modulus::encrypt(void *dest_, void *source_, int size_)
{
	int temp_size = size_;
	int temp_size2;
	int original_size;

	uint8_t *temp_dest = (uint8_t*)dest_;
	uint8_t *temp_source = (uint8_t*)source_;

	int dec = ((size_ + 7) / 8);
	size_ = (dec + dec * 4) * 2 + dec;

	if (NULL != dest_)
	{
		original_size = temp_size;

		for (int i = 0; i < temp_size; i += 8, original_size -= 8, temp_dest += 11)
		{
			temp_size2 = original_size;

			if (original_size >= 8)
			{
				temp_size2 = 8;
			}

			this->encrypt_block(temp_dest, temp_source + i, temp_size2);
		}
	}

	return size_;
}

int simple_modulus::decrypt(void *dest_, void *source_, int size_)
{
	if (NULL == dest_)
	{
		return size_ * 8 / 11;
	}

	uint8_t *temp_dest = (uint8_t*)dest_;
	uint8_t *temp_src = (uint8_t*)source_;

	int result = 0;
	int dec_len = 0;

	if (size_ > 0)
	{
		while (dec_len < size_)
		{
			int temp_result = this->decrypt_block(temp_dest, temp_src);

			if (result < 0)
			{
				return result;
			}

			result += temp_result;
			dec_len += 11;
			temp_src += 11;
			temp_dest += 8;
		}
	}

	return result;
}

int simple_modulus::encrypt_block(void *dest_, void *source_, int size_)
{
	uint32_t enc_buffer[4];
	uint32_t enc_value = 0;

	uint8_t *enc_dest = (uint8_t*)dest_;
	uint8_t *enc_source = (uint8_t*)source_;

	memset(enc_dest, 0, 11);

	for (int i = 0; i < 4; i++)
	{
		enc_buffer[i] = ((this->xor_key[i] ^ ((uint16_t*)source_)[i] ^ enc_value) * this->encryption_key[i]) % this->modulus[i];
		enc_value = enc_buffer[i] & 0xFFFF;
	}

	for (int i = 0; i < 3; i++)
	{
		enc_buffer[i] = enc_buffer[i] ^ this->xor_key[i] ^ (enc_buffer[i + 1] & 0xFFFF);
	}

	int bit_pos = 0;

	for (int i = 0; i < 4; i++)
	{
		bit_pos = this->add_bits(dest_, bit_pos, &enc_buffer[i], 0, 16);
		bit_pos = this->add_bits(dest_, bit_pos, &enc_buffer[i], 22, 2);
	}

	uint8_t check_sum = 0xF8;

	for (int i = 0; i < 8; i++)
	{
		check_sum ^= enc_source[i];
	}

	((uint8_t*)&enc_value)[1] = check_sum;
	((uint8_t*)&enc_value)[0] = check_sum ^ size_ ^ 0x3D;

	return this->add_bits(dest_, bit_pos, &enc_value, 0, 16);
}

int simple_modulus::decrypt_block(void *dest_, void *source_)
{
	memset(dest_, 0, 8);
	uint32_t dec_buffer[4] = { 0 };
	int bit_pos = 0;

	uint8_t *dec_dest = (uint8_t*)dest_;
	uint8_t *dec_source = (uint8_t*)source_;

	for (int i = 0; i < 4; i++)
	{
		this->add_bits(&dec_buffer[i], 0, dec_source, bit_pos, 16);
		bit_pos += 16;

		this->add_bits(&dec_buffer[i], 22, dec_source, bit_pos, 2);
		bit_pos += 2;
	}

	for (int i = 2; i >= 0; i--)
	{
		dec_buffer[i] = dec_buffer[i] ^ this->xor_key[i] ^ (dec_buffer[i + 1] & 0xFFFF);
	}

	uint32_t temp = 0, temp1;

	for (int i = 0; i < 4; i++)
	{
		temp1 = ((this->decryption_key[i] * (dec_buffer[i])) % (this->modulus[i])) ^ this->xor_key[i] ^ temp;
		temp = dec_buffer[i] & 0xFFFF;
		((uint16_t*)dec_dest)[i] = temp1;
	}

	dec_buffer[0] = 0;
	this->add_bits(&dec_buffer[0], 0, dec_source, bit_pos, 16);
	((uint8_t*)dec_buffer)[0] = ((uint8_t*)dec_buffer)[1] ^ ((uint8_t*)dec_buffer)[0] ^ 0x3D;

	uint8_t check_sum = 0xF8;

	for (int i = 0; i < 8; i++)
	{
		check_sum ^= dec_dest[i];
	}

	if (check_sum != ((uint8_t*)dec_buffer)[1])
	{
		return -1;
	}

	return ((uint8_t*)dec_buffer)[0];
}

int simple_modulus::add_bits(void *dest_, int dest_bit_pos_, void *source_, int source_bit_pos_, int bit_len_)
{
	int source_buffer_bit_len = bit_len_ + source_bit_pos_;
	int temp_buffer_len = this->get_byte_of_bit(source_buffer_bit_len - 1);
	temp_buffer_len += 1 - this->get_byte_of_bit(source_bit_pos_);

	uint8_t *temp_buffer = new uint8_t[temp_buffer_len + 1];
	memset(temp_buffer, 0, temp_buffer_len + 1);
	memcpy(temp_buffer, (uint8_t*)source_ + this->get_byte_of_bit(source_bit_pos_), temp_buffer_len);

	if ((source_buffer_bit_len % 8) != 0)
	{
		temp_buffer[temp_buffer_len - 1] &= 255 << (8 - (source_buffer_bit_len % 8));
	}

	int shift_left = (source_bit_pos_ % 8);
	int shift_right = (dest_bit_pos_ % 8);

	this->shift(temp_buffer, temp_buffer_len, -shift_left);
	this->shift(temp_buffer, temp_buffer_len + 1, shift_right);

	int new_temp_buffer_len = ((shift_right <= shift_left) ? 0 : 1) + temp_buffer_len;
	uint8_t *temp_dest = (uint8_t*)dest_ + this->get_byte_of_bit(dest_bit_pos_);

	for (int i = 0; i < new_temp_buffer_len; i++)
	{
		temp_dest[i] |= temp_buffer[i];
	}

	delete[] temp_buffer;
	return dest_bit_pos_ + bit_len_;
}

void simple_modulus::shift(void *buffer_, int size_, int shift_len_)
{
	uint8_t *temp_buff = (uint8_t*)buffer_;

	if (shift_len_ != 0)
	{
		if (shift_len_ > 0)
		{
			if ((size_ - 1) > 0)
			{
				for (int i = (size_ -1); i > 0; i--)
				{
					temp_buff[i] = (temp_buff[i - 1] << ((8 - shift_len_))) | (temp_buff[i] >> shift_len_);
				}
			}

			temp_buff[0] >>= shift_len_;
		}
		else
		{
			shift_len_ = -shift_len_;

			if ((size_ - 1) > 0)
			{
				for (int i = 0; i < (size_ - 1); i++)
				{
					temp_buff[i] = (temp_buff[i + 1] >> ((8 - shift_len_))) | (temp_buff[i] << shift_len_);
				}
			}

			temp_buff[size_ - 1] <<= shift_len_;
		}
	}
}

int simple_modulus::get_byte_of_bit(int byte_)
{
	return byte_ >> 3;
}

bool simple_modulus::save_all_key(const std::string &file_name_)
{
	return this->save_key(file_name_, 4370, true, true, true, true);
}

bool simple_modulus::load_all_key(const std::string &file_name_)
{
	return this->load_key(file_name_, 4370, true, true, true, true);
}

bool simple_modulus::save_encryption_key(const std::string &file_name_)
{
	return this->save_key(file_name_, 4370, true, true, false, true);
}

bool simple_modulus::load_encryption_key(const std::string &file_name_)
{
	return this->load_key(file_name_, 4370, true, true, false, true);
}

bool simple_modulus::save_decryption_key(const std::string &file_name_)
{
	return this->save_key(file_name_, 4370, true, false, true, true);
}

bool simple_modulus::load_decryption_key(const std::string &file_name_)
{
	return this->load_key(file_name_, 4370, true, false, true, true);
}

bool simple_modulus::save_key(const std::string &file_name_, uint16_t file_header_, bool save_modulus_, bool save_enc_key_,
	bool save_dec_key_, bool save_xor_key_)
{
	return false;
}

bool simple_modulus::load_key(const std::string &file_name_, uint16_t file_header_, bool load_modulus_, bool load_enc_key_,
	bool load_dec_key_, bool load_xor_key_)
{
	s_encdec_fileheader header_buffer;
	int size;
	uint32_t xor_table[4];

	FILE *file = fopen(file_name_.c_str(), "rb");

	if (NULL != file)
	{
		size = fread(&header_buffer, sizeof(s_encdec_fileheader), 1, file);
	}

	if (header_buffer.file_header == file_header_)
	{
		if (header_buffer.size == (((load_modulus_ + load_enc_key_ + load_dec_key_ + load_xor_key_) * sizeof(xor_table)) + sizeof(s_encdec_fileheader)))
		{
			if (load_modulus_ != false)
			{
				size = fread(&xor_table, sizeof(xor_table), 1, file);

				for (int i = 0; i < 4; i++)
				{
					this->modulus[i] = save_load_xor[i] ^ xor_table[i];
				}
			}

			if (load_enc_key_ != false)
			{
				size = fread(&xor_table, sizeof(xor_table), 1, file);

				for (int i = 0; i < 4; i++)
				{
					this->encryption_key[i] = save_load_xor[i] ^ xor_table[i];
				}
			}

			if (load_dec_key_ != false)
			{
				size = fread(&xor_table, sizeof(xor_table), 1, file);

				for (int i = 0; i < 4; i++)
				{
					this->decryption_key[i] = save_load_xor[i] ^ xor_table[i];
				}
			}

			if (load_xor_key_ != false)
			{
				size = fread(&xor_table, sizeof(xor_table), 1, file);

				for (int i = 0; i < 4; i++)
				{
					this->xor_key[i] = save_load_xor[i] ^ xor_table[i];
				}
			}

			fclose(file);
			return true;
		}
	}

	if (NULL != file)
	{
		fclose(file);
	}

	return false;
}