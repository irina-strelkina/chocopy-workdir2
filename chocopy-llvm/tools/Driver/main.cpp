#include "chocopy-llvm/AST/ASTContext.h"
#include "chocopy-llvm/AST/TextDiagnosticPrinter.h"
#include "chocopy-llvm/Analysis/CFG.h"
#include "chocopy-llvm/CodeGen/ModuleBuilder.h"
#include "chocopy-llvm/Lexer/Lexer.h"
#include "chocopy-llvm/Parser/Parser.h"
#include "chocopy-llvm/Sema/Sema.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>

using namespace chocopy;
using namespace llvm;

static cl::opt<bool> RunSemaOpt("run-sema", cl::NotHidden,
                                cl::desc("Run semantics analysis"),
                                cl::init(false));

static cl::opt<bool> AstDumpOpt("ast-dump", cl::NotHidden,
                                cl::desc("Build ASTs and then debug dump them"),
                                cl::init(false));

static cl::opt<bool> CfgDumpOpt("cfg-dump", cl::NotHidden,
                                cl::desc("Display Control-Flow Graphs"),
                                cl::init(false));

static cl::opt<bool> EmitLLVMOpt("emit-llvm", cl::NotHidden,
                                 cl::desc("Build ASTs and convert to LLVM"),
                                 cl::init(false));

static cl::opt<std::string> InputOpt(cl::Positional, cl::desc("<INPUT>"),
                                     cl::init("-"));

static cl::opt<std::string>
    OutputOpt("o", cl::NotHidden, cl::desc("Output LLVM file"), cl::init("-"));

std::unique_ptr<MemoryBuffer> getBufferForFile(StringRef Filename) {
  if (auto Buf = MemoryBuffer::getFile(Filename))
    return std::move(*Buf);
  return nullptr;
}

int main(int argc, char **argv) {

  cl::ParseCommandLineOptions(argc, argv);

  if (InputOpt == "-")
    return 0;

  SourceMgr SrcMgr;
  SrcMgr.AddNewSourceBuffer(getBufferForFile(InputOpt), llvm::SMLoc());

  TextDiagnosticPrinter DiagPrinter(SrcMgr);
  DiagnosticsEngine DiagsEngine(&DiagPrinter);

  ASTContext ASTCtx(SrcMgr);
  Lexer TheLexer(DiagsEngine, SrcMgr);
  TheLexer.reset();

  Sema Actions(TheLexer, ASTCtx);
  Parser TheParser(ASTCtx, TheLexer, Actions);

  ASTCtx.initialize(TheLexer);
  Actions.initialize();
  if (Program *P = TheParser.parse()) {

    if (RunSemaOpt || EmitLLVMOpt)
      Actions.run();

    if (AstDumpOpt) {
      P->dump(ASTCtx);
      llvm::outs() << "\n";
    }

    if (CfgDumpOpt)
      dumpCFG(P);

    if (EmitLLVMOpt) {
      llvm::LLVMContext LLVMCtx;
      auto CodeGen = createLLVMCodegen(LLVMCtx, ASTCtx);
      std::unique_ptr<llvm::Module> M = CodeGen->handleProgram(P, InputOpt);
      if (OutputOpt != "-") {
        std::error_code EC;
        llvm::raw_fd_ostream OS(OutputOpt, EC, llvm::sys::fs::OF_Text);
        OS << *M;
      } else
        M->dump();
    }
  }

  if (int ErrCnt = DiagsEngine.getNumErrors())
    llvm::outs() << ErrCnt << " error" << (ErrCnt == 1 ? "" : "s")
                 << " generated!" << "\n";
  return 0;
}
