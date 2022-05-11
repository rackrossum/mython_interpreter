#include "object_holder.h"

namespace Runtime {

ObjectHolder ObjectHolder::Share(IObject& object) {
  return ObjectHolder(std::shared_ptr<IObject>(&object, [](auto*) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None() {
  return ObjectHolder();
}

IObject& ObjectHolder::operator *() {
  return *Get();
}

const IObject& ObjectHolder::operator *() const {
  return *Get();
}

IObject* ObjectHolder::operator ->() {
  return Get();
}

const IObject* ObjectHolder::operator ->() const {
  return Get();
}

IObject* ObjectHolder::Get() {
  return data.get();
}

const IObject* ObjectHolder::Get() const {
  return data.get();
}

ObjectHolder::operator bool() const {
  return Get();
}

typename IObject::Type ObjectHolder::GetType() const
{
    return data->GetType();
}

bool ObjectHolder::IsSameType(const ObjectHolder& other) const
{
    return data->GetType() == other->GetType();
}
}
