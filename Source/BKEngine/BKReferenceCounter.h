// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKReferenceCounter
#define Pragma_Once_BKReferenceCounter

#include "BKEngine.h"
#include "BKMutex.h"

class BKReferenceCountable
{

private:
    friend class BKReferenceCounter_Internal;

    int32 ReferenceCounter = 0;
    BKMutex ReferenceCounter_Mutex;

protected:
    BKReferenceCountable() = default;

public:
    bool IsReferenced()
    {
        return ReferenceCounter > 0;
    }
};

#define BKReferenceCounter volatile BKReferenceCounter_Internal
class BKReferenceCounter_Internal
{

private:
    BKReferenceCountable* CountableObject = nullptr;

public:
    explicit BKReferenceCounter_Internal(BKReferenceCountable* _CountableObject)
    {
        if (_CountableObject)
        {
            CountableObject = _CountableObject;

            BKScopeGuard ReferenceCounter_Guard(&CountableObject->ReferenceCounter_Mutex);
            CountableObject->ReferenceCounter++;
        }
    }
    ~BKReferenceCounter_Internal()
    {
        if (CountableObject)
        {
            BKScopeGuard ReferenceCounter_Guard(&CountableObject->ReferenceCounter_Mutex);
            CountableObject->ReferenceCounter--;
        }
    }
};

#endif //Pragma_Once_BKReferenceCounter