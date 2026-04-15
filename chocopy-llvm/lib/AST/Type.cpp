#include "chocopy-llvm/AST/Type.h"
#include "chocopy-llvm/AST/AST.h"
#include "chocopy-llvm/AST/ASTContext.h"

namespace chocopy {
bool ValueType::isInt() const { return this == Ctx.getIntTy(); }

bool ValueType::isStr() const { return this == Ctx.getStrTy(); }
bool ValueType::isBool() const { return this == Ctx.getBoolTy(); }

bool ValueType::isNone() const { return this == Ctx.getNoneTy(); }

StringRef ClassValueType::getClassName() const { return ClassDecl->getName(); }
ClassDef *ClassValueType::getClassDef() const { return ClassDecl; }
} // namespace chocopy
