// Copyright Pagansoft.com, All rights reserved.

#include "WEngine.h"
#include <iostream>
#include "WUtilities.h"

int main()
{
    UWUtilities::Print(EWLogType::Log, L"Test");

    int32 exit_signal;
    std::cin >> exit_signal;
    return 0;
}