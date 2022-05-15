#pragma once

#include "object_holder.h"
#include "object.h"

#include <stdexcept>
#include <unordered_map>
#include <string>
#include <functional>
#include <memory>
#include <vector>

class TestRunner;

namespace Ast {

struct Result : public ObjectHolder
{
    Result() = default;
    Result(const ObjectHolder& oh)
        : ObjectHolder(oh)
    {}
    Result(ObjectHolder&& oh)
        : ObjectHolder(std::move(oh))
    {}
    bool IsNeedToReturn()
    {
        return needToReturn;
    }

    void SetNeedToReturn()
    {
        needToReturn = true;
    }

private:
    bool needToReturn = false;
};

struct Statement {
  virtual ~Statement() = default;
  virtual Result Execute(Runtime::Closure& closure) = 0;

  template <typename T>
  T* TryAs()
  {
      return dynamic_cast<T>(this);
  }
};

template <typename T>
struct ValueStatement : Statement {
  T value;

  explicit ValueStatement(T v) : value(std::move(v)) {
  }

  Result Execute(Runtime::Closure&) override {
    return ObjectHolder::Share(value);
  }
};

using NumericConst = ValueStatement<Runtime::Number>;
using StringConst = ValueStatement<Runtime::String>;
using BoolConst = ValueStatement<Runtime::Bool>;

struct VariableValue : Statement {
  std::vector<std::string> dotted_ids;

  explicit VariableValue(std::string var_name);
  explicit VariableValue(std::vector<std::string> dotted_ids);
  Result Execute(Runtime::Closure& closure) override;
};

struct Assignment : Statement {
  std::string var;
  std::unique_ptr<Statement> rv;

  Assignment(std::string var, std::unique_ptr<Statement> rv);
  Result Execute(Runtime::Closure& closure) override;
};

struct FieldAssignment : Statement {
  VariableValue object;
  std::string field_name;
  std::unique_ptr<Statement> right_value;

  FieldAssignment(VariableValue object, std::string field_name, std::unique_ptr<Statement> rv);
  Result Execute(Runtime::Closure& closure) override;
};

struct None : Statement {
  Result Execute(Runtime::Closure&) override {
    return ObjectHolder();
  }
};

class Print : public Statement {
public:
  explicit Print(std::unique_ptr<Statement> argument);
  explicit Print(std::vector<std::unique_ptr<Statement>> args);

  static std::unique_ptr<Print> Variable(std::string name);

  Result Execute(Runtime::Closure& closure) override;

  static void SetOutputStream(std::ostream& output_stream);

private:
  std::vector<std::unique_ptr<Statement>> args;
  static std::ostream* output;
};

struct MethodCall : Statement {
  std::unique_ptr<Statement> object;
  std::string method;
  std::vector<std::unique_ptr<Statement>> args;

  MethodCall(
    std::unique_ptr<Statement> object,
    std::string method,
    std::vector<std::unique_ptr<Statement>> args
  );

  Result Execute(Runtime::Closure& closure) override;
};

struct NewInstance : Statement {
  const Runtime::Class& class_;
  std::vector<std::unique_ptr<Statement>> args;

  NewInstance(const Runtime::Class& class_);
  NewInstance(const Runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args);
  Result Execute(Runtime::Closure& closure) override;
};

class UnaryOperation : public Statement {
public:
  UnaryOperation(std::unique_ptr<Statement> argument) : argument(std::move(argument)) {
      if (!this->argument)
          throw std::runtime_error("UnaryOperation: arg is missed");
  }

protected:
  std::unique_ptr<Statement> argument;
};

class Stringify : public UnaryOperation {
public:
  using UnaryOperation::UnaryOperation;
  Result Execute(Runtime::Closure& closure) override;
};

class BinaryOperation : public Statement {
public:
  BinaryOperation(std::unique_ptr<Statement> lhs, std::unique_ptr<Statement> rhs)
    : lhs(std::move(lhs))
    , rhs(std::move(rhs))
  {
      if (!this->lhs || !this->rhs)
          throw std::runtime_error("BinaryOperation: args are missed");
  }

protected:
  std::unique_ptr<Statement> lhs, rhs;
};

class Add : public BinaryOperation {
public:
  using BinaryOperation::BinaryOperation;
  Result Execute(Runtime::Closure& closure) override;
};

class Sub : public BinaryOperation {
public:
  using BinaryOperation::BinaryOperation;
  Result Execute(Runtime::Closure& closure) override;
};

class Mult : public BinaryOperation {
public:
  using BinaryOperation::BinaryOperation;
  Result Execute(Runtime::Closure& closure) override;
};

class Div : public BinaryOperation {
public:
  using BinaryOperation::BinaryOperation;
  Result Execute(Runtime::Closure& closure) override;
};

class Or : public BinaryOperation {
public:
  using BinaryOperation::BinaryOperation;
  Result Execute(Runtime::Closure& closure) override;
};

class And : public BinaryOperation {
public:
  using BinaryOperation::BinaryOperation;
  Result Execute(Runtime::Closure& closure) override;
};

class Not : public UnaryOperation {
public:
  using UnaryOperation::UnaryOperation;
  Result Execute(Runtime::Closure& closure) override;
};

class Compound : public Statement {
public:
  template <typename ...Args>
  explicit Compound(Args&& ...args) {
    (statements.push_back(std::forward<Args>(args)), ...);
  }

  void AddStatement(std::unique_ptr<Statement> stmt) {
    statements.push_back(std::move(stmt));
  }

  Result Execute(Runtime::Closure& closure) override;

private:
  std::vector<std::unique_ptr<Statement>> statements;
};

class Return : public Statement {
public:
  explicit Return(std::unique_ptr<Statement> statement)
    : statement(std::move(statement))
  {
  }

  Result Execute(Runtime::Closure& closure) override;

private:
  std::unique_ptr<Statement> statement;
};

class ClassDefinition : public Statement {
public:
  explicit ClassDefinition(ObjectHolder cls);

  Result Execute(Runtime::Closure& closure) override;

private:
  ObjectHolder cls;
  const std::string& class_name;
};

class IfElse : public Statement {
public:
  IfElse(
    std::unique_ptr<Statement> condition,
    std::unique_ptr<Statement> if_body,
    std::unique_ptr<Statement> else_body
  );

  Result Execute(Runtime::Closure& closure) override;

private:
  std::unique_ptr<Statement> condition, if_body, else_body;
};

class Comparison : public Statement {
public:
  using Comparator = std::function<bool(const ObjectHolder&, const ObjectHolder&)>;

  Comparison(
    Comparator cmp,
    std::unique_ptr<Statement> lhs,
    std::unique_ptr<Statement> rhs
  );

  Result Execute(Runtime::Closure& closure) override;

private:
  Comparator comparator;
  std::unique_ptr<Statement> left, right;
};

void RunUnitTests(TestRunner& tr);

}

using Statement = Ast::Statement;

