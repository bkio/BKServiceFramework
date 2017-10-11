// Copyright Pagansoft.com, All rights reserved.

#include "WEngine.h"
#include "WUtilities.h"
#include <iostream>

int main()
{
    UWUtilities::Print(EWLogType::Log, L"Test");

    int32 exit_signal;
    std::cin >> exit_signal;
    return 0;
}