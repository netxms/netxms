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
private:
   VolatileCounter useCount;
   VolatileCounter weakCount;
   VolatileCounter spinLock;

public:
   shared_ptr_count()
   {
      useCount = 0;
      weakCount = 0;
      spinLock = 0;
   }

   /// @brief getter of the underlying reference counter
   long use_count(void) const
   {
      return useCount;
   }

   /// @brief acquire/share the ownership of the pointer, initializing the reference counter
   template<class U> void acquire(U *p)
   {
      if (NULL != p)
      {
         InterlockedIncrement(&useCount);
         InterlockedIncrement(&weakCount);
      }
   }

   /// @brief release the ownership of the px pointer, destroying the object when appropriate
   template<class U> void release(U *p)
   {
      if (InterlockedDecrement(&useCount) == 0)
      {
         delete p;
         if (InterlockedDecrement(&weakCount) == 0)
         {
            delete this;
         }
      }
      else
      {
         InterlockedDecrement(&weakCount);
      }
   }

   void weakAcquire()
   {
      InterlockedIncrement(&weakCount);
   }

   bool weakLock()
   {
      while (InterlockedIncrement(&spinLock) != 1)
         InterlockedDecrement(&spinLock);
      bool success;
      if (InterlockedIncrement(&useCount) > 1)
      {
         InterlockedIncrement(&weakCount);
         success = true;
      }
      else
      {
         // Use count 1 after increment indicates no more owning shared pointers
         InterlockedDecrement(&useCount);
         success = false;
      }
      InterlockedDecrement(&spinLock);
      return success;
   }

   void weakRelease()
   {
      if (InterlockedDecrement(&weakCount) == 0)
      {
         delete this;
      }
   }
};

/**
 * @brief minimal implementation of smart pointer, a subset of the C++11 std::shared_ptr or boost::shared_ptr.
 *
 * shared_ptr is a smart pointer retaining ownership of an object through a provided pointer,
 * and sharing this ownership with a reference counter.
 * It destroys the object when the last shared pointer pointing to it is destroyed or reset.
 */
template<class T> class shared_ptr
{
   // This allow pointer_cast functions to share the reference counter between different shared_ptr types
   template<class _Ts> friend class shared_ptr;
   template<class _Tw> friend class weak_ptr;

private:
   T *px; //!< Native pointer
   shared_ptr_count *pn; //!< Reference counter

   // For use by weak_ptr
   shared_ptr(T *p, shared_ptr_count *c)
   {
      pn = c;
      px = p;
   }

public:
   /// The type of the managed object, aliased as member type
   typedef T element_type;

   /// @brief Default constructor
   shared_ptr()
   {
      px = NULL;
      pn = NULL;
   }
   /// @brief Constructor with the provided pointer to manage
   explicit shared_ptr(T *p)
   {
      pn = new shared_ptr_count();
      acquire(p);   // may throw std::bad_alloc
   }
   /// @brief Constructor to share ownership. Warning : to be used for pointer_cast only ! (does not manage two separate <T> and <U> pointers)
   template<class U> shared_ptr(const shared_ptr<U> &ptr, T *p)
   {
      pn = ptr.pn;
      acquire(p);   // may throw std::bad_alloc
   }
   /// @brief Copy constructor to convert from another pointer type
   template<class U> shared_ptr(const shared_ptr<U> &ptr)
   {
      pn = ptr.pn;
      acquire(static_cast<typename shared_ptr<T>::element_type*>(ptr.px));   // will never throw std::bad_alloc
   }
   /// @brief Copy constructor (used by the copy-and-swap idiom)
   shared_ptr(const shared_ptr &ptr)
   {
      pn = ptr.pn;
      acquire(ptr.px);   // will never throw std::bad_alloc
   }
   /// @brief Assignment operator using the copy-and-swap idiom (copy constructor and swap method)
   shared_ptr& operator=(shared_ptr ptr)
   {
      swap(ptr);
      return *this;
   }
   /// @brief the destructor releases its ownership
   ~shared_ptr()
   {
      release();
   }
   /// @brief this reset releases its ownership
   void reset()
   {
      release();
   }
   /// @brief this reset release its ownership and re-acquire another one
   void reset(T *p)
   {
      release();
      acquire(p);
   }

   /// @brief Swap method for the copy-and-swap idiom (copy constructor and swap method)
   void swap(shared_ptr &lhs)
   {
      std::swap(px, lhs.px);
      std::swap(pn, lhs.pn);
   }

   // reference counter operations :
   long use_count() const
   {
      return (pn != NULL) ? pn->use_count() : 0;
   }
   operator bool() const
   {
      return use_count() > 0;
   }
   bool unique() const
   {
      return use_count() == 1;
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
   T* get() const
   {
      return px;
   }

private:
   /// @brief acquire/share the ownership of the px pointer, initializing the reference counter
   void acquire(T *p)
   {
      if (p != NULL)
      {
         if (pn == NULL)
            pn = new shared_ptr_count();
         pn->acquire(p);
      }
      px = p; // here it is safe to acquire the ownership of the provided raw pointer, where exception cannot be thrown any more
   }

   /// @brief release the ownership of the px pointer, destroying the object when appropriate
   void release(void)
   {
      if (pn != NULL)
      {
         pn->release(px);
         pn = NULL;
      }
      px = NULL;
   }
};

template<class T> class weak_ptr
{
private:
   T *px;
   shared_ptr_count *pn;

public:
   weak_ptr()
   {
      px = NULL;
      pn = NULL;
   }

   weak_ptr(const shared_ptr<T> &sptr)
   {
      pn = sptr.pn;
      px = sptr.px;
      if (pn != NULL)
         pn->weakAcquire();
   }

   ~weak_ptr()
   {
      if (pn != NULL)
         pn->weakRelease();
   }

   void
   reset()
   {
      if (pn != NULL)
      {
         pn->weakRelease();
         pn = NULL;
      }
      px = NULL;
   }

   shared_ptr<T>
   lock() const
   {
      if ((pn != NULL) && pn->weakLock())
      {
         return shared_ptr<T>(px, pn);
      }
      return shared_ptr<T>();
   }
};

// comparaison operators
template<class T, class U> bool operator==(const shared_ptr<T> &l, const shared_ptr<U> &r)
{
   return (l.get() == r.get());
}

template<class T> bool operator==(const shared_ptr<T> &l, nullptr_t r)
{
   return l.get() == NULL;
}

template<class T, class U> bool operator!=(const shared_ptr<T> &l, const shared_ptr<U> &r)
{
   return (l.get() != r.get());
}

template<class T> bool operator!=(const shared_ptr<T> &l, nullptr_t r)
{
   return l.get() != NULL;
}

template<class T, class U> bool operator<=(const shared_ptr<T> &l, const shared_ptr<U> &r)
{
   return (l.get() <= r.get());
}

template<class T, class U> bool operator<(const shared_ptr<T> &l, const shared_ptr<U> &r)
{
   return (l.get() < r.get());
}

template<class T, class U> bool operator>=(const shared_ptr<T> &l, const shared_ptr<U> &r)
{
   return (l.get() >= r.get());
}

template<class T, class U> bool operator>(const shared_ptr<T> &l, const shared_ptr<U> &r)
{
   return (l.get() > r.get());
}

// static cast of shared_ptr
template<class T, class U> shared_ptr<T> static_pointer_cast(const shared_ptr<U> &ptr)
{
   return shared_ptr<T>(ptr, static_cast<typename shared_ptr<T>::element_type*>(ptr.get()));
}

// dynamic cast of shared_ptr
template<class T, class U> shared_ptr<T> dynamic_pointer_cast(const shared_ptr<U> &ptr)
{
   T *p = dynamic_cast<typename shared_ptr<T>::element_type*>(ptr.get());
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
template<typename T, typename ... Args> inline shared_ptr<T> make_shared(Args &&... args)
{
   return shared_ptr<T>(new T(args...));
}

#endif
