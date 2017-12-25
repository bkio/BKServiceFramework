// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WElementWrapper
#define Pragma_Once_WElementWrapper

#include "WEngine.h"
#include "WMutex.h"

template <class T1, class T2>
class WNonComparable_ElementWrapper
{

public:
    WNonComparable_ElementWrapper(T1 _Comparable, T2 _NonComparable)
    {
        Comparable = _Comparable;
        NonComparable = _NonComparable;
    }
    ~WNonComparable_ElementWrapper() = default;

    bool operator==( const WNonComparable_ElementWrapper& O ) const
    {
        return O.Comparable == Comparable;
    }
    bool operator!=( const WNonComparable_ElementWrapper& O ) const
    {
        return O.Comparable != Comparable;
    }

    T1 Comparable;
    T2 NonComparable;
};

#endif //Pragma_Once_WElementWrapper