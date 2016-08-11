//
//  V5Thread.cpp
//  V5 Engine
//
//  Created by mofed arafat on 6/4/15.
//  Copyright (c) 2016 org. All rights reserved.
//

#include "V5Thread.h"

#include <string.h>

#if WIN32

#include <process.h>

#else

#include <pthread.h>

#endif



namespace V5Graphics
{
    class impv5thread
    {
#if WIN32
    public:
        HANDLE thread;
        volatile bool completed;
#else
    public:
        pthread_t thread;
        volatile bool completed;
#endif
    };
    
    V5Thread::V5Thread(void *(*func)(void *), void* param)
    {
        thread = new impv5thread;
#if WIN32
        thread->completed = false;
        struct Func_args
        {
            void *(*func)(void *);
            void* param;
            volatile bool* comp;
        };
        
        Func_args* fa = new Func_args;
        fa->func = func;
        fa->param = param;
        fa->comp = &thread->completed;
        
        struct X { // struct's as good as class
            static unsigned __stdcall a(void* args)
            {
                Func_args* fa = (Func_args*)args;
                fa->func(fa->param);
                *fa->comp = true;
                return 0;
            }
        };
        if ((thread->thread = (HANDLE)_beginthreadex(NULL, 0, &X::a, fa, 0, NULL)) == NULL)
            throw new Exception("Unable to create Thread");
#else
        thread->completed = false;
        struct Func_args
        {
            void *(*func)(void *);
            void* param;
            volatile bool* comp;
        };
        
        Func_args* fa = new Func_args;
        fa->func = func;
        fa->param = param;
        fa->comp = &thread->completed;
        
        struct X { // struct's as good as class
            
            static void* a(void* args)
            {
                Func_args* fa = (Func_args*)args;
                fa->func(fa->param);
                *fa->comp = true;
                return 0;
            }
        };
        
        if (pthread_create(&thread->thread, NULL, X::a, fa))
            throw new Exception("Unable to create Thread");
#endif
    }
    
    V5Thread::~V5Thread()
    {
#if WIN32
        CloseHandle(thread->thread);
#endif
        delete thread;
    }
    
    void V5Thread::Join() const
    {
#if WIN32
        WaitForSingleObject(thread->thread, INFINITE);
#else
        void* status;
        pthread_join(thread->thread, &status);
#endif
    }
    
    bool V5Thread::IsCompleted() const
    {
#if WIN32
		return thread->completed;
#else
        return thread->completed;
#endif
    }
    
    
    void V5Thread::ThreadExit()
    {
#if WIN32
        _endthreadex(0);
#else
        pthread_exit(NULL);
#endif
    }
    
    
    
    class impv5mutex
    {
    public:
#if WIN32
        CRITICAL_SECTION mutex;
        impv5mutex()
        {
            InitializeCriticalSection(&mutex);
        }
#else
        impv5mutex()
        {
            pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
            memcpy(&mutex, &m, sizeof(m));
        }
        pthread_mutex_t mutex;
#endif
    };
    
    
    V5Mutex::V5Mutex()
    {
        mutex = new impv5mutex;
    }
    
    V5Mutex::~V5Mutex()
    {
        delete mutex;
    }
    
    bool V5Mutex::TryLock()
    {
#if WIN32
		return TryEnterCriticalSection(&mutex->mutex);
#else
        return pthread_mutex_trylock(&mutex->mutex) == 0;
#endif
    }
    
    void V5Mutex::Lock()
    {
#if WIN32
        EnterCriticalSection(&mutex->mutex);
#else
        pthread_mutex_lock(&mutex->mutex);
#endif
    }
    
    void V5Mutex::Unlock()
    {
#if WIN32
        LeaveCriticalSection(&mutex->mutex);
#else
        pthread_mutex_unlock(&mutex->mutex);
#endif
    }
    
    
    
    
//    class impv5condition
//    {
//    public:
//#if WIN32
//        CONDITION_VARIABLE cond;
//        impv5condition()
//        {
//            InitializeConditionVariable(&cond);
//        }
//#else
//        impv5condition()
//        {
//            pthread_cond_t c = PTHREAD_COND_INITIALIZER;
//            memcpy(&cond, &c, sizeof(c));
//        }
//        pthread_cond_t cond;
//#endif
//    };
//    
//    
//    V5Condition::V5Condition()
//    {
//        condition = new impv5condition;
//    }
//    
//    V5Condition::~V5Condition()
//    {
//        delete condition;
//    }
//    
//    void V5Condition::Wait(V5Mutex* mutex)
//    {
//#if WIN32
//        SleepConditionVariableCS(&condition->cond, &mutex->mutex->mutex, INFINITE);
//#else
//        pthread_cond_wait(&condition->cond, &mutex->mutex->mutex);
//#endif
//    }
//    
//    void V5Condition::Signal()
//    {
//#if WIN32
//        WakeConditionVariable(&condition->cond);
//#else
//        pthread_cond_signal(&condition->cond);
//#endif
//    }
    
    
    class impv5worker
    {
        struct tthreadArgs
        {
            vector<void *(*)(void*)>* tasks;
            vector<void*>* taskArgs;
            V5Mutex* mutex;
            V5Mutex* mutex2;
            V5Mutex* mutex3;
            volatile bool* exit;
            volatile bool* startedExecuting;
            volatile bool* isPaused;
        };
        static void *tthread(void* _args)
        {
            tthreadArgs* args = (tthreadArgs*)_args;
            bool mutex2IsLocked = true;
            args->mutex2->Lock();
            
            while (!*args->exit)
            {
                args->mutex3->Lock();
                args->mutex3->Unlock();
                bool haveTask = false;
                void *(* func)(void*);
                void* targs;
                args->mutex->Lock();
                if (args->tasks->size() != 0)
                {
                    
                    func = args->tasks->operator[](0);
                    args->tasks->erase(args->tasks->begin());
                    targs = args->taskArgs->operator[](0);
                    args->taskArgs->erase(args->taskArgs->begin());
                    haveTask = true;
                    *args->startedExecuting = true;
                    if (!mutex2IsLocked)
                    {
                        mutex2IsLocked = true;
                        args->mutex2->Lock();
                    }
                }
                else
                {
                    if (mutex2IsLocked)
                    {
                        mutex2IsLocked = false;
                        args->mutex2->Unlock();
                    }
                    if (!*args->isPaused)
                    {
                        *args->isPaused = true;
                        args->mutex3->Lock();
                    }
                }
                args->mutex->Unlock();
                
                if (haveTask)
                {
                    func(targs);
                }
            }
            delete args;
            return  NULL;
        }
        V5Mutex mutex;
        V5Mutex mutex2;
        V5Mutex mutex3;
        volatile bool isPaused;
        volatile bool exit;
        vector<void *(*)(void*)> tasks;
        vector<void*> taskArgs;
        V5Thread* thread;
        vector<void *(*)(void*)> todotasks;
        vector<void*> todoArgs;
        
    public:
        bool hasTasks;
        volatile bool startedExecuting;
        impv5worker()
        {
            isPaused = true;
            mutex3.Lock();
            exit = false;
            hasTasks = false;
            tthreadArgs* args = new tthreadArgs;
            args->exit = &exit;
            args->mutex = &mutex;
            args->mutex2 = &mutex2;
            args->mutex3 = &mutex3;
            args->isPaused = &isPaused;
            args->taskArgs = &taskArgs;
            args->tasks = &tasks;
            args->startedExecuting = &startedExecuting;
            thread = new V5Thread(tthread, args);
        }
        
        ~impv5worker()
        {
            if (isPaused)
            {
                isPaused = false;
                mutex3.Unlock();
            }
            exit = true;
            if (!thread->IsCompleted())
                thread->Join();
            
            delete thread;
        }
        
        void addTask(void *(*func)(void*), void* args)
        {
            todotasks.push_back(func);
            todoArgs.push_back(args);
            hasTasks = true;
        }
        void startAll()
        {
            if (todotasks.size() == 0)
                return;
            
            startedExecuting = false;
            mutex.Lock();
            for (int32_t i = 0; i < todotasks.size(); ++i)
            {
                tasks.push_back(todotasks[i]);
                taskArgs.push_back(todoArgs[i]);
                hasTasks = true;
            }
            mutex.Unlock();
            if (isPaused)
            {
                isPaused = false;
                mutex3.Unlock();
            }
            todotasks.clear();
            todoArgs.clear();
            while (!startedExecuting)
                usleep(1);
        }
        
        void WaitForAll()
        {
            if (!hasTasks || isPaused)
                return;
            
            mutex2.Lock();
            mutex2.Unlock();
            hasTasks = false;
        }
        
        bool isFinished()
        {
            if (isPaused)
                return true;
            
            bool isEmpty;
            mutex.Lock();
            isEmpty = tasks.size() == 0;
            mutex.Unlock();
            if (isEmpty)
                hasTasks = false;
            return isEmpty;
        }
    };
   
    V5Worker::V5Worker()
    {
        worker = new impv5worker;
    }
    
    V5Worker::~V5Worker()
    {
        delete worker;
    }
    
    void V5Worker::AddTask(void *(*func)(void*), void* args)
    {
        worker->addTask(func, args);
    }
    
    void V5Worker::StartAll()
    {
        worker->startAll();
    }
    
    void V5Worker::WaitForAll()
    {
        worker->WaitForAll();
    }
    
    bool V5Worker::IsFinished()
    {
        return worker->isFinished();
    }
}
