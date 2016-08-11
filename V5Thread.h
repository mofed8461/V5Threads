//
//  V5 Engine
//
//  Created by mofed arafat on 6/4/15.
//  Copyright (c) 2016 org. All rights reserved.
//


#pragma once
#include <iostream>



namespace V5Graphics
{
    class impv5thread;
    class V5Thread
    {
    private:
        impv5thread* thread;
    public:
        V5Thread(void *(*func)(void *), void* param);
        
        ~V5Thread();
        
        void Join() const;
        
        bool IsCompleted() const;
        
        static void ThreadExit();
    };
    
    class impv5mutex;
    class V5Mutex
    {
        friend class V5Condition;
    private:
        impv5mutex* mutex;
    public:
        V5Mutex();
        ~V5Mutex();
        bool TryLock();
        void Lock();
        void Unlock();
    };
    
    //class impv5condition;
    //class V5Condition
    //{
    //private:
    //    impv5condition* condition;
    //public:
    //    V5Condition();
    //    ~V5Condition();
    //    void Wait(V5Mutex* mutex);
    //    void Signal();
    //};
    class impv5worker;
    class V5Worker
    {
        impv5worker* worker;
    public:
        V5Worker();
        ~V5Worker();
        void AddTask(void *(*func)(void*), void* args);
        void StartAll();
        void WaitForAll();
        bool IsFinished();
    };
}

