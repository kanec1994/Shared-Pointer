#include <cstddef>
#include <stdio.h>
#include <assert.h>
#include <mutex>
#include <algorithm>
#include <functional>

#ifndef SHAREDPTR_HPP
#define SHAREDPTR_HPP

namespace cs540
{

template<typename T>
void del(void *obj)
{
    if(obj) delete static_cast<T *>(obj);
}

class RefCount
{
public:
    void (*delet)(void *);
    void *void_obj;
    RefCount();
    void inc();
    void dec();
    int get_count();
    ~RefCount();
private:
    std::mutex mtx;
    int ref_count;
};

RefCount::RefCount()
{
    ref_count = 1;
}

void RefCount::inc()
{
    mtx.lock();
    (ref_count)++;
    mtx.unlock();
}

void RefCount::dec()
{
    mtx.lock();
    (ref_count)--;
    mtx.unlock();
}

int RefCount::get_count()
{
    return ref_count;
}

RefCount::~RefCount()
{

}

template<typename T>
class SharedPtr
{
public:
    RefCount *owners;
    SharedPtr();
    template<typename U> explicit SharedPtr(U *);
    SharedPtr(const SharedPtr &ptr);
    template<typename U> SharedPtr(const SharedPtr<U> &ptr);
    template<typename U> SharedPtr(const SharedPtr<U> &ptr, U *cast_obj);
    SharedPtr(SharedPtr &&ptr);
    template<typename U> SharedPtr(SharedPtr<U> &&ptr);
    SharedPtr &operator=(const SharedPtr &ptr);
    template<typename U> SharedPtr<T> &operator=(const SharedPtr<U> &ptr);
    SharedPtr &operator=(SharedPtr &&ptr);
    template<typename U> SharedPtr &operator=(SharedPtr<U> &&ptr);
    void reset();
    template<typename U> void reset(U *ptr);
    T *get() const;
    T &operator*() const;
    T *operator->() const;
    explicit operator bool() const;
    ~SharedPtr();
private:
    T *obj;
    std::mutex mtx;
};

template<typename T>
SharedPtr<T>::SharedPtr()
{
    obj = nullptr;
    owners = nullptr;
}

template<typename T> template<typename U>
SharedPtr<T>::SharedPtr(U *ptr)
{
    owners = new RefCount();
    owners->delet = &del<U>;
    owners->void_obj = static_cast<void *>(ptr);
    obj = ptr;
}

template<typename T>
SharedPtr<T>::SharedPtr(const SharedPtr &ptr)
{
    if(ptr == nullptr)
    {
        obj = nullptr;
        owners = nullptr;
    }
    else
    {
        obj = ptr.get();
        owners = ptr.owners;
        owners->inc();
    }
}

template<typename T> template<typename U>
SharedPtr<T>::SharedPtr(const SharedPtr<U> &ptr)
{
    if(ptr == nullptr)
    {
        obj = nullptr;
        owners = nullptr;
    }
    else
    {
        obj = static_cast<T*>(ptr.get());
        owners = ptr.owners;
        owners->inc();
    }
}

template<typename T> template<typename U>
SharedPtr<T>::SharedPtr(const SharedPtr<U> &ptr, U *cast_obj)
{
    obj = dynamic_cast<T*>(cast_obj);
    owners = ptr.owners;
    if(owners == nullptr)
    {
        owners = new RefCount();
        owners->delet = ptr.owners->delet;
        owners->void_obj = ptr.owners->void_obj;
    }
    else
    {
        owners->inc();
    }
}

template<typename T>
SharedPtr<T>::SharedPtr(SharedPtr &&ptr)
{
    obj = ptr.obj;
    owners = ptr.owners;
    ptr.obj = nullptr;
    ptr.owners = nullptr;
}

template<typename T> template<typename U>
SharedPtr<T>::SharedPtr(SharedPtr<U> &&ptr)
{
    obj = ptr.obj;
    owners = ptr.owners;
    ptr.obj = nullptr;
    ptr.owners = nullptr;
}

template<typename T>
SharedPtr<T> &SharedPtr<T>::operator=(const SharedPtr &ptr)
{
    if(*this != ptr)
    {
        if(*this != nullptr)
        {
            owners->dec();
            if(owners->get_count() == 0)
            {
                mtx.lock();
                owners->delet(owners->void_obj);
                delete owners;
                mtx.unlock();
            }
        }
        if(ptr == nullptr)
        {
            obj = nullptr;
            owners = nullptr;
        }
        else
        {
            obj = ptr.get();
            owners = ptr.owners;
            owners->inc();
        }
    }
    return *this;
}

template<typename T> template<typename U>
SharedPtr<T> &SharedPtr<T>::operator=(const SharedPtr<U> &ptr)
{
    if(*this != ptr)
    {
        if(*this != nullptr)
        {
            owners->dec();
            if(owners->get_count() == 0)
            {
                mtx.lock();
                owners->delet(owners->void_obj);
                owners->void_obj = nullptr;
                delete owners;
                obj = nullptr;
                owners = nullptr;
                mtx.unlock();
            }
        }
        if(ptr == nullptr)
        {
            obj = nullptr;
            owners = nullptr;
        }
        else
        {
            obj = ptr.get();
            owners = ptr.owners;
            owners->inc();
        }
    }
    return *this;
}

template<typename T>
SharedPtr<T> &SharedPtr<T>::operator=(SharedPtr &&ptr)
{
    obj = ptr.obj;
    owners = ptr.owners;
    ptr.obj = nullptr;
    ptr.owners = nullptr;
    return *this;
}

template<typename T> template<typename U>
SharedPtr<T> &SharedPtr<T>::operator=(SharedPtr<U> &&ptr)
{
    obj = ptr.obj;
    owners = ptr.owners;
    ptr.obj = nullptr;
    ptr.owners = nullptr;
    return *this;
}

template<typename T>
void SharedPtr<T>::reset()
{
    if(this != nullptr)
    {
        if(owners != nullptr)
        {
            owners->dec();
            if(owners->get_count() == 0)
            {
                mtx.lock();
                if(obj != nullptr) owners->delet(owners->void_obj);
                owners->void_obj = nullptr;
                delete owners;
                obj = nullptr;
                owners = nullptr;
                mtx.unlock();
            }
        }
    }
    obj = nullptr;
    owners = nullptr;
}

template<typename T> template<typename U>
void SharedPtr<T>::reset(U *ptr)
{
    if(this != nullptr)
    {
        if(owners != nullptr)
        {
            owners->dec();
            if(owners->get_count() == 0)
            {
                mtx.lock();
                if(obj != nullptr) owners->delet(owners->void_obj);
                owners->void_obj = nullptr;
                delete owners;
                obj = nullptr;
                owners = nullptr;
                mtx.unlock();
            }
        }
    }
    obj = ptr;
    owners = new RefCount();
    owners->delet = &del<U>;
    owners->void_obj = static_cast<void *>(ptr);
}

template<typename T>
T *SharedPtr<T>::get() const
{
   return obj;
}

template<typename T>
T &SharedPtr<T>::operator*() const
{
    return *obj;
}

template<typename T>
T *SharedPtr<T>::operator->() const
{
    return obj;
}

template<typename T>
SharedPtr<T>::operator bool() const
{
    if(obj != nullptr) return true;
    else return false;
}

template<typename T>
SharedPtr<T>::~SharedPtr()
{
    if(owners != nullptr)
    {
        owners->dec();
        if(owners->get_count() == 0)
        {
            mtx.lock();
            if(obj != nullptr) owners->delet(owners->void_obj);
            owners->void_obj = nullptr;
            delete owners;
            obj = nullptr;
            owners = nullptr;
            mtx.unlock();
        }
    }
}

//Start of non-member functions
template<typename T1, typename T2>
bool operator==(const SharedPtr<T1> &ptr1, const SharedPtr<T2> &ptr2)
{
    return ptr1.get() == ptr2.get();
}

template<typename T>
bool operator==(const SharedPtr<T> &ptr, std::nullptr_t np)
{
    return ptr.get() == np;
}

template<typename T>
bool operator==(std::nullptr_t np, const SharedPtr<T> &ptr)
{
    return ptr.get() == np;
}

template<typename T1, typename T2>
bool operator!=(const SharedPtr<T1> &ptr1, const SharedPtr<T2> &ptr2)
{
    return ptr1.get() != ptr2.get();
}

template<typename T>
bool operator!=(const SharedPtr<T> &ptr, std::nullptr_t np)
{
    return ptr.get() != np;
}

template<typename T>
bool operator!=(std::nullptr_t np, const SharedPtr<T> &ptr)
{
    return ptr.get() != np;
}

template<typename T, typename U>
SharedPtr<T> static_pointer_cast(const SharedPtr<U> &sp)
{
    return SharedPtr<T>(sp);
}

template<typename T, typename U>
SharedPtr<T> dynamic_pointer_cast(const SharedPtr<U> &sp)
{
    return SharedPtr<T>(sp, sp.get());
}

#endif // SHAREDPTR_HPP

}
