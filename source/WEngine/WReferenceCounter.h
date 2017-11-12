// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WReferenceCounter
#define Pragma_Once_WReferenceCounter

#include "WEngine.h"
#include "WMutex.h"

class WReferenceCountable
{

private:
    friend class WReferenceCounter_Internal;

    int32 ReferenceCounter = 0;
    WMutex ReferenceCounter_Mutex;

protected:
    WReferenceCountable() = default;

public:
    bool IsReferenced()
    {
        return ReferenceCounter > 0;
    }
};

#define WReferenceCounter volatile WReferenceCounter_Internal
class WReferenceCounter_Internal
{

private:
    WReferenceCountable* CountableObject = nullptr;

public:
    explicit WReferenceCounter_Internal(WReferenceCountable* _CountableObject)
    {
        if (_CountableObject)
        {
            CountableObject = _CountableObject;

            WScopeGuard ReferenceCounter_Guard(&CountableObject->ReferenceCounter_Mutex);
            CountableObject->ReferenceCounter++;
        }
    }
    ~WReferenceCounter_Internal()
    {
        if (CountableObject)
        {
            WScopeGuard ReferenceCounter_Guard(&CountableObject->ReferenceCounter_Mutex);
            CountableObject->ReferenceCounter--;
        }
    }
};

#endif //Pragma_Once_WReferenceCounter