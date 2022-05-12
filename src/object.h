#pragma once

#include "Iobject.h"
#include "object_holder.h"

#include <ostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace Ast {
  class Statement;
}

class TestRunner;

namespace Runtime {


class Object : public IObject
{
public:
    using typename Runtime::IObject::Type;

    Object (Type type)
        : type(type)
    {}

    Type GetType() const override;

    Type type;
};

template <typename T>
class ValueObject : public Object {
    using typename Runtime::IObject::Type;
protected:
    ValueObject(Type type, const T& value)
        : Object(type), value(value)
    {}

    ValueObject(Type type, T&& value)
        : Object(type), value(std::move(value))
    {}


public:
  void Print(std::ostream& os) override {
    os << value;
  }

  const T& GetValue() const {
    return value;
  }

protected:
  T value;
};

class Number : public ValueObject<int>
{
    using typename Runtime::IObject::Type;
public:
    Number(int n)
        : ValueObject(Type::Number, n)
    {}
};

class Bool : public ValueObject<bool>
{
    using typename Runtime::IObject::Type;
public:
    Bool(bool b)
        : ValueObject(Type::Bool, b)
    {}

    void Print(std::ostream& os) override;
};

class String : public ValueObject<std::string>
{
    using typename Runtime::IObject::Type;
public:
    String(std::string&& str)
        : ValueObject(Type::String, std::move(str))
    {}

    String(const std::string& str)
        : ValueObject(Type::String, str)
    {}
};

struct Method {
  std::string name;
  std::vector<std::string> formal_params;
  std::unique_ptr<Ast::Statement> body;
};

class Class : public Object {
public:
  explicit Class(std::string name, std::vector<Method> methods, const Class* parent = nullptr);
  const Method* GetMethod(const std::string& name) const;
  const std::string& GetName() const;
  void Print(std::ostream& os) override;

private:
  std::string name;
  std::vector<Method> methods;
  const Class* parent;
};

class ClassInstance : public Object {
public:
  explicit ClassInstance(const Class& cls);

  void Print(std::ostream& os) override;

  ObjectHolder Call(const std::string& method, const std::vector<ObjectHolder>& actual_args);
  bool HasMethod(const std::string& method, size_t argument_count) const;

  Closure& Fields();
  const Closure& Fields() const;

private:
  const Class& cls;
  Closure fields;
};

class None : public Object
{
public:
    explicit None();
    void Print(std::ostream& os) override;
};

void RunObjectsTests(TestRunner& test_runner);

}
