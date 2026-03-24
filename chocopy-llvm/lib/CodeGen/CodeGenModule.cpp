#include "chocopy-llvm/CodeGen/CodeGenModule.h"
#include "chocopy-llvm/AST/AST.h"
#include "chocopy-llvm/AST/ASTContext.h"
#include "chocopy-llvm/AST/Type.h"
#include "chocopy-llvm/CodeGen/CodeGenFunction.h"

#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/TypeSwitch.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

namespace chocopy {
namespace codegen {
CodeGenModule::CodeGenModule(ASTContext &C, llvm::Module &M)
    : C(C), M(M), LLVMCtx(M.getContext()) {
  VoidTy = llvm::Type::getVoidTy(LLVMCtx);
  I1Ty = llvm::Type::getInt1Ty(LLVMCtx);
  I8Ty = llvm::Type::getInt8Ty(LLVMCtx);
  I32Ty = llvm::Type::getInt32Ty(LLVMCtx);
  PtrTy = llvm::PointerType::getUnqual(LLVMCtx);
  I32Zero = llvm::ConstantInt::get(I32Ty, 0, true);

  emitBuiltins();
}

void CodeGenModule::release() {
  Program *P = C.getProgram();
  CodeGenFunction CGF(*this);
  CGF.emitMain();

  for (Declaration *D : P->getDeclarations())
    emitDeclaration(D);

  for (Stmt *S : P->getStatements())
    CGF.emitStmt(S);
}

void CodeGenModule::emitDeclaration(Declaration *D) {
  VarDef *V = dyn_cast<VarDef>(D);

  assert(V && "Chocopy supports only Variable definitions for now!");

  StringRef Name = getFQName(V);
  ValueType *VT = C.convertAnnotationToVType(V->getType());
  llvm::Type *T = convertType(VT);
  llvm::Constant *Lit = convertLiteral(V->getValue());
  llvm::GlobalVariable *GV = new llvm::GlobalVariable(
      T, false, llvm::GlobalValue::PrivateLinkage, Lit, Name);
  M.insertGlobalVariable(GV);
}

llvm::Type *CodeGenModule::convertType(Type *T) {
  if (auto *V = dyn_cast<ValueType>(T)) {
    if (V->isInt())
      return I32Ty;
    if (V->isBool())
      return I1Ty;
    if (V->isStr())
      return I8Ty;
    return PtrTy;
  }

  llvm::report_fatal_error("Unsupported type! Add support...");
}

llvm::Constant *CodeGenModule::convertLiteral(Literal *L) {
  return llvm::TypeSwitch<Literal *, llvm::Constant *>(L)
      .Case([&](BooleanLiteral *B) {
        return llvm::ConstantInt::getBool(LLVMCtx, B->getValue());
      })
      .Case([&](IntegerLiteral *I) {
        return llvm::ConstantInt::get(I32Ty, I->getValue());
      })
      .Case(
          [&](NoneLiteral *N) { return llvm::ConstantPointerNull::get(PtrTy); })
      .Case([&](StringLiteral *S) {
        llvm::report_fatal_error("Unsupported literal! Add support...");
        return nullptr;
      });
}

llvm::StructType *CodeGenModule::getStrType(int Len) {
  auto *ArrTy = llvm::ArrayType::get(I8Ty, Len);
  return llvm::StructType::get(LLVMCtx, {StrTy, ArrTy}, true);
}

StringRef CodeGenModule::getFQName(Declaration *D) {
  if (auto It = FQDeclNames.find(D); It != FQDeclNames.end())
    return It->getSecond();

  llvm::SmallString<256> Buf;
  llvm::raw_svector_ostream Out(Buf);
  Out << "$" << D->getName();
  auto It = FQNames.insert(std::make_pair(Buf, D));
  return FQDeclNames[D] = It.first->first();
}

void CodeGenModule::emitBuiltins() {
  // class.object { i32 tag, i32 size_in_bytes, ptr dispatch_table }
  ObjTy = llvm::StructType::get(LLVMCtx, {I32Ty, I32Ty, PtrTy}, true);

  // int.object { class.object, i32 value }
  IntTy = llvm::StructType::get(LLVMCtx, {ObjTy, I32Ty}, true);

  // bool.object { class.object, i1 value }
  BoolTy = llvm::StructType::get(LLVMCtx, {ObjTy, I1Ty}, true);

  // trailing raw string
  // str.object { class.object, i32 len } + data + \0
  StrTy = llvm::StructType::get(LLVMCtx, {ObjTy, I32Ty}, true);

  auto *VoidPtrFTy = llvm::FunctionType::get(
      VoidTy, llvm::ArrayRef<llvm::Type *>{PtrTy}, false);
  ChpyAbort = llvm::Function::Create(
      VoidPtrFTy, llvm::GlobalValue::ExternalLinkage, "$abort", &M);
  ChpyPrint = llvm::Function::Create(
      VoidPtrFTy, llvm::GlobalValue::ExternalLinkage, "$print", &M);

  auto *PtrPtrFTy = llvm::FunctionType::get(
      PtrTy, llvm::ArrayRef<llvm::Type *>{PtrTy}, false);
  ChpyAlloc = llvm::Function::Create(
      PtrPtrFTy, llvm::GlobalValue::ExternalLinkage, "$alloc", &M);

  IntProto = new llvm::GlobalVariable(
      IntTy, true, llvm::GlobalValue::ExternalLinkage, nullptr,
      "$int.class.prototype", llvm::GlobalValue::NotThreadLocal, 0, true);
  BoolProto = new llvm::GlobalVariable(
      BoolTy, true, llvm::GlobalValue::ExternalLinkage, nullptr,
      "$bool.class.prototype", llvm::GlobalValue::NotThreadLocal, 0, true);

  StrProto = new llvm::GlobalVariable(
      getStrType(1), true, llvm::GlobalValue::ExternalLinkage, nullptr,
      "$str.class.prototype", llvm::GlobalValue::NotThreadLocal, 0, true);

  M.insertGlobalVariable(IntProto);
  M.insertGlobalVariable(BoolProto);
  M.insertGlobalVariable(StrProto);
}
} // namespace codegen
} // namespace chocopy
