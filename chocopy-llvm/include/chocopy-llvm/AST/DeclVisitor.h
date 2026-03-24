#ifndef CHOCOPY_LLVM_AST_DECLVISITOR_H
#define CHOCOPY_LLVM_AST_DECLVISITOR_H

#include "chocopy-llvm/AST/AST.h"

#include <llvm/ADT/STLExtras.h>

namespace chocopy {
namespace declvisitor {

template <template <typename> class Ptr, typename ImplClass,
          typename RetTy = void>
class Base {
public:
#define PTR(CLASS) typename Ptr<CLASS>::type
#define DISPATCH(NAME, CLASS)                                                  \
  return static_cast<ImplClass *>(this)->visit##NAME(static_cast<PTR(CLASS)>(D))

  RetTy visit(PTR(Declaration) D) {
    switch (D->getKind()) {
#define DECL(CLASS, KIND)                                                      \
  case Declaration::DeclKind::KIND:                                            \
    DISPATCH(CLASS, CLASS);
#include "chocopy-llvm/AST/DeclarationNodes.def"
    }

    llvm_unreachable("Unsupported declaration!");
    return RetTy();
  }

  // Default implementation
  // clang-format off
  RetTy visitClassDef(PTR(ClassDef) D) {                DISPATCH(Declaration, Declaration); }
  RetTy visitFuncDef(PTR(FuncDef) D) {                  DISPATCH(Declaration, Declaration); }
  RetTy visitGlobalDecl(PTR(GlobalDecl) D) {            DISPATCH(Declaration, Declaration); }
  RetTy visitNonLocalDecl(PTR(NonLocalDecl) D) {        DISPATCH(Declaration, Declaration); }
  RetTy visitVarDef(PTR(VarDef) D) {                    DISPATCH(Declaration, Declaration); }
  RetTy visitParamDecl(PTR(ParamDecl) D) {              DISPATCH(Declaration, Declaration); }
  RetTy visitDeclaration(PTR(Declaration) D) {          return RetTy(); }
  // clang-format on

#undef PTR
#undef DISPATCH
};
} // namespace declvisitor

template <typename Impl>
class DeclVisitor : public declvisitor::Base<std::add_pointer, Impl> {};

template <typename Impl>
class ConstDeclVisitor : public declvisitor::Base<llvm::make_const_ptr, Impl> {
};
} // namespace chocopy

#endif // CHOCOPY_LLVM_AST_DECLVISITOR_H
