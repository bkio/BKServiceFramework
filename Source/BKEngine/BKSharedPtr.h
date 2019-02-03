// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKSharedPtr
#define Pragma_Once_BKSharedPtr

class TSharedPtr_ReferenceCounter
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
    friend class TSharedPtr;

    TSharedPtr_ReferenceCounter() = default;
};

template <typename T>
class TSharedPtr
{
private:
    T* Pointer;
    TSharedPtr_ReferenceCounter* ReferenceCounter;

public:
    TSharedPtr() : Pointer(nullptr), ReferenceCounter(nullptr)
    {
        ReferenceCounter = new TSharedPtr_ReferenceCounter();
        ReferenceCounter->AddRef();
    }

    TSharedPtr(T* _PointerValue) : Pointer(_PointerValue), ReferenceCounter(nullptr)
    {
        ReferenceCounter = new TSharedPtr_ReferenceCounter();
        ReferenceCounter->AddRef();
    }

    TSharedPtr(const TSharedPtr<T>& _OtherSharedPtr) : Pointer(_OtherSharedPtr.Pointer), ReferenceCounter(_OtherSharedPtr.ReferenceCounter)
    {
        // Copy constructor
        ReferenceCounter->AddRef();
    }

    ~TSharedPtr()
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

    TSharedPtr<T>& operator = (const TSharedPtr<T>& _OtherSharedPtr)
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
            Pointer = _OtherSharedPtr.pData;
            ReferenceCounter = _OtherSharedPtr.reference;
            ReferenceCounter->AddRef();
        }
        return *this;
    }
};

#endif //Pragma_Once_BKSharedPtr
