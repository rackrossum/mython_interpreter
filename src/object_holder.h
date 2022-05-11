#pragma once

#include <memory>
#include <stdexcept>
#include <typeinfo>
#include <unordered_map>

#include "Iobject.h"

class TestRunner;

namespace Runtime {

struct TypeError : std::runtime_error
{
    TypeError()
        : std::runtime_error("Wrong type")
    {}

    TypeError(const std::type_info& expected)
        : std::runtime_error(std::string("Wrong type. Expected: ") + expected.name())
    {}
};

class IObject;

class ObjectHolder {
public:
  ObjectHolder() = default;

  template <typename T>
  static ObjectHolder Own(T&& object) {
    return ObjectHolder(
      std::make_shared<T>(std::forward<T>(object))
    );
  }

  static ObjectHolder Share(IObject& object);
  static ObjectHolder None();

  IObject& operator*();
  const IObject& operator*() const;
  IObject* operator->();
  const IObject* operator->() const;

  IObject* Get();
  const IObject* Get() const;

  template <typename T>
  T* TryAs() {
    return dynamic_cast<T*>(this->Get());
  }

  template <typename T>
  const T* TryAs() const {
    return dynamic_cast<const T*>(this->Get());
  }

  template <typename T>
  const T* GetAs() const
  {
      auto res = TryAs<T>();
      if (res == nullptr)
          throw TypeError(typeid(T));
      return res;
  }

  template <typename T>
  T* GetAs()
  {
     return const_cast<T*>(static_cast<const ObjectHolder*>(this)->TryAs<T>());
  }

  typename IObject::Type GetType() const;
  bool IsSameType(const ObjectHolder& other) const;

  explicit operator bool() const;

private:
  ObjectHolder(std::shared_ptr<IObject> data) : data(std::move(data)) {
  }

  std::shared_ptr<IObject> data;
};

using Closure = std::unordered_map<std::string, ObjectHolder>;


void RunObjectHolderTests(TestRunner& tr);


} /* namespace Runtime */

using ObjectHolder = Runtime::ObjectHolder;
