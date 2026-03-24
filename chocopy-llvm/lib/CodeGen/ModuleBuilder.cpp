#include "chocopy-llvm/CodeGen/ModuleBuilder.h"
#include "chocopy-llvm/AST/AST.h"

namespace chocopy {
std::unique_ptr<llvm::Module>
CodeGenerator::handleProgram(Program *Program, std::string FileName) {
  M.reset(new llvm::Module(FileName, Ctx));
  Builder.reset(new codegen::CodeGenModule(ASTCtx, *M));
  Builder->release();

  // for (Declaration *D : Program->getDeclarations())

  return std::move(M);
}

std::unique_ptr<CodeGenerator> createLLVMCodegen(llvm::LLVMContext &Ctx,
                                                 ASTContext &ASTCtx) {
  return std::make_unique<CodeGenerator>(Ctx, ASTCtx);
}
} // namespace chocopy