#ifndef CHOCOPY_LLVM_AST_TEXT_DIAGNOSTICPRINTER_H
#define CHOCOPY_LLVM_AST_TEXT_DIAGNOSTICPRINTER_H

#include "chocopy-llvm/Basic/Diagnostic.h"
#include "chocopy-llvm/Basic/LLVM.h"

namespace chocopy {
class TextDiagnosticPrinter final : public DiagnosticConsumer {
public:
  TextDiagnosticPrinter(SourceMgr &SrcMgr)
      : DiagnosticConsumer(), SrcMgr(SrcMgr) {}

  void handleDiagnostic(const Diagnostic &Diag) override;

private:
  SourceMgr &SrcMgr;
};
} // namespace chocopy

#endif // CHOCOPY_LLVM_AST_TEXT_DIAGNOSTICPRINTER_H
