#include "chocopy-llvm/CodeGen/CodeGenFunction.h"
#include "chocopy-llvm/AST/AST.h"
#include "chocopy-llvm/AST/Type.h"

#include <llvm/ADT/TypeSwitch.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

namespace chocopy {
namespace codegen {
void CodeGenFunction::emitMain() {
  llvm::LLVMContext &C = CGM.getContext();
  llvm::FunctionType *MainTy = llvm::FunctionType::get(CGM.getVoidTy(), {});
  Fn = llvm::Function::Create(MainTy, llvm::Function::ExternalLinkage, "Main",
                              &CGM.getModule());

  BB = llvm::BasicBlock::Create(C, "Entry", Fn);

  Builder.SetInsertPoint(BB);
  Builder.SetInsertPoint(Builder.CreateRetVoid());
}

void CodeGenFunction::emit() {
  assert(F);
  llvm::report_fatal_error(
      "Functions are not supported by codegen! Add support");
}

void CodeGenFunction::emitDeclaration(Declaration *D) { assert(BB); }

void CodeGenFunction::emitAssignStmt(AssignStmt *A) {
  llvm::Value *V = emitExpr(A->getValue());
  for (Expr *TExpr : A->getTargets()) {
    llvm::Value *T =
        llvm::TypeSwitch<Expr *, llvm::Value *>(TExpr)
            .Case([this](DeclRef *D) { return emitDeclRef(D, false); })
            .Default([](auto) {
              llvm::report_fatal_error("Unsupported target! Add support...");
              return nullptr;
            });
    Builder.CreateStore(V, T);
  }
}

void CodeGenFunction::emitStmt(Stmt *S) {
  assert(BB);
  llvm::TypeSwitch<Stmt *>(S)
      .Case([this](AssignStmt *A) { emitAssignStmt(A); })
      .Case([this](ExprStmt *E) { emitExpr(E->getExpr()); })
      .Default([](auto) {
        llvm::report_fatal_error("Unsupported statement! Add support...");
      });
}

llvm::Value *CodeGenFunction::emitExpr(Expr *E) {
  return llvm::TypeSwitch<Expr *, llvm::Value *>(E)
      .Case([this](DeclRef *D) { return emitDeclRef(D); })
      .Case([this](BinaryExpr *B) { return emitBinaryExpr(B); })
      .Case([this](CallExpr *C) { return emitCallExpr(C); })
      .Case([this](IntegerLiteral *I) { return emitIntLiteral(I); })
      .Default([](auto) {
        llvm::report_fatal_error("Unsupported expression!");
        return nullptr;
      });
}

llvm::Value *CodeGenFunction::emitDeclRef(DeclRef *D, bool LoadVal) {
  llvm::Type *Ty = CGM.convertType(D->getInferredType());
  VarDef *VD = cast<VarDef>(D->getDeclInfo());
  StringRef Name = CGM.getFQName(VD);
  llvm::Value *V = CGM.getModule().getGlobalVariable(Name, true);
  return LoadVal ? Builder.CreateLoad(Ty, V) : V;
}

llvm::Value *CodeGenFunction::emitBinaryExpr(BinaryExpr *B) {
  llvm::Value *L = emitExpr(B->getLeft());
  llvm::Value *R = emitExpr(B->getRight());

  switch (B->getOpKind()) {
  case BinaryExpr::OpKind::And:
    return Builder.CreateAnd(L, R);
  case BinaryExpr::OpKind::Or:
    return Builder.CreateOr(L, R);
  case BinaryExpr::OpKind::Add:
    return Builder.CreateAdd(L, R);
  case BinaryExpr::OpKind::Sub:
    return Builder.CreateSub(L, R);
  case BinaryExpr::OpKind::Mul:
    return Builder.CreateMul(L, R);
  case BinaryExpr::OpKind::FloorDiv:
  case BinaryExpr::OpKind::Mod:
  case BinaryExpr::OpKind::EqCmp:
  case BinaryExpr::OpKind::NEqCmp:
  case BinaryExpr::OpKind::LEqCmp:
  case BinaryExpr::OpKind::GEqCmp:
  case BinaryExpr::OpKind::LCmp:
  case BinaryExpr::OpKind::GCmp:
  case BinaryExpr::OpKind::Is:
    llvm::report_fatal_error("Unsupported binary expression!");
    return nullptr;
  }
}

llvm::Value *CodeGenFunction::emitCallExpr(CallExpr *C) {
  auto *F = cast<DeclRef>(C->getFunction());
  if (F->getName() != "print")
    llvm::report_fatal_error("Call of unsupported function!");

  auto *A = cast<DeclRef>(C->getArgs().front());
  auto *ATy = cast<ClassValueType>(A->getInferredType());

  if (!ATy->isInt())
    llvm::report_fatal_error("Call of unsupported function!");

  llvm::Value *O = emitCreateInitObj(A);
  llvm::Function *PF = CGM.getPrintFn();
  return Builder.CreateCall(PF, O);
}

llvm::Value *CodeGenFunction::emitIntLiteral(IntegerLiteral *I) {
  return Builder.getInt32(I->getValue());
}

llvm::Value *CodeGenFunction::emitCreateInitObj(DeclRef *V) {
  StringRef VName = CGM.getFQName(V->getDeclInfo());
  llvm::Module &M = CGM.getModule();
  llvm::GlobalValue *GV = M.getNamedValue(VName);
  llvm::Function *F = CGM.getAllocFn();
  llvm::GlobalValue *IntProto = CGM.getIntProto();
  llvm::Value *O = Builder.CreateCall(F, IntProto);
  llvm::Value *DstPtr = Builder.CreateStructGEP(IntProto->getValueType(), O, 1);
  llvm::Value *Val = Builder.CreateLoad(GV->getValueType(), GV);
  Builder.CreateStore(Val, DstPtr);
  return O;
}
} // namespace codegen
} // namespace chocopy