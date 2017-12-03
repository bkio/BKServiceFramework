// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WQueue
#define Pragma_Once_WQueue

#include "WEngine.h"
#include <queue>

template <class T>
class WQueue
{
public:
    WQueue() : q()
    {
    }

    virtual ~WQueue() = default;

    // Add an element to the queue.
    virtual void Push(T t)
    {
        q.push(t);
    }

    // Get the "front"-element.
    virtual bool Pop(T& val)
    {
        if(q.empty())
        {
            return false;
        }
        val = q.front();
        q.pop();
        return true;
    }

    virtual int32 Size()
    {
        return static_cast<int32>(q.size());
    }

    virtual void Clear()
    {
        std::queue<T> empty;
        std::swap(q, empty);
    }

    virtual void ReplaceQueue(WQueue<T>& Other, bool NoCopy)
    {
        if (NoCopy)
        {
            std::swap(q, Other.q);
        }
        else
        {
            std::queue<T> CopiedOther = Other.q;
            std::swap(q, CopiedOther);
        }
    }

    //Other queue must be a WQueue, not WSafeQueue.
    virtual void CopyTo(WQueue<T>& Other, bool bThenClearThis)
    {
        Other.q = q;
        if (bThenClearThis)
        {
            Clear();
        }
    }

    std::queue<T> q;
};

#endif //Pragma_Once_WQueue