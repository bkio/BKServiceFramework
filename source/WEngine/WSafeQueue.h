// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WSafeQueue
#define Pragma_Once_WSafeQueue

#include "WEngine.h"
#include "WMutex.h"
#include <queue>

// A threadsafe-queue.
template <class T>
class WSafeQueue
{
public:
    WSafeQueue()
            : q()
            , m()
    {}

    ~WSafeQueue()
    {}

    // Add an element to the queue.
    void Push(T t)
    {
        WScopeGuard lock(&m);
        q.push(t);
    }

    // Get the "front"-element.
    bool Pop(T& val)
    {
        WScopeGuard lock(&m);
        if(q.empty())
        {
            return false;
        }
        val = q.front();
        q.pop();
        return true;
    }

private:
    std::queue<T> q;
    mutable WMutex m;
};

#endif //Pragma_Once_WSafeQueue