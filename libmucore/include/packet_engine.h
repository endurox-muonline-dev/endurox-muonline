#ifndef _packet_engine_h_
#define _packet_engine_h_

#include <cstdint>
#include <vector>

#include "ndebug.h"
#include "muprotocol.h"

class packet_engine
{
	uint16_t size;
	uint8_t buffer[MAX_C2_MESSAGE_SIZE];
	std::vector<uint8_t> xor_filter;

public:
	packet_engine()
	{
		this->clear();
		xor_filter =
		{
			0xE7, 0x6D, 0x3A, 0x89, 0xBC, 0xB2, 0x9F, 0x73,
			0x23, 0xA8, 0xFE, 0xB6, 0x49, 0x5D, 0x39, 0x5D,
			0x8A, 0xCB, 0x63, 0x8D, 0xEA, 0x7D, 0x2B, 0x5F,
			0xC3, 0xB1, 0xE9, 0x83, 0x29, 0x51, 0xE8, 0x56
		};
	}

	~packet_engine()
	{

	}

	void clear()
	{
		this->size = 0;
	}

protected:
	void xor_data(int start_, int end_, int dir_)
	{
		if (start_ < end_)
		{
			TP_LOG(log_error, "packet_engine() xor_data error %d, %d", start_, end_);
		}

		for (int i = start_; i != end_; i+= dir_)
		{
			this->buffer[i] ^= this->buffer[i - 1] ^ this->xor_filter[i % 32];
		}
	}

public:
	bool add_data(void *source_, uint16_t size_)
	{
		if (((this->size + size_) >= 2048) || (size_ == 0))
		{
			TP_LOG(log_error, "packet_engine() adding buffer size error %d", this->size + size_);
			return false;
		}

		memcpy((void*)&this->buffer[this->size], source_, size_);
		this->size += size_;
		return true;
	}

	bool extract_packet(void *target_)
	{
		uint16_t size_;
		uint8_t temp_[2048];

		switch (this->buffer[0])
		{
			case 0xC1:
				size_ = this->buffer[1];
				break;
			case 0xC2:
				size_ = this->buffer[1] * 256 + this->buffer[2];
				break;
			default:
				return true;
				break;
		}

		if (this->size < size_)
		{
			return 2;
		}

		this->xor_data(size_ - 1, (this->buffer[0] == 0xC1 ? 2 : 3), -1);
		memcpy(target_, this->buffer, size_);
		this->size -= size_;

		memcpy(temp_, &this->buffer[size_], this->size);
		memcpy(this->buffer, temp_, this->size);
		
		return false;
	}
};

#endif