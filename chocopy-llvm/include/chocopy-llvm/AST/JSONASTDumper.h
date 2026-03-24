#ifndef CHOCOPY_LLVM_AST_JSONASTDUMPER_H
#define CHOCOPY_LLVM_AST_JSONASTDUMPER_H

#include "chocopy-llvm/AST/AST.h"
#include "chocopy-llvm/AST/ASTNodeTraverser.h"
#include "chocopy-llvm/Basic/LLVM.h"

#include <llvm/Support/JSON.h>
#include <llvm/Support/raw_ostream.h>

namespace chocopy {
class JSONNodeDumper : public ConstDeclVisitor<JSONNodeDumper>,
                       public ConstStmtVisitor<JSONNodeDumper>,
                       public ConstExprVisitor<JSONNodeDumper>,
                       public ConstTypeAnnotationVisitor<JSONNodeDumper>,
                       public ConstTypeVisitor<JSONNodeDumper> {
public:
  JSONNodeDumper(ASTContext &Ctx) : Ctx(Ctx) {}

public:
  template <typename Fn> void addChildArray(Fn DoAddChild) {
    addChildArray(StringRef{}, DoAddChild);
  }
  template <typename Fn> void addChildObject(Fn DoAddChild) {
    addChildObject(StringRef{}, DoAddChild);
  }

  template <typename Fn> void addChildArray(StringRef Label, Fn DoAddChild) {
    JOS.attributeBegin(Label);
    JOS.arrayBegin();
    DoAddChild();
    JOS.arrayEnd();
    JOS.attributeEnd();
  }

  template <typename Fn> void addChildObject(StringRef Label, Fn DoAddChild) {
    if (TopLevel) {
      TopLevel = false;
      JOS.objectBegin();
      DoAddChild();
      JOS.objectEnd();
      TopLevel = true;
      return;
    }

    if (!Label.empty())
      JOS.attributeBegin(Label);
    else
      JOS.objectBegin();

    DoAddChild();

    if (!Label.empty())
      JOS.attributeEnd();
    else
      JOS.objectEnd();
  }

public:
  void visit(const Program *P);
  void visit(const Declaration *D);
  void visit(const Stmt *S);
  void visit(const Expr *E);
  void visit(const Literal *L);
  void visit(const TypeAnnotation *T);
  void visit(const Type *T);
  void visit(const Identifier *I);

  void visitClassDef(const ClassDef *C);
  void visitFundDef(const FuncDef *F);
  void visitClassType(const ClassType *C);
  void visitBinaryExpr(const BinaryExpr *B);
  void visitCallExpr(const CallExpr *C);
  void visitDeclRef(const DeclRef *R);
  void visitBooleanLiteral(const BooleanLiteral *B);
  void visitIntegerLiteral(const IntegerLiteral *I);
  void visitStringLiteral(const StringLiteral *S);
  void visitUnaryExpr(const UnaryExpr *U);

  void visitFuncType(const FuncType *F);
  void visitClassValueType(const ClassValueType *C);
  void visitListValueType(const ListValueType *L);

private:
  void writeLocation(llvm::SMRange Loc);

private:
  ASTContext &Ctx;
  llvm::json::OStream JOS = llvm::json::OStream(llvm::outs(), 2);
  bool TopLevel = true;
};

class JSONDumper : public ASTNodeTraverser<JSONDumper, JSONNodeDumper> {
public:
  JSONDumper(ASTContext &Ctx) : NodeDumper(Ctx) {}

  JSONNodeDumper &doGetNodeDelegate() { return NodeDumper; }

private:
  JSONNodeDumper NodeDumper;
};
} // namespace chocopy
#endif // CHOCOPY_LLVM_AST_JSONASTDUMPER_H
