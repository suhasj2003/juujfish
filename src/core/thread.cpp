#include "thread.h"

namespace Juujfish {

// Function implementations for Thread wrapper class

Thread::Thread(ThreadPool& tp, int thread_id)
    : _thread_id(thread_id), sys_thread(&Thread::loop, this) {

  wait_for_search_finish();

  dispatch_job([this, &tp, thread_id]() {
    this->worker = std::make_unique<Search::Worker>(tp, thread_id);
  });

  wait_for_search_finish();
}

Thread::~Thread() {
  {
    std::unique_lock<std::mutex> lock(mtx);
    running = false;
  }
  start_searching();
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
    searching = true;
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
  while (true) {
    std::unique_lock<std::mutex> lock(mtx);
    if (job_func == nullptr) {
      searching = false;
      cv.notify_one();
    }
    cv.wait(lock, [&] { return searching; });

    if (!running)
      return;

    std::function<void()> job = std::move(job_func);
    job_func = nullptr;
    searching = false;

    lock.unlock();

    if (job) {
      job();
    }
  }
}

// Function implementations for ThreadPool class

ThreadPool::ThreadPool(TranspositionTable* tt, int num_threads) {
  threads.resize(num_threads);
  for (int thread_id = 0; thread_id < num_threads; ++thread_id)
    threads[thread_id] = std::make_unique<Thread>(*this, thread_id);

  _tt = tt;
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
    threads[thread_id] = std::make_unique<Thread>(*this, thread_id);
}

void ThreadPool::wait_for_all_threads() {
  for (auto&& thread : threads)
    if (thread != threads.front())
      thread->wait_for_search_finish();
}

void ThreadPool::start(Position& root_pos, StatesDequePtr& initial_states) {
  main_thread()->wait_for_search_finish();
  stop = false;

  RootMoves root_moves;
  for (const auto& m : MoveList<LEGAL>(root_pos))
    root_moves.emplace_back(m);

  states = std::move(initial_states);

  for (auto&& thread : threads) {
    thread->worker->tt = _tt;
    thread->worker->root_moves = root_moves;
    thread->worker->root_depth = 0;
    thread->worker->root_pos.set(root_pos.fen(), &thread->worker->root_state);
    thread->worker->root_state = states->back();
  }

  for (auto&& thread : threads)
    thread->wait_for_search_finish();

  main_thread()->start_searching();
}

void ThreadPool::start_searching() {
  for (auto&& thread : threads)
    if (thread != threads.front())
      thread->start_searching();
}

}  // namespace Juujfish
