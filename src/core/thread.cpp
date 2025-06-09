#include "thread.h"

namespace Juujfish {

	Thread::Thread() {
        this->worker = std::make_unique<Search::Worker>();

		running = false;
        job_func = nullptr;

		dispatch_job([this]() {  });
	}

	Thread::~Thread() {

	}

	void Thread::dispatch_job(std::function<void()> job) {
		{
			std::unique_lock<std::mutex> lock(mtx);
			cv.wait(lock, [this]() { return !running; });
			job_func = std::move(job);
		}
        cv.notify_one();
    }

} // namespace Juujfish