// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WSafeQueue
#define Pragma_Once_WSafeQueue

#include "WEngine.h"
#include "WMutex.h"
#include "WQueue.h"

// A threadsafe-queue.
template <class T>
class WSafeQueue : public WQueue<T>
{

public:
    WSafeQueue() : WQueue<T>(), m()
    {
    }

    ~WSafeQueue() override
    {
    }

    // Add an element to the queue.
    void Push(T t) override
    {
        WScopeGuard lock(&m);
        WQueue<T>::Push(t);
    }

    // Get the "front"-element.
    bool Pop(T& val) override
    {
        WScopeGuard lock(&m);
        return WQueue<T>::Pop(val);
    }

    int32 Size() override
    {
        WScopeGuard lock(&m);
        return WQueue<T>::Size();
    }

    void Clear() override
    {
        WScopeGuard lock(&m);
        WQueue<T>::Clear();
    }

    void ReplaceQueue(WQueue<T>& Other, bool NoCopy) override
    {
        WScopeGuard lock(&m);
        WQueue<T>::ReplaceQueue(Other, NoCopy);
    }

    //Other queue must be a temporary WQueue, not WSafeQueue.
    void AddAll_NotTSTemporaryQueue(WQueue<T>& Other)
    {
        if (Other.Size() == 0) return;

        WScopeGuard lock(&m);

        if (WQueue<T>::Size() > 0)
        {
            T TmpVal;
            while (Other.Pop(TmpVal))
            {
                WQueue<T>::Push(TmpVal);
            }
        }
        else
        {
            WQueue<T>::ReplaceQueue(Other, true);
        }
    }

    //Other queue must be a WQueue, not WSafeQueue.
    void CopyTo(WQueue<T>& Other, bool bThenClearThis) override
    {
        WScopeGuard lock(&m);
        WQueue<T>::CopyTo(Other, bThenClearThis);
    }

private:
    mutable WMutex m;
};

#endif //Pragma_Once_WSafeQueue