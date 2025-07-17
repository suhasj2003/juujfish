#ifndef THREAD_H
#define THREAD_H

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>

#include "position.h"
#include "search.h"
#include "systhread.h"
#include "transposition.h"

namespace Juujfish {

const int DEFAULT_NUM_THREADS = 8;

class ThreadPool;

class Thread {
 public:
  Thread(ThreadPool& tp, int thread_id);
  ~Thread();

  void clear();

  void dispatch_job(std::function<void()> task);

  void start_searching();
  void wait_for_search_finish();

  std::unique_ptr<Search::Worker> worker;
 private:
  int _thread_id, _num_threads;
  bool running = true, searching = false;
  std::function<void()> job_func;

  SysThread sys_thread;
  std::condition_variable cv;
  std::mutex mtx;

  void loop();
};

class ThreadPool {
 public:
  ThreadPool(int num_threads = DEFAULT_NUM_THREADS, TranspositionTable* tt);
  ~ThreadPool();

  void clear();
  void set(int num_threads);

  void start(Position& rootPos, StatesDequePtr& initialStates);
  void start_searching();

  void wait_for_all_threads();

  Thread* main_thread() { return threads.front().get(); }

  std::atomic<bool> stop;

 private:
  std::vector<std::unique_ptr<Thread>> threads;
  
  StatesDequePtr states;
  TranspositionTable* _tt;
};

}  // namespace Juujfish

#endif  // ifndef THREAD_H
