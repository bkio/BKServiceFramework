// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WTaskDefines
#define Pragma_Once_WTaskDefines

#include <utility>

#include "WEngine.h"
#include "WThread.h"
#include "WConditionVariable.h"

struct FWAsyncTaskParameter
{

public:
    FWAsyncTaskParameter() = default;
    virtual ~FWAsyncTaskParameter() = default;
};

typedef std::function<void(TArray<FWAsyncTaskParameter*>)> WFutureAsyncTask;

struct FWAwaitingTask
{

public:
    uint32 PassedTimeMs = 0;
    uint32 WaitTimeMs = 0;
    bool bLoop = false;
    bool bDoNotDeallocateParameters = false;

    bool bQueued = false;
    double QueuedTimestamp = 0;

    WFutureAsyncTask FunctionPtr;
    TArray<FWAsyncTaskParameter*> Parameters;

    //For normal tasks
    FWAwaitingTask(WFutureAsyncTask& Function, TArray<FWAsyncTaskParameter*>& Array, bool _bDoNotDeallocateParameters = false)
    {
        FunctionPtr = Function;
        Parameters = Array;
        bDoNotDeallocateParameters = _bDoNotDeallocateParameters;
    }

    //For scheduled tasks
    FWAwaitingTask(WFutureAsyncTask& Function, TArray<FWAsyncTaskParameter*>& Array, uint32 _WaitTimeMs, bool _bLoop, bool _bDoNotDeallocateParameters = false)
    {
        FunctionPtr = Function;
        Parameters = Array;
        WaitTimeMs = _WaitTimeMs;
        bLoop = _bLoop;
        bDoNotDeallocateParameters = _bDoNotDeallocateParameters;
    }
};

#endif //Pragma_Once_WTaskDefines