// This file was GENERATED by command:
//     pump.py callback_registry.h.pump
// DO NOT EDIT BY HAND!!!


// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_CALLBACK_REGISTRY_H_
#define BASE_CALLBACK_REGISTRY_H_

#include <list>

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/callback_internal.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"

// OVERVIEW:
//
// A container for a list of callbacks.  Unlike a normal STL vector or list,
// this container can be modified during iteration without invalidating the
// iterator. It safely handles the case of a callback removing itself
// or another callback from the list while callbacks are being run.
//
// TYPICAL USAGE:
//
// class MyWidget {
//  public:
//   ...
//
//   typedef base::Callback<void(const Foo&)> OnFooCallback;
//
//   scoped_ptr<base::CallbackRegistry<void(const Foo&)>::Subscription>
//   RegisterCallback(const OnFooCallback& cb) {
//     return callback_registry_.Add(cb);
//   }
//
//  private:
//   void NotifyFoo(const Foo& foo) {
//      callback_registry_.Notify(foo);
//   }
//
//   base::CallbackRegistry<void(const Foo&)> callback_registry_;
// };
//
//
// class MyWidgetListener {
//  public:
//   MyWidgetListener::MyWidgetListener() {
//     foo_subscription_ = MyWidget::GetCurrent()->RegisterCallback(
//             base::Bind(&MyWidgetListener::OnFoo, this)));
//   }
//
//   MyWidgetListener::~MyWidgetListener() {
//      // Subscription gets deleted automatically and will deregister
//      // the callback in the process.
//   }
//
//  private:
//   void OnFoo(const Foo& foo) {
//     // Do something.
//   }
//
//   scoped_ptr<base::CallbackRegistry<void(const Foo&)>::Subscription>
//       foo_subscription_;
// };

namespace base {

namespace internal {

template <typename CallbackType>
class CallbackRegistryBase {
 public:
  class Subscription {
   public:
    Subscription(CallbackRegistryBase<CallbackType>* list,
                 typename std::list<CallbackType>::iterator iter)
        : list_(list),
          iter_(iter) {}

    ~Subscription() {
      if (list_->active_iterator_count_)
        (*iter_).Reset();
      else
        list_->callbacks_.erase(iter_);
    }

   private:
    CallbackRegistryBase<CallbackType>* list_;
    typename std::list<CallbackType>::iterator iter_;

    DISALLOW_COPY_AND_ASSIGN(Subscription);
  };

  // Add a callback to the list. The callback will remain registered until the
  // returned Subscription is destroyed, which must occur before the
  // CallbackRegistry is destroyed.
  scoped_ptr<Subscription> Add(const CallbackType& cb) {
    DCHECK(!cb.is_null());
    return scoped_ptr<Subscription>(
        new Subscription(this, callbacks_.insert(callbacks_.end(), cb)));
  }

 protected:
  // An iterator class that can be used to access the list of callbacks.
  class Iterator {
   public:
    explicit Iterator(CallbackRegistryBase<CallbackType>* list)
        : list_(list),
          list_iter_(list_->callbacks_.begin()) {
      ++list_->active_iterator_count_;
    }

    Iterator(const Iterator& iter)
        : list_(iter.list_),
          list_iter_(iter.list_iter_) {
      ++list_->active_iterator_count_;
    }

    ~Iterator() {
      if (list_ && --list_->active_iterator_count_ == 0) {
        list_->Compact();
      }
    }

    CallbackType* GetNext() {
      while ((list_iter_ != list_->callbacks_.end()) && list_iter_->is_null())
        ++list_iter_;

      CallbackType* cb = NULL;
      if (list_iter_ != list_->callbacks_.end()) {
        cb = &(*list_iter_);
        ++list_iter_;
      }
      return cb;
    }

   private:
    CallbackRegistryBase<CallbackType>* list_;
    typename std::list<CallbackType>::iterator list_iter_;
  };

  CallbackRegistryBase()
      : active_iterator_count_(0) {}

  ~CallbackRegistryBase() {
    DCHECK_EQ(0, active_iterator_count_);
    DCHECK_EQ(0U, callbacks_.size());
  }

  // Returns an instance of a CallbackRegistryBase::Iterator which can be used
  // to run callbacks.
  Iterator GetIterator() {
    return Iterator(this);
  }

  // Compact the list: remove any entries which were NULLed out during
  // iteration.
  void Compact() {
    typename std::list<CallbackType>::iterator it = callbacks_.begin();
    while (it != callbacks_.end()) {
      if ((*it).is_null())
        it = callbacks_.erase(it);
      else
        ++it;
    }
  }

 private:
  std::list<CallbackType> callbacks_;
  int active_iterator_count_;

  DISALLOW_COPY_AND_ASSIGN(CallbackRegistryBase);
};

}  // namespace internal

template <typename Sig> class CallbackRegistry;

template <>
class CallbackRegistry<void(void)>
    : public internal::CallbackRegistryBase<Callback<void(void)> > {
 public:
  typedef Callback<void(void)> CallbackType;

  CallbackRegistry() {}

  void Notify() {
    internal::CallbackRegistryBase<CallbackType>::Iterator it =
        this->GetIterator();
    CallbackType* cb;
    while((cb = it.GetNext()) != NULL) {
      cb->Run();
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CallbackRegistry);
};

template <typename A1>
class CallbackRegistry<void(A1)>
    : public internal::CallbackRegistryBase<
          Callback<void(A1)> > {
 public:
  typedef Callback<void(A1)> CallbackType;

  CallbackRegistry() {}

  void Notify(typename internal::CallbackParamTraits<A1>::ForwardType a1) {
    typename internal::CallbackRegistryBase<CallbackType>::Iterator it =
        this->GetIterator();
    CallbackType* cb;
    while((cb = it.GetNext()) != NULL) {
      cb->Run(a1);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CallbackRegistry);
};

template <typename A1, typename A2>
class CallbackRegistry<void(A1, A2)>
    : public internal::CallbackRegistryBase<
          Callback<void(A1, A2)> > {
 public:
  typedef Callback<void(A1, A2)> CallbackType;

  CallbackRegistry() {}

  void Notify(typename internal::CallbackParamTraits<A1>::ForwardType a1,
              typename internal::CallbackParamTraits<A2>::ForwardType a2) {
    typename internal::CallbackRegistryBase<CallbackType>::Iterator it =
        this->GetIterator();
    CallbackType* cb;
    while((cb = it.GetNext()) != NULL) {
      cb->Run(a1, a2);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CallbackRegistry);
};

template <typename A1, typename A2, typename A3>
class CallbackRegistry<void(A1, A2, A3)>
    : public internal::CallbackRegistryBase<
          Callback<void(A1, A2, A3)> > {
 public:
  typedef Callback<void(A1, A2, A3)> CallbackType;

  CallbackRegistry() {}

  void Notify(typename internal::CallbackParamTraits<A1>::ForwardType a1,
              typename internal::CallbackParamTraits<A2>::ForwardType a2,
              typename internal::CallbackParamTraits<A3>::ForwardType a3) {
    typename internal::CallbackRegistryBase<CallbackType>::Iterator it =
        this->GetIterator();
    CallbackType* cb;
    while((cb = it.GetNext()) != NULL) {
      cb->Run(a1, a2, a3);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CallbackRegistry);
};

template <typename A1, typename A2, typename A3, typename A4>
class CallbackRegistry<void(A1, A2, A3, A4)>
    : public internal::CallbackRegistryBase<
          Callback<void(A1, A2, A3, A4)> > {
 public:
  typedef Callback<void(A1, A2, A3, A4)> CallbackType;

  CallbackRegistry() {}

  void Notify(typename internal::CallbackParamTraits<A1>::ForwardType a1,
              typename internal::CallbackParamTraits<A2>::ForwardType a2,
              typename internal::CallbackParamTraits<A3>::ForwardType a3,
              typename internal::CallbackParamTraits<A4>::ForwardType a4) {
    typename internal::CallbackRegistryBase<CallbackType>::Iterator it =
        this->GetIterator();
    CallbackType* cb;
    while((cb = it.GetNext()) != NULL) {
      cb->Run(a1, a2, a3, a4);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CallbackRegistry);
};

template <typename A1, typename A2, typename A3, typename A4, typename A5>
class CallbackRegistry<void(A1, A2, A3, A4, A5)>
    : public internal::CallbackRegistryBase<
          Callback<void(A1, A2, A3, A4, A5)> > {
 public:
  typedef Callback<void(A1, A2, A3, A4, A5)> CallbackType;

  CallbackRegistry() {}

  void Notify(typename internal::CallbackParamTraits<A1>::ForwardType a1,
              typename internal::CallbackParamTraits<A2>::ForwardType a2,
              typename internal::CallbackParamTraits<A3>::ForwardType a3,
              typename internal::CallbackParamTraits<A4>::ForwardType a4,
              typename internal::CallbackParamTraits<A5>::ForwardType a5) {
    typename internal::CallbackRegistryBase<CallbackType>::Iterator it =
        this->GetIterator();
    CallbackType* cb;
    while((cb = it.GetNext()) != NULL) {
      cb->Run(a1, a2, a3, a4, a5);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CallbackRegistry);
};

template <typename A1, typename A2, typename A3, typename A4, typename A5,
    typename A6>
class CallbackRegistry<void(A1, A2, A3, A4, A5, A6)>
    : public internal::CallbackRegistryBase<
          Callback<void(A1, A2, A3, A4, A5, A6)> > {
 public:
  typedef Callback<void(A1, A2, A3, A4, A5, A6)> CallbackType;

  CallbackRegistry() {}

  void Notify(typename internal::CallbackParamTraits<A1>::ForwardType a1,
              typename internal::CallbackParamTraits<A2>::ForwardType a2,
              typename internal::CallbackParamTraits<A3>::ForwardType a3,
              typename internal::CallbackParamTraits<A4>::ForwardType a4,
              typename internal::CallbackParamTraits<A5>::ForwardType a5,
              typename internal::CallbackParamTraits<A6>::ForwardType a6) {
    typename internal::CallbackRegistryBase<CallbackType>::Iterator it =
        this->GetIterator();
    CallbackType* cb;
    while((cb = it.GetNext()) != NULL) {
      cb->Run(a1, a2, a3, a4, a5, a6);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CallbackRegistry);
};

template <typename A1, typename A2, typename A3, typename A4, typename A5,
    typename A6, typename A7>
class CallbackRegistry<void(A1, A2, A3, A4, A5, A6, A7)>
    : public internal::CallbackRegistryBase<
          Callback<void(A1, A2, A3, A4, A5, A6, A7)> > {
 public:
  typedef Callback<void(A1, A2, A3, A4, A5, A6, A7)> CallbackType;

  CallbackRegistry() {}

  void Notify(typename internal::CallbackParamTraits<A1>::ForwardType a1,
              typename internal::CallbackParamTraits<A2>::ForwardType a2,
              typename internal::CallbackParamTraits<A3>::ForwardType a3,
              typename internal::CallbackParamTraits<A4>::ForwardType a4,
              typename internal::CallbackParamTraits<A5>::ForwardType a5,
              typename internal::CallbackParamTraits<A6>::ForwardType a6,
              typename internal::CallbackParamTraits<A7>::ForwardType a7) {
    typename internal::CallbackRegistryBase<CallbackType>::Iterator it =
        this->GetIterator();
    CallbackType* cb;
    while((cb = it.GetNext()) != NULL) {
      cb->Run(a1, a2, a3, a4, a5, a6, a7);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CallbackRegistry);
};

}  // namespace base

#endif  // BASE_CALLBACK_REGISTRY_H
