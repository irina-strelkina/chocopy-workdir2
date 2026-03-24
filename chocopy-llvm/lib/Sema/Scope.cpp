#include "chocopy-llvm/Sema/Scope.h"
#include "chocopy-llvm/AST/AST.h"

namespace chocopy {
void Scope::addDecl(Declaration *D) { Decls.insert(D); }
} // namespace chocopy