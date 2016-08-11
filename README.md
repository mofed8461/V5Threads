# V5Threads
Cross-platform C++ implementation for threads and workers, perfect to do scheduled tasks

its very simple this example for worker and mutex
you can put this implementation inside any function without implementing structs outside function

    //4 workers
    V5Worker workers[4];//(0% cpu usage, sleeping workers are waiting)
    
    //we need to calculate the sum of arr[100] in parallel
    int32_t arr[100];
    
    for (int32_t i = 0; i < 100; ++i)
        arr[i] = rand() % 100;
    
    //mutex to bound the sum (as shared buffer)
    V5Mutex mutex;
    int32_t sum = 0;
    
    //parameters we need to pass to every worker
    //you can put this inside any function
    struct args
    {
        int32_t* arr;
        int32_t* sum;
        V5Mutex* mutex;
    };
    
    //i'm using this way to create function inside function
    //kind of anonymous function
    struct justCLASS
    {
        static void *func(void* _args)
        {
            struct args* args = (struct args*)_args;
            int32_t sum = 0;
            
            // 100 elements / 4 workers = 25 elements
            for (int32_t i = 0; i < 25; ++i)
                sum += args->arr[i];
            
            
            args->mutex->Lock();
            *args->sum += sum;
            args->mutex->Unlock();
            
            delete args;
            return NULL;
        }
    };
    
    
    for (int32_t i = 0; i < 100; i += 25)
    {
        //add task to each worker
        args* arg = new args;
        arg->mutex = &mutex;
        arg->sum = &sum;
        arg->arr = arr + i;
        workers[i / 25].AddTask(justCLASS::func, arg);
    }
    
    // start all tasks for each worker
    for (int32_t i = 0; i < 4; ++i)
        workers[i].StartAll();
    
    // wait until finish
    for (int32_t i = 0; i < 4; ++i)
        workers[i].WaitForAll();
    
    cout << sum;
