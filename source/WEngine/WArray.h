// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WArray
#define Pragma_Once_WArray

#include "WEngine.h"
#include <vector>
#include <algorithm>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ClangTidyInspection"

template <class T>
class TArray
{

private:
    std::vector<T> Array;

public:
    TArray() {}
    TArray( const TArray<T>& Other)
    {
        Array = Other.Array;
    }
    TArray<T>& operator=(const TArray<T>& Other)
    {
        Array = Other.Array;
        return *this;
    }
    TArray(const T* Other, int32 Length)
    {
        for (int32 i = 0; i < Length; i++)
        {
            Array.push_back(Other[i]);
        }
    }
    explicit TArray(T Other)
    {
        Array.push_back(Other);
    }
    bool operator==(const TArray<T>& Other)
    {
        return Array == Other.Array;
    }
    bool operator!=(const TArray<T>& Other)
    {
        return Array != Other.Array;
    }

    void Add(const T& Item)
    {
        Array.push_back(Item);
    }
    void Add(const T* Ptr, int32 Count)
    {
        std::vector<T> NewItems;
        for (int32 i = 0; i < Count; i++)
        {
            NewItems.push_back(Ptr + i);
        }
        Array.insert(Array.end(), NewItems.begin(), NewItems.end());
    }
    bool AddUnique(const T& Item)
    {
        if (std::find(Array.begin(), Array.end(), Item) == Array.end())
        {
            Array.push_back(Item);
            return true;
        }
        return false;
    }
    void Append(const TArray<T>& Other)
    {
        for (auto& Item : Other.Array)
        {
            Add(Item);
        }
    }
    void AppendUnique(const TArray<T>& Other)
    {
        for (auto& Item : Other.Array)
        {
            AddUnique(Item);
        }
    }
    bool Contains(const T& Item)
    {
        return std::find(Array.begin(), Array.end(), Item) == Array.end();
    }
    void Empty(int32 Slack = 0)
    {
        Array.clear();
        if (Slack > 0)
        {
            Array.resize(Slack);
        }
    }
    int32 Find(const T& Item)
    {
        int32 Ix = 0;
        for (auto ExIt = Array.begin(); ExIt != Array.end() ; ++ExIt)
        {
            if (Item == *ExIt)
            {
                return Ix;
            }
            Ix++;
        }
        return INDEX_NONE;
    }
    bool Find(const T& Item, int32& Index)
    {
        Index = INDEX_NONE;
        int32 Ix = 0;
        for (auto ExIt = Array.begin(); ExIt != Array.end() ; ++ExIt)
        {
            if (Item == *ExIt)
            {
                Index = Ix;
                return true;
            }
            Ix++;
        }
        return false;
    }
    int32 FindLast(const T& Item)
    {
        int32 Ix = Array.size() - 1;
        for (auto ExIt = Array.end(); ExIt != Array.begin() ; --ExIt)
        {
            if (Item == *ExIt)
            {
                return Ix;
            }
            Ix--;
        }
        return INDEX_NONE;
    }
    bool FindLast(const T& Item, int32& Index)
    {
        Index = INDEX_NONE;
        int32 Ix = Array.size() - 1;
        for (auto ExIt = Array.end(); ExIt != Array.begin() ; --ExIt)
        {
            if (Item == *ExIt)
            {
                Index = Ix;
                return true;
            }
            Ix--;
        }
        return false;
    }
    const T* GetData() const
    {
        return Array.data();
    }
    T* GetMutableData()
    {
        return Array.data();
    }
    void Insert(const T& Item, int32 Index)
    {
        Array.insert(Array.begin() + Index, Item);
    }
    void Insert(const T* Ptr, int32 Count, int32 Index)
    {
        std::vector<T> NewItems;
        for (int32 i = 0; i < Count; i++)
        {
            NewItems.push_back(*(Ptr + i));
        }
        Array.insert(Array.begin() + Index, NewItems.begin(), NewItems.end());
    }
    void Insert(const T& Item, int32 Count, int32 Index)
    {
        std::vector<T> NewItems;
        for (int32 i = 0; i < Count; i++)
        {
            NewItems.push_back(Item);
        }
        Array.insert(Array.begin() + Index, NewItems.begin(), NewItems.end());
    }
    bool IsValidIndex(int32 Index)
    {
        return Index >= 0 && Index < Array.size();
    }
    const T& Last()
    {
        return Array.back();
    }
    const int32 Num() const
    {
        return Array.size();
    }
    const T& Pop()
    {
        if (!Array.empty())
        {
            return Array.pop_back();
        }
        return nullptr;
    }
    void Push(const T& Item)
    {
        Array.push_back(Item);
    }
    int32 Remove(const T& Item)
    {
        int32 RemovedNo = 0;
        for (auto ExIt = Array.end(); ExIt != Array.begin() ; --ExIt)
        {
            if (Item == *ExIt)
            {
                Array.erase(ExIt);
                RemovedNo++;
            }
        }
        return RemovedNo;
    }
    void RemoveAt(int32 Index, int32 Count = 1)
    {
        Array.erase(Array.begin() + Index, Array.begin() + Index + Count);
    }
    void Reset()
    {
        Array.clear();
    }
    T operator[](int32 Index) const
    {
        return Array.at(Index);
    }
    TArray<T>& operator+= (const TArray<T>& Other)
    {
        if (Other != *this)
        {
            Append(Other);
        }
        return *this;
    }
    void SetNum(int32 NewNum)
    {
        Array.resize(NewNum);
    }
    void SetNumUninitialized(int32 NewNum)
    {
        Array.reserve(NewNum);
    }
    void SetNumZeroed(int32 NewNum)
    {
        int32 OldSize = Array.size();
        Array.resize(NewNum);
        for (int32 i = OldSize; i < NewNum; i++)
        {
            Array[i] = (T)0;
        }
    }
    template<class... Args>
    void Emplace(Args&&... args)
    {
        Array.push_back(T(args...));
    }
    int32 AddUninitialized(int32 Count = 1)
    {
        const int32 OldNum = Array.size();
        Array.resize(Array.size() + Count);
        return OldNum;
    }
    void InsertUninitialized(int32 Index, int32 Count = 1)
    {
        const int32 OldNum = Array.size();
        Array.resize(Array.size() + Count);
    }
    int32 AddZeroed(int32 Count = 1)
    {
        const int32 Index = AddUninitialized(Count);
        for (int32 i = Index; i < Index + Count; i++)
        {
            Array[i] = (T)0;
        }
        return Index;
    }

    typedef T* iterator;
    typedef const T* const_iterator;
    iterator begin() { return &Array[0]; }
    const_iterator begin() const { return &Array[0]; }
    iterator end() { return &Array[Array.size()]; }
    const_iterator end() const { return &Array[Array.size()]; }
};

template <class T>
inline TArray<T> operator+ (const TArray<T>& Left, const TArray<T>& Right)
{
    TArray<T> NewArray = Left;
    NewArray += Right;
    return NewArray;
}
template <class T>
inline TArray<T> operator+ (const TArray<T>& Left, T Right)
{
    TArray<T> NewArray = Left;
    NewArray.Add(Right);
    return NewArray;
}
template <class T>
inline TArray<T> operator+ (const TArray<T>& Left, T* Right)
{
    TArray<T> NewArray = Left;
    NewArray += Right;
    return NewArray;
}

#endif