// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKTaskDefines
#define Pragma_Once_BKTaskDefines

#include "BKEngine.h"
#include "BKThread.h"
#include "BKConditionVariable.h"
#include <utility>

class BKAsyncTaskParameter
{

public:
    BKAsyncTaskParameter() = default;
    virtual ~BKAsyncTaskParameter() = default;
};

typedef std::function<void(TArray<BKAsyncTaskParameter*>)> BKFutureAsyncTask;

struct FBKAwaitingTask
{

public:
    uint32 PassedTimeMs = 0;
    uint32 WaitTimeMs = 0;
    bool bLoop = false;
    bool bDoNotDeallocateParameters = false;

    bool bQueued = false;
    double QueuedTimestamp = 0;

    uint32 TaskUniqueIx = 0;

    BKFutureAsyncTask FunctionPtr;
    TArray<BKAsyncTaskParameter*> Parameters;

    //For normal tasks
    FBKAwaitingTask(BKFutureAsyncTask& Function, TArray<BKAsyncTaskParameter*>& Array, bool _bDoNotDeallocateParameters = false)
    {
        FunctionPtr = Function;
        Parameters = Array;
        bDoNotDeallocateParameters = _bDoNotDeallocateParameters;
    }

    //For scheduled tasks
    FBKAwaitingTask(uint32 TaskIx, BKFutureAsyncTask& Function, TArray<BKAsyncTaskParameter*>& Array, uint32 _WaitTimeMs, bool _bLoop, bool _bDoNotDeallocateParameters = false)
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
class BKGenericParameter : public BKAsyncTaskParameter
{

private:
    BKGenericParameter() = default;

    T Value;

    BKMutex Mutex;

public:
    explicit BKGenericParameter(T _Value)
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
        Mutex.Lock();
    }
    void UnlockValue()
    {
        Mutex.Unlock();
    }
};

#endif //Pragma_Once_BKTaskDefines