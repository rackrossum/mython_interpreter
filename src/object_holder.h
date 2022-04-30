#pragma once

#include <memory>
#include <stdexcept>
#include <typeinfo>
#include <unordered_map>

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

class Object;

class ObjectHolder {
public:
  ObjectHolder() = default;

  template <typename T>
  static ObjectHolder Own(T&& object) {
    return ObjectHolder(
      std::make_shared<T>(std::forward<T>(object))
    );
  }

  static ObjectHolder Share(Object& object);
  static ObjectHolder None();

  Object& operator*();
  const Object& operator*() const;
  Object* operator->();
  const Object* operator->() const;

  Object* Get();
  const Object* Get() const;

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

  explicit operator bool() const;

private:
  ObjectHolder(std::shared_ptr<Object> data) : data(std::move(data)) {
  }

  std::shared_ptr<Object> data;
};

using Closure = std::unordered_map<std::string, ObjectHolder>;

bool IsTrue(ObjectHolder object);

void RunObjectHolderTests(TestRunner& tr);


} /* namespace Runtime */

using ObjectHolder = Runtime::ObjectHolder;
