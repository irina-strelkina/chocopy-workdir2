#ifndef CHOCOPY_LLVM_AST_TYPE_H
#define CHOCOPY_LLVM_AST_TYPE_H

#include "chocopy-llvm/Basic/LLVM.h"

#include <llvm/ADT/StringRef.h>

namespace chocopy {
class ASTContext;
class ValueType;

using ValueTypeList = SmallVector<ValueType *>;

class alignas(void *) Type {
public:
  enum class TypeKind {
    Func,
    Value,
  };

public:
  TypeKind getTypeKind() const { return TKind; }

protected:
  Type(TypeKind TKind) : TKind(TKind) {}

private:
  TypeKind TKind;
};

class FuncType final : public Type {
  friend ASTContext;

public:
  const ValueTypeList &getParametersTypes() const { return ParametersTy; }
  const ValueType *getReturnType() const { return RetTy; }

public:
  static bool classof(const Type *T) {
    return T->getTypeKind() == TypeKind::Func;
  }

private:
  FuncType(ValueTypeList ParametersTy, ValueType *RetTy)
      : Type(TypeKind::Func), ParametersTy(std::move(ParametersTy)),
        RetTy(RetTy) {}

private:
  ValueTypeList ParametersTy;
  ValueType *RetTy;
};

class ValueType : public Type {
public:
  enum class ValueKind {
    Class,
    List,
  };

public:
  static bool classof(const Type *T) {
    return T->getTypeKind() == TypeKind::Value;
  }

public:
  ValueKind getValueKind() const { return VKind; }

  bool isInt() const;
  bool isStr() const;
  bool isBool() const;
  bool isNone() const;

protected:
  ValueType(ASTContext &Ctx, ValueKind VKind)
      : Type(TypeKind::Value), VKind(VKind), Ctx(Ctx) {}

  friend bool operator<=(const ValueType &Sub, const ValueType &Sup);

private:
  ValueKind VKind;
  ASTContext &Ctx;
};

class ClassValueType final : public ValueType {
  friend ASTContext;

public:
  StringRef getClassName() const { return ClassName; }

public:
  static bool classof(const Type *T) {
    if (const ValueType *VT = dyn_cast<ValueType>(T))
      return classof(VT);
    return false;
  }

  static bool classof(const ValueType *T) {
    return T->getValueKind() == ValueKind::Class;
  }

protected:
  ClassValueType(ASTContext &Ctx, StringRef ClassName)
      : ValueType(Ctx, ValueKind::Class), ClassName(ClassName) {}

private:
  StringRef ClassName;
};

class ListValueType final : public ValueType {
  friend ASTContext;

public:
  ValueType *getElementType() const { return ElementType; }

public:
  static bool classof(const Type *T) {
    if (const ValueType *VT = dyn_cast<ValueType>(T))
      return classof(VT);
    return false;
  }

  static bool classof(const ValueType *T) {
    return T->getValueKind() == ValueKind::List;
  }

protected:
  ListValueType(ASTContext &Ctx, ValueType *ElementType)
      : ValueType(Ctx, ValueKind::List), ElementType(ElementType) {}

private:
  ValueType *ElementType;
};
} // namespace chocopy
#endif // CHOCOPY_LLVM_AST_TYPE_H
