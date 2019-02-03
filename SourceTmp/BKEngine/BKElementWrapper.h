// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKElementWrapper
#define Pragma_Once_BKElementWrapper

#include "BKEngine.h"
#include "BKMutex.h"

template <class T1, class T2>
class BKNonComparable_ElementWrapper
{

public:
    BKNonComparable_ElementWrapper(T1 _Comparable, T2 _NonComparable)
    {
        Comparable = _Comparable;
        NonComparable = _NonComparable;
    }
    ~BKNonComparable_ElementWrapper() = default;

    bool operator==( const BKNonComparable_ElementWrapper& O ) const
    {
        return O.Comparable == Comparable;
    }
    bool operator!=( const BKNonComparable_ElementWrapper& O ) const
    {
        return O.Comparable != Comparable;
    }

    T1 Comparable;
    T2 NonComparable;
};

#endif //Pragma_Once_BKElementWrapper