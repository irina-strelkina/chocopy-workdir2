#include "chocopy-llvm/AST/AST.h"
#include "chocopy-llvm/AST/ASTContext.h"
#include "chocopy-llvm/AST/JSONASTDumper.h"

namespace chocopy {
void Declaration::dump(ASTContext &C) const {
  JSONDumper Dumper(C);
  Dumper.visit(this);
}
} // namespace chocopy
