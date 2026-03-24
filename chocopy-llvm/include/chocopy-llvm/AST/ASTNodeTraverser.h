#ifndef CHOCOPY_LLVM_AST_ASTNODETRAVERSER_H
#define CHOCOPY_LLVM_AST_ASTNODETRAVERSER_H

#include "chocopy-llvm/AST/DeclVisitor.h"
#include "chocopy-llvm/AST/ExprVisitor.h"
#include "chocopy-llvm/AST/StmtVisitor.h"
#include "chocopy-llvm/AST/TypeAnnotationVisitor.h"
#include "chocopy-llvm/AST/TypeVisitor.h"

namespace chocopy {
template <typename Derived, typename NodeDelegateType>
class ASTNodeTraverser : public ConstDeclVisitor<Derived>,
                         public ConstStmtVisitor<Derived>,
                         public ConstExprVisitor<Derived>,
                         public ConstTypeAnnotationVisitor<Derived>,
                         public ConstTypeVisitor<Derived> {

  NodeDelegateType &getNodeDelegate() {
    return getDerived().doGetNodeDelegate();
  }

  Derived &getDerived() { return *static_cast<Derived *>(this); }

public:
  void visit(const Identifier *I) {
    getNodeDelegate().addChildObject([this, I] { getNodeDelegate().visit(I); });
  }

  void visit(const Program *P) { visitProgram(P); }
  void visit(const Declaration *D) { ConstDeclVisitor<Derived>::visit(D); }

  void visit(const Stmt *S) { ConstStmtVisitor<Derived>::visit(S); }

  void visit(const Expr *E) {
    getNodeDelegate().addChildObject([this, E] {
      ConstExprVisitor<Derived>::visit(E);

      if (Type *InferredType = E->getInferredType()) {
        getNodeDelegate().addChildObject(
            "inferredType", [this, InferredType] { visit(InferredType); });
      }
    });
  }

  void visit(const TypeAnnotation *T) {
    getNodeDelegate().addChildObject(
        [this, T] { ConstTypeAnnotationVisitor<Derived>::visit(T); });
  }

  void visit(const Type *T) {
    getNodeDelegate().addChildObject([this, T] {
      getNodeDelegate().visit(T);
      ConstTypeVisitor<Derived>::visit(T);
    });
  }

  void visitProgram(const Program *P) {
    getNodeDelegate().addChildObject([this, P] {
      getNodeDelegate().visit(P);

      getNodeDelegate().addChildArray("declarations", [this, P] {
        for (auto *D : P->getDeclarations())
          visit(D);
      });

      getNodeDelegate().addChildArray("statements", [this, P] {
        for (auto *S : P->getStatements())
          visit(S);
      });
      /// @todo Support errors!
    });
  }

  void visitClassDef(const ClassDef *C) {
    getNodeDelegate().addChildObject([this, C] {
      getNodeDelegate().visit(C);
      getNodeDelegate().addChildObject("name",
                                       [this, C] { visit(C->getNameId()); });
      getNodeDelegate().addChildObject(
          "superClass", [this, C] { visit(C->getSuperClass()); });
      getNodeDelegate().addChildArray("declarations", [this, C] {
        for (Declaration *D : C->getDeclarations())
          visit(D);
      });
    });
  }

  void visitFuncDef(const FuncDef *F) {
    getNodeDelegate().addChildObject([this, F] {
      getNodeDelegate().visit(F);
      getNodeDelegate().addChildObject("name",
                                       [this, F] { visit(F->getNameId()); });

      getNodeDelegate().addChildArray("params", [this, F] {
        for (auto *A : F->getParams())
          visit(A);
      });

      if (TypeAnnotation *RT = F->getReturnType())
        getNodeDelegate().addChildObject("returnType",
                                         [this, RT] { visit(RT); });

      getNodeDelegate().addChildArray("declarations", [this, F] {
        for (auto *D : F->getDeclarations())
          visit(D);
      });

      getNodeDelegate().addChildArray("statemets", [this, F] {
        for (auto *S : F->getStatements())
          visit(S);
      });
      /// @todo Support errors!
    });
  }

  void visitGlobalDecl(const GlobalDecl *D) {
    getNodeDelegate().addChildObject([this, D] {
      getNodeDelegate().visit(D);
      getNodeDelegate().addChildObject("name",
                                       [this, D] { visit(D->getNameId()); });
    });
  }

  void visitNonLocalDecl(const NonLocalDecl *D) {
    getNodeDelegate().addChildObject([this, D] {
      getNodeDelegate().visit(D);
      getNodeDelegate().addChildObject("name",
                                       [this, D] { visit(D->getNameId()); });
    });
  }

  void visitVarDef(const VarDef *V) {
    getNodeDelegate().addChildObject([this, V] {
      getNodeDelegate().visit(V);
      getNodeDelegate().addChildObject("name",
                                       [this, V] { visit(V->getNameId()); });
      getNodeDelegate().addChildObject("type",
                                       [this, V] { visit(V->getType()); });
      getNodeDelegate().addChildObject("value",
                                       [this, V] { visit(V->getValue()); });
    });
  }

  void visitParamDecl(const ParamDecl *P) {
    getNodeDelegate().addChildObject([this, P] {
      getNodeDelegate().visit(P);
      getNodeDelegate().addChildObject("name",
                                       [this, P] { visit(P->getNameId()); });
      getNodeDelegate().addChildObject("type",
                                       [this, P] { visit(P->getType()); });
    });
  }

  void visitClassType(const ClassType *T) { getNodeDelegate().visit(T); }

  void visitListType(const ListType *T) {
    getNodeDelegate().visit(T);
    getNodeDelegate().addChildObject("elementType",
                                     [this, T] { visit(T->getElementType()); });
  }

  void visitAssignStmt(const AssignStmt *S) {
    getNodeDelegate().addChildObject([this, S] {
      getNodeDelegate().visit(S);

      getNodeDelegate().addChildArray("targets", [this, S] {
        for (auto *E : S->getTargets())
          visit(E);
      });

      getNodeDelegate().addChildObject("value",
                                       [this, S] { visit(S->getValue()); });
    });
  }

  void visitExprStmt(const ExprStmt *S) {
    getNodeDelegate().addChildObject([this, S] {
      getNodeDelegate().visit(S);
      getNodeDelegate().addChildObject("expr",
                                       [this, S] { visit(S->getExpr()); });
    });
  }

  void visitForStmt(const ForStmt *S) {
    getNodeDelegate().addChildObject([this, S] {
      getNodeDelegate().visit(S);

      getNodeDelegate().addChildObject("identifier",
                                       [this, S] { visit(S->getTarget()); });

      getNodeDelegate().addChildObject("iterable",
                                       [this, S] { visit(S->getIterable()); });

      getNodeDelegate().addChildArray("body", [this, S] {
        for (auto *B : S->getBody())
          visit(B);
      });
    });
  }

  void visitIfStmt(const IfStmt *S) {
    getNodeDelegate().addChildObject([this, S] {
      getNodeDelegate().visit(S);

      getNodeDelegate().addChildObject("condition",
                                       [this, S] { visit(S->getCondition()); });

      getNodeDelegate().addChildArray("thenBody", [this, S] {
        for (auto *B : S->getThenBody())
          visit(B);
      });

      getNodeDelegate().addChildArray("elseBody", [this, S] {
        for (auto *B : S->getElseBody())
          visit(B);
      });
    });
  }

  void visitReturnStmt(const ReturnStmt *S) {
    getNodeDelegate().addChildObject([this, S] {
      getNodeDelegate().visit(S);
      if (Expr *E = S->getValue())
        getNodeDelegate().addChildObject("value", [this, E] { visit(E); });
    });
  }

  void visitWhileStmt(const WhileStmt *S) {
    getNodeDelegate().addChildObject([this, S] {
      getNodeDelegate().visit(S);

      getNodeDelegate().addChildObject("condition",
                                       [this, S] { visit(S->getCondition()); });

      getNodeDelegate().addChildArray("body", [this, S] {
        for (auto *B : S->getBody())
          visit(B);
      });
    });
  }

  void visitBinaryExpr(const BinaryExpr *E) {
    getNodeDelegate().addChildObject("left",
                                     [this, E] { visit(E->getLeft()); });
    getNodeDelegate().visit(E);
    getNodeDelegate().addChildObject("right",
                                     [this, E] { visit(E->getRight()); });
  }

  void visitCallExpr(const CallExpr *E) {
    getNodeDelegate().visit(E);

    getNodeDelegate().addChildObject("function",
                                     [this, E] { visit(E->getFunction()); });

    getNodeDelegate().addChildArray("args", [this, E] {
      for (auto *A : E->getArgs())
        visit(A);
    });
  }

  void visitDeclRef(const DeclRef *E) { getNodeDelegate().visit(E); }

  void visitIfExpr(const IfExpr *E) {
    getNodeDelegate().visit(E);

    getNodeDelegate().addChildObject("condition",
                                     [this, E] { visit(E->getCondExpr()); });

    getNodeDelegate().addChildObject("thenExpr",
                                     [this, E] { visit(E->getThenExpr()); });

    getNodeDelegate().addChildObject("elseExpr",
                                     [this, E] { visit(E->getElseExpr()); });
  }

  void visitIndexExpr(const IndexExpr *E) {
    getNodeDelegate().visit(E);
    getNodeDelegate().addChildObject("list",
                                     [this, E] { visit(E->getList()); });
    getNodeDelegate().addChildObject("index",
                                     [this, E] { visit(E->getIndex()); });
  }

  void visitListExpr(const ListExpr *E) {
    getNodeDelegate().visit(E);
    getNodeDelegate().addChildArray("elements", [this, E] {
      for (auto *El : E->getElements())
        visit(El);
    });
  }

  void visitLiteral(const Literal *E) { getNodeDelegate().visit(E); }

  void visitMemberExpr(const MemberExpr *E) {
    getNodeDelegate().visit(E);
    getNodeDelegate().addChildObject("object",
                                     [this, E] { visit(E->getObject()); });
    getNodeDelegate().addChildObject("member",
                                     [this, E] { visit(E->getMember()); });
  }

  void visitMethodCallExpr(const MethodCallExpr *E) {
    getNodeDelegate().visit(E);

    getNodeDelegate().addChildObject("method",
                                     [this, E] { visit(E->getMethod()); });

    getNodeDelegate().addChildArray("args", [this, E] {
      for (auto *A : E->getArgs())
        visit(A);
    });
  }
  void visitUnaryExpr(const UnaryExpr *E) {
    getNodeDelegate().visit(E);
    getNodeDelegate().addChildObject("operand",
                                     [this, E] { visit(E->getOperand()); });
  }
};
} // namespace chocopy
#endif // CHOCOPY_LLVM_AST_ASTNODETRAVERSER_H
