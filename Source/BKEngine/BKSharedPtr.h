// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKSharedPtr
#define Pragma_Once_BKSharedPtr

class BKSharedPtr_ReferenceCounter
{
private:
    int Count;

    void AddRef()
    {
        Count++;
    }

    int Release()
    {
        return --Count;
    }

    template<class T>
    friend class BKSharedPtr;

    BKSharedPtr_ReferenceCounter() = default;
};

template <typename T>
class BKSharedPtr
{
private:
    T* Pointer;
    BKSharedPtr_ReferenceCounter* ReferenceCounter;

public:
    BKSharedPtr() : Pointer(nullptr), ReferenceCounter(nullptr)
    {
        ReferenceCounter = new BKSharedPtr_ReferenceCounter();
        ReferenceCounter->AddRef();
    }

    BKSharedPtr(T* _PointerValue) : Pointer(_PointerValue), ReferenceCounter(nullptr)
    {
        ReferenceCounter = new BKSharedPtr_ReferenceCounter();
        ReferenceCounter->AddRef();
    }

    BKSharedPtr(const BKSharedPtr<T>& _OtherSharedPtr) : Pointer(_OtherSharedPtr.Pointer), ReferenceCounter(_OtherSharedPtr.ReferenceCounter)
    {
        // Copy constructor
        ReferenceCounter->AddRef();
    }

    ~BKSharedPtr()
    {
        if(ReferenceCounter->Release() == 0)
        {
            delete Pointer;
            delete ReferenceCounter;
        }
    }

    T& operator* ()
    {
        return *Pointer;
    }

    T* operator-> ()
    {
        return Pointer;
    }

    bool IsValid()
    {
        return Pointer != nullptr;
    }

    BKSharedPtr<T>& operator = (const BKSharedPtr<T>& _OtherSharedPtr)
    {
        // Assignment operator
        if (this != &_OtherSharedPtr) // Avoid self assignment
        {
            // Decrement the old reference count
            // if reference become zero delete the old data
            if(ReferenceCounter->Release() == 0)
            {
                delete Pointer;
                delete ReferenceCounter;
            }

            // Copy the data and reference pointer
            // and increment the reference count
            Pointer = _OtherSharedPtr.Pointer;
            ReferenceCounter = _OtherSharedPtr.ReferenceCounter;
            ReferenceCounter->AddRef();
        }
        return *this;
    }
};

#endif //Pragma_Once_BKSharedPtr
