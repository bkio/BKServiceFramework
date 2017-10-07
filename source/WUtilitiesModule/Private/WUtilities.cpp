// Copyright Pagansoft.com, All rights reserved.

#include "WUtilities.h"
#include <chrono>

void UWUtilities::Print(EWLogType LogType, const FString& Format)
{
}

int64 UWUtilities::GetTimeStampInMS()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
int32 UWUtilities::GetSafeTimeStampInMS()
{
    int64 currentTime = GetTimeStampInMS();
    const int32 MaxInt = 2147483647;
    return (int32)(currentTime > MaxInt ? (currentTime % MaxInt) : currentTime);
}