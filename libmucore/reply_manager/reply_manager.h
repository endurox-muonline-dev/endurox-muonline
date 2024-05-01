#ifndef _reply_manager_h_
#define _reply_manager_h_

#include <thread>
#include <vector>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>

class reply_manager
{
	typedef std::function<void(const std::string &, uint8_t*&, uint16_t, int)> fp_t;

	struct queue_data
	{
		fp_t function;
		std::string service_name;
		uint8_t *input_data;
		uint16_t input_len;
	};

public:
	reply_manager(const std::string &name_, size_t thread_cnt_ = 1);
	~reply_manager();

	void dispatch(const fp_t &op_);
	void dispatch(fp_t &&op_);
	void dispatch(fp_t &&op_, const std::string &service_name_, uint8_t *&input_data_, uint16_t input_len_);

	reply_manager(const reply_manager &rhs_) = delete;
	reply_manager &operator=(const reply_manager &rhs) = delete;
	reply_manager(reply_manager &&rhs_) = delete;
	reply_manager &operator=(reply_manager &&rhs_) = delete;

private:
	std::string name;
	std::vector<std::thread> threads;
	std::mutex mlock;
	std::queue<queue_data> queue;
	std::condition_variable cv;

	bool quit = false;
	void dispatch_thread_handler(int thread_id_);
};

#endif