#ifndef CHOCOPY_LLVM_CODEGEN_MODULEBUILDER_H
#define CHOCOPY_LLVM_CODEGEN_MODULEBUILDER_H

#include "chocopy-llvm/CodeGen/CodeGenModule.h"

#include <llvm/IR/Module.h>
#include <llvm/Target/TargetMachine.h>

#include <memory>
#include <string>

namespace chocopy {
class ASTContext;
class Program;

class CodeGenerator {
public:
  CodeGenerator(llvm::LLVMContext &Ctx, ASTContext &ASTCtx)
      : Ctx(Ctx), ASTCtx(ASTCtx) {}

  std::unique_ptr<llvm::Module> handleProgram(Program *Program,
                                              std::string FileName);

private:
  llvm::LLVMContext &Ctx;
  ASTContext &ASTCtx;

  std::unique_ptr<llvm::Module> M;
  std::unique_ptr<codegen::CodeGenModule> Builder;
};

std::unique_ptr<CodeGenerator> createLLVMCodegen(llvm::LLVMContext &Ctx,
                                                 ASTContext &ASTCtx);
} // namespace chocopy
#endif // CHOCOPY_LLVM_CODEGEN_MODULEBUILDER_H