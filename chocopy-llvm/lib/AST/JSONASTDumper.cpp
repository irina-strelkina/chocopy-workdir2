#include "chocopy-llvm/AST/JSONASTDumper.h"
#include "chocopy-llvm/AST/ASTContext.h"

namespace chocopy {
using llvm::SMRange;

void JSONNodeDumper::visit(const Program *P) {
  JOS.attribute("kind", "Program");
}

void JSONNodeDumper::visit(const Declaration *D) {

  switch (D->getKind()) {
#define DECL(CLASS, KIND)                                                      \
  case Declaration::DeclKind::KIND:                                            \
    JOS.attribute("kind", #KIND);                                              \
    break;
#include "chocopy-llvm/AST/DeclarationNodes.def"
  }
  writeLocation(D->getLocation());
  ConstDeclVisitor<JSONNodeDumper>::visit(D);
}

void JSONNodeDumper::visit(const TypeAnnotation *T) {

  switch (T->getKind()) {
#define TYPE_ANNOTATION(CLASS, KIND)                                           \
  case TypeAnnotation::Kind::KIND:                                             \
    JOS.attribute("kind", #CLASS);                                             \
    break;
#include "chocopy-llvm/AST/TypeAnnotationNodes.def"
  }

  writeLocation(T->getLocation());
  ConstTypeAnnotationVisitor<JSONNodeDumper>::visit(T);
}

void JSONNodeDumper::visit(const Type *T) {
  ConstTypeVisitor<JSONNodeDumper>::visit(T);
}

void JSONNodeDumper::visit(const Identifier *I) {
  JOS.attribute("kind", "Identifier");
  writeLocation(I->getLocation());
  JOS.attribute("name", I->getName());
}

void JSONNodeDumper::visit(const Stmt *S) {
  switch (S->getKind()) {
#define STMT(CLASS, KIND)                                                      \
  case Stmt::StmtKind::KIND:                                                   \
    JOS.attribute("kind", #KIND);                                              \
    break;
#include "chocopy-llvm/AST/StmtNodes.def"
  }

  writeLocation(S->getLocation());
  ConstStmtVisitor<JSONNodeDumper>::visit(S);
}

void JSONNodeDumper::visit(const Expr *E) {
  switch (E->getKind()) {
#define EXPR(CLASS, KIND)                                                      \
  case Expr::Kind::KIND:                                                       \
    JOS.attribute("kind", #KIND);                                              \
    break;
#include "chocopy-llvm/AST/ExprNodes.def"
  }

  writeLocation(E->getLocation());
  ConstExprVisitor<JSONNodeDumper>::visit(E);
}

void JSONNodeDumper::visit(const Literal *L) {
  switch (L->getLiteralType()) {
#define LITERAL_EXPR(CLASS, KIND)                                              \
  case Literal::LiteralType::KIND:                                             \
    JOS.attribute("kind", #CLASS);                                             \
    break;
#include "chocopy-llvm/AST/ExprNodes.def"
  }

  writeLocation(L->getLocation());
  ConstExprVisitor<JSONNodeDumper>::visit(L);
}

void JSONNodeDumper::visitClassDef(const ClassDef *C) {
  // C->getSuperClass()
}

void JSONNodeDumper::visitClassType(const ClassType *C) {
  JOS.attribute("className", C->getClassName());
}

void JSONNodeDumper::visitBinaryExpr(const BinaryExpr *B) {
  JOS.attribute("operator", B->getOpKindStr());
}

void JSONNodeDumper::visitCallExpr(const CallExpr *C) {}

void JSONNodeDumper::visitDeclRef(const DeclRef *E) {
  JOS.attribute("name", E->getName());
}

void JSONNodeDumper::visitBooleanLiteral(const BooleanLiteral *B) {
  JOS.attribute("value", B->getValue() ? "true" : "false");
}

void JSONNodeDumper::visitIntegerLiteral(const IntegerLiteral *I) {
  JOS.attribute("value", I->getValue());
}

void JSONNodeDumper::visitStringLiteral(const StringLiteral *S) {
  JOS.attribute("value", S->getValue());
}

void JSONNodeDumper::visitUnaryExpr(const UnaryExpr *U) {
  StringRef Operator;
  switch (U->getOpKind()) {
  case UnaryExpr::OpKind::Minus:
    Operator = "-";
    break;
  case UnaryExpr::OpKind::Not:
    Operator = "not";
    break;
  }
  JOS.attribute("operator", Operator);
}

void JSONNodeDumper::visitFuncType(const FuncType *F) {
  JOS.attribute("kind", "FuncType");
  /// @todo
}

void JSONNodeDumper::visitClassValueType(const ClassValueType *C) {
  JOS.attribute("kind", "ClassValueType");
  JOS.attribute("name", C->getClassName());
}

void JSONNodeDumper::visitListValueType(const ListValueType *L) {
  JOS.attribute("kind", "ListValueType");
}

void JSONNodeDumper::writeLocation(llvm::SMRange Loc) {
  auto Start = Ctx.getSourceMgr().getLineAndColumn(Loc.Start);
  auto End = Ctx.getSourceMgr().getLineAndColumn(Loc.End);
  JOS.attributeBegin("location");
  JOS.arrayBegin();
  JOS.value(Start.first);
  JOS.value(Start.second);
  JOS.value(End.first);
  JOS.value(End.second);
  JOS.arrayEnd();
  JOS.attributeEnd();
}

} // namespace chocopy
