#ifndef CHOCOPY_LLVM_CODEGEN_CODEGENMODULE_H
#define CHOCOPY_LLVM_CODEGEN_CODEGENMODULE_H

#include "chocopy-llvm/Basic/LLVM.h"

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/Support/Allocator.h>

namespace llvm {
class Module;
class PointerType;
class StructType;
class Type;
class Value;
class LLVMContext;
class GlobalVariable;
class Constant;
class Function;
} // namespace llvm

namespace chocopy {
class ASTContext;
class Declaration;
class Type;
class Literal;

namespace codegen {
class CodeGenModule {
public:
  CodeGenModule(ASTContext &C, llvm::Module &M);
  void release();

  void emitDeclaration(Declaration *D);
  llvm::Type *convertType(Type *T);
  llvm::Constant *convertLiteral(Literal *L);

public:
  llvm::StructType *getStrType(int Len);
  // Get Fully-Qualified Name for declaration d.
  StringRef getFQName(Declaration *D);

  llvm::Type *getVoidTy() const { return VoidTy; }
  llvm::Type *getI1Ty() const { return I1Ty; }
  llvm::Type *getI32Ty() const { return I32Ty; }
  llvm::PointerType *getPtrTy() const { return PtrTy; }

  llvm::LLVMContext &getContext() const { return LLVMCtx; }
  llvm::Module &getModule() const { return M; }

  llvm::Function *getAllocFn() const { return ChpyAlloc; }
  llvm::Function *getPrintFn() const { return ChpyPrint; }

  llvm::GlobalVariable *getIntProto() const { return IntProto; }
  llvm::GlobalVariable *getBoolProto() const { return BoolProto; }
  llvm::GlobalVariable *getStrProto() const { return StrProto; }

private:
  void emitBuiltins();

private:
  ASTContext &C;
  llvm::Module &M;
  llvm::LLVMContext &LLVMCtx;
  llvm::Type *VoidTy;
  llvm::Type *I1Ty;
  llvm::Type *I8Ty;
  llvm::Type *I32Ty;
  llvm::PointerType *PtrTy;
  llvm::StructType *ObjTy;
  llvm::StructType *IntTy;
  llvm::StructType *BoolTy;
  llvm::StructType *StrTy;
  llvm::Constant *I32Zero;
  llvm::Function *ChpyAbort;
  llvm::Function *ChpyPrint;
  llvm::Function *ChpyAlloc;
  llvm::GlobalVariable *IntProto;
  llvm::GlobalVariable *BoolProto;
  llvm::GlobalVariable *StrProto;

  llvm::DenseMap<Declaration *, StringRef> FQDeclNames;
  llvm::StringMap<Declaration *, llvm::BumpPtrAllocator> FQNames;
  // llvm::ConstantInt *True;
};
} // namespace codegen
} // namespace chocopy
#endif // CHOCOPY_LLVM_CODEGEN_CODEGENMODULE_H