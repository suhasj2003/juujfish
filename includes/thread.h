#ifndef THREAD_H
    #define THREAD_H

    #include <condition_variable>
    #include <mutex>
    #include <functional>
    #include <memory>

    #include "systhread.h"
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
            Thread();
            ~Thread();

            void dispatch_job(std::function<void()> task);

            void start_searching();
            void wait_for_search_finish();

            private:
                std::unique_ptr<Search::Worker> worker;

                SysThread sys_thread;
                std::condition_variable cv;
                std::mutex mtx;

                int thread_id, num_threads;
                bool running = true;
                std::function<void()> job_func;

                void loop();
        };

        class ThreadPool {
            public:
                ThreadPool(int num_threads = DEFAULT_NUM_THREADS);
                ~ThreadPool();
                
                void start_thinking();

                void wait_for_all_threads();

            private:
                std::vector<std::unique_ptr<Thread>> threads;

                void start_searching();
               
        };
        
    } // namespace Juujfish

#endif // ifndef THREAD_H
    