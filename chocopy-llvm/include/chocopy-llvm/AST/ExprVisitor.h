#ifndef CHOCOPY_LLVM_AST_EXPRVISITOR_H
#define CHOCOPY_LLVM_AST_EXPRVISITOR_H

#include "chocopy-llvm/AST/AST.h"

#include <llvm/ADT/STLExtras.h>

namespace chocopy {
namespace exprvisitor {

template <template <typename> class Ptr, typename ImplClass,
          typename RetTy = void>
class Base {
public:
#define PTR(CLASS) typename Ptr<CLASS>::type
#define DISPATCH(NAME, CLASS)                                                  \
  return static_cast<ImplClass *>(this)->visit##NAME(static_cast<PTR(CLASS)>(E))

  // clang-format off
  RetTy visit(PTR(Literal) E) {
    switch (E->getLiteralType()) {
#define LITERAL_EXPR(CLASS, KIND)                                              \
  case Literal::LiteralType::KIND:  DISPATCH(CLASS, CLASS);
#include "chocopy-llvm/AST/ExprNodes.def"
    }
  }

  RetTy visit(PTR(Expr) E) {
    switch (E->getKind()) {
#define EXPR(CLASS, KIND)                                                      \
  case Expr::Kind::KIND:  DISPATCH(CLASS, CLASS);
#include "chocopy-llvm/AST/ExprNodes.def"
    }

    llvm_unreachable("Unsupported expression!");
    return RetTy();
  }
  // clang-format on

  // clang-format off
  // Default implementation
  RetTy visitBinaryExpr(PTR(BinaryExpr) E) {          DISPATCH(Expr, Expr); }
  RetTy visitCallExpr(PTR(CallExpr) E) {              DISPATCH(Expr, Expr); }
  RetTy visitDeclRef(PTR(DeclRef) E) {                DISPATCH(Expr, Expr); }
  RetTy visitIfExpr(PTR(IfExpr) E) {                  DISPATCH(Expr, Expr); }
  RetTy visitIndexExpr(PTR(IndexExpr) E) {            DISPATCH(Expr, Expr); }
  RetTy visitListExpr(PTR(ListExpr) E) {              DISPATCH(Expr, Expr); }
  RetTy visitLiteral(PTR(Literal) E) {                visit(E); }
  RetTy visitBooleanLiteral(PTR(BooleanLiteral) E) {  DISPATCH(Expr, Literal); }
  RetTy visitIntegerLiteral(PTR(IntegerLiteral) E) {  DISPATCH(Expr, Literal); }
  RetTy visitNoneLiteral(PTR(NoneLiteral) E) {        DISPATCH(Expr, Literal); }
  RetTy visitStringLiteral(PTR(StringLiteral) E) {    DISPATCH(Expr, Literal); }
  RetTy visitMemberExpr(PTR(MemberExpr) E) {          DISPATCH(Expr, Expr); }
  RetTy visitMethodCallExpr(PTR(MethodCallExpr) E) {  DISPATCH(Expr, Expr); }
  RetTy visitUnaryExpr(PTR(UnaryExpr) E) {            DISPATCH(Expr, Expr); }
  RetTy visitExpr(PTR(Expr) E) {                      return RetTy(); }
  // clang-format on

#undef PTR
#undef DISPATCH
};
} // namespace exprvisitor

template <typename Impl, typename RetTy = void>
class ExprVisitor : public exprvisitor::Base<std::add_pointer, Impl, RetTy> {};

template <typename Impl, typename RetTy = void>
class ConstExprVisitor
    : public exprvisitor::Base<llvm::make_const_ptr, Impl, RetTy> {};
} // namespace chocopy

#endif // CHOCOPY_LLVM_AST_EXPRVISITOR_H
