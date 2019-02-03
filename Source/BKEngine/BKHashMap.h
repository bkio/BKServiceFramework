// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKHashMap
#define Pragma_Once_BKHashMap

#define BK_HASH_MAP_TABLE_SIZE 20

// Hash node class template
template <typename K, typename V>
class BKHashNode
{
public:
    BKHashNode(const K &_Key, const V &_Value) : KeyMember(_Key), ValueMember(_Value), Next(nullptr)
    {
    }

    K GetKey() const
    {
        return KeyMember;
    }

    V GetValue() const
    {
        return ValueMember;
    }

    void SetValue(V _Value)
    {
        ValueMember = _Value;
    }

    BKHashNode *GetNext() const
    {
        return Next;
    }

    void SetNext(BKHashNode *_Next)
    {
        Next = _Next;
    }

private:
    K KeyMember;
    V ValueMember;
    BKHashNode *Next;
};

// Default hash function class
template <typename K>
struct BKDefaultKeyHash
{
    unsigned long operator()(const K& _Key) const
    {
        return reinterpret_cast<unsigned long>(_Key) % BK_HASH_MAP_TABLE_SIZE;
    }
};

struct BKFStringKeyHash
{
    unsigned long operator()(const FString& _Key) const
    {
        return std::hash<std::wstring>()(_Key.GetWideCharString()) % BK_HASH_MAP_TABLE_SIZE;
    }
};

// Hash map class template
template <typename K, typename V, typename F = BKDefaultKeyHash<K>>
class BKHashMap
{
public:
    BKHashMap()
    {
        // construct zero initialized hash table of size
        Table = new BKHashNode<K, V> *[BK_HASH_MAP_TABLE_SIZE]();
    }

    ~BKHashMap()
    {
        // destroy all buckets one by one
        Clear();
        // destroy the hash table
        delete[] Table;
    }

    void Clear()
    {
        // destroy all buckets one by one
        for (int i = 0; i < BK_HASH_MAP_TABLE_SIZE; ++i)
        {
            BKHashNode<K, V> *Entry = Table[i];
            while (Entry)
            {
                BKHashNode<K, V> *Prev = Entry;
                Entry = Entry->GetNext();
                delete Prev;
            }
            Table[i] = nullptr;
        }
        ElementNo = 0;
    }

    void Iterate(std::function<void(class BKHashNode<K, V>*)> _Callback)
    {
        for (int i = 0; i < BK_HASH_MAP_TABLE_SIZE; ++i)
        {
            BKHashNode<K, V> *Entry = Table[i];
            while (Entry)
            {
                BKHashNode<K, V> *Prev = Entry;
                Entry = Entry->GetNext();
                _Callback(Prev);
            }
        }
    }

    bool Get(const K &_Key, V &_Value)
    {
        unsigned long HashValue = HashFunc(_Key);
        BKHashNode<K, V> *Entry = Table[HashValue];

        while (Entry)
        {
            if (Entry->GetKey() == _Key)
            {
                _Value = Entry->GetValue();
                return true;
            }
            Entry = Entry->GetNext();
        }
        return false;
    }

    void Put(const K &_Key, const V &_Value)
    {
        unsigned long HashValue = HashFunc(_Key);
        BKHashNode<K, V> *Prev = nullptr;
        BKHashNode<K, V> *Entry = Table[HashValue];

        while (Entry && Entry->GetKey() != _Key)
        {
            Prev = Entry;
            Entry = Entry->GetNext();
        }

        if (Entry == nullptr)
        {
            Entry = new BKHashNode<K, V>(_Key, _Value);
            if (Prev == nullptr)
            {
                // insert as first bucket
                Table[HashValue] = Entry;
            }
            else
            {
                Prev->SetNext(Entry);
            }
            ElementNo++;
        }
        else
        {
            // just update the value
            Entry->SetValue(_Value);
        }
    }

    void Remove(const K &_Key)
    {
        unsigned long HashValue = HashFunc(_Key);
        BKHashNode<K, V> *Prev = nullptr;
        BKHashNode<K, V> *Entry = Table[HashValue];

        while (Entry && Entry->GetKey() != _Key)
        {
            Prev = Entry;
            Entry = Entry->GetNext();
        }

        if (Entry == nullptr)
        {
            // key not found
            return;
        }
        else
        {
            if (Prev == nullptr)
            {
                // remove first bucket of the list
                Table[HashValue] = Entry->GetNext();
            }
            else
            {
                Prev->SetNext(Entry->GetNext());
            }
            delete Entry;

            ElementNo--;
        }
    }

    int Size()
    {
        return ElementNo;
    }

    bool IsEmpty()
    {
        return ElementNo == 0;
    }

private:
    // hash table
    BKHashNode<K, V>** Table;
    F HashFunc;

    int ElementNo = 0;
};

#endif //Pragma_Once_BKHashMap