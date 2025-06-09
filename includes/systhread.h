#ifndef SYSTHREAD_H
    #define SYSTHREAD_H
    
    #ifdef __OSX__
    #include <pthread.h>

    namespace Juujfish {
        // pthread wrapper that sets thread stack size to 8MB MacOS (default is 512KB)
        class SysThread {
            static constexpr size_t THREAD_STACK_SIZE = 8 * 1024 * 1024; // 8MB

        public:
            template<class Function, class... Args>
            explicit SysThread(Function&& fun, Args &&... args) {
                pthread_attr_init(attrp);
                pthread_attr_setstacksize(attrp, THREAD_STACK_SIZE);

                pthread_create(&thread, attrp, [](void* arg) -> void* {
                    auto f = reinterpret_cast<std::function<void()>*>(arg);
                    (*f)();
                    delete f;
                    return nullptr;
                    }, new std::function<void()>(std::bind(std::forward<Function>(fun), std::forward<Args>(args)...)));
            }

            void* join(void* ptr) {
                pthread_join(thread, ptr);
                return ptr;
            }

        private:
            pthread_t thread;
            pthread_attr_t attr, * attrp = &attr;
        };
	} // namespace Juujfish
    

    #else
    #include <thread>

    namespace Juujfish {
        using SysThread = std::thread;
	} // namespace Juujfish

    #endif

#endif // ifndef SYSTHREAD_H