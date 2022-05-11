#pragma once

#include <iostream>

namespace Runtime
{
class IObject {
public:
  virtual ~IObject() = default;
  virtual void Print(std::ostream& os) = 0;

  enum class Type
  {
      Number,
      String,
      Bool,
      Class,
      Instance,
      None,
      Unknown
  };

  virtual Type GetType() const;
};
}
