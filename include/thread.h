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
  bool running = true, searching = true;
  std::function<void()> job_func = nullptr;

  std::condition_variable cv;
  std::mutex mtx;
  SysThread sys_thread;

  void loop();
};

class ThreadPool {
 public:
  ThreadPool(TranspositionTable* tt, int num_threads = DEFAULT_NUM_THREADS);
  ~ThreadPool();

  void clear();
  void set(int num_threads);

  void start(Position& root_pos, StatesDequePtr& initial_states);
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
