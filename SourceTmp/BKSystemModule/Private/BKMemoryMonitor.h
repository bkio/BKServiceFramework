// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKMemoryMonitor
#define Pragma_Once_BKMemoryMonitor

#include "BKEngine.h"
#include "BKMutex.h"

class BKMemoryMonitor
{

private:
    BKMutex m_lock;

public:
    int32 GetUsage();
};

#endif //Pragma_Once_BKMemoryMonitor