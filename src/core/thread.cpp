#include "thread.h"

namespace Juujfish {

  // Function implementations for Thread wrapper class

	Thread::Thread(int thread_id) : 
    _thread_id(thread_id),
    sys_thread(&Thread::loop, this) {
    worker = std::make_unique<Search::Worker>();

		running = true;
    searching = false;
    job_func = nullptr;
	}

	Thread::~Thread() {
    running = false;
    searching = false;
    sys_thread.join();
	}

  void Thread::clear() {
    worker->clear(); 
  }

	void Thread::dispatch_job(std::function<void()> job) {
		{
			std::unique_lock<std::mutex> lock(mtx);
			cv.wait(lock, [&] { return !searching; });
			job_func = std::move(job);
		}
    cv.notify_one();
  }

  void Thread::start_searching() {
    assert(worker != nullptr);
    dispatch_job([this] { worker->start_searching(); });
  }  

  void Thread::wait_for_search_finish() {
    {
      std::unique_lock<std::mutex> lock(mtx);
      cv.wait(lock, [&] { return !searching; });
    }
  }

  void Thread::loop() {
    while (running) {
      std::unique_lock<std::mutex> lock(mtx);
      cv.notify_one();
      searching = false;
      cv.wait(lock, [&] { return searching; });

      if (!running)
        return;
      
      std::function<void()> job = std::move(job_func);
      job_func = nullptr;
      
      if (job)
        job();
    }
  }

  // Function implementations for ThreadPool class

  ThreadPool::ThreadPool(int num_threads) {
    threads.resize(num_threads);
    
    for (int thread_id = 0; thread_id < num_threads; ++thread_id)
         threads[thread_id] = std::make_unique<Thread>(thread_id);

  }

  ThreadPool::~ThreadPool() {
    wait_for_all_threads();

    clear();
  }

  void ThreadPool::clear() {
    for (auto&& thread : threads)
      thread->clear();
  }

  void ThreadPool::set(int num_threads) {
    wait_for_all_threads();  

    clear();

    threads.resize(num_threads);
    
    for (int thread_id = 0; thread_id < num_threads; ++thread_id)
         threads[thread_id] = std::make_unique<Thread>(thread_id);

  }

  void ThreadPool::wait_for_all_threads() {
    for (auto&& thread : threads)
      if (thread != threads.front())
        thread->wait_for_search_finish();
  }

  void ThreadPool::start_thinking(Position &pos, StatesDeque &states) {
    
  }
  

} // namespace Juujfish
