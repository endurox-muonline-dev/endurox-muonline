#include "reply_manager.h"

reply_manager::reply_manager(const std::string &name_, size_t thread_cnt_) :
	name(std::move(name_)), threads(thread_cnt_) {
		TP_LOG(log_debug, "%s() created: %s", __FUNCTION__, name.c_str());
		TP_LOG(log_debug, "%s() thread count: %zu", __FUNCTION__, thread_cnt_);

		for (size_t i = 0; i < threads.size(); i++)
		{
			threads[i] = std::thread(&reply_manager::dispatch_thread_handler, this, i);
		}
}

reply_manager::~reply_manager()
{
	TP_LOG(log_debug, "%s() destructor", __FUNCTION__);

	std::unique_lock<std::mutex> lock(mlock);
	quit = true;

	cv.notify_all();
	lock.unlock();

	for (size_t i = 0; i < threads.size(); i++)
	{
		if (threads[i].joinable())
		{
			TP_LOG(log_debug, "%s() joining thread %zu until completion", __FUNCTION__, i);
			threads[i].join();
		}
	}
}

void reply_manager::dispatch(const fp_t &op_)
{
	std::unique_lock<std::mutex> lock(mlock);
	queue.push({ op_, "", NULL, 0 });
	cv.notify_one();
}

void reply_manager::dispatch(fp_t &&op_)
{
	std::unique_lock<std::mutex> lock(mlock);
	queue.push({ std::move(op_), "", NULL, 0 });
	cv.notify_one();
}

void reply_manager::dispatch(fp_t &&op_, const std::string &service_name_, uint8_t *&input_data_, uint16_t input_len_)
{
	std::unique_lock<std::mutex> lock(mlock);

	uint8_t *queue_data = (uint8_t*)malloc(input_len_);
	memcpy(queue_data, &input_data_, input_len_);

	queue.push({ std::move(op_), service_name_, queue_data, input_len_ });
	cv.notify_one();
}

void reply_manager::dispatch_thread_handler(int thread_id_)
{
	std::unique_lock<std::mutex> lock(mlock);

	do
	{
		cv.wait(lock, [this] {
			return (queue.size() || quit);
		});

		if (!quit && queue.size())
		{
			auto op = std::move(queue.front());
			queue.pop();
			lock.unlock();

			op.function(op.service_name, op.input_data, op.input_len, thread_id_);

			if (NULL != op.input_data)
			{
				free(op.input_data);
			}

			lock.lock();
		}
	} while (!quit);

	// Terminate client session, tpacall() will open reply queue as client event if TPNOREPLY flag is present.
	tpterm();
}