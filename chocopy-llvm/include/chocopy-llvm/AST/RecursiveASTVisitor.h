#ifndef CHOCOPY_LLVM_AST_RECURSIVEASTVISITOR_H
#define CHOCOPY_LLVM_AST_RECURSIVEASTVISITOR_H

#include "chocopy-llvm/AST/AST.h"
#include "chocopy-llvm/AST/ASTContext.h"

namespace chocopy {

#define TRY_TO(CALL_EXPR)                                                      \
  do {                                                                         \
    if (!getDerived().CALL_EXPR)                                               \
      return false;                                                            \
  } while (false)

template <typename Derived> class RecursiveASTVisitor {
private:
  Derived &getDerived() { return *static_cast<Derived *>(this); }

public:
  bool walkUpFromProgram(Program *P) { return getDerived().visitProgram(P); }
  bool visitProgram(Program *P) { return true; }

  bool walkUpFromDeclaration(Declaration *D) {
    return getDerived().visitDeclaration(D);
  }
  bool visitDeclaration(Declaration *D) { return true; }

#define DECL(CLASS, KIND)                                                      \
  bool walkUpFrom##CLASS(CLASS *D) {                                           \
    TRY_TO(walkUpFromDeclaration(D));                                          \
    TRY_TO(visit##CLASS(D));                                                   \
    return true;                                                               \
  }                                                                            \
  bool visit##CLASS(CLASS *D) { return true; }
#include "chocopy-llvm/AST/DeclarationNodes.def"

  bool walkUpFromTypeAnnotation(TypeAnnotation *T) {
    return getDerived().visitTypeAnnotation(T);
  }
  bool visitTypeAnnotation(TypeAnnotation *T) { return true; }

#define TYPE_ANNOTATION(CLASS, KIND)                                           \
  bool walkUpFrom##CLASS(CLASS *T) {                                           \
    TRY_TO(walkUpFromTypeAnnotation(T));                                       \
    TRY_TO(visit##CLASS(T));                                                   \
    return true;                                                               \
  }                                                                            \
  bool visit##CLASS(CLASS *T) { return true; }
#include "chocopy-llvm/AST/TypeAnnotationNodes.def"

  bool walkUpFromStmt(Stmt *S) { return getDerived().visitStmt(S); }
  bool visitStmt(Stmt *) { return true; }

#define STMT(CLASS, KIND)                                                      \
  bool walkUpFrom##CLASS(CLASS *S) {                                           \
    TRY_TO(walkUpFromStmt(S));                                                 \
    TRY_TO(visit##CLASS(S));                                                   \
    return true;                                                               \
  }                                                                            \
  bool visit##CLASS(CLASS *S) { return true; }
#include "chocopy-llvm/AST/StmtNodes.def"

  bool walkUpFromExpr(Expr *E) { return getDerived().visitExpr(E); }
  bool visitExpr(Expr *) { return true; }

#define EXPR(CLASS, KIND)                                                      \
  bool walkUpFrom##CLASS(CLASS *E) {                                           \
    TRY_TO(walkUpFromExpr(E));                                                 \
    TRY_TO(visit##CLASS(E));                                                   \
    return true;                                                               \
  }                                                                            \
  bool visit##CLASS(CLASS *E) { return true; }
#define LITERAL_EXPR(CLASS, KIND)                                              \
  bool walkUpFrom##CLASS(CLASS *E) {                                           \
    TRY_TO(walkUpFromLiteral(E));                                              \
    TRY_TO(visit##CLASS(E));                                                   \
    return true;                                                               \
  }                                                                            \
  bool visit##CLASS(CLASS *E) { return true; }
#include "chocopy-llvm/AST/ExprNodes.def"

  bool traverseAST(ASTContext &AST) {
    return getDerived().traverseProgram(AST.getProgram());
  }

  bool traverseProgram(Program *);

  bool traverseDeclaration(Declaration *);
  bool traverseTypeAnnotation(TypeAnnotation *);
  bool traverseStmt(Stmt *);
  bool traverseExpr(Expr *);
  bool traverseLiteral(Literal *);

  bool traverseClassDef(ClassDef *);
  bool traverseFuncDef(FuncDef *);
  bool traverseGlobalDecl(GlobalDecl *);
  bool traverseNonLocalDecl(NonLocalDecl *);
  bool traverseParamDecl(ParamDecl *);
  bool traverseVarDef(VarDef *);

  bool traverseClassType(ClassType *);
  bool traverseListType(ListType *);

  bool traverseAssignStmt(AssignStmt *);
  bool traverseExprStmt(ExprStmt *);
  bool traverseForStmt(ForStmt *);
  bool traverseIfStmt(IfStmt *);
  bool traverseReturnStmt(ReturnStmt *);
  bool traverseWhileStmt(WhileStmt *);

  bool traverseBinaryExpr(BinaryExpr *);
  bool traverseCallExpr(CallExpr *);
  bool traverseDeclRef(DeclRef *);
  bool traverseIfExpr(IfExpr *);
  bool traverseIndexExpr(IndexExpr *);
  bool traverseListExpr(ListExpr *);

  bool traverseBooleanLiteral(BooleanLiteral *);
  bool traverseIntegerLiteral(IntegerLiteral *);
  bool traverseNoneLiteral(NoneLiteral *);
  bool traverseStringLiteral(StringLiteral *);

  bool traverseMemberExpr(MemberExpr *);
  bool traverseMethodCallExpr(MethodCallExpr *);
  bool traverseUnaryExpr(UnaryExpr *);
};

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseProgram(Program *P) {
  TRY_TO(walkUpFromProgram(P));
  for (auto *D : P->getDeclarations())
    TRY_TO(traverseDeclaration(D));

  for (auto *S : P->getStatements())
    TRY_TO(traverseStmt(S));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseDeclaration(Declaration *D) {
  switch (D->getKind()) {
#define DECL(CLASS, KIND)                                                      \
  case Declaration::DeclKind::KIND:                                            \
    TRY_TO(traverse##CLASS(static_cast<CLASS *>(D)));                          \
    break;
#include "chocopy-llvm/AST/DeclarationNodes.def"
  }
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseTypeAnnotation(TypeAnnotation *TA) {
  switch (TA->getKind()) {
#define TYPE_ANNOTATION(CLASS, KIND)                                           \
  case TypeAnnotation::Kind::KIND:                                             \
    TRY_TO(traverse##CLASS(static_cast<CLASS *>(TA)));                         \
    break;
#include "chocopy-llvm/AST/TypeAnnotationNodes.def"
  }
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseStmt(Stmt *S) {
  switch (S->getKind()) {
#define STMT(CLASS, KIND)                                                      \
  case Stmt::StmtKind::KIND:                                                   \
    TRY_TO(traverse##CLASS(static_cast<CLASS *>(S)));                          \
    break;
#include "chocopy-llvm/AST/StmtNodes.def"
  }
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseExpr(Expr *E) {
  switch (E->getKind()) {
#define EXPR(CLASS, KIND)                                                      \
  case Expr::Kind::KIND:                                                       \
    TRY_TO(traverse##CLASS(static_cast<CLASS *>(E)));                          \
    break;
#include "chocopy-llvm/AST/ExprNodes.def"
  }
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseLiteral(Literal *L) {
  switch (L->getLiteralType()) {
#define LITERAL_EXPR(CLASS, KIND)                                              \
  case Literal::LiteralType::KIND:                                             \
    TRY_TO(traverse##CLASS(static_cast<CLASS *>(L)));                          \
    break;
#include "chocopy-llvm/AST/ExprNodes.def"
  }
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseClassDef(ClassDef *C) {
  TRY_TO(walkUpFromClassDef(C));

  for (auto *D : C->getDeclarations())
    TRY_TO(traverseDeclaration(D));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseFuncDef(FuncDef *F) {
  TRY_TO(walkUpFromFuncDef(F));

  for (auto *P : F->getParams())
    TRY_TO(traverseDeclaration(P));

  for (auto *D : F->getDeclarations())
    TRY_TO(traverseDeclaration(D));

  if (auto *T = F->getReturnType())
    TRY_TO(traverseTypeAnnotation(const_cast<TypeAnnotation *>(T)));

  for (auto *S : F->getStatements())
    TRY_TO(traverseStmt(S));

  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseGlobalDecl(GlobalDecl *D) {
  TRY_TO(walkUpFromGlobalDecl(D));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseNonLocalDecl(NonLocalDecl *D) {
  TRY_TO(walkUpFromNonLocalDecl(D));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseParamDecl(ParamDecl *P) {
  TRY_TO(walkUpFromParamDecl(P));
  TRY_TO(traverseTypeAnnotation(const_cast<TypeAnnotation *>(P->getType())));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseVarDef(VarDef *V) {
  TRY_TO(walkUpFromVarDef(V));
  TRY_TO(traverseTypeAnnotation(const_cast<TypeAnnotation *>(V->getType())));
  TRY_TO(traverseLiteral(const_cast<Literal *>(V->getValue())));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseClassType(ClassType *T) {
  TRY_TO(walkUpFromClassType(T));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseListType(ListType *L) {
  TRY_TO(walkUpFromListType(L));
  TRY_TO(traverseTypeAnnotation(
      const_cast<TypeAnnotation *>(L->getElementType())));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseAssignStmt(AssignStmt *A) {
  TRY_TO(walkUpFromAssignStmt(A));
  for (auto *E : A->getTargets())
    TRY_TO(traverseExpr(E));
  TRY_TO(traverseExpr(const_cast<Expr *>(A->getValue())));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseExprStmt(ExprStmt *E) {
  TRY_TO(walkUpFromExprStmt(E));
  TRY_TO(traverseExpr(const_cast<Expr *>(E->getExpr())));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseForStmt(ForStmt *F) {
  TRY_TO(walkUpFromForStmt(F));
  TRY_TO(traverseExpr(const_cast<DeclRef *>(F->getTarget())));
  TRY_TO(traverseExpr(const_cast<Expr *>(F->getIterable())));
  for (auto *S : F->getBody())
    TRY_TO(traverseStmt(S));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseIfStmt(IfStmt *I) {
  TRY_TO(walkUpFromIfStmt(I));
  for (auto *S : I->getThenBody())
    TRY_TO(traverseStmt(S));
  for (auto *S : I->getElseBody())
    TRY_TO(traverseStmt(S));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseReturnStmt(ReturnStmt *R) {
  TRY_TO(walkUpFromReturnStmt(R));
  if (Expr *E = R->getValue())
    TRY_TO(traverseExpr(E));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseWhileStmt(WhileStmt *W) {
  TRY_TO(walkUpFromWhileStmt(W));
  TRY_TO(traverseExpr(const_cast<Expr *>(W->getCondition())));
  for (auto *S : W->getBody())
    TRY_TO(traverseStmt(S));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseBinaryExpr(BinaryExpr *B) {
  TRY_TO(walkUpFromBinaryExpr(B));
  TRY_TO(traverseExpr(const_cast<Expr *>(B->getLeft())));
  TRY_TO(traverseExpr(const_cast<Expr *>(B->getRight())));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseCallExpr(CallExpr *C) {
  TRY_TO(walkUpFromCallExpr(C));
  TRY_TO(traverseExpr(const_cast<Expr *>(C->getFunction())));
  for (auto *E : C->getArgs())
    TRY_TO(traverseExpr(E));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseDeclRef(DeclRef *D) {
  TRY_TO(walkUpFromDeclRef(D));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseIfExpr(IfExpr *I) {
  TRY_TO(walkUpFromIfExpr(I));
  TRY_TO(traverseExpr(I->getCondExpr()));
  TRY_TO(traverseExpr(I->getThenExpr()));
  TRY_TO(traverseExpr(I->getElseExpr()));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseIndexExpr(IndexExpr *I) {
  TRY_TO(walkUpFromIndexExpr(I));
  TRY_TO(traverseExpr(const_cast<Expr *>(I->getList())));
  TRY_TO(traverseExpr(const_cast<Expr *>(I->getIndex())));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseListExpr(ListExpr *L) {
  TRY_TO(walkUpFromListExpr(L));
  for (auto *E : L->getElements())
    TRY_TO(traverseExpr(E));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseBooleanLiteral(BooleanLiteral *B) {
  TRY_TO(walkUpFromBooleanLiteral(B));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseIntegerLiteral(IntegerLiteral *I) {
  TRY_TO(walkUpFromIntegerLiteral(I));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseNoneLiteral(NoneLiteral *N) {
  TRY_TO(walkUpFromNoneLiteral(N));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseStringLiteral(StringLiteral *S) {
  TRY_TO(walkUpFromStringLiteral(S));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseMemberExpr(MemberExpr *M) {
  TRY_TO(walkUpFromMemberExpr(M));
  TRY_TO(traverseExpr(const_cast<Expr *>(M->getObject())));
  TRY_TO(traverseExpr(const_cast<DeclRef *>(M->getMember())));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseMethodCallExpr(MethodCallExpr *M) {
  TRY_TO(walkUpFromMethodCallExpr(M));
  TRY_TO(traverseExpr(M->getMethod()));
  for (auto *E : M->getArgs())
    TRY_TO(traverseExpr(E));
  return true;
}

template <typename Derived>
bool RecursiveASTVisitor<Derived>::traverseUnaryExpr(UnaryExpr *U) {
  TRY_TO(walkUpFromUnaryExpr(U));
  TRY_TO(traverseExpr(const_cast<Expr *>(U->getOperand())));
  return true;
}
} // namespace chocopy
#endif // CHOCOPY_LLVM_AST_RECURSIVEASTVISITOR_H
