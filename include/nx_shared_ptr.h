/**
 * @file  shared_ptr.hpp
 * @brief shared_ptr is a minimal implementation of smart pointer, a subset of the C++11 std::shared_ptr or boost::shared_ptr.
 *
 * Copyright (c) 2013-2019 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Modified for use within NetXMS
 *
 * Distributed under the MIT License (MIT) (See http://opensource.org/licenses/MIT)
 */

#ifndef _nx_shared_ptr_h_
#define _nx_shared_ptr_h_

#include <cstddef>      // NULL
#include <algorithm>    // std::swap
#include <nxatomic.h>

/**
 * @brief implementation of reference counter for the following minimal smart pointer.
 *
 * shared_ptr_count is a container for the allocated pn reference counter.
 */
class shared_ptr_count
{
public:
    shared_ptr_count()
    {
       refcount = NULL;
    }

    shared_ptr_count(const shared_ptr_count& count)
    {
       refcount = count.refcount;
    }

    /// @brief Swap method for the copy-and-swap idiom (copy constructor and swap method)
    void swap(shared_ptr_count& lhs)
    {
        std::swap(refcount, lhs.refcount);
    }

    /// @brief getter of the underlying reference counter
    long use_count(void) const
    {
        long count = 0;
        if (NULL != refcount)
        {
            count = *refcount;
        }
        return count;
    }

    /// @brief acquire/share the ownership of the pointer, initializing the reference counter
    template<class U>
    void acquire(U* p)
    {
        if (NULL != p)
        {
            if (NULL == refcount)
            {
               refcount = new VolatileCounter(1);
            }
            else
            {
               InterlockedIncrement(refcount);
            }
        }
    }
    /// @brief release the ownership of the px pointer, destroying the object when appropriate
    template<class U>
    void release(U* p)
    {
        if (refcount != NULL)
        {
            if (InterlockedDecrement(refcount) == 0)
            {
                delete p;
                delete refcount;
            }
            refcount = NULL;
        }
    }

private:
    VolatileCounter *refcount;
};


/**
 * @brief minimal implementation of smart pointer, a subset of the C++11 std::shared_ptr or boost::shared_ptr.
 *
 * shared_ptr is a smart pointer retaining ownership of an object through a provided pointer,
 * and sharing this ownership with a reference counter.
 * It destroys the object when the last shared pointer pointing to it is destroyed or reset.
 */
template<class T>
class shared_ptr
{
public:
    /// The type of the managed object, aliased as member type
    typedef T element_type;

    /// @brief Default constructor
    shared_ptr(void) : // never throws
        px(NULL),
        pn()
    {
    }
    /// @brief Constructor with the provided pointer to manage
    explicit shared_ptr(T* p) : // may throw std::bad_alloc
      //px(p), would be unsafe as acquire() may throw, which would call release() in destructor
        pn()
    {
        acquire(p);   // may throw std::bad_alloc
    }
    /// @brief Constructor to share ownership. Warning : to be used for pointer_cast only ! (does not manage two separate <T> and <U> pointers)
    template <class U>
    shared_ptr(const shared_ptr<U>& ptr, T* p) :
     //px(p), would be unsafe as acquire() may throw, which would call release() in destructor
       pn(ptr.pn)
    {
       acquire(p);   // may throw std::bad_alloc
    }
    /// @brief Copy constructor to convert from another pointer type
    template <class U>
    shared_ptr(const shared_ptr<U>& ptr) :
      //px(ptr.px),
        pn(ptr.pn)
    {
        acquire(static_cast<typename shared_ptr<T>::element_type*>(ptr.px));   // will never throw std::bad_alloc
    }
    /// @brief Copy constructor (used by the copy-and-swap idiom)
    shared_ptr(const shared_ptr& ptr) :
       //px(ptr.px),
        pn(ptr.pn)
    {
        acquire(ptr.px);   // will never throw std::bad_alloc
    }
    /// @brief Assignment operator using the copy-and-swap idiom (copy constructor and swap method)
    shared_ptr& operator=(shared_ptr ptr)
    {
        swap(ptr);
        return *this;
    }
    /// @brief the destructor releases its ownership
    ~shared_ptr(void)
    {
        release();
    }
    /// @brief this reset releases its ownership
    void reset(void)
    {
        release();
    }
    /// @brief this reset release its ownership and re-acquire another one
    void reset(T* p)
    {
        release();
        acquire(p);
    }

    /// @brief Swap method for the copy-and-swap idiom (copy constructor and swap method)
    void swap(shared_ptr& lhs)
    {
        std::swap(px, lhs.px);
        pn.swap(lhs.pn);
    }

    // reference counter operations :
    operator bool() const
    {
        return (0 < pn.use_count());
    }
    bool unique(void) const
    {
        return (1 == pn.use_count());
    }
    long use_count(void) const
    {
        return pn.use_count();
    }

    // underlying pointer operations :
    T& operator*() const
    {
        return *px;
    }
    T* operator->() const
    {
        return px;
    }
    T* get(void) const
    {
        return px;
    }

private:
    /// @brief acquire/share the ownership of the px pointer, initializing the reference counter
    void acquire(T* p)
    {
        pn.acquire(p);
        px = p; // here it is safe to acquire the ownership of the provided raw pointer, where exception cannot be thrown any more
    }

    /// @brief release the ownership of the px pointer, destroying the object when appropriate
    void release(void)
    {
        pn.release(px);
        px = NULL;
    }

private:
    // This allow pointer_cast functions to share the reference counter between different shared_ptr types
    template<class U>
    friend class shared_ptr;

private:
    T*                  px; //!< Native pointer
    shared_ptr_count    pn; //!< Reference counter
};


// comparaison operators
template<class T, class U> bool operator==(const shared_ptr<T>& l, const shared_ptr<U>& r)
{
    return (l.get() == r.get());
}
template<class T, class U> bool operator!=(const shared_ptr<T>& l, const shared_ptr<U>& r)
{
    return (l.get() != r.get());
}
template<class T, class U> bool operator<=(const shared_ptr<T>& l, const shared_ptr<U>& r)
{
    return (l.get() <= r.get());
}
template<class T, class U> bool operator<(const shared_ptr<T>& l, const shared_ptr<U>& r)
{
    return (l.get() < r.get());
}
template<class T, class U> bool operator>=(const shared_ptr<T>& l, const shared_ptr<U>& r)
{
    return (l.get() >= r.get());
}
template<class T, class U> bool operator>(const shared_ptr<T>& l, const shared_ptr<U>& r)
{
    return (l.get() > r.get());
}


// static cast of shared_ptr
template<class T, class U>
shared_ptr<T> static_pointer_cast(const shared_ptr<U>& ptr)
{
    return shared_ptr<T>(ptr, static_cast<typename shared_ptr<T>::element_type*>(ptr.get()));
}

// dynamic cast of shared_ptr
template<class T, class U>
shared_ptr<T> dynamic_pointer_cast(const shared_ptr<U>& ptr)
{
    T* p = dynamic_cast<typename shared_ptr<T>::element_type*>(ptr.get());
    if (NULL != p)
    {
        return shared_ptr<T>(ptr, p);
    }
    else
    {
        return shared_ptr<T>();
    }
}

// make shared object
template<typename T, typename... Args>
inline shared_ptr<T> make_shared(Args&&... args)
{
   return shared_ptr<T>(new T(args...));
}

#endif
