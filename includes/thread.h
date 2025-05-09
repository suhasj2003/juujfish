#ifndef THREAD_H
    #define THREAD_H

    #include <pthread.h>

    namespace Juujfish {

        // pthread wrapper that sets thread stack size to 8MB for Windows and MacOS (default is 512KB)
        class SystemThread {
            public:
                template<class Function, class... Args>
                SystemThread(Function &&fun, Args &&... args) {
                    pthread_attr_init(attrp);
                    pthread_attr_setstacksize(attrp, 8 * 1024 * 1024); // 8MB

                    pthread_create(&thread, attrp, [](void *arg) -> void* {
                        auto f = static_cast<std::function<void()>*>(arg);
                        (*f)();
                        delete f;
                        return nullptr;
                    }, new std::function<void()>(std::bind(std::forward<Function>(fun), std::forward<Args>(args)...)));
                }

                
                

            private:
                pthread_t thread;
                pthread_attr_t attr, *attrp = &attr;
        };
        
    } // namespace Juujfish

#endif // ifndef THREAD_H
