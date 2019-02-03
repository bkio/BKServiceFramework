// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKSafeQueue
#define Pragma_Once_BKSafeQueue

#include "BKEngine.h"
#include "BKMutex.h"
#include "BKQueue.h"

// A threadsafe-queue.
template <class T>
class BKSafeQueue : public BKQueue<T>
{

public:
    BKSafeQueue() : BKQueue<T>(), m()
    {
    }

    ~BKSafeQueue() override = default;

    // Add an element to the queue.
    void Push(T t) override
    {
        BKScopeGuard lock(&m);
        BKQueue<T>::Push(t);
    }

    // Get the "front"-element.
    bool Pop(T& val) override
    {
        BKScopeGuard lock(&m);
        return BKQueue<T>::Pop(val);
    }

    int32 Size() override
    {
        BKScopeGuard lock(&m);
        return BKQueue<T>::Size();
    }

    void Clear() override
    {
        BKScopeGuard lock(&m);
        BKQueue<T>::Clear();
    }

    void ReplaceQueue(BKQueue<T>& Other, bool NoCopy) override
    {
        BKScopeGuard lock(&m);
        BKQueue<T>::ReplaceQueue(Other, NoCopy);
    }

    //Other queue must be a temporary BKQueue, not BKSafeQueue.
    void AddAll_NotTSTemporaryQueue(BKQueue<T>& Other)
    {
        if (Other.Size() == 0) return;

        BKScopeGuard lock(&m);

        if (BKQueue<T>::Size() > 0)
        {
            T TmpVal;
            while (Other.Pop(TmpVal))
            {
                BKQueue<T>::Push(TmpVal);
            }
        }
        else
        {
            BKQueue<T>::ReplaceQueue(Other, true);
        }
    }

    //Other queue must be a BKQueue, not BKSafeQueue.
    void CopyTo(BKQueue<T>& Other, bool bThenClearThis) override
    {
        BKScopeGuard lock(&m);
        BKQueue<T>::CopyTo(Other, bThenClearThis);
    }

private:
    mutable BKMutex m;
};

#endif //Pragma_Once_BKSafeQueue