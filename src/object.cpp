#include "object.h"
#include "object_holder.h"
#include "statement.h"

#include <algorithm>
#include <ios>
#include <sstream>
#include <stdexcept>
#include <string_view>

using namespace std;

namespace Runtime {
typename Object::Type Object::GetType() const
{
    return type;
}

void Bool::Print(std::ostream& os)
{
    os << (value ? "True" : "False");
}


// Class
Class::Class(std::string name, std::vector<Method> methods, const Class* parent)
: Object(Type::Class), name(std::move(name)), methods(std::move(methods)), parent(parent)
{
}

const std::string& Class::GetName() const
{
    return name;
}

const Method* Class::GetMethod(const std::string& name) const 
{
    auto it = std::find_if(methods.cbegin(), methods.cend(), [&name](const auto& method){ return method.name == name;});
    if (it == methods.cend())
    {
        if (parent != nullptr)
            return parent->GetMethod(name);

        return nullptr;
    }

    return &*it;
}


void Class::Print(ostream& os)
{
    // ???
    os << this;
}

// ClassInstance
ClassInstance::ClassInstance(const Class& cls)
: Object(Type::Instance), cls(cls)
{
    fields["self"] = ObjectHolder::Share(*this);
}

void ClassInstance::Print(std::ostream& os)
{
    const char* str = "__str__";
    if (HasMethod(str, 0))
        Call(str, {})->Print(os);
    else
        os << this;
}

bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
    auto met = cls.GetMethod(method);
    if (!met || met->formal_params.size() != argument_count)
        return false;

    return true;
}

const Closure& ClassInstance::Fields() const {
    return fields;
}

Closure& ClassInstance::Fields() {
    return fields;
}


ObjectHolder ClassInstance::Call(const std::string& method, const std::vector<ObjectHolder>& actual_args) 
{
    if (!HasMethod(method, actual_args.size()))
        throw std::runtime_error(std::string("ClassInstance ") + cls.GetName() +
                " doesnt have method " + method + "(" + std::to_string(actual_args.size()) + ")");  

    auto met = cls.GetMethod(method);
    auto tempClosure = fields;
    for (size_t i = 0; i < actual_args.size(); ++i)
        tempClosure[met->formal_params[i]] = actual_args[i];

    auto res = met->body->Execute(tempClosure);
    return res;
}

// None
//
None::None()
    :Object(Type::None)
{}

void None::Print(std::ostream& os)
{
    os << "None";
}
} /* namespace Runtime */
