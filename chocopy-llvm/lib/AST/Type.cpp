#include "chocopy-llvm/AST/Type.h"
#include "chocopy-llvm/AST/ASTContext.h"

namespace chocopy {
bool ValueType::isInt() const { return this == Ctx.getIntTy(); }

bool ValueType::isStr() const { return this == Ctx.getStrTy(); }
bool ValueType::isBool() const { return this == Ctx.getBoolTy(); }

bool ValueType::isNone() const { return this == Ctx.getNoneTy(); }

bool operator<=(const ValueType &Sub, const ValueType &Sup) {
  return Sub.Ctx.isAssignementCompatibility(&Sub, &Sup);
}
} // namespace chocopy
