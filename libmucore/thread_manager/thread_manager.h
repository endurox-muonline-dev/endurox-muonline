#include <thread>
#include <vector>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>

class thread_manager
{
	typedef std::function<void(UBFH*&, int)> fp_t;

	struct queue_data
	{
		fp_t function;
		UBFH *buffer;
	};

public:
	thread_manager(const std::string &name_, size_t thread_cnt_ = 1);
	~thread_manager();

	void dispatch(const fp_t &op_);
	void dispatch(fp_t &&op_);
	void dispatch(fp_t &&op_, UBFH *&data_);

	thread_manager(const thread_manager &rhs_) = delete;
	thread_manager &operator=(const thread_manager &rhs) = delete;
	thread_manager(thread_manager &&rhs_) = delete;
	thread_manager &operator=(thread_manager &&rhs_) = delete;

private:
	std::string name;
	std::vector<std::thread> threads;
	std::mutex mlock;
	std::queue<queue_data> queue;
	std::condition_variable cv;

	bool quit = false;
	void dispatch_thread_handler(int thread_id_);
};