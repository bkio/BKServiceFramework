// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WTaskDefines
#define Pragma_Once_WTaskDefines

#include "WEngine.h"
#include "WThread.h"
#include "WConditionVariable.h"
#include <utility>

class WAsyncTaskParameter
{

public:
    WAsyncTaskParameter() = default;
    virtual ~WAsyncTaskParameter() = default;
};

typedef std::function<void(TArray<WAsyncTaskParameter*>)> WFutureAsyncTask;

struct FWAwaitingTask
{

public:
    uint32 PassedTimeMs = 0;
    uint32 WaitTimeMs = 0;
    bool bLoop = false;
    bool bDoNotDeallocateParameters = false;

    bool bQueued = false;
    double QueuedTimestamp = 0;

    uint32 TaskUniqueIx = 0;

    WFutureAsyncTask FunctionPtr;
    TArray<WAsyncTaskParameter*> Parameters;

    //For normal tasks
    FWAwaitingTask(WFutureAsyncTask& Function, TArray<WAsyncTaskParameter*>& Array, bool _bDoNotDeallocateParameters = false)
    {
        FunctionPtr = Function;
        Parameters = Array;
        bDoNotDeallocateParameters = _bDoNotDeallocateParameters;
    }

    //For scheduled tasks
    FWAwaitingTask(uint32 TaskIx, WFutureAsyncTask& Function, TArray<WAsyncTaskParameter*>& Array, uint32 _WaitTimeMs, bool _bLoop, bool _bDoNotDeallocateParameters = false)
    {
        TaskUniqueIx = TaskIx;
        FunctionPtr = Function;
        Parameters = Array;
        WaitTimeMs = _WaitTimeMs;
        bLoop = _bLoop;
        bDoNotDeallocateParameters = _bDoNotDeallocateParameters;
    }
};

template <typename T>
class WGenericParameter : public WAsyncTaskParameter
{

private:
    WGenericParameter() = default;

    T Value;

    WMutex Mutex;

public:
    explicit WGenericParameter(T _Value)
    {
        Value = _Value;
    }

    T GetValue()
    {
        return Value;
    }
    void SetValue(T _Value)
    {
        Value = _Value;
    }

    void LockValue()
    {
        Mutex.lock();
    }
    void UnlockValue()
    {
        Mutex.unlock();
    }
};

#endif //Pragma_Once_WTaskDefines