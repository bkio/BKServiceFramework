// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKHashMap
#define Pragma_Once_BKHashMap

#include <unordered_map>
#include "BKSharedPtr.h"

#define BK_HASH_MAP_TABLE_SIZE 20

// Hash node class template
template <typename K, typename V>
class BKHashNode
{
public:
    BKHashNode(const K &_Key, const V &_Value) : KeyMember(_Key), ValueMember(_Value) {}

    K GetKey() const
    {
        return KeyMember;
    }

    V GetValue() const
    {
        return ValueMember;
    }

private:
    K KeyMember;
    V ValueMember;
};

// Hash map class template
template <typename K, typename V>
class BKHashMap
{
public:
    BKHashMap() = default;

    ~BKHashMap()
    {
        Clear();
    }

    void Clear()
    {
        HashMap.clear();
    }

    void Iterate(std::function<void(BKSharedPtr<class BKHashNode<K, V>>)> _Callback)
    {
        TArray<BKSharedPtr<class BKHashNode<K, V>>> TmpArray;
        for (auto& Iterator : HashMap)
        {
            TmpArray.Add(BKSharedPtr<class BKHashNode<K, V>>(new BKHashNode<K, V>(Iterator.first, Iterator.second)));
        }
        for (int32 i = TmpArray.Num() - 1; i >= 0; i--)
        {
            _Callback(TmpArray[i]);
        }
    }

    bool Get(const K &_Key, V &_Value)
    {
        if (HashMap.find(_Key) != HashMap.end())
        {
            _Value = HashMap.at(_Key);
            return true;
        }
        return false;
    }

    void Put(const K &_Key, const V &_Value)
    {
        HashMap.insert(std::make_pair(_Key, _Value));
    }

    void Remove(const K &_Key)
    {
        HashMap.erase(_Key);
    }

    bool IsEmpty()
    {
        return HashMap.empty();
    }

private:
    std::unordered_map<K, V> HashMap;
};

#endif //Pragma_Once_BKHashMap