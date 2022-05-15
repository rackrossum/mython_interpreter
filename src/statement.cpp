#include "statement.h"
#include "Iobject.h"
#include "object.h"
#include "object_holder.h"

#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <map>
#include <string>

using namespace std;
namespace
{
    const char* initFunc = "__init__";
}

namespace Ast {

using Runtime::Closure;
using pStatement = std::unique_ptr<Statement>;

namespace
{
    const std::string VAR_STR = "VariableValue";
    const std::string FIELD_STR = "FieldAssignment";
}

// Free
//
void Throw(const std::string& scope, const std::string& message)
{
    throw std::runtime_error(scope + ": " + message);
}

std::string Concatenate(const vector<std::string>& v)
{
    std::string res = v[0];
    for (size_t i = 1; i < v.size(); ++i)
        res += "." + v[i];
    return res;
}

std::vector<ObjectHolder> ActualizeArgs(std::vector<pStatement>& args, Closure& closure)
{
    std::vector<ObjectHolder> res;
    res.reserve(args.size());
    for (auto& st : args)
        res.push_back(st->Execute(closure));

    return res;
}


template <typename T>
std::optional<std::pair<T*, T*>> TryAs(ObjectHolder left, ObjectHolder right)
{
    auto pLeft = left.TryAs<T>();
    auto pRight = right.TryAs<T>();
    if (pLeft && pRight)
        return std::pair{pLeft, pRight};

    return std::nullopt;
}


Closure& GetClosure(Closure& outer, const std::vector<std::string>& dotted_ids, const std::string& scope)
{
    auto* clsr = &outer;
    const auto& ids = dotted_ids;
    for (size_t i = 0, end = ids.size() - 1; i < end; ++i)
    {
        auto clsrIt = clsr->find(ids[i]);
        if (clsrIt == clsr->end()) 
            Throw(scope, "\"" + clsrIt->first + "\" wasnt found in closure. Ids: " + Concatenate(ids));
        
        if (clsrIt->second->GetType() != Runtime::IObject::Type::Instance)
            Throw(scope, "\"" + clsrIt->first + "\" isnt class Instance. Ids: " + Concatenate(ids));

        clsr = &(clsrIt->second.GetAs<Runtime::ClassInstance>()->Fields());
    }

    return *clsr;
}


// VariableValue
//

VariableValue::VariableValue(std::string var_name)
    :dotted_ids({std::move(var_name)})
{
    if (this->dotted_ids.empty())
        Throw(VAR_STR, "dotted_ids are empty");
}

VariableValue::VariableValue(std::vector<std::string> dotted_ids)
: dotted_ids(std::move(dotted_ids))
{
    if (this->dotted_ids.empty())
        Throw(VAR_STR, "dotted_ids are empty");
}


Result VariableValue::Execute(Closure& closure)
{
    using ObjType = decltype(ObjectHolder().GetType());

    auto& inner = GetClosure(closure, dotted_ids, VAR_STR);

    auto it = inner.find(dotted_ids.back());
    if (it == inner.end())
        Throw(VAR_STR, Concatenate(dotted_ids) + " cant be found");

    auto res = it->second;
    if (!res)
        return ObjectHolder::Own(Runtime::None());
    
    return res;
}

// Assignment
//
Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv) 
    : var(var), rv(std::move(rv))
{
    if (var.empty())
        throw std::runtime_error("Assignment: var name is empty");
}

Result Assignment::Execute(Closure& closure) 
{
    auto &obj = closure[var];
    obj = rv->Execute(closure);
    return obj;
}

// FieldAssignment
FieldAssignment::FieldAssignment(
  VariableValue object, std::string field_name, std::unique_ptr<Statement> rv
)
  : object(std::move(object))
  , field_name(std::move(field_name))
  , right_value(std::move(rv))
{
    if (this->field_name.empty())
        Throw(FIELD_STR, "field name is empty");
}

Result FieldAssignment::Execute(Runtime::Closure& closure)
{
    auto pCls = object.Execute(closure);
    if (pCls->GetType() != Runtime::IObject::Type::Instance)
        Throw(FIELD_STR, "");

    auto &res = pCls.GetAs<Runtime::ClassInstance>()->Fields()[field_name];
    res = right_value->Execute(closure);
    return res;
}

// Print
//

Print::Print(unique_ptr<Statement> argument)
{
    args.push_back(std::move(argument));
}

Print::Print(vector<unique_ptr<Statement>> args)
    :args(std::move(args))
{
}

unique_ptr<Print> Print::Variable(std::string var)
{
    return std::make_unique<Print>(std::make_unique<VariableValue>(var));
}

Result Print::Execute(Closure& closure) 
{
    auto it = args.begin();
    (*it++)->Execute(closure).Get()->Print(*output);

    for (auto end = args.end(); it != end; ++it)
    {
        (*output) << " ";
        auto res = (*it)->Execute(closure).Get();
        if (res)
            res->Print(*output);
        else
            Runtime::None{}.Print(*output);
    }
    (*output) << std::endl;

    return ObjectHolder();
}

ostream* Print::output = &cout;

void Print::SetOutputStream(ostream& output_stream) {
  output = &output_stream;
}

// MethodCall
//

MethodCall::MethodCall(
  std::unique_ptr<Statement> object
  , std::string method
  , std::vector<std::unique_ptr<Statement>> args
)
    :object(std::move(object)), method(std::move(method)), args(std::move(args))
{
}

Result MethodCall::Execute(Closure& closure)
{
    auto obj = object->Execute(closure);
    return obj.GetAs<Runtime::ClassInstance>()->Call(method, std::move(ActualizeArgs(args, closure)));
}

// NewInstance
//

NewInstance::NewInstance(
  const Runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args
)
  : class_(class_)
  , args(std::move(args))
{
}

NewInstance::NewInstance(const Runtime::Class& class_) 
    : NewInstance(class_, {}) 
{
}

Result NewInstance::Execute(Runtime::Closure& closure) 
{
    auto cls = Runtime::ClassInstance(class_);
    auto actualArgs = ActualizeArgs(args, closure);
    if (cls.HasMethod(initFunc, actualArgs.size()))
        cls.Call(initFunc, actualArgs);

    return ObjectHolder::Own(std::move(cls));
}

// Stringify
//

Result Stringify::Execute(Closure& closure)
{
    if (!argument)
        throw std::runtime_error("Stringify: no argument");

    std::ostringstream os;
    argument->Execute(closure)->Print(os);
    
    return ObjectHolder::Own(Runtime::String(os.str()));
}

// BinaryOps
//
namespace {
    enum class Op
    {
        Add,
        Sub,
        Mult,
        Div,
        And,
        Or
    };

    std::map<Op, std::string> opToStr = {
        {Op::Add, "__add__"},
        {Op::Sub, "__sub__"},
        {Op::Mult, "__mult__"},
        {Op::Div, "__div__"},
        {Op::And, "__and__"},
        {Op::Or, "__or__"}
    };
}

ObjectHolder CallOperatorNums(std::pair<Runtime::Number*, Runtime::Number*> p, Op op)
{
    auto* left = p.first;
    auto* right = p.second;
    int val;

    const auto& leftV = left->GetValue(), rightV = right->GetValue();
    switch (op)
    {
        case Op::Add:
            val = leftV + rightV;
            break;
        case Op::Sub:
            val = leftV - rightV;
            break;
        case Op::Mult:
            val = leftV * rightV;
            break;
        case Op::Div:
            val = leftV / rightV;
            break;
        default:
            throw std::runtime_error("Wrong operator for numbers");
    }

    return ObjectHolder::Own(Runtime::Number(val));
}

ObjectHolder CallOperatorBool(std::pair<Runtime::Bool*, Runtime::Bool*> p, Op op)
{
    auto* left = p.first;
    auto* right = p.second;
    bool val;
    
    const auto& leftV = left->GetValue(), rightV = right->GetValue();
    switch (op)
    {
        case Op::And:
            val = leftV && rightV;
            break;
        case Op::Or:
            val = leftV || rightV;
            break;
        default:
            throw std::runtime_error("Wrong operator for bools");
    }

    return ObjectHolder::Own(Runtime::Bool(val));
}


std::optional<ObjectHolder> CallOperatorCls(ObjectHolder left, ObjectHolder right, Op op)
{
    auto cls = left.GetAs<Runtime::ClassInstance>();
    const std::string& method = opToStr.at(op);
    if (!cls->HasMethod(method, 1))
        return nullopt;

    return cls->Call(method, {right});
}

ObjectHolder CallOperator(ObjectHolder left, ObjectHolder right, Op op)
{
    if (op == Op::And || op == Op::Or)
    {
        auto bools = TryAs<Runtime::Bool>(left, right);
        if (bools)
            return CallOperatorBool(*bools, op);
    }
    else
    {
        if (op == Op::Add)
        {
            auto strs = TryAs<Runtime::String>(left, right);
            if (strs)
                return ObjectHolder::Own(Runtime::String(strs->first->GetValue() + strs->second->GetValue()));
        }

        auto nums = TryAs<Runtime::Number>(left, right);
        if (nums)
            return CallOperatorNums(*nums, op);
    }

    auto cls = left.TryAs<Runtime::ClassInstance>();
    if (cls)
    {
        auto res = CallOperatorCls(left, right, op);
        if (res)
            return res.value();
    }

    throw std::runtime_error("No valid types for " + opToStr.at(op) + " operation");
}

Result Add::Execute(Closure& closure) 
{
    auto left = lhs->Execute(closure), right = rhs->Execute(closure);
    return CallOperator(left, right, Op::Add);
}

Result Sub::Execute(Closure& closure) 
{
    auto left = lhs->Execute(closure), right = rhs->Execute(closure);
    return CallOperator(left, right, Op::Sub);
}

Result Mult::Execute(Runtime::Closure& closure) 
{
    auto left = lhs->Execute(closure), right = rhs->Execute(closure);
    return CallOperator(left, right, Op::Mult);
}

Result Div::Execute(Runtime::Closure& closure) 
{
    auto left = lhs->Execute(closure), right = rhs->Execute(closure);
    return CallOperator(left, right, Op::Div);
}

Result Or::Execute(Runtime::Closure& closure) 
{
    auto left = lhs->Execute(closure), right = rhs->Execute(closure);
    return CallOperator(left, right, Op::Or);
}

Result And::Execute(Runtime::Closure& closure) 
{
    auto left = lhs->Execute(closure), right = rhs->Execute(closure);
    return CallOperator(left, right, Op::And);
}

// Compound

Result Compound::Execute(Closure& closure) 
{
    Result res;
    for (auto &st : statements)
    {
        res = st->Execute(closure);
        if (res.IsNeedToReturn())
            return res;
    }

    return Result();
}

// Return
Result Return::Execute(Closure& closure)
{
    Result res(statement->Execute(closure));
    res.SetNeedToReturn();
    return res;
}

ClassDefinition::ClassDefinition(ObjectHolder class_)
    : cls(std::move(class_)), class_name(cls.GetAs<Runtime::Class>()->GetName())
{
}

Result ClassDefinition::Execute(Runtime::Closure& closure) {
    return cls;
}

// IfElse
//
IfElse::IfElse(
  std::unique_ptr<Statement> condition,
  std::unique_ptr<Statement> if_body,
  std::unique_ptr<Statement> else_body
)
    :condition(std::move(condition)), if_body(std::move(if_body)), else_body(std::move(else_body))
{
}

Result IfElse::Execute(Runtime::Closure& closure) {
    if (!condition)
        throw std::runtime_error("No condition in IfElseBlock");

    if (!if_body)
        throw std::runtime_error("No If Body in IfElseBlock");

    Result res;
    if (condition->Execute(closure).GetAs<Runtime::Bool>()->GetValue() == true)
        res = if_body->Execute(closure);
    else if (else_body)
        res = else_body->Execute(closure);

    return res;
}

// Not
//

Result Not::Execute(Runtime::Closure& closure) {

    ObjectHolder res;
    auto obj = argument->Execute(closure);
    if (obj.TryAs<Runtime::Bool>() != nullptr)
        return ObjectHolder::Own(Runtime::Bool(!obj.GetAs<Runtime::Bool>()->GetValue()));

    const char* notName = "__not__";

    auto cls = obj.GetAs<Runtime::ClassInstance>();
    if (!cls->HasMethod(notName, 0))
        throw std::runtime_error("Not: cls has no such method");

    return cls->Call(notName, {});
}

// Comparison
//
Comparison::Comparison(
  Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs
) 
    : comparator(std::move(cmp)), left(std::move(lhs)), right(std::move(rhs))
{
    if (!left || !right)
        throw std::runtime_error("Comparison: arguments are missed");
}

Result Comparison::Execute(Runtime::Closure& closure) {
    return ObjectHolder::Own(Runtime::Bool(comparator(left->Execute(closure), right->Execute(closure))));
}

} /* namespace Ast */
