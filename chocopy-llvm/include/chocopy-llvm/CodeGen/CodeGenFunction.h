#ifndef CHOCOPY_LLVM_CODEGEN_CODEGENFUNCTION_H
#define CHOCOPY_LLVM_CODEGEN_CODEGENFUNCTION_H

#include "chocopy-llvm/CodeGen/CodeGenModule.h"

#include <llvm/IR/IRBuilder.h>

namespace llvm {
class Function;
class BasicBlock;
class Value;
class Type;
} // namespace llvm

namespace chocopy {
class ASTContext;
class Declaration;
class FuncDef;
class Stmt;
class AssignStmt;
class Expr;
class DeclRef;
class BinaryExpr;
class CallExpr;
class IntegerLiteral;
class ValueType;

namespace codegen {
class CodeGenFunction {
public:
  CodeGenFunction(CodeGenModule &CGM) : CGM(CGM), Builder(CGM.getContext()) {}
  CodeGenFunction(CodeGenModule &CGM, FuncDef *F)
      : CGM(CGM), Builder(CGM.getContext()), F(F) {}

  void emitMain();
  void emit();
  void emitDeclaration(Declaration *D);
  void emitStmt(Stmt *S);
  void emitAssignStmt(AssignStmt *A);

  llvm::Value *emitExpr(Expr *E);
  llvm::Value *emitDeclRef(DeclRef *D, bool LoadVal = true);
  llvm::Value *emitBinaryExpr(BinaryExpr *B);
  llvm::Value *emitCallExpr(CallExpr *C);
  llvm::Value *emitIntLiteral(IntegerLiteral *I);

  llvm::Value *emitCreateInitObj(DeclRef *V);

private:
  CodeGenModule &CGM;
  llvm::IRBuilder<> Builder;
  FuncDef *F = nullptr;
  llvm::Function *Fn;
  llvm::BasicBlock *BB = nullptr;
};
} // namespace codegen
} // namespace chocopy
#endif // CHOCOPY_LLVM_CODEGEN_CODEGENFUNCTION_H