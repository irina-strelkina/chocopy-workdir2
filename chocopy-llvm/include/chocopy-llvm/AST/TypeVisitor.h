#ifndef CHOCOPY_LLVM_AST_TYPEVISITOR_H
#define CHOCOPY_LLVM_AST_TYPEVISITOR_H

#include "chocopy-llvm/AST/Type.h"

#include <llvm/ADT/STLExtras.h>

namespace chocopy {
namespace typevisitor {

template <template <typename> class Ptr, typename ImplClass,
          typename RetTy = void>
class Base {
public:
#define PTR(CLASS) typename Ptr<CLASS>::type
#define DISPATCH(NAME, CLASS)                                                  \
  return static_cast<ImplClass *>(this)->visit##NAME(static_cast<PTR(CLASS)>(T))

  // clang-format off
  RetTy visit(PTR(Type) T) {
    switch (T->getTypeKind()) {
#define TYPE(CLASS, KIND)                                                      \
  case Type::TypeKind::KIND:  DISPATCH(CLASS, CLASS);
#include "chocopy-llvm/AST/TypeNodes.def"
    }

    llvm_unreachable("Unsupported type!");
    return RetTy();
  }

  RetTy visit(PTR(ValueType) T) {
    switch (T->getValueKind()) {
#define VALUE_TYPE(CLASS, KIND)                                                      \
  case ValueType::ValueKind::KIND:  DISPATCH(CLASS, CLASS);
#include "chocopy-llvm/AST/TypeNodes.def"
    }

    llvm_unreachable("Unsupported value type!");
    return RetTy();
  }
  // clang-format on

  // clang-format off
  // Default implementation
  RetTy visitFuncType(PTR(FuncType) T) {              DISPATCH(Type, Type); }
  RetTy visitValueType(PTR(ValueType) T) {            visit(T); }
  RetTy visitClassValueType(PTR(ClassValueType) T) {  DISPATCH(Type, ValueType); }
  RetTy visitListValueType(PTR(ListValueType) T) {    DISPATCH(Type, ValueType); }

  RetTy visitType(PTR(Type) ) { return RetTy(); }
  // clang-format on

#undef PTR
#undef DISPATCH
};
} // namespace typevisitor

template <typename Impl>
class TypeVisitor : public typevisitor::Base<std::add_pointer, Impl> {};

template <typename Impl>
class ConstTypeVisitor : public typevisitor::Base<llvm::make_const_ptr, Impl> {
};
} // namespace chocopy

#endif // CHOCOPY_LLVM_AST_TYPEVISITOR_H
