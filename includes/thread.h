#ifndef THREAD_H
    #define THREAD_H

    #include <condition_variable>
    #include <mutex>
    #include <functional>
    #include <memory>

    #include "systhread.h"
    #include "position.h"
    #include "search.h"
    #include "transposition.h"

    namespace Juujfish {
        
        const int DEFAULT_NUM_THREADS = 8;  // Default number of threads to use

        struct SharedData {
            TranspositionTable tt_table;
        };

        struct Stack {
            
        };

        class Thread {
          public:
            Thread(int thread_id);
            ~Thread();

            void clear();

            void dispatch_job(std::function<void()> task);

            void start_searching();
            void wait_for_search_finish();

          private:
            std::unique_ptr<Search::Worker> worker;

            int _thread_id, _num_threads;
            bool running = true, searching = false;
            std::function<void()> job_func;
                
            SysThread               sys_thread;
            std::condition_variable cv;
            std::mutex              mtx;

            void loop();
        };

        class ThreadPool {
            public:
                ThreadPool(int num_threads = DEFAULT_NUM_THREADS);
                ~ThreadPool();
                
                void clear();
                void set(int num_threads);

                void start_thinking(Position &pos, StatesDeque &states);

                void wait_for_all_threads();

                Thread* main_thread() { return threads.front().get(); }

            private:
                std::vector<std::unique_ptr<Thread>> threads;
                
                StatesDeque states;  

                void start_searching();
               
        };
        
    } // namespace Juujfish

#endif // ifndef THREAD_H
