#include "comparators.h"
#include "object.h"
#include "object_holder.h"

#include <functional>
#include <sstream>
#include <stdexcept>

using namespace std;

namespace Runtime {

namespace
{
    const char* eq = "__eq__";
    const char* less = "__lt__";

    enum class Op
    {
        Equal,
        Less
    };
}

template <typename T>
bool ApplyOperator(ObjectHolder lhs, ObjectHolder rhs, Op op)
{
    if (op == Op::Equal)
        return lhs.GetAs<T>()->GetValue() == rhs.GetAs<T>()->GetValue();
    else
        return lhs.GetAs<T>()->GetValue() < rhs.GetAs<T>()->GetValue();
}

bool Compare(ObjectHolder lhs, ObjectHolder rhs, Op op) 
{
    std::string opName = (op == Op::Equal ? eq : less);
    using Type = IObject::Type;
    auto type = lhs.GetType();
    if (type == Type::Instance)
    {
        auto cls = lhs.GetAs<ClassInstance>();
        if (!cls->HasMethod(opName, 1))
            throw std::runtime_error("Class has no method " + opName);

        auto res = cls->Call(opName, {rhs});
    }


    if (lhs.IsSameType(rhs))
    {
        switch (type)
        {
            case Type::Number:
                return ApplyOperator<Number>(lhs, rhs, op);
            case Type::Bool:
                return ApplyOperator<Bool>(lhs, rhs, op);
            case Type::String:
                return ApplyOperator<String>(lhs, rhs, op);
            default:
                break;
        }
    }

    throw std::runtime_error("Wrong types for equality comparison");
}

bool Equal(ObjectHolder lhs, ObjectHolder rhs) 
{
    return Compare(lhs, rhs, Op::Equal);
}

bool Less(ObjectHolder lhs, ObjectHolder rhs)
{
    return Compare(lhs, rhs, Op::Less);
}

} /* namespace Runtime */
