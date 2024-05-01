#include "thread_manager.h"

thread_manager::thread_manager(const std::string &name_, size_t thread_cnt_) :
	name(std::move(name_)), threads(thread_cnt_) {
		TP_LOG(log_debug, "%s() created: %s", __FUNCTION__, name.c_str());
		TP_LOG(log_debug, "%s() thread count: %zu", __FUNCTION__, thread_cnt_);

		for (size_t i = 0; i < threads.size(); i++)
		{
			threads[i] = std::thread(&thread_manager::dispatch_thread_handler, this, i);
		}
}

thread_manager::~thread_manager()
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

void thread_manager::dispatch(const fp_t &op_)
{
	std::unique_lock<std::mutex> lock(mlock);
	queue.push({ op_, NULL });
	cv.notify_one();
}

void thread_manager::dispatch(fp_t &&op_)
{
	std::unique_lock<std::mutex> lock(mlock);
	queue.push({ std::move(op_), NULL });
	cv.notify_one();
}

void thread_manager::dispatch(fp_t &&op_, UBFH *&data_)
{
	std::unique_lock<std::mutex> lock(mlock);

	UBFH *dispatch_buffer = (UBFH*)tpalloc((char*)"UBF", NULL, Bsizeof(data_));
	Bcpy(dispatch_buffer, data_);

	queue.push({ std::move(op_), dispatch_buffer });
	cv.notify_one();
}

void thread_manager::dispatch_thread_handler(int thread_id_)
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

			op.function(op.buffer, thread_id_);

			if (NULL != op.buffer)
			{
				tpfree((char*)op.buffer);
			}

			lock.lock();
		}
	} while (!quit);

	// Terminate client session, tpacall() will open reply queue as client event if TPNOREPLY flag is present.
	tpterm();
}